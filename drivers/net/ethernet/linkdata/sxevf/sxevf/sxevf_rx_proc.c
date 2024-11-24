#include <linux/etherdevice.h>
#include <linux/vmalloc.h>

#include "sxevf.h"
#ifdef HAVE_NO_OVERFLOW_H
#include <sxe_compat_overflow.h>
#else
#include <linux/overflow.h>
#endif
#include "sxe_log.h"
#include "sxevf_hw.h"
#include "sxevf_rx_proc.h"
#include "sxevf_netdev.h"
#include "sxevf_pci.h"
#include "sxevf_msg.h"
#include "sxevf_csum.h"
#include "sxevf_ipsec.h"
#include "sxevf_debug.h"
#include "sxevf_xdp.h"

#ifdef XDP_RXQ_INFO_REQ_API_NEED_3_PARAMS
static inline int xdp_rxq_info_reg_compat(struct xdp_rxq_info *xdp_rxq,
			struct net_device *dev,
			u32 queue_index, unsigned int __always_unused napi_id)
{
	return xdp_rxq_info_reg(xdp_rxq, dev, queue_index);
}

#define xdp_rxq_info_reg(xdp_rxq, dev, queue_index, napi_id) \
	xdp_rxq_info_reg_compat(xdp_rxq, dev, queue_index, napi_id)
#endif

static s32 sxevf_rx_ring_alloc(struct sxevf_ring *ring)
{
	s32 ret = 0;
	u32 size;
	union sxevf_rx_data_desc *desc;
	struct sxevf_adapter *adapter = netdev_priv(ring->netdev);

	size = sizeof(struct sxevf_rx_buffer) * ring->depth;
	ring->rx_buffer_info = vmalloc(size);
	if (!ring->rx_buffer_info) {
		LOG_ERROR_BDF("ring[%u] unable to allocate memory for"
			"the rx_buffer_info\n", ring->idx);
		ret = -ENOMEM;
		goto l_rx_buf_alloc_failed;
	}

	memset(ring->rx_buffer_info, 0,
		   sizeof(struct sxevf_rx_buffer) * ring->depth);

	u64_stats_init(&ring->syncp);

	ring->size = ring->depth * sizeof(union sxevf_rx_data_desc);
	ring->size = ALIGN(ring->size, SXEVF_DESC_ALIGN_4K);

	ring->desc.base_addr = dma_alloc_coherent(ring->dev, ring->size,
					   &ring->desc.dma, GFP_KERNEL);
	if (!ring->desc.base_addr) {
		LOG_ERROR_BDF("ring[%u] unable to allocate memory for the descriptor\n",
				ring->idx);
		ret = -ENOMEM;
		goto l_base_addr_alloc_failed;
	}

	desc = SXEVF_RX_DESC(ring, 0);
	desc->wb.upper.length = 0;

	ring->next_to_clean = 0;
	ring->next_to_use   = 0;

#ifndef HAVE_NO_XDP_BUFF_RXQ
	if (xdp_rxq_info_reg(&ring->xdp_rxq, adapter->netdev, ring->idx, 0) < 0) {
		LOG_ERROR_BDF("ring[%u] xdp rxq info reg failed\n",ring->idx);
		ret = -ENOMEM;
		goto l_xdp_rxq_reg_failed;
	}
#endif

	ring->xdp_prog = adapter->xdp_prog;

	return ret;

#ifndef HAVE_NO_XDP_BUFF_RXQ
l_xdp_rxq_reg_failed:
	dma_free_coherent(ring->dev, ring->size,
			ring->desc.base_addr, ring->desc.dma);
	ring->desc.base_addr = NULL;
#endif

l_base_addr_alloc_failed:
	vfree(ring->rx_buffer_info);
	ring->rx_buffer_info = NULL;

l_rx_buf_alloc_failed:
	LOG_DEV_ERR("unable to allocate memory for the Rx descriptor ring\n");
	return ret;
}

void sxevf_rx_ring_buffer_clean(struct sxevf_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	struct sxevf_rx_buffer *rx_buffer = &ring->rx_buffer_info[ntc];
#ifdef HAVE_DMA_ATTRS_STRUCT
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif

	LOG_DEBUG("clean ring[%u] from ntc=%u, next_to_alloc=%u, rx_buffer[%p]\n",
		ring->idx, ntc, ring->next_to_alloc, rx_buffer);

	if (ring->skb) {
		dev_kfree_skb(ring->skb);
		ring->skb = NULL;
	}

	while (ntc != ring->next_to_alloc) {

		dma_sync_single_range_for_cpu(ring->dev,
						rx_buffer->dma,
						rx_buffer->page_offset,
						sxevf_rx_bufsz(ring),
						DMA_FROM_DEVICE);

		dma_unmap_page_attrs(ring->dev, rx_buffer->dma,
					sxevf_rx_pg_size(ring),
					DMA_FROM_DEVICE,
#ifdef HAVE_DMA_ATTRS_STRUCT
					&attrs);
#else
					SXEVF_RX_DMA_ATTR);
#endif
		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);

		ntc++;
		rx_buffer++;
		if (ntc == ring->depth) {
			ntc = 0;
			rx_buffer = ring->rx_buffer_info;
		}
	}

	ring->next_to_alloc = 0;
	ring->next_to_clean = 0;
	ring->next_to_use = 0;

	return;
}

