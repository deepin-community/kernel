#ifndef __SXEVF_TX_PROC_H__
#define __SXEVF_TX_PROC_H__

#include "sxevf.h"
#include "sxevf_ring.h"

#define SXEVF_TX_CTXTD_TUCMD_IPV4     0x00000400 
#define SXEVF_TX_CTXTD_TUCMD_L4T_UDP  0x00000000 
#define SXEVF_TX_CTXTD_TUCMD_L4T_TCP  0x00000800 
#define SXEVF_TX_CTXTD_TUCMD_L4T_SCTP 0x00001000 
#define SXEVF_TX_CTXTD_TUCMD_IPSEC_TYPE_ESP   0x00002000 
#define SXEVF_TX_CTXTD_TUCMD_IPSEC_ENCRYPT_EN 0x00004000 

#define SXEVF_TX_CTXTD_L4LEN_SHIFT    8  
#define SXEVF_TX_CTXTD_MSS_SHIFT      16 
#define SXEVF_TX_CTXTD_MACLEN_SHIFT   9  
#define SXEVF_TX_CTXTD_VLAN_MASK      0xffff0000

enum sxevf_tx_features {
	SXEVF_TX_FEATURE_CSUM	= BIT(0),
	SXEVF_TX_FEATURE_VLAN   = BIT(1),
	SXEVF_TX_FEATURE_TSO	= BIT(2),
	SXEVF_TX_FEATURE_IPV4	= BIT(3),
	SXEVF_TX_FEATURE_IPSEC	= BIT(4),
};

union sxevf_ip_hdr {
	struct iphdr *v4;
	struct ipv6hdr *v6;
	unsigned char *hdr;
};
union sxevf_l4_hdr {
	struct tcphdr *tcp;
	unsigned char *hdr;
};

#ifdef HAVE_TIMEOUT_TXQUEUE_IDX
void sxevf_tx_timeout(struct net_device *netdev, u32 __always_unused txqueue);
#else
void sxevf_tx_timeout(struct net_device *netdev);
#endif

s32 sxevf_tx_ring_depth_reset(struct sxevf_adapter *adapter, u32 tx_cnt);

void sxevf_hw_tx_disable(struct sxevf_adapter *adapter);

void sxevf_hw_tx_configure(struct sxevf_adapter *adapter);

s32 sxevf_tx_configure(struct sxevf_adapter *adapter);

void sxevf_tx_ring_buffer_clean(struct sxevf_ring *ring);

void sxevf_tx_resources_free(struct sxevf_adapter *adapter);

bool sxevf_tx_ring_irq_clean(struct sxevf_irq_data *irq,
				struct sxevf_ring *ring, s32 napi_budget);

bool sxevf_xdp_ring_irq_clean(struct sxevf_irq_data *irq,
				struct sxevf_ring *xdp_ring, s32 napi_budget);

netdev_tx_t sxevf_xmit(struct sk_buff *skb, struct net_device *netdev);

static inline void sxevf_ctxt_desc_iplen_set(
		struct sxevf_tx_context_desc *ctxt_desc, u32 iplen)
{
	ctxt_desc->vlan_macip_lens |= iplen;
	return;
}

static inline void sxevf_ctxt_desc_maclen_set(
		struct sxevf_tx_context_desc *ctxt_desc, u32 maclen)
{
	ctxt_desc->vlan_macip_lens &= ~SXEVF_TX_CTXTD_MACLEN_MASK;
	ctxt_desc->vlan_macip_lens |= maclen << SXEVF_TX_CTXTD_MACLEN_SHIFT;
	return;
}

static inline void sxevf_ctxt_desc_vlan_tag_set(
		struct sxevf_tx_context_desc *ctxt_desc, u32 vlan_tag)
{
	ctxt_desc->vlan_macip_lens |= vlan_tag << SXEVF_TX_CTXTD_VLAN_SHIFT;
	return;
}

static inline void sxevf_ctxt_desc_sa_idx_set(
		struct sxevf_tx_context_desc *ctxt_desc, u32 sa_idx)
{
	ctxt_desc->sa_idx = sa_idx;
	return;
}

static inline void sxevf_ctxt_desc_tucmd_set(
		struct sxevf_tx_context_desc *ctxt_desc, u32 tucmd)
{
	ctxt_desc->type_tucmd_mlhl |= tucmd;
	return;
}

static inline void sxevf_ctxt_desc_mss_l4len_set(
		struct sxevf_tx_context_desc *ctxt_desc, u32 mss_l4len)
{
	ctxt_desc->mss_l4len_idx = mss_l4len;
	return;
}

static inline __be16 sxevf_ctxt_desc_vlan_tag_get(
		struct sxevf_tx_context_desc *ctxt_desc)
{
	return (ctxt_desc->vlan_macip_lens >> SXEVF_TX_CTXTD_VLAN_SHIFT);
}

static inline void sxevf_tx_release(struct sxevf_adapter *adapter)
{
	sxevf_tx_resources_free(adapter);

	return;
}
#endif 