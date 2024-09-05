
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/netdev_features.h>

#include "sxe_csum.h"
#include "sxe_ring.h"
#include "sxe_tx_proc.h"
#include "sxe_log.h"

#ifndef HAVE_SKB_CSUM_SCTP_API
static inline bool sxe_is_sctp_ipv4(__be16 protocol, struct sk_buff *skb)
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

static inline bool sxe_is_sctp_ipv6(__be16 protocol, struct sk_buff *skb)
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

static inline bool sxe_prot_is_sctp(__be16 protocol, struct sk_buff *skb)
{
	bool ret = false;

	if (sxe_is_sctp_ipv4(protocol, skb) ||
	    sxe_is_sctp_ipv6(protocol, skb)) {
		ret = true;
	}

	return ret;
}
#else
#define sxe_prot_is_sctp(protocol, skb) skb_csum_is_sctp(skb)
#endif 

void sxe_tx_csum_offload(struct sxe_ring *tx_ring,
				  struct sxe_tx_buffer *first,
				  struct sxe_tx_context_desc *ctxt_desc)
{
	u16 tucmd;
	u16 ip_len;
	u16 mac_len;
	struct sk_buff *skb = first->skb;
	struct sxe_adapter *adapter = netdev_priv(tx_ring->netdev);

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
	case SXE_TCP_CSUM_OFFSET:
		tucmd = SXE_TX_CTXTD_TUCMD_L4T_TCP;
		break;
	case SXE_UDP_CSUM_OFFSET:
		tucmd = SXE_TX_CTXTD_TUCMD_L4T_UDP;
		break;
	case SXE_SCTP_CSUM_OFFSET:
		if (sxe_prot_is_sctp(first->protocol, skb)) {
			tucmd = SXE_TX_CTXTD_TUCMD_L4T_SCTP;
			break;
		}
		fallthrough;
	default:
		skb_checksum_help(skb);
		goto no_checksum;
	}

	first->tx_features |= SXE_TX_FEATURE_CSUM;
	ip_len = skb_checksum_start_offset(skb) - skb_network_offset(skb);

	mac_len = skb_network_offset(skb);

	sxe_ctxt_desc_tucmd_set(ctxt_desc,  tucmd);
	sxe_ctxt_desc_iplen_set(ctxt_desc,  ip_len);
	sxe_ctxt_desc_maclen_set(ctxt_desc, mac_len);

	LOG_DEBUG_BDF("tx ring[%d] L3 protocol:%d tucmd:0x%x "
		"iplen:0x%x mac_len:0x%x, tx_features:0x%x\n",
		tx_ring->idx, first->protocol, tucmd,
		ip_len, mac_len, first->tx_features);

no_checksum:
	return;
}

void sxe_rx_csum_verify(struct sxe_ring *ring,
			      union sxe_rx_data_desc *desc,
			      struct sk_buff *skb)
{
#ifndef SXE_DRIVER_RELEASE
	__le16 pkt_info = desc->wb.lower.lo_dword.hs_rss.pkt_info;

	LOG_DEBUG("rx ring[%d] csum verify ip_summed:%d "
		 "csum_offset:%d csum_start:%d pkt_info:0x%x "
		 "netdev features:0x%llx\n",
		 ring->idx, skb->ip_summed,
		 skb->csum_offset, skb->csum_start,
		 pkt_info, ring->netdev->features);
#endif
	skb_checksum_none_assert(skb);

	if (!(ring->netdev->features & NETIF_F_RXCSUM)) {
		LOG_WARN("rx ring[%d] no offload checksum verify.\n", ring->idx);
		goto l_out;
	}

	if (sxe_status_err_check(desc, SXE_RXD_STAT_IPCS) &&
	    sxe_status_err_check(desc, SXE_RXDADV_ERR_IPE)) {
		ring->rx_stats.csum_err++;
		LOG_ERROR("rx ring [%d] ip checksum fail.csum_err:%llu\n",
			   ring->idx, ring->rx_stats.csum_err);
		goto l_out;
	}

	if (sxe_status_err_check(desc, SXE_RXD_STAT_LB)) {
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		goto l_out;
	}

	if (!sxe_status_err_check(desc, SXE_RXD_STAT_L4CS)) {
		LOG_DEBUG("rx ring[%d] no need verify L4 checksum\n",
			  ring->idx);
		goto l_out;
	}

	if (sxe_status_err_check(desc, SXE_RXDADV_ERR_L4E)) {
		ring->rx_stats.csum_err++;

		LOG_ERROR("rx ring[%d] L4 checksum verify error.csum_err:%llu\n",
			  ring->idx, ring->rx_stats.csum_err);
		goto l_out;
	}

	skb->ip_summed = CHECKSUM_UNNECESSARY;

	LOG_DEBUG("rx ring[%d] ip_summed:%d sxe hw "
		"verify checksum pass.\n",
		ring->idx, skb->ip_summed);

l_out:
	return;
}