static void sxevf_rx_ring_free(struct sxevf_ring *ring)
{
	sxevf_rx_ring_buffer_clean(ring);

	ring->xdp_prog = NULL;
#ifndef HAVE_NO_XDP_BUFF_RXQ
	xdp_rxq_info_unreg(&ring->xdp_rxq);
#endif

	if (ring->rx_buffer_info) {
		vfree(ring->rx_buffer_info);
		ring->rx_buffer_info = NULL;
	}

	if (ring->desc.base_addr) {
		dma_free_coherent(ring->dev, ring->size,
			ring->desc.base_addr, ring->desc.dma);
		ring->desc.base_addr = NULL;
	}

	return;
}

void sxevf_rx_resources_free(struct sxevf_adapter *adapter)
{
	u16 i;

	LOG_DEBUG_BDF("free:adapter->rx_ring_ctxt.num = %u\n", adapter->rx_ring_ctxt.num);
	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		if (adapter->rx_ring_ctxt.ring[i]->desc.base_addr) {
			sxevf_rx_ring_free(adapter->rx_ring_ctxt.ring[i]);
		}
	}

	return;
}

static int sxevf_rx_resources_alloc(struct sxevf_adapter *adapter)
{
	int ret;
	u16 i;

	LOG_DEBUG_BDF("adapter->rx_ring_ctxt.num = %u\n", adapter->rx_ring_ctxt.num);
	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		ret = sxevf_rx_ring_alloc(adapter->rx_ring_ctxt.ring[i]);
		if (!ret) {
			continue;
		}

		LOG_DEV_ERR("allocation for Rx ring[%u] failed\n", i);
		goto l_err;
	}

	return 0;

l_err:
	while (i--) {
		sxevf_rx_ring_free(adapter->rx_ring_ctxt.ring[i]);
	}

	return ret;
}

s32 sxevf_rx_ring_depth_reset(struct sxevf_adapter *adapter, u32 rx_cnt)
{
	s32 ret;
	u32 i, rx_ring_cnt;
	struct sxevf_ring *temp_ring;
	struct sxevf_ring **rx_ring   = adapter->rx_ring_ctxt.ring;

	rx_ring_cnt = adapter->rx_ring_ctxt.num;
	temp_ring = vmalloc(array_size(rx_ring_cnt, sizeof(struct sxevf_ring)));
	if (!temp_ring) {
		LOG_ERROR_BDF("vmalloc temp_ring failed, size=%lu\n",
			array_size(rx_ring_cnt, sizeof(struct sxevf_ring)));
		ret = -ENOMEM;
		goto l_end;
	}

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		memcpy(&temp_ring[i], rx_ring[i], sizeof(struct sxevf_ring));

#ifndef HAVE_NO_XDP_BUFF_RXQ
		memset(&temp_ring[i].xdp_rxq, 0, sizeof(temp_ring[i].xdp_rxq));
#endif
		temp_ring[i].depth = rx_cnt;
		ret = sxevf_rx_ring_alloc(&temp_ring[i]);
		if (ret) {
			LOG_ERROR_BDF("xdp ring alloc failed, rx ring idx=%d\n", i);
			goto l_rx_free;
		}
	}

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		sxevf_rx_ring_free(rx_ring[i]);
		memcpy(rx_ring[i], &temp_ring[i], sizeof(struct sxevf_ring));
	}

	adapter->rx_ring_ctxt.depth = rx_cnt;
	goto l_temp_free;

l_rx_free:
	while (i--) {
		sxevf_rx_ring_free(&temp_ring[i]);
	}

l_temp_free:
	vfree(temp_ring);

l_end:
	return ret;
}

static void sxevf_rx_buffer_size_set(struct sxevf_adapter *adapter,
						struct sxevf_ring *rx_ring)
{
#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
#if (PAGE_SIZE < SXEVF_PAGE_SIZE_8KB)
	u32 max_frame = sxevf_sw_mtu_get(adapter);
#endif
#endif

	clear_bit(SXEVF_RX_3K_BUFFER, &rx_ring->state);
	clear_bit(SXEVF_RX_BUILD_SKB_ENABLED, &rx_ring->state);

#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
	if (adapter->cap & SXEVF_RX_LEGACY_ENABLE) {
		LOG_INFO_BDF("in rx legacy mode, buffer size=2K\n");
		goto l_end;
	}

	set_bit(SXEVF_RX_BUILD_SKB_ENABLED, &rx_ring->state);
	LOG_INFO_BDF("in rx build skb mode, buffer size=2K\n");

#if (PAGE_SIZE < SXEVF_PAGE_SIZE_8KB)
	if (max_frame > SXEVF_MAX_FRAME_BUILD_SKB) {
		set_bit(SXEVF_RX_3K_BUFFER, &rx_ring->state);
		LOG_INFO_BDF("in rx max frame build skb mode, buffer size=3K\n");
	}

#endif
#else
	adapter->cap |= SXEVF_RX_LEGACY_ENABLE;
#endif

#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
l_end:
#endif
	return;
}

