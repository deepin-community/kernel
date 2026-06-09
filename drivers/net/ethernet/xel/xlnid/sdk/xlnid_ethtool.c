/***************************************************************************************************
	Copyright (C), 2008-2023, XEL Tech. CO., Ltd.
	Filename: xlnid_ethtool.c
	Description: adaption of ethtool
	Function List:
	1. xlnid_get_ts_info           get ptp timestamp info
	2. xlnid_get_link_ksettings get link status of port
	3. xlnid_set_link_ksettings set link status of port
	4. xlnid_get_pauseparam     get flow control info
	5. xlnid_set_pauseparam     set flow control info
	6. xlnid_get_drvinfo           get driver info
	7. xlnid_get_ringparam       get ring descriptor info
	8. xlnid_set_ringparam       set ring descriptor info
	9. xlnid_diag_test           reg、eeprom、interrupt and   loopback testing
	10. xlnid_nway_reset           restart port auto negotiation
	11. xlnid_phys_id             set specified port-led status to active
	12. xlnid_get_coalesce       get the coalesce parameters
	13. xlnid_set_coalesce       set the coalesce parameters
	14. xlnid_set_rxnfc         execute different command of queue classification
	15. xlnid_get_channels       get current channel info
	16. xlnid_set_channels       set new channel info
	History:
	<author>          <time>    <version>   <desc>
	Zhong Yuting    2022/12/17  1.0  create file
***************************************************************************************************/

#include <linux/types.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>

#ifdef SIOCETHTOOL
#include <asm/uaccess.h>

#include "xlnid.h"
#include "xlnid_debug.h"

#ifdef HAVE_ETHTOOL_GET_TS_INFO
#include <linux/net_tstamp.h>
#endif

#ifndef ETH_GSTRING_LEN
#define ETH_GSTRING_LEN 32
#endif

#define XLNID_ALL_RAR_ENTRIES 16

#ifdef HAVE_XDP_SUPPORT
#include <linux/bpf_trace.h>
#endif
#include "xlnid_txrx_common.h"
#ifdef ETHTOOL_OPS_COMPAT
#include "kcompat_ethtool.c"
#endif

extern xel_pci_info_t xel_pci_info[16];
extern xel_pci_info_t *xlnid_find_pci_info(unsigned char bus_number,
		unsigned int devfn);

#ifdef ETHTOOL_GSTATS
struct xlnid_stats {
	char stat_string[ETH_GSTRING_LEN];
	int sizeof_stat;
	int stat_offset;
};

#define XLNID_NETDEV_STAT(_net_stat)                                        \
	{                                                                   \
		.stat_string = #_net_stat,                                  \
		.sizeof_stat =                                              \
			sizeof_field(struct net_device_stats, _net_stat),   \
		.stat_offset = offsetof(struct net_device_stats, _net_stat) \
	}

/* part of the coalesce */
static const struct xlnid_stats xlnid_gstrings_net_stats[] = {
	XLNID_NETDEV_STAT(rx_packets),
	XLNID_NETDEV_STAT(tx_packets),
	XLNID_NETDEV_STAT(rx_bytes),
	XLNID_NETDEV_STAT(tx_bytes),
	XLNID_NETDEV_STAT(rx_errors),
	XLNID_NETDEV_STAT(tx_errors),
	XLNID_NETDEV_STAT(rx_dropped),
	XLNID_NETDEV_STAT(tx_dropped),
	XLNID_NETDEV_STAT(multicast),
	XLNID_NETDEV_STAT(collisions),
	XLNID_NETDEV_STAT(rx_over_errors),
	XLNID_NETDEV_STAT(rx_crc_errors),
	XLNID_NETDEV_STAT(rx_frame_errors),
	XLNID_NETDEV_STAT(rx_fifo_errors),
	XLNID_NETDEV_STAT(rx_missed_errors),
	XLNID_NETDEV_STAT(tx_aborted_errors),
	XLNID_NETDEV_STAT(tx_carrier_errors),
	XLNID_NETDEV_STAT(tx_fifo_errors),
	XLNID_NETDEV_STAT(tx_heartbeat_errors),
};

#define XLNID_STAT(_name, _stat)                                          \
	{                                                                 \
		.stat_string = _name,                                     \
		.sizeof_stat = sizeof_field(struct xlnid_adapter, _stat), \
		.stat_offset = offsetof(struct xlnid_adapter, _stat)      \
	}

/* part of the coalesce */
static struct xlnid_stats xlnid_gstrings_stats[] = {
	XLNID_STAT("rx_pkts_nic", stats.gprc),
	XLNID_STAT("tx_pkts_nic", stats.gptc),
	XLNID_STAT("rx_bytes_nic", stats.gorc),
	XLNID_STAT("tx_bytes_nic", stats.gotc),
	XLNID_STAT("lsc_int", lsc_int),
	XLNID_STAT("tx_busy", tx_busy),
	XLNID_STAT("non_eop_descs", non_eop_descs),
	XLNID_STAT("broadcast", stats.bprc),
	XLNID_STAT("rx_no_buffer_count", stats.rnbc[0]),
	XLNID_STAT("tx_timeout_count", tx_timeout_count),
	XLNID_STAT("tx_restart_queue", restart_queue),
	XLNID_STAT("rx_length_errors", stats.rlec),
	XLNID_STAT("rx_long_length_errors", stats.roc),
	XLNID_STAT("rx_short_length_errors", stats.ruc),
	XLNID_STAT("tx_flow_control_xon", stats.lxontxc),
	XLNID_STAT("rx_flow_control_xon", stats.lxonrxc),
	XLNID_STAT("tx_flow_control_xoff", stats.lxofftxc),
	XLNID_STAT("rx_flow_control_xoff", stats.lxoffrxc),
	XLNID_STAT("rx_csum_offload_errors", hw_csum_rx_error),
	XLNID_STAT("alloc_rx_page", alloc_rx_page),
	XLNID_STAT("alloc_rx_page_failed", alloc_rx_page_failed),
	XLNID_STAT("alloc_rx_buff_failed", alloc_rx_buff_failed),
	XLNID_STAT("rx_no_dma_resources", hw_rx_no_dma_resources),
	XLNID_STAT("hw_rsc_aggregated", rsc_total_count),
	XLNID_STAT("hw_rsc_flushed", rsc_total_flush),

#ifdef HAVE_TX_MQ
	XLNID_STAT("fdir_match", stats.fdirmatch),
	XLNID_STAT("fdir_miss", stats.fdirmiss),
	XLNID_STAT("fdir_overflow", fdir_overflow),
#endif /* HAVE_TX_MQ */

#if IS_ENABLED(CONFIG_FCOE)
	XLNID_STAT("fcoe_bad_fccrc", stats.fccrc),
	XLNID_STAT("fcoe_last_errors", stats.fclast),
	XLNID_STAT("rx_fcoe_dropped", stats.fcoerpdc),
	XLNID_STAT("rx_fcoe_packets", stats.fcoeprc),
	XLNID_STAT("rx_fcoe_dwords", stats.fcoedwrc),
	XLNID_STAT("fcoe_noddp", stats.fcoe_noddp),
	XLNID_STAT("fcoe_noddp_ext_buff", stats.fcoe_noddp_ext_buff),
	XLNID_STAT("tx_fcoe_packets", stats.fcoeptc),
	XLNID_STAT("tx_fcoe_dwords", stats.fcoedwtc),
#endif /* CONFIG_FCOE */

	XLNID_STAT("os2bmc_rx_by_bmc", stats.o2bgptc),
	XLNID_STAT("os2bmc_tx_by_bmc", stats.b2ospc),
	XLNID_STAT("os2bmc_tx_by_host", stats.o2bspc),
	XLNID_STAT("os2bmc_rx_by_host", stats.b2ogprc),

#ifdef HAVE_PTP_1588_CLOCK
	XLNID_STAT("tx_hwtstamp_timeouts", tx_hwtstamp_timeouts),
	XLNID_STAT("tx_hwtstamp_skipped", tx_hwtstamp_skipped),
	XLNID_STAT("rx_hwtstamp_cleared", rx_hwtstamp_cleared),
#endif /* HAVE_PTP_1588_CLOCK */
};

/*  xlnid allocates num_tx_queues and num_rx_queues symmetrically so
		we set the num_rx_queues to evaluate to num_tx_queues. This is
		used because we do not have a good way to get the max number of
		rx queues with CONFIG_RPS disabled.
*/
#ifdef HAVE_TX_MQ

#ifdef HAVE_NETDEV_SELECT_QUEUE
#define XLNID_NUM_RX_QUEUES netdev->num_tx_queues
#define XLNID_NUM_TX_QUEUES netdev->num_tx_queues

#else /* HAVE_NETDEV_SELECT_QUEUE */
#define XLNID_NUM_RX_QUEUES adapter->indices
#define XLNID_NUM_TX_QUEUES adapter->indices

#endif /* HAVE_NETDEV_SELECT_QUEUE */

#else /* HAVE_TX_MQ */
#define XLNID_NUM_TX_QUEUES 1

#define XLNID_NUM_RX_QUEUES \
	(((struct xlnid_adapter *)netdev_priv(netdev))->num_rx_queues)

#endif /* HAVE_TX_MQ */

#define XLNID_QUEUE_STATS_LEN                          \
	((XLNID_NUM_TX_QUEUES + XLNID_NUM_RX_QUEUES) * \
		(sizeof(struct xlnid_queue_stats) / sizeof(u64)))

#define XLNID_GLOBAL_STATS_LEN ARRAY_SIZE(xlnid_gstrings_stats)
#define XLNID_NETDEV_STATS_LEN ARRAY_SIZE(xlnid_gstrings_net_stats)

#define XLNID_PB_STATS_LEN                                       \
	((sizeof(((struct xlnid_adapter *)0)->stats.pxonrxc) +   \
		sizeof(((struct xlnid_adapter *)0)->stats.pxontxc) +   \
		sizeof(((struct xlnid_adapter *)0)->stats.pxoffrxc) +  \
		sizeof(((struct xlnid_adapter *)0)->stats.pxofftxc)) / \
		sizeof(u64))

#define XLNID_VF_STATS_LEN                                          \
	((((struct xlnid_adapter *)netdev_priv(netdev))->num_vfs) * \
		(sizeof(struct vf_stats) / sizeof(u64)))

#define XLNID_STATS_LEN                                    \
	(XLNID_GLOBAL_STATS_LEN + XLNID_NETDEV_STATS_LEN + \
		XLNID_PB_STATS_LEN + XLNID_QUEUE_STATS_LEN + XLNID_VF_STATS_LEN)

#endif /* ETHTOOL_GSTATS */

#ifdef ETHTOOL_TEST
static const char xlnid_gstrings_test[][ETH_GSTRING_LEN] = {
	"Register test  (offline)", "Eeprom test	(offline)",
	"Interrupt test (offline)", "Loopback test  (offline)",
	"Link test   (on/offline)"
};

#define XLNID_TEST_LEN (sizeof(xlnid_gstrings_test) / ETH_GSTRING_LEN)
#endif /* ETHTOOL_TEST */

#ifdef HAVE_ETHTOOL_GET_SSET_COUNT
static const char xlnid_priv_flags_strings[][ETH_GSTRING_LEN] = {
#define XLNID_PRIV_FLAGS_FD_ATR BIT(0)
	"flow-director-atr",

#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC
#define XLNID_PRIV_FLAGS_LEGACY_RX BIT(1)
	"legacy-rx",

#endif /* HAVE_SWIOTLB_SKIP_CPU_SYNC */
};

#define XLNID_PRIV_FLAGS_STR_LEN ARRAY_SIZE(xlnid_priv_flags_strings)
#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */

/* currently supported speeds for 10G */
#define ADVERTISED_MASK_10G                                        \
	(SUPPORTED_10000baseT_Full | SUPPORTED_10000baseKX4_Full | \
		SUPPORTED_10000baseKR_Full)

#define xlnid_isbackplane(type) \
	((type == xlnid_media_type_backplane) ? true : false)

#ifdef ETHTOOL_GLINKSETTINGS

/*
		Function:
		xlnid_get_link_ksettings
		Porpose:
		Get current link information to dump
		ethtool DEVNAME
		Parameters:
		netdev  - (IN) device information
		cmd    - (OUT) current link information
		Returns:
		0        success
		otherwise fail
*/
static int xlnid_get_link_ksettings(struct net_device *netdev,
									struct ethtool_link_ksettings *cmd)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	xlnid_link_speed supported_link;
	bool autoneg = false;
	u32 regval = 0;
	xel_pci_info_t *slave_pinfo = NULL;

	if (hw->ptsw_enable && hw->bus.func == 0 && hw->ptsw_backup) {
		slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
		hw = slave_pinfo->hw;
	}

	ethtool_link_ksettings_zero_link_mode(cmd, supported);
	ethtool_link_ksettings_zero_link_mode(cmd, advertising);

	/* Get link mode */
	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL);

	/* Get link capabilities */
	hw->mac.ops.get_link_capabilities(hw, &supported_link, &autoneg);

#ifdef XLNID_GEPHY_SUPPORT
	regval = XLNID_LINK_MODE_GEPHY;
#endif

#ifdef XLNID_1000BASEX_SUPPORT
	regval = XLNID_LINK_MODE_1000BASEX;
#endif

#ifdef XLNID_FORCE_1000BASEX_SUPPORT
	regval = XLNID_LINK_MODE_1000BASEX;
#endif

	if (hw->device_id == XLNID_DEV_ID_1000BASEX) {
		hw->phy.media_type = xlnid_media_type_fiber;
	}

	if (hw->device_id == XLNID_DEV_ID_SGMII_MAC) {
		hw->phy.media_type = xlnid_media_type_sgmii_mac;
		ethtool_link_ksettings_add_link_mode(cmd, supported, MII);
		cmd->base.port |= PORT_MII;
		goto out;
	}

	if (hw->device_id == XLNID_DEV_ID_SGMII_PHY) {
		hw->phy.media_type = xlnid_media_type_sgmii_phy;
		ethtool_link_ksettings_add_link_mode(cmd, supported, MII);
		cmd->base.port |= PORT_MII;
		goto out;
	}

	/* set the supported link information according to different port type */
	if (hw->phy.media_type == xlnid_media_type_copper) {
		ethtool_link_ksettings_add_link_mode(cmd, supported, TP);
		cmd->base.port |= PORT_TP;
		adapter->hw.phy.media_type = xlnid_media_type_copper;
	}

	else if (hw->phy.media_type == xlnid_media_type_fiber) {
		ethtool_link_ksettings_add_link_mode(cmd, supported, FIBRE);
		cmd->base.port |= PORT_FIBRE;
		adapter->hw.phy.multispeed_fiber = 1;
	}

#ifdef XLNID_SGMII_MAC_SUPPORT
	hw->phy.media_type = xlnid_media_type_sgmii_mac;
	ethtool_link_ksettings_add_link_mode(cmd, supported, MII);
	cmd->base.port |= PORT_MII;
#endif

#ifdef XLNID_SGMII_PHY_SUPPORT
	hw->phy.media_type = xlnid_media_type_sgmii_phy;
	ethtool_link_ksettings_add_link_mode(cmd, supported, MII);
	cmd->base.port |= PORT_MII;
#endif

out:

	if (hw->mac.type == xlnid_mac_WESTLAKE &&
			hw->phy.media_type == xlnid_media_type_fiber) {
		ethtool_link_ksettings_add_link_mode(cmd, supported,
												1000baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
												1000baseT_Full);
		goto check_advertising;
	}

	/*  set the supported link speed and link duplex
		according to the link capability
	*/
	if (supported_link & XLNID_LINK_SPEED_10GB_FULL) {
		ethtool_link_ksettings_add_link_mode(cmd, supported,
												10000baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
												10000baseT_Full);
	}

	if (supported_link & XLNID_LINK_SPEED_1GB_FULL) {
		ethtool_link_ksettings_add_link_mode(cmd, supported,
												1000baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
												1000baseT_Full);
	}

	if (supported_link & XLNID_LINK_SPEED_100_FULL) {
		ethtool_link_ksettings_add_link_mode(cmd, supported,
												100baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
												100baseT_Full);
	}

	if (supported_link & XLNID_LINK_SPEED_100_HALF) {
		ethtool_link_ksettings_add_link_mode(cmd, supported,
												100baseT_Half);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
												100baseT_Half);
	}

	if (supported_link & XLNID_LINK_SPEED_10_FULL) {
		ethtool_link_ksettings_add_link_mode(cmd, supported,
												10baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
												10baseT_Full);
	}

	if (supported_link & XLNID_LINK_SPEED_10_HALF) {
		ethtool_link_ksettings_add_link_mode(cmd, supported,
												10baseT_Half);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
												10baseT_Half);
	}

	/* set the advertised speeds */
	if (hw->phy.autoneg_advertised) {
		ethtool_link_ksettings_zero_link_mode(cmd, advertising);

		if (hw->phy.autoneg_advertised & XLNID_LINK_SPEED_10_HALF)
			ethtool_link_ksettings_add_link_mode(cmd, advertising,
													10baseT_Half);

		if (hw->phy.autoneg_advertised & XLNID_LINK_SPEED_10_FULL)
			ethtool_link_ksettings_add_link_mode(cmd, advertising,
													10baseT_Full);

		if (hw->phy.autoneg_advertised & XLNID_LINK_SPEED_100_HALF)
			ethtool_link_ksettings_add_link_mode(cmd, advertising,
													100baseT_Half);

		if (hw->phy.autoneg_advertised & XLNID_LINK_SPEED_100_FULL)
			ethtool_link_ksettings_add_link_mode(cmd, advertising,
													100baseT_Full);

		if (hw->phy.autoneg_advertised & XLNID_LINK_SPEED_10GB_FULL)
			ethtool_link_ksettings_add_link_mode(cmd, advertising,
													10000baseT_Full);

		if (hw->phy.autoneg_advertised & XLNID_LINK_SPEED_1GB_FULL) {
			if (ethtool_link_ksettings_test_link_mode(
						cmd, supported, 1000baseKX_Full))
				ethtool_link_ksettings_add_link_mode(
					cmd, advertising, 1000baseKX_Full);
			else
				ethtool_link_ksettings_add_link_mode(
					cmd, advertising, 1000baseT_Full);
		}
	}

	else {
		if (hw->phy.multispeed_fiber && !autoneg) {
			if (supported_link & XLNID_LINK_SPEED_10GB_FULL)
				ethtool_link_ksettings_add_link_mode(
					cmd, advertising, 10000baseT_Full);
		}
	}

check_advertising:

	if (autoneg) {
		/* when auto negotiation is supported */
		ethtool_link_ksettings_add_link_mode(cmd, supported, Autoneg);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, Autoneg);
		cmd->base.autoneg = AUTONEG_ENABLE;
	}

	else {
		/* when auto negotiation is not supported */
		cmd->base.autoneg = AUTONEG_DISABLE;
	}

	/*  Determine the remaining settings based on the PHY type.
		Different case refers to different PHY type.
	*/
	switch (adapter->hw.phy.type) {
	case xlnid_phy_tn:
	case xlnid_phy_aq:
	case xlnid_phy_fw:
	case xlnid_phy_cu_unknown:
		ethtool_link_ksettings_add_link_mode(cmd, supported, TP);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, TP);
		cmd->base.port = PORT_TP;
		break;

	case xlnid_phy_qt:
		ethtool_link_ksettings_add_link_mode(cmd, supported, FIBRE);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, FIBRE);
		cmd->base.port = PORT_FIBRE;
		break;

	case xlnid_phy_nl:
	case xlnid_phy_sfp_passive_tyco:
	case xlnid_phy_sfp_passive_unknown:
	case xlnid_phy_sfp_ftl:
	case xlnid_phy_sfp_avago:
	case xlnid_phy_sfp_unknown:
	case xlnid_phy_qsfp_passive_unknown:
	case xlnid_phy_qsfp_active_unknown:
	case xlnid_phy_qsfp_unknown:
		switch (adapter->hw.phy.sfp_type) {
			/* SFP+ devices, further checking needed */
		case xlnid_sfp_type_da_cu:
		case xlnid_sfp_type_da_cu_core0:
		case xlnid_sfp_type_da_cu_core1:
			ethtool_link_ksettings_add_link_mode(cmd, supported,
			FIBRE);
			ethtool_link_ksettings_add_link_mode(cmd, advertising,
			FIBRE);
			cmd->base.port = PORT_DA;
			break;

		case xlnid_sfp_type_sr:
		case xlnid_sfp_type_lr:
		case xlnid_sfp_type_srlr_core0:
		case xlnid_sfp_type_srlr_core1:
		case xlnid_sfp_type_1g_sx_core0:
		case xlnid_sfp_type_1g_sx_core1:
		case xlnid_sfp_type_1g_lx_core0:
		case xlnid_sfp_type_1g_lx_core1:
			ethtool_link_ksettings_add_link_mode(cmd, supported,
			FIBRE);
			ethtool_link_ksettings_add_link_mode(cmd, advertising,
			FIBRE);
			cmd->base.port = PORT_FIBRE;
			break;

		case xlnid_sfp_type_not_present:
			ethtool_link_ksettings_add_link_mode(cmd, supported,
			FIBRE);
			ethtool_link_ksettings_add_link_mode(cmd, advertising,
			FIBRE);
			cmd->base.port = PORT_NONE;
			break;

		case xlnid_sfp_type_1g_cu_core0:
		case xlnid_sfp_type_1g_cu_core1:
			ethtool_link_ksettings_add_link_mode(cmd, supported,
			TP);
			ethtool_link_ksettings_add_link_mode(cmd, advertising,
			TP);
			cmd->base.port = PORT_TP;
			break;

		case xlnid_sfp_type_unknown:
		default:
			ethtool_link_ksettings_add_link_mode(cmd, supported,
			FIBRE);
			ethtool_link_ksettings_add_link_mode(cmd, advertising,
			FIBRE);
			cmd->base.port = PORT_OTHER;
			break;
		}

		break;

	case xlnid_phy_xaui:
		ethtool_link_ksettings_add_link_mode(cmd, supported, FIBRE);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, FIBRE);
		cmd->base.port = PORT_NONE;
		break;

	case xlnid_phy_none:
		break;

	case xlnid_phy_unknown:
	case xlnid_phy_generic:
	case xlnid_phy_sfp_unsupported:
	default:
		ethtool_link_ksettings_add_link_mode(cmd, supported, FIBRE);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, FIBRE);

#ifdef XEL_XGE_SUPPORT
		/* when link speed is up to 10GB */
		cmd->base.port = PORT_FIBRE;
#else
		/* when fibre type is not in the above range */
		cmd->base.port = PORT_OTHER;
#endif /* XEL_XGE_SUPPORT */

		break;
	}

	/* Indicate pause support */
	ethtool_link_ksettings_add_link_mode(cmd, supported, Pause);

	switch (hw->fc.requested_mode) {
	case xlnid_fc_full: /* rx and tx */
		ethtool_link_ksettings_add_link_mode(cmd, advertising, Pause);
		break;

	case xlnid_fc_rx_pause: /* only rx */
		ethtool_link_ksettings_add_link_mode(cmd, advertising, Pause);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
		Asym_Pause);
		break;

	case xlnid_fc_tx_pause: /* only tx */
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
		Asym_Pause);
		break;

	default: /* none of them */
		ethtool_link_ksettings_del_link_mode(cmd, advertising, Pause);
		ethtool_link_ksettings_del_link_mode(cmd, advertising,
		Asym_Pause);
	}

	if (netif_carrier_ok(netdev)) {
	/* Get current port speed and duplex for dump */
		switch (adapter->link_speed) {
		case XLNID_LINK_SPEED_10GB_FULL:
			cmd->base.speed = SPEED_10000;
			cmd->base.duplex = DUPLEX_FULL;
			break;

		case XLNID_LINK_SPEED_5GB_FULL:
			cmd->base.speed = SPEED_5000;
			cmd->base.duplex = DUPLEX_FULL;
			break;

			#ifdef SUPPORTED_2500baseX_Full

		case XLNID_LINK_SPEED_2_5GB_FULL:
			cmd->base.speed = SPEED_2500;
			cmd->base.duplex = DUPLEX_FULL;
			break;
			#endif /* SUPPORTED_2500baseX_Full */

		case XLNID_LINK_SPEED_1GB_FULL:
			cmd->base.speed = SPEED_1000;
			cmd->base.duplex = DUPLEX_FULL;
			break;

		case XLNID_LINK_SPEED_100_FULL:
			cmd->base.speed = SPEED_100;
			cmd->base.duplex = DUPLEX_FULL;
			break;

		case XLNID_LINK_SPEED_100_HALF:
			cmd->base.speed = SPEED_100;
			cmd->base.duplex = DUPLEX_HALF;
			break;

		case XLNID_LINK_SPEED_10_FULL:
			cmd->base.speed = SPEED_10;
			cmd->base.duplex = DUPLEX_FULL;
			break;

		case XLNID_LINK_SPEED_10_HALF:
			cmd->base.speed = SPEED_10;
			cmd->base.duplex = DUPLEX_HALF;
			break;

		default:
			break;
		}

	}

	else {
		/* when current port speed and duplex is not in the above range */
		cmd->base.speed = SPEED_UNKNOWN;
		cmd->base.duplex = DUPLEX_UNKNOWN;
	}

	return 0;
}

