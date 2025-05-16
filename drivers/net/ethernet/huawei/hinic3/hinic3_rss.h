/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_RSS_H
#define HINIC3_RSS_H

#include "hinic3_nic_dev.h"

#define HINIC_NUM_IQ_PER_FUNC	8

int hinic3_rss_init(struct hinic3_nic_dev *nic_dev, u8 *rq2iq_map,
		    u32 map_size, u8 dcb_en);

void hinic3_rss_deinit(struct hinic3_nic_dev *nic_dev);

int hinic3_set_hw_rss_parameters(struct net_device *netdev, u8 rss_en,
				 u8 cos_num, u8 *cos_map, u8 dcb_en);

void hinic3_init_rss_parameters(struct net_device *netdev);

void hinic3_set_default_rss_indir(struct net_device *netdev);

void hinic3_try_to_enable_rss(struct hinic3_nic_dev *nic_dev);

void hinic3_clear_rss_config(struct hinic3_nic_dev *nic_dev);

void hinic3_flush_rx_flow_rule(struct hinic3_nic_dev *nic_dev);
int hinic3_ethtool_get_flow(const struct hinic3_nic_dev *nic_dev,
			    struct ethtool_rxnfc *info, u32 location);

int hinic3_ethtool_get_all_flows(const struct hinic3_nic_dev *nic_dev,
				 struct ethtool_rxnfc *info, u32 *rule_locs);

int hinic3_ethtool_flow_remove(struct hinic3_nic_dev *nic_dev, u32 location);

int hinic3_ethtool_flow_replace(struct hinic3_nic_dev *nic_dev,
				struct ethtool_rx_flow_spec *fs);

bool hinic3_validate_channel_setting_in_ntuple(const struct hinic3_nic_dev *nic_dev, u32 q_num);

/* for ethtool */
int hinic3_get_rxnfc(struct net_device *netdev,
		     struct ethtool_rxnfc *cmd, u32 *rule_locs);

int hinic3_set_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *cmd);

void hinic3_get_channels(struct net_device *netdev,
			 struct ethtool_channels *channels);

int hinic3_set_channels(struct net_device *netdev,
			struct ethtool_channels *channels);

#ifndef NOT_HAVE_GET_RXFH_INDIR_SIZE
u32 hinic3_get_rxfh_indir_size(struct net_device *netdev);
#endif /* NOT_HAVE_GET_RXFH_INDIR_SIZE */

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
u32 hinic3_get_rxfh_key_size(struct net_device *netdev);

#ifdef HAVE_RXFH_HASHFUNC
int hinic3_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key, u8 *hfunc);
#else /* HAVE_RXFH_HASHFUNC */
int hinic3_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key);
#endif /* HAVE_RXFH_HASHFUNC */

#ifdef HAVE_RXFH_HASHFUNC
int hinic3_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key,
		    const u8 hfunc);
#else
#ifdef HAVE_RXFH_NONCONST
int hinic3_set_rxfh(struct net_device *netdev, u32 *indir, u8 *key);
#else
int hinic3_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key);
#endif /* HAVE_RXFH_NONCONST */
#endif /* HAVE_RXFH_HASHFUNC */

#else /* !(defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)) */

#ifdef NOT_HAVE_GET_RXFH_INDIR_SIZE
int hinic3_get_rxfh_indir(struct net_device *netdev,
			  struct ethtool_rxfh_indir *indir1);
#else
int hinic3_get_rxfh_indir(struct net_device *netdev, u32 *indir);
#endif

#ifdef NOT_HAVE_GET_RXFH_INDIR_SIZE
int hinic3_set_rxfh_indir(struct net_device *netdev,
			  const struct ethtool_rxfh_indir *indir1);
#else
int hinic3_set_rxfh_indir(struct net_device *netdev, const u32 *indir);
#endif /* NOT_HAVE_GET_RXFH_INDIR_SIZE */

#endif /* (defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)) */

#endif
