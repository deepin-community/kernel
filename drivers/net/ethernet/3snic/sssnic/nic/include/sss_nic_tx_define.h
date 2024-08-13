/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_TX_DEFINE_H
#define SSS_NIC_TX_DEFINE_H

#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/device.h>

#include "sss_kernel.h"
#include "sss_nic_io_define.h"

struct sss_nic_dma_info {
	dma_addr_t dma;
	u32 len;
};

struct sss_nic_tx_desc {
	struct sk_buff		*skb;

	u16			wqebb_cnt;
	u16			nr_frags;

	int			sge_num;
	u16			nr_pkt_cnt;
	u16			rsvd1;
	u32			rsvd2;

	u64			bytes;
	struct sss_nic_dma_info	*dma_group;
	u64			rsvd3;
};

struct sss_nic_sq_stats {
	u64	tx_packets;
	u64	tx_bytes;
	u64	tx_busy;
	u64	wake;
	u64	tx_dropped;

	/* Subdivision statistics show in private tool */
	u64	skb_pad_err;
	u64	offload_err;
	u64	dma_map_err;
	u64	unknown_tunnel_proto;
	u64	frag_size_zero;
	u64	frag_len_overflow;
	u64	rsvd1;
	u64	rsvd2;

#ifdef HAVE_NDO_GET_STATS64
	struct u64_stats_sync	stats_sync;
#else
	struct u64_stats_sync_empty stats_sync;
#endif
};

struct sss_nic_sq_desc {
	struct net_device	*netdev;
	struct device		*dev;

	struct sss_nic_sq_stats stats;

	u8			cos;
	u8			rsvd1;
	u16			qid;
	u32			qid_mask;
	u32			q_depth;
	u32			rsvd2;

	struct sss_nic_tx_desc	*tx_desc_group;
	struct sss_nic_io_queue	*sq;

	u64			last_tx_pkts;
	u64			last_tx_bytes;
	u64			rsvd3;
} ____cacheline_aligned;

struct sss_nic_sq_resource {
	struct sss_nic_tx_desc *tx_desc_group;
	struct sss_nic_dma_info *dma_group;
};

#endif
