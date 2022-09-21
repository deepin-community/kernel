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

/* ethtool support for ngbe */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>
#include <linux/firmware.h>
#ifdef SIOCETHTOOL
#include <asm/uaccess.h>

#include "ngbe.h"
#include "ngbe_hw.h"
#include "ngbe_phy.h"
#ifdef HAVE_ETHTOOL_GET_TS_INFO
#include <linux/net_tstamp.h>
#endif

#ifndef ETH_GSTRING_LEN
#define ETH_GSTRING_LEN 32
#endif

#define NGBE_ALL_RAR_ENTRIES 16

#ifdef ETHTOOL_OPS_COMPAT
#include "kcompat_ethtool.c"
#endif

#ifdef ETHTOOL_GSTATS
struct ngbe_stats {
	char stat_string[ETH_GSTRING_LEN];
	int sizeof_stat;
	int stat_offset;
};

#define NGBE_NETDEV_STAT(_net_stat) { \
	.stat_string = #_net_stat, \
	.sizeof_stat = sizeof_field(struct net_device_stats, _net_stat), \
	.stat_offset = offsetof(struct net_device_stats, _net_stat) \
}
static const struct ngbe_stats ngbe_gstrings_net_stats[] = {
	NGBE_NETDEV_STAT(rx_packets),
	NGBE_NETDEV_STAT(tx_packets),
	NGBE_NETDEV_STAT(rx_bytes),
	NGBE_NETDEV_STAT(tx_bytes),
	NGBE_NETDEV_STAT(rx_errors),
	NGBE_NETDEV_STAT(tx_errors),
	NGBE_NETDEV_STAT(rx_dropped),
	NGBE_NETDEV_STAT(tx_dropped),
	NGBE_NETDEV_STAT(multicast),
	NGBE_NETDEV_STAT(collisions),
	NGBE_NETDEV_STAT(rx_over_errors),
	NGBE_NETDEV_STAT(rx_crc_errors),
	NGBE_NETDEV_STAT(rx_frame_errors),
	NGBE_NETDEV_STAT(rx_fifo_errors),
	NGBE_NETDEV_STAT(rx_missed_errors),
	NGBE_NETDEV_STAT(tx_aborted_errors),
	NGBE_NETDEV_STAT(tx_carrier_errors),
	NGBE_NETDEV_STAT(tx_fifo_errors),
	NGBE_NETDEV_STAT(tx_heartbeat_errors),
};

#define NGBE_STAT(_name, _stat) { \
	.stat_string = _name, \
	.sizeof_stat = sizeof_field(struct ngbe_adapter, _stat), \
	.stat_offset = offsetof(struct ngbe_adapter, _stat) \
}
static struct ngbe_stats ngbe_gstrings_stats[] = {
	NGBE_STAT("rx_pkts_nic", stats.gprc),
	NGBE_STAT("tx_pkts_nic", stats.gptc),
	NGBE_STAT("rx_bytes_nic", stats.gorc),
	NGBE_STAT("tx_bytes_nic", stats.gotc),
	NGBE_STAT("lsc_int", lsc_int),
	NGBE_STAT("tx_busy", tx_busy),
	NGBE_STAT("non_eop_descs", non_eop_descs),
	NGBE_STAT("broadcast", stats.bprc),
	NGBE_STAT("rx_no_buffer_count", stats.rnbc[0]),
	NGBE_STAT("tx_timeout_count", tx_timeout_count),
	NGBE_STAT("tx_restart_queue", restart_queue),
	NGBE_STAT("rx_long_length_count", stats.roc),
	NGBE_STAT("rx_short_length_count", stats.ruc),
	NGBE_STAT("tx_flow_control_xon", stats.lxontxc),
	NGBE_STAT("rx_flow_control_xon", stats.lxonrxc),
	NGBE_STAT("tx_flow_control_xoff", stats.lxofftxc),
	NGBE_STAT("rx_flow_control_xoff", stats.lxoffrxc),
	NGBE_STAT("rx_csum_offload_good_count", hw_csum_rx_good),
	NGBE_STAT("rx_csum_offload_errors", hw_csum_rx_error),
	NGBE_STAT("alloc_rx_page_failed", alloc_rx_page_failed),
	NGBE_STAT("alloc_rx_buff_failed", alloc_rx_buff_failed),
#ifndef NGBE_NO_LRO
	NGBE_STAT("lro_aggregated", lro_stats.coal),
	NGBE_STAT("lro_flushed", lro_stats.flushed),
#endif /* NGBE_NO_LRO */
	NGBE_STAT("rx_no_dma_resources", hw_rx_no_dma_resources),
	NGBE_STAT("os2bmc_rx_by_bmc", stats.o2bgptc),
	NGBE_STAT("os2bmc_tx_by_bmc", stats.b2ospc),
	NGBE_STAT("os2bmc_tx_by_host", stats.o2bspc),
	NGBE_STAT("os2bmc_rx_by_host", stats.b2ogprc),
#ifdef HAVE_PTP_1588_CLOCK
	NGBE_STAT("tx_hwtstamp_timeouts", tx_hwtstamp_timeouts),
	NGBE_STAT("rx_hwtstamp_cleared", rx_hwtstamp_cleared),
#endif /* HAVE_PTP_1588_CLOCK */
};

/* ngbe allocates num_tx_queues and num_rx_queues symmetrically so
 * we set the num_rx_queues to evaluate to num_tx_queues. This is
 * used because we do not have a good way to get the max number of
 * rx queues with CONFIG_RPS disabled.
 */
#ifdef HAVE_TX_MQ
#ifdef HAVE_NETDEV_SELECT_QUEUE
#define NGBE_NUM_RX_QUEUES netdev->num_tx_queues
#define NGBE_NUM_TX_QUEUES netdev->num_tx_queues
#else
#define NGBE_NUM_RX_QUEUES adapter->indices
#define NGBE_NUM_TX_QUEUES adapter->indices
#endif /* HAVE_NETDEV_SELECT_QUEUE */
#else /* HAVE_TX_MQ */
#define NGBE_NUM_RX_QUEUES 1
#define NGBE_NUM_TX_QUEUES ( \
		((struct ngbe_adapter *)netdev_priv(netdev))->num_tx_queues)
#endif /* HAVE_TX_MQ */

#define NGBE_QUEUE_STATS_LEN ( \
		(NGBE_NUM_TX_QUEUES + NGBE_NUM_RX_QUEUES) * \
		(sizeof(struct ngbe_queue_stats) / sizeof(u64)))
#define NGBE_GLOBAL_STATS_LEN  ARRAY_SIZE(ngbe_gstrings_stats)
#define NGBE_NETDEV_STATS_LEN  ARRAY_SIZE(ngbe_gstrings_net_stats)
#define NGBE_PB_STATS_LEN ( \
		(sizeof(((struct ngbe_adapter *)0)->stats.pxonrxc) + \
		 sizeof(((struct ngbe_adapter *)0)->stats.pxontxc) + \
		 sizeof(((struct ngbe_adapter *)0)->stats.pxoffrxc) + \
		 sizeof(((struct ngbe_adapter *)0)->stats.pxofftxc)) \
		/ sizeof(u64))
#define NGBE_VF_STATS_LEN \
	((((struct ngbe_adapter *)netdev_priv(netdev))->num_vfs) * \
	  (sizeof(struct vf_stats) / sizeof(u64)))
#define NGBE_STATS_LEN (NGBE_GLOBAL_STATS_LEN + \
			 NGBE_NETDEV_STATS_LEN + \
			 NGBE_PB_STATS_LEN + \
			 NGBE_QUEUE_STATS_LEN + \
			 NGBE_VF_STATS_LEN)

#endif /* ETHTOOL_GSTATS */
#ifdef ETHTOOL_TEST
static const char ngbe_gstrings_test[][ETH_GSTRING_LEN] = {
	"Register test  (offline)", "Eeprom test    (offline)",
	"Interrupt test (offline)", "Loopback test  (offline)",
	"Link test   (on/offline)"
};
#define NGBE_TEST_LEN  (sizeof(ngbe_gstrings_test) / ETH_GSTRING_LEN)
#endif /* ETHTOOL_TEST */

#define ngbe_isbackplane(type)  \
			((type == ngbe_media_type_backplane) ? true : false)


#ifdef HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE
int ngbe_get_link_ksettings(struct net_device *netdev,
		    struct ethtool_link_ksettings *cmd)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	u32 supported_link = 0;
	u32 link_speed = 0;
	bool autoneg = false;
	u32 supported, advertising = 0;
	bool link_up = 0;
	u16 value = 0;
	
	ethtool_convert_link_mode_to_legacy_u32(&supported,
						cmd->link_modes.supported);

	TCALL(hw, mac.ops.get_link_capabilities, &supported_link, &autoneg);

	/* set the supported link speeds */
	if (supported_link & NGBE_LINK_SPEED_1GB_FULL)
		supported |= (ngbe_isbackplane(hw->phy.media_type)) ?
			SUPPORTED_1000baseKX_Full : SUPPORTED_1000baseT_Full;
	if (supported_link & NGBE_LINK_SPEED_100_FULL)
		supported |= SUPPORTED_100baseT_Full;
	if (supported_link & NGBE_LINK_SPEED_10_FULL)
		supported |= SUPPORTED_10baseT_Full;

	/* set the advertised speeds */
	if (hw->phy.autoneg_advertised) {
		if (hw->phy.autoneg_advertised & NGBE_LINK_SPEED_100_FULL)
			advertising |= ADVERTISED_100baseT_Full;
		if (hw->phy.autoneg_advertised & NGBE_LINK_SPEED_1GB_FULL) {
			if (supported & SUPPORTED_1000baseKX_Full)
				advertising |= ADVERTISED_1000baseKX_Full;
			else
				advertising |= ADVERTISED_1000baseT_Full;
		}
		if (hw->phy.autoneg_advertised & NGBE_LINK_SPEED_10_FULL)
			advertising |= ADVERTISED_10baseT_Full;
	}
	supported |= SUPPORTED_Autoneg;
	if (autoneg) {
		advertising |= ADVERTISED_Autoneg;
		autoneg = AUTONEG_ENABLE;
		cmd->base.autoneg = AUTONEG_ENABLE;
	} else
		cmd->base.autoneg = AUTONEG_DISABLE;

	/* Determine the remaining settings based on the PHY type. */
	switch (adapter->hw.phy.type) {
	case ngbe_phy_internal:
	case ngbe_phy_m88e1512:
	case ngbe_phy_zte:
		supported |= SUPPORTED_TP;
		advertising |= ADVERTISED_TP;
		cmd->base.port = PORT_TP;
		break;
	case ngbe_phy_sfp_passive_tyco:
	case ngbe_phy_sfp_passive_unknown:
	case ngbe_phy_sfp_ftl:
	case ngbe_phy_sfp_avago:
	case ngbe_phy_sfp_intel:
	case ngbe_phy_sfp_unknown:
		switch (adapter->hw.phy.sfp_type) {
			/* SFP+ devices, further checking needed */
		case ngbe_sfp_type_da_cu:
		case ngbe_sfp_type_da_cu_core0:
		case ngbe_sfp_type_da_cu_core1:
			supported |= SUPPORTED_FIBRE;
			advertising |= ADVERTISED_FIBRE;
			cmd->base.port = PORT_DA;
			break;
		case ngbe_sfp_type_sr:
		case ngbe_sfp_type_lr:
		case ngbe_sfp_type_srlr_core0:
		case ngbe_sfp_type_srlr_core1:
		case ngbe_sfp_type_1g_sx_core0:
		case ngbe_sfp_type_1g_sx_core1:
		case ngbe_sfp_type_1g_lx_core0:
		case ngbe_sfp_type_1g_lx_core1:
			supported |= SUPPORTED_FIBRE;
			advertising |= ADVERTISED_FIBRE;
			cmd->base.port = PORT_FIBRE;
			break;
		case ngbe_sfp_type_not_present:
			supported |= SUPPORTED_FIBRE;
			advertising |= ADVERTISED_FIBRE;
			cmd->base.port = PORT_NONE;
			break;
		case ngbe_sfp_type_1g_cu_core0:
		case ngbe_sfp_type_1g_cu_core1:
			supported |= SUPPORTED_TP;
			advertising |= ADVERTISED_TP;
			cmd->base.port = PORT_TP;
			break;
		case ngbe_sfp_type_unknown:
		default:
			supported |= SUPPORTED_FIBRE;
			advertising |= ADVERTISED_FIBRE;
			cmd->base.port = PORT_OTHER;
			break;
		}
		break;
	case ngbe_phy_yt8521s_sfi:
		ngbe_phy_read_reg_ext_yt8521s(hw, 0xA001, 0, &value);
		if((value & 7) == 0) {//utp_to_rgmii
			supported |= SUPPORTED_TP;
			advertising |= ADVERTISED_TP;
			cmd->base.port = PORT_TP;			
		} else {
			supported |= SUPPORTED_FIBRE;
			advertising |= ADVERTISED_FIBRE;
			cmd->base.port = PORT_FIBRE;			
		}
		break;
	case ngbe_phy_internal_yt8521s_sfi:
		supported |= SUPPORTED_FIBRE;
		advertising |= ADVERTISED_FIBRE;
		cmd->base.port = PORT_FIBRE;			
		break;
	case ngbe_phy_unknown:
	case ngbe_phy_generic:
	case ngbe_phy_sfp_unsupported:
	default:
		supported |= SUPPORTED_FIBRE;
		advertising |= ADVERTISED_FIBRE;
		cmd->base.port = PORT_OTHER;
		break;
	}

	if (!in_interrupt()) {
		TCALL(hw, mac.ops.check_link, &link_speed, &link_up, false);
	} else {
		/*
		 * this case is a special workaround for RHEL5 bonding
		 * that calls this routine from interrupt context
		 */
		link_speed = adapter->link_speed;
		link_up = adapter->link_up;
	}

	supported |= SUPPORTED_Pause;

	switch (hw->fc.requested_mode) {
	case ngbe_fc_full:
		advertising |= ADVERTISED_Pause;
		break;
	case ngbe_fc_rx_pause:
		advertising |= ADVERTISED_Pause |
				     ADVERTISED_Asym_Pause;
		break;
	case ngbe_fc_tx_pause:
		advertising |= ADVERTISED_Asym_Pause;
		break;
	default:
		advertising &= ~(ADVERTISED_Pause |
				       ADVERTISED_Asym_Pause);
	}

	if (link_up) {
		switch (link_speed) {
		case NGBE_LINK_SPEED_1GB_FULL:
			cmd->base.speed = SPEED_1000;
			break;
		case NGBE_LINK_SPEED_100_FULL:
			cmd->base.speed = SPEED_100;
			break;
		case NGBE_LINK_SPEED_10_FULL:
			cmd->base.speed = SPEED_10;
			break;
		default:
			break;
		}
		cmd->base.duplex = DUPLEX_FULL;
	} else {
		cmd->base.speed = -1;
		cmd->base.duplex = -1;
	}

	ethtool_convert_legacy_u32_to_link_mode(cmd->link_modes.supported,
						supported);
	ethtool_convert_legacy_u32_to_link_mode(cmd->link_modes.advertising,
						advertising);
	return 0;
}
#else /* !HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE */
int ngbe_get_settings(struct net_device *netdev,
					struct ethtool_cmd *ecmd)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	u32 supported_link = 0;
	u32 link_speed = 0;
	bool autoneg = false;
	bool link_up = 0;
	u16 value = 0;
	
	TCALL(hw, mac.ops.get_link_capabilities, &supported_link, &autoneg);

	/* set the supported link speeds */
	if (supported_link & NGBE_LINK_SPEED_1GB_FULL)
		ecmd->supported |= (ngbe_isbackplane(hw->phy.media_type)) ?
			SUPPORTED_1000baseKX_Full : SUPPORTED_1000baseT_Full;
	if (supported_link & NGBE_LINK_SPEED_100_FULL)
		ecmd->supported |= SUPPORTED_100baseT_Full;
	if (supported_link & NGBE_LINK_SPEED_10_FULL)
		ecmd->supported |= SUPPORTED_10baseT_Full;

	/* set the advertised speeds */
	if (hw->phy.autoneg_advertised) {
		if (hw->phy.autoneg_advertised & NGBE_LINK_SPEED_100_FULL)
			ecmd->advertising |= ADVERTISED_100baseT_Full;
		if (hw->phy.autoneg_advertised & NGBE_LINK_SPEED_1GB_FULL) {
			if (ecmd->supported & SUPPORTED_1000baseKX_Full)
				ecmd->advertising |= ADVERTISED_1000baseKX_Full;
			else
				ecmd->advertising |= ADVERTISED_1000baseT_Full;
		}
		if (hw->phy.autoneg_advertised & NGBE_LINK_SPEED_10_FULL)
			ecmd->advertising |= ADVERTISED_10baseT_Full;
	}

	ecmd->supported |= SUPPORTED_Autoneg;
	if (autoneg) {
		ecmd->advertising |= ADVERTISED_Autoneg;
		ecmd->autoneg = AUTONEG_ENABLE;
	} else
		ecmd->autoneg = AUTONEG_DISABLE;

	ecmd->transceiver = XCVR_EXTERNAL;

	/* Determine the remaining settings based on the PHY type. */
	switch (adapter->hw.phy.type) {
	case ngbe_phy_internal:
	case ngbe_phy_m88e1512:
	case ngbe_phy_zte:
		ecmd->supported |= SUPPORTED_TP;
		ecmd->advertising |= ADVERTISED_TP;
		ecmd->port = PORT_TP;
		break;
	case ngbe_phy_sfp_passive_tyco:
	case ngbe_phy_sfp_passive_unknown:
	case ngbe_phy_sfp_ftl:
	case ngbe_phy_sfp_avago:
	case ngbe_phy_sfp_intel:
	case ngbe_phy_sfp_unknown:
		switch (adapter->hw.phy.sfp_type) {
			/* SFP+ devices, further checking needed */
		case ngbe_sfp_type_da_cu:
		case ngbe_sfp_type_da_cu_core0:
		case ngbe_sfp_type_da_cu_core1:
			ecmd->supported |= SUPPORTED_FIBRE;
			ecmd->advertising |= ADVERTISED_FIBRE;
			ecmd->port = PORT_DA;
			break;
		case ngbe_sfp_type_sr:
		case ngbe_sfp_type_lr:
		case ngbe_sfp_type_srlr_core0:
		case ngbe_sfp_type_srlr_core1:
		case ngbe_sfp_type_1g_sx_core0:
		case ngbe_sfp_type_1g_sx_core1:
		case ngbe_sfp_type_1g_lx_core0:
		case ngbe_sfp_type_1g_lx_core1:
			ecmd->supported |= SUPPORTED_FIBRE;
			ecmd->advertising |= ADVERTISED_FIBRE;
			ecmd->port = PORT_FIBRE;
			break;
		case ngbe_sfp_type_not_present:
			ecmd->supported |= SUPPORTED_FIBRE;
			ecmd->advertising |= ADVERTISED_FIBRE;
			ecmd->port = PORT_NONE;
			break;
		case ngbe_sfp_type_1g_cu_core0:
		case ngbe_sfp_type_1g_cu_core1:
			ecmd->supported |= SUPPORTED_TP;
			ecmd->advertising |= ADVERTISED_TP;
			ecmd->port = PORT_TP;
			break;
		case ngbe_sfp_type_unknown:
		default:
			ecmd->supported |= SUPPORTED_FIBRE;
			ecmd->advertising |= ADVERTISED_FIBRE;
			ecmd->port = PORT_OTHER;
			break;
		}
		break;
	case ngbe_phy_yt8521s_sfi:
	ngbe_phy_read_reg_ext_yt8521s(hw, 0xA001, 0, &value);
	if((value & 7) == 0) {//utp_to_rgmii
		ecmd->supported |= SUPPORTED_TP;
		ecmd->advertising |= ADVERTISED_TP;
		ecmd->port = PORT_TP;		
	} else {
		ecmd->supported |= SUPPORTED_FIBRE;
		ecmd->advertising |= ADVERTISED_FIBRE;
		ecmd->port = PORT_FIBRE;		
	}
	break;
	case ngbe_phy_internal_yt8521s_sfi:
		ecmd->supported |= SUPPORTED_FIBRE;
		ecmd->advertising |= ADVERTISED_FIBRE;
		ecmd->port = PORT_FIBRE;				
		break;
	case ngbe_phy_unknown:
	case ngbe_phy_generic:
	case ngbe_phy_sfp_unsupported:
	default:
		ecmd->supported |= SUPPORTED_FIBRE;
		ecmd->advertising |= ADVERTISED_FIBRE;
		ecmd->port = PORT_OTHER;
		break;
	}

	if (!in_interrupt()) {
		TCALL(hw, mac.ops.check_link, &link_speed, &link_up, false);
	} else {
		/*
		 * this case is a special workaround for RHEL5 bonding
		 * that calls this routine from interrupt context
		 */
		link_speed = adapter->link_speed;
		link_up = adapter->link_up;
	}

	ecmd->supported |= SUPPORTED_Pause;

	switch (hw->fc.requested_mode) {
	case ngbe_fc_full:
		ecmd->advertising |= ADVERTISED_Pause;
		break;
	case ngbe_fc_rx_pause:
		ecmd->advertising |= ADVERTISED_Pause |
				     ADVERTISED_Asym_Pause;
		break;
	case ngbe_fc_tx_pause:
		ecmd->advertising |= ADVERTISED_Asym_Pause;
		break;
	default:
		ecmd->advertising &= ~(ADVERTISED_Pause |
				       ADVERTISED_Asym_Pause);
	}

	if (link_up) {
		switch (link_speed) {
		case NGBE_LINK_SPEED_1GB_FULL:
			ecmd->speed = SPEED_1000;
			break;
		case NGBE_LINK_SPEED_100_FULL:
			ecmd->speed = SPEED_100;
			break;
		case NGBE_LINK_SPEED_10_FULL:
			ecmd->speed = SPEED_10;
			break;
		default:
			break;
		}
		ecmd->duplex = DUPLEX_FULL;
	} else {
		ecmd->speed = -1;
		ecmd->duplex = -1;
	}

	return 0;
}
#endif /* !HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE */

