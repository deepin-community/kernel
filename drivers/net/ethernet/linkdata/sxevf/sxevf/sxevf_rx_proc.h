
#ifndef __SXEVF_RX_H__
#define __SXEVF_RX_H__

#include "sxevf.h"

#define ALIGN_4K    (4096)
#define SXEVF_RX_BUFFER_WRITE	16	

#define SXEVF_RXBUFFER_256    256  
#define SXEVF_RXBUFFER_1536  1536
#define SXEVF_RXBUFFER_2K    2048
#define SXEVF_RXBUFFER_3K    3072
#define SXEVF_RXBUFFER_4K    4096
#define SXEVF_MAX_RXBUFFER  16384  

#define SXEVF_RX_HDR_SIZE  SXEVF_RXBUFFER_256

#define SXEVF_RXDADV_RSSTYPE_MASK       0x0000000F

#define SXEVF_RXDADV_RSSTYPE_NONE       0x00000000
#define SXEVF_RXDADV_RSSTYPE_IPV4_TCP   0x00000001
#define SXEVF_RXDADV_RSSTYPE_IPV4       0x00000002
#define SXEVF_RXDADV_RSSTYPE_IPV6_TCP   0x00000003
#define SXEVF_RXDADV_RSSTYPE_IPV6_EX    0x00000004
#define SXEVF_RXDADV_RSSTYPE_IPV6       0x00000005
#define SXEVF_RXDADV_RSSTYPE_IPV6_TCP_EX 0x00000006
#define SXEVF_RXDADV_RSSTYPE_IPV4_UDP   0x00000007
#define SXEVF_RXDADV_RSSTYPE_IPV6_UDP   0x00000008
#define SXEVF_RXDADV_RSSTYPE_IPV6_UDP_EX 0x00000009

#define SXEVF_RSS_L4_TYPES_MASK \
	((1ul << SXEVF_RXDADV_RSSTYPE_IPV4_TCP) | \
	 (1ul << SXEVF_RXDADV_RSSTYPE_IPV4_UDP) | \
	 (1ul << SXEVF_RXDADV_RSSTYPE_IPV6_TCP) | \
	 (1ul << SXEVF_RXDADV_RSSTYPE_IPV6_UDP))

#define SXEVF_SKB_PAD		(NET_SKB_PAD + NET_IP_ALIGN)
#if (PAGE_SIZE < 8192)
#define SXEVF_MAX_FRAME_BUILD_SKB \
	(SKB_WITH_OVERHEAD(SXEVF_RXBUFFER_2K) - SXEVF_SKB_PAD)
#else
#define SXEVF_MAX_FRAME_BUILD_SKB	SXEVF_RXBUFFER_2K
#endif

#define vf_ring_uses_build_skb(ring) \
		test_bit(SXEVF_RX_BUILD_SKB_ENABLED, &(ring)->state)

#ifdef HAVE_DMA_ATTRS_STRUCT
#define SXEVF_RX_DMA_ATTR NULL
#else
#define SXEVF_RX_DMA_ATTR \
	(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)
#endif

static inline u16 sxevf_rx_pg_order(struct sxevf_ring *ring)
{
#if (PAGE_SIZE < 8192)
	if (test_bit(SXEVF_RX_3K_BUFFER, &ring->state)) {
		return 1;
	}

#endif
	return 0;
}

#define sxevf_rx_pg_size(_ring) (PAGE_SIZE << sxevf_rx_pg_order(_ring))

s32 sxevf_rx_configure(struct sxevf_adapter *adapter);
void sxevf_rx_resources_free(struct sxevf_adapter *adapter);

static inline u16 sxevf_desc_unused(struct sxevf_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->depth) + ntc - ntu - 1;
}

static inline u32 sxevf_rx_bufsz(struct sxevf_ring *ring)
{
	u32 bufsz;

#if (PAGE_SIZE < 8192)
	if (test_bit(SXEVF_RX_3K_BUFFER, &ring->state)) {
		bufsz = SXEVF_RXBUFFER_3K;
		goto l_ret;
	}

	if (vf_ring_uses_build_skb(ring)) {
		bufsz = SXEVF_MAX_FRAME_BUILD_SKB;
		goto l_ret;
	}

#endif
	bufsz = SXEVF_RXBUFFER_2K;

#if (PAGE_SIZE < 8192)
l_ret:
#endif
	return bufsz;
}

static inline u32 sxevf_rx_offset(struct sxevf_ring *rx_ring)
{
	return vf_ring_uses_build_skb(rx_ring) ? SXEVF_SKB_PAD : 0;
}

void sxevf_rx_ring_buffer_clean(struct sxevf_ring *ring);

u32 sxevf_rx_ring_irq_clean(struct sxevf_irq_data *q_vector,
			       struct sxevf_ring *rx_ring,
			       const u32 budget);

s32 sxevf_rx_ring_depth_reset(struct sxevf_adapter *adapter, u32 rx_cnt);

void sxevf_hw_rx_disable(struct sxevf_adapter *adapter);

static inline void sxevf_rx_release(struct sxevf_adapter *adapter)
{
	sxevf_rx_resources_free(adapter);

	return;
}

s32 sxevf_rx_configure(struct sxevf_adapter *adapter);

void sxevf_hw_rx_configure(struct sxevf_adapter *adapter);

s32 sxevf_rx_max_frame_configure(struct sxevf_adapter *adapter, u32 mtu);

#endif
