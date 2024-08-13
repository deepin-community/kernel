// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/u64_stats_sync.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/sctp.h>
#include <linux/pkt_sched.h>
#include <linux/ipv6.h>
#include <linux/module.h>
#include <linux/compiler.h>
#include <net/xdp.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_io.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_rss.h"
#include "sss_nic_rx.h"
#include "sss_nic_cfg.h"

/* rx cqe checksum err */
#define SSSNIC_RX_IP_CSUM_ERR	BIT(0)
#define SSSNIC_RX_TCP_CSUM_ERR	BIT(1)
#define SSSNIC_RX_UDP_CSUM_ERR	BIT(2)
#define SSSNIC_RX_IGMP_CSUM_ERR	BIT(3)
#define SSSNIC_RX_ICMPV4_CSUM_ERR	BIT(4)
#define SSSNIC_RX_ICMPV6_CSUM_ERR	BIT(5)
#define SSSNIC_RX_SCTP_CRC_ERR	BIT(6)
#define SSSNIC_RX_CSUM_HW_CHECK_NONE	BIT(7)
#define SSSNIC_RX_CSUM_IPSU_OTHER_ERR	BIT(8)

#define LRO_PKT_HDR_LEN_IPV4		66
#define LRO_PKT_HDR_LEN_IPV6		86
#define LRO_PKT_HDR_LEN(cqe)		\
	(SSSNIC_GET_RX_IP_TYPE(sss_hw_cpu32((cqe)->offload_type)) == \
	 SSSNIC_RX_IPV6_PKT ? LRO_PKT_HDR_LEN_IPV6 : LRO_PKT_HDR_LEN_IPV4)

#define SSSNIC_MAX_NUM_RQ 256

#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_PKT_TYPE_SHIFT 0
#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_IP_TYPE_SHIFT 5
#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_ENC_L3_TYPE_SHIFT 7
#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_TUNNEL_PKT_FORMAT_SHIFT 8
#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_PKT_UMBCAST_SHIFT 19
#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_VLAN_EN_SHIFT 21
#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_RSS_TYPE_SHIFT 24

#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_PKT_TYPE_MASK 0x1FU
#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_IP_TYPE_MASK 0x3U
#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_ENC_L3_TYPE_MASK 0x1U
#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_TUNNEL_PKT_FORMAT_MASK 0xFU
#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_PKT_UMBCAST_MASK 0x3U
#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_VLAN_EN_MASK 0x1U
#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_RSS_TYPE_MASK 0xFFU