STATIC bool sxevf_mapped_page_alloc(struct sxevf_ring *rx_ring,
				    struct sxevf_rx_buffer *rx_buf)
{
	bool ret;
	struct page *page = rx_buf->page;
	dma_addr_t dma;
#ifdef HAVE_DMA_ATTRS_STRUCT
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif

	if (likely(page)) {
		ret = true;
		goto l_ret;
	}

	page = dev_alloc_pages(sxevf_rx_pg_order(rx_ring));
	if (unlikely(!page)) {
		LOG_DEBUG("ring[%u] page alloc failed\n", rx_ring->idx);
		rx_ring->rx_stats.alloc_rx_page_failed++;
		ret = false;
		goto l_ret;
	}

	dma = dma_map_page_attrs(rx_ring->dev, page, 0,
				 sxevf_rx_pg_size(rx_ring),
				 DMA_FROM_DEVICE,
#ifdef HAVE_DMA_ATTRS_STRUCT
				 &attrs);
#else
				 SXEVF_RX_DMA_ATTR);
#endif
	if (dma_mapping_error(rx_ring->dev, dma)) {
		LOG_DEBUG("ring[%u] dma mapping failed\n", rx_ring->idx);
		__free_pages(page, sxevf_rx_pg_order(rx_ring));

		rx_ring->rx_stats.alloc_rx_page_failed++;
		ret = false;
		goto l_ret;
	}

	rx_buf->dma = dma;
	rx_buf->page = page;
	rx_buf->page_offset = sxevf_rx_offset(rx_ring);
	rx_buf->pagecnt_bias = 1;
	rx_ring->rx_stats.alloc_rx_page++;

	return true;

l_ret:
	return ret;
}

void sxevf_rx_ring_buffers_alloc(struct sxevf_ring *rx_ring, u16 cleaned_count)
{
	union sxevf_rx_data_desc *rx_desc;
	struct sxevf_rx_buffer *rx_buffer;
	u16 ntu = rx_ring->next_to_use;
	u16 bufsz;

	LOG_DEBUG("ring[%u] entry, cleand_count=%u, next_to_use=%u\n",
			rx_ring->idx, cleaned_count, ntu);

	if (!cleaned_count || !rx_ring->netdev) {
		return;
	}

	rx_desc = SXEVF_RX_DESC(rx_ring, ntu);
	rx_buffer = &rx_ring->rx_buffer_info[ntu];
	ntu -= rx_ring->depth;

	bufsz = sxevf_rx_bufsz(rx_ring);
	LOG_DEBUG("bufsz=%u\n", bufsz);

	do {
		if (!sxevf_mapped_page_alloc(rx_ring, rx_buffer)) {
			LOG_DEBUG("page alloc failed, clean_count left:%u\n",
				cleaned_count);
			break;
		}

		dma_sync_single_range_for_device(rx_ring->dev, rx_buffer->dma,
						 rx_buffer->page_offset, bufsz,
						 DMA_FROM_DEVICE);

		rx_desc->read.pkt_addr =
			cpu_to_le64(rx_buffer->dma + rx_buffer->page_offset);

		rx_desc++;
		rx_buffer++;
		ntu++;
		if (unlikely(!ntu)) {
			rx_desc = SXEVF_RX_DESC(rx_ring, 0);
			rx_buffer = rx_ring->rx_buffer_info;
			ntu -= rx_ring->depth;
		}

		rx_desc->wb.upper.length = 0;

		cleaned_count--;
	} while (cleaned_count);

	ntu += rx_ring->depth;

	if (rx_ring->next_to_use != ntu) {
		rx_ring->next_to_use = ntu;

		rx_ring->next_to_alloc = ntu;

		wmb();
		writel(ntu, rx_ring->desc.tail);
	}

	return;
}

static inline void sxevf_rx_ring_source_init(struct sxevf_ring *ring)
{
	union sxevf_rx_data_desc *rx_desc;

	memset(ring->rx_buffer_info, 0,
	       sizeof(struct sxevf_rx_buffer) * ring->depth);

	rx_desc = SXEVF_RX_DESC(ring, 0);
	rx_desc->wb.upper.length = 0;

	ring->next_to_clean = 0;
	ring->next_to_use = 0;
	ring->next_to_alloc = 0;

	return;
}

static inline void sxevf_rx_ring_reg_configure(struct sxevf_adapter *adapter,
						struct sxevf_ring *ring)
{
	struct sxevf_hw *hw = &adapter->hw;
	u64 desc_dma_addr = ring->desc.dma;
	u8 reg_idx = ring->reg_idx;
	u32 desc_mem_len;
	u32 pkg_buf_len;

	hw->dma.ops->rx_ring_switch(hw, reg_idx, false);

