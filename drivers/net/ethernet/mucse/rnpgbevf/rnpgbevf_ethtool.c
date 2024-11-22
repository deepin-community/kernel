// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/types.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/vmalloc.h>
#include <linux/if_vlan.h>
#include <linux/uaccess.h>

#include "rnpgbevf.h"

#define RNPGBE_ALL_RAR_ENTRIES 16

struct rnpgbevf_stats {
	char stat_string[ETH_GSTRING_LEN];
	int sizeof_stat;
	int stat_offset;
	int base_stat_offset;
	int saved_reset_offset;
};

#define RNPVF_NUM_RX_QUEUES netdev->real_num_rx_queues
#define RNPVF_NUM_TX_QUEUES netdev->real_num_tx_queues

#define RNPGBE_NETDEV_STAT(_net_stat)                                          \
	{                                                                      \
		.stat_string = #_net_stat,                                     \
		.sizeof_stat =                                                 \
			sizeof_field(struct net_device_stats, _net_stat),      \
		.stat_offset = offsetof(struct net_device_stats, _net_stat)    \
	}

static const struct rnpgbevf_stats rnp_gstrings_net_stats[] = {
	RNPGBE_NETDEV_STAT(rx_packets),
	RNPGBE_NETDEV_STAT(tx_packets),
	RNPGBE_NETDEV_STAT(rx_bytes),
	RNPGBE_NETDEV_STAT(tx_bytes),
	RNPGBE_NETDEV_STAT(rx_errors),
	RNPGBE_NETDEV_STAT(tx_errors),
	RNPGBE_NETDEV_STAT(rx_dropped),
	RNPGBE_NETDEV_STAT(tx_dropped),
	RNPGBE_NETDEV_STAT(multicast),
	RNPGBE_NETDEV_STAT(collisions),
	RNPGBE_NETDEV_STAT(rx_over_errors),
	RNPGBE_NETDEV_STAT(rx_crc_errors),
	RNPGBE_NETDEV_STAT(rx_frame_errors),
	RNPGBE_NETDEV_STAT(rx_fifo_errors),
	RNPGBE_NETDEV_STAT(rx_missed_errors),
	RNPGBE_NETDEV_STAT(tx_aborted_errors),
	RNPGBE_NETDEV_STAT(tx_carrier_errors),
	RNPGBE_NETDEV_STAT(tx_fifo_errors),
	RNPGBE_NETDEV_STAT(tx_heartbeat_errors),
};

#define RNPVF_GLOBAL_STATS_LEN ARRAY_SIZE(rnp_gstrings_net_stats)
#define RNPVF_HW_STAT(_name, _stat)                                            \
	{                                                                      \
		.stat_string = _name,                                          \
		.sizeof_stat = sizeof_field(struct rnpgbevf_adapter, _stat),   \
		.stat_offset = offsetof(struct rnpgbevf_adapter, _stat)        \
	}
static struct rnpgbevf_stats rnpgbevf_hwstrings_stats[] = {
	RNPVF_HW_STAT("vlan_add_cnt", hw_stats.vlan_add_cnt),
	RNPVF_HW_STAT("vlan_strip_cnt", hw_stats.vlan_strip_cnt),
	RNPVF_HW_STAT("rx_csum_offload_errors", hw_stats.csum_err),
	RNPVF_HW_STAT("rx_csum_offload_good", hw_stats.csum_good),
};

#define RNPVF_HWSTRINGS_STATS_LEN ARRAY_SIZE(rnpgbevf_hwstrings_stats)

struct rnpgbevf_tx_queue_ring_stat {
	u64 hw_head;
	u64 hw_tail;
	u64 sw_to_clean;
};

struct rnpgbevf_rx_queue_ring_stat {
	u64 hw_head;
	u64 hw_tail;
	u64 sw_to_use;
};