#else /* ETHTOOL_GLINKSETTINGS */

/*
		Function:
		xlnid_get_settings
		Porpose:
		Get current link information
		ethtool DEVNAME
		when linux version is less than or equal to 4.7.0
		Parameters:
		netdev  - (IN) device information
		ecmd      - (OUT) current link information
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_get_settings(struct net_device *netdev,
								struct ethtool_cmd *ecmd)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	xlnid_link_speed supported_link;
	bool autoneg = false;

	/* Get link capabilities */
	hw->mac.ops.get_link_capabilities(hw, &supported_link, &autoneg);

	if (hw->device_id == XLNID_DEV_ID_SGMII_MAC) {
		hw->phy.media_type = xlnid_media_type_sgmii_mac;
		ecmd->supported |= SUPPORTED_MII;
		ecmd->port |= PORT_MII;
		goto out;
	}

	if (hw->device_id == XLNID_DEV_ID_SGMII_PHY) {
		hw->phy.media_type = xlnid_media_type_sgmii_phy;
		ecmd->supported |= SUPPORTED_MII;
		ecmd->port |= PORT_MII;
		goto out;
	}

#ifdef XLNID_GEPHY_SUPPORT
	hw->phy.media_type = xlnid_media_type_copper;
#endif

#ifdef XLNID_1000BASEX_SUPPORT
	hw->phy.media_type = xlnid_media_type_fiber;
#endif

#ifdef XLNID_FORCE_1000BASEX_SUPPORT
	hw->phy.media_type = xlnid_media_type_fiber;
#endif

#ifdef XLNID_SGMII_MAC_SUPPORT
	hw->phy.media_type = xlnid_media_type_sgmii_mac;
	ecmd->supported |= SUPPORTED_MII;
	ecmd->port |= PORT_MII;
#endif

#ifdef XLNID_SGMII_PHY_SUPPORT
	hw->phy.media_type = xlnid_media_type_sgmii_phy;
	ecmd->supported |= SUPPORTED_MII;
	ecmd->port |= PORT_MII;
#endif

	if (hw->device_id == XLNID_DEV_ID_1000BASEX) {
		hw->phy.media_type = xlnid_media_type_fiber;
	}

out:

	/* set the supported link information according to different port type */
	if (hw->phy.media_type == xlnid_media_type_copper) {
		ecmd->supported |= SUPPORTED_TP;
		ecmd->port |= PORT_TP;
		ecmd->eth_tp_mdix |= ETH_TP_MDI_X;
		ecmd->eth_tp_mdix_ctrl |= ETH_TP_MDI_AUTO;
	}

	else if (hw->phy.media_type == xlnid_media_type_fiber) {
		ecmd->supported |= SUPPORTED_FIBRE;
		ecmd->port |= PORT_FIBRE;
	}

	if (hw->mac.type == xlnid_mac_WESTLAKE &&
			hw->phy.media_type == xlnid_media_type_fiber) {
		ecmd->supported |= SUPPORTED_1000baseT_Full;
		ecmd->advertising = ecmd->supported;
		goto advertising;
	}

	/* match the supported link speeds */
	if (supported_link & XLNID_LINK_SPEED_10GB_FULL) {
		ecmd->supported |= SUPPORTED_10000baseT_Full;
	}

	if (supported_link & XLNID_LINK_SPEED_1GB_FULL) {
		ecmd->supported |= SUPPORTED_1000baseT_Full;
	}

	if (supported_link & XLNID_LINK_SPEED_100_FULL) {
		ecmd->supported |= SUPPORTED_100baseT_Full;
	}

	if (supported_link & XLNID_LINK_SPEED_100_HALF) {
		ecmd->supported |= SUPPORTED_100baseT_Half;
	}

	if (supported_link & XLNID_LINK_SPEED_10_FULL) {
		ecmd->supported |= SUPPORTED_10baseT_Full;
	}

	if (supported_link & XLNID_LINK_SPEED_10_HALF) {
		ecmd->supported |= SUPPORTED_10baseT_Half;
	}

	/* default advertised speed if phy.autoneg_advertised isn't set */
	ecmd->advertising = ecmd->supported;

	/* set the advertised speeds */
	if (hw->phy.autoneg_advertised) {
		ecmd->advertising = 0;

		if (hw->phy.autoneg_advertised & XLNID_LINK_SPEED_10_HALF) {
			ecmd->advertising |= ADVERTISED_10baseT_Half;
		}

		if (hw->phy.autoneg_advertised & XLNID_LINK_SPEED_10_FULL) {
			ecmd->advertising |= ADVERTISED_10baseT_Full;
		}

		if (hw->phy.autoneg_advertised & XLNID_LINK_SPEED_100_HALF) {
			ecmd->advertising |= ADVERTISED_100baseT_Half;
		}

		if (hw->phy.autoneg_advertised & XLNID_LINK_SPEED_100_FULL) {
			ecmd->advertising |= ADVERTISED_100baseT_Full;
		}

		if (hw->phy.autoneg_advertised & XLNID_LINK_SPEED_10GB_FULL)
			ecmd->advertising |= ecmd->supported &
									ADVERTISED_MASK_10G;

		if (hw->phy.autoneg_advertised & XLNID_LINK_SPEED_1GB_FULL) {
			if (ecmd->supported & SUPPORTED_1000baseKX_Full) {
				ecmd->advertising |= ADVERTISED_1000baseKX_Full;
			}

			else {
				ecmd->advertising |= ADVERTISED_1000baseT_Full;
			}
		}
	}

	else {
		if (hw->phy.multispeed_fiber && !autoneg) {
			if (supported_link & XLNID_LINK_SPEED_10GB_FULL) {
				ecmd->advertising = ADVERTISED_10000baseT_Full;
			}
		}
	}

advertising:

	/* check whether auto negotiation is supported or not */
	if (autoneg) {
		ecmd->supported |= SUPPORTED_Autoneg;
		ecmd->advertising |= ADVERTISED_Autoneg;
		ecmd->autoneg = AUTONEG_ENABLE;
	}

	else {
		ecmd->autoneg = AUTONEG_DISABLE;
	}

	if (hw->phy.media_type == xlnid_media_type_fiber) {
		ecmd->transceiver = XCVR_EXTERNAL;
	}

	else {
		ecmd->transceiver = XCVR_INTERNAL;
	}

	/* Determine the remaining settings based on the PHY type. */
	switch (adapter->hw.phy.type) {
	case xlnid_phy_tn:
	case xlnid_phy_aq:
	case xlnid_phy_fw:
	case xlnid_phy_cu_unknown:
		ecmd->supported |= SUPPORTED_TP;
		ecmd->advertising |= ADVERTISED_TP;
		ecmd->port = PORT_TP;
		break;

	case xlnid_phy_qt:
		ecmd->supported |= SUPPORTED_FIBRE;
		ecmd->advertising |= ADVERTISED_FIBRE;
		ecmd->port = PORT_FIBRE;
		break;

	case xlnid_phy_nl:
	case xlnid_phy_sfp_passive_tyco:
	case xlnid_phy_sfp_passive_unknown:
	case xlnid_phy_sfp_ftl:
	case xlnid_phy_sfp_avago:
	case xlnid_phy_sfp_unknown:
	case xlnid_phy_qsfp_passive_unknown:
	case xlnid_phy_qsfp_active_unknown:
	case xlnid_phy_qsfp_unknown:
		switch (adapter->hw.phy.sfp_type) {
			/* SFP+ devices, further checking needed */
		case xlnid_sfp_type_da_cu:
		case xlnid_sfp_type_da_cu_core0:
		case xlnid_sfp_type_da_cu_core1:
			ecmd->supported |= SUPPORTED_FIBRE;
			ecmd->advertising |= ADVERTISED_FIBRE;
			ecmd->port = PORT_DA;
			break;

		case xlnid_sfp_type_sr:
		case xlnid_sfp_type_lr:
		case xlnid_sfp_type_srlr_core0:
		case xlnid_sfp_type_srlr_core1:
		case xlnid_sfp_type_1g_sx_core0:
		case xlnid_sfp_type_1g_sx_core1:
		case xlnid_sfp_type_1g_lx_core0:
		case xlnid_sfp_type_1g_lx_core1:
			ecmd->supported |= SUPPORTED_FIBRE;
			ecmd->advertising |= ADVERTISED_FIBRE;
			ecmd->port = PORT_FIBRE;
			break;

		case xlnid_sfp_type_not_present:
			ecmd->supported |= SUPPORTED_FIBRE;
			ecmd->advertising |= ADVERTISED_FIBRE;
			ecmd->port = PORT_NONE;
			break;

		case xlnid_sfp_type_1g_cu_core0:
		case xlnid_sfp_type_1g_cu_core1:
			ecmd->supported |= SUPPORTED_TP;
			ecmd->advertising |= ADVERTISED_TP;
			ecmd->port = PORT_TP;
			break;

		case xlnid_sfp_type_unknown:
		default:
			ecmd->supported |= SUPPORTED_FIBRE;
			ecmd->advertising |= ADVERTISED_FIBRE;
			ecmd->port = PORT_OTHER;
			break;
		}

		break;

	case xlnid_phy_xaui:
		ecmd->supported |= SUPPORTED_FIBRE;
		ecmd->advertising |= ADVERTISED_FIBRE;
		ecmd->port = PORT_NONE;
		break;

	case xlnid_phy_none:
		break;

	case xlnid_phy_unknown:
	case xlnid_phy_generic:
	case xlnid_phy_sfp_unsupported:
	default:
		ecmd->supported |= SUPPORTED_FIBRE;
		ecmd->advertising |= ADVERTISED_FIBRE;

#ifdef XEL_XGE_SUPPORT
		ecmd->port = PORT_FIBRE;
#else
		ecmd->port = PORT_OTHER;
#endif
		break;
	}

	/* Indicate pause support */
	ecmd->supported |= SUPPORTED_Pause;

	/* get the flow control type */
	switch (hw->fc.requested_mode) {
	case xlnid_fc_full:
		ecmd->advertising |= ADVERTISED_Pause;
		break;

	case xlnid_fc_rx_pause:
		ecmd->advertising |= ADVERTISED_Pause | ADVERTISED_Asym_Pause;
		break;

	case xlnid_fc_tx_pause:
		ecmd->advertising |= ADVERTISED_Asym_Pause;
		break;

	default:
		ecmd->advertising &=
		~(ADVERTISED_Pause | ADVERTISED_Asym_Pause);
	}

	/* Get current port speed and duplex for dump */
	if (netif_carrier_ok(netdev)) {
		switch (adapter->link_speed) {
		case XLNID_LINK_SPEED_10GB_FULL:
			ethtool_cmd_speed_set(ecmd, SPEED_10000);
			ecmd->duplex = DUPLEX_FULL;
			break;

		case XLNID_LINK_SPEED_5GB_FULL:
			ethtool_cmd_speed_set(ecmd, SPEED_5000);
			ecmd->duplex = DUPLEX_FULL;
			break;

			#ifdef SUPPORTED_2500baseX_Full

		case XLNID_LINK_SPEED_2_5GB_FULL:
			ethtool_cmd_speed_set(ecmd, SPEED_2500);
			ecmd->duplex = DUPLEX_FULL;
			break;
			#endif /* SUPPORTED_2500baseX_Full */

		case XLNID_LINK_SPEED_1GB_FULL:
			ethtool_cmd_speed_set(ecmd, SPEED_1000);
			ecmd->duplex = DUPLEX_FULL;
			break;

		case XLNID_LINK_SPEED_100_FULL:
			ethtool_cmd_speed_set(ecmd, SPEED_100);
			ecmd->duplex = DUPLEX_FULL;
			break;

		case XLNID_LINK_SPEED_100_HALF:
			ethtool_cmd_speed_set(ecmd, SPEED_100);
			ecmd->duplex = DUPLEX_HALF;
			break;

		case XLNID_LINK_SPEED_10_FULL:
			ethtool_cmd_speed_set(ecmd, SPEED_10);
			ecmd->duplex = DUPLEX_FULL;
			break;

		case XLNID_LINK_SPEED_10_HALF:
			ethtool_cmd_speed_set(ecmd, SPEED_10);
			ecmd->duplex = DUPLEX_HALF;
			break;

		default:
			break;
		}

	}

	/* when current port speed and duplex is not in the above range */
	else {
		ethtool_cmd_speed_set(ecmd, SPEED_UNKNOWN);
		ecmd->duplex = DUPLEX_UNKNOWN;
	}

	return 0;
}
#endif /* !ETHTOOL_GLINKSETTINGS */

/* when linux version is greater than 4.7.0 */
#ifdef ETHTOOL_GLINKSETTINGS

/*
		Function:
		xlnid_set_link_ksettings
		Porpose:
		Set current link information according to the command
		ethtool -s DEVNAME
		Parameters:
		netdev  - (IN) device information
		cmd    - (IN) link information needed to be set
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_set_link_ksettings(struct net_device *netdev,
									const struct ethtool_link_ksettings *cmd)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	u32 advertised = 0;
	u32 old = 0;
	s32 err = 0;
	s32 err_backup = 0;
	xel_pci_info_t *slave_pinfo = NULL;

	if (hw->phy.media_type == xlnid_media_type_fiber &&
			hw->mac.type == xlnid_mac_WESTLAKE) {
		if (cmd->base.autoneg == AUTONEG_DISABLE) {
			XLNID_WRITE_REG_MAC(hw, WESTALEK_CFG_AN_EN, 0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x1);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0x64);
			xlnid_write_mac_westlake(hw, WESTLAKE_GMAC_CONFIG,
										0x2F12E8C);
		}

		else {
			XLNID_WRITE_REG_MAC(hw, WESTALEK_CFG_AN_EN, 0x1);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x1);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0x1);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0xE4);
			xlnid_write_mac_westlake(hw, WESTLAKE_GMAC_CONFIG,
										0x2F1268C);

			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_SPEED, 0x2);
		}

		return 0;
	}

	if ((hw->phy.media_type == xlnid_media_type_copper) ||
			(hw->phy.multispeed_fiber) ||
			(hw->phy.media_type == xlnid_media_type_sgmii_phy)) {
		/*
			this function does not support duplex forcing, but can
			limit the advertising of the adapter to the specified speed
		*/
		if (!bitmap_subset(cmd->link_modes.advertising,
							cmd->link_modes.supported,
							__ETHTOOL_LINK_MODE_MASK_NBITS)) {
			return -EINVAL;
		}

		/* only allow one speed at a time if no autoneg */
		if (!cmd->base.autoneg && hw->phy.multispeed_fiber) {
			if (ethtool_link_ksettings_test_link_mode(
						cmd, advertising, 10000baseT_Full) &&
					ethtool_link_ksettings_test_link_mode(
						cmd, advertising, 1000baseT_Full)) {
				return -EINVAL;
			}
		}

		old = hw->phy.autoneg_advertised;
		advertised = 0;

		if (hw->phy.media_type == xlnid_media_type_sgmii_phy) {
			if (cmd->base.speed == SPEED_10 &&
					cmd->base.duplex == DUPLEX_HALF) {
				advertised |= XLNID_LINK_SPEED_10_HALF;
			}

			if (cmd->base.speed == SPEED_10 &&
					cmd->base.duplex == DUPLEX_FULL) {
				advertised |= XLNID_LINK_SPEED_10_FULL;
			}

			if (cmd->base.speed == SPEED_100 &&
					cmd->base.duplex == DUPLEX_HALF) {
				advertised |= XLNID_LINK_SPEED_100_HALF;
			}

			if (cmd->base.speed == SPEED_100 &&
					cmd->base.duplex == DUPLEX_FULL) {
				advertised |= XLNID_LINK_SPEED_100_FULL;
			}

			if (cmd->base.speed == SPEED_1000 &&
					cmd->base.duplex == DUPLEX_HALF) {
				advertised |= XLNID_LINK_SPEED_1GB_HALF;
			}

			if (cmd->base.speed == SPEED_1000 &&
					cmd->base.duplex == DUPLEX_FULL) {
				advertised |= XLNID_LINK_SPEED_1GB_FULL;
			}
		}

		else {
			/* compared with different speed and duplex */
			if (ethtool_link_ksettings_test_link_mode(
						cmd, advertising, 10000baseT_Full)) {
				advertised |= XLNID_LINK_SPEED_10GB_FULL;
			}

			if (ethtool_link_ksettings_test_link_mode(
						cmd, advertising, 1000baseT_Full)) {
				advertised |= XLNID_LINK_SPEED_1GB_FULL;
			}

			if (ethtool_link_ksettings_test_link_mode(
						cmd, advertising, 100baseT_Full)) {
				advertised |= XLNID_LINK_SPEED_100_FULL;
			}

			if (ethtool_link_ksettings_test_link_mode(
						cmd, advertising, 100baseT_Half)) {
				advertised |= XLNID_LINK_SPEED_100_HALF;
			}

			if (ethtool_link_ksettings_test_link_mode(
						cmd, advertising, 10baseT_Full)) {
				advertised |= XLNID_LINK_SPEED_10_FULL;
			}

			if (ethtool_link_ksettings_test_link_mode(
						cmd, advertising, 10baseT_Half)) {
				advertised |= XLNID_LINK_SPEED_10_HALF;
			}
		}

		if (old == advertised) {
			return err;
		}

		/* this sets the link speed and restarts auto-neg */
		while (test_and_set_bit(__XLNID_IN_SFP_INIT, &adapter->state)) {
			usleep_range(1000, 2000);
		}

		hw->mac.autotry_restart = true;

		if (hw->mac.type == xlnid_mac_WESTLAKE && hw->ptsw_enable &&
				hw->bus.func == 0) {
			slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
			err_backup = hw->mac.ops.setup_link(slave_pinfo->hw,
												advertised, true);

			if (!(hw->ptsw_backup)) {
				err = hw->mac.ops.setup_link(hw, advertised,
												true);
			}
		}

		else {
			err = hw->mac.ops.setup_link(hw, advertised, true);
		}

		clear_bit(__XLNID_IN_SFP_INIT, &adapter->state);
	}

	else {
		/* in this case we currently only support 10Gb/FULL */
		u32 speed = cmd->base.speed;

		if ((cmd->base.autoneg == AUTONEG_ENABLE) ||
				(!ethtool_link_ksettings_test_link_mode(cmd, advertising,
						10000baseT_Full)) ||
				(speed + cmd->base.duplex != SPEED_10000 + DUPLEX_FULL)) {
			return -EINVAL;
		}
	}

	return 0;
}

#else

/*
		Function:
		xlnid_set_link_ksettings
		Porpose:
		Set current link information according to the command
		ethtool -s DEVNAME
		when linux version is less than or equal to 4.7.0
		Parameters:
		netdev  - (IN) device information
		cmd    - (IN) link information needed to be set
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_set_settings(struct net_device *netdev,
								struct ethtool_cmd *ecmd)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	u32 advertised = 0;
	u32 old = 0;
	s32 err = 0;
	s32 err_backup = 0;
	xel_pci_info_t *slave_pinfo = NULL;

	if (hw->phy.media_type == xlnid_media_type_fiber &&
			hw->mac.type == xlnid_mac_WESTLAKE) {
		if (ecmd->autoneg == AUTONEG_DISABLE) {
			XLNID_WRITE_REG_MAC(hw, WESTALEK_CFG_AN_EN, 0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x1);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0x64);
			xlnid_write_mac_westlake(hw, WESTLAKE_GMAC_CONFIG,
										0x2F12E8C);
		}

		else {
			XLNID_WRITE_REG_MAC(hw, WESTALEK_CFG_AN_EN, 0x1);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x1);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0x1);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0xE4);
			xlnid_write_mac_westlake(hw, WESTLAKE_GMAC_CONFIG,
										0x2F1268C);

			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_SPEED, 0x2);
		}

		return 0;
	}

	if ((hw->phy.media_type == xlnid_media_type_copper) ||
			(hw->phy.multispeed_fiber)) {
		/*
			this function does not support duplex forcing, but can
			limit the advertising of the adapter to the specified speed
		*/
		if (ecmd->advertising & ~ecmd->supported) {
			return -EINVAL;
		}

		/* only allow one speed at a time if no autoneg */
		if (!ecmd->autoneg && hw->phy.multispeed_fiber) {
			if (ecmd->advertising == (ADVERTISED_10000baseT_Full |
										ADVERTISED_1000baseT_Full)) {
				return -EINVAL;
			}
		}

		old = hw->phy.autoneg_advertised;
		advertised = 0;

		/* compared with different speed and duplex */
		if (ecmd->advertising & ADVERTISED_10000baseT_Full) {
			advertised |= XLNID_LINK_SPEED_10GB_FULL;
		}

		if (ecmd->advertising & ADVERTISED_1000baseT_Full) {
			advertised |= XLNID_LINK_SPEED_1GB_FULL;
		}

		if (ecmd->advertising & ADVERTISED_100baseT_Half) {
			advertised |= XLNID_LINK_SPEED_100_HALF;
		}

		if (ecmd->advertising & ADVERTISED_100baseT_Full) {
			advertised |= XLNID_LINK_SPEED_100_FULL;
		}

		if (ecmd->advertising & ADVERTISED_10baseT_Half) {
			advertised |= XLNID_LINK_SPEED_10_HALF;
		}

		if (ecmd->advertising & ADVERTISED_10baseT_Full) {
			advertised |= XLNID_LINK_SPEED_10_FULL;
		}

		if (old == advertised) {
			return err;
		}

		/* this sets the link speed and restarts auto-neg */
		while (test_and_set_bit(__XLNID_IN_SFP_INIT, &adapter->state)) {
			usleep_range(1000, 2000);
		}

		hw->mac.autotry_restart = true;

		if (hw->mac.type == xlnid_mac_WESTLAKE && hw->ptsw_enable &&
				hw->bus.func == 0) {
			slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
			err_backup = hw->mac.ops.setup_link(slave_pinfo->hw,
												advertised, true);

			if (!(hw->ptsw_backup)) {
				err = hw->mac.ops.setup_link(hw, advertised,
												true);
			}
		}

		else {
			err = hw->mac.ops.setup_link(hw, advertised, true);
		}

		clear_bit(__XLNID_IN_SFP_INIT, &adapter->state);
	}

	else {
		/* in this case we currently only support 10Gb/FULL */
		u32 speed = ethtool_cmd_speed(ecmd);

		if ((ecmd->autoneg == AUTONEG_ENABLE) ||
				(ecmd->advertising != ADVERTISED_10000baseT_Full) ||
				(speed + ecmd->duplex != SPEED_10000 + DUPLEX_FULL)) {
			return -EINVAL;
		}
	}

	return 0;
}
#endif /* !ETHTOOL_GLINKSETTINGS */

/*
		Function:
		xlnid_get_pauseparam
		Porpose:
		Get the current flow control information of
		TX 、RX and auto negotiation
		ethtool -a DEVNAME
		Parameters:
		netdev  - (IN) device information
		pause    - (OUT) current flow control information
		Returns:
		void
*/
static void xlnid_get_pauseparam(struct net_device *netdev,
									struct ethtool_pauseparam *pause)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;

	/*  autoneg to indicate whether the auto-negotiation is supported
		get current autoneg status
	*/
	if (xlnid_device_supports_autoneg_fc(hw) &&
			(!hw->fc.disable_fc_autoneg)) {
		pause->autoneg = 1;
	}

	else {
		pause->autoneg = 0;
	}

	/* get current flow control status of rx and tx */
	if (hw->fc.current_mode == xlnid_fc_rx_pause) {
		pause->rx_pause = 1;
	}

	else if (hw->fc.current_mode == xlnid_fc_tx_pause) {
		pause->tx_pause = 1;
	}

	else if (hw->fc.current_mode == xlnid_fc_full) {
		pause->rx_pause = 1;
		pause->tx_pause = 1;
	}

	else if (hw->fc.current_mode == xlnid_fc_none) {
		pause->tx_pause = 0;
		pause->rx_pause = 0;
	}
}