#ifdef HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE
static int ngbe_set_link_ksettings(struct net_device *netdev,
				const struct ethtool_link_ksettings *cmd)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	u32 advertised, old;
	s32 err = 0;
	u32 supported, advertising;
	ethtool_convert_link_mode_to_legacy_u32(&supported,
						cmd->link_modes.supported);
	ethtool_convert_link_mode_to_legacy_u32(&advertising,
						cmd->link_modes.advertising);

	if ((hw->phy.media_type == ngbe_media_type_copper) ||
	    (hw->phy.multispeed_fiber)) {
		/*
		 * this function does not support duplex forcing, but can
		 * limit the advertising of the adapter to the specified speed
		 */
		if (advertising & ~supported) {
			return -EINVAL;
		}
		old = hw->phy.autoneg_advertised;
		advertised = 0;

		if (cmd->base.autoneg == AUTONEG_ENABLE) {
			hw->mac.autoneg = true;
			if (advertising & ADVERTISED_1000baseT_Full)
				advertised |= NGBE_LINK_SPEED_1GB_FULL;

			if (advertising & ADVERTISED_100baseT_Full)
				advertised |= NGBE_LINK_SPEED_100_FULL;

			if (advertising & ADVERTISED_10baseT_Full)
				advertised |= NGBE_LINK_SPEED_10_FULL;

			if (old == advertised) {
				return err;
			}
		} else {
			if (cmd->base.duplex == DUPLEX_HALF) {
				e_err(probe, "unsupported duplex\n");
				return -EINVAL;
			}

			switch (cmd->base.speed) {
				case SPEED_10:
					advertised = NGBE_LINK_SPEED_10_FULL;
					break;
				case SPEED_100:
					advertised = NGBE_LINK_SPEED_100_FULL;
					break;
				case SPEED_1000:
					advertised = NGBE_LINK_SPEED_1GB_FULL;
					break;
				default:
					e_err(probe, "unsupported speed\n");
					return -EINVAL;
			}

			hw->mac.autoneg = false;
		}

		hw->mac.autotry_restart = true;
		TCALL(hw, mac.ops.setup_link, advertised, true);
		if (cmd->base.autoneg == AUTONEG_ENABLE) {
			hw->phy.autoneg_advertised = advertised;
		} else {
			hw->phy.autoneg_advertised = NGBE_LINK_SPEED_UNKNOWN;
			hw->phy.force_speed = advertised;
		}
	} else {
		/* in this case we currently only support 1Gb/FULL */
		u32 speed = cmd->base.speed;
		if ((cmd->base.autoneg == AUTONEG_ENABLE) ||
		    (advertising != ADVERTISED_10000baseT_Full) ||
		    (speed + cmd->base.duplex != SPEED_10000 + DUPLEX_FULL))
			return -EINVAL;
	}

	return err;
}

#else /* !HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE */
static int ngbe_set_settings(struct net_device *netdev,
							 struct ethtool_cmd *ecmd)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	u32 advertised, old;
	s32 err = 0;

	if ((hw->phy.media_type == ngbe_media_type_copper) ||
		(hw->phy.multispeed_fiber)) {
		/*
		 * this function does not support duplex forcing, but can
		 * limit the advertising of the adapter to the specified speed
		 */
		if (ecmd->advertising & ~ecmd->supported) {
			return -EINVAL;
		}

		old = hw->phy.autoneg_advertised;
		advertised = 0;

		if (ecmd->autoneg == AUTONEG_ENABLE) {
			hw->mac.autoneg = true;
			if (ecmd->advertising & ADVERTISED_1000baseT_Full)
				advertised |= NGBE_LINK_SPEED_1GB_FULL;

			if (ecmd->advertising & ADVERTISED_100baseT_Full)
				advertised |= NGBE_LINK_SPEED_100_FULL;

			if (ecmd->advertising & ADVERTISED_10baseT_Full)
				advertised |= NGBE_LINK_SPEED_10_FULL;

			if (old == advertised) {
				return err;
			}
		} else {
			if (ecmd->duplex == DUPLEX_HALF) {
				e_err(probe, "unsupported duplex\n");
				return -EINVAL;
			}

			switch (ecmd->speed) {
			case SPEED_10:
				advertised = NGBE_LINK_SPEED_10_FULL;
				break;
			case SPEED_100:
				advertised = NGBE_LINK_SPEED_100_FULL;
				break;
			case SPEED_1000:
				advertised = NGBE_LINK_SPEED_1GB_FULL;
				break;
			default:
				e_err(probe, "unsupported speed\n");
				return -EINVAL;
			}
			hw->mac.autoneg = false;
		}

		hw->mac.autotry_restart = true;
		TCALL(hw, mac.ops.setup_link, advertised, true);
		if (ecmd->autoneg == AUTONEG_ENABLE) {
			hw->phy.autoneg_advertised = advertised;
		} else {
			hw->phy.autoneg_advertised = NGBE_LINK_SPEED_UNKNOWN;
			hw->phy.force_speed = advertised;
		}
	} else {
		/* in this case we currently only support 10Gb/FULL */
		u32 speed = ethtool_cmd_speed(ecmd);
		if ((ecmd->autoneg == AUTONEG_ENABLE) ||
		    (ecmd->advertising != ADVERTISED_10000baseT_Full) ||
		    (speed + ecmd->duplex != SPEED_10000 + DUPLEX_FULL))
			return -EINVAL;
	}

	return err;
}
#endif /* !HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE */

static void ngbe_get_pauseparam(struct net_device *netdev,
				 struct ethtool_pauseparam *pause)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;

	if (!hw->fc.disable_fc_autoneg)
		pause->autoneg = 1;
	else
		pause->autoneg = 0;

	if (hw->fc.current_mode == ngbe_fc_rx_pause) {
		pause->rx_pause = 1;
	} else if (hw->fc.current_mode == ngbe_fc_tx_pause) {
		pause->tx_pause = 1;
	} else if (hw->fc.current_mode == ngbe_fc_full) {
		pause->rx_pause = 1;
		pause->tx_pause = 1;
	}
}

static int ngbe_set_pauseparam(struct net_device *netdev,
				struct ethtool_pauseparam *pause)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	struct ngbe_fc_info fc = hw->fc;

	fc.disable_fc_autoneg = (pause->autoneg != AUTONEG_ENABLE);

	if ((pause->rx_pause && pause->tx_pause) || pause->autoneg)
		fc.requested_mode = ngbe_fc_full;
	else if (pause->rx_pause)
		fc.requested_mode = ngbe_fc_rx_pause;
	else if (pause->tx_pause)
		fc.requested_mode = ngbe_fc_tx_pause;
	else
		fc.requested_mode = ngbe_fc_none;

	/* if the thing changed then we'll update and use new autoneg */
	if (memcmp(&fc, &hw->fc, sizeof(struct ngbe_fc_info))) {
		hw->fc = fc;
		if (netif_running(netdev))
			ngbe_reinit_locked(adapter);
		else
			ngbe_reset(adapter);
	}

	return 0;
}

static u32 ngbe_get_msglevel(struct net_device *netdev)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	return adapter->msg_enable;
}

static void ngbe_set_msglevel(struct net_device *netdev, u32 data)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	adapter->msg_enable = data;
}

static int ngbe_get_regs_len(struct net_device __always_unused *netdev)
{
#define NGBE_REGS_LEN  4096
	return NGBE_REGS_LEN * sizeof(u32);
}

#define NGBE_GET_STAT(_A_, _R_)        (_A_->stats._R_)