	desc_mem_len = ring->depth * sizeof(union sxevf_rx_data_desc);
	hw->dma.ops->rx_ring_desc_configure(hw, desc_mem_len, desc_dma_addr, reg_idx);

	ring->desc.tail = adapter->hw.reg_base_addr + SXE_VFRDT(reg_idx);

	if (test_bit(SXEVF_RX_3K_BUFFER, &ring->state)) {
		pkg_buf_len = SXEVF_RXBUFFER_3K;
	} else {
		pkg_buf_len = SXEVF_RXBUFFER_2K;
	}

	hw->dma.ops->rx_rcv_ctl_configure(hw, reg_idx,
				SXEVF_RX_HDR_SIZE, pkg_buf_len, true);

	hw->dma.ops->rx_ring_switch(hw, reg_idx, true);

	return;
}

static void sxevf_rx_ring_configure(struct sxevf_adapter *adapter)
{
	u32 i;
	struct sxevf_ring **ring = adapter->rx_ring_ctxt.ring;

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		sxevf_rx_ring_source_init(ring[i]);

		sxevf_rx_buffer_size_set(adapter, ring[i]);

		sxevf_rx_ring_reg_configure(adapter, ring[i]);

		sxevf_rx_ring_buffers_alloc(ring[i],
					sxevf_desc_unused(ring[i]));
	}

	return;
}

static void sxevf_vlan_restore(struct sxevf_adapter *adapter)
{
	u16 vid;

	for_each_set_bit(vid, adapter->active_vlans, VLAN_N_VID) {
		sxevf_vlan_rx_add_vid(adapter->netdev,
					htons(ETH_P_8021Q), vid);
		LOG_DEBUG_BDF("restore vlan[%u] succeed\n", vid);
	}

	return;
}

static void sxevf_max_used_rx_queue_configure(struct sxevf_adapter *adapter)
{
	struct sxevf_hw *hw = &adapter->hw;
	u16 max_rx_ring = adapter->rx_ring_ctxt.num;

	hw->dbu.ops->rx_max_used_ring_set(hw, max_rx_ring);
	return;
}

s32 sxevf_rx_max_frame_configure(struct sxevf_adapter *adapter, u32 mtu)
{
	s32 ret;
	struct sxevf_hw *hw = &adapter->hw;

	spin_lock_bh(&adapter->mbx_lock);
	ret = sxevf_rx_max_frame_set(hw, mtu);
	spin_unlock_bh(&adapter->mbx_lock);
	if (ret) {
		LOG_DEV_ERR("failed to set MTU at %u\n", mtu);
	}

	return ret;
}

void sxevf_hw_rx_configure(struct sxevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	sxevf_set_rx_mode(netdev);

	sxevf_vlan_restore(adapter);

	sxevf_max_used_rx_queue_configure(adapter);
	sxevf_rx_max_frame_configure(adapter, netdev->mtu);

	sxevf_rx_ring_configure(adapter);

	return;
}

s32 sxevf_rx_configure(struct sxevf_adapter *adapter)
{
	s32 ret;
	u16 queues;
	struct net_device *netdev = adapter->netdev;

	ret = sxevf_rx_resources_alloc(adapter);
	if (ret) {
		LOG_ERROR_BDF("allocation for Rx resources failed:Out of memory\n");
		goto l_ret;
	}

	sxevf_hw_rx_configure(adapter);

	queues = adapter->rx_ring_ctxt.num;
	ret = netif_set_real_num_rx_queues(netdev, queues);
	if (ret) {
		goto l_err_clean;
	}

	return 0;

l_err_clean:
	sxevf_rx_release(adapter);

l_ret:
	return ret;
}

static struct sxevf_rx_buffer *sxevf_rx_buffer_get(struct sxevf_ring *rx_ring,
					union sxevf_rx_data_desc *rx_desc,
					struct sk_buff **skb,
					const u32 size)
{
	struct sxevf_rx_buffer *rx_buffer;

	rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
	prefetchw(rx_buffer->page);

	dma_sync_single_range_for_cpu(rx_ring->dev,
				      rx_buffer->dma,
				      rx_buffer->page_offset,
				      size,
				      DMA_FROM_DEVICE);

	rx_buffer->pagecnt_bias--;

	return rx_buffer;
}

static void sxevf_rx_buffer_flip(struct sxevf_ring *rx_ring,
				 struct sxevf_rx_buffer *rx_buffer,
				 unsigned int size)
{
#if (PAGE_SIZE < SXEVF_PAGE_SIZE_8KB)
	u32 truesize = sxevf_rx_pg_size(rx_ring) / SXEVF_BUFFER_PER_4KPAGE;

	rx_buffer->page_offset ^= truesize;
#else
	u32 truesize = vf_ring_uses_build_skb(rx_ring) ?
				SKB_DATA_ALIGN(SXEVF_SKB_PAD + size) :
				SKB_DATA_ALIGN(size);

	rx_buffer->page_offset += truesize;
#endif
	return;
}