#define SSSNIC_RQ_CQE_OFFOLAD_TYPE_GET(val, member) \
	(((val) >> SSSNIC_RQ_CQE_OFFOLAD_TYPE_##member##_SHIFT) & \
	 SSSNIC_RQ_CQE_OFFOLAD_TYPE_##member##_MASK)

#define SSSNIC_GET_RX_PKT_TYPE(offload_type) \
	SSSNIC_RQ_CQE_OFFOLAD_TYPE_GET(offload_type, PKT_TYPE)
#define SSSNIC_GET_RX_IP_TYPE(offload_type) \
	SSSNIC_RQ_CQE_OFFOLAD_TYPE_GET(offload_type, IP_TYPE)
#define SSSNIC_GET_RX_ENC_L3_TYPE(offload_type) \
	SSSNIC_RQ_CQE_OFFOLAD_TYPE_GET(offload_type, ENC_L3_TYPE)
#define SSSNIC_GET_RX_TUNNEL_PKT_FORMAT(offload_type) \
	SSSNIC_RQ_CQE_OFFOLAD_TYPE_GET(offload_type, TUNNEL_PKT_FORMAT)

#define SSSNIC_GET_RX_PKT_UMBCAST(offload_type) \
	SSSNIC_RQ_CQE_OFFOLAD_TYPE_GET(offload_type, PKT_UMBCAST)

#define SSSNIC_GET_RX_VLAN_OFFLOAD_EN(offload_type) \
	SSSNIC_RQ_CQE_OFFOLAD_TYPE_GET(offload_type, VLAN_EN)

#define SSSNIC_GET_RSS_TYPES(offload_type) \
	SSSNIC_RQ_CQE_OFFOLAD_TYPE_GET(offload_type, RSS_TYPE)

#define SSSNIC_RQ_CQE_SGE_VLAN_SHIFT 0
#define SSSNIC_RQ_CQE_SGE_LEN_SHIFT 16

#define SSSNIC_RQ_CQE_SGE_VLAN_MASK 0xFFFFU
#define SSSNIC_RQ_CQE_SGE_LEN_MASK 0xFFFFU

#define SSSNIC_RQ_CQE_SGE_GET(val, member) \
	(((val) >> SSSNIC_RQ_CQE_SGE_##member##_SHIFT) & SSSNIC_RQ_CQE_SGE_##member##_MASK)

#define SSSNIC_GET_RX_VLAN_TAG(vlan_len) SSSNIC_RQ_CQE_SGE_GET(vlan_len, VLAN)

#define SSSNIC_GET_RX_PKT_LEN(vlan_len) SSSNIC_RQ_CQE_SGE_GET(vlan_len, LEN)

#define SSSNIC_GET_RX_CSUM_ERR(status) SSSNIC_RQ_CQE_STATUS_GET(status, CSUM_ERR)

#define SSSNIC_GET_RX_FLUSH(status) SSSNIC_RQ_CQE_STATUS_GET(status, FLUSH)

#define SSSNIC_GET_RX_BP_EN(status) SSSNIC_RQ_CQE_STATUS_GET(status, BP_EN)

#define SSSNIC_GET_RX_NUM_LRO(status) SSSNIC_RQ_CQE_STATUS_GET(status, NUM_LRO)

#define SSSNIC_RX_IS_DECRY_PKT(status) SSSNIC_RQ_CQE_STATUS_GET(status, DECRY_PKT)

#define SSSNIC_RQ_CQE_SUPER_CQE_EN_SHIFT 0
#define SSSNIC_RQ_CQE_PKT_NUM_SHIFT 1
#define SSSNIC_RQ_CQE_PKT_LAST_LEN_SHIFT 6
#define SSSNIC_RQ_CQE_PKT_FIRST_LEN_SHIFT 19

#define SSSNIC_RQ_CQE_SUPER_CQE_EN_MASK 0x1
#define SSSNIC_RQ_CQE_PKT_NUM_MASK 0x1FU
#define SSSNIC_RQ_CQE_PKT_FIRST_LEN_MASK 0x1FFFU
#define SSSNIC_RQ_CQE_PKT_LAST_LEN_MASK 0x1FFFU

#define SSSNIC_RQ_CQE_PKT_NUM_GET(val, member) \
	(((val) >> SSSNIC_RQ_CQE_PKT_##member##_SHIFT) & SSSNIC_RQ_CQE_PKT_##member##_MASK)
#define SSSNIC_GET_RQ_CQE_PKT_NUM(pkt_info) SSSNIC_RQ_CQE_PKT_NUM_GET(pkt_info, NUM)

#define SSSNIC_RQ_CQE_SUPER_CQE_EN_GET(val, member) \
	(((val) >> SSSNIC_RQ_CQE_##member##_SHIFT) & SSSNIC_RQ_CQE_##member##_MASK)
#define SSSNIC_GET_SUPER_CQE_EN(pkt_info) \
	SSSNIC_RQ_CQE_SUPER_CQE_EN_GET(pkt_info, SUPER_CQE_EN)

/* performance: ci addr RTE_CACHE_SIZE(64B) alignment */
#define SSSNIC_RX_HDR_SIZE			256
#define SSSNIC_RX_BUFFER_WRITE			16

#define SSSNIC_RX_TCP_PKT			0x3
#define SSSNIC_RX_UDP_PKT			0x4
#define SSSNIC_RX_SCTP_PKT			0x7

#define SSSNIC_RX_IPV4_PKT			0
#define SSSNIC_RX_IPV6_PKT			1
#define SSSNIC_RX_INVALID_IP_TYPE		2

#define SSSNIC_RX_PKT_FORMAT_NON_TUNNEL		0
#define SSSNIC_RX_PKT_FORMAT_VXLAN		1

#ifdef HAVE_XDP_SUPPORT
enum sss_nic_xdp_pkt {
	SSSNIC_XDP_PKT_PASS,
	SSSNIC_XDP_PKT_DROP,
};
#endif

#define SSSNIC_LRO_PKT_HDR_LEN_IPV4		66
#define SSSNIC_LRO_PKT_HDR_LEN_IPV6		86
#define SSSNIC_LRO_PKT_HDR_LEN(cqe)		\
	(SSSNIC_GET_RX_IP_TYPE(sss_hw_cpu32((cqe)->offload_type)) == \
	 SSSNIC_RX_IPV6_PKT ? SSSNIC_LRO_PKT_HDR_LEN_IPV6 : SSSNIC_LRO_PKT_HDR_LEN_IPV4)

#define SSSNIC_GET_SGE_NUM(pkt_len, rxq)	\
		((u8)(((pkt_len) >> (rxq)->buff_size_shift) + \
		      (((pkt_len) & ((rxq)->buf_len - 1)) ? 1 : 0)))

bool sss_nic_rx_alloc_dma_page(struct sss_nic_dev *nic_dev,
			       struct sss_nic_rx_desc *rx_desc)
{
	struct page *page = rx_desc->page;
	dma_addr_t dma_addr = rx_desc->buf_daddr;

	if (likely(dma_addr != 0))
		return true;

	page = alloc_pages_node(NUMA_NO_NODE,
				GFP_ATOMIC | __GFP_COLD | __GFP_COMP, nic_dev->page_order);
	if (unlikely(!page))
		return false;

	dma_addr = dma_map_page(nic_dev->dev_hdl, page, 0,
				nic_dev->rx_dma_buff_size, DMA_FROM_DEVICE);
	if (unlikely(dma_mapping_error(nic_dev->dev_hdl, dma_addr) != 0)) {
		__free_pages(page, nic_dev->page_order);
		return false;
	}

	rx_desc->page = page;
	rx_desc->buf_daddr = dma_addr;
	rx_desc->page_offset = 0;

	return true;
}

u32 sss_nic_fill_bd_sge(struct sss_nic_rq_desc *rq_desc)
{
	struct net_device *netdev = rq_desc->netdev;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	struct sss_nic_rx_desc *rx_desc = NULL;
	struct sss_nic_rqe *rqe = NULL;
	u32 idle_wqe = rq_desc->delta - 1;
	dma_addr_t dma_addr;
	u32 i;

	for (i = 0; i < idle_wqe; i++) {
		rx_desc = &rq_desc->rx_desc_group[rq_desc->pi];
		rqe = rx_desc->rqe;

		if (unlikely(!sss_nic_rx_alloc_dma_page(nic_dev, rx_desc))) {
			SSSNIC_RQ_STATS_INC(rq_desc, alloc_rx_dma_err);
			break;
		}

		dma_addr = rx_desc->buf_daddr + rx_desc->page_offset;

		if (rq_desc->rq->wqe_type == SSSNIC_EXTEND_RQ_WQE) {
			rqe->extend_rqe.bd_sect.sge.low_addr =
				sss_hw_be32(lower_32_bits(dma_addr));
			rqe->extend_rqe.bd_sect.sge.high_addr =
				sss_hw_be32(upper_32_bits(dma_addr));
		} else {
			rqe->normal_rqe.bd_lo_addr = sss_hw_be32(lower_32_bits(dma_addr));
			rqe->normal_rqe.bd_hi_addr = sss_hw_be32(upper_32_bits(dma_addr));
		}
		rq_desc->pi = (u16)((rq_desc->pi + 1) & rq_desc->qid_mask);
	}

	if (likely(i != 0)) {
		sss_nic_write_db(rq_desc->rq, rq_desc->qid & (SSSNIC_DCB_COS_MAX - 1),
				 RQ_CFLAG_DP, (u16)((u32)rq_desc->pi << rq_desc->rq->wqe_type));

		rq_desc->delta -= i;
		rq_desc->backup_pi = rq_desc->pi;
	} else if (idle_wqe == rq_desc->q_depth - 1) {
		SSSNIC_RQ_STATS_INC(rq_desc, rx_buf_errors);
	}

	return i;
}

#define SSS_NIC_FILL_BD_SGE(rq_desc)	\
do { \
	struct sss_nic_dev *nic_dev = netdev_priv((rq_desc)->netdev); \
	struct sss_nic_rx_desc *_rx_desc = NULL; \
	struct sss_nic_rqe *_rqe = NULL; \
	u32 _idle_wqe = (rq_desc)->delta - 1; \
	dma_addr_t _dma_addr; \
	u32 _id; \
\
	for (_id = 0; _id < _idle_wqe; _id++) { \
		_rx_desc = &(rq_desc)->rx_desc_group[(rq_desc)->pi]; \
		_rqe = _rx_desc->rqe; \
\
		if (unlikely(!sss_nic_rx_alloc_dma_page(nic_dev, _rx_desc))) { \
			SSSNIC_RQ_STATS_INC((rq_desc), alloc_rx_dma_err); \
			break; \
		} \
\
		_dma_addr = _rx_desc->buf_daddr + _rx_desc->page_offset; \
\
		if ((rq_desc)->rq->wqe_type == SSSNIC_EXTEND_RQ_WQE) { \
			_rqe->extend_rqe.bd_sect.sge.low_addr = \
				sss_hw_be32(lower_32_bits(_dma_addr)); \
			_rqe->extend_rqe.bd_sect.sge.high_addr = \
				sss_hw_be32(upper_32_bits(_dma_addr)); \
		} else { \
			_rqe->normal_rqe.bd_lo_addr = sss_hw_be32(lower_32_bits(_dma_addr)); \
			_rqe->normal_rqe.bd_hi_addr = sss_hw_be32(upper_32_bits(_dma_addr)); \
		} \
		(rq_desc)->pi = (u16)(((rq_desc)->pi + 1) & (rq_desc)->qid_mask); \
	} \
\
	if (likely(_id != 0)) { \
		sss_nic_write_db((rq_desc)->rq, (rq_desc)->qid & (SSSNIC_DCB_COS_MAX - 1), \
				 RQ_CFLAG_DP, \
				 (u16)((u32)(rq_desc)->pi << (rq_desc)->rq->wqe_type)); \
\
		(rq_desc)->delta -= _id; \
		(rq_desc)->backup_pi = (rq_desc)->pi; \
	} else if (_idle_wqe == (rq_desc)->q_depth - 1) { \
		SSSNIC_RQ_STATS_INC((rq_desc), rx_buf_errors); \
	} \
} while (0)

#define sss_nic_rx_reuse_dma_page(rq_desc, old_rqe_desc)	\
do { \
	u16 _pi = (rq_desc)->backup_pi; \
	struct sss_nic_rx_desc *new_rqe_desc; \
\
	new_rqe_desc = &(rq_desc)->rx_desc_group[_pi++]; \
\
	(rq_desc)->backup_pi = (_pi < (rq_desc)->q_depth) ? _pi : 0; \
\
	new_rqe_desc->page = (old_rqe_desc)->page; \
	new_rqe_desc->page_offset = (old_rqe_desc)->page_offset; \
	new_rqe_desc->buf_daddr = (old_rqe_desc)->buf_daddr; \
\
	dma_sync_single_range_for_device((rq_desc)->dev, new_rqe_desc->buf_daddr, \
					 new_rqe_desc->page_offset, (rq_desc)->buf_len, \
					 DMA_FROM_DEVICE); \
} while (0)

#if L1_CACHE_BYTES < 128
#define PREFETCH_L1_CACHE(vaddr)	prefetch((vaddr) + L1_CACHE_BYTES)
#else
#define PREFETCH_L1_CACHE(vaddr)	do {} while (0)
#endif

#define sss_nic_skb_add_rx_frag(rq_desc, rx_desc, skb, size, ret_flag)	\
do { \
	u8 *vaddr; \
	struct page *page; \
\
	page = (rx_desc)->page; \
	vaddr = (u8 *)page_address(page) + (rx_desc)->page_offset; \
	prefetch(vaddr); \
	PREFETCH_L1_CACHE(vaddr); \
\
	dma_sync_single_range_for_cpu((rq_desc)->dev, (rx_desc)->buf_daddr, \
				      (rx_desc)->page_offset, (rq_desc)->buf_len,  \
				      DMA_FROM_DEVICE); \
\
	if ((size) <= SSSNIC_RX_HDR_SIZE && !skb_is_nonlinear(skb)) { \
		memcpy(__skb_put((skb), (size)), vaddr, ALIGN((size), sizeof(long))); \
		if (likely(page_to_nid(page) == numa_node_id())) \
			*(ret_flag) = true; \
		else { \
			put_page(page); \
			*(ret_flag) = false; \
		} \
	} else { \
		skb_add_rx_frag((skb), skb_shinfo(skb)->nr_frags, page, \
				(int)(rx_desc)->page_offset, (int)(size), (rq_desc)->buf_len); \
		if (unlikely(page_count(page) != 1)) \
			*(ret_flag) = false; \
		else if (unlikely(page_to_nid(page) != numa_node_id())) \
			*(ret_flag) = false; \
		else { \
			(rx_desc)->page_offset ^= (rq_desc)->buf_len; \
			get_page(page); \
			*(ret_flag) = true; \
		} \
	} \
} while (0)

#define sss_nic_combine_skb(rq_desc, head_skb, sge_num, pkt_size)	\
do { \
	struct sss_nic_rx_desc *_rx_desc = NULL; \
	struct sk_buff *_skb = NULL; \
	u8 _frag_num = 0; \
	u32 tmp_pkt_sz = (pkt_size); \
	u8 tmp_sge_num = (sge_num); \
	u32 _size; \
	u32 _ci; \
	u8 _ret; \
\
	_skb = (head_skb); \
	_ci = (rq_desc)->ci & (rq_desc)->qid_mask; \
	while (tmp_sge_num > 0) { \
		_rx_desc = &(rq_desc)->rx_desc_group[_ci]; \
		if (unlikely(tmp_pkt_sz > (rq_desc)->buf_len)) { \
			_size = (rq_desc)->buf_len; \
			tmp_pkt_sz -= (rq_desc)->buf_len; \
		} else { \
			_size = tmp_pkt_sz; \
		} \
\
		if (unlikely(_frag_num == MAX_SKB_FRAGS)) { \
			if (_skb == (head_skb)) \
				_skb = skb_shinfo(_skb)->frag_list; \
			else \
				_skb = _skb->next; \
\
			_frag_num = 0; \
		} \
\
		if (unlikely(_skb != (head_skb))) { \
			(head_skb)->truesize += (rq_desc)->buf_len; \
			(head_skb)->len += _size; \
			(head_skb)->data_len += _size; \
		} \
\
		sss_nic_skb_add_rx_frag((rq_desc), _rx_desc, _skb, _size, &_ret); \
		if (likely(_ret)) \
			sss_nic_rx_reuse_dma_page((rq_desc), _rx_desc); \
		else \
			dma_unmap_page((rq_desc)->dev, _rx_desc->buf_daddr, \
				       (rq_desc)->dma_buff_size, DMA_FROM_DEVICE); \
\
		_rx_desc->buf_daddr = 0; \
		_rx_desc->page = NULL; \
		tmp_sge_num--; \
		_frag_num++; \
		_ci = (_ci + 1) & (rq_desc)->qid_mask; \
	} \
} while (0)

#define sss_nic_fetch_one_skb(rq_desc, pkt_size, ret_skb)	\
do { \
	struct net_device *_netdev = (rq_desc)->netdev; \
	struct sk_buff *head_skb = NULL; \
	struct sk_buff *next_skb = NULL; \
	struct sk_buff *_skb = NULL; \
	u8 sge_num; \
	u8 skb_num; \
\
	head_skb = netdev_alloc_skb_ip_align((rq_desc)->netdev, SSSNIC_RX_HDR_SIZE); \
	if (likely(head_skb)) { \
		sge_num = SSSNIC_GET_SGE_NUM((pkt_size), (rq_desc)); \
		if (likely(sge_num <= MAX_SKB_FRAGS)) \
			skb_num = 1; \
		else \
			skb_num = (sge_num / MAX_SKB_FRAGS) + \
				  ((sge_num % MAX_SKB_FRAGS) ? 1 : 0); \
\
		while (unlikely(skb_num > 1)) { \
			next_skb = netdev_alloc_skb_ip_align(_netdev, SSSNIC_RX_HDR_SIZE); \
			if (unlikely(!next_skb)) { \
				dev_kfree_skb_any(head_skb); \
				break; \
			} \
\
			if (!_skb) { \
				skb_shinfo(head_skb)->frag_list = next_skb; \
				_skb = next_skb; \
			} else { \
				_skb->next = next_skb; \
				_skb = next_skb; \
			} \
\
			skb_num--; \
		} \
\
		if (likely(skb_num <= 1)) { \
			prefetchw(head_skb->data); \
			sss_nic_combine_skb((rq_desc), head_skb, sge_num, (pkt_size)); \
\
			(rq_desc)->delta += sge_num; \
			(rq_desc)->ci += sge_num; \
\
			(ret_skb) = head_skb; \
		} else { \
			(ret_skb) = NULL; \
		} \
	} else { \
		(ret_skb) = NULL; \
	} \
} while (0)

void sss_nic_get_rq_stats(struct sss_nic_rq_desc *rq_desc,
			  struct sss_nic_rq_stats *stats)
{
	struct sss_nic_rq_stats *rq_stats = &rq_desc->stats;
	unsigned int start;

	u64_stats_update_begin(&stats->stats_sync);
	do {
		start = u64_stats_fetch_begin(&rq_stats->stats_sync);
		stats->rx_bytes = rq_stats->rx_bytes;
		stats->rx_packets = rq_stats->rx_packets;
		stats->csum_errors = rq_stats->csum_errors;
		stats->other_errors = rq_stats->other_errors;
		stats->errors = rq_stats->csum_errors + rq_stats->other_errors;
		stats->rx_dropped = rq_stats->rx_dropped;
		stats->xdp_dropped = rq_stats->xdp_dropped;
		stats->rx_buf_errors = rq_stats->rx_buf_errors;
	} while (u64_stats_fetch_retry(&rq_stats->stats_sync, start));
	u64_stats_update_end(&stats->stats_sync);
}

static unsigned int sss_nic_eth_get_headlen(struct sk_buff *skb,
					    unsigned char *data,
					    unsigned int max_hlen)
{
#ifdef HAVE_ETH_GET_HEADLEN_FUNC
#ifdef ETH_GET_HEADLEN_NEED_DEV
	return eth_get_headlen(skb->dev, data, SSSNIC_RX_HDR_SIZE);
#else
	return eth_get_headlen(data, SSSNIC_RX_HDR_SIZE);
#endif
#else
#define IP_FRAG_OFFSET	0x1FFF
#define FCOE_HLEN	38
#define TCP_HEAD_OFFSET	12
	u8 nexthdr = 0;
	u16 proto;
	u8 hlen;
	union {
		struct ethhdr *eth;
		struct vlan_ethhdr *vlan;
		struct iphdr *ipv4;
		struct ipv6hdr *ipv6;
		unsigned char *data;
	} header;

	if (unlikely(max_hlen < ETH_HLEN))
		return max_hlen;

	header.data = data;
	proto = header.eth->h_proto;

	if (proto == htons(ETH_P_8021AD) || proto == htons(ETH_P_8021Q)) {
		if (unlikely(max_hlen < ETH_HLEN + VLAN_HLEN))
			return max_hlen;

		proto = header.vlan->h_vlan_encapsulated_proto;
		header.data += sizeof(struct vlan_ethhdr);
	} else {
		header.data += ETH_HLEN;
	}

	if (proto == htons(ETH_P_IP)) {
		if ((int)(header.data - data) > (int)(max_hlen - sizeof(struct iphdr)))
			return max_hlen;

		hlen = (header.data[0] & 0x0F) << 2;
		if (hlen < sizeof(struct iphdr))
			return (unsigned int)(header.data - data);

		if ((header.ipv4->frag_off & htons(IP_FRAG_OFFSET)) == 0)
			nexthdr = header.ipv4->proto;

		header.data += hlen;
	} else if (proto == htons(ETH_P_IPV6)) {
		if ((int)(header.data - data) > (int)(max_hlen - sizeof(struct ipv6hdr)))
			return max_hlen;

		nexthdr = header.ipv6->nexthdr;
		header.data += sizeof(struct ipv6hdr);
	} else if (proto == htons(ETH_P_FCOE)) {
		header.data += FCOE_HLEN;
	} else {
		return (unsigned int)(header.data - data);
	}

	if (nexthdr == IPPROTO_TCP) {
		if ((int)(header.data - data) > (int)(max_hlen - sizeof(struct tcphdr)))
			return max_hlen;

		if (SSSNIC_HEADER_LEN_TO_BYTE(header.data[TCP_HEAD_OFFSET] & 0xF0) >
					      sizeof(struct tcphdr))
			header.data += SSSNIC_HEADER_LEN_TO_BYTE(header.data[TCP_HEAD_OFFSET] &
								 0xF0);
		else
			header.data += sizeof(struct tcphdr);
	} else if (nexthdr == IPPROTO_UDP || nexthdr == IPPROTO_UDPLITE) {
		header.data += sizeof(struct udphdr);
	} else if (nexthdr == IPPROTO_SCTP) {
		header.data += sizeof(struct sctphdr);
	}

	if ((header.data - data) > max_hlen)
		return max_hlen;
	else
		return (unsigned int)(header.data - data);
#endif
}

#define sss_nic_pull_tail(skb)	\
do { \
	skb_frag_t *_frag = &skb_shinfo(skb)->frags[0]; \
	unsigned int _len; \
	unsigned char *_data = NULL; \
\
	_data = skb_frag_address(_frag); \
\
	_len = sss_nic_eth_get_headlen((skb), _data, SSSNIC_RX_HDR_SIZE); \
\
	skb_copy_to_linear_data((skb), _data, ALIGN(_len, sizeof(long))); \
\
	skb_frag_size_sub(_frag, (int)_len); \
	skb_frag_off_add(_frag, (int)_len); \
\
	(skb)->tail += _len; \
	(skb)->data_len -= _len; \
} while (0)

#define sss_nic_check_rx_csum(rq_desc, offload_type, status, skb)	\
do { \
	struct net_device *_netdev = (rq_desc)->netdev; \
	u32 pkt_fmt = SSSNIC_GET_RX_TUNNEL_PKT_FORMAT(offload_type); \
	u32 pkt_type = SSSNIC_GET_RX_PKT_TYPE(offload_type); \
	u32 ip_type = SSSNIC_GET_RX_IP_TYPE(offload_type); \
	u32 chksum_err; \
\
	chksum_err = SSSNIC_GET_RX_CSUM_ERR(status); \
	if (unlikely(chksum_err == SSSNIC_RX_CSUM_IPSU_OTHER_ERR)) \
		(rq_desc)->stats.other_errors++; \
\
	if ((_netdev->features & NETIF_F_RXCSUM)) { \
		if (unlikely(chksum_err != 0)) { \
			if ((chksum_err & \
			    (SSSNIC_RX_CSUM_HW_CHECK_NONE | \
			    SSSNIC_RX_CSUM_IPSU_OTHER_ERR)) == 0) \
				(rq_desc)->stats.csum_errors++; \
			(skb)->ip_summed = CHECKSUM_NONE; \
		} else if (ip_type == SSSNIC_RX_INVALID_IP_TYPE || \
		    !(pkt_fmt == SSSNIC_RX_PKT_FORMAT_NON_TUNNEL || \
		    pkt_fmt == SSSNIC_RX_PKT_FORMAT_VXLAN)) { \
			(skb)->ip_summed = CHECKSUM_NONE; \
		} else if (pkt_type == SSSNIC_RX_TCP_PKT ||  \
			   pkt_type == SSSNIC_RX_UDP_PKT || \
			   pkt_type == SSSNIC_RX_SCTP_PKT) \
			(skb)->ip_summed = CHECKSUM_UNNECESSARY; \
		else \
			(skb)->ip_summed = CHECKSUM_NONE; \
	} \
} while (0)

#ifdef HAVE_SKBUFF_CSUM_LEVEL
#define sss_nic_check_rx_gso(rq_desc, offload_type, skb)	\
do { \
	struct net_device *_netdev = (rq_desc)->netdev; \
\
	if (_netdev->features & NETIF_F_GRO) { \
		if (SSSNIC_GET_RX_TUNNEL_PKT_FORMAT(offload_type) == \
		    SSSNIC_RX_PKT_FORMAT_VXLAN && \
		    (skb)->ip_summed == CHECKSUM_UNNECESSARY) \
			(skb)->csum_level = 1; \
	} \
} while (0)
#else
#define sss_nic_check_rx_gso(rq_desc, offload_type, skb)	do {} while (0)
#endif /* HAVE_SKBUFF_CSUM_LEVEL */

static void sss_nic_loop_copy_data(struct sss_nic_dev *nic_dev,
				   struct sk_buff *skb)
{
	struct net_device *netdev = nic_dev->netdev;
	u8 *loop_test_rx_buf = nic_dev->loop_test_rx_buf;
	int loop_pkt_len = nic_dev->loop_pkt_len;
	void *frag_data = NULL;
	int frag_size;
	int pkt_off;
	int i;

	if (nic_dev->loop_test_rx_cnt == SSSNIC_LP_PKT_CNT) {
		nic_dev->loop_test_rx_cnt = 0;
		nicif_warn(nic_dev, rx_err, netdev, "Loopback test received too many pkts\n");
	}

	if (skb->len != loop_pkt_len) {
		nicif_warn(nic_dev, rx_err, netdev, "Invalid packet length\n");
		nic_dev->loop_test_rx_cnt++;
		return;
	}

	pkt_off = nic_dev->loop_test_rx_cnt * loop_pkt_len;
	frag_size = (int)skb_headlen(skb);
	memcpy(loop_test_rx_buf + pkt_off, skb->data, (size_t)(u32)frag_size);

	pkt_off += frag_size;
	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		frag_data = skb_frag_address(&skb_shinfo(skb)->frags[i]);
		frag_size = (int)skb_frag_size(&skb_shinfo(skb)->frags[i]);
		memcpy(loop_test_rx_buf + pkt_off, frag_data, (size_t)(u32)frag_size);

		pkt_off += frag_size;
	}
	nic_dev->loop_test_rx_cnt++;
}

#define sss_nic_update_gso_params(skb, gso)	\
do { \
	struct ethhdr *_ether = (struct ethhdr *)((skb)->data); \
	__be16 _protocol; \
\
	_protocol = __vlan_get_protocol((skb), _ether->h_proto, NULL); \
\
	skb_shinfo(skb)->gso_segs = gso; \
	skb_shinfo(skb)->gso_size = (u16)DIV_ROUND_UP(((skb)->len - skb_headlen(skb)), \
						      gso); \
	skb_shinfo(skb)->gso_type = (_protocol == htons(ETH_P_IP)) ? \
				    SKB_GSO_TCPV4 : SKB_GSO_TCPV6; \
} while (0)

#ifdef HAVE_XDP_SUPPORT
#define sss_nic_xdp_update_rx_info(rq_desc, wqe_num)	\
do { \
	struct sss_nic_rx_desc *_rx_desc = NULL; \
	u16 _wqe_cnt = wqe_num; \
\
	while (_wqe_cnt > 0) { \
		_rx_desc = &(rq_desc)->rx_desc_group[(rq_desc)->ci & (rq_desc)->qid_mask]; \
		if (likely(page_to_nid(_rx_desc->page) == numa_node_id())) \
			sss_nic_rx_reuse_dma_page((rq_desc), _rx_desc); \
\
		(rq_desc)->ci++; \
		(rq_desc)->delta++; \
		_rx_desc->buf_daddr = 0; \
		_rx_desc->page = NULL; \
\
		_wqe_cnt--; \
	} \
} while (0)

#ifdef HAVE_XDP_FRAME_SZ
#define SSSNIC_SET_XDP_FRAME_SZ(xdp, len)	((xdp)->frame_sz = (len))
#else
#define SSSNIC_SET_XDP_FRAME_SZ(xdp, len)	do {} while (0)
#endif

#ifdef HAVE_XDP_DATA_META
#define SSSNIC_XDP_SET_DATA_META_INVALID(xdp)	xdp_set_data_meta_invalid(xdp)
#else
#define SSSNIC_XDP_SET_DATA_META_INVALID(xdp)	do {} while (0)
#endif

#ifdef HAVE_BFP_WARN_NETDEV_PARAM
#define SSSNIC_BDF_WARN_INVALID_XDP_ACTION(netdev, xdp_prog, ret)	\
		bpf_warn_invalid_xdp_action(netdev, xdp_prog, ret)
#else
#define SSSNIC_BDF_WARN_INVALID_XDP_ACTION(netdev, xdp_prog, ret)	\
		bpf_warn_invalid_xdp_action(ret)
#endif

#define sss_nic_bpf_prog_run_xdp(rq_desc, pkt_size, result)	\
do { \
	struct bpf_prog *xdp_prog = NULL; \
	struct sss_nic_rx_desc *rx_desc = NULL; \
	struct xdp_buff xdp; \
	u16 _wqe_num = 1; \
	u8 *_data = NULL; \
	u32 _ret; \
\
	rcu_read_lock(); \
\
	xdp_prog = READ_ONCE((rq_desc)->xdp_prog); \
	if (!xdp_prog) { \
		*(result) = SSSNIC_XDP_PKT_PASS; \
	} else if (unlikely((pkt_size) > (rq_desc)->buf_len)) { \
		SSSNIC_RQ_STATS_INC((rq_desc), large_xdp_pkts); \
		_wqe_num = (u16)((pkt_size) >> (rq_desc)->buff_size_shift) + \
			  (((pkt_size) & ((rq_desc)->buf_len - 1)) ? 1 : 0); \
		SSSNIC_RQ_STATS_INC((rq_desc), xdp_dropped); \
		sss_nic_xdp_update_rx_info((rq_desc), _wqe_num); \
		*(result) = SSSNIC_XDP_PKT_DROP; \
	} else { \
		rx_desc = &(rq_desc)->rx_desc_group[(rq_desc)->ci & (rq_desc)->qid_mask]; \
		_data = (u8 *)page_address(rx_desc->page) + rx_desc->page_offset; \
		prefetch(_data); \
		dma_sync_single_range_for_cpu((rq_desc)->dev, rx_desc->buf_daddr, \
					      rx_desc->page_offset, (rq_desc)->buf_len, \
					      DMA_FROM_DEVICE); \
		xdp.data = _data; \
		xdp.data_hard_start = xdp.data; \
		xdp.data_end = xdp.data + (pkt_size); \
		SSSNIC_SET_XDP_FRAME_SZ(&xdp, (rq_desc)->buf_len); \
		SSSNIC_XDP_SET_DATA_META_INVALID(&xdp); \
		prefetchw(xdp.data_hard_start); \
\
		_ret = bpf_prog_run_xdp(xdp_prog, &xdp); \
		if (_ret == XDP_PASS) { \
			*(result) = SSSNIC_XDP_PKT_PASS; \
		} else { \
			*(result) = SSSNIC_XDP_PKT_DROP; \
			if (_ret != XDP_DROP) { \
				SSSNIC_BDF_WARN_INVALID_XDP_ACTION((rq_desc)->netdev, \
					xdp_prog, _ret); \
			} \
			SSSNIC_RQ_STATS_INC((rq_desc), xdp_dropped); \
			sss_nic_xdp_update_rx_info((rq_desc), _wqe_num); \
		} \
	} \
\
	rcu_read_unlock(); \
} while (0)
#endif

#if defined(NETIF_F_HW_VLAN_CTAG_RX)
#define sss_nic_vlan_put_tag(skb, netdev, offload_type, vlan_len)	\
do { \
	u16 vlan_id; \
	if (((netdev)->features & NETIF_F_HW_VLAN_CTAG_RX) != 0 && \
	    SSSNIC_GET_RX_VLAN_OFFLOAD_EN(offload_type) != 0) { \
		vlan_id = SSSNIC_GET_RX_VLAN_TAG(vlan_len); \
\
		/* if the packet is a vlan pkt, the vid may be 0 */ \
		__vlan_hwaccel_put_tag((skb), htons(ETH_P_8021Q), vlan_id); \
	} \
} while (0)
#else
#define sss_nic_vlan_put_tag(skb, netdev, offload_type, vlan_len)	\
do { \
	u16 vlan_id; \
	if (((netdev)->features & NETIF_F_HW_VLAN_RX) != 0 && \
	    SSSNIC_GET_RX_VLAN_OFFLOAD_EN(offload_type) != 0) { \
		vlan_id = SSSNIC_GET_RX_VLAN_TAG(vlan_len); \
\
		/* if the packet is a vlan pkt, the vid may be 0 */ \
		__vlan_hwaccel_put_tag((skb), htons(ETH_P_8021Q), vlan_id); \
	} \
} while (0)
#endif

static int sss_nic_recv_one_packet(struct sss_nic_rq_desc *rq_desc,
				   struct sss_nic_cqe *rx_cqe, u32 pkt_len,
				   u32 vlan_len, u32 status)
{
	struct net_device *netdev = rq_desc->netdev;
	struct sss_nic_dev *nic_dev = netdev_priv(rq_desc->netdev);
	struct sk_buff *skb = NULL;
	u32 offload_type;
	u16 lro_segs;

#ifdef HAVE_XDP_SUPPORT
	u32 xdp_result;

	sss_nic_bpf_prog_run_xdp(rq_desc, pkt_len, &xdp_result);
	if (xdp_result == SSSNIC_XDP_PKT_DROP)
		return 0;
#endif

	sss_nic_fetch_one_skb(rq_desc, pkt_len, skb);
	if (unlikely(!skb)) {
		SSSNIC_RQ_STATS_INC(rq_desc, alloc_skb_err);
		return -ENOMEM;
	}

	/* place header in linear portion of buffer */
	if (skb_is_nonlinear(skb))
		sss_nic_pull_tail(skb);

	offload_type = sss_hw_cpu32(rx_cqe->offload_type);
	sss_nic_check_rx_csum(rq_desc, offload_type, status, skb);
	sss_nic_check_rx_gso(rq_desc, offload_type, skb);
	sss_nic_vlan_put_tag(skb, netdev, offload_type, vlan_len);

	if (unlikely(test_bit(SSSNIC_LP_TEST, &nic_dev->flags)))
		sss_nic_loop_copy_data(nic_dev, skb);

	lro_segs = SSSNIC_GET_RX_NUM_LRO(status);
	if (lro_segs > 0)
		sss_nic_update_gso_params(skb, lro_segs);

	skb_record_rx_queue(skb, rq_desc->qid);
	skb->protocol = eth_type_trans(skb, netdev);

	if (skb_has_frag_list(skb)) {
#ifdef HAVE_NAPI_GRO_FLUSH_OLD
		napi_gro_flush(&rq_desc->irq_cfg->napi, false);
#else
		napi_gro_flush(&rq_desc->irq_cfg->napi);
#endif
		netif_receive_skb(skb);
	} else {
		napi_gro_receive(&rq_desc->irq_cfg->napi, skb);
	}

	return 0;
}

int sss_nic_rx_poll(struct sss_nic_rq_desc *rq_desc, int budget)
{
	struct sss_nic_dev *nic_dev = netdev_priv(rq_desc->netdev);
	struct sss_nic_cqe *rx_cqe = NULL;
	u64 rx_bytes = 0;
	int pkts = 0;
	int rx_packets = 0;
	u16 wqe_num = 0;
	u16 lro_segs;
	u32 ci;
	u32 status;
	u32 pkt_len;
	u32 vlan_len;

	while (likely(pkts < budget)) {
		ci = rq_desc->ci & rq_desc->qid_mask;
		rx_cqe = rq_desc->rx_desc_group[ci].cqe;
		status = sss_hw_cpu32(rx_cqe->state);
		if (!SSSNIC_GET_RX_DONE(status))
			break;

		/* read rx cqe firstly */
		rmb();

		vlan_len = sss_hw_cpu32(rx_cqe->vlan_len);
		pkt_len = SSSNIC_GET_RX_PKT_LEN(vlan_len);
		if (sss_nic_recv_one_packet(rq_desc, rx_cqe, pkt_len, vlan_len, status))
			break;

		rx_bytes += pkt_len;
		pkts++;
		rx_packets++;

		lro_segs = SSSNIC_GET_RX_NUM_LRO(status);
		if (lro_segs > 0) {
			rx_bytes += ((lro_segs - 1) * SSSNIC_LRO_PKT_HDR_LEN(rx_cqe));
			wqe_num += SSSNIC_GET_SGE_NUM(pkt_len, rq_desc);
		}

		rx_cqe->state = 0;

		if (wqe_num >= nic_dev->rx_poll_wqe)
			break;
	}

	if (rq_desc->delta >= SSSNIC_RX_BUFFER_WRITE)
		SSS_NIC_FILL_BD_SGE(rq_desc);

	u64_stats_update_begin(&rq_desc->stats.stats_sync);
	rq_desc->stats.rx_packets += (u64)(u32)rx_packets;
	rq_desc->stats.rx_bytes += rx_bytes;
	u64_stats_update_end(&rq_desc->stats.stats_sync);

	return pkts;
}
