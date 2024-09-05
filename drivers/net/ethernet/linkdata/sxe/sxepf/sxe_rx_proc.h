
#ifndef __SXE_RX_H__
#define __SXE_RX_H__

#include "sxe.h"

#define ALIGN_4K    		(4096)
#define SXE_RX_BUFFER_WRITE	(16)	

#define SXE_RXBUFFER_256    (256)    
#define SXE_RXBUFFER_1536   (1536)
#define SXE_RXBUFFER_2K     (2048)
#define SXE_RXBUFFER_3K     (3072)
#define SXE_RXBUFFER_4K     (4096)
#define SXE_MAX_RXBUFFER    (16384)  

#define SXE_RX_HDR_SIZE     SXE_RXBUFFER_256
#define SXE_MIN_LRO_ITR     (24)
#define SXE_RXDADV_RSSTYPE_MASK       (0x0000000F)

#define SXE_ETH_FRAMING     (20)

#define ETH_RSS_IPV4               (1ULL << 2)
#define ETH_RSS_NONFRAG_IPV4_TCP   (1ULL << 4)
#define ETH_RSS_NONFRAG_IPV4_UDP   (1ULL << 5)
#define ETH_RSS_IPV6               (1ULL << 8)
#define ETH_RSS_NONFRAG_IPV6_TCP   (1ULL << 10)
#define ETH_RSS_NONFRAG_IPV6_UDP   (1ULL << 11)


#define SXE_BT2KB(BT) ((BT + (8 * 1024 - 1)) / (8 * 1024))
#define SXE_B2BT(BT) (BT * 8)

#define SXE_PFC_D	672

#define SXE_CABLE_DC	5556 
#define SXE_CABLE_DO	5000 

#define SXE_PHY_D	12800
#define SXE_MAC_D	4096
#define SXE_XAUI_D	(2 * 1024)

#define SXE_ID	(SXE_MAC_D + SXE_XAUI_D + SXE_PHY_D)

#define SXE_HD	6144

#define SXE_PCI_DELAY	10000

#define SXE_DV(_max_frame_link, _max_frame_tc) \
			((36 * \
			  (SXE_B2BT(_max_frame_link) + \
			   SXE_PFC_D + \
			   (2 * SXE_CABLE_DC) + \
			   (2 * SXE_ID) + \
			   SXE_HD) / 25 + 1) + \
			 2 * SXE_B2BT(_max_frame_tc))

#define SXE_LOW_DV(_max_frame_tc) \
			(2 * (2 * SXE_B2BT(_max_frame_tc) + \
			(36 * SXE_PCI_DELAY / 25) + 1))

struct sxe_skb_ctrl_buffer {
	union {
		struct sk_buff *head;
		struct sk_buff *tail;
	};

	dma_addr_t dma;     
	u16 lro_cnt;        
	bool page_released; 
};

struct sxe_rss_hash_config {
	u8 *rss_key;
	u8  rss_key_len;
	u64 rss_hf;
};

#define SXE_CTRL_BUFFER(skb) ((struct sxe_skb_ctrl_buffer *)(skb)->cb)

#ifdef HAVE_DMA_ATTRS_STRUCT
#define SXE_RX_DMA_ATTR NULL
#else 
#define SXE_RX_DMA_ATTR \
	(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)
#endif

#define ring_uses_build_skb(ring) \
		test_bit(SXE_RX_BUILD_SKB_ENABLED, &(ring)->state)

#if (PAGE_SIZE < 8192)
#define SXE_MAX_2K_FRAME_BUILD_SKB (SXE_RXBUFFER_1536 - NET_IP_ALIGN)
#define SXE_2K_TOO_SMALL_WITH_PADDING \
	((NET_SKB_PAD + SXE_RXBUFFER_1536) > SKB_WITH_OVERHEAD(SXE_RXBUFFER_2K))

static inline u32 sxe_compute_pad(u32 rx_buf_len)
{
	u32 page_size, pad_size;

	page_size = ALIGN(rx_buf_len, PAGE_SIZE / 2);
	pad_size = SKB_WITH_OVERHEAD(page_size) - rx_buf_len;

	return pad_size;
}