/*
		Function:
		xlnid_set_pauseparam
		Porpose:
		Set the current flow control information
		according to the command
		ethtool -A DEVNAME
		Parameters:
		netdev   - (IN) device information
		pause        - (IN) current flow control information
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_set_pauseparam(struct net_device *netdev,
								struct ethtool_pauseparam *pause)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	struct xlnid_fc_info fc = hw->fc;

	/* some devices do not support autoneg of flow control */
	if ((pause->autoneg == AUTONEG_ENABLE) &&
			(!xlnid_device_supports_autoneg_fc(hw))) {
		return -EINVAL;
	}

	fc.disable_fc_autoneg = (pause->autoneg != AUTONEG_ENABLE);

	/* Set flow control status according to the command */
	if ((pause->rx_pause && pause->tx_pause) || pause->autoneg) {
		fc.requested_mode = xlnid_fc_full;
	}

	else if ((pause->rx_pause) || pause->tx_pause) {
		if (fc.current_mode == xlnid_fc_full) {
			fc.requested_mode = xlnid_fc_none;
		}

		else if (fc.current_mode == xlnid_fc_none) {
			fc.requested_mode = xlnid_fc_full;
		}
	}

	/* if the thing changed then we'll update and use new autoneg */
	if (memcmp(&fc, &hw->fc, sizeof(struct xlnid_fc_info))) {
		hw->fc = fc;

		if (netif_running(netdev)) {
			xlnid_reinit_locked(adapter);
		}

		else {
			xlnid_reset(adapter);
		}
	}

	return 0;
}

/*
		Function:
		xlnid_get_msglevel
		Porpose:
		Get the current message level
		ethtool DEVNAME
		Parameters:
		netdev   - (IN) device information
		Return:
		msg_enable  current message level
*/
static u32 xlnid_get_msglevel(struct net_device *netdev)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	return adapter->msg_enable;
}

/*
		Function:
		xlnid_set_msglevel
		Porpose:
		Set the message level
		ethtool -s DEVNAME
		Parameters:
		netdev   - (IN) device information
		data       - (OUT) the message level needed to be set
		Return：
		void
*/
static void xlnid_set_msglevel(struct net_device *netdev, u32 data)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	adapter->msg_enable = data;
}

/*
		Function:
		xlnid_get_regs_len
		Porpose:
		Get length of the regs
		Parameters:
		netdev   - (IN) device information
		Return:
		length of the regs matched with type
*/
static int xlnid_get_regs_len(struct net_device __always_unused *netdev)
{
#define XLNID_REGS_LEN 656
	return XLNID_REGS_LEN * sizeof(u32);
}

/* #define XLNID_GET_STAT(_A_, _R_) (_A_->stats._R_) */

/*
		Function:
		xlnid_get_regs
		Porpose:
		Get the value of the regs
		Parameters:
		netdev  - (IN) device information
		regs       - (OUT) read the regs and dump the value
		Return：
		void
*/
static void xlnid_get_regs(struct net_device *netdev, struct ethtool_regs *regs,
							void *p)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	u32 *regs_buff = p;
	u32 addr, val;
	u16 page, phy_data;
	int i;
	int pos = 0;

	memset(p, 0, XLNID_REGS_LEN * sizeof(u32));
	regs->version = hw->mac.type << 24 | hw->revision_id << 16 |
					hw->device_id;

	/* ETHPORT Registers */
	val = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL);
	regs_buff[pos++] = val;
	e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", WESTLAKE_CFG_INTF_CTRL,
				val);

	val = XLNID_READ_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG1);
	regs_buff[pos++] = val;
	e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", WESTLAKE_SGM_PCS_CFG1,
				val);

	val = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_AN_EN);
	regs_buff[pos++] = val;
	e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", WESTLAKE_CFG_AN_EN,
				val);

	val = XLNID_READ_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0);
	regs_buff[pos++] = val;
	e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", WESTLAKE_SGM_PCS_CFG0,
				val);

	val = XLNID_READ_REG_MAC(hw, WESTLAKE_SDS_TX_EQ);
	regs_buff[pos++] = val;
	e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", WESTLAKE_SDS_TX_EQ,
				val);

	val = XLNID_READ_REG_MAC(hw, WESTLAKE_SDS_TXDEEMPH);
	regs_buff[pos++] = val;
	e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", WESTLAKE_SDS_TXDEEMPH,
				val);

	val = XLNID_READ_REG_MAC(hw, WESTLAKE_SDS_RX_EQ0);
	regs_buff[pos++] = val;
	e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", WESTLAKE_SDS_RX_EQ0,
				val);

	val = XLNID_READ_REG_MAC(hw, WESTLAKE_SDS_RX_EQ1);
	regs_buff[pos++] = val;
	e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", WESTLAKE_SDS_RX_EQ1,
				val);

	val = XLNID_READ_REG_MAC(hw, WESTLAKE_SDS_RX_EQ2);
	regs_buff[pos++] = val;
	e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", WESTLAKE_SDS_RX_EQ2,
				val);

	val = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN);
	regs_buff[pos++] = val;
	e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", WESTLAKE_CFG_AN_EDIT_EN,
				val);

	for (i = 0; i < 3; i++) {
		val = XLNID_READ_REG_MAC(hw, WESTLAKE_SDS_OCTL_STATUS0);
		regs_buff[pos++] = val;
		e_dev_info("ETHPORT register(0x%x) = 0x%08x\n",
					WESTLAKE_SDS_OCTL_STATUS0, val);
	}

	for (i = 0; i < 3; i++) {
		val = XLNID_READ_REG_MAC(hw, WESTLAKE_SGM_PCS_STAS0);
		regs_buff[pos++] = val;
		e_dev_info("ETHPORT register(0x%x) = 0x%08x\n",
					WESTLAKE_SGM_PCS_STAS0, val);
	}

	/* GMAC Registers */
	val = xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_CONFIG);
	regs_buff[pos++] = val;
	e_dev_info("GMAC register(0x%x) = 0x%08x\n", WESTLAKE_GMAC_CONFIG, val);

	/* RXCTL Registers */
	for (i = 0; i < 15; i++) {
		addr = WESTLAKE_RX_CTL_BASE + 0x3050 + (4 * i);
		val = XLNID_READ_REG_MAC(hw, addr);
		regs_buff[pos++] = val;
		e_dev_info("RXCTL register(0x%x) = 0x%08x\n", addr, val);
	}

	/* DMA Registers */
	for (i = 0; i < WESTLAKE_MAX_RX_QUEUES; i++) {
		val = XLNID_READ_REG_MAC_DIRECT(hw, WESTLAKE_RDBAL(i));
		regs_buff[pos++] = val;
		e_dev_info("DMA register(0x%x) = 0x%08x\n", WESTLAKE_RDBAL(i),
					val);

		val = XLNID_READ_REG_MAC_DIRECT(hw, WESTLAKE_RDBAH(i));
		regs_buff[pos++] = val;
		e_dev_info("DMA register(0x%x) = 0x%08x\n", WESTLAKE_RDBAH(i),
					val);
	}

	for (i = 0; i < WESTLAKE_MAX_TX_QUEUES; i++) {
		val = XLNID_READ_REG_MAC_DIRECT(hw, WESTLAKE_TDBAL(i));
		regs_buff[pos++] = val;
		e_dev_info("DMA register(0x%x) = 0x%08x\n", WESTLAKE_TDBAL(i),
					val);

		val = XLNID_READ_REG_MAC_DIRECT(hw, WESTLAKE_TDBAH(i));
		regs_buff[pos++] = val;
		e_dev_info("DMA register(0x%x) = 0x%08x\n", WESTLAKE_TDBAH(i),
					val);
	}

	/* PTSW Registers */
	val = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_PTSW_MODE);
	regs_buff[pos++] = val;
	e_dev_info("CRG register(0x%x) = 0x%08x\n", WESTLAKE_CFG_PTSW_MODE,
				val);

	val = XLNID_READ_REG_MAC_DIRECT(hw, 0x4);
	regs_buff[pos++] = val;
	e_dev_info("CFG register(0x4) = 0x%08x\n", val);

	val = XLNID_READ_REG_MAC_DIRECT(hw, WESTLAKE_TX_PTSW);
	regs_buff[pos++] = val;
	e_dev_info("CFG register(0x%x) = 0x%08x\n", WESTLAKE_TX_PTSW, val);

	/* GEPHY Registers */
	i = 0;

	for (page = 0; page < 16; page++) {
		xlnid_write_phy_reg(hw, 0x1f, 0, page);

		for (addr = 0; addr <= 0x1f; addr++) {
			xlnid_read_phy_reg(hw, addr, 0, &phy_data);
			regs_buff[pos++] = phy_data;
			i++;

			e_dev_info("GEPHY page(%d) register(0x%02x) = 0x%04x\n",
						page, addr, phy_data);
		}
	}

	xlnid_write_phy_reg(hw, 0x1f, 0, 0);

	/* ETHCOMM Registers */
	for (i = 1; i <= 42; i++) {
		addr = WESTLAKE_ETH_COMMON_BASE + i * 4;
		val = XLNID_READ_REG_MAC(hw, addr);
		e_dev_info("ETHCOMM register(0x%x) = 0x%08x\n", addr, val);

		regs_buff[pos++] = val;
	}

	/* ETHPORT Registers */
	for (i = 0; i < 2; i++) {
		addr = WESTLAKE_ETH_PORT_BASE + 0x200 + i * 4;
		val = XLNID_READ_REG_MAC(hw, addr);
		regs_buff[pos++] = val;
		e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", addr, val);
	}

	addr = WESTLAKE_ETH_PORT_BASE + 0x214;
	val = XLNID_READ_REG_MAC(hw, addr);
	regs_buff[pos++] = val;
	e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", addr, val);

	for (i = 0; i < 7; i++) {
		addr = WESTLAKE_ETH_PORT_BASE + 0x220 + i * 4;
		val = XLNID_READ_REG_MAC(hw, addr);
		regs_buff[pos++] = val;
		e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", addr, val);
	}

	for (i = 0; i < 4; i++) {
		addr = WESTLAKE_ETH_PORT_BASE + 0x248 + i * 4;
		val = XLNID_READ_REG_MAC(hw, addr);
		regs_buff[pos++] = val;
		e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", addr, val);
	}

	for (i = 0; i < 3; i++) {
		addr = WESTLAKE_ETH_PORT_BASE + 0x260 + i * 4;
		val = XLNID_READ_REG_MAC(hw, addr);
		regs_buff[pos++] = val;
		e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", addr, val);
	}

	for (i = 0; i < 7; i++) {
		addr = WESTLAKE_ETH_PORT_BASE + 0x274 + i * 4;
		val = XLNID_READ_REG_MAC(hw, addr);
		regs_buff[pos++] = val;
		e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", addr, val);
	}

	for (i = 0; i < 2; i++) {
		addr = WESTLAKE_ETH_PORT_BASE + 0x29c + i * 4;
		val = XLNID_READ_REG_MAC(hw, addr);
		regs_buff[pos++] = val;
		e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", addr, val);
	}

	for (i = 0; i < 5; i++) {
		addr = WESTLAKE_ETH_PORT_BASE + 0x2a8 + i * 4;
		val = XLNID_READ_REG_MAC(hw, addr);
		regs_buff[pos++] = val;
		e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", addr, val);
	}

	addr = WESTLAKE_ETH_PORT_BASE + 0x580;
	val = XLNID_READ_REG_MAC(hw, addr);
	regs_buff[pos++] = val;
	e_dev_info("ETHPORT register(0x%x) = 0x%08x\n", addr, val);

	//printk("pos = %d\n", pos);
}

/*
		Function:
		xlnid_get_eeprom_len
		Purpose:
		Get the word size of eeprom
		Parameters:
		netdev  - (IN) device information
		Returns:
		the word size of eeprom
*/
static int xlnid_get_eeprom_len(struct net_device *netdev)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	return adapter->hw.eeprom.word_size * 2;
}

/*
		Function:
		xlnid_get_eeprom
		Purpose:
		Read the buffer through eeprom
		Parameters:
		netdev  - (IN) device information
		eeprom  - (OUT) record the information of eeprom
		Returns:
		result of eeprom-reading
*/
static int xlnid_get_eeprom(struct net_device *netdev,
							struct ethtool_eeprom *eeprom, u8 *bytes)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	u8 *eeprom_buff = NULL;
	int first_byte = 0;
	int last_byte = 0;
	int eeprom_len = 0;
	int ret_val = 0;

	if (eeprom->len == 0) {
		return -EINVAL;
	}

	eeprom->magic = hw->vendor_id | (hw->device_id << 16);

	first_byte = eeprom->offset;
	last_byte = (eeprom->offset + eeprom->len - 1);
	eeprom_len = (last_byte - first_byte + 1);
	eeprom_buff = kmalloc(sizeof(u8) * eeprom_len, GFP_KERNEL);

	if (!eeprom_buff) {
		return -ENOMEM;
	}

	ret_val = hw->eeprom.ops.read_buffer(hw, first_byte, eeprom_len,
											eeprom_buff);

	memcpy(bytes, eeprom_buff, eeprom->len);
	kfree(eeprom_buff);

	return ret_val;
}

/*
		Function:
		xlnid_set_eeprom
		Purpose:
		write the buffer through the eeprom
		Parameters:
		netdev  - (IN) device information
		eeprom  - (IN) the information of eeprom
		bytes    - (IN) data need to be set
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_set_eeprom(struct net_device *netdev,
							struct ethtool_eeprom *eeprom, u8 *bytes)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	int first_byte = 0;
	int last_byte = 0;
	int ret_val = 0;

	if (eeprom->len == 0) {
		return -EINVAL;
	}

	if (hw->device_id == XLNID_DEV_ID_SGMII_MAC ||
			hw->device_id == XLNID_DEV_ID_1000BASEX ||
			hw->device_id == XLNID_DEV_ID_SGMII_PHY) {
		goto out;
	}

	if (eeprom->magic != (hw->vendor_id | (hw->device_id << 16))) {
		return -EINVAL;
	}

out:
	first_byte = eeprom->offset;
	last_byte = (eeprom->offset + eeprom->len - 1);

	ret_val = hw->eeprom.ops.write_buffer(
					hw, first_byte, last_byte - first_byte + 1, bytes);

	return ret_val;
}

/*
		Function:
		xlnid_get_drvinfo
		Porpose:
		Get drive information:name and version
		ethtool -g DEVNAME
		Parameters:
		netdev  - (IN) device information
		ring    - (OUT) current link information
		Return:
		void
*/
static void xlnid_get_drvinfo(struct net_device *netdev,
								struct ethtool_drvinfo *drvinfo)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	/* get driver name */
	strscpy(drvinfo->driver, xlnid_driver_name, sizeof(drvinfo->driver));

	/* get driver version */
	strscpy(drvinfo->version, xlnid_driver_version,
			sizeof(drvinfo->version));

	/* get eeprom_id */
	strscpy(drvinfo->fw_version, "null", sizeof(drvinfo->fw_version));

	/* get eeprom_word_size */
	drvinfo->eedump_len = adapter->hw.eeprom.word_size;

	/* get pci_name */
	strscpy(drvinfo->bus_info, pci_name(adapter->pdev),
			sizeof(drvinfo->bus_info));

#ifdef HAVE_ETHTOOL_GET_SSET_COUNT
	drvinfo->n_priv_flags = XLNID_PRIV_FLAGS_STR_LEN;
#endif
}

/*
		Function:
		xlnid_get_ringparam
		Porpose:
		Get current RX-ring and TX-ring information
		ethtool -g DEVNAME
		Parameters:
		netdev  - (IN) device information
		ring    - (OUT) current link information
		Return:
		void
*/
#ifdef HAVE_ETHTOOL_EXTENDED_RINGPARAMS
static void
xlnid_get_ringparam(struct net_device *netdev, struct ethtool_ringparam *ring,
					struct kernel_ethtool_ringparam __always_unused *ker,
					struct netlink_ext_ack __always_unused *extack)
#else
static void xlnid_get_ringparam(struct net_device *netdev,
								struct ethtool_ringparam *ring)
#endif /* HAVE_ETHTOOL_EXTENDED_RINGPARAMS */
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	ring->rx_max_pending =
		XLNID_MAX_NUM_DESCRIPTORS; /* maximum of rx descriptor */
	ring->tx_max_pending =
		XLNID_MAX_NUM_DESCRIPTORS; /* maximum of tx descriptor */
	ring->rx_mini_max_pending =
		0; /* not support specific rx mini packet queue */
	ring->rx_jumbo_max_pending =
		0; /* not support specific rx jumbo packet queue */
	ring->rx_pending = adapter->rx_ring_count; /* current rx descriptor */
	ring->tx_pending = adapter->tx_ring_count; /* current tx descriptor */
	ring->rx_mini_pending =
		0; /* not support specific tx mini packet queue */
	ring->rx_jumbo_pending =
		0; /* not support specific tx jumbo packet queue */
}

/*
		Function:
		xlnid_set_ringparam
		Porpose:
		Set RX-ring and TX-ring information
		ethtool -G DEVNAME
		Parameters:
		netdev  - (IN) device information
		ring      - (OUT) current link information
		Returns:
		0         success
		otherwise  fail
*/
#ifdef HAVE_ETHTOOL_EXTENDED_RINGPARAMS
static int
xlnid_set_ringparam(struct net_device *netdev, struct ethtool_ringparam *ring,
					struct kernel_ethtool_ringparam __always_unused *ker,
					struct netlink_ext_ack __always_unused *extack)
#else
static int xlnid_set_ringparam(struct net_device *netdev,
								struct ethtool_ringparam *ring)
#endif /* HAVE_ETHTOOL_EXTENDED_RINGPARAMS */
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_ring *temp_ring = NULL;
	int i = 0;
	int j = 0;
	int err = 0;
	u32 new_rx_count = 0;
	u32 new_tx_count = 0;

	if ((ring->rx_mini_pending) || (ring->rx_jumbo_pending)) {
		return -EINVAL;
	}

	if (ring->tx_pending > XLNID_MAX_NUM_DESCRIPTORS ||
			ring->tx_pending < XLNID_MIN_NUM_DESCRIPTORS ||
			ring->rx_pending > XLNID_MAX_NUM_DESCRIPTORS ||
			ring->rx_pending < XLNID_MIN_NUM_DESCRIPTORS) {
		netdev_info(
			netdev,
			"Descriptors requested (Tx: %d / Rx: %d) out of range [%d-%d]\n",
			ring->tx_pending, ring->rx_pending,
			XLNID_MIN_NUM_DESCRIPTORS, XLNID_MAX_NUM_DESCRIPTORS);
		return -EINVAL;
	}

	new_tx_count =
		ALIGN(ring->tx_pending, XLNID_REQ_TX_DESCRIPTOR_MULTIPLE);
	new_rx_count =
		ALIGN(ring->rx_pending, XLNID_REQ_RX_DESCRIPTOR_MULTIPLE);

	if ((new_tx_count == adapter->tx_ring_count) &&
			(new_rx_count == adapter->rx_ring_count)) {
		/* nothing to do */
		return 0;
	}

#ifdef HAVE_AF_XDP_ZC_SUPPORT

	/*  If there is a AF_XDP UMEM attached to any of Rx rings,
		disallow changing the number of descriptors -- regardless
		if the netdev is running or not.
	*/
	if (xlnid_xsk_any_rx_ring_enabled(adapter)) {
		return -EBUSY;
	}

#endif /* HAVE_AF_XDP_ZC_SUPPORT */

	while (test_and_set_bit(__XLNID_RESETTING, &adapter->state)) {
		usleep_range(1000, 2000);
	}

	if (!netif_running(adapter->netdev)) {
		/* Set the descriptor-number of each TX queue */
		for (i = 0; i < adapter->num_tx_queues; i++) {
			adapter->tx_ring[i]->count = new_tx_count;
		}

		/* Set the descriptor-number of each xdp queue */
		for (i = 0; i < adapter->num_xdp_queues; i++) {
			adapter->xdp_ring[i]->count = new_tx_count;
		}

		/* Set the descriptor-number of each RX queue */
		for (i = 0; i < adapter->num_rx_queues; i++) {
			adapter->rx_ring[i]->count = new_rx_count;
		}

		/* Set new descriptor-number */
		adapter->tx_ring_count = new_tx_count;
		adapter->xdp_ring_count = new_tx_count;
		adapter->rx_ring_count = new_rx_count;

		goto clear_reset;
	}

	/* allocate temporary buffer to store rings in */
	i = max_t(int, adapter->num_tx_queues + adapter->num_xdp_queues,
				adapter->num_rx_queues);
	temp_ring = vmalloc(i * sizeof(struct xlnid_ring));

	if (!temp_ring) {
		err = -ENOMEM;
		goto clear_reset;
	}

	xlnid_down(adapter);

	/*
		Setup new Tx resources and free the old Tx resources in that order.
		We can then assign the new resources to the rings via a memcpy.
		The advantage to this approach is that we are guaranteed to still
		have resources even in the case of an allocation failure.
	*/
	if (new_tx_count != adapter->tx_ring_count) {
		for (i = 0; i < adapter->num_tx_queues; i++) {
			memcpy(&temp_ring[i], adapter->tx_ring[i],
					sizeof(struct xlnid_ring));

			temp_ring[i].count = new_tx_count;
			err = xlnid_setup_tx_resources(&temp_ring[i]);

			if (err) {
				while (i) {
					i--;
					xlnid_free_tx_resources(&temp_ring[i]);
				}

				goto err_setup;
			}
		}

		for (j = 0; j < adapter->num_xdp_queues; j++, i++) {
			memcpy(&temp_ring[i], adapter->xdp_ring[j],
					sizeof(struct xlnid_ring));

			temp_ring[i].count = new_tx_count;
			err = xlnid_setup_tx_resources(&temp_ring[i]);

			if (err) {
				while (i) {
					i--;
					xlnid_free_tx_resources(&temp_ring[i]);
				}

				goto err_setup;
			}
		}

		for (i = 0; i < adapter->num_tx_queues; i++) {
			xlnid_free_tx_resources(adapter->tx_ring[i]);

			memcpy(adapter->tx_ring[i], &temp_ring[i],
					sizeof(struct xlnid_ring));
		}

		for (j = 0; j < adapter->num_xdp_queues; j++, i++) {
			xlnid_free_tx_resources(adapter->xdp_ring[j]);

			memcpy(adapter->xdp_ring[j], &temp_ring[i],
					sizeof(struct xlnid_ring));
		}

		adapter->tx_ring_count = new_tx_count;
	}

	/* Repeat the process for the Rx rings if needed */
	if (new_rx_count != adapter->rx_ring_count) {
		for (i = 0; i < adapter->num_rx_queues; i++) {
			memcpy(&temp_ring[i], adapter->rx_ring[i],
					sizeof(struct xlnid_ring));
#ifdef HAVE_XDP_BUFF_RXQ
			/* Clear copied XDP RX-queue info */
			memset(&temp_ring[i].xdp_rxq, 0,
					sizeof(temp_ring[i].xdp_rxq));
#endif

			temp_ring[i].count = new_rx_count;
			err = xlnid_setup_rx_resources(adapter, &temp_ring[i]);

			if (err) {
				while (i) {
					i--;
					xlnid_free_rx_resources(&temp_ring[i]);
				}

				goto err_setup;
			}
		}

		for (i = 0; i < adapter->num_rx_queues; i++) {
			xlnid_free_rx_resources(adapter->rx_ring[i]);

			memcpy(adapter->rx_ring[i], &temp_ring[i],
					sizeof(struct xlnid_ring));
		}

		adapter->rx_ring_count = new_rx_count;
	}

err_setup:
	xlnid_up(adapter);
	vfree(temp_ring);

clear_reset:
	clear_bit(__XLNID_RESETTING, &adapter->state);
	return err;
}

#ifndef HAVE_ETHTOOL_GET_SSET_COUNT

/*
		Function:
		xlnid_get_stats_count
		Porpose:
		Get the total length of features
		Parameters:
		netdev           - (IN) device information
		XLNID_STATS_LEN - (OUT) length of features
		Return:
		the total length of features
*/
static int xlnid_get_stats_count(struct net_device *netdev)
{
	return XLNID_STATS_LEN;
}