#define RNPGBE_QUEUE_STATS_LEN                                                 \
	(RNPVF_NUM_TX_QUEUES *                                                 \
		 (sizeof(struct rnpgbevf_tx_queue_stats) / sizeof(u64) +       \
		  sizeof(struct rnpgbevf_queue_stats) / sizeof(u64) +          \
		  sizeof(struct rnpgbevf_tx_queue_ring_stat) / sizeof(u64)) +  \
	 RNPVF_NUM_RX_QUEUES *                                                 \
		 (sizeof(struct rnpgbevf_rx_queue_stats) / sizeof(u64) +       \
		  sizeof(struct rnpgbevf_queue_stats) / sizeof(u64) +          \
		  sizeof(struct rnpgbevf_rx_queue_ring_stat) / sizeof(u64)))

#define RNPVF_STATS_LEN                                                        \
	(RNPVF_GLOBAL_STATS_LEN + RNPGBE_QUEUE_STATS_LEN +                     \
	 RNPVF_HWSTRINGS_STATS_LEN)

static const char rnp_gstrings_test[][ETH_GSTRING_LEN] = {
	"Register test  (offline)", "Link test   (on/offline)"
};

#define RNPVF_TEST_LEN (sizeof(rnp_gstrings_test) / ETH_GSTRING_LEN)

enum priv_bits {
	padding_enable = 0,
};

static const char rnpgbevf_priv_flags_strings[][ETH_GSTRING_LEN] = {
#define RNPVF_FT_PADDING BIT(0)
#define RNPVF_FCS_ON BIT(1)
	"ft_padding", "fcs"
};

#define RNPVF_PRIV_FLAGS_STR_LEN ARRAY_SIZE(rnpgbevf_priv_flags_strings)

#define ADVERTISED_MASK_10G                                                    \
	(SUPPORTED_10000baseT_Full | SUPPORTED_10000baseKX4_Full |             \
	 SUPPORTED_10000baseKR_Full)
static int rnpgbevf_get_link_ksettings(struct net_device *netdev,
				       struct ethtool_link_ksettings *cmd)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct rnpgbevf_hw *hw = &adapter->hw;
	bool autoneg = false;
	bool link_up;
	u32 supported;
	u32 advertising = 0;
	u32 link_speed = 0;

	ethtool_convert_link_mode_to_legacy_u32(&supported,
						cmd->link_modes.supported);

	hw->mac.ops.check_link(hw, &link_speed, &link_up, false);

	switch (link_speed) {
	case RNPGBE_LINK_SPEED_1GB_FULL:
		supported |= SUPPORTED_1000baseT_Full;
		supported |= SUPPORTED_FIBRE;
		advertising |= ADVERTISED_FIBRE | ADVERTISED_1000baseKX_Full;
		cmd->base.port = PORT_FIBRE;
		break;
	case RNPGBE_LINK_SPEED_10GB_FULL:
		supported |= SUPPORTED_10000baseT_Full;
		supported |= SUPPORTED_FIBRE;
		advertising |= ADVERTISED_FIBRE | SUPPORTED_10000baseT_Full;
		cmd->base.port = PORT_FIBRE;
		break;
	case RNPGBE_LINK_SPEED_25GB_FULL:
		supported |= SUPPORTED_40000baseKR4_Full;
		supported |= SUPPORTED_FIBRE;
		advertising |= ADVERTISED_FIBRE | SUPPORTED_40000baseKR4_Full;
		cmd->base.port = PORT_FIBRE;
		break;
	case RNPGBE_LINK_SPEED_40GB_FULL:
		supported |= SUPPORTED_40000baseCR4_Full |
			     SUPPORTED_40000baseSR4_Full |
			     SUPPORTED_40000baseLR4_Full;
		supported |= SUPPORTED_FIBRE;
		advertising |= ADVERTISED_FIBRE;
		cmd->base.port = PORT_FIBRE;
		break;
	}

	if (autoneg) {
		supported |= SUPPORTED_Autoneg;
		advertising |= ADVERTISED_Autoneg;
		cmd->base.autoneg = AUTONEG_ENABLE;
	} else
		cmd->base.autoneg = AUTONEG_DISABLE;

	/* set pause support */
	supported |= SUPPORTED_Pause;

	switch (hw->fc.current_mode) {
	case rnp_fc_full:
		advertising |= ADVERTISED_Pause;
		break;
	case rnp_fc_rx_pause:
		advertising |= ADVERTISED_Pause | ADVERTISED_Asym_Pause;
		break;
	case rnp_fc_tx_pause:
		advertising |= ADVERTISED_Asym_Pause;
		break;
	default:
		advertising &= ~(ADVERTISED_Pause | ADVERTISED_Asym_Pause);
	}

	if (link_up) {
		switch (link_speed) {
		case RNPGBE_LINK_SPEED_40GB_FULL:
			cmd->base.speed = SPEED_40000;
			break;
		case RNPGBE_LINK_SPEED_25GB_FULL:
			cmd->base.speed = SPEED_25000;
			break;
		case RNPGBE_LINK_SPEED_10GB_FULL:
			cmd->base.speed = SPEED_10000;
			break;
		case RNPGBE_LINK_SPEED_1GB_FULL:
			cmd->base.speed = SPEED_1000;
			break;
		case RNPGBE_LINK_SPEED_100_FULL:
			cmd->base.speed = SPEED_100;
			break;
		default:
			break;
		}
		cmd->base.duplex = DUPLEX_FULL;
	} else {
		cmd->base.speed = SPEED_UNKNOWN;
		cmd->base.duplex = DUPLEX_UNKNOWN;
	}

	ethtool_convert_legacy_u32_to_link_mode(cmd->link_modes.supported,
						supported);
	ethtool_convert_legacy_u32_to_link_mode(cmd->link_modes.advertising,
						supported);
	return 0;
}

