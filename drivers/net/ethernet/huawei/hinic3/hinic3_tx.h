/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_TX_H
#define HINIC3_TX_H

#include <net/ipv6.h>
#include <net/checksum.h>
#include <net/ip6_checksum.h>
#include <linux/ip.h>
#include <linux/ipv6.h>

#include "hinic3_nic_qp.h"
#include "hinic3_nic_io.h"

#define VXLAN_OFFLOAD_PORT_LE 0xb512 /* big end is 4789 */

#define COMPACET_WQ_SKB_MAX_LEN 16383

#define IP4_VERSION 4
#define IP6_VERSION 6
#define IP_HDR_IHL_UNIT_SHIFT 2
#define TCP_HDR_DATA_OFF_UNIT_SHIFT 2

enum tx_offload_type {
	TX_OFFLOAD_TSO = BIT(0),
	TX_OFFLOAD_CSUM = BIT(1),
	TX_OFFLOAD_VLAN = BIT(2),
	TX_OFFLOAD_INVALID = BIT(3),
	TX_OFFLOAD_ESP = BIT(4),
};

struct hinic3_txq_stats {
	u64	packets;
	u64	bytes;
	u64	busy;
	u64	wake;
	u64	dropped;

	/* Subdivision statistics show in private tool */
	u64	skb_pad_err;
	u64	frag_len_overflow;
	u64	offload_cow_skb_err;
	u64	map_frag_err;
	u64	unknown_tunnel_pkt;
	u64	frag_size_err;
	u64	rsvd1;
	u64	rsvd2;

#ifdef HAVE_NDO_GET_STATS64
	struct u64_stats_sync	syncp;
#else
	struct u64_stats_sync_empty syncp;
#endif
};

struct hinic3_dma_info {
	dma_addr_t dma;
	u32 len;
};

#define IPV4_VERSION		4
#define IPV6_VERSION		6
#define TCP_HDR_DOFF_UNIT	2
#define TRANSPORT_OFFSET(l4_hdr, skb) ((u32)((l4_hdr) - (skb)->data))

union hinic3_ip {
	struct iphdr *v4;
	struct ipv6hdr *v6;
	unsigned char *hdr;
};

struct hinic3_tx_info {
	struct sk_buff		*skb;

	u16			wqebb_cnt;
	u16			valid_nr_frags;

	int			num_sge;
	u16			num_pkts;
	u16			rsvd1;
	u32			rsvd2;
	u64			num_bytes;
	struct hinic3_dma_info	*dma_info;
	u64			rsvd3;
};

struct hinic3_txq {
	struct net_device	*netdev;
	struct device		*dev;

	struct hinic3_txq_stats txq_stats;

	u8			cos;
	u8			rsvd1;
	u16			q_id;
	u32			q_mask;
	u32			q_depth;
	u32			rsvd2;

	struct hinic3_tx_info	*tx_info;
	struct hinic3_io_queue	*sq;

	u64			last_moder_packets;
	u64			last_moder_bytes;
	u64			rsvd3;
} ____cacheline_aligned;

netdev_tx_t hinic3_lb_xmit_frame(struct sk_buff *skb,
				 struct net_device *netdev);

struct hinic3_dyna_txq_res {
	struct hinic3_tx_info *tx_info;
	struct hinic3_dma_info *bds;
};

netdev_tx_t hinic3_xmit_frame(struct sk_buff *skb, struct net_device *netdev);

void hinic3_txq_get_stats(struct hinic3_txq *txq,
			  struct hinic3_txq_stats *stats);

void hinic3_txq_clean_stats(struct hinic3_txq_stats *txq_stats);

struct hinic3_nic_dev;
int hinic3_alloc_txqs_res(struct hinic3_nic_dev *nic_dev, u16 num_sq,
			  u32 sq_depth, struct hinic3_dyna_txq_res *txqs_res);

void hinic3_free_txqs_res(struct hinic3_nic_dev *nic_dev, u16 num_sq,
			  u32 sq_depth, struct hinic3_dyna_txq_res *txqs_res);

int hinic3_configure_txqs(struct hinic3_nic_dev *nic_dev, u16 num_sq,
			  u32 sq_depth, struct hinic3_dyna_txq_res *txqs_res);

int hinic3_alloc_txqs(struct net_device *netdev);

void hinic3_free_txqs(struct net_device *netdev);

int hinic3_tx_poll(struct hinic3_txq *txq, int budget);

int hinic3_flush_txqs(struct net_device *netdev);

void hinic3_set_txq_cos(struct hinic3_nic_dev *nic_dev, u16 start_qid,
			u16 q_num, u8 cos);

void hinic3_tx_set_wqebb_cnt(void *wqe_combo, u32 offload, u16 num_sge);

void hinic3_tx_set_compact_offload_wqebb_cnt(void *wqe_combo, u32 offload, u16 num_sge);

void hinic3_tx_set_wqe_task(void *wqe_combo, void *offload_info);

void hinic3_tx_set_compact_offload_wqe_task(void *wqe_combo, void *offload_info);

#ifdef static
#undef static
#define LLT_STATIC_DEF_SAVED
#endif

static inline __sum16 csum_magic(union hinic3_ip *ip, unsigned short proto)
{
	return (ip->v4->version == IPV4_VERSION) ?
		csum_tcpudp_magic(ip->v4->saddr, ip->v4->daddr, 0, proto, 0) :
		csum_ipv6_magic(&ip->v6->saddr, &ip->v6->daddr, 0, proto, 0);
}

#endif
