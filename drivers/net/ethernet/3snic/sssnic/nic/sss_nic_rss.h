/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_RSS_H
#define SSS_NIC_RSS_H

#include "sss_nic_dev_define.h"

#define SSS_NIC_NUM_IQ_PER_FUNC	8

int sss_nic_update_rss_cfg(struct sss_nic_dev *nic_dev);

void sss_nic_reset_rss_cfg(struct sss_nic_dev *nic_dev);

void sss_nic_set_default_rss_indir(struct net_device *netdev);

void sss_nic_try_to_enable_rss(struct sss_nic_dev *nic_dev);

void sss_nic_free_rss_key(struct sss_nic_dev *nic_dev);

/* for ethtool */
int sss_nic_get_rxnfc(struct net_device *netdev,
		      struct ethtool_rxnfc *cmd, u32 *rule_locs);

int sss_nic_set_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *cmd);

void sss_nic_get_channels(struct net_device *netdev,
			  struct ethtool_channels *channels);

int sss_nic_set_channels(struct net_device *netdev,
			 struct ethtool_channels *channels);

#ifndef NOT_HAVE_GET_RXFH_INDIR_SIZE
u32 sss_nic_get_rxfh_indir_size(struct net_device *netdev);
#endif /* NOT_HAVE_GET_RXFH_INDIR_SIZE */

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
u32 sss_nic_get_rxfh_key_size(struct net_device *netdev);

#ifdef HAVE_RXFH_HASHFUNC
int sss_nic_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key, u8 *hfunc);
#else /* HAVE_RXFH_HASHFUNC */
int sss_nic_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key);
#endif /* HAVE_RXFH_HASHFUNC */

#ifdef HAVE_RXFH_HASHFUNC
int sss_nic_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key,
		     const u8 hfunc);
#else
#ifdef HAVE_RXFH_NONCONST
int sss_nic_set_rxfh(struct net_device *netdev, u32 *indir, u8 *key);
#else
int sss_nic_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key);
#endif /* HAVE_RXFH_NONCONST */
#endif /* HAVE_RXFH_HASHFUNC */

#else /* !(defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)) */

#ifdef NOT_HAVE_GET_RXFH_INDIR_SIZE
int sss_nic_get_rxfh_indir(struct net_device *netdev,
			   struct ethtool_rxfh_indir *indir1);
#else
int sss_nic_get_rxfh_indir(struct net_device *netdev, u32 *indir);
#endif

#ifdef NOT_HAVE_GET_RXFH_INDIR_SIZE
int sss_nic_set_rxfh_indir(struct net_device *netdev,
			   const struct ethtool_rxfh_indir *indir1);
#else
int sss_nic_set_rxfh_indir(struct net_device *netdev, const u32 *indir);
#endif /* NOT_HAVE_GET_RXFH_INDIR_SIZE */

#endif /* (defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)) */

#endif
