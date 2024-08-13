/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_ETHTOOL_STATS_API_H
#define SSS_NIC_ETHTOOL_STATS_API_H

#include <linux/types.h>
#include <linux/netdevice.h>

#include "sss_kernel.h"

struct sss_nic_stats {
	char name[ETH_GSTRING_LEN];
	u32 len;
	int offset;
};

struct sss_nic_cmd_link_settings {
	__ETHTOOL_DECLARE_LINK_MODE_MASK(supported);
	__ETHTOOL_DECLARE_LINK_MODE_MASK(advertising);

	u32 speed;
	u8 duplex;
	u8 port;
	u8 autoneg;
};

#define sss_nic_ethtool_ksetting_clear(ptr, name) \
	ethtool_link_ksettings_zero_link_mode(ptr, name)

int sss_nic_eth_ss_test(struct sss_nic_dev *nic_dev);

int sss_nic_eth_ss_stats(struct sss_nic_dev *nic_dev);

int sss_nic_eth_ss_priv_flags(struct sss_nic_dev *nic_dev);

u16 sss_nic_get_ethtool_dev_stats(struct sss_nic_dev *nic_dev,
				  u64 *data);

void sss_nic_get_drv_queue_stats(struct sss_nic_dev *nic_dev,
				 u64 *data);

int sss_nic_get_ethtool_vport_stats(struct sss_nic_dev *nic_dev,
				    u64 *data);

u16 sss_nic_get_ethtool_port_stats(struct sss_nic_dev *nic_dev,
				   u64 *data);

u16 sss_nic_get_stats_strings(struct sss_nic_stats *stats,
			      u16 stats_len, char *buffer);

u16 sss_nic_get_drv_dev_strings(struct sss_nic_dev *nic_dev,
				char *buffer);

u16 sss_nic_get_hw_stats_strings(struct sss_nic_dev *nic_dev,
				 char *buffer);

int sss_nic_get_queue_stats_cnt(const struct sss_nic_dev *nic_dev,
				struct sss_nic_stats *stats, u16 stats_len, u16 qid, char *buffer);

u16 sss_nic_get_qp_stats_strings(const struct sss_nic_dev *nic_dev,
				 char *buffer);

void sss_nic_get_test_strings(struct sss_nic_dev *nic_dev, u8 *buffer);

void sss_nic_get_drv_stats_strings(struct sss_nic_dev *nic_dev,
				   u8 *buffer);

void sss_nic_get_priv_flags_strings(struct sss_nic_dev *nic_dev,
				    u8 *buffer);

int sss_nic_get_speed_level(u32 speed);

void sss_nic_add_ethtool_link_mode(struct sss_nic_cmd_link_settings *cmd, u32 hw_mode, u32 op);

void sss_nic_set_link_speed(struct sss_nic_dev *nic_dev,
			    struct sss_nic_cmd_link_settings *cmd,
			    struct sss_nic_port_info *port_info);

void sss_nic_link_port_type(struct sss_nic_cmd_link_settings *cmd,
			    u8 port_type);

int sss_nic_get_link_pause_setting(struct sss_nic_dev *nic_dev,
				   struct sss_nic_cmd_link_settings *cmd);

int sss_nic_get_link_setting(struct net_device *net_dev,
			     struct sss_nic_cmd_link_settings *cmd);

#ifdef ETHTOOL_GLINKSETTINGS
#ifndef XENSERVER_HAVE_NEW_ETHTOOL_OPS
void sss_nic_copy_ksetting(struct ethtool_link_ksettings *ksetting,
			   struct sss_nic_cmd_link_settings *cmd);
#endif
#endif

bool sss_nic_is_support_speed(u32 support_mode, u32 speed);

int sss_nic_get_link_settings_param(struct sss_nic_dev *nic_dev,
				    u8 autoneg, u32 speed, u32 *settings);

int sss_nic_set_settings_to_hw(struct sss_nic_dev *nic_dev,
			       u8 autoneg, u32 speed, u32 settings);

int sssnic_set_link_settings(struct net_device *netdev,
			     u8 autoneg, u32 speed);

void sss_nic_get_io_stats(const struct sss_nic_dev *nic_dev, void *stats);

#endif
