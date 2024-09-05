#ifndef __SXEVF_ETHTOOL_H__
#define __SXEVF_ETHTOOL_H__

#include <linux/ethtool.h>
#include "sxevf.h"

#define SXEVF_TEST_GSTRING_ARRAY_SIZE sxevf_self_test_suite_num_get()

#define SXEVF_RING_STATS_LEN ( \
	(((struct sxevf_adapter *)netdev_priv(netdev))->tx_ring_ctxt.num + \
	((struct sxevf_adapter *)netdev_priv(netdev))->xdp_ring_ctxt.num + \
	((struct sxevf_adapter *)netdev_priv(netdev))->rx_ring_ctxt.num) * \
	(sizeof(struct sxevf_ring_stats) / sizeof(u64)))

#define SXEVF_STATS_ARRAY_SIZE sxevf_stats_num_get()

#define SXEVF_STATS_LEN (SXEVF_STATS_ARRAY_SIZE + SXEVF_RING_STATS_LEN)

#define SXEVF_PRIV_FLAGS_LEGACY_RX	BIT(0)
#define SXEVF_PRIV_FLAGS_STR_LEN	sxevf_priv_flags_num_get()

enum {
	NETDEV_STATS,
	SXEVF_STATS
};

struct sxevf_ethtool_stats {
	char stat_string[ETH_GSTRING_LEN];
	int type;
	int sizeof_stat;
	int stat_offset;
};

u32 sxevf_self_test_suite_num_get(void);

u32 sxevf_stats_num_get(void);

u32 sxevf_priv_flags_num_get(void);

void sxevf_ethtool_ops_set(struct net_device *netdev);

#endif 