static void sxevf_rx_frag_add(struct sxevf_ring *rx_ring,
			      struct sxevf_rx_buffer *rx_buffer,
			      struct sk_buff *skb,
			      u32 size)
{
#if (PAGE_SIZE < SXEVF_PAGE_SIZE_8KB)
	u32 truesize = sxevf_rx_pg_size(rx_ring) / SXEVF_BUFFER_PER_4KPAGE;
#else
	u32 truesize = vf_ring_uses_build_skb(rx_ring) ?
				SKB_DATA_ALIGN(SXEVF_SKB_PAD + size) :
				SKB_DATA_ALIGN(size);
#endif
	skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, rx_buffer->page,
			rx_buffer->page_offset, size, truesize);
#if (PAGE_SIZE < 8192)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif
	return;
}

#ifndef HAVE_NET_PREFETCH_API
static inline void net_prefetch(void *data)
{
	prefetch(data);
#if L1_CACHE_BYTES < 128
	prefetch(data + L1_CACHE_BYTES);
#endif
}
#endif

#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
STATIC struct sk_buff *sxevf_skb_build(struct sxevf_ring *rx_ring,
				       struct sxevf_rx_buffer *rx_buffer,
				       struct xdp_buff *xdp,
				       union sxevf_rx_data_desc *rx_desc)
{
#ifdef HAVE_XDP_BUFF_DATA_META
	u32 metasize = xdp->data - xdp->data_meta;
#endif

#if (PAGE_SIZE < SXEVF_PAGE_SIZE_8KB)
	u32 truesize = sxevf_rx_pg_size(rx_ring) / SXEVF_BUFFER_PER_4KPAGE;
#else
	u32 truesize = SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
				SKB_DATA_ALIGN(xdp->data_end -
					       xdp->data_hard_start);
#endif
	struct sk_buff *skb;

#ifdef HAVE_XDP_BUFF_DATA_META
		net_prefetch(xdp->data_meta);
#else
		net_prefetch(xdp->data);
#endif

	skb = build_skb(xdp->data_hard_start, truesize);
	if (unlikely(!skb)) {
		goto l_ret;
	}

	skb_reserve(skb, xdp->data - xdp->data_hard_start);
	__skb_put(skb, xdp->data_end - xdp->data);
#ifdef HAVE_XDP_BUFF_DATA_META
	if (metasize) {
		skb_metadata_set(skb, metasize);
	}
#endif

#if (PAGE_SIZE < SXEVF_PAGE_SIZE_8KB)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif

l_ret:
	return skb;
}
#endif  

#ifdef ETH_GET_HEADLEN_API_NEED_2_PARAM
static inline u32
eth_get_headlen_compat(const struct net_device __always_unused *dev, void *data,
		     unsigned int len)
{
	return eth_get_headlen(data, len);
}

#define eth_get_headlen(dev, data, len) eth_get_headlen_compat(dev, data, len)
#endif

static struct sk_buff *sxevf_skb_construct(struct sxevf_ring *rx_ring,
					   struct sxevf_rx_buffer *rx_buffer,
					   struct xdp_buff *xdp,
					   union sxevf_rx_data_desc *rx_desc)
{
	u32 size = xdp->data_end - xdp->data;
#if (PAGE_SIZE < SXEVF_PAGE_SIZE_8KB)
	u32 truesize = sxevf_rx_pg_size(rx_ring) / SXEVF_BUFFER_PER_4KPAGE;
#else
	u32 truesize = SKB_DATA_ALIGN(xdp->data_end -
					       xdp->data_hard_start);
#endif
	struct sk_buff *skb;
	u32 headlen;

	net_prefetch(xdp->data);

	skb = napi_alloc_skb(&rx_ring->irq_data->napi, SXEVF_RX_HDR_SIZE);
	if (unlikely(!skb)) {
		goto l_ret;
	}

	headlen = size;
	if (headlen > SXEVF_RX_HDR_SIZE) {
		headlen = eth_get_headlen(skb->dev, xdp->data,
					  SXEVF_RX_HDR_SIZE);
	}

	memcpy(__skb_put(skb, headlen), xdp->data,
	       ALIGN(headlen, sizeof(long)));

	size -= headlen;
	if (size) {
		skb_add_rx_frag(skb, 0, rx_buffer->page,
				(xdp->data + headlen) -
				page_address(rx_buffer->page),
				size, truesize);
#if (PAGE_SIZE < 8192)
		rx_buffer->page_offset ^= truesize;
#else
		rx_buffer->page_offset += truesize;
#endif
	} else {
		rx_buffer->pagecnt_bias++;
	}

l_ret:
	return skb;
}

#ifndef HAVE_DEV_PAGE_IS_REUSABLE_API
static inline bool dev_page_is_reusable_compat(struct page *page)
{
	return likely(page_to_nid(page) == numa_mem_id() && \
					!page_is_pfmemalloc(page));
}
#define dev_page_is_reusable dev_page_is_reusable_compat
#endif