static void ngbe_get_regs(struct net_device *netdev,
						struct ethtool_regs *regs,
						void *p)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	u32 *regs_buff = p;
	u32 i;
	u32 id = 0;

	memset(p, 0, NGBE_REGS_LEN * sizeof(u32));
	regs_buff[NGBE_REGS_LEN - 1] = 0x55555555;

	regs->version = hw->revision_id << 16 |
			hw->device_id;

	/* Global Registers */
	/* chip control */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MIS_PWR);//0
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MIS_CTL);//1
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MIS_PF_SM);//2
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MIS_RST);//3
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MIS_ST);//4
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MIS_SWSM);//5
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MIS_RST_ST);//6
	/* pvt sensor */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TS_CTL);//7
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TS_EN);//8
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TS_ST);//9
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TS_ALARM_THRE);//10
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TS_DALARM_THRE);//11
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TS_INT_EN);//12
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TS_ALARM_ST);//13
	/* Fmgr Register */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_SPI_CMD);//14
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_SPI_DATA);//15
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_SPI_STATUS);//16
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_SPI_USR_CMD);//17
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_SPI_CMDCFG0);//18
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_SPI_CMDCFG1);//19
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_SPI_ILDR_STATUS);//20
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_SPI_ILDR_SWPTR);//21

	/* Port Registers */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_CFG_PORT_CTL);//22
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_CFG_PORT_ST);//23
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_CFG_EX_VTYPE);//24
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_CFG_TCP_TIME);//25
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_CFG_LED_CTL);//26
	/* GPIO */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_GPIO_DR);//27
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_GPIO_DDR);//28
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_GPIO_CTL);//29
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_GPIO_INTEN);//30
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_GPIO_INTMASK);//31
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_GPIO_INTSTATUS);//32
	/* TX TPH */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_CFG_TPH_TDESC);//33
	/* RX TPH */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_CFG_TPH_RDESC);//34
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_CFG_TPH_RHDR);//35
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_CFG_TPH_RPL);//36

	/* TDMA */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_CTL);//37
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_POOL_TE);//38
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_PB_THRE);//39

	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_LLQ);//40
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_ETYPE_LB_L);//41

	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_ETYPE_AS_L);//42
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_MAC_AS_L);//43
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_VLAN_AS_L);//44
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_TCP_FLG_L);//45
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_TCP_FLG_H);//46
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_VLAN_INS(i));//47-54
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_ETAG_INS(i));//55-62
	}
	/* Transmit QOS */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_PBWARB_CTL);//63

	/* statistics */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_DRP_CNT);//64
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_SEC_DRP);//65
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_PKT_CNT);//66
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_BYTE_CNT_L);//67
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_BYTE_CNT_H);//68
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDM_OS2BMC_CNT);//69

	/* RDMA */
	/* receive control */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDM_ARB_CTL);//70
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDM_POOL_RE);//71
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDM_PF_QDE);//72
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDM_PF_HIDE);//73
	/* static */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDM_DRP_PKT);//74
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDM_PKT_CNT);//75
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDM_BYTE_CNT_L);//76
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDM_BYTE_CNT_H);//77
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDM_BMC2OS_CNT);//78

	/* RDB */
	/*flow control */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_RFCV);//79
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_RFCL);//80
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_RFCH);//81
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_RFCRT);//82
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_RFCC);//83
	/* receive packet buffer */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_PB_CTL);//84
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_PB_SZ);//85

	/* lli interrupt */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_LLI_THRE);//86
	/* ring assignment */
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_PL_CFG(i));//87-94
	}
	for (i = 0; i < 32; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_RSSTBL(i));//95-126
	}
	for (i = 0; i < 10; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_RSSRK(i));//127-136
	}
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_RA_CTL);//137
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_5T_SDP(i));//138-145
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_5T_CTL0(i));//146-153
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_5T_CTL1(i));//154-161
	}
	
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_SYN_CLS);//162
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_ETYPE_CLS(i));//163-170
	}
	/* statistics */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_MPCNT);//171
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_PKT_CNT);//172
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_REPLI_CNT);//173
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_DRP_CNT);//174
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_LXONTXC);//175
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_LXOFFTXC);//176
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_PFCMACDAL);//177
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_PFCMACDAH);//178
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RDB_TXSWERR);//179
	
	/* PSR */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_CTL);//180
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_MAX_SZ);//181
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_VLAN_CTL);//182
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_VM_CTL);//183
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_PKT_CNT);//184
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_MNG_PKT_CNT);//185
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_DBG_DOP_CNT);//186
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_MNG_DOP_CNT);//187
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_VM_FLP_L);//188
	
	/* vm l2 control */
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_VM_L2CTL(i));//189-196
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_ETYPE_SWC(i));//197-204
	}
	for (i = 0; i < 128; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_MC_TBL(i));//205-332
	}
	for (i = 0; i < 128; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_UC_TBL(i));///333-460
	}
	for (i = 0; i < 128; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_VLAN_TBL(i));//461-588
	}
	/* mac switcher */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_MAC_SWC_AD_L);//589
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_MAC_SWC_AD_H);//590
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_MAC_SWC_VM);//591
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_MAC_SWC_IDX);//592
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_VLAN_SWC);//593
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_VLAN_SWC_VM_L);//594
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_VLAN_SWC_IDX);//595

	/* mirror */
	for (i = 0; i < 4; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_MR_CTL(i));//596-599
	}
	for (i = 0; i < 4; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_MR_VLAN_L(i));//600-603
	}
	for (i = 0; i < 4; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_MR_VM_L(i));//604-607
	}
	/* 1588 */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_1588_CTL);//608
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_1588_STMPL);//609
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_1588_STMPH);//610
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_1588_ATTRL);//611
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_1588_ATTRH);//612
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_1588_MSGTYPE);//613
	/* wake up */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_WKUP_CTL);//614
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_WKUP_IPV);//615
	for (i = 0; i < 4; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_WKUP_IP4TBL(i));//616-619
	}
	for (i = 0; i < 4; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_WKUP_IP6TBL(i));//620-623
	}
	for (i = 0; i < 16; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_LAN_FLEX_DW_L(i));//624-639
	}
	for (i = 0; i < 16; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_LAN_FLEX_DW_H(i));//640-655
	}
	for (i = 0; i < 16; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_LAN_FLEX_MSK(i));//656-671
	}
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PSR_LAN_FLEX_CTL);//672

	/* TDB */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDB_RFCS);//673
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDB_PB_SZ);//674
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDB_PBRARB_CTL);//675
	/* statistic */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDB_OUT_PKT_CNT);//676
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDB_MNG_PKT_CNT);//677
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDB_LB_PKT_CNT);//678
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TDB_MNG_LARGE_DOP_CNT);//679

	/* TSEC */
	/* general tsec */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_CTL);//680
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_ST);//681
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_BUF_AF);//682
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_BUF_AE);//683
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_MIN_IFG);//684
	/* 1588 */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_1588_CTL);//685
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_1588_STMPL);//686
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_1588_STMPH);//687
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_1588_SYSTIML);//688
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_1588_SYSTIMH);//689
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_1588_INC);//690
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_1588_ADJL);//691
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_1588_ADJH);//692

	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_1588_INT_ST);//693
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_1588_INT_EN);//694
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_1588_AUX_CTL);//695
	for (i = 0; i < 4; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TSEC_1588_SDP(i));//696-699
	}
	
	/* RSEC */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RSEC_CTL);//700
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RSEC_ST);//701
	/* mac wrapper */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MAC_TX_CFG);//702
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MAC_RX_CFG);//703
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MAC_PKT_FLT);//704
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MAC_WDG_TIMEOUT);//705
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MAC_TX_FLOW_CTRL);//706
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MAC_RX_FLOW_CTRL);//707
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MAC_INT_ST);//708
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_MAC_INT_EN);//709
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_RX_FRAME_CNT_GOOD_BAD_LOW);//710
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_TX_FRAME_CNT_GOOD_BAD_LOW);//711

	/* BAR register */
	/* pf interrupt register */
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_MISC_IC);//712
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_MISC_ICS);//713
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_MISC_IEN);//714
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_GPIE);//715
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_IC);//716
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_ICS);//717
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_IMS);//718
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_IMC);//719
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_ISB_ADDR_L);//720
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_ISB_ADDR_H);//721
	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_ITRSEL);//722
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_ITR(i));//723-730
	}
	for (i = 0; i < 4; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_IVAR(i));//731-734
	}

	regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_MISC_IVAR);//735
	/* pf receive ring register */
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_RR_BAL(i));//736-743
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_RR_BAH(i));//744-751
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_RR_WP(i));//752-759
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_RR_RP(i));//760-767
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_RR_CFG(i));//768-775
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_TR_BAL(i));//776-783
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_TR_BAH(i));//784-791
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_TR_WP(i));//792-709
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_TR_RP(i));//800-807
	}
	for (i = 0; i < 8; i++) {
		regs_buff[id++] = NGBE_R32_Q(hw, NGBE_PX_TR_CFG(i));//808-815
	}
}

static int ngbe_get_eeprom_len(struct net_device *netdev)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	return adapter->hw.eeprom.word_size * 2;
}

static int ngbe_get_eeprom(struct net_device *netdev,
			    struct ethtool_eeprom *eeprom, u8 *bytes)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	u16 *eeprom_buff;
	int first_word, last_word, eeprom_len;
	int ret_val = 0;
	u16 i;

	if (eeprom->len == 0)
		return -EINVAL;

	eeprom->magic = hw->vendor_id | (hw->device_id << 16);

	first_word = eeprom->offset >> 1;
	last_word = (eeprom->offset + eeprom->len - 1) >> 1;
	eeprom_len = last_word - first_word + 1;

	eeprom_buff = kmalloc(sizeof(u16) * eeprom_len, GFP_KERNEL);
	if (!eeprom_buff)
		return -ENOMEM;

	ret_val = TCALL(hw, eeprom.ops.read_buffer, first_word, eeprom_len,
					   eeprom_buff);

	/* Device's eeprom is always little-endian, word addressable */
	for (i = 0; i < eeprom_len; i++)
		le16_to_cpus(&eeprom_buff[i]);

	memcpy(bytes, (u8 *)eeprom_buff + (eeprom->offset & 1), eeprom->len);
	kfree(eeprom_buff);

	return ret_val;
}

static int ngbe_set_eeprom(struct net_device *netdev,
			    struct ethtool_eeprom *eeprom, u8 *bytes)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	u16 *eeprom_buff;
	void *ptr;
	int max_len, first_word, last_word, ret_val = 0;
	u16 i;

	if (eeprom->len == 0)
		return -EINVAL;

	if (eeprom->magic != (hw->vendor_id | (hw->device_id << 16)))
		return -EINVAL;

	max_len = hw->eeprom.word_size * 2;

	first_word = eeprom->offset >> 1;
	last_word = (eeprom->offset + eeprom->len - 1) >> 1;
	eeprom_buff = kmalloc(max_len, GFP_KERNEL);
	if (!eeprom_buff)
		return -ENOMEM;

	ptr = eeprom_buff;

	if (eeprom->offset & 1) {
		/*
		 * need read/modify/write of first changed EEPROM word
		 * only the second byte of the word is being modified
		 */
		ret_val = TCALL(hw, eeprom.ops.read, first_word,
				&eeprom_buff[0]);
		if (ret_val)
			goto err;

		ptr++;
	}
	if (((eeprom->offset + eeprom->len) & 1) && (ret_val == 0)) {
		/*
		 * need read/modify/write of last changed EEPROM word
		 * only the first byte of the word is being modified
		 */
		ret_val = TCALL(hw, eeprom.ops.read, last_word,
				&eeprom_buff[last_word - first_word]);
		if (ret_val)
			goto err;
	}

	/* Device's eeprom is always little-endian, word addressable */
	for (i = 0; i < last_word - first_word + 1; i++)
		le16_to_cpus(&eeprom_buff[i]);

	memcpy(ptr, bytes, eeprom->len);

	for (i = 0; i < last_word - first_word + 1; i++)
		cpu_to_le16s(&eeprom_buff[i]);

	ret_val = TCALL(hw, eeprom.ops.write_buffer, first_word,
					    last_word - first_word + 1,
					    eeprom_buff);

	/* Update the checksum */
	if (ret_val == 0)
		TCALL(hw, eeprom.ops.update_checksum);

err:
	kfree(eeprom_buff);
	return ret_val;
}

static void ngbe_get_drvinfo(struct net_device *netdev,
							struct ethtool_drvinfo *drvinfo)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	strncpy(drvinfo->driver, ngbe_driver_name,
		sizeof(drvinfo->driver) - 1);
	strncpy(drvinfo->version, ngbe_driver_version,
		sizeof(drvinfo->version) - 1);
	strncpy(drvinfo->fw_version, adapter->eeprom_id,
		sizeof(drvinfo->fw_version));
	strncpy(drvinfo->bus_info, pci_name(adapter->pdev),
		sizeof(drvinfo->bus_info) - 1);
	if (adapter->num_tx_queues <= NGBE_NUM_RX_QUEUES) {
		drvinfo->n_stats = NGBE_STATS_LEN -
				   (NGBE_NUM_RX_QUEUES - adapter->num_tx_queues)*
					(sizeof(struct ngbe_queue_stats) / sizeof(u64))*2;
	}else{
		drvinfo->n_stats = NGBE_STATS_LEN;
	}
	drvinfo->testinfo_len = NGBE_TEST_LEN;
	drvinfo->regdump_len = ngbe_get_regs_len(netdev);
}

static void ngbe_get_ringparam(struct net_device *netdev,
				struct ethtool_ringparam *ring)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

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
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_ring *temp_ring;
	int i, err = 0;
	u32 new_rx_count, new_tx_count;

	if ((ring->rx_mini_pending) || (ring->rx_jumbo_pending))
		return -EINVAL;

	new_tx_count = clamp_t(u32, ring->tx_pending,
			       NGBE_MIN_TXD, NGBE_MAX_TXD);
	new_tx_count = ALIGN(new_tx_count, NGBE_REQ_TX_DESCRIPTOR_MULTIPLE);

	new_rx_count = clamp_t(u32, ring->rx_pending,
			       NGBE_MIN_RXD, NGBE_MAX_RXD);
	new_rx_count = ALIGN(new_rx_count, NGBE_REQ_RX_DESCRIPTOR_MULTIPLE);

	if ((new_tx_count == adapter->tx_ring_count) &&
	    (new_rx_count == adapter->rx_ring_count)) {
		/* nothing to do */
		return 0;
	}

	while (test_and_set_bit(__NGBE_RESETTING, &adapter->state))
		usleep_range(1000, 2000);

	if (!netif_running(adapter->netdev)) {
		for (i = 0; i < adapter->num_tx_queues; i++)
			adapter->tx_ring[i]->count = new_tx_count;
		for (i = 0; i < adapter->num_rx_queues; i++)
			adapter->rx_ring[i]->count = new_rx_count;
		adapter->tx_ring_count = new_tx_count;
		adapter->rx_ring_count = new_rx_count;
		goto clear_reset;
	}

	/* allocate temporary buffer to store rings in */
	i = max_t(int, adapter->num_tx_queues, adapter->num_rx_queues);
	temp_ring = vmalloc(i * sizeof(struct ngbe_ring));

	if (!temp_ring) {
		err = -ENOMEM;
		goto clear_reset;
	}

	ngbe_down(adapter);

	/*
	 * Setup new Tx resources and free the old Tx resources in that order.
	 * We can then assign the new resources to the rings via a memcpy.
	 * The advantage to this approach is that we are guaranteed to still
	 * have resources even in the case of an allocation failure.
	 */
	if (new_tx_count != adapter->tx_ring_count) {
		for (i = 0; i < adapter->num_tx_queues; i++) {
			memcpy(&temp_ring[i], adapter->tx_ring[i],
			       sizeof(struct ngbe_ring));

			temp_ring[i].count = new_tx_count;
			err = ngbe_setup_tx_resources(&temp_ring[i]);
			if (err) {
				while (i) {
					i--;
					ngbe_free_tx_resources(&temp_ring[i]);
				}
				goto err_setup;
			}
		}

		for (i = 0; i < adapter->num_tx_queues; i++) {
			ngbe_free_tx_resources(adapter->tx_ring[i]);

			memcpy(adapter->tx_ring[i], &temp_ring[i],
			       sizeof(struct ngbe_ring));
		}

		adapter->tx_ring_count = new_tx_count;
	}

	/* Repeat the process for the Rx rings if needed */
	if (new_rx_count != adapter->rx_ring_count) {
		for (i = 0; i < adapter->num_rx_queues; i++) {
			memcpy(&temp_ring[i], adapter->rx_ring[i],
			       sizeof(struct ngbe_ring));

			temp_ring[i].count = new_rx_count;
			err = ngbe_setup_rx_resources(&temp_ring[i]);
			if (err) {
				while (i) {
					i--;
					ngbe_free_rx_resources(&temp_ring[i]);
				}
				goto err_setup;
			}
		}


		for (i = 0; i < adapter->num_rx_queues; i++) {
			ngbe_free_rx_resources(adapter->rx_ring[i]);

			memcpy(adapter->rx_ring[i], &temp_ring[i],
			       sizeof(struct ngbe_ring));
		}

		adapter->rx_ring_count = new_rx_count;
	}

