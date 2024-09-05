#ifndef __SXE_TX_PROC_H__
#define __SXE_TX_PROC_H__

#include "sxe.h"
#include "sxe_ring.h"

#define SXE_IPV4     (4)
#define SXE_IPV6     (6)
#define SXE_ALIGN_4K (4096)

#define SXE_TX_FEATURE_VLAN_PRIO_MASK	0xe0000000
#define SXE_TX_FEATURE_VLAN_PRIO_SHIFT  29
#define SXE_TX_FEATURE_VLAN_SHIFT	16

enum sxe_tx_features {
	SXE_TX_FEATURE_HW_VLAN	= 0x01,
	SXE_TX_FEATURE_TSO	= 0x02,
	SXE_TX_FEATURE_TSTAMP	= 0x04,

	SXE_TX_FEATURE_CC	= 0x08,
	SXE_TX_FEATURE_IPV4	= 0x10,
	SXE_TX_FEATURE_CSUM	= 0x20,
	SXE_TX_FEATURE_IPSEC	= 0x40,
	SXE_TX_FEATURE_SW_VLAN	= 0x80,
};

#define SXE_TX_SET_FLAG(_input, _flag, _result) \
	((_flag <= _result) ? \
	 ((u32)(_input & _flag) * (_result / _flag)) : \
	 ((u32)(_input & _flag) / (_flag / _result)))

union sxe_ip_hdr {
	struct iphdr *v4;
	struct ipv6hdr *v6;
	u8 *hdr;
};
union sxe_l4_hdr {
	struct tcphdr *tcp;
	u8 *hdr;
};

union app_tr_data_hdr {
	u8 *network;
	struct iphdr *ipv4;
	struct ipv6hdr *ipv6;
};

int sxe_tx_configure(struct sxe_adapter *adapter);

void sxe_tx_ring_buffer_clean(struct sxe_ring *ring);

bool sxe_tx_ring_irq_clean(struct sxe_irq_data *irq,
			       struct sxe_ring *ring, s32 napi_budget);

netdev_tx_t sxe_xmit(struct sk_buff *skb, struct net_device *netdev);

s32 sxe_tx_ring_alloc(struct sxe_ring *ring);

void sxe_tx_ring_free(struct sxe_ring *ring);

void sxe_tx_resources_free(struct sxe_adapter *adapter);

s32 sxe_tx_ring_depth_reset(struct sxe_adapter *adapter, u32 tx_cnt);

void sxe_hw_tx_disable(struct sxe_adapter *adapter);

void sxe_hw_tx_configure(struct sxe_adapter *adapter);

#ifdef HAVE_TIMEOUT_TXQUEUE_IDX
void sxe_tx_timeout(struct net_device *netdev, u32 __always_unused txqueue);
#else
void sxe_tx_timeout(struct net_device *netdev);
#endif

bool sxe_tx_ring_pending(struct sxe_adapter *adapter);

void sxe_tx_buffer_dump(struct sxe_adapter *adapter);

void sxe_tx_ring_attr_configure(struct sxe_adapter *adapter, 
					struct sxe_ring *ring);

void sxe_tx_ring_reg_configure(struct sxe_adapter *adapter,
				struct sxe_ring *ring);

netdev_tx_t sxe_ring_xmit(struct sk_buff *skb,
				struct net_device *netdev,
				struct sxe_ring *ring);

s32 sxe_test_tx_configure(struct sxe_adapter *adapter, struct sxe_ring *ring);

static inline void sxe_tx_buffer_init(struct sxe_ring *ring)
{
	memset(ring->tx_buffer_info, 0,
	       sizeof(struct sxe_tx_buffer) * ring->depth);
	return;
}

static inline void sxe_ctxt_desc_iplen_set(
		struct sxe_tx_context_desc *ctxt_desc, u32 iplen)
{
	ctxt_desc->vlan_macip_lens |= iplen;
	return;
}

static inline void sxe_ctxt_desc_maclen_set(
		struct sxe_tx_context_desc *ctxt_desc, u32 maclen)
{
	ctxt_desc->vlan_macip_lens &= ~SXE_TX_CTXTD_MACLEN_MASK;
	ctxt_desc->vlan_macip_lens |= maclen << SXE_TX_CTXTD_MACLEN_SHIFT;
	return;
}

static inline void sxe_ctxt_desc_vlan_tag_set(
		struct sxe_tx_context_desc *ctxt_desc, u32 vlan_tag)
{
	ctxt_desc->vlan_macip_lens |= vlan_tag << SXE_TX_CTXTD_VLAN_SHIFT;
	return;
}

static inline void sxe_ctxt_desc_tucmd_set(
		struct sxe_tx_context_desc *ctxt_desc, u32 tucmd)
{
	ctxt_desc->type_tucmd_mlhl |= tucmd;
	return;
}

static inline void sxe_ctxt_desc_sa_idx_set(
		struct sxe_tx_context_desc *ctxt_desc, u32 sa_idx)
{
	ctxt_desc->sa_idx = sa_idx;
	return;
}

static inline void sxe_ctxt_desc_mss_l4len_set(
		struct sxe_tx_context_desc *ctxt_desc, u32 mss_l4len)
{
	ctxt_desc->mss_l4len_idx = mss_l4len;
	return;
}

static inline __be16 sxe_ctxt_desc_vlan_tag_get(
		struct sxe_tx_context_desc *ctxt_desc)
{
	return (ctxt_desc->vlan_macip_lens >> SXE_TX_CTXTD_VLAN_SHIFT);
}

static inline void sxe_tx_release(struct sxe_adapter *adapter)
{
	sxe_tx_resources_free(adapter);

	return;
}
#endif