static inline u32 sxe_skb_pad(void)
{
	u32 rx_buf_len;

	if (SXE_2K_TOO_SMALL_WITH_PADDING) {
		rx_buf_len = SXE_RXBUFFER_3K + SKB_DATA_ALIGN(NET_IP_ALIGN);
	} else {
		rx_buf_len = SXE_RXBUFFER_1536;
	}

	rx_buf_len -= NET_IP_ALIGN;

	return sxe_compute_pad(rx_buf_len);
}

#define SXE_SKB_PAD	sxe_skb_pad()
#else
#define SXE_SKB_PAD	(NET_SKB_PAD + NET_IP_ALIGN)
#endif

static inline u16 sxe_rx_pg_order(struct sxe_ring *ring)
{
#if (PAGE_SIZE < 8192)
	if (test_bit(SXE_RX_3K_BUFFER, &ring->state)) {
		return 1;
	}

#endif
	return 0;
}

#define sxe_rx_pg_size(_ring) (PAGE_SIZE << sxe_rx_pg_order(_ring))

s32 sxe_rss_hash_conf_get(struct sxe_adapter *adapter,
				struct sxe_rss_hash_config *rss_conf);

s32 sxe_rx_configure(struct sxe_adapter *adapter);

void sxe_rx_ring_free(struct sxe_ring *ring);

void sxe_rx_resources_free(struct sxe_adapter *adapter);

s32 sxe_rx_ring_depth_reset(struct sxe_adapter *adapter, u32 rx_cnt);

void sxe_rx_ring_buffer_clean(struct sxe_ring *ring);

u32 sxe_rx_ring_irq_clean(struct sxe_irq_data *q_vector,
			       struct sxe_ring *rx_ring,
			       const u32 budget);

void sxe_hw_rx_disable(struct sxe_adapter *adapter);

void sxe_hw_rx_configure(struct sxe_adapter *adapter);

void sxe_skb_fields_process(struct sxe_ring *rx_ring,
			      union sxe_rx_data_desc *rx_desc,
			      struct sk_buff *skb);

void sxe_rx_skb_deliver(struct sxe_irq_data *irq_data,
		  struct sk_buff *skb);

void sxe_rx_ring_attr_configure(struct sxe_adapter *adapter,
					struct sxe_ring *ring);

static inline void sxe_rx_ring_buffer_init(struct sxe_ring *rx_ring)
{
	memset(rx_ring->rx_buffer_info, 0,
	   sizeof(struct sxe_rx_buffer) * rx_ring->depth);
	return;
}

static inline void sxe_rx_pkt_stats_update(struct sxe_ring *rx_ring,
					struct sxe_irq_rate *irq_rate,
					struct sxe_ring_stats *stats)
{
	LOG_DEBUG("in the irq, process total packets[%llu], bytes[%llu]\n",
			  stats->packets, stats->bytes);

	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += stats->packets;
	rx_ring->stats.bytes += stats->bytes;
	u64_stats_update_end(&rx_ring->syncp);

	irq_rate->total_packets += stats->packets;
	irq_rate->total_bytes += stats->bytes;

	return;
}

void sxe_rx_ring_buffers_alloc(struct sxe_ring *rx_ring, u16 cleaned_count);

s32 sxe_test_rx_configure(struct sxe_adapter *adapter, struct sxe_ring *ring);

static inline u16 sxe_desc_unused(struct sxe_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->depth) + ntc - ntu - 1;
}

static inline u32 sxe_rx_bufsz(struct sxe_ring *ring)
{
	u32 bufsz;

	if (test_bit(SXE_RX_3K_BUFFER, &ring->state)) {
		bufsz = SXE_RXBUFFER_3K;
		goto l_ret;
	}

#if (PAGE_SIZE < 8192)
	if (ring_uses_build_skb(ring)) {
		bufsz = SXE_MAX_2K_FRAME_BUILD_SKB;
		goto l_ret;
	}

#endif
	bufsz = SXE_RXBUFFER_2K;

l_ret:
	return bufsz;
}

static inline void sxe_rx_release(struct sxe_adapter *adapter)
{
	sxe_rx_resources_free(adapter);

	return;
}

static inline u32 sxe_rss_redir_tbl_size_get(void)
{
	return SXE_MAX_RETA_ENTRIES;
}
#endif
