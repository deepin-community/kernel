/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_ETHTOOL_STATS_H
#define SSS_NIC_ETHTOOL_STATS_H

#include <linux/types.h>
#include <linux/netdevice.h>

#include "sss_kernel.h"

void sss_nic_get_strings(struct net_device *netdev, u32 stringset, u8 *buf);
void sss_nic_get_ethtool_stats(struct net_device *netdev,
			       struct ethtool_stats *stats, u64 *data);
int sss_nic_get_sset_count(struct net_device *netdev, int settings);

#ifdef ETHTOOL_GLINKSETTINGS
#ifndef XENSERVER_HAVE_NEW_ETHTOOL_OPS
int sss_nic_get_link_ksettings(struct net_device *net_dev,
			       struct ethtool_link_ksettings *ksetting);
int sss_nic_set_link_ksettings(struct net_device *netdev,
			       const struct ethtool_link_ksettings *ksettings);
#endif
#endif

#endif