err_setup:
	ngbe_up(adapter);
	vfree(temp_ring);
clear_reset:
	clear_bit(__NGBE_RESETTING, &adapter->state);
	return err;
}

#ifndef HAVE_ETHTOOL_GET_SSET_COUNT
static int ngbe_get_stats_count(struct net_device *netdev)
{
	if (adapter->num_tx_queues <= NGBE_NUM_RX_QUEUES) {
		return  NGBE_STATS_LEN - (NGBE_NUM_RX_QUEUES - adapter->num_tx_queues)*
					(sizeof(struct ngbe_queue_stats) / sizeof(u64))*2;
	}else{
		return NGBE_STATS_LEN;
	}
}

#else /* HAVE_ETHTOOL_GET_SSET_COUNT */
static int ngbe_get_sset_count(struct net_device *netdev, int sset)
{
#ifdef HAVE_TX_MQ
#ifndef HAVE_NETDEV_SELECT_QUEUE
	struct ngbe_adapter *adapter = netdev_priv(netdev);
#endif
#endif
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	switch (sset) {
	case ETH_SS_TEST:
		return NGBE_TEST_LEN;
	case ETH_SS_STATS:
		if (adapter->num_tx_queues <= NGBE_NUM_RX_QUEUES) {
			return  NGBE_STATS_LEN - (NGBE_NUM_RX_QUEUES - adapter->num_tx_queues)*
					(sizeof(struct ngbe_queue_stats) / sizeof(u64))*2;
		}else{
			return NGBE_STATS_LEN;
		}
	default:
		return -EOPNOTSUPP;
	}
}

#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */
static void ngbe_get_ethtool_stats(struct net_device *netdev,
				    struct ethtool_stats __always_unused *stats,
				    u64 *data)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
#ifdef HAVE_NETDEV_STATS_IN_NETDEV
	struct net_device_stats *net_stats = &netdev->stats;
#else
	struct net_device_stats *net_stats = &adapter->net_stats;
#endif
	u64 *queue_stat;
	int stat_count, k;
#ifdef HAVE_NDO_GET_STATS64
	unsigned int start;
#endif
	struct ngbe_ring *ring;
	int i, j;
	char *p;

	ngbe_update_stats(adapter);

	for (i = 0; i < NGBE_NETDEV_STATS_LEN; i++) {
		p = (char *)net_stats + ngbe_gstrings_net_stats[i].stat_offset;
		data[i] = (ngbe_gstrings_net_stats[i].sizeof_stat ==
			sizeof(u64)) ? *(u64 *)p : *(u32 *)p;
	}
	for (j = 0; j < NGBE_GLOBAL_STATS_LEN; j++, i++) {
		p = (char *)adapter + ngbe_gstrings_stats[j].stat_offset;
		data[i] = (ngbe_gstrings_stats[j].sizeof_stat ==
			   sizeof(u64)) ? *(u64 *)p : *(u32 *)p;
	}

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

	for (j = 0; j < NGBE_MAX_PACKET_BUFFERS; j++) {
		data[i++] = adapter->stats.pxontxc[j];
		data[i++] = adapter->stats.pxofftxc[j];
	}
	for (j = 0; j < NGBE_MAX_PACKET_BUFFERS; j++) {
		data[i++] = adapter->stats.pxonrxc[j];
		data[i++] = adapter->stats.pxoffrxc[j];
	}

	stat_count = sizeof(struct vf_stats) / sizeof(u64);
	for (j = 0; j < adapter->num_vfs; j++) {
		queue_stat = (u64 *)&adapter->vfinfo[j].vfstats;
		for (k = 0; k < stat_count; k++)
			data[i + k] = queue_stat[k];
		queue_stat = (u64 *)&adapter->vfinfo[j].saved_rst_vfstats;
		for (k = 0; k < stat_count; k++)
			data[i + k] += queue_stat[k];
		i += k;
	}
}

