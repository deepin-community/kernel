/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#ifndef _RNPGBE_ETHTOOL_H_
#define _RNPGBE_ETHTOOL_H_

enum { NETDEV_STATS, RNP_STATS };

struct rnpgbe_stats {
	char stat_string[ETH_GSTRING_LEN];
	int sizeof_stat;
	int stat_offset;
};

/* rnp allocates num_tx_queues and num_rx_queues symmetrically so
 * we set the num_rx_queues to evaluate to num_tx_queues. This is
 * used because we do not have a good way to get the max number of
 * rx queues with CONFIG_RPS disabled.
 */
#define RNP_NUM_RX_QUEUES netdev->num_tx_queues
#define RNP_NUM_TX_QUEUES netdev->num_tx_queues

#define RNP_NETDEV_STAT(_net_stat)                                             \
	{                                                                      \
		.stat_string = #_net_stat,                                     \
		.sizeof_stat =                                                 \
			sizeof_field(struct net_device_stats, _net_stat),      \
		.stat_offset = offsetof(struct net_device_stats, _net_stat)    \
	}

#define RNP_HW_STAT(_name, _stat)                                              \
	{                                                                      \
		.stat_string = _name,                                          \
		.sizeof_stat = sizeof_field(struct rnpgbe_adapter, _stat),     \
		.stat_offset = offsetof(struct rnpgbe_adapter, _stat)          \
	}

struct rnpgbe_tx_queue_ring_stat {
	u64 hw_head;
	u64 hw_tail;
	u64 sw_to_clean;
	u64 sw_to_next_to_use;
};

struct rnpgbe_rx_queue_ring_stat {
	u64 hw_head;
	u64 hw_tail;
	u64 sw_to_use;
	u64 sw_to_clean;
};

#define RNP_QUEUE_STATS_LEN                                                    \
	(RNP_NUM_TX_QUEUES *                                                   \
		 (sizeof(struct rnpgbe_tx_queue_stats) / sizeof(u64) +         \
		  sizeof(struct rnpgbe_queue_stats) / sizeof(u64) +            \
		  sizeof(struct rnpgbe_tx_queue_ring_stat) / sizeof(u64)) +    \
	 RNP_NUM_RX_QUEUES *                                                   \
		 (sizeof(struct rnpgbe_rx_queue_stats) / sizeof(u64) +         \
		  sizeof(struct rnpgbe_queue_stats) / sizeof(u64) +            \
		  sizeof(struct rnpgbe_rx_queue_ring_stat) / sizeof(u64)))

#define RNP_STATS_LEN                                                          \
	(RNP_GLOBAL_STATS_LEN + RNP_HWSTRINGS_STATS_LEN + RNP_QUEUE_STATS_LEN)

struct ethtool_keee {
        __ETHTOOL_DECLARE_LINK_MODE_MASK(supported);
        __ETHTOOL_DECLARE_LINK_MODE_MASK(advertised);
        __ETHTOOL_DECLARE_LINK_MODE_MASK(lp_advertised);
        u32     supported_u32;
        u32     advertised_u32;
        u32     lp_advertised_u32;
        u32     tx_lpi_timer;
        bool    tx_lpi_enabled;
        bool    eee_active;
        bool    eee_enabled;
};

int rnpgbe_wol_exclusion(struct rnpgbe_adapter *adapter,
			 struct ethtool_wolinfo *wol);
void rnpgbe_get_wol(struct net_device *netdev, struct ethtool_wolinfo *wol);
int rnpgbe_wol_exclusion(struct rnpgbe_adapter *adapter,
			 struct ethtool_wolinfo *wol);
int rnpgbe_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wol);
void rnpgbe_diag_test(struct net_device *netdev, struct ethtool_test *eth_test,
		      u64 *data);
u32 rnpgbe_get_msglevel(struct net_device *netdev);
void rnpgbe_set_msglevel(struct net_device *netdev, u32 data);
int rnpgbe_set_phys_id(struct net_device *netdev,
		       enum ethtool_phys_id_state state);
int rnpgbe_get_ts_info(struct net_device *dev, struct ethtool_ts_info *info);
void rnpgbe_get_channels(struct net_device *dev, struct ethtool_channels *ch);
int rnpgbe_set_channels(struct net_device *dev, struct ethtool_channels *ch);
int rnpgbe_get_module_info(struct net_device *dev,
			   struct ethtool_modinfo *modinfo);
int rnpgbe_get_module_eeprom(struct net_device *dev,
			     struct ethtool_eeprom *eeprom, u8 *data);
int rnpgbe_get_keee(struct net_device *netdev, struct ethtool_keee *edata);
int rnpgbe_get_eee(struct net_device *netdev, struct ethtool_eee *edata);
int rnpgbe_set_keee(struct net_device *netdev, struct ethtool_keee *edata);
int rnpgbe_set_eee(struct net_device *netdev, struct ethtool_eee *edata);
void rnpgbe_get_ringparam(struct net_device *netdev,
			  struct ethtool_ringparam *ring,
			  struct kernel_ethtool_ringparam __always_unused *ker,
			  struct netlink_ext_ack __always_unused *extack);
int rnpgbe_set_ringparam(struct net_device *netdev,
			 struct ethtool_ringparam *ring,
			 struct kernel_ethtool_ringparam __always_unused *ker,
			 struct netlink_ext_ack __always_unused *extack);
int rnpgbe_get_dump_flag(struct net_device *netdev, struct ethtool_dump *dump);
int rnpgbe_get_dump_data(struct net_device *netdev, struct ethtool_dump *dump,
			 void *buffer);
int rnpgbe_set_dump(struct net_device *netdev, struct ethtool_dump *dump);
int rnpgbe_get_coalesce(struct net_device *netdev,
			struct ethtool_coalesce *coal,
			struct kernel_ethtool_coalesce *kernel_coal,
			struct netlink_ext_ack *extack);
int rnpgbe_set_coalesce(struct net_device *netdev,
			struct ethtool_coalesce *ec,
			struct kernel_ethtool_coalesce *kernel_coal,
			struct netlink_ext_ack *extack);
int rnpgbe_get_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd,
		     u32 *rule_locs);
int rnpgbe_set_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd);
u32 rnpgbe_rss_indir_size(struct net_device *netdev);
u32 rnpgbe_get_rxfh_key_size(struct net_device *netdev);
void rnpgbe_get_reta(struct rnpgbe_adapter *adapter, u32 *indir);
int rnpgbe_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key, u8 *hfunc);
int rnpgbe_flash_device(struct net_device *dev, struct ethtool_flash *flash);
int rnpgbe_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key,
		    const u8 hfunc);

#endif /* _RNPGBE_ETHTOOL_H_ */