#else /* HAVE_ETHTOOL_GET_SSET_COUNT */

/*
		Function:
		xlnid_get_sset_count
		Porpose:
		Get the length of features
		Parameters:
		netdev           - (IN) device information
		XLNID_STATS_LEN - (OUT) length of features
		Return:
		feature length
*/
static int xlnid_get_sset_count(struct net_device *netdev, int sset)
{
#ifdef HAVE_TX_MQ
#ifndef HAVE_NETDEV_SELECT_QUEUE
	struct xlnid_adapter *adapter = netdev_priv(netdev);
#endif
#endif

	switch (sset) {
	case ETH_SS_TEST:
		return XLNID_TEST_LEN;

	case ETH_SS_STATS:
		return XLNID_STATS_LEN;

	case ETH_SS_PRIV_FLAGS:
		return XLNID_PRIV_FLAGS_STR_LEN;

	default:
		return -EOPNOTSUPP;
	}
}

#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */

/*
		Function:
		xlnid_get_ethtool_stats
		Purpose:
		Get RX、TX packets information on different queues
		ethtool -S DEVNAME
		Parameters:
		netdev  - (IN) device information
		stats    - (OUT) the statistics of queue
		data      - (OUT) the value finally put out
		Return:
		void
*/
static void xlnid_get_ethtool_stats(struct net_device *netdev,
									struct ethtool_stats __always_unused *stats,
									u64 *data)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

#ifdef HAVE_NDO_GET_STATS64
	const struct rtnl_link_stats64 *net_stats;
	struct rtnl_link_stats64 temp;
	unsigned int start = 0;
#else
#ifdef HAVE_NETDEV_STATS_IN_NETDEV
	struct net_device_stats *net_stats = &netdev->stats;
#else
	struct net_device_stats *net_stats = &adapter->net_stats;
#endif /* HAVE_NETDEV_STATS_IN_NETDEV */
#endif /* HAVE_NDO_GET_STATS64 */

	u64 *queue_stat = NULL;
	int stat_count = 0;
	int k = 0;
	struct xlnid_ring *ring;
	int i = 0;
	int data_index = 0;
	char *p = 0;

	xlnid_update_stats(adapter);
#ifdef HAVE_NDO_GET_STATS64
	net_stats = dev_get_stats(netdev, &temp);
#endif

	/* get statistics from XLNID_NETDEV_STATS */
	for (i = 0; i < XLNID_NETDEV_STATS_LEN; i++) {
		p = (char *)net_stats + xlnid_gstrings_net_stats[i].stat_offset;
		data[data_index++] = (xlnid_gstrings_net_stats[i].sizeof_stat ==
								sizeof(u64)) ?
								*(u64 *)p :
								*(u32 *)p;
	}

	/* get statistics from XLNID_GLOBAL_STATS */
	for (i = 0; i < XLNID_GLOBAL_STATS_LEN; i++) {
		p = (char *)adapter + xlnid_gstrings_stats[i].stat_offset;
		data[data_index++] =
			(xlnid_gstrings_stats[i].sizeof_stat == sizeof(u64)) ?
			*(u64 *)p :
			*(u32 *)p;
	}

	/* get statistics from TX queues */
	for (i = 0; i < XLNID_NUM_TX_QUEUES; i++) {
		ring = adapter->tx_ring[i];

		if (!ring) {
			data[data_index++] = 0;
			data[data_index++] = 0;

#ifdef BP_EXTENDED_STATS
			data[data_index++] = 0;
			data[data_index++] = 0;
			data[data_index++] = 0;
#endif

			continue;
		}

#ifdef HAVE_NDO_GET_STATS64

		do {
			start = u64_stats_fetch_begin(&ring->syncp);
#endif

			data[data_index] = ring->stats.packets;
			data[data_index + 1] = ring->stats.bytes;

#ifdef HAVE_NDO_GET_STATS64
		} while (u64_stats_fetch_retry(&ring->syncp, start));

#endif

		data_index += 2;

#ifdef BP_EXTENDED_STATS
		data[data_index] = ring->stats.yields;
		data[data_index + 1] = ring->stats.misses;
		data[data_index + 2] = ring->stats.cleaned;
		data_index += 3;
#endif
	}

	/* get statistics from RX queues */
	for (i = 0; i < XLNID_NUM_RX_QUEUES; i++) {
		ring = adapter->rx_ring[i];

		if (!ring) {
			data[data_index++] = 0;
			data[data_index++] = 0;

#ifdef BP_EXTENDED_STATS
			data[data_index++] = 0;
			data[data_index++] = 0;
			data[data_index++] = 0;
#endif

			continue;
		}

#ifdef HAVE_NDO_GET_STATS64

		do {
			start = u64_stats_fetch_begin(&ring->syncp);
#endif

			data[data_index] = ring->stats.packets;
			data[data_index + 1] = ring->stats.bytes;

#ifdef HAVE_NDO_GET_STATS64
		} while (u64_stats_fetch_retry(&ring->syncp, start));

#endif

		data_index += 2;

#ifdef BP_EXTENDED_STATS
		data[data_index] = ring->stats.yields;
		data[data_index + 1] = ring->stats.misses;
		data[data_index + 2] = ring->stats.cleaned;
		data_index += 3;
#endif
	}

	/* get statistics from TX packet buffer */
	for (i = 0; i < XLNID_MAX_PACKET_BUFFERS; i++) {
		data[data_index++] = adapter->stats.pxontxc[i];
		data[data_index++] = adapter->stats.pxofftxc[i];
	}

	/* get statistics from RX packet buffer */
	for (i = 0; i < XLNID_MAX_PACKET_BUFFERS; i++) {
		data[data_index++] = adapter->stats.pxonrxc[i];
		data[data_index++] = adapter->stats.pxoffrxc[i];
	}

	stat_count = sizeof(struct vf_stats) / sizeof(u64);

	/* get statistics from vfs */
	for (i = 0; i < adapter->num_vfs; i++) {
		queue_stat = (u64 *)&adapter->vfinfo[i].vfstats;

		for (k = 0; k < stat_count; k++) {
			data[data_index + k] = queue_stat[k];
		}

		queue_stat = (u64 *)&adapter->vfinfo[i].saved_rst_vfstats;

		for (k = 0; k < stat_count; k++) {
			data[data_index + k] += queue_stat[k];
		}

		data_index += k;
	}
}

/*
		Function:
		xlnid_get_strings
		Purpose:
		print out different statistics
		Parameters:
		netdev     - (IN) device information
		stringset   - (IN) whether to set
		data         - (IN) the value of statistics
		Returns:
		0          success
		otherwise   fail
*/
static void xlnid_get_strings(struct net_device *netdev, u32 stringset,
								u8 *data)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	char *p = (char *)data;
	unsigned int i = 0;

	switch (stringset) {
	case ETH_SS_TEST:
		memcpy(data, *xlnid_gstrings_test,
		XLNID_TEST_LEN * ETH_GSTRING_LEN);
		break;

	case ETH_SS_STATS:
		for (i = 0; i < XLNID_NETDEV_STATS_LEN; i++) {
		memcpy(p, xlnid_gstrings_net_stats[i].stat_string,
		ETH_GSTRING_LEN);
		p += ETH_GSTRING_LEN;
			}

			for (i = 0; i < XLNID_GLOBAL_STATS_LEN; i++) {
				memcpy(p, xlnid_gstrings_stats[i].stat_string,
						ETH_GSTRING_LEN);
				p += ETH_GSTRING_LEN;
			}

			/* get TX packet number of different type */
			for (i = 0; i < XLNID_NUM_TX_QUEUES; i++) {
				snprintf(p, ETH_GSTRING_LEN, "tx_queue_%u_packets", i);
				p += ETH_GSTRING_LEN;

				snprintf(p, ETH_GSTRING_LEN, "tx_queue_%u_bytes", i);
				p += ETH_GSTRING_LEN;

#ifdef BP_EXTENDED_STATS
				snprintf(p, ETH_GSTRING_LEN,
							"tx_queue_%u_bp_napi_yield", i);
				p += ETH_GSTRING_LEN;

				snprintf(p, ETH_GSTRING_LEN, "tx_queue_%u_bp_misses",
							i);
				p += ETH_GSTRING_LEN;

				snprintf(p, ETH_GSTRING_LEN, "tx_queue_%u_bp_cleaned",
							i);
				p += ETH_GSTRING_LEN;
#endif /* BP_EXTENDED_STATS */
			}

			/* get RX packet number of different type */
			for (i = 0; i < XLNID_NUM_RX_QUEUES; i++) {
				snprintf(p, ETH_GSTRING_LEN, "rx_queue_%u_packets", i);
				p += ETH_GSTRING_LEN;

				snprintf(p, ETH_GSTRING_LEN, "rx_queue_%u_bytes", i);
				p += ETH_GSTRING_LEN;

#ifdef BP_EXTENDED_STATS
				snprintf(p, ETH_GSTRING_LEN,
							"rx_queue_%u_bp_poll_yield", i);
				p += ETH_GSTRING_LEN;

				snprintf(p, ETH_GSTRING_LEN, "rx_queue_%u_bp_misses",
							i);
				p += ETH_GSTRING_LEN;

				snprintf(p, ETH_GSTRING_LEN, "rx_queue_%u_bp_cleaned",
							i);
				p += ETH_GSTRING_LEN;
#endif /* BP_EXTENDED_STATS */
			}

			for (i = 0; i < XLNID_MAX_PACKET_BUFFERS; i++) {
				snprintf(p, ETH_GSTRING_LEN, "tx_pb_%u_pxon", i);
				p += ETH_GSTRING_LEN;

				snprintf(p, ETH_GSTRING_LEN, "tx_pb_%u_pxoff", i);
				p += ETH_GSTRING_LEN;
			}

			for (i = 0; i < XLNID_MAX_PACKET_BUFFERS; i++) {
				snprintf(p, ETH_GSTRING_LEN, "rx_pb_%u_pxon", i);
				p += ETH_GSTRING_LEN;

				snprintf(p, ETH_GSTRING_LEN, "rx_pb_%u_pxoff", i);
				p += ETH_GSTRING_LEN;
			}

			for (i = 0; i < adapter->num_vfs; i++) {
				snprintf(p, ETH_GSTRING_LEN, "VF %u Rx Packets", i);
				p += ETH_GSTRING_LEN;

				snprintf(p, ETH_GSTRING_LEN, "VF %u Rx Bytes", i);
				p += ETH_GSTRING_LEN;

				snprintf(p, ETH_GSTRING_LEN, "VF %u Tx Packets", i);
				p += ETH_GSTRING_LEN;

				snprintf(p, ETH_GSTRING_LEN, "VF %u Tx Bytes", i);
				p += ETH_GSTRING_LEN;

				snprintf(p, ETH_GSTRING_LEN, "VF %u MC Packets", i);
				p += ETH_GSTRING_LEN;
			}

			/* BUG_ON(p - data != XLNID_STATS_LEN * ETH_GSTRING_LEN); */
			break;

#ifdef HAVE_ETHTOOL_GET_SSET_COUNT

	case ETH_SS_PRIV_FLAGS:
		memcpy(data, xlnid_priv_flags_strings,
				XLNID_PRIV_FLAGS_STR_LEN * ETH_GSTRING_LEN);
		break;
#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */
	}
}

/*
	Function:
	xlnid_link_test
	Porpose:
	Link-testing
	ethtool -t DEVNAME
	Parameters:
	adapter   - (IN) hardware information
	data        - (OUT) current link information
	Returns:
	0          success
	otherwise   fail
*/
static int xlnid_link_test(struct xlnid_adapter *adapter, u64 *data)
{
	struct xlnid_hw *hw = &adapter->hw;
	bool link_up = false;
	u32 link_speed = 0;

	if (XLNID_REMOVED(hw->hw_addr)) {
		*data = 1;
		return 1;
	}

	*data = 0;

	hw->mac.ops.check_link(hw, &link_speed, &link_up, true);

	if (link_up) {
		return *data;
	}

	else {
		*data = 1;
	}

	return *data;
}

/* ethtool register test data */
struct xlnid_reg_test {
	u32 reg;
	u8 array_len;
	u8 test_type;
	u32 mask;
	u32 write;
};

/*  In the hardware, registers are laid out either singly, in arrays
		spaced 0x40 bytes apart, or in contiguous tables.  We assume
		most tests take place on arrays or single registers (handled
		as a single-element array) and special-case the tables.
		Table tests are always pattern tests.

		We also make provision for some required setup steps by specifying
		registers to be written without any read-back testing.
*/

#define PATTERN_TEST 1
#define SET_READ_TEST 2
#define WRITE_NO_TEST 3
#define TABLE32_TEST 4
#define TABLE64_TEST_LO 5
#define TABLE64_TEST_HI 6

/* default skylake register test */
static struct xlnid_reg_test reg_test_skylake[] = {
	{ SKYLAKE_VLNCTRL, 1, PATTERN_TEST, 0x00000000, 0x00000000 },
	{ SKYLAKE_RDBAL(0), 4, PATTERN_TEST, 0xFFFFFF80, 0xFFFFFFFF },
	{ SKYLAKE_RDBAH(0), 4, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ SKYLAKE_RDLEN(0), 4, PATTERN_TEST, 0x000FFF80, 0x000FFFFF },
	{ SKYLAKE_RXDCTL(0), 4, WRITE_NO_TEST, 0, XLNID_RXDCTL_ENABLE },
	{ SKYLAKE_RDT(0), 4, PATTERN_TEST, 0x0000FFFF, 0x0000FFFF },
	{ SKYLAKE_RXDCTL(0), 4, WRITE_NO_TEST, 0, 0 },
	{ SKYLAKE_TDBAL(0), 4, PATTERN_TEST, 0xFFFFFF80, 0xFFFFFFFF },
	{ SKYLAKE_TDBAH(0), 4, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ SKYLAKE_TDLEN(0), 4, PATTERN_TEST, 0x000FFF80, 0x000FFFFF },
	{ SKYLAKE_RAL(0), 16, TABLE64_TEST_LO, 0xFFFFFFFF, 0xFFFFFFFF },
	{ SKYLAKE_RAL(0), 16, TABLE64_TEST_HI, 0x8001FFFF, 0x800CFFFF },
	{ SKYLAKE_MTA(0), 128, TABLE32_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ .reg = 0 }
};

/* default westlake register test */
static struct xlnid_reg_test reg_test_westlake[] = {
	/* { WESTLAKE_VLNCTRL, 1, PATTERN_TEST, 0x00000000, 0x00000000 }, */
	{ WESTLAKE_RDBAL(0), 4, PATTERN_TEST, 0xFFFFFFE0, 0xFFFFFFFF },
	{ WESTLAKE_RDBAH(0), 4, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ WESTLAKE_RDLEN(0), 4, PATTERN_TEST, 0x000FFF80, 0x000FFFFF },
	{ WESTLAKE_RXDCTL(0), 4, WRITE_NO_TEST, 0, XLNID_RXDCTL_ENABLE },
	{ WESTLAKE_RDT(0), 4, PATTERN_TEST, 0x0000FFFF, 0x0000FFFF },
	{ WESTLAKE_RXDCTL(0), 4, WRITE_NO_TEST, 0, 0 },
	{ WESTLAKE_TDBAL(0), 4, PATTERN_TEST, 0xFFFFFFE0, 0xFFFFFFFF },
	{ WESTLAKE_TDBAH(0), 4, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ WESTLAKE_TDLEN(0), 4, PATTERN_TEST, 0x000FFF80, 0x000FFFFF },
	/*  { WESTLAKE_RAL(0), 16, TABLE64_TEST_LO, 0xFFFFFFFF, 0xFFFFFFFF },
		{ WESTLAKE_RAL(0), 16, TABLE64_TEST_HI, 0x8001FFFF, 0x800CFFFF },
		{ WESTLAKE_MTA(0), 128, TABLE32_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	*/
	{ .reg = 0 }
};

/*
		Function:
		reg_pattern_test
		Porpose:
		Reg-writing test
		ethtool -t DEVNAME
		Parameters:
		adapter - (IN) hardware information
		data       - (OUT) the information of failed reg
		reg     - (IN) reg information
		Returns:
		true    fail to write
		fail    succeed to write
*/
static bool reg_pattern_test(struct xlnid_adapter *adapter, u64 *data, int reg,
								u32 mask, u32 write, bool direct)
{
	u32 pat = 0;
	u32 val = 0;
	u32 before = 0;

	static const u32 test_pattern[] = { 0x5A5A5A5A, 0xA5A5A5A5, 0x00000000,
										0xFFFFFFFF
										};

	if (XLNID_REMOVED(adapter->hw.hw_addr)) {
		*data = 1;
		return true;
	}

	for (pat = 0; pat < ARRAY_SIZE(test_pattern); pat++) {
		/* record the reg value before writing */
		before = xlnid_read_reg(&adapter->hw, reg, false, direct);

		/* write different values to the reg */
		xlnid_write_reg(&adapter->hw, reg, test_pattern[pat] & write,
						direct);

		/* read again to check whether the reg are rewrited */
		val = xlnid_read_reg(&adapter->hw, reg, false, direct);

		if (val != (test_pattern[pat] & write & mask)) {
			e_err(drv,
					"pattern test reg %04X failed: got 0x%08X expected 0x%08X\n",
					reg, val, test_pattern[pat] & write & mask);
			*data = reg;

			/* set the reg with before values */
			xlnid_write_reg(&adapter->hw, reg, before, direct);
			return true;
		}

		/* set the reg with before values */
		xlnid_write_reg(&adapter->hw, reg, before, direct);
	}

	return false;
}

/*
	Function:
	reg_set_and_check
	Porpose:
	Reg-set_and_check test
	ethtool -t DEVNAME
	Parameters:
	adapter - (IN) hardware information
	data       - (OUT) the information of failed reg
	reg     - (IN) reg information
	Returns:
	true    fail to write
	fail    succeed to write
*/
static bool reg_set_and_check(struct xlnid_adapter *adapter, u64 *data, int reg,
								u32 mask, u32 write, bool direct)
{
	u32 val = 0;
	u32 before = 0;

	if (XLNID_REMOVED(adapter->hw.hw_addr)) {
		*data = 1;
		return true;
	}

	/* record the reg value before writing */
	before = xlnid_read_reg(&adapter->hw, reg, false, direct);

	/* write different values to the reg */
	xlnid_write_reg(&adapter->hw, reg, write & mask, direct);

	/* read again to check whether the reg are rewrited */
	val = xlnid_read_reg(&adapter->hw, reg, false, direct);

	if ((write & mask) != (val & mask)) {
		e_err(drv,
				"set/check reg %04X test failed: got 0x%08X expected 0x%08X\n",
				reg, (val & mask), (write & mask));
		*data = reg;

		/* set the reg with before values */
		xlnid_write_reg(&adapter->hw, reg, before, direct);
		return true;
	}

	/* set the reg with before values */
	xlnid_write_reg(&adapter->hw, reg, before, direct);

	return false;
}

/*
		Function:
		xlnid_reg_test
		Porpose:
		Reg-testing
		ethtool -t DEVNAME
		Parameters:
		adapter   - (IN) hardware information
		data      - (OUT) testing result
		Returns:
		true     test fail
		fail     test succeed
*/
static bool xlnid_reg_test(struct xlnid_adapter *adapter, u64 *data)
{
	struct xlnid_reg_test *test = NULL;
	struct xlnid_hw *hw = &adapter->hw;
	u32 i = 0;
	u32 toggle = 0;
	bool direct = true;

	if (XLNID_REMOVED(hw->hw_addr)) {
		e_err(drv, "Adapter removed - register test blocked\n");
		*data = 1;
		return true;
	}

	switch (hw->mac.type) {
	case xlnid_mac_SKYLAKE:
		toggle = 0x7FFFF30F;
		test = reg_test_skylake;
		break;

	case xlnid_mac_WESTLAKE:
		toggle = 0x7FFFF30F;
		test = reg_test_westlake;
		direct = true;
		break;

	default:
		*data = 1;
		return true;
	}

	/*
		Perform the remainder of the register test, looping through
		the test table until we either fail or reach the null entry.
	*/
	while (test->reg) {
		for (i = 0; i < test->array_len; i++) {
			bool b = false;

			/* different type of reg-testing */
			switch (test->test_type) {
			case PATTERN_TEST:
				b = reg_pattern_test(adapter, data,
				test->reg + (i * 0x40),
				test->mask, test->write,
				direct);
				break;

			case SET_READ_TEST:
				b = reg_set_and_check(adapter, data,
				test->reg + (i * 0x40),
				test->mask, test->write,
				direct);
				break;

			case WRITE_NO_TEST:
				xlnid_write_reg(hw, test->reg + (i * 0x40),
				test->write, direct);
				break;

			case TABLE32_TEST:
				b = reg_pattern_test(adapter, data,
				test->reg + (i * 4),
				test->mask, test->write,
				direct);
				break;

			case TABLE64_TEST_LO:
				b = reg_pattern_test(adapter, data,
				test->reg + (i * 8),
				test->mask, test->write,
				direct);
				break;

			case TABLE64_TEST_HI:
				b = reg_pattern_test(adapter, data,
				(test->reg + 4) + (i * 8),
				test->mask, test->write,
				direct);
				break;
			}

			if (b) {
				return true;
			}
		}

		test++;
	}

	*data = 0;
	return false;
}

/*
		Function:
		xlnid_eeprom_test
		Purpose:
		EEPROM-testing
		ethtool -t DEVNAME
		Parameters:
		adapter   - (IN) hardware information
		data      - (OUT) testing result
		Returns:
		true     test fail
		fail     test succeed
*/
static bool xlnid_eeprom_test(struct xlnid_adapter *adapter, u64 *data)
{
	/* struct xlnid_hw *hw = &adapter->hw; */
	*data = 0;
	return false;
}

/*
		Function:
		xlnid_test_intr
		Porpose:
		Read the interrupt reg -- EICR
		ethtool -t DEVNAME
		Parameters:
		irq - (IN) number of interrupt
		data   - (IN) information of device
		Return:
		result of interrupt-testing
*/
static irqreturn_t xlnid_test_intr(int __always_unused irq, void *data)
{
	struct net_device *netdev = (struct net_device *)data;
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	adapter->test_icr |= XLNID_READ_REG_DIRECT(&adapter->hw, EICR);

	return IRQ_HANDLED;
}