static void rnpgbevf_get_drvinfo(struct net_device *netdev,
				 struct ethtool_drvinfo *drvinfo)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct rnpgbevf_hw *hw = &adapter->hw;

	strlcpy(drvinfo->driver, rnpgbevf_driver_name, sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, rnpgbevf_driver_version,
		sizeof(drvinfo->version));
	strlcpy(drvinfo->bus_info, pci_name(adapter->pdev),
		sizeof(drvinfo->bus_info));
	if (hw->board_type == rnp_board_n10) {
		snprintf(drvinfo->fw_version, sizeof(drvinfo->fw_version),
			 "%d.%d.%d.%d", ((char *)&(hw->fw_version))[3],
			 ((char *)&(hw->fw_version))[2],
			 ((char *)&(hw->fw_version))[1],
			 ((char *)&(hw->fw_version))[0]);
	} else if (hw->board_type == rnp_board_n500) {
		snprintf(drvinfo->fw_version, sizeof(drvinfo->fw_version),
			 "%d.%d.%d.%d", ((char *)&(hw->fw_version))[3],
			 ((char *)&(hw->fw_version))[2],
			 ((char *)&(hw->fw_version))[1],
			 ((char *)&(hw->fw_version))[0]);
	} else if (hw->board_type == rnp_board_n210) {
		snprintf(drvinfo->fw_version, sizeof(drvinfo->fw_version),
			 "fw %x", hw->fw_version);
	}
	drvinfo->n_priv_flags = RNPVF_PRIV_FLAGS_STR_LEN;
}

void rnpgbevf_get_ringparam(struct net_device *netdev,
			    struct ethtool_ringparam *ring,
			    struct kernel_ethtool_ringparam __always_unused *ker,
			    struct netlink_ext_ack __always_unused *extack)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);

	ring->rx_max_pending = RNPVF_MAX_RXD;
	ring->tx_max_pending = RNPVF_MAX_TXD;
	ring->rx_pending = adapter->rx_ring_item_count;
	ring->tx_pending = adapter->tx_ring_item_count;
}