static void ngbe_get_strings(struct net_device *netdev, u32 stringset,
			      u8 *data)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	char *p = (char *)data;
	int i;

	switch (stringset) {
	case ETH_SS_TEST:
		memcpy(data, *ngbe_gstrings_test,
		       NGBE_TEST_LEN * ETH_GSTRING_LEN);
		break;
	case ETH_SS_STATS:
		for (i = 0; i < NGBE_NETDEV_STATS_LEN; i++) {
			memcpy(p, ngbe_gstrings_net_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		for (i = 0; i < NGBE_GLOBAL_STATS_LEN; i++) {
			memcpy(p, ngbe_gstrings_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
        for (i = 0; i < adapter->num_tx_queues; i++) {  /*temp setting2*/ 
			sprintf(p, "tx_queue_%u_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "tx_queue_%u_bytes", i);
			p += ETH_GSTRING_LEN;
#ifdef BP_EXTENDED_STATS
			sprintf(p, "tx_queue_%u_bp_napi_yield", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "tx_queue_%u_bp_misses", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "tx_queue_%u_bp_cleaned", i);
			p += ETH_GSTRING_LEN;
#endif /* BP_EXTENDED_STATS */
		}
        for (i = 0; i < adapter->num_rx_queues; i++) {  /*temp setting2*/ 
			sprintf(p, "rx_queue_%u_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "rx_queue_%u_bytes", i);
			p += ETH_GSTRING_LEN;
#ifdef BP_EXTENDED_STATS
			sprintf(p, "rx_queue_%u_bp_poll_yield", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "rx_queue_%u_bp_misses", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "rx_queue_%u_bp_cleaned", i);
			p += ETH_GSTRING_LEN;
#endif /* BP_EXTENDED_STATS */
		}
		for (i = 0; i < NGBE_MAX_PACKET_BUFFERS; i++) {
			sprintf(p, "tx_pb_%u_pxon", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "tx_pb_%u_pxoff", i);
			p += ETH_GSTRING_LEN;
		}
		for (i = 0; i < NGBE_MAX_PACKET_BUFFERS; i++) {
			sprintf(p, "rx_pb_%u_pxon", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "rx_pb_%u_pxoff", i);
			p += ETH_GSTRING_LEN;
		}
		for (i = 0; i < adapter->num_vfs; i++) {
			sprintf(p, "VF %d Rx Packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "VF %d Rx Bytes", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "VF %d Tx Packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "VF %d Tx Bytes", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "VF %d MC Packets", i);
			p += ETH_GSTRING_LEN;
		}
		/* BUG_ON(p - data != NGBE_STATS_LEN * ETH_GSTRING_LEN); */
		break;
	}
}

static int ngbe_link_test(struct ngbe_adapter *adapter, u64 *data)
{
	struct ngbe_hw *hw = &adapter->hw;
	bool link_up;
	u32 link_speed = 0;

	if (NGBE_REMOVED(hw->hw_addr)) {
		*data = 1;
		return 1;
	}
	*data = 0;
    TCALL(hw, mac.ops.check_link, &link_speed, &link_up, true);
	if (link_up)
		return *data;
	else
		*data = 1;
	return *data;
}

/* ethtool register test data */
struct ngbe_reg_test {
	u32 reg;
	u8  array_len;
	u8  test_type;
	u32 mask;
	u32 write;
};

/* In the hardware, registers are laid out either singly, in arrays
 * spaced 0x40 bytes apart, or in contiguous tables.  We assume
 * most tests take place on arrays or single registers (handled
 * as a single-element array) and special-case the tables.
 * Table tests are always pattern tests.
 *
 * We also make provision for some required setup steps by specifying
 * registers to be written without any read-back testing.
 */

#define PATTERN_TEST    1
#define SET_READ_TEST   2
#define WRITE_NO_TEST   3
#define TABLE32_TEST    4
#define TABLE64_TEST_LO 5
#define TABLE64_TEST_HI 6


/* default sapphire register test */
static struct ngbe_reg_test reg_test_sapphire[] = {
	{ NGBE_RDB_RFCL, 1, PATTERN_TEST, 0x8007FFE0, 0x8007FFE0 },
	{ NGBE_RDB_RFCH, 1, PATTERN_TEST, 0x8007FFE0, 0x8007FFE0 },
	{ NGBE_PSR_VLAN_CTL, 1, PATTERN_TEST, 0x00000000, 0x00000000 },
	{ NGBE_PX_RR_BAL(0), 4, PATTERN_TEST, 0xFFFFFF80, 0xFFFFFF80 },
	{ NGBE_PX_RR_BAH(0), 4, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ NGBE_PX_RR_CFG(0), 4, WRITE_NO_TEST, 0, NGBE_PX_RR_CFG_RR_EN },
	{ NGBE_RDB_RFCH, 1, PATTERN_TEST, 0x8007FFE0, 0x8007FFE0 },
	{ NGBE_RDB_RFCV, 1, PATTERN_TEST, 0xFFFF0000, 0xFFFF0000 },
	{ NGBE_PX_TR_BAL(0), 4, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ NGBE_PX_TR_BAH(0), 4, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ NGBE_RDB_PB_CTL, 1, SET_READ_TEST, 0x00000001, 0x00000001 },
	{ NGBE_PSR_MC_TBL(0), 128, TABLE32_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ .reg = 0 }
};


static bool reg_pattern_test(struct ngbe_adapter *adapter, u64 *data, int reg,
			     u32 mask, u32 write)
{
	u32 pat, val, before;
	static const u32 test_pattern[] = {
		0x5A5A5A5A, 0xA5A5A5A5, 0x00000000, 0xFFFFFFFF
	};

	if (NGBE_REMOVED(adapter->hw.hw_addr)) {
		*data = 1;
		return true;
	}
	for (pat = 0; pat < ARRAY_SIZE(test_pattern); pat++) {
		before = rd32(&adapter->hw, reg);
		wr32(&adapter->hw, reg, test_pattern[pat] & write);
		val = rd32(&adapter->hw, reg);
		if (val != (test_pattern[pat] & write & mask)) {
			e_err(drv,
			      "pattern test reg %04X failed: got 0x%08X "
			      "expected 0x%08X\n",
			      reg, val, test_pattern[pat] & write & mask);
			*data = reg;
			wr32(&adapter->hw, reg, before);
			return true;
		}
		wr32(&adapter->hw, reg, before);
	}
	return false;
}

static bool reg_set_and_check(struct ngbe_adapter *adapter, u64 *data, int reg,
			      u32 mask, u32 write)
{
	u32 val, before;

	if (NGBE_REMOVED(adapter->hw.hw_addr)) {
		*data = 1;
		return true;
	}
	before = rd32(&adapter->hw, reg);
	wr32(&adapter->hw, reg, write & mask);
	val = rd32(&adapter->hw, reg);
	if ((write & mask) != (val & mask)) {
		e_err(drv,
		      "set/check reg %04X test failed: got 0x%08X expected"
		      "0x%08X\n",
		      reg, (val & mask), (write & mask));
		*data = reg;
		wr32(&adapter->hw, reg, before);
		return true;
	}
	wr32(&adapter->hw, reg, before);
	return false;
}




static bool ngbe_reg_test(struct ngbe_adapter *adapter, u64 *data)
{
	struct ngbe_reg_test *test;
	struct ngbe_hw *hw = &adapter->hw;
	u32 i;

	if (NGBE_REMOVED(hw->hw_addr)) {
		e_err(drv, "Adapter removed - register test blocked\n");
		*data = 1;
		return true;
	}

	test = reg_test_sapphire;

	/*
	 * Perform the remainder of the register test, looping through
	 * the test table until we either fail or reach the null entry.
	 */
	while (test->reg) {
		for (i = 0; i < test->array_len; i++) {
			bool b = false;

			switch (test->test_type) {
			case PATTERN_TEST:
				b = reg_pattern_test(adapter, data,
						      test->reg + (i * 0x40),
						      test->mask,
						      test->write);
				break;
			case SET_READ_TEST:
				b = reg_set_and_check(adapter, data,
						       test->reg + (i * 0x40),
						       test->mask,
						       test->write);
				break;
			case WRITE_NO_TEST:
				wr32(hw, test->reg + (i * 0x40),
						test->write);
				break;
			case TABLE32_TEST:
				b = reg_pattern_test(adapter, data,
						      test->reg + (i * 4),
						      test->mask,
						      test->write);
				break;
			case TABLE64_TEST_LO:
				b = reg_pattern_test(adapter, data,
						      test->reg + (i * 8),
						      test->mask,
						      test->write);
				break;
			case TABLE64_TEST_HI:
				b = reg_pattern_test(adapter, data,
						      (test->reg + 4) + (i * 8),
						      test->mask,
						      test->write);
				break;
			}
			if (b)
				return true;
		}
		test++;
	}

	*data = 0;
	return false;
}

#ifndef SIMULATION_DEBUG
static bool ngbe_eeprom_test(struct ngbe_adapter *adapter, u64 *data)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 devcap;

	if (TCALL(hw, eeprom.ops.eeprom_chksum_cap_st, NGBE_CALSUM_COMMAND, &devcap)) {
		*data = 1;
		return true;
	} else {
		*data = 0;
		return false;
	}
}
#endif


static irqreturn_t ngbe_test_intr(int __always_unused irq, void *data)
{
	struct net_device *netdev = (struct net_device *) data;
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	u64 icr;

	/* get misc interrupt, as cannot get ring interrupt status */
	icr = ngbe_misc_isb(adapter, NGBE_ISB_VEC1);
	icr <<= 32;
	icr |= ngbe_misc_isb(adapter, NGBE_ISB_VEC0);

	adapter->test_icr = icr;

	return IRQ_HANDLED;
}



static int ngbe_intr_test(struct ngbe_adapter *adapter, u64 *data)
{
	struct net_device *netdev = adapter->netdev;
	u64 mask;
	u32 i = 0, shared_int = true;
	u32 irq = adapter->pdev->irq;

	if (NGBE_REMOVED(adapter->hw.hw_addr)) {
		*data = 1;
		return -1;
	}
	*data = 0;

	/* Hook up test interrupt handler just for this test */
	if (adapter->msix_entries) {
		/* NOTE: we don't test MSI-X interrupts here, yet */
		return 0;
	} else if (adapter->flags & NGBE_FLAG_MSI_ENABLED) {
		shared_int = false;
		if (request_irq(irq, &ngbe_test_intr, 0, netdev->name,
				netdev)) {
			*data = 1;
			return -1;
		}
	} else if (!request_irq(irq, &ngbe_test_intr, IRQF_PROBE_SHARED,
				netdev->name, netdev)) {
		shared_int = false;
	} else if (request_irq(irq, &ngbe_test_intr, IRQF_SHARED,
			       netdev->name, netdev)) {
		*data = 1;
		return -1;
	}
	e_info(hw, "testing %s interrupt\n",
	       (shared_int ? "shared" : "unshared"));

	/* Disable all the interrupts */
	ngbe_irq_disable(adapter);
	NGBE_WRITE_FLUSH(&adapter->hw);
	usleep_range(10000, 20000);

	/* Test each interrupt */
	for (; i < 1; i++) {
		/* Interrupt to test */
		mask = 1ULL << i;

		if (!shared_int) {
			/*
			 * Disable the interrupts to be reported in
			 * the cause register and then force the same
			 * interrupt and see if one gets posted.  If
			 * an interrupt was posted to the bus, the
			 * test failed.
			 */
			adapter->test_icr = 0;
			ngbe_intr_disable(&adapter->hw, ~mask);
			ngbe_intr_trigger(&adapter->hw, mask);
			NGBE_WRITE_FLUSH(&adapter->hw);
			usleep_range(10000, 20000);

			if (adapter->test_icr & mask) {
				*data = 3;
				break;
			}
		}

		/*
		 * Enable the interrupt to be reported in the cause
		 * register and then force the same interrupt and see
		 * if one gets posted.  If an interrupt was not posted
		 * to the bus, the test failed.
		 */
		adapter->test_icr = 0;
		ngbe_intr_disable(&adapter->hw, NGBE_INTR_ALL);
		ngbe_intr_trigger(&adapter->hw, mask);
		NGBE_WRITE_FLUSH(&adapter->hw);
		usleep_range(10000, 20000);

		if (!(adapter->test_icr & mask)) {
			*data = 0;
			break;
		}
	}

	/* Disable all the interrupts */
	ngbe_intr_disable(&adapter->hw, NGBE_INTR_ALL);
	NGBE_WRITE_FLUSH(&adapter->hw);
	usleep_range(10000, 20000);

	/* Unhook test interrupt handler */
	free_irq(irq, netdev);

	return *data;
}

static void ngbe_free_desc_rings(struct ngbe_adapter *adapter)
{
	struct ngbe_ring *tx_ring = &adapter->test_tx_ring;
	struct ngbe_ring *rx_ring = &adapter->test_rx_ring;
	struct ngbe_hw *hw = &adapter->hw;

	/* shut down the DMA engines now so they can be reinitialized later */

	/* first Rx */
	TCALL(hw, mac.ops.disable_rx);
	ngbe_disable_rx_queue(adapter, rx_ring);

	/* now Tx */
	wr32(hw, NGBE_PX_TR_CFG(tx_ring->reg_idx), 0);

	wr32m(hw, NGBE_TDM_CTL, NGBE_TDM_CTL_TE, 0);

	ngbe_reset(adapter);

	ngbe_free_tx_resources(&adapter->test_tx_ring);
	ngbe_free_rx_resources(&adapter->test_rx_ring);
}

static int ngbe_setup_desc_rings(struct ngbe_adapter *adapter)
{
	struct ngbe_ring *tx_ring = &adapter->test_tx_ring;
	struct ngbe_ring *rx_ring = &adapter->test_rx_ring;
	struct ngbe_hw *hw = &adapter->hw;
	int ret_val;
	int err;

	TCALL(hw, mac.ops.setup_rxpba, 0, 0, PBA_STRATEGY_EQUAL);

	/* Setup Tx descriptor ring and Tx buffers */
	tx_ring->count = NGBE_DEFAULT_TXD;
	tx_ring->queue_index = 0;
	tx_ring->dev = pci_dev_to_dev(adapter->pdev);
	tx_ring->netdev = adapter->netdev;
	tx_ring->reg_idx = adapter->tx_ring[0]->reg_idx;

	err = ngbe_setup_tx_resources(tx_ring);
	if (err)
		return 1;

	wr32m(&adapter->hw, NGBE_TDM_CTL,
		NGBE_TDM_CTL_TE, NGBE_TDM_CTL_TE);
	wr32m(hw, NGBE_TSEC_CTL, 0x2, 0);
	wr32m(hw, NGBE_RSEC_CTL, 0x2, 0);
	ngbe_configure_tx_ring(adapter, tx_ring);


	/* enable mac transmitter */
	wr32m(hw, NGBE_MAC_TX_CFG,
		NGBE_MAC_TX_CFG_TE | NGBE_MAC_TX_CFG_SPEED_MASK,
		NGBE_MAC_TX_CFG_TE | NGBE_MAC_TX_CFG_SPEED_1G);

	/* Setup Rx Descriptor ring and Rx buffers */
	rx_ring->count = NGBE_DEFAULT_RXD;
	rx_ring->queue_index = 0;
	rx_ring->dev = pci_dev_to_dev(adapter->pdev);
	rx_ring->netdev = adapter->netdev;
	rx_ring->reg_idx = adapter->rx_ring[0]->reg_idx;
#ifdef CONFIG_NGBE_DISABLE_PACKET_SPLIT
	rx_ring->rx_buf_len = NGBE_RXBUFFER_2K;
#endif

	err = ngbe_setup_rx_resources(rx_ring);
	if (err) {
		ret_val = 4;
		goto err_nomem;
	}

	TCALL(hw, mac.ops.disable_rx);

	ngbe_configure_rx_ring(adapter, rx_ring);

	TCALL(hw, mac.ops.enable_rx);

	return 0;

err_nomem:
	ngbe_free_desc_rings(adapter);
	return ret_val;
}

static int ngbe_setup_loopback_test(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 reg_data;

	/* Setup MAC loopback */
	wr32m(hw, NGBE_MAC_RX_CFG,
		NGBE_MAC_RX_CFG_LM, NGBE_MAC_RX_CFG_LM);

	reg_data = rd32(hw, NGBE_PSR_CTL);
	reg_data |= NGBE_PSR_CTL_BAM | NGBE_PSR_CTL_UPE |
		NGBE_PSR_CTL_MPE | NGBE_PSR_CTL_TPE;
	wr32(hw, NGBE_PSR_CTL, reg_data);

	wr32(hw, 0x17000,
		(rd32(hw, 0x17000 )|
		0x00000040U) & ~0x1U);

	wr32(hw, 0x17204, 0x4);
	wr32(hw, NGBE_PSR_VLAN_CTL,
		rd32(hw, NGBE_PSR_VLAN_CTL) &
		~NGBE_PSR_VLAN_CTL_VFE);

	NGBE_WRITE_FLUSH(hw);
	usleep_range(10000, 20000);

	return 0;
}

static void ngbe_loopback_cleanup(struct ngbe_adapter *adapter)
{
	wr32m(&adapter->hw, NGBE_MAC_RX_CFG,
		NGBE_MAC_RX_CFG_LM, ~NGBE_MAC_RX_CFG_LM);
}


static void ngbe_create_lbtest_frame(struct sk_buff *skb,
				      unsigned int frame_size)
{
	memset(skb->data, 0xFF, frame_size);
	frame_size >>= 1;
	memset(&skb->data[frame_size], 0xAA, frame_size / 2 - 1);
	memset(&skb->data[frame_size + 10], 0xBE, 1);
	memset(&skb->data[frame_size + 12], 0xAF, 1);
}

static bool ngbe_check_lbtest_frame(struct ngbe_rx_buffer *rx_buffer,
				     unsigned int frame_size)
{
	unsigned char *data;
	bool match = true;

	frame_size >>= 1;

#ifdef CONFIG_NGBE_DISABLE_PACKET_SPLIT
	data = rx_buffer->skb->data;
#else
	data = kmap(rx_buffer->page) + rx_buffer->page_offset;
#endif

	if (data[3] != 0xFF ||
	    data[frame_size + 10] != 0xBE ||
	    data[frame_size + 12] != 0xAF)
		match = false;

#ifndef CONFIG_NGBE_DISABLE_PACKET_SPLIT
	kunmap(rx_buffer->page);

#endif
	return match;
}

static u16 ngbe_clean_test_rings(struct ngbe_ring *rx_ring,
				  struct ngbe_ring *tx_ring,
				  unsigned int size)
{
	union ngbe_rx_desc *rx_desc;
	struct ngbe_rx_buffer *rx_buffer;
	struct ngbe_tx_buffer *tx_buffer;
#ifdef CONFIG_NGBE_DISABLE_PACKET_SPLIT
	const int bufsz = rx_ring->rx_buf_len;
#else
	const int bufsz = ngbe_rx_bufsz(rx_ring);
#endif
	u16 rx_ntc, tx_ntc, count = 0;

	/* initialize next to clean and descriptor values */
	rx_ntc = rx_ring->next_to_clean;
	tx_ntc = tx_ring->next_to_clean;
	rx_desc = NGBE_RX_DESC(rx_ring, rx_ntc);

	while (ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_DD)) {
		/* unmap buffer on Tx side */
		tx_buffer = &tx_ring->tx_buffer_info[tx_ntc];
		ngbe_unmap_and_free_tx_resource(tx_ring, tx_buffer);

		/* check Rx buffer */
		rx_buffer = &rx_ring->rx_buffer_info[rx_ntc];

#ifndef CONFIG_NGBE_DISABLE_PACKET_SPLIT
		/* sync Rx buffer for CPU read */
		dma_sync_single_for_cpu(rx_ring->dev,
					rx_buffer->page_dma,
					bufsz,
					DMA_FROM_DEVICE);
#else
		/* sync Rx buffer for CPU read */
		dma_sync_single_for_cpu(rx_ring->dev,
					rx_buffer->dma,
					bufsz,
					DMA_FROM_DEVICE);	
#endif
		/* verify contents of skb */
		if (ngbe_check_lbtest_frame(rx_buffer, size))
			count++;

#ifndef CONFIG_NGBE_DISABLE_PACKET_SPLIT
		/* sync Rx buffer for device write */
		dma_sync_single_for_device(rx_ring->dev,
					   rx_buffer->page_dma,
					   bufsz,
					   DMA_FROM_DEVICE);
#else 
		dma_sync_single_for_device(rx_ring->dev,
					   rx_buffer->dma,
					   bufsz,
					   DMA_FROM_DEVICE);
#endif
		/* increment Rx/Tx next to clean counters */
		rx_ntc++;
		if (rx_ntc == rx_ring->count)
			rx_ntc = 0;
		tx_ntc++;
		if (tx_ntc == tx_ring->count)
			tx_ntc = 0;

		/* fetch next descriptor */
		rx_desc = NGBE_RX_DESC(rx_ring, rx_ntc);
	}

	/* re-map buffers to ring, store next to clean values */
	ngbe_alloc_rx_buffers(rx_ring, count);
	rx_ring->next_to_clean = rx_ntc;
	tx_ring->next_to_clean = tx_ntc;

	return count;
}

static int ngbe_run_loopback_test(struct ngbe_adapter *adapter)
{
	struct ngbe_ring *tx_ring = &adapter->test_tx_ring;
	struct ngbe_ring *rx_ring = &adapter->test_rx_ring;
	int i, j, lc, good_cnt, ret_val = 0;
	unsigned int size = 1024;
	netdev_tx_t tx_ret_val;
	struct sk_buff *skb;
	u32 flags_orig = adapter->flags;

	//struct ngbe_hw *hw = &adapter->hw;


	/* DCB can modify the frames on Tx */
	adapter->flags &= ~NGBE_FLAG_DCB_ENABLED;

	/* allocate test skb */
	skb = alloc_skb(size, GFP_KERNEL);
	if (!skb)
		return 11;

	/* place data into test skb */
	ngbe_create_lbtest_frame(skb, size);
	skb_put(skb, size);

	/*
	 * Calculate the loop count based on the largest descriptor ring
	 * The idea is to wrap the largest ring a number of times using 64
	 * send/receive pairs during each loop
	 */

	if (rx_ring->count <= tx_ring->count)
		lc = ((tx_ring->count / 64) * 2) + 1;
	else
		lc = ((rx_ring->count / 64) * 2) + 1;

	for (j = 0; j <= lc; j++) {
		/* reset count of good packets */
		good_cnt = 0;

		/* place 64 packets on the transmit queue*/
		for (i = 0; i < 64; i++) {
			skb_get(skb);
			tx_ret_val = ngbe_xmit_frame_ring(skb,
							   adapter,
							   tx_ring);
			if (tx_ret_val == NETDEV_TX_OK)
				good_cnt++;
		}

		msleep(10);
		//e_dev_info("====hw_cnt = %d====\n", rd32(hw, 0x18308));


		if (good_cnt != 64) {
			ret_val = 12;
		//	e_dev_err("====tran_cnt = %d====\n", good_cnt);
			break;
		}

		/* allow 200 milliseconds for packets to go from Tx to Rx */
		msleep(200);

		good_cnt = ngbe_clean_test_rings(rx_ring, tx_ring, size);
		if (good_cnt != 64) {
			ret_val = 13;
			e_dev_err("ngbe_run_loopback_test: recv_cnt = %d\n", good_cnt);
			break;
		}
	}

	/* free the original skb */
	kfree_skb(skb);
	adapter->flags = flags_orig;

	return ret_val;
}

static int ngbe_loopback_test(struct ngbe_adapter *adapter, u64 *data)
{
	*data = ngbe_setup_desc_rings(adapter);
	if (*data)
		goto out;
	*data = ngbe_setup_loopback_test(adapter);
	if (*data)
		goto err_loopback;
	*data = ngbe_run_loopback_test(adapter);
	if (*data)
		e_info(hw, "mac loopback testing failed\n");
	ngbe_loopback_cleanup(adapter);

err_loopback:
	ngbe_free_desc_rings(adapter);
out:
	return *data;
}

#ifndef HAVE_ETHTOOL_GET_SSET_COUNT
static int ngbe_diag_test_count(struct net_device __always_unused *netdev)
{
	return NGBE_TEST_LEN;
}

#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */
static void ngbe_diag_test(struct net_device *netdev,
			    struct ethtool_test *eth_test, u64 *data)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	bool if_running = netif_running(netdev);
	struct ngbe_hw *hw = &adapter->hw;

	e_dev_info("ngbe_diag_test: start test\n");


	if (NGBE_REMOVED(hw->hw_addr)) {
		e_err(hw, "Adapter removed - test blocked\n");
		data[0] = 1;
		data[1] = 1;
		data[2] = 1;
		data[3] = 1;
		data[4] = 1;
		eth_test->flags |= ETH_TEST_FL_FAILED;
		return;
	}
	set_bit(__NGBE_TESTING, &adapter->state);
	if (eth_test->flags == ETH_TEST_FL_OFFLINE) {
		if (adapter->flags & NGBE_FLAG_SRIOV_ENABLED) {
			int i;
			for (i = 0; i < adapter->num_vfs; i++) {
				if (adapter->vfinfo[i].clear_to_send) {
					e_warn(drv, "Please take active VFS "
					       "offline and restart the "
					       "adapter before running NIC "
					       "diagnostics\n");
					data[0] = 1;
					data[1] = 1;
					data[2] = 1;
					data[3] = 1;
					data[4] = 1;
					eth_test->flags |= ETH_TEST_FL_FAILED;
					clear_bit(__NGBE_TESTING,
						  &adapter->state);
					goto skip_ol_tests;
				}
			}
		}

		/* Offline tests */
		e_info(hw, "offline testing starting\n");


		/* Link test performed before hardware reset so autoneg doesn't
		 * interfere with test result */
		if (ngbe_link_test(adapter, &data[4]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		if (if_running)
			/* indicate we're in test mode */
			ngbe_close(netdev);
		else{
			msleep(20);
			ngbe_reset(adapter);
			}


		e_info(hw, "register testing starting\n");


		if (ngbe_reg_test(adapter, &data[0]))
			eth_test->flags |= ETH_TEST_FL_FAILED;
#ifndef SIMULATION_DEBUG
		msleep(20);
		ngbe_reset(adapter);
		e_info(hw, "eeprom testing starting\n");
		if (ngbe_eeprom_test(adapter, &data[1]))
			eth_test->flags |= ETH_TEST_FL_FAILED;
#else
		data[1] = 0;
#endif
		msleep(20);
		ngbe_reset(adapter);
		e_info(hw, "interrupt testing starting\n");
		if (ngbe_intr_test(adapter, &data[2]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

	if (! (((hw->subsystem_device_id & OEM_MASK) == OCP_CARD) || 
		((hw->subsystem_device_id & NCSI_SUP_MASK) == NCSI_SUP))){
		/* If SRIOV or VMDq is enabled then skip MAC
		 * loopback diagnostic. */
		if (adapter->flags & (NGBE_FLAG_SRIOV_ENABLED |
				      NGBE_FLAG_VMDQ_ENABLED)) {
			e_info(hw, "skip MAC loopback diagnostic in VT mode\n");
			data[3] = 0;
			goto skip_loopback;
		}

		e_info(hw, "loopback testing starting\n");
		ngbe_loopback_test(adapter, &data[3]);
	}
	

	data[3] = 0;

skip_loopback:
		msleep(20);
		ngbe_reset(adapter);

		/* clear testing bit and return adapter to previous state */
		clear_bit(__NGBE_TESTING, &adapter->state);
		if (if_running)
			ngbe_open(netdev);
	} else {
		e_info(hw, "online testing starting\n");

		/* Online tests */
		if (ngbe_link_test(adapter, &data[4]))
			eth_test->flags |= ETH_TEST_FL_FAILED;

		/* Offline tests aren't run; pass by default */
		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;

		clear_bit(__NGBE_TESTING, &adapter->state);
	}

skip_ol_tests:
	msleep_interruptible(4 * 1000);
}

static int ngbe_wol_exclusion(struct ngbe_adapter *adapter,
			       struct ethtool_wolinfo *wol)
{
	int retval = 0;

	/* WOL not supported for all devices */
	if (!ngbe_wol_supported(adapter)) {
		retval = 1;
		wol->supported = 0;
	}

	return retval;
}

static void ngbe_get_wol(struct net_device *netdev,
			  struct ethtool_wolinfo *wol)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	
	struct ngbe_hw *hw = &adapter->hw;

	wol->supported = WAKE_UCAST | WAKE_MCAST |
			 WAKE_BCAST | WAKE_MAGIC;
	wol->wolopts = 0;

	if (ngbe_wol_exclusion(adapter, wol) ||
	    !device_can_wakeup(pci_dev_to_dev(adapter->pdev)))
		return;

	if (adapter->wol & NGBE_PSR_WKUP_CTL_EX)
		wol->wolopts |= WAKE_UCAST;
	if (adapter->wol & NGBE_PSR_WKUP_CTL_MC)
		wol->wolopts |= WAKE_MCAST;
	if (adapter->wol & NGBE_PSR_WKUP_CTL_BC)
		wol->wolopts |= WAKE_BCAST;
	if (adapter->wol & NGBE_PSR_WKUP_CTL_MAG)
		wol->wolopts |= WAKE_MAGIC;

	if ( !((hw->subsystem_device_id & WOL_SUP_MASK) == WOL_SUP))
		wol->wolopts = 0;
}

static int ngbe_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wol)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	u32 slot = hw->bus.lan_id;
	u16 value;

	if (wol->wolopts & (WAKE_PHY | WAKE_ARP | WAKE_MAGICSECURE))
		return -EOPNOTSUPP;

	if (ngbe_wol_exclusion(adapter, wol))
		return wol->wolopts ? -EOPNOTSUPP : 0;
	if ( !((hw->subsystem_device_id & WOL_SUP_MASK) == WOL_SUP))
		return -EOPNOTSUPP;
	adapter->wol = 0;

	if (wol->wolopts & WAKE_UCAST)
		adapter->wol |= NGBE_PSR_WKUP_CTL_EX;
	if (wol->wolopts & WAKE_MCAST)
		adapter->wol |= NGBE_PSR_WKUP_CTL_MC;
	if (wol->wolopts & WAKE_BCAST)
		adapter->wol |= NGBE_PSR_WKUP_CTL_BC;
	if (wol->wolopts & WAKE_MAGIC){
		adapter->wol |= NGBE_PSR_WKUP_CTL_MAG;
		hw->wol_enabled = !!(adapter->wol);
		wr32(hw, NGBE_PSR_WKUP_CTL, adapter->wol);
		ngbe_read_ee_hostif(hw, 0x7FE, &value);
		/*enable wol in shadow ram*/
		ngbe_write_ee_hostif(hw, 0x7FE, value | ( 1 << slot) );
		ngbe_write_ee_hostif(hw, 0x7FF, 0x5a5a);
		device_set_wakeup_enable(pci_dev_to_dev(adapter->pdev), adapter->wol);
		return 0;
	}
	
	ngbe_read_ee_hostif(hw, 0x7FE, &value);
	/*disable wol in shadow ram*/
	ngbe_write_ee_hostif(hw, 0x7FE, value & ~( 1 << slot ));
	ngbe_write_ee_hostif(hw, 0x7FF, 0x5a5a);
	return 0;
}

static int ngbe_nway_reset(struct net_device *netdev)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	if (netif_running(netdev))
		ngbe_reinit_locked(adapter);

	return 0;
}

#ifdef HAVE_ETHTOOL_SET_PHYS_ID
static int ngbe_set_phys_id(struct net_device *netdev,
			     enum ethtool_phys_id_state state)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;

	switch (state) {
	case ETHTOOL_ID_ACTIVE:
		
		if (adapter->hw.phy.type == ngbe_phy_yt8521s_sfi ||
			adapter->hw.phy.type == ngbe_phy_internal_yt8521s_sfi) {
			ngbe_phy_read_reg_ext_yt8521s(hw, 0xA00B, 0, (u16*)&adapter->led_reg);
		} else if (adapter->hw.phy.type == ngbe_phy_m88e1512 ||
			adapter->hw.phy.type == ngbe_phy_m88e1512_sfi) {
			TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 3);
			TCALL(hw, phy.ops.read_reg_mdi, 16, 0, (u16*)&adapter->led_reg);
		} else
			adapter->led_reg = rd32(hw, NGBE_CFG_LED_CTL);
		return 2;

	case ETHTOOL_ID_ON:
		if (adapter->hw.phy.type == ngbe_phy_yt8521s_sfi ||
			adapter->hw.phy.type == ngbe_phy_internal_yt8521s_sfi) {
			ngbe_phy_write_reg_ext_yt8521s(hw, 0xA00B, 0, adapter->led_reg | 0x140);
		} else if (adapter->hw.phy.type == ngbe_phy_m88e1512 ||
					adapter->hw.phy.type == ngbe_phy_m88e1512_sfi) {
			TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 3);
			TCALL(hw, phy.ops.write_reg_mdi, 16, 0, (adapter->led_reg & ~0xF) | 0x9);			
		} else		
			TCALL(hw, mac.ops.led_on, NGBE_LED_LINK_1G);
		break;

	case ETHTOOL_ID_OFF:
		if (adapter->hw.phy.type == ngbe_phy_yt8521s_sfi ||
			adapter->hw.phy.type == ngbe_phy_internal_yt8521s_sfi) {
			ngbe_phy_write_reg_ext_yt8521s(hw, 0xA00B, 0, adapter->led_reg | 0x100);
		} else if (adapter->hw.phy.type == ngbe_phy_m88e1512 ||
					adapter->hw.phy.type == ngbe_phy_m88e1512_sfi) {
			TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 3);
			TCALL(hw, phy.ops.write_reg_mdi, 16, 0, (adapter->led_reg & ~0xF) | 0x8);	
		} else			
			TCALL(hw, mac.ops.led_off, NGBE_LED_LINK_100M | NGBE_LED_LINK_1G);
		break;

	case ETHTOOL_ID_INACTIVE:
		/* Restore LED settings */
		if (adapter->hw.phy.type == ngbe_phy_yt8521s_sfi ||
			adapter->hw.phy.type == ngbe_phy_internal_yt8521s_sfi) {
			ngbe_phy_write_reg_ext_yt8521s(hw, 0xA00B, 0, adapter->led_reg);
		} else if (adapter->hw.phy.type == ngbe_phy_m88e1512 ||
			adapter->hw.phy.type == ngbe_phy_m88e1512_sfi) {
			TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 3);
			TCALL(hw, phy.ops.write_reg_mdi, 16, 0, adapter->led_reg);
		} else		
			wr32(&adapter->hw, NGBE_CFG_LED_CTL,
				adapter->led_reg);
		break;
	}

	return 0;
}
#else
static int ngbe_phys_id(struct net_device *netdev, u32 data)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	u32 led_reg;
	u32 i;

	if (!data || data > 300)
		data = 300;

	if (adapter->hw.phy.type == ngbe_phy_yt8521s_sfi ||
		adapter->hw.phy.type == ngbe_phy_internal_yt8521s_sfi) {
		ngbe_phy_read_reg_ext_yt8521s(hw, 0xA00B, 0, (u16*)&led_reg);
		for (i = 0; i < (data * 1000); i += 400) {
			ngbe_phy_write_reg_ext_yt8521s(hw, 0xA00B, 0, led_reg | 0x140);
			msleep_interruptible(200);
			ngbe_phy_write_reg_ext_yt8521s(hw, 0xA00B, 0, led_reg | 0x100);
			msleep_interruptible(200);
		}
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xA00B, 0, led_reg);
	} else if (adapter->hw.phy.type == ngbe_phy_m88e1512 ||
		adapter->hw.phy.type == ngbe_phy_m88e1512_sfi) {		
		TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 3);
		TCALL(hw, phy.ops.read_reg_mdi, 16, 0, (u16*)&led_reg);
		for (i = 0; i < (data * 1000); i += 400) {
			TCALL(hw, phy.ops.write_reg_mdi, 16, 0, (adapter->led_reg & ~0xF) | 0x9);
			msleep_interruptible(200);
			TCALL(hw, phy.ops.write_reg_mdi, 16, 0, (adapter->led_reg & ~0xF) | 0x8);
			msleep_interruptible(200);
		}
		TCALL(hw, phy.ops.write_reg_mdi, 16, 0, led_reg);
	} else {	
		led_reg = rd32(hw, NGBE_CFG_LED_CTL);
		for (i = 0; i < (data * 1000); i += 400) {
			TCALL(hw, mac.ops.led_on, NGBE_LED_LINK_1G);
			msleep_interruptible(200);
			TCALL(hw, mac.ops.led_off, NGBE_LED_LINK_100M | NGBE_LED_LINK_1G);
			msleep_interruptible(200);
		}
		/* Restore LED settings */
		wr32(hw, NGBE_CFG_LED_CTL, led_reg);
	}


	return 0;
}
#endif /* HAVE_ETHTOOL_SET_PHYS_ID */