/*
		Function:
		xlnid_intr_test
		Porpose:
		Read the interrupt reg -- EICR
		ethtool -t DEVNAME
		Parameters:
		irq  - (IN) number of interrupt
		data    - (OUT) testing result
		Returns:
		true   test fail
		fail   test succeed
*/
static int xlnid_intr_test(struct xlnid_adapter *adapter, u64 *data)
{
	struct net_device *netdev = adapter->netdev;
	u32 mask = 0;
	u32 i = 0;
	u32 shared_int = true;
	u32 irq = adapter->pdev->irq;

	if (XLNID_REMOVED(adapter->hw.hw_addr)) {
		*data = 1;
		return -1;
	}

	*data = 0;

	/* Hook up test interrupt handler just for this test */
	if (adapter->msix_entries) {
		/* NOTE: we don't test MSI-X interrupts here, yet */
		return 0;
	}

	else if (adapter->flags & XLNID_FLAG_MSI_ENABLED) {
		shared_int = false;

		if (request_irq(irq, &xlnid_test_intr, 0, netdev->name,
						netdev)) {
			*data = 1;
			return -1;
		}
	}

	else if (!request_irq(irq, &xlnid_test_intr, IRQF_PROBE_SHARED,
							netdev->name, netdev)) {
		shared_int = false;
	}

	else if (request_irq(irq, &xlnid_test_intr, IRQF_SHARED, netdev->name,
							netdev)) {
		*data = 1;
		return -1;
	}

	e_info(hw, "testing %s interrupt\n",
			(shared_int ? "shared" : "unshared"));

	/* Disable all the interrupts */
	XLNID_WRITE_REG_DIRECT(&adapter->hw, EIMC, 0xFFFFFFFF);
	XLNID_WRITE_FLUSH(&adapter->hw);
	usleep_range(10000, 20000);

	/* Test each interrupt */
	for (; i < 10; i++) {
		/* Interrupt to test */
		mask = 1 << i;

		if (!shared_int) {
			/*
				Disable the interrupts to be reported in
				the cause register and then force the same
				interrupt and see if one gets posted.  If
				an interrupt was posted to the bus, the
				test failed.
			*/
			adapter->test_icr = 0;
			XLNID_WRITE_REG_DIRECT(&adapter->hw, EIMC,
									~mask & 0x00007FFF);
			XLNID_WRITE_REG_DIRECT(&adapter->hw, EICS,
									~mask & 0x00007FFF);
			XLNID_WRITE_FLUSH(&adapter->hw);
			usleep_range(10000, 20000);

			if (adapter->test_icr & mask) {
				*data = 3;
				break;
			}
		}

		/*
			Enable the interrupt to be reported in the cause
			register and then force the same interrupt and see
			if one gets posted.  If an interrupt was not posted
			to the bus, the test failed.
		*/
		adapter->test_icr = 0;
		XLNID_WRITE_REG_DIRECT(&adapter->hw, EIMS, mask);
		XLNID_WRITE_REG_DIRECT(&adapter->hw, EICS, mask);
		XLNID_WRITE_FLUSH(&adapter->hw);
		usleep_range(10000, 20000);

		if (!(adapter->test_icr & mask)) {
			*data = 4;
			break;
		}

		if (!shared_int) {
			/*
				Disable the other interrupts to be reported in
				the cause register and then force the other
				interrupts and see if any get posted.  If
				an interrupt was posted to the bus, the
				test failed.
			*/
			adapter->test_icr = 0;
			XLNID_WRITE_REG_DIRECT(&adapter->hw, EIMC,
									~mask & 0x00007FFF);
			XLNID_WRITE_REG_DIRECT(&adapter->hw, EICS,
									~mask & 0x00007FFF);
			XLNID_WRITE_FLUSH(&adapter->hw);

			usleep_range(10000, 20000);

			if (adapter->test_icr) {
				*data = 5;
				break;
			}
		}
	}

	/* Disable all the interrupts */
	XLNID_WRITE_REG_DIRECT(&adapter->hw, EIMC, 0xFFFFFFFF);
	XLNID_WRITE_FLUSH(&adapter->hw);
	usleep_range(10000, 20000);

	/* Unhook test interrupt handler */
	free_irq(irq, netdev);

	return *data;
}

/*
		Function:
		xlnid_free_desc_rings
		Purpose:
		free the descriptor of RX and TX
		Parameters:
		adapter - (IN) hardware information
		Return：
		void
*/
static void xlnid_free_desc_rings(struct xlnid_adapter *adapter)
{
	/*  Shut down the DMA engines now so they can be reinitialized later,
		since the test rings and normally used rings should overlap on
		queue 0 we can just use the standard disable Rx/Tx calls and they
		will take care of disabling the test rings for us.
	*/

	/* first Rx */
	xlnid_disable_rx_queue(adapter);

	/* now Tx */
	xlnid_disable_tx_queue(adapter);

	xlnid_reset(adapter);

	xlnid_free_tx_resources(&adapter->test_tx_ring);
	xlnid_free_rx_resources(&adapter->test_rx_ring);
}

/*
		Function:
		xlnid_setup_desc_rings
		Purpose:
		setup the descriptor of RX and TX
		Parameters:
		adapter - (IN) hardware information
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_setup_desc_rings(struct xlnid_adapter *adapter)
{
	struct xlnid_ring *tx_ring = &adapter->test_tx_ring;
	struct xlnid_ring *rx_ring = &adapter->test_rx_ring;

	/* u32 rctl, reg_data; */
	int ret_val = 0;
	int err = 0;

	/* Setup Tx descriptor ring and Tx buffers */
	tx_ring->count = XLNID_DEFAULT_TXD;
	tx_ring->queue_index = 0;
	tx_ring->dev = xlnid_pf_to_dev(adapter);
	tx_ring->netdev = adapter->netdev;
	tx_ring->reg_idx = adapter->tx_ring[0]->reg_idx;

	err = xlnid_setup_tx_resources(tx_ring);

	if (err) {
		return 1;
	}

#if 0
	reg_data = XLNID_READ_REG(&adapter->hw, DMATXCTL);
	reg_data |= XLNID_DMATXCTL_TE;
	XLNID_WRITE_REG(&adapter->hw, DMATXCTL, reg_data);
#endif

	xlnid_configure_tx_ring(adapter, tx_ring);

	/* Setup Rx Descriptor ring and Rx buffers */
	rx_ring->count = XLNID_DEFAULT_RXD;
	rx_ring->queue_index = 0;
	rx_ring->dev = xlnid_pf_to_dev(adapter);
	rx_ring->netdev = adapter->netdev;
	rx_ring->reg_idx = adapter->rx_ring[0]->reg_idx;

#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	rx_ring->rx_buf_len = XLNID_RXBUFFER_2K;
#endif

	err = xlnid_setup_rx_resources(adapter, rx_ring);

	if (err) {
		ret_val = 4;
		goto err_nomem;
	}

	xlnid_disable_rx(&adapter->hw);

	xlnid_configure_rx_ring(adapter, rx_ring);

	xlnid_enable_rx(&adapter->hw);

	return 0;

err_nomem:
	xlnid_free_desc_rings(adapter);
	return ret_val;
}

/*
		Function:
		xlnid_setup_loopback_test
		Purpose:
		Setup loopback testing through reg-writing
		Parameters:
		adapter - (IN) hardware information
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_setup_loopback_test(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 reg_data = 0;

	/* Setup MAC loopback */
	reg_data = XLNID_READ_REG(hw, FCTRL);
	reg_data |= XLNID_FCTRL_BAM | XLNID_FCTRL_SBP | XLNID_FCTRL_MPE;
	XLNID_WRITE_REG(hw, FCTRL, reg_data);

	switch (adapter->hw.mac.type) {
	case xlnid_mac_SKYLAKE:
		#ifdef XEL_XGE_SUPPORT
		reg_data = XLNID_READ_REG(hw, XGMAC_LOOPBACK_CTRL);
		reg_data |= 0x00000002;
		XLNID_WRITE_REG(hw, XGMAC_LOOPBACK_CTRL, reg_data);
		#endif
		break;

	case xlnid_mac_WESTLAKE:
		reg_data = xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_CONFIG);
		reg_data |= XLNID_GMAC_LM_MASK;
		xlnid_write_mac_westlake(hw, WESTLAKE_GMAC_CONFIG, reg_data);
		break;

	default:
		break;
	}

	XLNID_WRITE_FLUSH(hw);
	usleep_range(10000, 20000);

	return 0;
}

/*
		Function:
		xlnid_loopback_cleanup
		Purpose:
		Reset the reg to cleanup loopback
		Parameters:
		adapter - (IN) hardware information
		Return：
		void
*/
static void xlnid_loopback_cleanup(struct xlnid_adapter *adapter)
{
	u32 reg_data = 0;

	reg_data = XLNID_READ_REG(&adapter->hw, HLREG0);
	reg_data &= ~XLNID_HLREG0_LPBK;
	XLNID_WRITE_REG(&adapter->hw, HLREG0, reg_data);

	switch (adapter->hw.mac.type) {
	case xlnid_mac_SKYLAKE:
		#ifdef XEL_XGE_SUPPORT
		reg_data = XLNID_READ_REG(&adapter->hw, XGMAC_LOOPBACK_CTRL);
		reg_data &= ~(0x00000002);
		XLNID_WRITE_REG_MAC(&adapter->hw, XGMAC_LOOPBACK_CTRL,
		reg_data);
		#endif
		break;

	case xlnid_mac_WESTLAKE:
		reg_data = xlnid_read_mac_westlake(&adapter->hw,
		WESTLAKE_GMAC_CONFIG);
		reg_data &= ~(XLNID_GMAC_LM_MASK);
		xlnid_write_mac_westlake(&adapter->hw, WESTLAKE_GMAC_CONFIG,
		reg_data);
		break;

	default:
		break;
	}
}

/*
		Function:
		xlnid_create_lbtest_frame
		Purpose:
		Create frames for loopback testing
		Parameters:
		skb       - (OUT) data of frames
		frame_size   - (OUT) size of frames
		Return：
		void
*/
static void xlnid_create_lbtest_frame(struct sk_buff *skb,
										unsigned int frame_size)
{
	memset(skb->data, 0xFF, frame_size);
	frame_size >>= 1;
	memset(&skb->data[frame_size], 0xAA, frame_size / 2 - 1);
	memset(&skb->data[frame_size + 10], 0xBE, 1);
	memset(&skb->data[frame_size + 12], 0xAF, 1);
}

/*
		Function:
		xlnid_check_lbtest_frame
		Purpose:
		Check whether the frames are matched with
		frames that created in xlnid_create_lbtest_frame
		Parameters:
		rx_buffer    - (IN) data of frames
		frame_size  - (OUT) size of frames
		Returns:
		true     match
		false   not match
*/
static bool xlnid_check_lbtest_frame(struct xlnid_rx_buffer *rx_buffer,
										unsigned int frame_size)
{
	unsigned char *data = NULL;
	bool match = true;

	frame_size >>= 1;

#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	data = rx_buffer->skb->data;
#else
	data = kmap(rx_buffer->page) + rx_buffer->page_offset;
#endif

	if (data[3] != 0xFF || data[frame_size + 10] != 0xBE ||
			data[frame_size + 12] != 0xAF) {
		match = false;
	}

#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	kunmap(rx_buffer->page);
#endif

	return match;
}

/*
		Function:
		xlnid_clean_test_rings
		Purpose:
		Clean the rings used for testing
		Parameters:
		rx_ring - (IN)
		tx_ring - (IN)
		size       - (IN) size of rings
		Returns:
		count   frames number
*/
static u16 xlnid_clean_test_rings(struct xlnid_ring *rx_ring,
									struct xlnid_ring *tx_ring, unsigned int size)
{
	union xlnid_adv_rx_desc *rx_desc;

#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	const int bufsz = rx_ring->rx_buf_len;
#else
	const int bufsz = xlnid_rx_bufsz(rx_ring);
#endif

	u16 rx_ntc = 0;
	u16 tx_ntc = 0;
	u16 count = 0;

	/* initialize next to clean and descriptor values */
	rx_ntc = rx_ring->next_to_clean;
	tx_ntc = tx_ring->next_to_clean;
	rx_desc = XLNID_RX_DESC(rx_ring, rx_ntc);

	while (tx_ntc != tx_ring->next_to_use) {
		union xlnid_adv_tx_desc *tx_desc;
		struct xlnid_tx_buffer *tx_buffer;

		tx_desc = XLNID_TX_DESC(tx_ring, tx_ntc);

		/* if DD is not set transmit has not completed */
		if (!(tx_desc->wb.status & cpu_to_le32(XLNID_TXD_STAT_DD))) {
			return count;
		}

		/* unmap buffer on Tx side */
		tx_buffer = &tx_ring->tx_buffer_info[tx_ntc];

		/* Free all the Tx ring sk_buffs */
		dev_kfree_skb_any(tx_buffer->skb);

		/* unmap skb header data */
		dma_unmap_single(tx_ring->dev, dma_unmap_addr(tx_buffer, dma),
							dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
		dma_unmap_len_set(tx_buffer, len, 0);

		/* increment Tx next to clean counter */
		tx_ntc++;

		if (tx_ntc == tx_ring->count) {
			tx_ntc = 0;
		}
	}

	while (rx_desc->wb.upper.length) {
		struct xlnid_rx_buffer *rx_buffer;

		/* check Rx buffer */
		rx_buffer = &rx_ring->rx_buffer_info[rx_ntc];

		/* sync Rx buffer for CPU read */
		dma_sync_single_for_cpu(rx_ring->dev, rx_buffer->dma, bufsz,
								DMA_FROM_DEVICE);

		/* verify contents of skb */
		if (xlnid_check_lbtest_frame(rx_buffer, size)) {
			count++;
		}

		else {
			break;
		}

		/* sync Rx buffer for device write */
		dma_sync_single_for_device(rx_ring->dev, rx_buffer->dma, bufsz,
									DMA_FROM_DEVICE);

		/* increment Rx next to clean counter */
		rx_ntc++;

		if (rx_ntc == rx_ring->count) {
			rx_ntc = 0;
		}

		/* fetch next descriptor */
		rx_desc = XLNID_RX_DESC(rx_ring, rx_ntc);
	}

	/* re-map buffers to ring, store next to clean values */
	xlnid_alloc_rx_buffers(rx_ring, count);

	rx_ring->next_to_clean = rx_ntc;
	tx_ring->next_to_clean = tx_ntc;

	return count;
}

/*
		Function:
		xlnid_run_loopback_test
		Purpose:
		Create the loopback frames and clean the RX and TX rings
		Parameters:
		adapter - (IN) hardware information
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_run_loopback_test(struct xlnid_adapter *adapter)
{
	struct xlnid_ring *tx_ring = &adapter->test_tx_ring;
	struct xlnid_ring *rx_ring = &adapter->test_rx_ring;
	int i = 0;
	int j = 0;
	int lc = 0;
	int ret_val = 0;
	unsigned int size = 1024;
	netdev_tx_t tx_ret_val;
	struct sk_buff *skb = NULL;
	u32 flags_orig = adapter->flags;

	/* DCB can modify the frames on Tx */
	adapter->flags &= ~XLNID_FLAG_DCB_ENABLED;

	/* allocate test skb */
	skb = alloc_skb(size, GFP_KERNEL);

	if (!skb) {
		return 11;
	}

	/* place data into test skb */
	xlnid_create_lbtest_frame(skb, size);
	skb_put(skb, size);

	/*
		Calculate the loop count based on the largest descriptor ring
		The idea is to wrap the largest ring a number of times using 64
		send/receive pairs during each loop
	*/

	if (rx_ring->count <= tx_ring->count) {
		lc = ((tx_ring->count / 64) * 2) + 1;
	}

	else {
		lc = ((rx_ring->count / 64) * 2) + 1;
	}

	for (j = 0; j <= lc; j++) {
		unsigned int good_cnt;

		/* reset count of good packets */
		good_cnt = 0;

		/* place 64 packets on the transmit queue */
		for (i = 0; i < 64; i++) {
			skb_get(skb);
			tx_ret_val =
				xlnid_xmit_frame_ring(skb, adapter, tx_ring);

			if (tx_ret_val == NETDEV_TX_OK) {
				good_cnt++;
			}
		}

		if (good_cnt != 64) {
			ret_val = 12;
			break;
		}

		/* allow 200 milliseconds for packets to go from Tx to Rx */
		msleep(200);

		good_cnt = xlnid_clean_test_rings(rx_ring, tx_ring, size);

		if (good_cnt != 64) {
			ret_val = 13;
			break;
		}
	}

	/* free the original skb */
	kfree_skb(skb);
	adapter->flags = flags_orig;

	return ret_val;
}

/*
		Function:
		xlnid_loopback_test
		Purpose:
		The whole process of loopback testing
		Parameters:
		adapter - (IN) hardware information
		data       - (OUT) record the result of every step
		Returns:
		0   success
		otherwise  fail
*/
static int xlnid_loopback_test(struct xlnid_adapter *adapter, u64 *data)
{
	*data = xlnid_setup_desc_rings(adapter);

	if (*data) {
		goto out;
	}

	*data = xlnid_setup_loopback_test(adapter);

	if (*data) {
		goto err_loopback;
	}

	*data = xlnid_run_loopback_test(adapter);
	xlnid_loopback_cleanup(adapter);

err_loopback:
	xlnid_free_desc_rings(adapter);

out:
	return *data;
}

#ifndef HAVE_ETHTOOL_GET_SSET_COUNT

/*
		Function:
		xlnid_diag_test_count
		Purpose:
		Record all the type of testing
		Parameters:
		netdev  - (IN) device information
		Returns:
		all the type of testing
*/
static int xlnid_diag_test_count(struct net_device __always_unused *netdev)
{
	return XLNID_TEST_LEN;
}

#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */

/*
		Function:
		xlnid_diag_test
		Porpose:
		Register、eeprom、interrupt and loopback testing
		ethtool -t DEVNAME
		Parameters:
		netdev  - (IN) device information
		Returns:
		void
*/
static void xlnid_diag_test(struct net_device *netdev,
							struct ethtool_test *eth_test, u64 *data)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	bool if_running = netif_running(netdev);
	struct xlnid_hw *hw = &adapter->hw;

	if (XLNID_REMOVED(hw->hw_addr)) {
		e_err(hw, "Adapter removed - test blocked\n");
		data[0] = 1;
		data[1] = 1;
		data[2] = 1;
		data[3] = 1;
		data[4] = 1;
		eth_test->flags |= ETH_TEST_FL_FAILED;
		return;
	}

	set_bit(__XLNID_TESTING, &adapter->state);

	if (eth_test->flags == ETH_TEST_FL_OFFLINE) {
		if (adapter->flags & XLNID_FLAG_SRIOV_ENABLED) {
			int i;

			for (i = 0; i < adapter->num_vfs; i++) {
				if (adapter->vfinfo[i].clear_to_send) {
					e_warn(drv,
							"Please take active VFS "
							"offline and restart the "
							"adapter before running NIC "
							"diagnostics\n");
					data[0] = 1;
					data[1] = 1;
					data[2] = 1;
					data[3] = 1;
					data[4] = 1;
					eth_test->flags |= ETH_TEST_FL_FAILED;
					clear_bit(__XLNID_TESTING,
								&adapter->state);
					goto skip_ol_tests;
				}
			}
		}

		/* Offline tests */
		e_info(hw, "offline testing starting\n");

		/*  Link test performed before hardware reset so autoneg doesn't
			interfere with test result */
		if (xlnid_link_test(adapter, &data[4])) {
			eth_test->flags |= ETH_TEST_FL_FAILED;
		}

		if (if_running) {
			/* indicate we're in test mode */
			xlnid_close(netdev);
		}

		else {
			xlnid_reset(adapter);
		}

		e_info(hw, "register testing starting\n");

		if (xlnid_reg_test(adapter, &data[0])) {
			eth_test->flags |= ETH_TEST_FL_FAILED;
		}

		xlnid_reset(adapter);
		e_info(hw, "eeprom testing starting\n");

		if (xlnid_eeprom_test(adapter, &data[1])) {
			eth_test->flags |= ETH_TEST_FL_FAILED;
		}

		xlnid_reset(adapter);
		e_info(hw, "interrupt testing starting\n");

		if (xlnid_intr_test(adapter, &data[2])) {
			eth_test->flags |= ETH_TEST_FL_FAILED;
		}

		/*  If SRIOV or VMDq is enabled then skip MAC
			loopback diagnostic. */
		if (adapter->flags &
				(XLNID_FLAG_SRIOV_ENABLED | XLNID_FLAG_VMDQ_ENABLED)) {
			e_info(hw, "skip MAC loopback diagnostic in VT mode\n");
			data[3] = 0;
			goto skip_loopback;
		}

		xlnid_reset(adapter);
		e_info(hw, "loopback testing starting\n");

		if (xlnid_loopback_test(adapter, &data[3])) {
			eth_test->flags |= ETH_TEST_FL_FAILED;
		}

skip_loopback:
		xlnid_reset(adapter);

		/* clear testing bit and return adapter to previous state */
		clear_bit(__XLNID_TESTING, &adapter->state);

		if (if_running) {
			xlnid_open(netdev);
		}

		else if (hw->mac.ops.disable_tx_laser) {
			hw->mac.ops.disable_tx_laser(hw);
		}
	}

	else {
		e_info(hw, "online testing starting\n");

		/* Online tests */
		if (xlnid_link_test(adapter, &data[4])) {
			eth_test->flags |= ETH_TEST_FL_FAILED;
		}

		/* Offline tests aren't run; pass by default */
		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;

		clear_bit(__XLNID_TESTING, &adapter->state);
	}

skip_ol_tests:
	msleep_interruptible(4 * 1000);
}

/*
		Function:
		xlnid_wol_exclusion
		Purpose:
		not supported wake on lan
		Parameters:
		adapter - (IN) hardware information
		wol     - (IN) the information of wol
		Returns:
		0   success
		otherwise  fail
*/
static int xlnid_wol_exclusion(struct xlnid_adapter *adapter,
								struct ethtool_wolinfo *wol)
{
	/* struct xlnid_hw *hw = &adapter->hw; */
	int retval = 0;

	/* WOL not supported for all devices */
	retval = 1;
	wol->supported = 0;

	return retval;
}

/*
		Function:
		xlnid_get_wol
		Porpose:
		Get current wake on lan type
		ethtool DEVNAME
		Parameters:
		netdev  - (IN) device information
		wol    - (IN) current wake on lan type
		Return：
		void
*/
static void xlnid_get_wol(struct net_device *netdev,
							struct ethtool_wolinfo *wol)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	wol->supported = WAKE_UCAST | WAKE_MCAST | WAKE_BCAST | WAKE_MAGIC;
	wol->wolopts = 0;

	if (xlnid_wol_exclusion(adapter, wol) ||
			(!device_can_wakeup(xlnid_pf_to_dev(adapter)))) {
		return;
	}

	/* match with different type of WOL */
	if (adapter->wol & XLNID_WUFC_EX) {
		wol->wolopts |= WAKE_UCAST;
	}

	if (adapter->wol & XLNID_WUFC_MC) {
		wol->wolopts |= WAKE_MCAST;
	}

	if (adapter->wol & XLNID_WUFC_BC) {
		wol->wolopts |= WAKE_BCAST;
	}

	if (adapter->wol & XLNID_WUFC_MAG) {
		wol->wolopts |= WAKE_MAGIC;
	}
}

/*
		Function:
		xlnid_set_wol
		Porpose:
		Set wake on lan type
		ethtool DEVNAME
		Parameters:
		netdev  - (IN) device information
		wol    - (IN) wake on lan type needed to be set
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wol)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;

	if (wol->wolopts &
			(WAKE_PHY | WAKE_ARP | WAKE_MAGICSECURE | WAKE_FILTER)) {
		return -EOPNOTSUPP;
	}

	if (xlnid_wol_exclusion(adapter, wol)) {
		return wol->wolopts ? -EOPNOTSUPP : 0;
	}

	adapter->wol = 0;

	/* match with different type of WOL */
	if (wol->wolopts & WAKE_UCAST) {
		adapter->wol |= XLNID_WUFC_EX;
	}

	if (wol->wolopts & WAKE_MCAST) {
		adapter->wol |= XLNID_WUFC_MC;
	}

	if (wol->wolopts & WAKE_BCAST) {
		adapter->wol |= XLNID_WUFC_BC;
	}

	if (wol->wolopts & WAKE_MAGIC) {
		adapter->wol |= XLNID_WUFC_MAG;
	}

	hw->wol_enabled = !!(adapter->wol);

	device_set_wakeup_enable(xlnid_pf_to_dev(adapter), adapter->wol);

	return 0;
}

/*
		Function:
		xlnid_nway_reset
		Porpose:
		Restart port auto negotiation
		ethtool -r DEVNAME
		Parameters:
		netdev  - (IN) device information
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_nway_reset(struct net_device *netdev)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev)) {
		xlnid_reinit_locked(adapter);
	}

	return 0;
}

#ifdef HAVE_ETHTOOL_SET_PHYS_ID

/*
		Function:
		xlnid_set_phys_id
		Porpose:
		Set the specified port-led status to active
		ethtool -p DEVNAME
		Parameters:
		netdev  - (IN) device information
		state    - (IN) current port-led state
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_set_phys_id(struct net_device *netdev,
								enum ethtool_phys_id_state state)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;

	if (!hw->mac.ops.led_on || !hw->mac.ops.led_off) {
		return -EOPNOTSUPP;
	}

	switch (state) {
	case ETHTOOL_ID_ACTIVE:
		if (hw->mac.type == xlnid_mac_WESTLAKE) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCMD, 0x0);
			}

			adapter->led_reg = XLNID_READ_REG(hw, LEDCTL);
			return 2;

	case ETHTOOL_ID_ON:
		if (hw->mac.ops.led_on(hw, hw->mac.led_link_act)) {
			return -EINVAL;
		}

		break;

	case ETHTOOL_ID_OFF:
		if (hw->mac.ops.led_off(hw, hw->mac.led_link_act)) {
			return -EINVAL;
		}

		break;

	case ETHTOOL_ID_INACTIVE:

		/* Restore LED settings */
		XLNID_WRITE_REG(&adapter->hw, LEDCTL, adapter->led_reg);
		break;
	}

	return 0;
}
#else /* HAVE_ETHTOOL_SET_PHYS_ID */

