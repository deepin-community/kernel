// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/if_vlan.h>
#include <linux/ethtool.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_cfg.h"
#include "sss_nic_vf_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_rss_cfg.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_tx.h"
#include "sss_nic_rx.h"
#include "sss_nic_ethtool_stats_api.h"

typedef int (*sss_nic_ss_handler_t)(struct sss_nic_dev *nic_dev);

struct sss_nic_handler {
	int  type;
	sss_nic_ss_handler_t handler_func;
};

typedef void (*sss_nic_strings_handler_t)(struct sss_nic_dev *nic_dev,
		u8 *buffer);

struct sss_nic_get_strings {
	int type;
	sss_nic_strings_handler_t handler_func;
};

int sss_nic_get_sset_count(struct net_device *netdev, int settings)
{
	int i;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	struct sss_nic_handler handler[] = {
		{ETH_SS_TEST, sss_nic_eth_ss_test},
		{ETH_SS_STATS, sss_nic_eth_ss_stats},
		{ETH_SS_PRIV_FLAGS, sss_nic_eth_ss_priv_flags},
	};

	for (i = 0; i < ARRAY_LEN(handler); i++)
		if (settings == handler[i].type)
			return handler[i].handler_func(nic_dev);

	return -EOPNOTSUPP;
}

void sss_nic_get_ethtool_stats(struct net_device *netdev,
			       struct ethtool_stats *stats, u64 *data)
{
	u16 cnt;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	cnt = sss_nic_get_ethtool_dev_stats(nic_dev, data);

	cnt += sss_nic_get_ethtool_vport_stats(nic_dev, data + cnt);

	if (!SSSNIC_FUNC_IS_VF(nic_dev->hwdev))
		cnt += sss_nic_get_ethtool_port_stats(nic_dev, data + cnt);

	sss_nic_get_drv_queue_stats(nic_dev, data + cnt);
}

void sss_nic_get_strings(struct net_device *netdev, u32 stringset, u8 *buf)
{
	int i;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	struct sss_nic_get_strings handler[] = {
		{ETH_SS_TEST, sss_nic_get_test_strings},
		{ETH_SS_STATS, sss_nic_get_drv_stats_strings},
		{ETH_SS_PRIV_FLAGS, sss_nic_get_priv_flags_strings},
	};

	for (i = 0; i < ARRAY_LEN(handler); i++)
		if (stringset == handler[i].type)
			return handler[i].handler_func(nic_dev, buf);

	nicif_err(nic_dev, drv, netdev, "Invalid string set %u.", stringset);
}

#ifdef ETHTOOL_GLINKSETTINGS
#ifndef XENSERVER_HAVE_NEW_ETHTOOL_OPS
int sss_nic_get_link_ksettings(struct net_device *net_dev,
			       struct ethtool_link_ksettings *ksetting)
{
	int ret;
	struct sss_nic_cmd_link_settings cmd = {0};

	sss_nic_ethtool_ksetting_clear(ksetting, supported);
	sss_nic_ethtool_ksetting_clear(ksetting, advertising);

	ret = sss_nic_get_link_setting(net_dev, &cmd);
	if (ret != 0)
		return ret;

	sss_nic_copy_ksetting(ksetting, &cmd);

	return 0;
}
#endif
#endif

#ifdef ETHTOOL_GLINKSETTINGS
#ifndef XENSERVER_HAVE_NEW_ETHTOOL_OPS
int sss_nic_set_link_ksettings(struct net_device *netdev,
			       const struct ethtool_link_ksettings *ksettings)
{
	/* Only support to set autoneg and speed */
	return sssnic_set_link_settings(netdev,
					ksettings->base.autoneg, ksettings->base.speed);
}
#endif
#endif