static int ngbe_get_coalesce(struct net_device *netdev,
			      struct ethtool_coalesce *ec)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	ec->tx_max_coalesced_frames_irq = adapter->tx_work_limit;
	/* only valid if in constant ITR mode */
	if (adapter->rx_itr_setting <= 1)
		ec->rx_coalesce_usecs = adapter->rx_itr_setting;
	else
		ec->rx_coalesce_usecs = adapter->rx_itr_setting >> 2;

	/* if in mixed tx/rx queues per vector mode, report only rx settings */
	if (adapter->q_vector[0]->tx.count && adapter->q_vector[0]->rx.count)
		return 0;

	/* only valid if in constant ITR mode */
	if (adapter->tx_itr_setting <= 1)
		ec->tx_coalesce_usecs = adapter->tx_itr_setting;
	else
		ec->tx_coalesce_usecs = adapter->tx_itr_setting >> 2;

	return 0;
}

static int ngbe_set_coalesce(struct net_device *netdev,
			      struct ethtool_coalesce *ec)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	struct ngbe_q_vector *q_vector;
	int i;
	u16 tx_itr_param, rx_itr_param;
	u16  tx_itr_prev;
	bool need_reset = false;

	if (adapter->q_vector[0]->tx.count && adapter->q_vector[0]->rx.count) {
		/* reject Tx specific changes in case of mixed RxTx vectors */
		if (ec->tx_coalesce_usecs)
			return -EINVAL;
		tx_itr_prev = adapter->rx_itr_setting;
	} else {
		tx_itr_prev = adapter->tx_itr_setting;
	}

	if (ec->tx_max_coalesced_frames_irq)
		adapter->tx_work_limit = ec->tx_max_coalesced_frames_irq;

	if ((ec->rx_coalesce_usecs > (NGBE_MAX_EITR >> 2)) ||
	    (ec->tx_coalesce_usecs > (NGBE_MAX_EITR >> 2)))
		return -EINVAL;

	if (ec->rx_coalesce_usecs > 1)
		adapter->rx_itr_setting = ec->rx_coalesce_usecs << 2;
	else
		adapter->rx_itr_setting = ec->rx_coalesce_usecs;

	if (adapter->rx_itr_setting == 1)
		rx_itr_param = NGBE_20K_ITR;
	else
		rx_itr_param = adapter->rx_itr_setting;

	if (ec->tx_coalesce_usecs > 1)
		adapter->tx_itr_setting = ec->tx_coalesce_usecs << 2;
	else
		adapter->tx_itr_setting = ec->tx_coalesce_usecs;

	if (adapter->tx_itr_setting == 1)
		tx_itr_param = NGBE_20K_ITR;
	else
		tx_itr_param = adapter->tx_itr_setting;

	/* mixed Rx/Tx */
	if (adapter->q_vector[0]->tx.count && adapter->q_vector[0]->rx.count)
		adapter->tx_itr_setting = adapter->rx_itr_setting;

	/* detect ITR changes that require update of TXDCTL.WTHRESH */
	if ((adapter->tx_itr_setting != 1) &&
	    (adapter->tx_itr_setting < NGBE_70K_ITR)) {
		if ((tx_itr_prev == 1) ||
		    (tx_itr_prev >= NGBE_70K_ITR))
			need_reset = true;
	} else {
		if ((tx_itr_prev != 1) &&
		    (tx_itr_prev < NGBE_70K_ITR))
			need_reset = true;
	}

	if (adapter->hw.mac.dmac_config.watchdog_timer &&
	    (!adapter->rx_itr_setting && !adapter->tx_itr_setting)) {
		e_info(probe,
		       "Disabling DMA coalescing because interrupt throttling "
		       "is disabled\n");
		adapter->hw.mac.dmac_config.watchdog_timer = 0;
		TCALL(hw, mac.ops.dmac_config);
	}

	for (i = 0; i < adapter->num_q_vectors; i++) {
		q_vector = adapter->q_vector[i];
		q_vector->tx.work_limit = adapter->tx_work_limit;
		q_vector->rx.work_limit = adapter->rx_work_limit;
		if (q_vector->tx.count && !q_vector->rx.count)
			/* tx only */
			q_vector->itr = tx_itr_param;
		else
			/* rx only or mixed */
			q_vector->itr = rx_itr_param;
		ngbe_write_eitr(q_vector);
	}

	/*
	 * do reset here at the end to make sure EITR==0 case is handled
	 * correctly w.r.t stopping tx, and changing TXDCTL.WTHRESH settings
	 * also locks in RSC enable/disable which requires reset
	 */
	if (need_reset)
		ngbe_do_reset(netdev);

	return 0;
}