/*
		Function:
		xlnid_phys_id
		Porpose:
		Set the specified port-led status to active
		ethtool -p DEVNAME
		Parameters:
		netdev  - (IN) device information
		data      - (IN) specified blink time(s)
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_phys_id(struct net_device *netdev, u32 data)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	u32 led_reg = 0;
	u32 i = 0;

	if (!hw->mac.ops.led_on || !hw->mac.ops.led_off) {
		return -EOPNOTSUPP;
	}

	if (!data || data > 300) {
		data = 300;
	}

	if (hw->mac.type == xlnid_mac_WESTLAKE) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCMD, 0x0);
	}

	led_reg = XLNID_READ_REG(hw, LEDCTL);

	for (i = 0; i < (data * 1000); i += 400) {
		if (hw->mac.ops.led_on(hw, hw->mac.led_link_act)) {
			return -EINVAL;
		}

		msleep_interruptible(200);

		if (hw->mac.ops.led_off(hw, hw->mac.led_link_act)) {
			return -EINVAL;
		}

		msleep_interruptible(200);
	}

	/* Restore LED settings */
	XLNID_WRITE_REG(hw, LEDCTL, led_reg);

	return XLNID_SUCCESS;
}
#endif /* HAVE_ETHTOOL_SET_PHYS_ID */

/*
		Function:
		xlnid_get_coalesce
		Porpose:
		Get the coalesce parameters
		ethtool -c DEVNAME
		Parameters:
		netdev  - (IN) device information
		data      - (IN) specified blink time(s)
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_get_coalesce(struct net_device *netdev,
#ifdef HAVE_ETHTOOL_COALESCE_EXTACK
								struct ethtool_coalesce *ec,
								struct kernel_ethtool_coalesce *kernel_coal,
								struct netlink_ext_ack *extack)
#else
								struct ethtool_coalesce *ec)
#endif
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	ec->tx_max_coalesced_frames_irq = adapter->tx_work_limit;

	/* only valid if in constant ITR mode */
	if (adapter->rx_itr_setting <= 1) {
		ec->rx_coalesce_usecs = adapter->rx_itr_setting;
	}

	else {
		ec->rx_coalesce_usecs = adapter->rx_itr_setting >> 2;
	}

	/* if in mixed tx/rx queues per vector mode, report only rx settings */
	if (adapter->q_vector[0]->tx.count && adapter->q_vector[0]->rx.count) {
		return 0;
	}

	/* only valid if in constant ITR mode */
	if (adapter->tx_itr_setting <= 1) {
		ec->tx_coalesce_usecs = adapter->tx_itr_setting;
	}

	else {
		ec->tx_coalesce_usecs = adapter->tx_itr_setting >> 2;
	}

	return 0;
}

/*
		Function:
		xlnid_update_rsc
		Purpose:
		update rx-usecs value
		this function must be called before setting the new value of
		rx_itr_setting
		Parameters:
		adapter - (IN) device information
		Returns:
		true    update succeed
		false   update fail
*/
static bool xlnid_update_rsc(struct xlnid_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	/* nothing to do if LRO or RSC are not enabled */
	if (!(adapter->flags2 & XLNID_FLAG2_RSC_CAPABLE) ||
			(!(netdev->features & NETIF_F_LRO))) {
		return false;
	}

	/* check the feature flag value and enable RSC if necessary */
	if ((adapter->rx_itr_setting == 1) ||
			(adapter->rx_itr_setting > XLNID_MIN_RSC_ITR)) {
		if (!(adapter->flags2 & XLNID_FLAG2_RSC_ENABLED)) {
			adapter->flags2 |= XLNID_FLAG2_RSC_ENABLED;
			e_info(probe, "rx-usecs value high enough "
					"to re-enable RSC\n");
			return true;
		}
	}

	/* if interrupt rate is too high then disable RSC */
	else if (adapter->flags2 & XLNID_FLAG2_RSC_ENABLED) {
		adapter->flags2 &= ~XLNID_FLAG2_RSC_ENABLED;
		e_info(probe, "rx-usecs set too low, disabling RSC\n");
		return true;
	}

	return false;
}

/*
		Function:
		xlnid_set_coalesce
		Porpose:
		Set the coalesce parameters
		ethtool -C DEVNAME
		Parameters:
		netdev  - (IN) device information
		ec      - (IN) specified blink time(s)
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_set_coalesce(struct net_device *netdev,
#ifdef HAVE_ETHTOOL_COALESCE_EXTACK
								struct ethtool_coalesce *ec,
								struct kernel_ethtool_coalesce *kernel_coal,
								struct netlink_ext_ack *extack)
#else
								struct ethtool_coalesce *ec)
#endif
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	int i = 0;
	u16 tx_itr_param = 0;
	u16 rx_itr_param = 0;
	u16 tx_itr_prev = 0;
	bool need_reset = false;

	if (adapter->q_vector[0]->tx.count && adapter->q_vector[0]->rx.count) {
		/* reject Tx specific changes in case of mixed RxTx vectors */
		if (ec->tx_coalesce_usecs) {
			return -EINVAL;
		}

		tx_itr_prev = adapter->rx_itr_setting;
	}

	else {
		tx_itr_prev = adapter->tx_itr_setting;
	}

	if (ec->tx_max_coalesced_frames_irq) {
		adapter->tx_work_limit = ec->tx_max_coalesced_frames_irq;
	}

	if ((ec->rx_coalesce_usecs > (XLNID_MAX_EITR >> 2)) ||
			(ec->tx_coalesce_usecs > (XLNID_MAX_EITR >> 2))) {
		return -EINVAL;
	}

	if (ec->rx_coalesce_usecs > 1) {
		adapter->rx_itr_setting = ec->rx_coalesce_usecs << 2;
	}

	else {
		adapter->rx_itr_setting = ec->rx_coalesce_usecs;
	}

	if (adapter->rx_itr_setting == 1) {
		rx_itr_param = XLNID_20K_ITR;
	}

	else {
		rx_itr_param = adapter->rx_itr_setting;
	}

	if (ec->tx_coalesce_usecs > 1) {
		adapter->tx_itr_setting = ec->tx_coalesce_usecs << 2;
	}

	else {
		adapter->tx_itr_setting = ec->tx_coalesce_usecs;
	}

	if (adapter->tx_itr_setting == 1) {
		tx_itr_param = XLNID_12K_ITR;
	}

	else {
		tx_itr_param = adapter->tx_itr_setting;
	}

	/* mixed Rx/Tx */
	if (adapter->q_vector[0]->tx.count && adapter->q_vector[0]->rx.count) {
		adapter->tx_itr_setting = adapter->rx_itr_setting;
	}

	/* detect ITR changes that require update of TXDCTL.WTHRESH */
	if ((adapter->tx_itr_setting != 1) &&
			(adapter->tx_itr_setting < XLNID_100K_ITR)) {
		if ((tx_itr_prev == 1) || (tx_itr_prev >= XLNID_100K_ITR)) {
			need_reset = true;
		}
	}

	else {
		if ((tx_itr_prev != 1) && (tx_itr_prev < XLNID_100K_ITR)) {
			need_reset = true;
		}
	}

	/* check the old value and enable RSC if necessary */
	need_reset |= xlnid_update_rsc(adapter);

	if (adapter->hw.mac.dmac_config.watchdog_timer &&
			(!adapter->rx_itr_setting && !adapter->tx_itr_setting)) {
		e_info(probe,
				"Disabling DMA coalescing because interrupt throttling is disabled\n");
		adapter->hw.mac.dmac_config.watchdog_timer = 0;
		xlnid_dmac_config(&adapter->hw);
	}

	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct xlnid_q_vector *q_vector = adapter->q_vector[i];

		q_vector->tx.work_limit = adapter->tx_work_limit;

		if (q_vector->tx.count && !q_vector->rx.count) {
			/* tx only */
			q_vector->itr = tx_itr_param;
		}

		else {
			/* rx only or mixed */
			q_vector->itr = rx_itr_param;
		}

		xlnid_write_eitr(q_vector);
	}

	/*
		do reset here at the end to make sure EITR == 0 case is handled
		correctly w.r.t stopping tx, and changing TXDCTL.WTHRESH settings
		also locks in RSC enable/disable which requires reset
	*/
	if (need_reset) {
		xlnid_do_reset(netdev);
	}

	return 0;
}

#ifndef HAVE_NDO_SET_FEATURES

/*
		Function:
		xlnid_get_rx_csum
		Purpose:
		Get all the features of RX
		Parameters:
		netdev  - (IN) device information
		Returns:
		features of RX
*/
static u32 xlnid_get_rx_csum(struct net_device *netdev)
{
	return !!(netdev->features & NETIF_F_RXCSUM);
}

/*
		Function:
		xlnid_set_rx_csum
		Purpose:
		Set all the features of RX
		Parameters:
		netdev  - (IN) device information
		data      - (IN) the feature need to reset
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_set_rx_csum(struct net_device *netdev, u32 data)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	bool need_reset = false;

	if (data) {
		netdev->features |= NETIF_F_RXCSUM;
	}

	else {
		netdev->features &= ~NETIF_F_RXCSUM;
	}

	/* LRO and RSC both depend on RX checksum to function */
	if (!data && (netdev->features & NETIF_F_LRO)) {
		netdev->features &= ~NETIF_F_LRO;

		if (adapter->flags2 & XLNID_FLAG2_RSC_ENABLED) {
			adapter->flags2 &= ~XLNID_FLAG2_RSC_ENABLED;
			need_reset = true;
		}
	}

#ifdef HAVE_VXLAN_RX_OFFLOAD

	if (adapter->flags & XLNID_FLAG_VXLAN_OFFLOAD_CAPABLE && data) {
		netdev->hw_enc_features |= NETIF_F_RXCSUM | NETIF_F_IP_CSUM |
									NETIF_F_IPV6_CSUM;

		if (!need_reset) {
			adapter->flags2 |= XLNID_FLAG2_VXLAN_REREG_NEEDED;
		}
	}

	else {
		netdev->hw_enc_features &=
			~(NETIF_F_RXCSUM | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
		xlnid_clear_udp_tunnel_port(adapter,
									XLNID_VXLANCTRL_ALL_UDPPORT_MASK);
	}

#endif /* HAVE_VXLAN_RX_OFFLOAD */

	if (need_reset) {
		xlnid_do_reset(netdev);
	}

	return 0;
}

/*
		Function:
		xlnid_set_tx_csum
		Purpose:
		Set all the features of TX
		Parameters:
		netdev  - (IN) device information
		data      - (IN) the feature need to reset
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_set_tx_csum(struct net_device *netdev, u32 data)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

#ifdef NETIF_F_IPV6_CSUM
	u32 feature_list = NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
#else
	u32 feature_list = NETIF_F_IP_CSUM;
#endif

	switch (adapter->hw.mac.type) {
	case xlnid_mac_SKYLAKE:

#ifdef HAVE_ENCAP_TSO_OFFLOAD
		if (data) {
		netdev->hw_enc_features |= NETIF_F_GSO_UDP_TUNNEL;
			}

			else {
				netdev->hw_enc_features &= ~NETIF_F_GSO_UDP_TUNNEL;
			}

			feature_list |= NETIF_F_GSO_UDP_TUNNEL;
#endif /* HAVE_ENCAP_TSO_OFFLOAD */

			feature_list |= NETIF_F_SCTP_CSUM;
			break;

	default:
		break;
	}

	if (data) {
		netdev->features |= feature_list;
	}

	else {
		netdev->features &= ~feature_list;
	}

	return 0;
}

#ifdef NETIF_F_TSO

/*
		Function:
		xlnid_set_tso
		Purpose:
		Enable TCP segment offload
		Parameters:
		netdev  - (IN) device information
		data      - (IN) the segment size of packet
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_set_tso(struct net_device *netdev, u32 data)
{
#ifdef NETIF_F_TSO6
	u32 feature_list = NETIF_F_TSO | NETIF_F_TSO6;
#else
	u32 feature_list = NETIF_F_TSO;
#endif

	if (data) {
		netdev->features |= feature_list;
	}

	else {
		netdev->features &= ~feature_list;
	}

#ifndef HAVE_NETDEV_VLAN_FEATURES

	if (!data) {
		struct xlnid_adapter *adapter = netdev_priv(netdev);
		struct net_device *v_netdev;
		int i = 0;

		/* disable TSO on all VLANs if they're present */
		if (!adapter->vlgrp) {
			goto tso_out;
		}

		for (i = 0; i < VLAN_GROUP_ARRAY_LEN; i++) {
			v_netdev = vlan_group_get_device(adapter->vlgrp, i);

			if (!v_netdev) {
				continue;
			}

			v_netdev->features &= ~feature_list;
			vlan_group_set_device(adapter->vlgrp, i, v_netdev);
		}
	}

tso_out:

#endif /* HAVE_NETDEV_VLAN_FEATURES */

	return 0;
}
#endif /* NETIF_F_TSO */

#ifdef ETHTOOL_GFLAGS

/*
		Function:
		xlnid_set_flags
		Purpose:
		Set different flags of the related features
		Parameters:
		netdev  - (IN) device information
		data      - (IN) change of features
		Returns:
		0          success
		otherwise   fail
*/
static int xlnid_set_flags(struct net_device *netdev, u32 data)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	u32 supported_flags = ETH_FLAG_RXVLAN | ETH_FLAG_TXVLAN;
	u32 changed = netdev->features ^ data;
	bool need_reset = false;
	int rc = 0;

#ifndef HAVE_VLAN_RX_REGISTER

	if ((adapter->flags & XLNID_FLAG_DCB_ENABLED) &&
			!(data & ETH_FLAG_RXVLAN)) {
		return -EINVAL;
	}

#endif

	if (adapter->flags2 & XLNID_FLAG2_RSC_CAPABLE) {
		supported_flags |= ETH_FLAG_LRO;
	}

#ifdef ETHTOOL_GRXRINGS

	switch (adapter->hw.mac.type) {
	case xlnid_mac_SKYLAKE:
		supported_flags |= ETH_FLAG_NTUPLE;

	default:
		break;
	}

#endif

#ifdef NETIF_F_RXHASH
	supported_flags |= ETH_FLAG_RXHASH;
#endif

	rc = ethtool_op_set_flags(netdev, data, supported_flags);

	if (rc) {
		return rc;
	}

#ifndef HAVE_VLAN_RX_REGISTER

	if (changed & ETH_FLAG_RXVLAN) {
		xlnid_vlan_mode(netdev, netdev->features);
	}

#endif

#ifdef HAVE_VXLAN_RX_OFFLOAD

	if (adapter->flags & XLNID_FLAG_VXLAN_OFFLOAD_CAPABLE &&
			netdev->features & NETIF_F_RXCSUM) {
		/* vxlan_get_rx_port(netdev); */
	} else {
		xlnid_clear_udp_tunnel_port(adapter,
									XLNID_VXLANCTRL_ALL_UDPPORT_MASK);
	}

#endif /* HAVE_VXLAN_RX_OFFLOAD */

	/* if state changes we need to update adapter->flags and reset */
	if (!(netdev->features & NETIF_F_LRO)) {
		if (adapter->flags2 & XLNID_FLAG2_RSC_ENABLED) {
			need_reset = true;
		}

		adapter->flags2 &= ~XLNID_FLAG2_RSC_ENABLED;
	}

	else if ((adapter->flags2 & XLNID_FLAG2_RSC_CAPABLE) &&
				(!(adapter->flags2 & XLNID_FLAG2_RSC_ENABLED))) {
		if (adapter->rx_itr_setting == 1 ||
				adapter->rx_itr_setting > XLNID_MIN_RSC_ITR) {
			adapter->flags2 |= XLNID_FLAG2_RSC_ENABLED;
			need_reset = true;
		}

		else if (changed & ETH_FLAG_LRO) {
			e_info(probe, "rx-usecs set too low, "
					"disabling RSC\n");
		}
	}

#ifdef ETHTOOL_GRXRINGS

	/*
		Check if Flow Director n-tuple support was enabled or disabled.  If
		the state changed, we need to reset.
	*/
	switch (netdev->features & NETIF_F_NTUPLE) {
	case NETIF_F_NTUPLE:

		/* turn off ATR, enable perfect filters and reset */
		if (!(adapter->flags & XLNID_FLAG_FDIR_PERFECT_CAPABLE)) {
		need_reset = true;
			}

			adapter->flags &= ~XLNID_FLAG_FDIR_HASH_CAPABLE;
			adapter->flags |= XLNID_FLAG_FDIR_PERFECT_CAPABLE;
			break;

	default:

		/* turn off perfect filters, enable ATR and reset */
		if (adapter->flags & XLNID_FLAG_FDIR_PERFECT_CAPABLE) {
			need_reset = true;
		}

		adapter->flags &= ~XLNID_FLAG_FDIR_PERFECT_CAPABLE;

		/* We cannot enable ATR if VMDq is enabled */
		if (adapter->flags & XLNID_FLAG_VMDQ_ENABLED) {
			break;
		}

		/* We cannot enable ATR if we have 2 or more traffic classes */
		if (netdev_get_num_tc(netdev) > 1) {
			break;
		}

		/* We cannot enable ATR if RSS is disabled */
		if (adapter->ring_feature[RING_F_RSS].limit <= 1) {
			break;
		}

		/* A sample rate of 0 indicates ATR disabled */
		if (!adapter->atr_sample_rate) {
			break;
		}

		adapter->flags |= XLNID_FLAG_FDIR_HASH_CAPABLE;
		break;
	}

#endif /* ETHTOOL_GRXRINGS */

	if (need_reset) {
		xlnid_do_reset(netdev);
	}

	return 0;
}

#endif /* ETHTOOL_GFLAGS */
#endif /* HAVE_NDO_SET_FEATURES */

#ifdef ETHTOOL_GRXRINGS

/*
	Function:
	xlnid_get_ethtool_fdir_entry
	Purpose:
	Get fdir entry according to different type of packets
	Parameters:
	adapter - (IN) device information
	cmd     - (OUT) record all the information of the packet
	Returns:
	0         success
	otherwise  fail
*/
static int xlnid_get_ethtool_fdir_entry(struct xlnid_adapter *adapter,
										struct ethtool_rxnfc *cmd)
{
	union xlnid_atr_input *mask = &adapter->fdir_mask;
	struct ethtool_rx_flow_spec *fsp =
		(struct ethtool_rx_flow_spec *)&cmd->fs;
	struct hlist_node *node2 = NULL;
	struct xlnid_fdir_filter *rule = NULL;

	/* report total rule count */
	cmd->data = (1024 << adapter->fdir_pballoc) - 2;

	hlist_for_each_entry_safe(rule, node2, &adapter->fdir_filter_list,
								fdir_node) {
		if (fsp->location <= rule->sw_idx) {
			break;
		}
	}

	if (!rule || fsp->location != rule->sw_idx) {
		return -EINVAL;
	}

	/* fill out the flow spec entry */

	/* set flow type field */
	switch (rule->filter.formatted.flow_type) {
	case XLNID_ATR_FLOW_TYPE_TCPV4:
		fsp->flow_type = TCP_V4_FLOW;
		break;

	case XLNID_ATR_FLOW_TYPE_UDPV4:
		fsp->flow_type = UDP_V4_FLOW;
		break;

	case XLNID_ATR_FLOW_TYPE_TCPV6:
		fsp->flow_type = TCP_V6_FLOW;
		break;

	case XLNID_ATR_FLOW_TYPE_UDPV6:
		fsp->flow_type = UDP_V6_FLOW;
		break;

	case XLNID_ATR_FLOW_TYPE_SCTPV4:
		fsp->flow_type = SCTP_V4_FLOW;
		break;

	case XLNID_ATR_FLOW_TYPE_IPV4:
		fsp->flow_type = IP_USER_FLOW;
		fsp->h_u.usr_ip4_spec.ip_ver = ETH_RX_NFC_IP4;
		fsp->h_u.usr_ip4_spec.proto = 0;
		fsp->m_u.usr_ip4_spec.proto = 0;
		break;

	default:
		return -EINVAL;
	}

	/* Copy input into formatted structures */
#ifdef HAVE_ETHTOOL_FLOW_UNION_IP6_SPEC

	if (rule->filter.formatted.flow_type & XLNID_ATR_L4TYPE_IPV6_MASK) {
		fsp->h_u.tcp_ip6_spec.psrc = rule->filter.formatted.src_port;
		fsp->m_u.tcp_ip6_spec.psrc = mask->formatted.src_port;
		fsp->h_u.tcp_ip6_spec.pdst = rule->filter.formatted.dst_port;
		fsp->m_u.tcp_ip6_spec.pdst = mask->formatted.dst_port;
	}

	else {
#endif
		fsp->h_u.tcp_ip4_spec.psrc = rule->filter.formatted.src_port;
		fsp->m_u.tcp_ip4_spec.psrc = mask->formatted.src_port;
		fsp->h_u.tcp_ip4_spec.pdst = rule->filter.formatted.dst_port;
		fsp->m_u.tcp_ip4_spec.pdst = mask->formatted.dst_port;
		fsp->h_u.tcp_ip4_spec.ip4src = rule->filter.formatted.src_ip[0];
		fsp->m_u.tcp_ip4_spec.ip4src = mask->formatted.src_ip[0];
		fsp->h_u.tcp_ip4_spec.ip4dst = rule->filter.formatted.dst_ip[0];
		fsp->m_u.tcp_ip4_spec.ip4dst = mask->formatted.dst_ip[0];
#ifdef HAVE_ETHTOOL_FLOW_UNION_IP6_SPEC
	}

#endif

	fsp->h_ext.vlan_tci = rule->filter.formatted.vlan_id;
	fsp->m_ext.vlan_tci = mask->formatted.vlan_id;
	fsp->h_ext.vlan_etype = rule->filter.formatted.flex_bytes;
	fsp->m_ext.vlan_etype = mask->formatted.flex_bytes;
	fsp->h_ext.data[1] = htonl(rule->filter.formatted.vm_pool);
	fsp->m_ext.data[1] = htonl(mask->formatted.vm_pool);
	fsp->flow_type |= FLOW_EXT;

	/* record action */
	if (rule->action == XLNID_FDIR_DROP_QUEUE) {
		fsp->ring_cookie = RX_CLS_FLOW_DISC;
	}

	else {
		fsp->ring_cookie = rule->action;
	}

	return 0;
}

/*
		Function:
		xlnid_get_ethtool_fdir_all
		Purpose:
		Get the count of all the rules
		Parameters:
		para1   - (IN)
		para2   - (OUT)
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_get_ethtool_fdir_all(struct xlnid_adapter *adapter,
										struct ethtool_rxnfc *cmd, u32 *rule_locs)
{
	struct hlist_node *node2 = NULL;
	struct xlnid_fdir_filter *rule = NULL;
	int cnt = 0;

	/* report total rule count */
	cmd->data = (1024 << adapter->fdir_pballoc) - 2;

	hlist_for_each_entry_safe(rule, node2, &adapter->fdir_filter_list,
								fdir_node) {
		if (cnt == cmd->rule_cnt) {
			return -EMSGSIZE;
		}

		rule_locs[cnt] = rule->sw_idx;
		cnt++;
	}

	cmd->rule_cnt = cnt;

	return 0;
}

/*
	Function:
	xlnid_get_rss_hash_opts
	Purpose:
	Get the hash options according to different type of packets
	Parameters:
	adapter - (IN) device information
	cmd     - (OUT) record all the information of the packet
	Returns:
	0         success
	otherwise  fail
*/
static int xlnid_get_rss_hash_opts(struct xlnid_adapter *adapter,
									struct ethtool_rxnfc *cmd)
{
	cmd->data = 0;

