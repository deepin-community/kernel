
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/netdev_features.h>

#include "sxevf_csum.h"
#include "sxevf_ring.h"
#include "sxevf_tx_proc.h"
#include "sxe_log.h"

#ifndef HAVE_SKB_CSUM_SCTP_API
static inline bool sxevf_is_sctp_ipv4(__be16 protocol, struct sk_buff *skb)
{
	bool ret = false;

	if ((protocol == htons(ETH_P_IP)) &&
	    (ip_hdr(skb)->protocol == IPPROTO_SCTP)) {
		LOG_DEBUG("protocal:0x%d tx packet type is ipv4 sctp.\n",
			 protocol);
		ret = true;
	}

	return ret;
}

static inline bool sxevf_is_sctp_ipv6(__be16 protocol, struct sk_buff *skb)
{
	bool ret = false;
	u32 offset = skb_checksum_start_offset(skb);
	u32 hdr_offset = 0;

	ipv6_find_hdr(skb, &hdr_offset, IPPROTO_SCTP, NULL, NULL);

	if ((protocol == htons(ETH_P_IPV6)) &&
	    (offset == hdr_offset)) {
		LOG_DEBUG("protocal:0x%d offset:%d tx packet type is ipv6 sctp.\n",
			   protocol, offset);
		ret = true;
	}

	return ret;
}

static inline bool sxevf_prot_is_sctp(__be16 protocol, struct sk_buff *skb)
{
	bool ret = false;

	if (sxevf_is_sctp_ipv4(protocol, skb) ||
	    sxevf_is_sctp_ipv6(protocol, skb)) {
		ret = true;
	}

	return ret;
}
#else
#define sxevf_prot_is_sctp(protocol, skb) skb_csum_is_sctp(skb)
#endif 

void sxevf_tx_csum_offload(struct sxevf_ring *tx_ring,
				  struct sxevf_tx_buffer *first,
				  struct sxevf_tx_context_desc *ctxt_desc)
{
	struct sk_buff *skb = first->skb;
	u16 tucmd;
	u16 ip_len;
	u16 mac_len;
	struct sxevf_adapter *adapter = netdev_priv(tx_ring->netdev);

	LOG_DEBUG_BDF("tx ring[%d] ip_summed:%d "
		 "csum_offset:%d csum_start:%d protocol:%d "
		 "netdev features:0x%llx\n",
		 tx_ring->idx, skb->ip_summed,
		 skb->csum_offset, skb->csum_start,
		 skb->protocol, tx_ring->netdev->features);

	if (skb->ip_summed != CHECKSUM_PARTIAL) {
		goto no_checksum;
	}

	switch (skb->csum_offset) {
	case SXEVF_TCP_CSUM_OFFSET:
		tucmd = SXEVF_TX_CTXTD_TUCMD_L4T_TCP;
		break;
	case SXEVF_UDP_CSUM_OFFSET:
		tucmd = SXEVF_TX_CTXTD_TUCMD_L4T_UDP;
		break;
	case SXEVF_SCTP_CSUM_OFFSET:
		if (sxevf_prot_is_sctp(first->protocol, skb)) {
			tucmd = SXEVF_TX_CTXTD_TUCMD_L4T_SCTP;
			break;
		}
		fallthrough;
	default:
		skb_checksum_help(skb);
		goto no_checksum;
	}

	if (first->protocol == htons(ETH_P_IP)) {
		tucmd |= SXEVF_TX_CTXTD_TUCMD_IPV4;
	}

	first->tx_features |= SXEVF_TX_FEATURE_CSUM;
	ip_len = skb_checksum_start_offset(skb) - skb_network_offset(skb);

	mac_len = skb_network_offset(skb);

	sxevf_ctxt_desc_tucmd_set(ctxt_desc,  tucmd);
	sxevf_ctxt_desc_iplen_set(ctxt_desc,  ip_len);
	sxevf_ctxt_desc_maclen_set(ctxt_desc, mac_len);

	LOG_DEBUG_BDF("tx ring[%d] protocol:%d tucmd:0x%x "
		"iplen:0x%x mac_len:0x%x, tx_features:0x%x\n",
		tx_ring->idx, first->protocol, tucmd,
		ip_len, mac_len, first->tx_features);

no_checksum:
	return;
}

void sxevf_rx_csum_verify(struct sxevf_ring *ring,
			      union sxevf_rx_data_desc *desc,
			      struct sk_buff *skb)
{
	LOG_DEBUG("rx ring[%d] csum verify ip_summed:%d "
		 "csum_offset:%d csum_start:%d pkt_info:0x%x "
		 "netdev feature:0x%llx\n",
		 ring->idx, skb->ip_summed,
		 skb->csum_offset, skb->csum_start,
		 desc->wb.lower.lo_dword.hs_rss.pkt_info,
		 ring->netdev->features);

	skb_checksum_none_assert(skb);

	if (!(ring->netdev->features & NETIF_F_RXCSUM)) {
		LOG_WARN("rx ring[%d] checksum verify no offload "
			 "ip_summed:%d csum_offset:%d csum_start:%d protocol:0x%x\n",
			 ring->idx, skb->ip_summed,
			 skb->csum_offset, skb->csum_start,
			 skb->protocol);
		goto l_out;
	}

	if (sxevf_status_err_check(desc, SXEVF_RXD_STAT_IPCS) &&
	    sxevf_status_err_check(desc, SXEVF_RXDADV_ERR_IPE)) {
		ring->rx_stats.csum_err++;
		LOG_ERROR("rx ring [%d] ip checksum fail.csum_err:%llu\n",
			   ring->idx, ring->rx_stats.csum_err);
		goto l_out;
	}

	if (sxevf_status_err_check(desc, SXEVF_RXD_STAT_LB)) {
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		goto l_out;
	}

	if (!sxevf_status_err_check(desc, SXEVF_RXD_STAT_L4CS)) {
		LOG_DEBUG("rx ring[%d] no need verify L4 checksum\n",
			  ring->idx);
		goto l_out;
	}

	if (sxevf_status_err_check(desc, SXEVF_RXDADV_ERR_L4E)) {

		ring->rx_stats.csum_err++;

		LOG_ERROR("rx ring[%d] L4 checksum verify error.\n", ring->idx);
		goto l_out;
	}

	skb->ip_summed = CHECKSUM_UNNECESSARY;

l_out:
	return;
}