#ifndef HAVE_NDO_SET_FEATURES
static u32 ngbe_get_rx_csum(struct net_device *netdev)
{
	return !!(netdev->features & NETIF_F_RXCSUM);
}

static int ngbe_set_rx_csum(struct net_device *netdev, u32 data)
{
#ifdef HAVE_VXLAN_RX_OFFLOAD
	struct ngbe_adapter *adapter = netdev_priv(netdev);
#endif
	bool need_reset = false;

	if (data)
		netdev->features |= NETIF_F_RXCSUM;
	else
		netdev->features &= ~NETIF_F_RXCSUM;

	if (!data && (netdev->features & NETIF_F_LRO)) {
		netdev->features &= ~NETIF_F_LRO;
	}

#ifdef HAVE_VXLAN_RX_OFFLOAD
	if (adapter->flags & NGBE_FLAG_VXLAN_OFFLOAD_CAPABLE && data) {
		netdev->hw_enc_features |= NETIF_F_RXCSUM |
					   NETIF_F_IP_CSUM |
					   NETIF_F_IPV6_CSUM;
		if (!need_reset)
			adapter->flags2 |= NGBE_FLAG2_VXLAN_REREG_NEEDED;
	} else {
		netdev->hw_enc_features &= ~(NETIF_F_RXCSUM |
					     NETIF_F_IP_CSUM |
					     NETIF_F_IPV6_CSUM);
		ngbe_clear_vxlan_port(adapter);
	}
#endif /* HAVE_VXLAN_RX_OFFLOAD */

	if (need_reset)
		ngbe_do_reset(netdev);

	return 0;
}

static int ngbe_set_tx_csum(struct net_device *netdev, u32 data)
{
#ifdef NETIF_F_IPV6_CSUM
	u32 feature_list = NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
#else
	u32 feature_list = NETIF_F_IP_CSUM;
#endif


#ifdef HAVE_ENCAP_TSO_OFFLOAD
	if (data)
		netdev->hw_enc_features |= NETIF_F_GSO_UDP_TUNNEL;
	else
		netdev->hw_enc_features &= ~NETIF_F_GSO_UDP_TUNNEL;
	feature_list |= NETIF_F_GSO_UDP_TUNNEL;
#endif /* HAVE_ENCAP_TSO_OFFLOAD */
	feature_list |= NETIF_F_SCTP_CSUM;


	if (data)
		netdev->features |= feature_list;
	else
		netdev->features &= ~feature_list;

	return 0;
}

#ifdef NETIF_F_TSO
static int ngbe_set_tso(struct net_device *netdev, u32 data)
{
#ifdef NETIF_F_TSO6
	u32 feature_list = NETIF_F_TSO | NETIF_F_TSO6;
#else
	u32 feature_list = NETIF_F_TSO;
#endif

	if (data)
		netdev->features |= feature_list;
	else
		netdev->features &= ~feature_list;

#ifndef HAVE_NETDEV_VLAN_FEATURES
	if (!data) {
		struct ngbe_adapter *adapter = netdev_priv(netdev);
		struct net_device *v_netdev;
		int i;

		/* disable TSO on all VLANs if they're present */
		if (!adapter->vlgrp)
			goto tso_out;

		for (i = 0; i < VLAN_GROUP_ARRAY_LEN; i++) {
			v_netdev = vlan_group_get_device(adapter->vlgrp, i);
			if (!v_netdev)
				continue;

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
static int ngbe_set_flags(struct net_device *netdev, u32 data)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	u32 supported_flags = ETH_FLAG_RXVLAN | ETH_FLAG_TXVLAN;
#ifndef HAVE_VLAN_RX_REGISTER	
	u32 changed = netdev->features ^ data;
#endif
	bool need_reset = false;
	int rc;

#ifndef HAVE_VLAN_RX_REGISTER
	if ((adapter->flags & NGBE_FLAG_DCB_ENABLED) &&
	    !(data & ETH_FLAG_RXVLAN))
		return -EINVAL;

#endif
	supported_flags |= ETH_FLAG_LRO;

#ifdef ETHTOOL_GRXRINGS

	supported_flags |= ETH_FLAG_NTUPLE;


#endif
#ifdef NETIF_F_RXHASH
	supported_flags |= ETH_FLAG_RXHASH;

#endif
	rc = ethtool_op_set_flags(netdev, data, supported_flags);
	if (rc)
		return rc;

#ifndef HAVE_VLAN_RX_REGISTER
	if (changed & ETH_FLAG_RXVLAN)
		ngbe_vlan_mode(netdev, netdev->features);

#endif

#ifdef HAVE_VXLAN_CHECKS
	if (adapter->flags & NGBE_FLAG_VXLAN_OFFLOAD_CAPABLE &&
	    netdev->features & NETIF_F_RXCSUM) {
		vxlan_get_rx_port(netdev);
	else
		ngbe_clear_vxlan_port(adapter);
	}
#endif /* HAVE_VXLAN_RX_OFFLOAD */

#ifdef ETHTOOL_GRXRINGS
	/*
	 * Check if Flow Director n-tuple support was enabled or disabled.  If
	 * the state changed, we need to reset.
	 */
	switch (netdev->features & NETIF_F_NTUPLE) {
	case NETIF_F_NTUPLE:
		/* turn off ATR, enable perfect filters and reset */
		if (!(adapter->flags & NGBE_FLAG_FDIR_PERFECT_CAPABLE))
			need_reset = true;
		break;
	default:
		/* turn off perfect filters, enable ATR and reset */
		if (adapter->flags & NGBE_FLAG_FDIR_PERFECT_CAPABLE)
			need_reset = true;

		/* We cannot enable ATR if VMDq is enabled */
		if (adapter->flags & NGBE_FLAG_VMDQ_ENABLED)
			break;

		/* We cannot enable ATR if we have 2 or more traffic classes */
		if (netdev_get_num_tc(netdev) > 1)
			break;

		/* We cannot enable ATR if RSS is disabled */
		if (adapter->ring_feature[RING_F_RSS].limit <= 1)
			break;

		/* A sample rate of 0 indicates ATR disabled */
		if (!adapter->atr_sample_rate)
			break;

		break;
	}

#endif /* ETHTOOL_GRXRINGS */
	if (need_reset)
		ngbe_do_reset(netdev);

	return 0;
}

#endif /* ETHTOOL_GFLAGS */
#endif /* HAVE_NDO_SET_FEATURES */
#ifdef ETHTOOL_GRXRINGS

static int ngbe_get_rss_hash_opts(struct ngbe_adapter *adapter,
				   struct ethtool_rxnfc *cmd)
{
	cmd->data = 0;

	/* Report default options for RSS on ngbe */
	switch (cmd->flow_type) {
	case TCP_V4_FLOW:
		cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		/* fall through */
	case UDP_V4_FLOW:
		if (adapter->flags2 & NGBE_FLAG2_RSS_FIELD_IPV4_UDP)
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
		if (adapter->flags2 & NGBE_FLAG2_RSS_FIELD_IPV6_UDP)
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

static int ngbe_get_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd,
#ifdef HAVE_ETHTOOL_GET_RXNFC_VOID_RULE_LOCS
			   void *rule_locs)
#else
			   u32 *rule_locs)
#endif
{
	struct ngbe_adapter *adapter = netdev_priv(dev);
	int ret = -EOPNOTSUPP;

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
		cmd->data = adapter->num_rx_queues;
		ret = 0;
		break;
	case ETHTOOL_GRXCLSRLCNT:
		ret = 0;
		break;
	case ETHTOOL_GRXCLSRULE:
		break;
	case ETHTOOL_GRXCLSRLALL:
		break;
	case ETHTOOL_GRXFH:
		ret = ngbe_get_rss_hash_opts(adapter, cmd);
		break;
	default:
		break;
	}

	return ret;
}

#ifdef ETHTOOL_SRXNTUPLE
/*
 * We need to keep this around for kernels 2.6.33 - 2.6.39 in order to avoid
 * a null pointer dereference as it was assumend if the NETIF_F_NTUPLE flag
 * was defined that this function was present.
 */
static int ngbe_set_rx_ntuple(struct net_device __always_unused *dev,
			       struct ethtool_rx_ntuple __always_unused *cmd)
{
	return -EOPNOTSUPP;
}

#endif
#define UDP_RSS_FLAGS (NGBE_FLAG2_RSS_FIELD_IPV4_UDP | \
		       NGBE_FLAG2_RSS_FIELD_IPV6_UDP)
static int ngbe_set_rss_hash_opt(struct ngbe_adapter *adapter,
				  struct ethtool_rxnfc *nfc)
{
	u32 flags2 = adapter->flags2;

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
			flags2 &= ~NGBE_FLAG2_RSS_FIELD_IPV4_UDP;
			break;
		case (RXH_L4_B_0_1 | RXH_L4_B_2_3):
			flags2 |= NGBE_FLAG2_RSS_FIELD_IPV4_UDP;
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
			flags2 &= ~NGBE_FLAG2_RSS_FIELD_IPV6_UDP;
			break;
		case (RXH_L4_B_0_1 | RXH_L4_B_2_3):
			flags2 |= NGBE_FLAG2_RSS_FIELD_IPV6_UDP;
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
	if (flags2 != adapter->flags2) {
		struct ngbe_hw *hw = &adapter->hw;
		u32 mrqc;

		mrqc = rd32(hw, NGBE_RDB_RA_CTL);

		if ((flags2 & UDP_RSS_FLAGS) &&
		    !(adapter->flags2 & UDP_RSS_FLAGS))
			e_warn(drv, "enabling UDP RSS: fragmented packets"
			       " may arrive out of order to the stack above\n");

		adapter->flags2 = flags2;

		/* Perform hash on these packet types */
		mrqc |= NGBE_RDB_RA_CTL_RSS_IPV4
		      | NGBE_RDB_RA_CTL_RSS_IPV4_TCP
		      | NGBE_RDB_RA_CTL_RSS_IPV6
		      | NGBE_RDB_RA_CTL_RSS_IPV6_TCP;

		mrqc &= ~(NGBE_RDB_RA_CTL_RSS_IPV4_UDP |
			  NGBE_RDB_RA_CTL_RSS_IPV6_UDP);

		if (flags2 & NGBE_FLAG2_RSS_FIELD_IPV4_UDP)
			mrqc |= NGBE_RDB_RA_CTL_RSS_IPV4_UDP;

		if (flags2 & NGBE_FLAG2_RSS_FIELD_IPV6_UDP)
			mrqc |= NGBE_RDB_RA_CTL_RSS_IPV6_UDP;

		wr32(hw, NGBE_RDB_RA_CTL, mrqc);
	}

	return 0;
}

static int ngbe_set_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd)
{
	struct ngbe_adapter *adapter = netdev_priv(dev);
	int ret = -EOPNOTSUPP;

	switch (cmd->cmd) {
	case ETHTOOL_SRXCLSRLINS:
		break;
	case ETHTOOL_SRXCLSRLDEL:
		break;
	case ETHTOOL_SRXFH:
		ret = ngbe_set_rss_hash_opt(adapter, cmd);
		break;
	default:
		break;
	}

	return ret;
}

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
static int ngbe_rss_indir_tbl_max(struct ngbe_adapter *adapter)
{
	return 64;
}

static u32 ngbe_get_rxfh_key_size(struct net_device *netdev)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	return sizeof(adapter->rss_key);
}

static u32 ngbe_rss_indir_size(struct net_device *netdev)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	return ngbe_rss_indir_tbl_entries(adapter);
}

static void ngbe_get_reta(struct ngbe_adapter *adapter, u32 *indir)
{
	int i, reta_size = ngbe_rss_indir_tbl_entries(adapter);

	for (i = 0; i < reta_size; i++)
		indir[i] = adapter->rss_indir_tbl[i];
}

#ifdef HAVE_RXFH_HASHFUNC
static int ngbe_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key,
			  u8 *hfunc)
#else /* HAVE_RXFH_HASHFUNC */
//#ifdef HAVE_RXFH_HASHKEY
static int ngbe_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key)
//#else
//static int ngbe_get_rxfh(struct net_device *netdev, u32 *indir)
//#endif
#endif /* HAVE_RXFH_HASHFUNC */
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc)
		*hfunc = ETH_RSS_HASH_TOP;
#endif

	if (indir)
		ngbe_get_reta(adapter, indir);
//#ifdef HAVE_RXFH_HASHKEY
	if (key)
		memcpy(key, adapter->rss_key, ngbe_get_rxfh_key_size(netdev));
//#endif

	return 0;
}

#ifdef HAVE_RXFH_HASHFUNC
static int ngbe_set_rxfh(struct net_device *netdev, const u32 *indir,
			  const u8 *key, const u8 hfunc)
#else
#ifdef HAVE_RXFH_NONCONST
//#ifdef HAVE_RXFH_HASHKEY
static int ngbe_set_rxfh(struct net_device *netdev, u32 *indir, u8 *key)
//#else
//static int ngbe_set_rxfh(struct net_device *netdev, u32 *indir)
//#endif
#else /* HAVE_RXFH_NONCONST */
//#ifdef HAVE_RXFH_HASHKEY
static int ngbe_set_rxfh(struct net_device *netdev, const u32 *indir,
			  const u8 *key)
//#else
//static int ngbe_set_rxfh(struct net_device *netdev, const u32 *indir)
//#endif
#endif /* HAVE_RXFH_NONCONST */
#endif /* HAVE_RXFH_HASHFUNC */
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	int i;
	u32 reta_entries = ngbe_rss_indir_tbl_entries(adapter);

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc)
		return -EINVAL;
#endif

	/* Fill out the redirection table */
	if (indir) {
		int max_queues = min_t(int, adapter->num_rx_queues,
				       ngbe_rss_indir_tbl_max(adapter));

		/*Allow at least 2 queues w/ SR-IOV.*/
		if ((adapter->flags & NGBE_FLAG_SRIOV_ENABLED) &&
		    (max_queues < 2))
			max_queues = 2;

		/* Verify user input. */
		for (i = 0; i < reta_entries; i++)
			if (indir[i] >= max_queues)
				return -EINVAL;

		for (i = 0; i < reta_entries; i++)
			adapter->rss_indir_tbl[i] = indir[i];
	}