int rnpgbevf_set_ringparam(struct net_device *netdev,
			   struct ethtool_ringparam *ring,
			   struct kernel_ethtool_ringparam __always_unused *ker,
			   struct netlink_ext_ack __always_unused *extack)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct rnpgbevf_ring *temp_ring;
	int i, err = 0;
	u32 new_rx_count, new_tx_count;

	if ((ring->rx_mini_pending) || (ring->rx_jumbo_pending))
		return -EINVAL;

	new_tx_count =
		clamp_t(u32, ring->tx_pending, RNPVF_MIN_TXD, RNPVF_MAX_TXD);
	new_tx_count = ALIGN(new_tx_count, RNPGBE_REQ_TX_DESCRIPTOR_MULTIPLE);

	new_rx_count =
		clamp_t(u32, ring->rx_pending, RNPVF_MIN_RXD, RNPVF_MAX_RXD);
	new_rx_count = ALIGN(new_rx_count, RNPGBE_REQ_RX_DESCRIPTOR_MULTIPLE);

	if ((new_tx_count == adapter->tx_ring_item_count) &&
	    (new_rx_count == adapter->rx_ring_item_count)) {
		/* nothing to do */
		return 0;
	}

	while (test_and_set_bit(__RNPVF_RESETTING, &adapter->state))
		usleep_range(1000, 2000);

	if (!netif_running(adapter->netdev)) {
		for (i = 0; i < adapter->num_tx_queues; i++)
			adapter->tx_ring[i]->count = new_tx_count;
		for (i = 0; i < adapter->num_rx_queues; i++)
			adapter->rx_ring[i]->count = new_rx_count;
		adapter->tx_ring_item_count = new_tx_count;
		adapter->rx_ring_item_count = new_rx_count;
		goto clear_reset;
	}

	/* allocate temporary buffer to store rings in */
	i = max_t(int, adapter->num_tx_queues, adapter->num_rx_queues);
	temp_ring = vmalloc(i * sizeof(struct rnpgbevf_ring));

	if (!temp_ring) {
		err = -ENOMEM;
		goto clear_reset;
	}
	memset(temp_ring, 0x00, i * sizeof(struct rnpgbevf_ring));

	rnpgbevf_down(adapter);

	/*
	 * Setup new Tx resources and free the old Tx resources in that order.
	 * We can then assign the new resources to the rings via a memcpy.
	 * The advantage to this approach is that we are guaranteed to still
	 * have resources even in the case of an allocation failure.
	 */
	if (new_tx_count != adapter->tx_ring_item_count) {
		for (i = 0; i < adapter->num_tx_queues; i++) {
			memcpy(&temp_ring[i], adapter->tx_ring[i],
			       sizeof(struct rnpgbevf_ring));

			temp_ring[i].count = new_tx_count;
			err = rnpgbevf_setup_tx_resources(adapter,
							  &temp_ring[i]);
			if (err) {
				while (i) {
					i--;
					rnpgbevf_free_tx_resources(
						adapter, &temp_ring[i]);
				}
				goto err_setup;
			}
		}

		for (i = 0; i < adapter->num_tx_queues; i++) {
			rnpgbevf_free_tx_resources(adapter,
						   adapter->tx_ring[i]);

			memcpy(adapter->tx_ring[i], &temp_ring[i],
			       sizeof(struct rnpgbevf_ring));
		}

		adapter->tx_ring_item_count = new_tx_count;
	}

	/* Repeat the process for the Rx rings if needed */
	if (new_rx_count != adapter->rx_ring_item_count) {
		for (i = 0; i < adapter->num_rx_queues; i++) {
			memcpy(&temp_ring[i], adapter->rx_ring[i],
			       sizeof(struct rnpgbevf_ring));

			temp_ring[i].count = new_rx_count;
			err = rnpgbevf_setup_rx_resources(adapter,
							  &temp_ring[i]);
			if (err) {
				while (i) {
					i--;
					rnpgbevf_free_rx_resources(
						adapter, &temp_ring[i]);
				}
				goto err_setup;
			}
		}

		for (i = 0; i < adapter->num_rx_queues; i++) {
			rnpgbevf_free_rx_resources(adapter,
						   adapter->rx_ring[i]);

			memcpy(adapter->rx_ring[i], &temp_ring[i],
			       sizeof(struct rnpgbevf_ring));
		}

		adapter->rx_ring_item_count = new_rx_count;
	}

err_setup:
	rnpgbevf_up(adapter);
	vfree(temp_ring);
clear_reset:
	clear_bit(__RNPVF_RESETTING, &adapter->state);
	return err;
}