	/* Report default options for RSS on xlnid */
	switch (cmd->flow_type) {
	case TCP_V4_FLOW:
		cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		fallthrough;

	case UDP_V4_FLOW:
		if (adapter->flags2 & XLNID_FLAG2_RSS_FIELD_IPV4_UDP) {
		cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
			}

			fallthrough;

	case SCTP_V4_FLOW:
	case AH_ESP_V4_FLOW:
	case AH_V4_FLOW:
	case ESP_V4_FLOW:
	case IPV4_FLOW:
		cmd->data |= RXH_IP_SRC | RXH_IP_DST;
		break;

	case TCP_V6_FLOW:
		cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		fallthrough;

	case UDP_V6_FLOW:
		if (adapter->flags2 & XLNID_FLAG2_RSS_FIELD_IPV6_UDP) {
			cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		}

		fallthrough;

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

/*
		Function:
		xlnid_get_rxnfc
		Purpose:
		get RX entry、ring、redirection table and hash key
		ethtool -x DEVNAME
		Parameters:
		dev - (IN) device information
		cmd - (IN) record all the information ethtool_rxnfc need
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_get_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd,
#ifdef HAVE_ETHTOOL_GET_RXNFC_VOID_RULE_LOCS
							void *rule_locs)
#else
							u32 *rule_locs)
#endif
{
	struct xlnid_adapter *adapter = netdev_priv(dev);
	int ret = -EOPNOTSUPP;

	switch (cmd->cmd) {
		/* Get RX rings available for LB */
	case ETHTOOL_GRXRINGS:
		cmd->data = adapter->num_rx_queues;
		ret = 0;
		break;

		/* Get RX class rule count */
	case ETHTOOL_GRXCLSRLCNT:
		cmd->rule_cnt = adapter->fdir_filter_count;
		ret = 0;
		break;

		/* Get RX classification rule */
	case ETHTOOL_GRXCLSRULE:
		ret = xlnid_get_ethtool_fdir_entry(adapter, cmd);
		break;

		/* Get all RX classification rule */
	case ETHTOOL_GRXCLSRLALL:
		ret = xlnid_get_ethtool_fdir_all(adapter, cmd,
		(u32 *)rule_locs);
		break;

		/* Get RX flow hash configuration */
	case ETHTOOL_GRXFH:
		ret = xlnid_get_rss_hash_opts(adapter, cmd);
		break;

	default:
		break;
	}

	return ret;
}

/*
		Function:
		xlnid_update_ethtool_fdir_entry
		Purpose:
		update the rule of fdir
		Parameters:
		adapter  - (IN) device information
		input   - (IN) the information of fdir
		sw_idx   - (IN) the hash index of rules
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_update_ethtool_fdir_entry(struct xlnid_adapter *adapter,
		struct xlnid_fdir_filter *input,
		u16 sw_idx)
{
	struct xlnid_hw *hw = &adapter->hw;
	struct hlist_node *node2 = NULL;
	struct xlnid_fdir_filter *rule = NULL;
	struct xlnid_fdir_filter *parent = NULL;
	bool deleted = false;
	s32 err = 0;

	hlist_for_each_entry_safe(rule, node2, &adapter->fdir_filter_list,
								fdir_node) {
		/* hash found, or no matching entry */
		if (rule->sw_idx >= sw_idx) {
			break;
		}

		parent = rule;
	}

	/* if there is an old rule occupying our place remove it */
	if (rule && (rule->sw_idx == sw_idx)) {
		/*  hardware filters are only configured when interface is up,
			and we should not issue filter commands while the interface
			is down
		*/
		if (netif_running(adapter->netdev) &&
				(!input || (rule->filter.formatted.bkt_hash !=
							input->filter.formatted.bkt_hash))) {
			/*
				err = xlnid_fdir_erase_perfect_filter(hw,
				&rule->filter,
				sw_idx);
			*/
			err = xlnid_fdir_remove_perfect_filter(
						hw, &rule->filter, sw_idx, rule->action,
						adapter->cloud_mode);

			if (err) {
				return -EINVAL;
			}
		}

		hlist_del(&rule->fdir_node);
		kfree(rule);
		adapter->fdir_filter_count--;
		deleted = true;
	}

	/*  If we weren't given an input, then this was a request to delete a
		filter. We should return -EINVAL if the filter wasn't found, but
		return 0 if the rule was successfully deleted.
	*/
	if (!input) {
		return deleted ? 0 : -EINVAL;
	}

	/* initialize node and set software index */
	INIT_HLIST_NODE(&input->fdir_node);

	/* add filter to the list */
	if (parent) {
		hlist_add_behind(&input->fdir_node, &parent->fdir_node);
	}

	else {
		hlist_add_head(&input->fdir_node, &adapter->fdir_filter_list);
	}

	/* update counts */
	adapter->fdir_filter_count++;

	return 0;
}

/*
		Function:
		xlnid_flowspec_to_flow_type
		Purpose:
		Distinguish different type of the packets
		Parameters:
		fsp       - (IN) all the information of flow
		flow_type   - (OUT) record the flow type
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_flowspec_to_flow_type(struct ethtool_rx_flow_spec *fsp,
										u8 *flow_type)
{
	switch (fsp->flow_type & ~FLOW_EXT) {
	case TCP_V4_FLOW:
		*flow_type = XLNID_ATR_FLOW_TYPE_TCPV4;
		break;

	case UDP_V4_FLOW:
		*flow_type = XLNID_ATR_FLOW_TYPE_UDPV4;
		break;

	case TCP_V6_FLOW:
		*flow_type = XLNID_ATR_FLOW_TYPE_TCPV6;
		break;

	case UDP_V6_FLOW:
		*flow_type = XLNID_ATR_FLOW_TYPE_UDPV6;
		break;

	case SCTP_V4_FLOW:
		*flow_type = XLNID_ATR_FLOW_TYPE_SCTPV4;
		break;

	case IP_USER_FLOW:
			switch (fsp->h_u.usr_ip4_spec.proto) {
			case IPPROTO_TCP:
				*flow_type = XLNID_ATR_FLOW_TYPE_TCPV4;
				break;

			case IPPROTO_UDP:
				*flow_type = XLNID_ATR_FLOW_TYPE_UDPV4;
				break;

			case IPPROTO_SCTP:
				*flow_type = XLNID_ATR_FLOW_TYPE_SCTPV4;
				break;

			case 0:
				if (!fsp->m_u.usr_ip4_spec.proto) {
				*flow_type = XLNID_ATR_FLOW_TYPE_IPV4;
				break;
					}

					fallthrough;

			default:
				return 0;
			}

			break;

	default:
		return 0;
	}

	return 1;
}

#if 0
static bool xlnid_match_ethtool_fdir_entry(struct xlnid_adapter *adapter,
		struct xlnid_fdir_filter *input)
{
	struct hlist_node *node2;
	struct xlnid_fdir_filter *rule = NULL;

	hlist_for_each_entry_safe(rule, node2,
								&adapter->fdir_filter_list, fdir_node) {
		if (rule->filter.formatted.bkt_hash ==
				input->filter.formatted.bkt_hash &&
				rule->action == input->action) {
			e_info(drv, "FDIR entry already exist\n");
			return true;
		}
	}
	return false;
}
#endif

/*
		Function:
		xlnid_add_ethtool_fdir_entry
		Purpose:
		Add a new rule for fdir
		Parameters:
		adapter - (IN) device information
		cmd     - (IN) transmit information to the new struct
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_add_ethtool_fdir_entry(struct xlnid_adapter *adapter,
										struct ethtool_rxnfc *cmd)
{
	struct ethtool_rx_flow_spec *fsp =
		(struct ethtool_rx_flow_spec *)&cmd->fs;
	struct xlnid_hw *hw = &adapter->hw;
	struct xlnid_fdir_filter *input = NULL;
	union xlnid_atr_input mask;
	u8 queue;
	int err = 0;

	/* whether perfect match filter supported or not */
	if (!(adapter->flags & XLNID_FLAG_FDIR_PERFECT_CAPABLE)) {
		return -EOPNOTSUPP;
	}

	/*
		ring_cookie is a masked into a set of queues or
		we use drop index
	*/
	if (fsp->ring_cookie == RX_CLS_FLOW_DISC) {
		queue = XLNID_FDIR_DROP_QUEUE;
	}

	else {
		u32 ring = ethtool_get_flow_spec_ring(fsp->ring_cookie);

		if (ring >= adapter->num_rx_queues) {
			return -EINVAL;
		}

		queue = adapter->rx_ring[ring]->reg_idx;
	}

	/*  Don't allow indexes to exist outside of available space
		index is depend on max supported fdir numbers
	*/
	if (fsp->location >= ((1024 << adapter->fdir_pballoc) - 2)) {
		e_err(drv, "Location out of range\n");
		return -EINVAL;
	}

	input = kzalloc(sizeof(*input), GFP_ATOMIC);

	if (!input) {
		return -ENOMEM;
	}

	memset(&mask, 0, sizeof(union xlnid_atr_input));

	/*  set SW index
		loc %d :fdir index
	*/
	input->sw_idx = fsp->location;

	/* record flow type */
	if (!xlnid_flowspec_to_flow_type(fsp,
										&input->filter.formatted.flow_type)) {
		e_err(drv, "Unrecognized flow type\n");
		goto err_out;
	}

	mask.formatted.flow_type = XLNID_ATR_L4TYPE_IPV6_MASK |
								XLNID_ATR_L4TYPE_MASK;

	if (input->filter.formatted.flow_type == XLNID_ATR_FLOW_TYPE_IPV4) {
		mask.formatted.flow_type &= XLNID_ATR_L4TYPE_IPV6_MASK;
	}

#ifdef HAVE_ETHTOOL_FLOW_UNION_IP6_SPEC

	/* not support full IPV6 address filtering */
	if (input->filter.formatted.flow_type & XLNID_ATR_L4TYPE_IPV6_MASK) {
		if (fsp->m_u.tcp_ip6_spec.ip6src[0] ||
				fsp->m_u.tcp_ip6_spec.ip6src[1] ||
				fsp->m_u.tcp_ip6_spec.ip6src[2] ||
				fsp->m_u.tcp_ip6_spec.ip6src[3] ||
				fsp->m_u.tcp_ip6_spec.ip6dst[0] ||
				fsp->m_u.tcp_ip6_spec.ip6dst[1] ||
				fsp->m_u.tcp_ip6_spec.ip6dst[2] ||
				fsp->m_u.tcp_ip6_spec.ip6dst[3]) {
			e_err(drv, "Error not support IPv6 address fitlers\n");
			goto err_out;
		}

		input->filter.formatted.src_port = fsp->h_u.tcp_ip6_spec.psrc;
		mask.formatted.src_port = fsp->m_u.tcp_ip6_spec.psrc;
		input->filter.formatted.dst_port = fsp->h_u.tcp_ip6_spec.pdst;
		mask.formatted.dst_port = fsp->m_u.tcp_ip6_spec.pdst;
	}

	else {
#endif
		/* Copy input into formatted structures */
		input->filter.formatted.src_ip[0] =
			fsp->h_u.tcp_ip4_spec.ip4src;
		mask.formatted.src_ip[0] = fsp->m_u.tcp_ip4_spec.ip4src;
		input->filter.formatted.dst_ip[0] =
			fsp->h_u.tcp_ip4_spec.ip4dst;
		mask.formatted.dst_ip[0] = fsp->m_u.tcp_ip4_spec.ip4dst;
		input->filter.formatted.src_port = fsp->h_u.tcp_ip4_spec.psrc;
		mask.formatted.src_port = fsp->m_u.tcp_ip4_spec.psrc;
		input->filter.formatted.dst_port = fsp->h_u.tcp_ip4_spec.pdst;
		mask.formatted.dst_port = fsp->m_u.tcp_ip4_spec.pdst;
#ifdef HAVE_ETHTOOL_FLOW_UNION_IP6_SPEC
	}

#endif

	if (fsp->flow_type & FLOW_EXT) {
		input->filter.formatted.vm_pool =
			(unsigned char)ntohl(fsp->h_ext.data[1]);
		mask.formatted.vm_pool =
			(unsigned char)ntohl(fsp->m_ext.data[1]);
		input->filter.formatted.vlan_id = fsp->h_ext.vlan_tci;
		mask.formatted.vlan_id = fsp->m_ext.vlan_tci;
		input->filter.formatted.flex_bytes = fsp->h_ext.vlan_etype;
		mask.formatted.flex_bytes = fsp->m_ext.vlan_etype;
	}

	/* determine if we need to drop or route the packet */
	if (fsp->ring_cookie == RX_CLS_FLOW_DISC) {
		input->action = XLNID_FDIR_DROP_QUEUE;
	}

	else {
		input->action = fsp->ring_cookie;
	}

	spin_lock(&adapter->fdir_perfect_lock);

	if (hlist_empty(&adapter->fdir_filter_list)) {
		/* save mask and program input mask into HW */
		memcpy(&adapter->fdir_mask, &mask, sizeof(mask));
		err = xlnid_fdir_set_input_mask(hw, &mask, adapter->cloud_mode);

		if (err) {
			e_err(drv, "Error writing mask\n");
			goto err_out_w_lock;
		}
	}

	else if (memcmp(&adapter->fdir_mask, &mask, sizeof(mask))) {
		e_err(drv,
				"Hardware only supports one mask per port. To change the mask you must first delete all the rules.\n");
		goto err_out_w_lock;
	}

#if 0
	/* apply mask and compute/store hash */
	xlnid_atr_compute_perfect_hash(&input->filter, &mask);

	/* check if new entry does not exist on filter list */
	if (xlnid_match_ethtool_fdir_entry(adapter, input)) {
		goto err_out_w_lock;
	}

#endif

	/*  only program filters to hardware if the net device is running, as
		we store the filters in the Rx buffer which is not allocated when
		the device is down
	*/
	if (netif_running(adapter->netdev)) {
		err = xlnid_fdir_write_perfect_filter(hw, &input->filter,
												input->sw_idx, queue,
												adapter->cloud_mode);

		if (err) {
			goto err_out_w_lock;
		}
	}

	xlnid_update_ethtool_fdir_entry(adapter, input, input->sw_idx);

	spin_unlock(&adapter->fdir_perfect_lock);

	return 0;

err_out_w_lock:
	spin_unlock(&adapter->fdir_perfect_lock);

err_out:
	kfree(input);
	return -EINVAL;
}

/*
		Function:
		xlnid_add_ethtool_fdir_entry
		Purpose:
		Delete an old rule of fdir
		Parameters:
		adapter - (IN) device information
		cmd     - (IN) transmit information to the new struct
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_del_ethtool_fdir_entry(struct xlnid_adapter *adapter,
										struct ethtool_rxnfc *cmd)
{
	struct ethtool_rx_flow_spec *fsp =
		(struct ethtool_rx_flow_spec *)&cmd->fs;
	int err = 0;

	spin_lock(&adapter->fdir_perfect_lock);
	err = xlnid_update_ethtool_fdir_entry(adapter, NULL, fsp->location);
	spin_unlock(&adapter->fdir_perfect_lock);

	return err;
}

#ifdef ETHTOOL_SRXNTUPLE

/*
		We need to keep this around for kernels 2.6.33 - 2.6.39 in order to avoid
		a null pointer dereference as it was assumend if the NETIF_F_NTUPLE flag
		was defined that this function was present.
*/
static int xlnid_set_rx_ntuple(struct net_device __always_unused *dev,
								struct ethtool_rx_ntuple __always_unused *cmd)
{
	return -EOPNOTSUPP;
}

#endif /* ETHTOOL_SRXNTUPLE */

#define UDP_RSS_FLAGS \
	(XLNID_FLAG2_RSS_FIELD_IPV4_UDP | XLNID_FLAG2_RSS_FIELD_IPV6_UDP)
/*
		Function:
		xlnid_set_rss_hash_opt
		Purpose:
		Set the hash option according to the type of flow
		Parameters:
		adapter - (IN) device information
		cmd     - (OUT) record the option of RSS
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_set_rss_hash_opt(struct xlnid_adapter *adapter,
									struct ethtool_rxnfc *nfc)
{
	u32 flags2 = adapter->flags2;

	/*
		RSS does not support anything other than hashing
		to queues on src and dst IPs and ports
	*/
	if (nfc->data &
			~(RXH_IP_SRC | RXH_IP_DST | RXH_L4_B_0_1 | RXH_L4_B_2_3)) {
		return -EINVAL;
	}

	switch (nfc->flow_type) {
	case TCP_V4_FLOW:
	case TCP_V6_FLOW:
		if (!(nfc->data & RXH_IP_SRC) || (!(nfc->data & RXH_IP_DST)) ||
		(!(nfc->data & RXH_L4_B_0_1)) ||
		(!(nfc->data & RXH_L4_B_2_3))) {
		return -EINVAL;
			}

			break;

	case UDP_V4_FLOW:
		if (!(nfc->data & RXH_IP_SRC) || (!(nfc->data & RXH_IP_DST))) {
			return -EINVAL;
		}

		switch (nfc->data & (RXH_L4_B_0_1 | RXH_L4_B_2_3)) {
		case 0:
			flags2 &= ~XLNID_FLAG2_RSS_FIELD_IPV4_UDP;
			break;

		case (RXH_L4_B_0_1 | RXH_L4_B_2_3):
			flags2 |= XLNID_FLAG2_RSS_FIELD_IPV4_UDP;
			break;

		default:
			return -EINVAL;
		}

		break;

	case UDP_V6_FLOW:
		if (!(nfc->data & RXH_IP_SRC) || (!(nfc->data & RXH_IP_DST))) {
			return -EINVAL;
		}

		switch (nfc->data & (RXH_L4_B_0_1 | RXH_L4_B_2_3)) {
		case 0:
			flags2 &= ~XLNID_FLAG2_RSS_FIELD_IPV6_UDP;
			break;

		case (RXH_L4_B_0_1 | RXH_L4_B_2_3):
			flags2 |= XLNID_FLAG2_RSS_FIELD_IPV6_UDP;
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
		if (!(nfc->data & RXH_IP_SRC) || (!(nfc->data & RXH_IP_DST)) ||
				(nfc->data & RXH_L4_B_0_1) || (nfc->data & RXH_L4_B_2_3)) {
			return -EINVAL;
		}

		break;

	default:
		return -EINVAL;
	}

	/* if we changed something we need to update flags */
	if (flags2 != adapter->flags2) {
		struct xlnid_hw *hw = &adapter->hw;
		u32 mrqc;

		mrqc = XLNID_READ_REG(hw, MRQC);

		if ((flags2 & UDP_RSS_FLAGS) &&
				!(adapter->flags2 & UDP_RSS_FLAGS)) {
			e_warn(drv,
					"enabling UDP RSS: fragmented packets"
					" may arrive out of order to the stack above\n");
		}

		adapter->flags2 = flags2;

		/* Perform hash on these packet types */
		mrqc |= XLNID_MRQC_RSS_FIELD_IPV4 |
				XLNID_MRQC_RSS_FIELD_IPV4_TCP |
				XLNID_MRQC_RSS_FIELD_IPV6 |
				XLNID_MRQC_RSS_FIELD_IPV6_TCP;

		mrqc &= ~(XLNID_MRQC_RSS_FIELD_IPV4_UDP |
					XLNID_MRQC_RSS_FIELD_IPV6_UDP);

		if (flags2 & XLNID_FLAG2_RSS_FIELD_IPV4_UDP) {
			mrqc |= XLNID_MRQC_RSS_FIELD_IPV4_UDP;
		}

		if (flags2 & XLNID_FLAG2_RSS_FIELD_IPV6_UDP) {
			mrqc |= XLNID_MRQC_RSS_FIELD_IPV6_UDP;
		}

		XLNID_WRITE_REG(hw, MRQC, mrqc);
	}

	return 0;
}

/*
		Function:
		xlnid_set_rxnfc
		Purpose:
		Execute different command of queue classification
		ethtool -X DEVNAME
		Parameters:
		dev - (IN) device information
		cmd - (IN) offer the command
		Returns:
		ret record the result
*/
static int xlnid_set_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd)
{
	struct xlnid_adapter *adapter = netdev_priv(dev);
	int ret = -EOPNOTSUPP;

	switch (cmd->cmd) {
	case ETHTOOL_SRXCLSRLINS:
		ret = xlnid_add_ethtool_fdir_entry(adapter, cmd);
		break;

	case ETHTOOL_SRXCLSRLDEL:
		ret = xlnid_del_ethtool_fdir_entry(adapter, cmd);
		break;

	case ETHTOOL_SRXFH:
		ret = xlnid_set_rss_hash_opt(adapter, cmd);
		break;

	default:
		break;
	}

	return ret;
}

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)

static int xlnid_rss_indir_tbl_max(struct xlnid_adapter *adapter)
{
	return 16;
}

static u32 xlnid_get_rxfh_key_size(struct net_device *netdev)
{
	return XLNID_RSS_KEY_SIZE;
}

static u32 xlnid_rss_indir_size(struct net_device *netdev)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);

	return xlnid_rss_indir_tbl_entries(adapter);
}

/*
		Function:
		xlnid_get_reta
		Purpose:
		Get the redirection table
		Parameters:
		adapter - (IN) device information
		indir     - (OUT) get the index of every entry
		Return:
		void
*/
static void xlnid_get_reta(struct xlnid_adapter *adapter, u32 *indir)
{
	int i, reta_size = xlnid_rss_indir_tbl_entries(adapter);
	u16 rss_m = adapter->ring_feature[RING_F_RSS].mask;

	if (adapter->flags & XLNID_FLAG_SRIOV_ENABLED) {
		rss_m = adapter->ring_feature[RING_F_RSS].indices - 1;
	}

	for (i = 0; i < reta_size; i++) {
		indir[i] = adapter->rss_indir_tbl[i] & rss_m;
	}
}

/*
		Function:
		xlnid_get_rxfh
		Purpose:
		Get RX redirection table and the hash key
		Parameters:
		dev - (IN) device information
		indir  - (OUT) get the index of every entry
		key - (OUT) get the hash key
		Returns:
		0         success
		otherwise  fail
*/
#ifdef HAVE_RXFH_HASHFUNC
#ifdef HAVE_ETHTOOL_RXFH_PARAM
static int xlnid_get_rxfh(struct net_device *netdev,
							struct ethtool_rxfh_param *rxfh)
#else
static int xlnid_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key,
							u8 *hfunc)
#endif /* HAVE_ETHTOOL_RXFH_PARAM */
#else
static int xlnid_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key)
#endif
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
#ifdef HAVE_ETHTOOL_RXFH_PARAM
	u32 *indir = rxfh->indir;
	u8 *key = rxfh->key;
#endif /* HAVE_ETHTOOL_RXFH_PARAM */

#ifdef HAVE_RXFH_HASHFUNC
#ifdef HAVE_ETHTOOL_RXFH_PARAM
	rxfh->hfunc = ETH_RSS_HASH_TOP;
#else

	if (hfunc) {
		*hfunc = ETH_RSS_HASH_TOP;
	}

#endif /* HAVE_ETHTOOL_RXFH_PARAM */
#endif

	if (indir) {
		xlnid_get_reta(adapter, indir);
	}

	if (key) {
		memcpy(key, adapter->rss_key, xlnid_get_rxfh_key_size(netdev));
	}

	return 0;
}

/*
		Function:
		xlnid_get_rxfh
		Purpose:
		Set RX redirection table and the hash key
		Parameters:
		dev  - (IN) device information
		indir   - (IN) set the index of every entry
		key  - (IN) set the hash key
		hfunc   - (IN) set the hash function
		Returns:
		0         success
		otherwise  fail
*/
#ifdef HAVE_RXFH_HASHFUNC
#ifdef HAVE_ETHTOOL_RXFH_PARAM
static int xlnid_set_rxfh(struct net_device *netdev,
							struct ethtool_rxfh_param *rxfh,
							struct netlink_ext_ack *extack)
#else
static int xlnid_set_rxfh(struct net_device *netdev, const u32 *indir,
							const u8 *key, const u8 hfunc)
#endif /* HAVE_ETHTOOL_RXFH_PARAM */
#else
#ifdef HAVE_RXFH_NONCONST
static int xlnid_set_rxfh(struct net_device *netdev, u32 *indir, u8 *key)
#else
static int xlnid_set_rxfh(struct net_device *netdev, const u32 *indir,
							const u8 *key)
#endif /* HAVE_RXFH_NONCONST */
#endif /* HAVE_RXFH_HASHFUNC */
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
#ifdef HAVE_ETHTOOL_RXFH_PARAM
	u32 *indir = rxfh->indir;
	u8 *key = rxfh->key;
#endif /* HAVE_ETHTOOL_RXFH_PARAM */
	int i = 0;
	u32 reta_entries = xlnid_rss_indir_tbl_entries(adapter);

#ifdef HAVE_RXFH_HASHFUNC
#ifndef HAVE_ETHTOOL_RXFH_PARAM

	if (hfunc) {
		return -EINVAL;
	}