//#ifdef HAVE_RXFH_HASHKEY
	/* Fill out the rss hash key */
	if (key)
		memcpy(adapter->rss_key, key, ngbe_get_rxfh_key_size(netdev));
//#endif

	ngbe_store_reta(adapter);

	return 0;
}
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */

#ifdef HAVE_ETHTOOL_GET_TS_INFO
static int ngbe_get_ts_info(struct net_device *dev,
			     struct ethtool_ts_info *info)
{
	struct ngbe_adapter *adapter = netdev_priv(dev);

	/* we always support timestamping disabled */
	info->rx_filters = 1 << HWTSTAMP_FILTER_NONE;

#ifdef HAVE_PTP_1588_CLOCK

	info->so_timestamping =
		SOF_TIMESTAMPING_TX_SOFTWARE |
		SOF_TIMESTAMPING_RX_SOFTWARE |
		SOF_TIMESTAMPING_SOFTWARE |
		SOF_TIMESTAMPING_TX_HARDWARE |
		SOF_TIMESTAMPING_RX_HARDWARE |
		SOF_TIMESTAMPING_RAW_HARDWARE;

	if (adapter->ptp_clock)
		info->phc_index = ptp_clock_index(adapter->ptp_clock);
	else
		info->phc_index = -1;

	info->tx_types =
		(1 << HWTSTAMP_TX_OFF) |
		(1 << HWTSTAMP_TX_ON);

	info->rx_filters |=
		(1 << HWTSTAMP_FILTER_PTP_V1_L4_SYNC) |
		(1 << HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ) |
		(1 << HWTSTAMP_FILTER_PTP_V2_L2_EVENT) |
		(1 << HWTSTAMP_FILTER_PTP_V2_L4_EVENT) |
		(1 << HWTSTAMP_FILTER_PTP_V2_SYNC) |
		(1 << HWTSTAMP_FILTER_PTP_V2_L2_SYNC) |
		(1 << HWTSTAMP_FILTER_PTP_V2_L4_SYNC) |
		(1 << HWTSTAMP_FILTER_PTP_V2_DELAY_REQ) |
		(1 << HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ) |
		(1 << HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ) |
		(1 << HWTSTAMP_FILTER_PTP_V2_EVENT);

#endif /* HAVE_PTP_1588_CLOCK */
	return 0;
}
#endif /* HAVE_ETHTOOL_GET_TS_INFO */

#endif /* ETHTOOL_GRXRINGS */
#ifdef ETHTOOL_SCHANNELS
static unsigned int ngbe_max_channels(struct ngbe_adapter *adapter)
{
	unsigned int max_combined;
	u8 tcs = netdev_get_num_tc(adapter->netdev);

	if (!(adapter->flags & NGBE_FLAG_MSIX_ENABLED)) {
		/* We only support one q_vector without MSI-X */
		max_combined = 1;
	} else if (adapter->flags & NGBE_FLAG_SRIOV_ENABLED) {
		/* SR-IOV currently only allows one queue on the PF */
		max_combined = 1;
	} else if (tcs > 1) {
		/* For DCB report channels per traffic class */
		if (tcs > 4) {
			/* 8 TC w/ 8 queues per TC */
			max_combined = 8;
		} else {
			/* 4 TC w/ 16 queues per TC */
			max_combined = 16;
		}
	} else if (adapter->atr_sample_rate) {
		/* support up to 64 queues with ATR */
		max_combined = NGBE_MAX_FDIR_INDICES;
	} else {
		/* support up to max allowed queues with RSS */
		max_combined = ngbe_max_rss_indices(adapter);
	}

	return max_combined;
}

static void ngbe_get_channels(struct net_device *dev,
			       struct ethtool_channels *ch)
{
	struct ngbe_adapter *adapter = netdev_priv(dev);

	/* report maximum channels */
	ch->max_combined = ngbe_max_channels(adapter);

	/* report info for other vector */
	if (adapter->flags & NGBE_FLAG_MSIX_ENABLED) {
		ch->max_other = NON_Q_VECTORS;
		ch->other_count = NON_Q_VECTORS;
	}

	/* record RSS queues */
	ch->combined_count = adapter->ring_feature[RING_F_RSS].indices;

	/* nothing else to report if RSS is disabled */
	if (ch->combined_count == 1)
		return;

	/* we do not support ATR queueing if SR-IOV is enabled */
	if (adapter->flags & NGBE_FLAG_SRIOV_ENABLED)
		return;

	/* same thing goes for being DCB enabled */
	if (netdev_get_num_tc(dev) > 1)
		return;

	/* if ATR is disabled we can exit */
	if (!adapter->atr_sample_rate)
		return;

}

static int ngbe_set_channels(struct net_device *dev,
			      struct ethtool_channels *ch)
{
	struct ngbe_adapter *adapter = netdev_priv(dev);
	unsigned int count = ch->combined_count;
	u8 max_rss_indices = ngbe_max_rss_indices(adapter);

	/* verify they are not requesting separate vectors */
	if (!count || ch->rx_count || ch->tx_count)
		return -EINVAL;

	/* verify other_count has not changed */
	if (ch->other_count != NON_Q_VECTORS)
		return -EINVAL;

	/* verify the number of channels does not exceed hardware limits */
	if (count > ngbe_max_channels(adapter))
		return -EINVAL;

	/* cap RSS limit */
	if (count > max_rss_indices)
		count = max_rss_indices;
	adapter->ring_feature[RING_F_RSS].limit = count;

	/* use setup TC to update any traffic class queue mapping */
	return ngbe_setup_tc(dev, netdev_get_num_tc(dev));
}
#endif /* ETHTOOL_SCHANNELS */

#if 0
#ifdef ETHTOOL_GMODULEINFO
static int ngbe_get_module_info(struct net_device *dev,
				       struct ethtool_modinfo *modinfo)


static int ngbe_get_module_eeprom(struct net_device *dev,
					struct ethtool_eeprom *ee, u8 *data)
#endif /* ETHTOOL_GMODULEINFO */
#endif

#ifdef ETHTOOL_GEEE
static int ngbe_get_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
	return 0;
}
#endif /* ETHTOOL_GEEE */

#ifdef ETHTOOL_SEEE
static int ngbe_set_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	struct ethtool_eee eee_data;
	s32 ret_val;

	if (!(hw->mac.ops.setup_eee &&
	    (adapter->flags2 & NGBE_FLAG2_EEE_CAPABLE)))
		return -EOPNOTSUPP;

	memset(&eee_data, 0, sizeof(struct ethtool_eee));

	ret_val = ngbe_get_eee(netdev, &eee_data);
	if (ret_val)
		return ret_val;

	if (eee_data.eee_enabled && !edata->eee_enabled) {
		if (eee_data.tx_lpi_enabled != edata->tx_lpi_enabled) {
			e_dev_err("Setting EEE tx-lpi is not supported\n");
			return -EINVAL;
		}

		if (eee_data.tx_lpi_timer != edata->tx_lpi_timer) {
			e_dev_err("Setting EEE Tx LPI timer is not "
				  "supported\n");
			return -EINVAL;
		}

		if (eee_data.advertised != edata->advertised) {
			e_dev_err("Setting EEE advertised speeds is not "
				  "supported\n");
			return -EINVAL;
		}

	}

	if (eee_data.eee_enabled != edata->eee_enabled) {

		if (edata->eee_enabled)
			adapter->flags2 |= NGBE_FLAG2_EEE_ENABLED;
		else
			adapter->flags2 &= ~NGBE_FLAG2_EEE_ENABLED;

		/* reset link */
		if (netif_running(netdev))
			ngbe_reinit_locked(adapter);
		else
			ngbe_reset(adapter);
	}

	return 0;
}
#endif /* ETHTOOL_SEEE */

static int ngbe_set_flash(struct net_device *netdev, struct ethtool_flash *ef)
{
	int ret;
	const struct firmware *fw;
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	ret = request_firmware(&fw, ef->data, &netdev->dev);
	if (ret < 0)
		return ret;

	if (ef->region == 0) {
		ret = ngbe_upgrade_flash(&adapter->hw, ef->region,
								fw->data, fw->size);
	} else {
		if (ngbe_mng_present(&adapter->hw)) {
			ret = ngbe_upgrade_flash_hostif(&adapter->hw, ef->region,
											fw->data, fw->size);
		} else
			ret = -EOPNOTSUPP;
	}

	release_firmware(fw);
	if (!ret)
		dev_info(&netdev->dev,
			 "loaded firmware %s, reboot to make firmware work\n", ef->data);
	return ret;
}


static struct ethtool_ops ngbe_ethtool_ops = {
#ifdef ETHTOOL_COALESCE_USECS
	.supported_coalesce_params = ETHTOOL_COALESCE_USECS,
#endif
#ifdef HAVE_ETHTOOL_CONVERT_U32_AND_LINK_MODE
	.get_link_ksettings = ngbe_get_link_ksettings,
	.set_link_ksettings = ngbe_set_link_ksettings,
#else
	.get_settings           = ngbe_get_settings,
	.set_settings           = ngbe_set_settings,
#endif
	.get_drvinfo            = ngbe_get_drvinfo,
	.get_regs_len           = ngbe_get_regs_len,
	.get_regs               = ngbe_get_regs,
	.get_wol                = ngbe_get_wol,
	.set_wol                = ngbe_set_wol,
	.nway_reset             = ngbe_nway_reset,
	.get_link               = ethtool_op_get_link,
	.get_eeprom_len         = ngbe_get_eeprom_len,
	.get_eeprom             = ngbe_get_eeprom,
	.set_eeprom             = ngbe_set_eeprom,
	.get_ringparam          = ngbe_get_ringparam,
	.set_ringparam          = ngbe_set_ringparam,
	.get_pauseparam         = ngbe_get_pauseparam,
	.set_pauseparam         = ngbe_set_pauseparam,
	.get_msglevel           = ngbe_get_msglevel,
	.set_msglevel           = ngbe_set_msglevel,
#ifndef HAVE_ETHTOOL_GET_SSET_COUNT
	.self_test_count        = ngbe_diag_test_count,
#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */
	.self_test              = ngbe_diag_test,
	.get_strings            = ngbe_get_strings,
#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
#ifdef HAVE_ETHTOOL_SET_PHYS_ID
	.set_phys_id            = ngbe_set_phys_id,
#else
	.phys_id                = ngbe_phys_id,
#endif /* HAVE_ETHTOOL_SET_PHYS_ID */
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
#ifndef HAVE_ETHTOOL_GET_SSET_COUNT
	.get_stats_count        = ngbe_get_stats_count,
#else /* HAVE_ETHTOOL_GET_SSET_COUNT */
	.get_sset_count         = ngbe_get_sset_count,
#endif /* HAVE_ETHTOOL_GET_SSET_COUNT */
	.get_ethtool_stats      = ngbe_get_ethtool_stats,
#ifdef HAVE_ETHTOOL_GET_PERM_ADDR
	.get_perm_addr          = ethtool_op_get_perm_addr,
#endif
	.get_coalesce           = ngbe_get_coalesce,
	.set_coalesce           = ngbe_set_coalesce,
#ifndef HAVE_NDO_SET_FEATURES
	.get_rx_csum            = ngbe_get_rx_csum,
	.set_rx_csum            = ngbe_set_rx_csum,
	.get_tx_csum            = ethtool_op_get_tx_csum,
	.set_tx_csum            = ngbe_set_tx_csum,
	.get_sg                 = ethtool_op_get_sg,
	.set_sg                 = ethtool_op_set_sg,
#ifdef NETIF_F_TSO
	.get_tso                = ethtool_op_get_tso,
	.set_tso                = ngbe_set_tso,
#endif
#ifdef ETHTOOL_GFLAGS
	.get_flags              = ethtool_op_get_flags,
	.set_flags              = ngbe_set_flags,
#endif
#endif /* HAVE_NDO_SET_FEATURES */
#ifdef ETHTOOL_GRXRINGS
	.get_rxnfc              = ngbe_get_rxnfc,
	.set_rxnfc              = ngbe_set_rxnfc,
#ifdef ETHTOOL_SRXNTUPLE
	.set_rx_ntuple          = ngbe_set_rx_ntuple,
#endif
#endif /* ETHTOOL_GRXRINGS */
#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT


#if 0
#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
#ifdef HAVE_RXFH_HASHKEY
	.get_rxfh_indir_size    = ngbe_rss_indir_size,
	.get_rxfh_key_size      = ngbe_get_rxfh_key_size,
	.get_rxfh               = ngbe_get_rxfh,
	.set_rxfh               = ngbe_set_rxfh,
#else/* HAVE_RXFH_HASHKEY */
	.get_rxfh_indir_size    = ngbe_rss_indir_size,
	.get_rxfh_indir         = ngbe_get_rxfh,
	.set_rxfh_indir         = ngbe_set_rxfh,
#endif /* HAVE_RXFH_HASHKEY */
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */
#endif

#ifdef ETHTOOL_GEEE
	.get_eee                = ngbe_get_eee,
#endif /* ETHTOOL_GEEE */
#ifdef ETHTOOL_SEEE
	.set_eee                = ngbe_set_eee,
#endif /* ETHTOOL_SEEE */
#ifdef ETHTOOL_SCHANNELS
	.get_channels           = ngbe_get_channels,
	.set_channels           = ngbe_set_channels,
#endif
#if 0
#ifdef ETHTOOL_GMODULEINFO
	.get_module_info        = ngbe_get_module_info,
	.get_module_eeprom      = ngbe_get_module_eeprom,
#endif
#endif
#ifdef HAVE_ETHTOOL_GET_TS_INFO
	.get_ts_info            = ngbe_get_ts_info,
#endif
#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
	.get_rxfh_indir_size	= ngbe_rss_indir_size,
	.get_rxfh_key_size	= ngbe_get_rxfh_key_size,
	.get_rxfh		= ngbe_get_rxfh,
	.set_rxfh		= ngbe_set_rxfh,
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
	.flash_device      	= ngbe_set_flash,
};

#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
static const struct ethtool_ops_ext ngbe_ethtool_ops_ext = {
	.size                   = sizeof(struct ethtool_ops_ext),
	.get_ts_info            = ngbe_get_ts_info,
	.set_phys_id            = ngbe_set_phys_id,
	.get_channels           = ngbe_get_channels,
	.set_channels           = ngbe_set_channels,
#if 0
#ifdef ETHTOOL_GMODULEINFO
	.get_module_info        = ngbe_get_module_info,
	.get_module_eeprom      = ngbe_get_module_eeprom,
#endif
#endif
#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
	.get_rxfh_indir_size	= ngbe_rss_indir_size,
	.get_rxfh_key_size	= ngbe_get_rxfh_key_size,
	.get_rxfh		= ngbe_get_rxfh,
	.set_rxfh		= ngbe_set_rxfh,
#endif /* ETHTOOL_GRSSH && ETHTOOL_SRSSH */
#ifdef ETHTOOL_GEEE
	.get_eee                = ngbe_get_eee,
#endif /* ETHTOOL_GEEE */
#ifdef ETHTOOL_SEEE
	.set_eee                = ngbe_set_eee,
#endif /* ETHTOOL_SEEE */
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
#endif /* SIOCETHTOOL */