static void rnpgbevf_get_strings(struct net_device *netdev, u32 stringset,
				 u8 *data)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	char *p = (char *)data;
	int i;
	struct rnpgbevf_ring *ring;
	u16 queue_idx;

	switch (stringset) {
	case ETH_SS_STATS:
		for (i = 0; i < RNPVF_GLOBAL_STATS_LEN; i++) {
			memcpy(p, rnp_gstrings_net_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}

		for (i = 0; i < RNPVF_HWSTRINGS_STATS_LEN; i++) {
			memcpy(p, rnpgbevf_hwstrings_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}

		BUG_ON(RNPVF_NUM_TX_QUEUES != RNPVF_NUM_RX_QUEUES);

		for (i = 0; i < RNPVF_NUM_TX_QUEUES; i++) {
			/* ====  tx ======== */
			ring = adapter->tx_ring[i];
			queue_idx = ring->rnpgbevf_queue_idx;
			sprintf(p, "\n     queue%u_tx_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_bytes", i);
			p += ETH_GSTRING_LEN;

			sprintf(p, "queue%u_tx_restart", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_busy", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_done_old", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_clean_desc", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_poll_count", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_irq_more", i);
			p += ETH_GSTRING_LEN;

			sprintf(p, "queue%u_tx_hw_head", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_hw_tail", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_sw_next_to_clean", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_added_vlan_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_irq_miss", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_next_to_clean", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_tx_equal_count", i);
			p += ETH_GSTRING_LEN;

			/* ====  rx ======== */
			ring = adapter->rx_ring[i];
			queue_idx = ring->rnpgbevf_queue_idx;
			sprintf(p, "\n     queue%u_rx_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_bytes", i);
			p += ETH_GSTRING_LEN;

			sprintf(p, "queue%u_rx_driver_drop_packets", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_rsc", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_rsc_flush", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_non_eop_descs", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_alloc_page_failed", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_alloc_buff_failed", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_alloc_page", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_csum_err", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_csum_good", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_poll_again_count", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_poll_count", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_rm_vlan_packets", i);
			p += ETH_GSTRING_LEN;

			sprintf(p, "queue%u_rx_hw_head", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_hw_tail", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_sw_next_to_use", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_irq_miss", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_next_to_clean", i);
			p += ETH_GSTRING_LEN;
			sprintf(p, "queue%u_rx_equal_count", i);
			p += ETH_GSTRING_LEN;
		}
		break;
	case ETH_SS_PRIV_FLAGS:
		memcpy(data, rnpgbevf_priv_flags_strings,
		       RNPVF_PRIV_FLAGS_STR_LEN * ETH_GSTRING_LEN);
		break;
	}
}

static int rnpgbevf_get_sset_count(struct net_device *netdev, int sset)
{
	switch (sset) {
	/* now we don't support test */
	case ETH_SS_STATS:
		return RNPVF_STATS_LEN;
	case ETH_SS_PRIV_FLAGS:
		return RNPVF_PRIV_FLAGS_STR_LEN;
	default:
		return -EOPNOTSUPP;
	}
}

static u32 rnpgbevf_get_priv_flags(struct net_device *netdev)
{
	struct rnpgbevf_adapter *adapter =
		(struct rnpgbevf_adapter *)netdev_priv(netdev);
	u32 priv_flags = 0;

	if (adapter->priv_flags & RNPVF_PRIV_FLAG_FT_PADDING)
		priv_flags |= RNPVF_FT_PADDING;
	if (adapter->priv_flags & RNPVF_PRIV_FLAG_FCS_ON)
		priv_flags |= RNPVF_FCS_ON;

	return priv_flags;
}

static int rnpgbevf_get_coalesce(struct net_device *netdev,
				 struct ethtool_coalesce *coal,
				 struct kernel_ethtool_coalesce *kernel_coal,
				 struct netlink_ext_ack *extack)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);

	coal->use_adaptive_tx_coalesce = adapter->adaptive_tx_coal;
	coal->tx_coalesce_usecs = adapter->tx_usecs;
	coal->tx_coalesce_usecs_irq = 0;
	coal->tx_max_coalesced_frames = adapter->tx_frames;
	coal->tx_max_coalesced_frames_irq = adapter->tx_work_limit;

	coal->use_adaptive_rx_coalesce = adapter->adaptive_rx_coal;
	coal->rx_coalesce_usecs_irq = 0;
	coal->rx_coalesce_usecs = adapter->rx_usecs;
	coal->rx_max_coalesced_frames = adapter->rx_frames;
	coal->rx_max_coalesced_frames_irq = adapter->napi_budge;

	/* this is not support */
	coal->pkt_rate_low = 0;
	coal->pkt_rate_high = 0;
	coal->rx_coalesce_usecs_low = 0;
	coal->rx_max_coalesced_frames_low = 0;
	coal->tx_coalesce_usecs_low = 0;
	coal->tx_max_coalesced_frames_low = 0;
	coal->rx_coalesce_usecs_high = 0;
	coal->rx_max_coalesced_frames_high = 0;
	coal->tx_coalesce_usecs_high = 0;
	coal->tx_max_coalesced_frames_high = 0;
	coal->rate_sample_interval = 0;
	return 0;
}

static int rnpgbevf_set_coalesce(struct net_device *netdev,
				 struct ethtool_coalesce *ec,
				 struct kernel_ethtool_coalesce *kernel_coal,
				 struct netlink_ext_ack *extack)
{
	int reset = 0;
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	u32 value;
	/* we don't support close tx and rx coalesce */
	if (!(ec->use_adaptive_tx_coalesce) ||
	    !(ec->use_adaptive_rx_coalesce)) {
		return -EINVAL;
	}

	if ((ec->tx_max_coalesced_frames_irq < RNPVF_MIN_TX_WORK) ||
	    (ec->tx_max_coalesced_frames_irq > RNPVF_MAX_TX_WORK))
		return -EINVAL;

	value = clamp_t(u32, ec->tx_max_coalesced_frames_irq, RNPVF_MIN_TX_WORK,
			RNPVF_MAX_TX_WORK);
	value = ALIGN(value, RNPVF_WORK_ALIGN);

	if (adapter->tx_work_limit != value) {
		reset = 1;
		adapter->tx_work_limit = value;
	}

	if ((ec->tx_max_coalesced_frames < RNPVF_MIN_TX_FRAME) ||
	    (ec->tx_max_coalesced_frames > RNPVF_MAX_TX_FRAME))
		return -EINVAL;

	value = clamp_t(u32, ec->tx_max_coalesced_frames, RNPVF_MIN_TX_FRAME,
			RNPVF_MAX_TX_FRAME);
	if (adapter->tx_frames != value) {
		reset = 1;
		adapter->tx_frames = value;
	}

	if ((ec->tx_coalesce_usecs < RNPVF_MIN_TX_USEC) ||
	    (ec->tx_coalesce_usecs > RNPVF_MAX_TX_USEC))
		return -EINVAL;

	value = clamp_t(u32, ec->tx_coalesce_usecs, RNPVF_MIN_TX_USEC,
			RNPVF_MAX_TX_USEC);
	if (adapter->tx_usecs != value) {
		reset = 1;
		adapter->tx_usecs = value;
	}

	if ((ec->rx_max_coalesced_frames_irq < RNPVF_MIN_RX_WORK) ||
	    (ec->rx_max_coalesced_frames_irq > RNPVF_MAX_RX_WORK))
		return -EINVAL;

	value = clamp_t(u32, ec->rx_max_coalesced_frames_irq, RNPVF_MIN_RX_WORK,
			RNPVF_MAX_RX_WORK);
	value = ALIGN(value, RNPVF_WORK_ALIGN);

	if (adapter->napi_budge != value) {
		reset = 1;
		adapter->napi_budge = value;
	}

	if ((ec->rx_max_coalesced_frames < RNPVF_MIN_RX_FRAME) ||
	    (ec->rx_max_coalesced_frames > RNPVF_MAX_RX_FRAME))
		return -EINVAL;

	value = clamp_t(u32, ec->rx_max_coalesced_frames, RNPVF_MIN_RX_FRAME,
			RNPVF_MAX_RX_FRAME);
	if (adapter->rx_frames != value) {
		reset = 1;
		adapter->rx_frames = value;
	}

	if ((ec->rx_coalesce_usecs < RNPVF_MIN_RX_USEC) ||
	    (ec->rx_coalesce_usecs > RNPVF_MAX_RX_USEC))
		return -EINVAL;

	value = clamp_t(u32, ec->rx_coalesce_usecs, RNPVF_MIN_RX_USEC,
			RNPVF_MAX_RX_USEC);

	if (adapter->rx_usecs != value) {
		reset = 1;
		adapter->rx_usecs = value;
	}
	/* other setup is not supported */
	if ((ec->pkt_rate_low) || (ec->pkt_rate_high) ||
	    (ec->rx_coalesce_usecs_low) || (ec->rx_max_coalesced_frames_low) ||
	    (ec->tx_coalesce_usecs_low) || (ec->tx_max_coalesced_frames_low) ||
	    (ec->rx_coalesce_usecs_high) ||
	    (ec->rx_max_coalesced_frames_high) ||
	    (ec->tx_coalesce_usecs_high) ||
	    (ec->tx_max_coalesced_frames_high) || (ec->rate_sample_interval) ||
	    (ec->tx_coalesce_usecs_irq) || (ec->rx_coalesce_usecs_irq))
		return -EINVAL;

	if (reset) {
		if (netif_running(netdev))
			rnpgbevf_close(netdev);
		remove_mbx_irq(adapter);
		rnpgbevf_clear_interrupt_scheme(adapter);
		rnpgbevf_init_interrupt_scheme(adapter);
		register_mbx_irq(adapter);
		if (netif_running(netdev))
			return rnpgbevf_open(netdev);
	}
	return 0;
}

static void rnpgbevf_get_ethtool_stats(struct net_device *netdev,
				       struct ethtool_stats *stats, u64 *data)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct net_device_stats *net_stats = &netdev->stats;
	struct rnpgbevf_ring *ring;
	int i = 0, j;
	char *p = NULL;
	struct rnpgbevf_hw *hw = &adapter->hw;

	rnpgbevf_update_stats(adapter);
	net_stats->multicast += rd32(hw, RNPVF500_VEB_VFMPRC(0));

	for (i = 0; i < RNPVF_GLOBAL_STATS_LEN; i++) {
		p = (char *)net_stats + rnp_gstrings_net_stats[i].stat_offset;
		data[i] =
			(rnp_gstrings_net_stats[i].sizeof_stat == sizeof(u64)) ?
				*(u64 *)p :
				*(u32 *)p;
	}
	for (j = 0; j < RNPVF_HWSTRINGS_STATS_LEN; j++, i++) {
		p = (char *)adapter + rnpgbevf_hwstrings_stats[j].stat_offset;
		data[i] = (rnpgbevf_hwstrings_stats[j].sizeof_stat ==
			   sizeof(u64)) ?
				  *(u64 *)p :
				  *(u32 *)p;
	}

	BUG_ON(RNPVF_NUM_TX_QUEUES != RNPVF_NUM_RX_QUEUES);

	for (j = 0; j < RNPVF_NUM_TX_QUEUES; j++) {
		/* ===== tx-ring == */
		ring = adapter->tx_ring[j];

		if (!ring) {
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			/* rnpgbevf_tx_queue_ring_stat */
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			/* ===== rx-ring == */
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
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			data[i++] = 0;
			continue;
		}

		data[i++] = ring->stats.packets;
		data[i++] = ring->stats.bytes;

		data[i++] = ring->tx_stats.restart_queue;
		data[i++] = ring->tx_stats.tx_busy;
		data[i++] = ring->tx_stats.tx_done_old;
		data[i++] = ring->tx_stats.clean_desc;
		data[i++] = ring->tx_stats.poll_count;
		data[i++] = ring->tx_stats.irq_more_count;

		/* rnpgbevf_tx_queue_ring_stat */
		data[i++] = ring_rd32(ring, RNPGBE_DMA_REG_TX_DESC_BUF_HEAD);
		data[i++] = ring_rd32(ring, RNPGBE_DMA_REG_TX_DESC_BUF_TAIL);
		data[i++] = ring->next_to_clean;
		data[i++] = ring->tx_stats.vlan_add;
		data[i++] = ring->tx_stats.tx_irq_miss;
		if (ring->tx_stats.tx_next_to_clean == -1)
			data[i++] = ring->count;
		else
			data[i++] = ring->tx_stats.tx_next_to_clean;
		data[i++] = ring->tx_stats.tx_equal_count;

		/* ===== rx-ring == */
		ring = adapter->rx_ring[j];

		if (!ring) {
			/* ===== rx-ring == */
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
			continue;
		}

		data[i++] = ring->stats.packets;
		data[i++] = ring->stats.bytes;
		data[i++] = ring->rx_stats.driver_drop_packets;
		data[i++] = ring->rx_stats.rsc_count;
		data[i++] = ring->rx_stats.rsc_flush;
		data[i++] = ring->rx_stats.non_eop_descs;
		data[i++] = ring->rx_stats.alloc_rx_page_failed;
		data[i++] = ring->rx_stats.alloc_rx_buff_failed;
		data[i++] = ring->rx_stats.alloc_rx_page;
		data[i++] = ring->rx_stats.csum_err;
		data[i++] = ring->rx_stats.csum_good;
		data[i++] = ring->rx_stats.poll_again_count;
		data[i++] = ring->rx_stats.poll_count;
		data[i++] = ring->rx_stats.vlan_remove;
		data[i++] = ring_rd32(ring, RNPGBE_DMA_REG_RX_DESC_BUF_HEAD);
		data[i++] = ring_rd32(ring, RNPGBE_DMA_REG_RX_DESC_BUF_TAIL);
		data[i++] = ring->next_to_clean;
		data[i++] = ring->rx_stats.rx_irq_miss;
		if (ring->rx_stats.rx_next_to_clean == -1)
			data[i++] = ring->count;
		else
			data[i++] = ring->rx_stats.rx_next_to_clean;
		data[i++] = ring->rx_stats.rx_equal_count;
	}
}

static void rnpgbevf_get_channels(struct net_device *dev,
				  struct ethtool_channels *ch)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(dev);

	/* report maximum channels */
	ch->max_combined = min_t(int, adapter->hw.mac.max_tx_queues,
				 adapter->hw.mac.max_rx_queues);

	/* report info for other vector */
	ch->max_other = NON_Q_VECTORS;
	ch->other_count = NON_Q_VECTORS;

	/* record RSS queues */
	ch->combined_count = adapter->dma_channels;
}