#endif /* !HAVE_ETHTOOL_RXFH_PARAM */
#endif

	/* Fill out the redirection table */
	if (indir) {
		int max_queues = min_t(int, adapter->num_rx_queues,
								xlnid_rss_indir_tbl_max(adapter));

		/*Allow at least 2 queues w/ SR-IOV. */
		if ((adapter->flags & XLNID_FLAG_SRIOV_ENABLED) &&
				(max_queues < 2)) {
			max_queues = 2;
		}

		/* Verify user input. */
		for (i = 0; i < reta_entries; i++) {
			if (indir[i] >= max_queues) {
				return -EINVAL;
			}
		}

		for (i = 0; i < reta_entries; i++) {
			adapter->rss_indir_tbl[i] = indir[i];
		}

		xlnid_store_reta(adapter);
	}

	/* Fill out the rss hash key */
	if (key) {
		memcpy(adapter->rss_key, key, xlnid_get_rxfh_key_size(netdev));
		xlnid_store_key(adapter);
	}

	return 0;
}
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */

#ifdef HAVE_ETHTOOL_GET_TS_INFO

/*
		Function:
		xlnid_get_ts_info
		Purpose:
		Get the information of timestamping.
		Parameters:
		dev  - (IN) device information
		info    - (OUT) record the information
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_get_ts_info(struct net_device *dev,
								struct ethtool_ts_info *info)
{
	struct xlnid_adapter *adapter = netdev_priv(dev);

	/* we always support timestamping disabled */
	info->rx_filters = BIT(HWTSTAMP_FILTER_NONE);

#ifdef HAVE_PTP_1588_CLOCK
	info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V2_L2_EVENT) |
						BIT(HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ) |
						BIT(HWTSTAMP_FILTER_PTP_V2_L2_SYNC) |
						BIT(HWTSTAMP_FILTER_PTP_V2_L4_EVENT) |
						BIT(HWTSTAMP_FILTER_PTP_V2_L4_SYNC) |
						BIT(HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ) |
						BIT(HWTSTAMP_FILTER_PTP_V2_EVENT) |
						BIT(HWTSTAMP_FILTER_PTP_V2_SYNC) |
						BIT(HWTSTAMP_FILTER_PTP_V2_DELAY_REQ);

#else
	return ethtool_op_get_ts_info(dev, info);
#endif /* HAVE_PTP_1588_CLOCK */

#ifdef HAVE_PTP_1588_CLOCK
	info->so_timestamping =
		SOF_TIMESTAMPING_TX_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE |
		SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_TX_HARDWARE |
		SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;

	if (adapter->ptp_clock) {
		info->phc_index = ptp_clock_index(adapter->ptp_clock);
	}

	else {
		info->phc_index = -1;
	}

	info->tx_types = BIT(HWTSTAMP_TX_OFF) | BIT(HWTSTAMP_TX_ON) |
						BIT(HWTSTAMP_TX_ONESTEP_SYNC);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
	info->tx_types |= BIT(HWTSTAMP_TX_ONESTEP_P2P);
#endif

	return 0;
#endif /* HAVE_PTP_1588_CLOCK */
}

#endif /* HAVE_ETHTOOL_GET_TS_INFO */

#endif /* ETHTOOL_GRXRINGS */

#ifdef ETHTOOL_SCHANNELS

/*
		Function:
		xlnid_max_channels
		Purpose:
		Get the maximum of channels according to
		the judgement of DCB、SRIOV and FDIR.
		Parameters:
		adapter - (IN) device information
		Return:
		max_combined    maximum of channels
*/
static unsigned int xlnid_max_channels(struct xlnid_adapter *adapter)
{
	unsigned int max_combined = 0;
	u8 tcs = netdev_get_num_tc(adapter->netdev);

	if (!(adapter->flags & XLNID_FLAG_MSIX_ENABLED)) {
		/* We only support one q_vector without MSI-X */
		max_combined = 1;
	}

	else if (adapter->flags & XLNID_FLAG_SRIOV_ENABLED) {
		/* Limit value based on the queue mask */
		max_combined = adapter->ring_feature[RING_F_RSS].mask + 1;
	}

	else if (tcs > 1) {
		if (tcs > 4) {
			/* 8 TC w/ 8 queues per TC */
			max_combined = 8;
		}

		else {
			/* 4 TC w/ 16 queues per TC */
			max_combined = 16;
		}
	}

	else if (adapter->atr_sample_rate) {
		/* support up to 64 queues with ATR */
		if (adapter->hw.mac.type == xlnid_mac_WESTLAKE) {
			max_combined = WESTLAKE_MAX_FDIR_INDICES;
		}

		else {
			max_combined = XLNID_MAX_FDIR_INDICES;
		}
	}

	else {
		/* support up to max allowed queues with RSS */
		max_combined = xlnid_max_rss_indices(adapter);
	}

	return max_combined;
}

/*
		Function:
		xlnid_get_channels
		Porpose:
		Get current channel information
		ethtool -l DEVNAME
		Parameters:
		netdev  - (IN) device information
		ch      - (OUT) current channel information
		Return:
		void
*/
static void xlnid_get_channels(struct net_device *dev,
								struct ethtool_channels *ch)
{
	struct xlnid_adapter *adapter = netdev_priv(dev);

	/* report maximum channels */
	ch->max_combined = xlnid_max_channels(adapter);

	/* report info for other vector */
	if (adapter->flags & XLNID_FLAG_MSIX_ENABLED) {
		ch->max_other = NON_Q_VECTORS;
		ch->other_count = NON_Q_VECTORS;
	}

	/* record RSS queues */
	ch->combined_count = adapter->ring_feature[RING_F_RSS].indices;

	/* nothing else to report if RSS is disabled */
	if (ch->combined_count == 1) {
		return;
	}

	/* we do not support ATR queueing if SR-IOV is enabled */
	if (adapter->flags & XLNID_FLAG_SRIOV_ENABLED) {
		return;
	}

	/* same thing goes for being DCB enabled */
	if (netdev_get_num_tc(dev) > 1) {
		return;
	}

	/* if ATR is disabled we can exit */
	if (!adapter->atr_sample_rate) {
		return;
	}

	/* report flow director queues as maximum channels */
	ch->combined_count = adapter->ring_feature[RING_F_FDIR].indices;
}

/*
	Function:
	xlnid_set_channels
	Porpose:
	Set new channel information
	ethtool -L DEVNAME
	Parameters:
	netdev  - (IN) device information
	ch      - (IN) new channel information
	Returns:
	0         success
	otherwise  fail
*/
static int xlnid_set_channels(struct net_device *dev,
								struct ethtool_channels *ch)
{
	struct xlnid_adapter *adapter = netdev_priv(dev);
	unsigned int count = ch->combined_count;
	u8 max_rss_indices = xlnid_max_rss_indices(adapter);

	/* verify they are not requesting separate vectors */
	if (!count || ch->rx_count || ch->tx_count) {
		return -EINVAL;
	}

	/* verify other_count has not changed */
	if (ch->other_count != NON_Q_VECTORS) {
		return -EINVAL;
	}

	/* verify the number of channels does not exceed hardware limits */
	if (count > xlnid_max_channels(adapter)) {
		return -EINVAL;
	}

	/* update feature limits from largest to smallest supported values */
	adapter->ring_feature[RING_F_FDIR].limit = count;

	/* cap RSS limit */
	if (count > max_rss_indices) {
		count = max_rss_indices;
	}

	adapter->ring_feature[RING_F_RSS].limit = count;

#if 0

	/* cap FCoE limit at 8 */
	if (count > XLNID_FCRETA_SIZE) {
		count = XLNID_FCRETA_SIZE;
	}

	adapter->ring_feature[RING_F_FCOE].limit = count;
#endif /* CONFIG_FCOE */

	/* use setup TC to update any traffic class queue mapping */
	return xlnid_setup_tc(dev, netdev_get_num_tc(dev));
}

#endif /* ETHTOOL_SCHANNELS */

#if 0
static int xlnid_get_module_info(struct net_device *dev,
									struct ethtool_modinfo *modinfo)
{
	struct xlnid_adapter *adapter = netdev_priv(dev);
	struct xlnid_hw *hw = &adapter->hw;
	u32 status;
	u8 sff8472_rev, addr_mode;
	bool page_swap = false;

	/* Check whether we support SFF-8472 or not */
	status = hw->phy.ops.read_i2c_eeprom(hw,
											XLNID_SFF_SFF_8472_COMP,
											&sff8472_rev);

	if (status != 0) {
		return -EIO;
	}

	/* addressing mode is not supported */
	status = hw->phy.ops.read_i2c_eeprom(hw,
											XLNID_SFF_SFF_8472_SWAP,
											&addr_mode);

	if (status != 0) {
		return -EIO;
	}

	if (addr_mode & XLNID_SFF_ADDRESSING_MODE) {
		e_err(drv,
				"Address change required to access page 0xA2, but not supported. Please report the module type to the driver maintainers.\n");
		page_swap = true;
	}

	if (sff8472_rev == XLNID_SFF_SFF_8472_UNSUP || page_swap) {
		/* We have a SFP, but it does not support SFF-8472 */
		modinfo->type = ETH_MODULE_SFF_8079;
		modinfo->eeprom_len = ETH_MODULE_SFF_8079_LEN;
	}

	else {
		/* We have a SFP which supports a revision of SFF-8472. */
		modinfo->type = ETH_MODULE_SFF_8472;
		modinfo->eeprom_len = ETH_MODULE_SFF_8472_LEN;
	}

	return 0;
}

static int xlnid_get_module_eeprom(struct net_device *dev,
									struct ethtool_eeprom *ee,
									u8 *data)
{
	struct xlnid_adapter *adapter = netdev_priv(dev);
	struct xlnid_hw *hw = &adapter->hw;
	u32 status = XLNID_ERR_PHY_ADDR_INVALID;
	u8 databyte = 0xFF;
	int i = 0;

	if (ee->len == 0) {
		return -EINVAL;
	}

	for (i = ee->offset; i < ee->offset + ee->len; i++) {
		/* I2C reads can take long time */
		if (test_bit(__XLNID_IN_SFP_INIT, &adapter->state)) {
			return -EBUSY;
		}

		if (i < ETH_MODULE_SFF_8079_LEN) {
			status = hw->phy.ops.read_i2c_eeprom(hw, i, &databyte);
		}

		else {
			status = hw->phy.ops.read_i2c_sff8472(hw, i, &databyte);
		}

		if (status != 0) {
			return -EIO;
		}

		data[i - ee->offset] = databyte;
	}

	return 0;
}
#endif /* ETHTOOL_GMODULEINFO */

#ifdef ETHTOOL_GEEE

static const struct {
	xlnid_link_speed mac_speed;
	u32 link_mode;
} xlnid_ls_map[] = {
	{ XLNID_LINK_SPEED_10_FULL, SUPPORTED_10baseT_Full },
	{ XLNID_LINK_SPEED_100_FULL, SUPPORTED_100baseT_Full },
	{ XLNID_LINK_SPEED_1GB_FULL, SUPPORTED_1000baseT_Full },
	{ XLNID_LINK_SPEED_2_5GB_FULL, SUPPORTED_2500baseX_Full },
	{ XLNID_LINK_SPEED_10GB_FULL, SUPPORTED_10000baseT_Full },
};

static const struct {
	u32 lp_advertised;
	u32 link_mode;
} xlnid_lp_map[] = {
	{ FW_PHY_ACT_UD_2_100M_TX_EEE, SUPPORTED_100baseT_Full },
	{ FW_PHY_ACT_UD_2_1G_T_EEE, SUPPORTED_1000baseT_Full },
	{ FW_PHY_ACT_UD_2_10G_T_EEE, SUPPORTED_10000baseT_Full },
	{ FW_PHY_ACT_UD_2_1G_KX_EEE, SUPPORTED_1000baseKX_Full },
	{ FW_PHY_ACT_UD_2_10G_KX4_EEE, SUPPORTED_10000baseKX4_Full },
	{ FW_PHY_ACT_UD_2_10G_KR_EEE, SUPPORTED_10000baseKR_Full },
};

static int xlnid_get_keee(struct net_device *netdev,
							struct ethtool_keee *kedata)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;

	if (!hw->mac.ops.setup_eee) {
		return -EOPNOTSUPP;
	}

	if (!(adapter->flags2 & XLNID_FLAG2_EEE_CAPABLE)) {
		return -EOPNOTSUPP;
	}

	return -EOPNOTSUPP;
}

#ifndef HAVE_ETHTOOL_KEEE
/*
		Function:
		xlnid_get_eee
		Purpose:
		Get the status of EEE
		Parameters:
		netdev  - (IN) device information
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_get_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
	struct ethtool_keee kedata;
	int ret;

	eee_to_keee(&kedata, edata);
	ret = xlnid_get_keee(netdev, &kedata);
	keee_to_eee(edata, &kedata);

	return ret;
}
#endif /* !HAVE_ETHTOOL_KEEE */
#endif /* ETHTOOL_GEEE */

#ifdef ETHTOOL_SEEE
static int xlnid_set_keee(struct net_device *netdev,
							struct ethtool_keee *kedata)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	struct xlnid_hw *hw = &adapter->hw;
	struct ethtool_keee
		keee_data; /* structure storing current eee settings */
	s32 ret_val;

	if (!(hw->mac.ops.setup_eee &&
			(adapter->flags2 & XLNID_FLAG2_EEE_CAPABLE))) {
		return -EOPNOTSUPP;
	}

	memset(&keee_data, 0, sizeof(struct ethtool_keee));

	ret_val = xlnid_get_keee(netdev, &keee_data);

	if (ret_val) {
		return ret_val;
	}

	if (keee_data.tx_lpi_enabled != kedata->tx_lpi_enabled) {
		e_dev_err("Setting EEE tx-lpi is not supported\n");
		return -EINVAL;
	}

	if (keee_data.tx_lpi_timer != kedata->tx_lpi_timer) {
		e_dev_err("Setting EEE Tx LPI timer is not supported\n");
		return -EINVAL;
	}

	if (keee_data.advertised != kedata->advertised) {
		e_dev_err("Setting EEE advertised speeds is not supported\n");
		return -EINVAL;
	}

	if (keee_data.eee_enabled == kedata->eee_enabled) {
		return 0;
	}

	if (kedata->eee_enabled) {
		adapter->flags2 |= XLNID_FLAG2_EEE_ENABLED;
		hw->phy.eee_speeds_advertised = hw->phy.eee_speeds_supported;
	}

	else {
		adapter->flags2 &= ~XLNID_FLAG2_EEE_ENABLED;
		hw->phy.eee_speeds_advertised = 0;
	}

	/* reset link */
	if (netif_running(netdev)) {
		xlnid_reinit_locked(adapter);
	}

	else {
		xlnid_reset(adapter);
	}

	return 0;
}

/*
		Function:
		xlnid_set_eee
		Purpose:
		Set up the EEE function and set related information
		Parameters:
		netdev  - (IN) device information
		edata    - (OUT) record the status of EEE
		Returns:
		0         success
		otherwise  fail
*/
#ifndef HAVE_ETHTOOL_KEEE
static int xlnid_set_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
	struct ethtool_keee kedata;
	int ret;

	eee_to_keee(&kedata, edata);
	ret = xlnid_set_keee(netdev, &kedata);
	keee_to_eee(edata, &kedata);

	return ret;
}
#endif /* !HAVE_ETHTOOL_KEEE */
#endif /* ETHTOOL_SEEE */

#ifdef HAVE_ETHTOOL_GET_SSET_COUNT

/**
		The get string set count and the string set should be matched for each
		flag returned.  Add new strings for each flag to the xlnid_priv_flags_strings
		array.
	**/

/*
		Function:
		xlnid_get_priv_flags
		Purpose:
		report device private flags
		Parameters:
		netdev  - (IN) device information
		Returns:
		priv_flags  bitmap of flags
*/
static u32 xlnid_get_priv_flags(struct net_device *netdev)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	u32 priv_flags = 0;

	if (adapter->flags & XLNID_FLAG_FDIR_HASH_CAPABLE) {
		priv_flags |= XLNID_PRIV_FLAGS_FD_ATR;
	}

#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC

	if (adapter->flags2 & XLNID_FLAG2_RX_LEGACY) {
		priv_flags |= XLNID_PRIV_FLAGS_LEGACY_RX;
	}

#endif

	return priv_flags;
}

/*
		Function:
		xlnid_set_priv_flags
		Purpose:
		set private flags
		Parameters:
		priv_flags  - (IN) bit flags to be set
		Returns:
		0         success
		otherwise  fail
*/
static int xlnid_set_priv_flags(struct net_device *netdev, u32 priv_flags)
{
	struct xlnid_adapter *adapter = netdev_priv(netdev);
	unsigned int flags2 = adapter->flags2;
	unsigned int flags = adapter->flags;

	/*  allow the user to control the state of the Flow
		Director ATR (Application Targeted Routing) feature
		of the driver
	*/
	flags &= ~XLNID_FLAG_FDIR_HASH_CAPABLE;

	if (priv_flags & XLNID_PRIV_FLAGS_FD_ATR) {
		/* We cannot enable ATR if VMDq is enabled */
		if (flags & XLNID_FLAG_VMDQ_ENABLED) {
			return -EINVAL;
		}

		/* We cannot enable ATR if we have 2 or more traffic classes */
		if (netdev_get_num_tc(netdev) > 1) {
			return -EINVAL;
		}

		/* We cannot enable ATR if RSS is disabled */
		if (adapter->ring_feature[RING_F_RSS].limit <= 1) {
			return -EINVAL;
		}

		/* A sample rate of 0 indicates ATR disabled */
		if (!adapter->atr_sample_rate) {
			return -EINVAL;
		}

		flags |= XLNID_FLAG_FDIR_HASH_CAPABLE;
	}

#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC
	flags2 &= ~XLNID_FLAG2_RX_LEGACY;

	if (priv_flags & XLNID_PRIV_FLAGS_LEGACY_RX) {
		flags2 |= XLNID_FLAG2_RX_LEGACY;
	}

#endif

	if (flags != adapter->flags) {
		adapter->flags = flags;

		/* ATR state change requires a reset */
		xlnid_do_reset(netdev);
	}

	else if (flags2 != adapter->flags2) {
		adapter->flags2 = flags2;

		/* reset interface to repopulate queues */
		if (netif_running(netdev)) {
			xlnid_reinit_locked(adapter);
		}
	}

	return 0;
}

#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */

static struct ethtool_ops xlnid_ethtool_ops = {
#ifdef ETHTOOL_GLINKSETTINGS
	.get_link_ksettings = xlnid_get_link_ksettings,
	.set_link_ksettings = xlnid_set_link_ksettings,
#else
	.get_settings = xlnid_get_settings,
	.set_settings = xlnid_set_settings,
#endif
	.get_drvinfo = xlnid_get_drvinfo,
	.get_regs_len = xlnid_get_regs_len,
	.get_regs = xlnid_get_regs,
	.get_wol = xlnid_get_wol,
	.set_wol = xlnid_set_wol,
	.nway_reset = xlnid_nway_reset,
	.get_link = ethtool_op_get_link,
	.get_eeprom_len = xlnid_get_eeprom_len,
	.get_eeprom = xlnid_get_eeprom,
	.set_eeprom = xlnid_set_eeprom,
	.get_ringparam = xlnid_get_ringparam,
	.set_ringparam = xlnid_set_ringparam,
	.get_pauseparam = xlnid_get_pauseparam,
	.set_pauseparam = xlnid_set_pauseparam,
	.get_msglevel = xlnid_get_msglevel,
	.set_msglevel = xlnid_set_msglevel,
#ifndef HAVE_ETHTOOL_GET_SSET_COUNT
	.self_test_count = xlnid_diag_test_count,
#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */
	.self_test = xlnid_diag_test,
	.get_strings = xlnid_get_strings,
#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
#ifdef HAVE_ETHTOOL_SET_PHYS_ID
	.set_phys_id = xlnid_set_phys_id,
#else
	.phys_id = xlnid_phys_id,
#endif /* HAVE_ETHTOOL_SET_PHYS_ID */
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
#ifndef HAVE_ETHTOOL_GET_SSET_COUNT
	.get_stats_count = xlnid_get_stats_count,
#else /* HAVE_ETHTOOL_GET_SSET_COUNT */
	.get_sset_count = xlnid_get_sset_count,
	.get_priv_flags = xlnid_get_priv_flags,
	.set_priv_flags = xlnid_set_priv_flags,
#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */
	.get_ethtool_stats = xlnid_get_ethtool_stats,
#ifdef HAVE_ETHTOOL_GET_PERM_ADDR
	.get_perm_addr = ethtool_op_get_perm_addr,
#endif
	.get_coalesce = xlnid_get_coalesce,
	.set_coalesce = xlnid_set_coalesce,
#ifdef ETHTOOL_COALESCE_USECS
	.supported_coalesce_params = ETHTOOL_COALESCE_USECS,
#endif
#ifndef HAVE_NDO_SET_FEATURES
	.get_rx_csum = xlnid_get_rx_csum,
	.set_rx_csum = xlnid_set_rx_csum,
	.get_tx_csum = ethtool_op_get_tx_csum,
	.set_tx_csum = xlnid_set_tx_csum,
	.get_sg = ethtool_op_get_sg,
	.set_sg = ethtool_op_set_sg,
#ifdef NETIF_F_TSO
	.get_tso = ethtool_op_get_tso,
	.set_tso = xlnid_set_tso,
#endif
#ifdef ETHTOOL_GFLAGS
	.get_flags = ethtool_op_get_flags,
	.set_flags = xlnid_set_flags,
#endif
#endif /* HAVE_NDO_SET_FEATURES */
#ifdef ETHTOOL_GRXRINGS
	.get_rxnfc = xlnid_get_rxnfc,
	.set_rxnfc = xlnid_set_rxnfc,
#ifdef ETHTOOL_SRXNTUPLE
	.set_rx_ntuple = xlnid_set_rx_ntuple,
#endif
#endif /* ETHTOOL_GRXRINGS */
#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
#ifdef ETHTOOL_GEEE
#ifdef HAVE_ETHTOOL_KEEE
	.get_eee = xlnid_get_keee,
#else
	.get_eee = xlnid_get_eee,
#endif /* HAVE_ETHTOOL_KEEE */
#endif /* ETHTOOL_GEEE */
#ifdef ETHTOOL_SEEE
#ifdef HAVE_ETHTOOL_KEEE
	.set_eee = xlnid_set_keee,
#else
	.set_eee = xlnid_set_eee,
#endif /* HAVE_ETHTOOL_KEEE */
#endif /* ETHTOOL_SEEE */
#ifdef ETHTOOL_SCHANNELS
	.get_channels = xlnid_get_channels,
	.set_channels = xlnid_set_channels,
#endif
#if 0
	.get_module_info    = xlnid_get_module_info,
	.get_module_eeprom  = xlnid_get_module_eeprom,
#endif
#ifdef HAVE_ETHTOOL_GET_TS_INFO
	.get_ts_info = xlnid_get_ts_info,
#endif
#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
	.get_rxfh_indir_size = xlnid_rss_indir_size,
	.get_rxfh_key_size = xlnid_get_rxfh_key_size,
	.get_rxfh = xlnid_get_rxfh,
	.set_rxfh = xlnid_set_rxfh,
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
};

#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
static const struct ethtool_ops_ext xlnid_ethtool_ops_ext = {
	.size = sizeof(struct ethtool_ops_ext),
	.get_ts_info = xlnid_get_ts_info,
	.set_phys_id = xlnid_set_phys_id,
	.get_channels = xlnid_get_channels,
	.set_channels = xlnid_set_channels,
#if 0
	.get_module_info    = xlnid_get_module_info,
	.get_module_eeprom  = xlnid_get_module_eeprom,
#endif
#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
	.get_rxfh_indir_size = xlnid_rss_indir_size,
	.get_rxfh_key_size = xlnid_get_rxfh_key_size,
	.get_rxfh = xlnid_get_rxfh,
	.set_rxfh = xlnid_set_rxfh,
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */
#ifdef ETHTOOL_GEEE
	.get_eee = xlnid_get_eee,
#endif /* ETHTOOL_GEEE */
#ifdef ETHTOOL_SEEE
	.set_eee = xlnid_set_eee,
#endif /* ETHTOOL_SEEE */
};

#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
void xlnid_set_ethtool_ops(struct net_device *netdev)
{
#ifndef ETHTOOL_OPS_COMPAT
	netdev->ethtool_ops = &xlnid_ethtool_ops;
#else
	SET_ETHTOOL_OPS(netdev, &xlnid_ethtool_ops);
#endif /* ETHTOOL_OPS_COMPAT */

#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
	set_ethtool_ops_ext(netdev, &xlnid_ethtool_ops_ext);
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
}
#endif /* SIOCETHTOOL */
