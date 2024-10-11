/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#ifndef _RNP_ETHTOOL_H_
#define _RNP_ETHTOOL_H_

enum { NETDEV_STATS, RNP_STATS };

struct rnp_stats {
	char stat_string[ETH_GSTRING_LEN];
	int sizeof_stat;
	int stat_offset;
};

/* rnp allocates num_tx_queues and num_rx_queues symmetrically so
 * we set the num_rx_queues to evaluate to num_tx_queues. This is
 * used because we do not have a good way to get the max number of
 * rx queues with CONFIG_RPS disabled.
 */
#define RNP_NUM_RX_QUEUES netdev->real_num_rx_queues
#define RNP_NUM_TX_QUEUES netdev->real_num_tx_queues

#define RNP_NETDEV_STAT(_net_stat)                                          \
	{                                                                   \
		.stat_string = #_net_stat,                                  \
		.sizeof_stat =                                              \
			sizeof_field(struct net_device_stats, _net_stat),   \
		.stat_offset = offsetof(struct net_device_stats, _net_stat) \
	}

#define RNP_HW_STAT(_name, _stat)                                       \
	{                                                               \
		.stat_string = _name,                                   \
		.sizeof_stat = sizeof_field(struct rnp_adapter, _stat), \
		.stat_offset = offsetof(struct rnp_adapter, _stat)      \
	}

struct rnp_tx_queue_ring_stat {
	u64 hw_head;
	u64 hw_tail;
	u64 sw_to_clean;
	u64 sw_to_next_to_use;
};

struct rnp_rx_queue_ring_stat {
	u64 hw_head;
	u64 hw_tail;
	u64 sw_to_use;
	u64 sw_to_clean;
};

#define RNP_QUEUE_STATS_LEN                                              \
	(RNP_NUM_TX_QUEUES *                                             \
		 (sizeof(struct rnp_tx_queue_stats) / sizeof(u64) +      \
		  sizeof(struct rnp_queue_stats) / sizeof(u64) +         \
		  sizeof(struct rnp_tx_queue_ring_stat) / sizeof(u64)) + \
	 RNP_NUM_RX_QUEUES *                                             \
		 (sizeof(struct rnp_rx_queue_stats) / sizeof(u64) +      \
		  sizeof(struct rnp_queue_stats) / sizeof(u64) +         \
		  sizeof(struct rnp_rx_queue_ring_stat) / sizeof(u64)))

#define RNP_STATS_LEN \
	(RNP_GLOBAL_STATS_LEN + RNP_HWSTRINGS_STATS_LEN + RNP_QUEUE_STATS_LEN)

int rnp_wol_exclusion(struct rnp_adapter *adapter, struct ethtool_wolinfo *wol);
void rnp_get_wol(struct net_device *netdev, struct ethtool_wolinfo *wol);
int rnp_wol_exclusion(struct rnp_adapter *adapter, struct ethtool_wolinfo *wol);
int rnp_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wol);
void rnp_diag_test(struct net_device *netdev, struct ethtool_test *eth_test,
		   u64 *data);
void rnp_get_pauseparam(struct net_device *netdev,
			struct ethtool_pauseparam *pause);
int rnp_set_pauseparam(struct net_device *netdev,
		       struct ethtool_pauseparam *pause);
int rnp_get_fecparam(struct net_device *netdev,
		     struct ethtool_fecparam *fecparam);
int rnp_set_fecparam(struct net_device *netdev,
		     struct ethtool_fecparam *fecparam);
u32 rnp_get_msglevel(struct net_device *netdev);
void rnp_set_msglevel(struct net_device *netdev, u32 data);
int rnp_set_phys_id(struct net_device *netdev,
		    enum ethtool_phys_id_state state);
int rnp_get_ts_info(struct net_device *dev, struct ethtool_ts_info *info);
void rnp_get_channels(struct net_device *dev, struct ethtool_channels *ch);
int rnp_set_channels(struct net_device *dev, struct ethtool_channels *ch);
int rnp_get_module_info(struct net_device *dev,
			struct ethtool_modinfo *modinfo);
int rnp_get_module_eeprom(struct net_device *dev, struct ethtool_eeprom *eeprom,
			  u8 *data);
void rnp_get_ringparam(struct net_device *netdev,
		       struct ethtool_ringparam *ring,
		       struct kernel_ethtool_ringparam __always_unused *ker,
		       struct netlink_ext_ack __always_unused *extack);
int rnp_set_ringparam(struct net_device *netdev, struct ethtool_ringparam *ring,
		      struct kernel_ethtool_ringparam __always_unused *ker,
		      struct netlink_ext_ack __always_unused *extack);
int rnp_get_dump_flag(struct net_device *netdev, struct ethtool_dump *dump);
int rnp_get_dump_data(struct net_device *netdev, struct ethtool_dump *dump,
		      void *buffer);
int rnp_set_dump(struct net_device *netdev, struct ethtool_dump *dump);
int rnp_get_coalesce(struct net_device *netdev,
		     struct ethtool_coalesce *coal,
		     struct kernel_ethtool_coalesce *kernel_coal,
		     struct netlink_ext_ack *extack);
int rnp_set_coalesce(struct net_device *netdev,
		     struct ethtool_coalesce *ec,
		     struct kernel_ethtool_coalesce *kernel_coal,
		     struct netlink_ext_ack *extack);
int rnp_get_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd,
		  u32 *rule_locs);
int rnp_set_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd);
u32 rnp_rss_indir_size(struct net_device *netdev);
u32 rnp_get_rxfh_key_size(struct net_device *netdev);
void rnp_get_reta(struct rnp_adapter *adapter, u32 *indir);
int rnp_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key, u8 *hfunc);
int rnp_flash_device(struct net_device *dev, struct ethtool_flash *flash);
int rnp_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key,
		 const u8 hfunc);
#define RNP_WOL_GET_SUPPORTED(adapter) (!!(adapter->wol & GENMASK(3, 0)))
#define RNP_WOL_GET_STATUS(adapter) (!!(adapter->wol & GENMASK(7, 4)))
#define RNP_WOL_SET_SUPPORTED(adapter) (adapter->wol |= BIT(0))
#define RNP_WOL_SET_STATUS(adapter) (adapter->wol |= BIT(4))
#define RNP_WOL_CLEAR_STATUS(adapter) (adapter->wol &= ~BIT(4))

#endif
