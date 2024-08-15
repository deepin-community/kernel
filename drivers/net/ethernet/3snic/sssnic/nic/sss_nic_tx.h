/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_TX_H
#define SSS_NIC_TX_H

#include <net/ipv6.h>
#include <net/checksum.h>
#include <net/ip6_checksum.h>
#include <linux/ip.h>
#include <linux/ipv6.h>

#include "sss_nic_io.h"
#include "sss_nic_dev_define.h"

void sss_nic_free_all_skb(struct sss_nic_dev *nic_dev, u32 sq_depth,
			  struct sss_nic_tx_desc *tx_desc_group);
netdev_tx_t sss_nic_loop_start_xmit(struct sk_buff *skb,
				    struct net_device *netdev);
netdev_tx_t sss_nic_ndo_start_xmit(struct sk_buff *skb,
				   struct net_device *netdev);
void sss_nic_get_sq_stats(struct sss_nic_sq_desc *sq_desc,
			  struct sss_nic_sq_stats *stats);
int sss_nic_tx_poll(struct sss_nic_sq_desc *sq_desc, int budget);

#endif