static u32 rnpgbevf_get_msglevel(struct net_device *netdev)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);

	return adapter->msg_enable;
}

static void rnpgbevf_get_pauseparam(struct net_device *netdev,
				    struct ethtool_pauseparam *pause)
{
	pause->autoneg = 0;
	/* vf fixed off */
	pause->rx_pause = 0;
	pause->rx_pause = 0;
}

static void rnpgbevf_set_msglevel(struct net_device *netdev, u32 data)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);

	adapter->msg_enable = data;
}

static const struct ethtool_ops rnpgbevf_ethtool_ops = {

	.get_link_ksettings = rnpgbevf_get_link_ksettings,
	.get_drvinfo = rnpgbevf_get_drvinfo,
	.get_link = ethtool_op_get_link,
	.get_ringparam = rnpgbevf_get_ringparam,
	.set_ringparam = rnpgbevf_set_ringparam,
	.get_strings = rnpgbevf_get_strings,
	/* vf juset get status */
	.get_pauseparam = rnpgbevf_get_pauseparam,
	.get_msglevel = rnpgbevf_get_msglevel,
	.set_msglevel = rnpgbevf_set_msglevel,
	.get_sset_count = rnpgbevf_get_sset_count,
	.get_priv_flags = rnpgbevf_get_priv_flags,
	.get_ethtool_stats = rnpgbevf_get_ethtool_stats,
	.get_coalesce = rnpgbevf_get_coalesce,
	.set_coalesce = rnpgbevf_set_coalesce,
	.supported_coalesce_params = ETHTOOL_COALESCE_USECS |
				     ETHTOOL_COALESCE_MAX_FRAMES_IRQ |
				     ETHTOOL_COALESCE_MAX_FRAMES,
	.get_channels = rnpgbevf_get_channels,
};

void rnpgbevf_set_ethtool_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &rnpgbevf_ethtool_ops;
}