STATIC bool sxevf_is_rx_page_can_reuse(struct sxevf_rx_buffer *rx_buffer)
{
	u32 pagecnt_bias = rx_buffer->pagecnt_bias;
	struct page *page = rx_buffer->page;

	if (!dev_page_is_reusable(page)) {
		LOG_DEBUG("page[%p] can not reuse since it is reserved page, "
			"page_numa_id=%d, cpu_numa_id=%d, pfmemalloc:%s\n",
			page, page_to_nid(page), numa_mem_id(),
			page_is_pfmemalloc(page) ? "yes" : "no");
		goto l_false;
	}

#if (PAGE_SIZE < SXEVF_PAGE_SIZE_8KB)
	LOG_DEBUG("page[%p] (page_ref_count(page) - pagecnt_bias)=%d\n",
			page, (page_ref_count(page) - pagecnt_bias));
	if (unlikely((page_ref_count(page) - pagecnt_bias) > 1)) {
		LOG_DEBUG("page[%p] can not reuse page_ref=%d --- bias=%d\n",
			page, page_ref_count(page), pagecnt_bias);
		goto l_false;
	}
#else
#define SXEVF_LAST_OFFSET \
	(SKB_WITH_OVERHEAD(PAGE_SIZE) - SXEVF_RXBUFFER_2K)

	if (rx_buffer->page_offset > SXEVF_LAST_OFFSET) {
		LOG_DEBUG("page[%p] can not reuse rx_buffer->page_offset=%u > %u\n",
			page, rx_buffer->page_offset, SXEVF_LAST_OFFSET);
		goto l_false;
	}
#endif

	if (unlikely(!pagecnt_bias)) {
		page_ref_add(page, USHRT_MAX);
		rx_buffer->pagecnt_bias = USHRT_MAX;
	}

	return true;

l_false:
	return false;
}

static void sxevf_rx_page_reuse(struct sxevf_ring *rx_ring,
				struct sxevf_rx_buffer *old_buff)
{
	struct sxevf_rx_buffer *new_buff;
	u16 nta = rx_ring->next_to_alloc;

	new_buff = &rx_ring->rx_buffer_info[nta];

	nta++;
	rx_ring->next_to_alloc = (nta < rx_ring->depth) ? nta : 0;

	new_buff->dma		= old_buff->dma;
	new_buff->page		= old_buff->page;
	new_buff->page_offset	= old_buff->page_offset;
	new_buff->pagecnt_bias	= old_buff->pagecnt_bias;

	return;
}

static inline void sxevf_rx_page_release(struct sxevf_ring *rx_ring,
				struct sxevf_rx_buffer *rx_buffer,
				struct sk_buff *skb)
{
#ifdef HAVE_DMA_ATTRS_STRUCT
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif
	dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
					sxevf_rx_pg_size(rx_ring),
					DMA_FROM_DEVICE,
#ifdef HAVE_DMA_ATTRS_STRUCT
					&attrs);
#else
					SXEVF_RX_DMA_ATTR);
#endif

	__page_frag_cache_drain(rx_buffer->page,
				rx_buffer->pagecnt_bias);

	return;
}

static void sxevf_put_rx_buffer(struct sxevf_ring *rx_ring,
				struct sxevf_rx_buffer *rx_buffer,
				struct sk_buff *skb)
{
	if (sxevf_is_rx_page_can_reuse(rx_buffer)) {
		sxevf_rx_page_reuse(rx_ring, rx_buffer);
	} else {
		LOG_DEBUG("ring[%u], rx_buffer[%p]'s page[%p] can release\n",
			rx_ring->idx, rx_buffer, rx_buffer->page);
		sxevf_rx_page_release(rx_ring, rx_buffer, skb);
	}

	rx_buffer->page = NULL;

	return;
}

static bool sxevf_is_non_eop(struct sxevf_ring *rx_ring,
			     union sxevf_rx_data_desc *rx_desc,
			     struct sk_buff *skb)
{
	u32 ntc = rx_ring->next_to_clean + 1;

	ntc = (ntc < rx_ring->depth) ? ntc : 0;
	rx_ring->next_to_clean = ntc;
	LOG_DEBUG("next to clean:ntc[%u]\n", ntc);

	prefetch(SXEVF_RX_DESC(rx_ring, ntc));

	if (likely(sxevf_status_err_check(rx_desc, SXEVF_RXD_STAT_EOP))) {
		LOG_DEBUG("rx_desc[%p] the last packet arrived\n",rx_desc);
		goto l_false;
	}

	LOG_INFO("rx_desc[%p] in chain mode\n",rx_desc);
	return true;

l_false:
	return false;
}

static bool sxevf_headers_cleanup(struct sxevf_ring *rx_ring,
			   union sxevf_rx_data_desc *rx_desc,
			   struct sk_buff *skb)
{
	bool ret = false;

	if (IS_ERR(skb)) {
		ret = true;
		goto l_ret;
	}

	if (eth_skb_pad(skb)) {
		ret = true;
		goto l_ret;
	}

l_ret:
	return ret;
}

static inline void sxevf_rx_hash_set(struct sxevf_ring *ring,
				 union sxevf_rx_data_desc *rx_desc,
				 struct sk_buff *skb)
{
	u16 rss_type;

	if (!(ring->netdev->features & NETIF_F_RXHASH)) {
		goto l_end;
	}

	rss_type = le16_to_cpu(rx_desc->wb.lower.lo_dword.hs_rss.pkt_info) &
		   SXEVF_RXDADV_RSSTYPE_MASK;
	if (!rss_type) {
		goto l_end;
	}
	LOG_DEBUG("rss_type:0x%x, rss hash value:%u\n", rss_type,
				le32_to_cpu(rx_desc->wb.lower.hi_dword.rss));

	skb_set_hash(skb, le32_to_cpu(rx_desc->wb.lower.hi_dword.rss),
		     (SXEVF_RSS_L4_TYPES_MASK & (1ul << rss_type)) ?
		     PKT_HASH_TYPE_L4 : PKT_HASH_TYPE_L3);
l_end:
	return;
}

static void sxevf_skb_fields_process(struct sxevf_ring *rx_ring,
			      union sxevf_rx_data_desc *rx_desc,
			      struct sk_buff *skb)
{
	struct net_device *dev = rx_ring->netdev;

	sxevf_rx_hash_set(rx_ring, rx_desc, skb);

	sxevf_rx_csum_verify(rx_ring, rx_desc, skb);

	if (sxevf_status_err_check(rx_desc, SXEVF_RXD_STAT_VP)) {
		u16 vid = le16_to_cpu(rx_desc->wb.upper.vlan);
		struct sxevf_adapter *adapter = netdev_priv(rx_ring->netdev);
		unsigned long *active_vlans = adapter->active_vlans;
		LOG_DEBUG("rx vlan id=%u\n", vid);

		if (test_bit(vid & VLAN_VID_MASK, active_vlans)) {
			__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), vid);
		}
	}

#ifdef SXE_IPSEC_CONFIGURE
		sxevf_rx_ipsec_proc(rx_ring, rx_desc, skb);
#endif

	skb->protocol = eth_type_trans(skb, dev);

	return;
}

static void sxevf_rx_skb_deliver(struct sxevf_irq_data *irq_data,
		  struct sk_buff *skb)
{
	napi_gro_receive(&irq_data->napi, skb);
	return;
}

static inline void sxevf_rx_stats_update(struct sxevf_ring *rx_ring,
					u32 total_rx_bytes,
					u32 total_rx_packets)
{
	LOG_DEBUG("in the irq, process total packets[%u], bytes[%u]\n",
			  total_rx_packets, total_rx_bytes);

	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	u64_stats_update_end(&rx_ring->syncp);

	rx_ring->irq_data->rx.irq_rate.total_bytes += total_rx_bytes;
	rx_ring->irq_data->rx.irq_rate.total_packets += total_rx_packets;

	return;
}

static inline u32 sxevf_rx_frame_truesize(struct sxevf_ring *rx_ring,
					      unsigned int size)
{
	u32 truesize;

#if (PAGE_SIZE < SXEVF_PAGE_SIZE_8KB)
	truesize = sxevf_rx_pg_size(rx_ring) / SXEVF_BUFFER_PER_4KPAGE;
#else
	truesize = vf_ring_uses_build_skb(rx_ring) ?
		SKB_DATA_ALIGN(SXEVF_SKB_PAD + size)
#ifdef HAVE_XDP_BUFF_FRAME_SIZE
		+ SKB_DATA_ALIGN(sizeof(struct skb_shared_info))
#endif
		: SKB_DATA_ALIGN(size);
#endif
	return truesize;
}

u32 sxevf_rx_ring_irq_clean(struct sxevf_irq_data *irq_data,
			       struct sxevf_ring *rx_ring,
			       const u32 budget)
{

	u32 total_rx_bytes = 0;
	u32 total_rx_packets = 0;
	u32 rx_offset = sxevf_rx_offset(rx_ring);
	struct sxevf_adapter *adapter = irq_data->adapter;
	u16 cleaned_count = sxevf_desc_unused(rx_ring);
	struct sk_buff *skb = rx_ring->skb;
	bool xdp_xmit = false;
	struct xdp_buff xdp;

#ifdef HAVE_XDP_BUFF_FRAME_SIZE
	u32 frame_sz = 0;
#if (PAGE_SIZE < 8192)
	frame_sz = sxevf_rx_frame_truesize(rx_ring, 0);
#endif

#ifdef HAVE_XDP_BUFF_INIT_API
	xdp_init_buff(&xdp, frame_sz, &rx_ring->xdp_rxq);
#else
	xdp.frame_sz = frame_sz;
#endif
#endif 

#ifndef HAVE_NO_XDP_BUFF_RXQ
	xdp.rxq = &rx_ring->xdp_rxq;
#endif

	LOG_DEBUG("entry rx IRQ:irq=%u, ring_idx=%u, ring_reg_idx=%u, ring_tc_idx=%u,"
		"next_to_clean=%u, next_to_use=%u, budget=%u\n",
		irq_data->irq_idx, rx_ring->idx, rx_ring->reg_idx, rx_ring->tc_idx,
		rx_ring->next_to_clean, rx_ring->next_to_use, budget);

	while (likely(total_rx_packets < budget)) {
		union sxevf_rx_data_desc *rx_desc;
		struct sxevf_rx_buffer *rx_buffer;
		u32 size;

		if (cleaned_count >= SXEVF_RX_BUFFER_WRITE) {
			sxevf_rx_ring_buffers_alloc(rx_ring, cleaned_count);
			cleaned_count = 0;
		}

		rx_desc = SXEVF_RX_DESC(rx_ring, rx_ring->next_to_clean);
		size = le16_to_cpu(rx_desc->wb.upper.length);
		if (!size) {
			break;
		}

		LOG_DEBUG("process rx_desc[%u], write back info:"
				"status_error=0x%x, length=%u, vlan=%u\n",
				rx_ring->next_to_clean,
				le16_to_cpu(rx_desc->wb.upper.status_error),
				le16_to_cpu(rx_desc->wb.upper.length),
				le16_to_cpu(rx_desc->wb.upper.vlan));

		rmb();

		rx_buffer = sxevf_rx_buffer_get(rx_ring, rx_desc, &skb, size);

		if (!skb) {
#ifdef HAVE_XDP_PREPARE_BUFF_API
			u8 *hard_start = page_address(rx_buffer->page) +
				     rx_buffer->page_offset - rx_offset;
			xdp_prepare_buff(&xdp, hard_start, rx_offset, size, true);
#else
			xdp.data = page_address(rx_buffer->page) +
				   rx_buffer->page_offset;
#ifdef HAVE_XDP_BUFF_DATA_META
			xdp.data_meta = xdp.data;
#endif
			xdp.data_hard_start = xdp.data - rx_offset;
			xdp.data_end = xdp.data + size;
#endif

#ifdef HAVE_XDP_BUFF_FRAME_SIZE
#if (PAGE_SIZE > SXEVF_PAGE_SIZE_4KB)
			xdp.frame_sz = sxevf_rx_frame_truesize(rx_ring, size);
#endif
#endif
			skb = sxevf_xdp_run(adapter, rx_ring, &xdp);
		}

		if (IS_ERR(skb)) {
			unsigned int xdp_res = - PTR_ERR(skb);

			if (xdp_res == SXEVF_XDP_TX) {
				xdp_xmit = true;
				sxevf_rx_buffer_flip(rx_ring, rx_buffer, size);
			} else {
				rx_buffer->pagecnt_bias++;
			}
			total_rx_packets++;
			total_rx_bytes += size;
		} else if (skb) {
			sxevf_rx_frag_add(rx_ring, rx_buffer, skb, size);
#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
		} else if (vf_ring_uses_build_skb(rx_ring)) {
			skb = sxevf_skb_build(rx_ring, rx_buffer,
					      &xdp, rx_desc);
#endif
		} else {
			skb = sxevf_skb_construct(rx_ring, rx_buffer,
						  &xdp, rx_desc);
		}

		if (!skb) {
			LOG_INFO("ring[%u] rx_buffer[%p] skb is NULL,"
				"failed to process\n",rx_ring->idx, rx_buffer);
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			rx_buffer->pagecnt_bias++;
			break;
		}

		sxevf_put_rx_buffer(rx_ring, rx_buffer, skb);
		cleaned_count++;

		if (sxevf_is_non_eop(rx_ring, rx_desc, skb)) {
			continue;
		}

		if (sxevf_headers_cleanup(rx_ring, rx_desc, skb)) {
			skb = NULL;
			continue;
		}

		total_rx_bytes += skb->len;

		if ((skb->pkt_type == PACKET_BROADCAST ||
		    skb->pkt_type == PACKET_MULTICAST) &&
		    ether_addr_equal(rx_ring->netdev->dev_addr,
		    eth_hdr(skb)->h_source)) {
			LOG_WARN("in vepa mode, can not handle\n");
			dev_kfree_skb_irq(skb);
			continue;
		}

		sxevf_skb_fields_process(rx_ring, rx_desc, skb);

		SKB_DUMP(skb);

		total_rx_packets++;
		LOG_DEBUG("in loop[%u], process total bytes:%u\n",
			  total_rx_packets, skb->len);

		sxevf_rx_skb_deliver(irq_data, skb);

		skb = NULL;
	}

	rx_ring->skb = skb;

	if (xdp_xmit) {
		struct sxevf_ring *ring = adapter->xdp_ring_ctxt.ring[rx_ring->idx];

		wmb();
		writel(ring->next_to_use, ring->desc.tail);
	}

	sxevf_rx_stats_update(rx_ring, total_rx_bytes, total_rx_packets);

	return total_rx_packets;
}

void sxevf_hw_rx_disable(struct sxevf_adapter *adapter)
{
	u32 i;
	u8 reg_idx;
	struct sxevf_hw *hw = &adapter->hw;

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		reg_idx = adapter->rx_ring_ctxt.ring[i]->reg_idx;

		hw->dma.ops->rx_disable(hw, reg_idx);
	}

	return;
}
