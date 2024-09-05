#include <linux/etherdevice.h>
#include <linux/if_macvlan.h>
#include <linux/vmalloc.h>

#include "sxe.h"
#ifdef HAVE_NO_OVERFLOW_H
#include <sxe_compat_overflow.h>
#else
#include <linux/overflow.h>
#endif
#include "sxe_log.h"
#include "sxe_hw.h"
#include "sxe_rx_proc.h"
#include "sxe_netdev.h"
#include "sxe_pci.h"
#include "sxe_debug.h"
#include "sxe_csum.h"
#include "sxe_filter.h"
#include "sxe_ptp.h"
#include "sxe_ipsec.h"
#include "sxe_xdp.h"

#ifdef SXE_DRIVER_TRACE
#include "sxe_trace.h"
#endif

#ifdef NEED_SKB_FRAG_SIZE_API
#define skb_frag_size(frag)	skb_frag_size_compat(frag)
static inline unsigned int skb_frag_size_compat(const skb_frag_t *frag)
{
	return frag->size;
}
#endif 

#ifdef NEED_SKB_FRAG_OFF_API
#define skb_frag_off(frag)	skb_frag_off_compat(frag)
static inline unsigned int skb_frag_off_compat(const skb_frag_t *frag)
{
	return frag->page_offset;
}
#endif

#ifdef NEED_SKB_FRAG_OFF_ADD_API
#define skb_frag_off_add(frag, delta)	skb_frag_off_add_compat(frag, delta)
static inline void skb_frag_off_add_compat(skb_frag_t *frag, int delta)
{
	frag->page_offset += delta;
}
#endif

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

#ifndef HAVE_NO_XDP_BUFF_RXQ 
static u32 sxe_rx_napi_id_get(struct sxe_ring *rx_ring)
{
	struct sxe_irq_data *data = rx_ring->irq_data;

	return data ? data->napi.napi_id : 0;
}
#endif

STATIC int sxe_rx_ring_alloc(struct sxe_adapter *adapter,
					struct sxe_ring *ring)
{
	int    ret;
	u32    size;
	s32    ring_node   = NUMA_NO_NODE;
	struct device *dev = ring->dev;
	s32    dev_node    = dev_to_node(dev);

	size = sizeof(struct sxe_rx_buffer) * ring->depth;

	if (ring->irq_data) {
		ring_node = ring->irq_data->numa_node;
	}

	ring->rx_buffer_info = vmalloc_node(size, ring_node);
	if (NULL == ring->rx_buffer_info) {
		LOG_ERROR_BDF("ring[%u] can not alloc mem from local numa mode[%d]，"
			"try remote\n", ring->idx, ring_node);
		ring->rx_buffer_info = vmalloc(size);
		if (NULL == ring->rx_buffer_info) {
			LOG_ERROR_BDF("ring[%u] unable to allocate memory for the"
				"rx_buffer_info\n", ring->idx);
			ret = -ENOMEM;
			goto l_rx_buf_alloc_failed;
		}
	}


	ring->size = sizeof(union sxe_rx_data_desc) * ring->depth;
	ring->size = ALIGN(ring->size, ALIGN_4K);

	set_dev_node(dev, ring_node);
	ring->desc.base_addr = dma_alloc_coherent(dev,
				ring->size, &ring->desc.dma, GFP_KERNEL);
	set_dev_node(dev, dev_node);
	if (NULL == ring->desc.base_addr) {
		LOG_ERROR_BDF("ring[%u] unable to alloc base_addr memory from the "
			"same numa mode[%d] with ring, try dev node\n",
			ring->idx, ring_node);
		ring->desc.base_addr = dma_alloc_coherent(dev, ring->size,
						&ring->desc.dma, GFP_KERNEL);
		if (NULL == ring->desc.base_addr) {
			LOG_ERROR_BDF("ring[%u] unable to allocate memory for"
				"the descriptor\n", ring->idx);
			ret = -ENOMEM;
			goto l_base_addr_alloc_failed;
		}
	}

	ring->next_to_clean = 0;
	ring->next_to_use   = 0;

#ifndef HAVE_NO_XDP_BUFF_RXQ
	ret = xdp_rxq_info_reg(&ring->xdp_rxq, adapter->netdev,
			ring->idx, sxe_rx_napi_id_get(ring));
	if (ret < 0) {
		goto l_xdp_rxq_reg_failed;
	}
#endif

	ring->xdp_prog = adapter->xdp_prog;

	return 0;

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

void sxe_rx_ring_buffer_clean(struct sxe_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	struct sxe_rx_buffer *rx_buffer = &ring->rx_buffer_info[ntc];
	struct sk_buff *skb;
#ifdef HAVE_DMA_ATTRS_STRUCT
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif

	LOG_DEBUG("clean ring[%u] from ntc=%u, next_to_alloc=%u, rx_buffer[%p]\n",
		ring->idx, ntc, ring->next_to_alloc, rx_buffer);
#ifdef HAVE_AF_XDP_ZERO_COPY
	if (ring->xsk_pool) {
		sxe_xsk_rx_ring_clean(ring);
		goto l_skip_free;
	}
#endif
	while (ntc != ring->next_to_alloc) {
		if (rx_buffer->skb) {
			skb = rx_buffer->skb;
			if (SXE_CTRL_BUFFER(skb)->page_released) {
				dma_unmap_page_attrs(ring->dev,
						SXE_CTRL_BUFFER(skb)->dma,
						sxe_rx_pg_size(ring),
						DMA_FROM_DEVICE,
#ifdef HAVE_DMA_ATTRS_STRUCT
						&attrs);
#else
						SXE_RX_DMA_ATTR);
#endif
			}
			dev_kfree_skb(skb);
		}

		dma_sync_single_range_for_cpu(ring->dev,
						rx_buffer->dma,
						rx_buffer->page_offset,
						sxe_rx_bufsz(ring),
						DMA_FROM_DEVICE);

		dma_unmap_page_attrs(ring->dev, rx_buffer->dma,
					sxe_rx_pg_size(ring),
					DMA_FROM_DEVICE,
#ifdef HAVE_DMA_ATTRS_STRUCT
					&attrs);
#else
					SXE_RX_DMA_ATTR);
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
#ifdef HAVE_AF_XDP_ZERO_COPY
l_skip_free:
#endif
	ring->next_to_alloc = 0;
	ring->next_to_clean = 0;
	ring->next_to_use = 0;

	return;
}

void sxe_rx_ring_free(struct sxe_ring *ring)
{
	sxe_rx_ring_buffer_clean(ring);

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

static void sxe_rss_key_free(struct sxe_adapter *adapter)
{
	if (adapter->rss_key) {
		kfree(adapter->rss_key);
		adapter->rss_key = NULL;
	}

	return;
}

void sxe_rx_resources_free(struct sxe_adapter *adapter)
{
	u16 i;

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		if (adapter->rx_ring_ctxt.ring[i]->desc.base_addr) {
			sxe_rx_ring_free(adapter->rx_ring_ctxt.ring[i]);
		}
	}

	sxe_rss_key_free(adapter);

	return;
}

STATIC inline s32 sxe_rss_key_init(struct sxe_adapter *adapter)
{
	s32 ret;
	u32 *rss_key;

	if (!adapter->rss_key) {
		rss_key = kzalloc(SXE_RSS_KEY_SIZE, GFP_KERNEL);
		if (unlikely(!rss_key)) {
			ret = -ENOMEM;
			goto l_ret;
		}

		netdev_rss_key_fill(rss_key, SXE_RSS_KEY_SIZE);
		adapter->rss_key = rss_key;
	}

	return 0;
l_ret:
	return ret;
}

static int sxe_rx_resources_alloc(struct sxe_adapter *adapter)
{
	int ret;
	u16 i;

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		ret = sxe_rx_ring_alloc(adapter, adapter->rx_ring_ctxt.ring[i]);
		if (!ret) {
			continue;
		}

		LOG_MSG_ERR(probe, "allocation for Rx ring %u failed\n", i);
		goto l_err;
	}

	ret = sxe_rss_key_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("sxe_rss_key_init failed, out of memory\n");
		goto l_err;
	}

	return 0;

l_err:
	while (i--) {
		sxe_rx_ring_free(adapter->rx_ring_ctxt.ring[i]);
	}

	return ret;
}

s32 sxe_rx_ring_depth_reset(struct sxe_adapter *adapter, u32 rx_cnt)
{
	s32 ret;
	u32 i, rx_ring_cnt;
	struct sxe_ring *temp_ring;
	struct sxe_ring **rx_ring   = adapter->rx_ring_ctxt.ring;

	rx_ring_cnt = adapter->rx_ring_ctxt.num;
	temp_ring = vmalloc(array_size(rx_ring_cnt, sizeof(struct sxe_ring)));
	if (!temp_ring) {
		LOG_ERROR_BDF("vmalloc failed, size=%lu\n",
			array_size(rx_ring_cnt, sizeof(struct sxe_ring)));
		ret = -ENOMEM;
		goto l_end;
	}

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		memcpy(&temp_ring[i], rx_ring[i], sizeof(struct sxe_ring));

#ifndef HAVE_NO_XDP_BUFF_RXQ
		memset(&temp_ring[i].xdp_rxq, 0, sizeof(temp_ring[i].xdp_rxq));
#endif
		temp_ring[i].depth = rx_cnt;
		ret = sxe_rx_ring_alloc(adapter, &temp_ring[i]);
		if (ret) {
			LOG_ERROR_BDF("xdp ring alloc failed, rx ring idx=%d\n", i);
			goto l_rx_free;
		}
	}

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		sxe_rx_ring_free(rx_ring[i]);
		memcpy(rx_ring[i], &temp_ring[i], sizeof(struct sxe_ring));
	}

	adapter->rx_ring_ctxt.depth = rx_cnt;
	goto l_temp_free;

l_rx_free:
	while (i--) {
		sxe_rx_ring_free(&temp_ring[i]);
	}

l_temp_free:
	vfree(temp_ring);

l_end:
	return ret;
}

STATIC inline void sxe_redir_tbl_init(struct sxe_adapter *adapter)
{
	u32 i, j;
	u32 tbl_entries = sxe_rss_redir_tbl_size_get();
	u16 rss_num = sxe_rss_num_get(adapter);
	struct sxe_hw *hw = &adapter->hw;

	if ((adapter->cap & SXE_SRIOV_ENABLE) &&
			(rss_num < MIN_QUEUES_IN_SRIOV)) {
		LOG_INFO_BDF("sriov enabled and rss ring num = %u too less, "
			"adjust to %u\n", rss_num, MIN_QUEUES_IN_SRIOV);
		rss_num = MIN_QUEUES_IN_SRIOV;
	}

	memset(adapter->rss_indir_tbl, 0, sizeof(adapter->rss_indir_tbl));

	for (i = 0, j = 0; i < tbl_entries; i++, j++) {
		if (j == rss_num) {
			j = 0;
		}

		adapter->rss_indir_tbl[i] = j;
	}

	hw->dbu.ops->rss_redir_tbl_set_all(hw, adapter->rss_indir_tbl);

	return;
}

static inline void sxe_hw_rss_configure(struct sxe_adapter *adapter)
{
	u32 version = 0;
	struct sxe_hw *hw = &adapter->hw;
	u32 rss_num = sxe_rss_num_get(adapter);
	u16 pool = adapter->pool_f.pf_num_used;

	hw->dbu.ops->rss_key_set_all(hw, adapter->rss_key);

	hw->filter.mac.ops->rx_udp_frag_checksum_disable(hw);

	if (adapter->cap & SXE_RSS_FIELD_IPV4_UDP) {
		version = SXE_RSS_IP_VER_4;
	} else if(adapter->cap & SXE_RSS_FIELD_IPV6_UDP) {
		version = SXE_RSS_IP_VER_6;
	}
	hw->dbu.ops->rss_hash_pkt_type_set(hw, version);

	hw->dbu.ops->rss_rings_used_set(hw, rss_num, pool, PF_POOL_INDEX(0));

	sxe_redir_tbl_init(adapter);

	return;
}

static void sxe_lro_configure(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	bool is_enable = !!(adapter->cap & SXE_LRO_ENABLE);

	hw->dbu.ops->lro_ack_switch(hw, false);

	hw->dma.ops->rx_dma_lro_ctl_set(hw);

	if (is_enable) {
		hw->filter.mac.ops->rx_nfs_filter_disable(hw);
	}

	hw->filter.mac.ops->rx_lro_enable(hw, is_enable);

	return;
}

static u32 sxe_high_water_mark_get(struct sxe_adapter *adapter,
						u8 pkt_buf_idx)
{
	struct sxe_hw *hw = &adapter->hw;
	struct net_device *dev = adapter->netdev;
	int link, tc, kb, marker;
	u32 dv_id, rx_pba;
	u32 rx_pkt_buf_size;

	tc = link = dev->mtu + SXE_ETH_DEAD_LOAD + SXE_ETH_FRAMING;

	dv_id = SXE_DV(link, tc);

	if (adapter->cap & SXE_SRIOV_ENABLE) {
		dv_id += SXE_B2BT(tc);
	}

	kb = SXE_BT2KB(dv_id);
	rx_pkt_buf_size = hw->dbu.ops->rx_pkt_buf_size_get(hw, pkt_buf_idx);
	rx_pba =  rx_pkt_buf_size >> 10;

	marker = rx_pba - kb;

	if (marker < 0) {
		LOG_MSG_WARN(drv, "packet buffer(%i) can not provide enough"
			    "headroom to support flow control."
			    "decrease mtu or number of traffic classes\n",
			    pkt_buf_idx);
		marker = tc + 1;
	}

	return marker;
}

static u32 sxe_low_water_mark_get(struct sxe_adapter *adapter,
						u8 pkt_buf_idx)
{
	struct net_device *dev = adapter->netdev;
	int tc;
	u32 dv_id;

	tc = dev->mtu + SXE_ETH_DEAD_LOAD;

	dv_id = SXE_LOW_DV(tc);

	return SXE_BT2KB(dv_id);
}

static void sxe_link_fc_water_mark_get(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	int num_tc = sxe_dcb_tc_get(adapter);
	u32 i, high_mark, low_mark;

	if (!num_tc) {
		num_tc = 1;
	}

	for (i = 0; i < num_tc; i++) {
		high_mark = sxe_high_water_mark_get(adapter, i);
		low_mark  = sxe_low_water_mark_get(adapter, i);

		hw->mac.ops->fc_tc_high_water_mark_set(hw, i, high_mark);
		hw->mac.ops->fc_tc_low_water_mark_set(hw, i, low_mark);

		if (low_mark > high_mark) {
			hw->mac.ops->fc_tc_low_water_mark_set(hw, i, 0);
		}
	}

	for (; i < MAX_TRAFFIC_CLASS; i++) {
		hw->mac.ops->fc_tc_high_water_mark_set(hw, i, 0);
	}

	return;
}

static void sxe_hw_rx_buf_configure(struct sxe_adapter *adapter)
{
	u32 headroom;
	struct sxe_hw *hw = &adapter->hw;
	u8  tcs = sxe_dcb_tc_get(adapter);

	hw->dbu.ops->rx_pkt_buf_switch(hw, false);

	if (adapter->cap & SXE_FNAV_SAMPLE_ENABLE ||
	        adapter->cap & SXE_FNAV_SPECIFIC_ENABLE) {
		headroom = SXE_FNAV_RULES_TABLE_PKT_SIZE << adapter->fnav_ctxt.rules_table_size;
	} else {
		headroom = 0;
	}

	LOG_DEBUG_BDF("config pkg buf: tcs=%u, headroom=%u\n", tcs, headroom);
	hw->dbu.ops->rx_pkt_buf_size_configure(hw, tcs, headroom, PBA_STRATEGY_EQUAL);

	sxe_link_fc_water_mark_get(adapter);

	hw->dbu.ops->rx_pkt_buf_switch(hw, true);

	return;
}

static void sxe_rx_hw_dma_ctrl_init(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;

	hw->dbu.ops->rx_cap_switch_off(hw);

	hw->dma.ops->rx_dma_ctrl_init(hw);
	return;
}

static void sxe_ring_lro_configure(struct sxe_adapter *adapter,
					struct sxe_ring *ring)
{
	clear_ring_lro_enabled(ring);

	if (adapter->cap & SXE_LRO_ENABLE) {
		set_ring_lro_enabled(ring);
		LOG_INFO_BDF("ring[%u] lro enable\n", ring->idx);
	}
	return;
}

static void sxe_rx_buffer_size_set(struct sxe_adapter *adapter,
					struct sxe_ring *ring)
{
#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
#if (PAGE_SIZE < SXE_PAGE_SIZE_8KB)
	int max_frame = sxe_sw_mtu_get(adapter);
#endif
#endif

	clear_bit(SXE_RX_3K_BUFFER, &ring->state);
	clear_bit(SXE_RX_BUILD_SKB_ENABLED, &ring->state);

#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
	if (adapter->cap & SXE_RX_LEGACY) {
		goto l_end;
	}

	set_bit(SXE_RX_BUILD_SKB_ENABLED, &ring->state);

#if (PAGE_SIZE < SXE_PAGE_SIZE_8KB)
	if (adapter->cap & SXE_LRO_ENABLE) {
		set_bit(SXE_RX_3K_BUFFER, &ring->state);
	}

	if (SXE_2K_TOO_SMALL_WITH_PADDING ||
			(max_frame > (ETH_DATA_LEN + SXE_ETH_DEAD_LOAD))) {
		set_bit(SXE_RX_3K_BUFFER, &ring->state);
	}
#endif
#else
	adapter->cap |= SXE_RX_LEGACY;
#endif 

#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
l_end:
#endif
	return;
}

static void sxe_rx_ring_lro_configure(struct sxe_adapter *adapter,
				   struct sxe_ring *ring)
{
	struct sxe_hw *hw = &adapter->hw;
	u8 reg_idx = ring->reg_idx;

	if (!ring_is_lro_enabled(ring)) {
		return;
	} else {
		hw->dma.ops->rx_lro_ctl_configure(hw, reg_idx, SXE_LROCTL_MAXDESC_16);
	}

	return;
}

STATIC bool sxe_mapped_page_alloc(struct sxe_ring *rx_ring,
				    struct sxe_rx_buffer *rx_buf)
{
	bool ret;
	struct page *page = rx_buf->page;
	dma_addr_t dma;
	u16 order;
#ifdef HAVE_DMA_ATTRS_STRUCT
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif

	if (likely(page)) {
		ret = true;
		goto l_ret;
	}

	order = sxe_rx_pg_order(rx_ring);
	LOG_DEBUG("ring[%u] page order = %u\n", rx_ring->idx, order);

	page = dev_alloc_pages(order);
	if (unlikely(!page)) {
		LOG_DEBUG("ring[%u] page alloc failed\n", rx_ring->idx);
		rx_ring->rx_stats.alloc_rx_page_failed++;
		ret = false;
		goto l_ret;
	}

	dma = dma_map_page_attrs(rx_ring->dev, page, 0,
				 sxe_rx_pg_size(rx_ring),
				 DMA_FROM_DEVICE,
#ifdef HAVE_DMA_ATTRS_STRUCT
				 &attrs);
#else
				 SXE_RX_DMA_ATTR);
#endif

	if (dma_mapping_error(rx_ring->dev, dma)) {
		LOG_DEBUG("ring[%u] dma mapping failed\n", rx_ring->idx);
		__free_pages(page, order);

		rx_ring->rx_stats.alloc_rx_page_failed++;
		ret = false;
		goto l_ret;
	}

	rx_buf->dma = dma;
	rx_buf->page = page;
	rx_buf->page_offset = rx_ring->rx_offset;
	page_ref_add(page, USHRT_MAX - 1);
	rx_buf->pagecnt_bias = USHRT_MAX;
	rx_ring->rx_stats.alloc_rx_page++;

	LOG_DEBUG("ring[%u] mapped alloc succeed:page=%p, dma=%llu, "
		"page_offset=%u,ref_count=%d, pagecnt_bias=%u\n",
		 rx_ring->idx, page, rx_buf->dma, rx_buf->page_offset,
		 page_ref_count(page), rx_buf->pagecnt_bias);

	return true;

l_ret:
	return ret;
}

void sxe_rx_ring_buffers_alloc(struct sxe_ring *rx_ring, u16 cleaned_count)
{
	union sxe_rx_data_desc *rx_desc;
	struct sxe_rx_buffer *rx_buffer;
	u16 ntu = rx_ring->next_to_use;
	u16 bufsz;

	LOG_DEBUG("ring[%u][%p] entry, cleand_count=%u, next_to_use=%u\n",
			rx_ring->idx, rx_ring, cleaned_count, ntu);

	if (!cleaned_count) {
		return;
	}

	rx_desc = SXE_RX_DESC(rx_ring, ntu);
	rx_buffer = &rx_ring->rx_buffer_info[ntu];
	ntu -= rx_ring->depth;

	bufsz = sxe_rx_bufsz(rx_ring);

	do {
		if (!sxe_mapped_page_alloc(rx_ring, rx_buffer)) {
			LOG_DEBUG("ring[%u] page alloc failed, clean_count"
				"left:%u\n",rx_ring->idx, cleaned_count);
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
			rx_desc = SXE_RX_DESC(rx_ring, 0);
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

	LOG_DEBUG("ring[%u] quit, next_to_use=%u\n",
			rx_ring->idx, rx_ring->next_to_use);

	return;
}

s32 sxe_rss_hash_conf_get(struct sxe_adapter *adapter,
			    struct sxe_rss_hash_config *rss_conf)
{
	struct sxe_hw *hw = &adapter->hw;
	u8 *hash_key;
	u32 rss_field;
	u32 rss_key;
	u64 rss_hf;
	u16 i;

	hash_key = rss_conf->rss_key;
	if (hash_key != NULL) {
		for (i = 0; i < SXE_MAX_RSS_KEY_ENTRIES; i++) {
			rss_key = sxe_hw_rss_key_get_by_idx(hw, i);
			hash_key[(i * 4)] = rss_key & 0x000000FF;
			hash_key[(i * 4) + 1] = (rss_key >> 8) & 0x000000FF;
			hash_key[(i * 4) + 2] = (rss_key >> 16) & 0x000000FF;
			hash_key[(i * 4) + 3] = (rss_key >> 24) & 0x000000FF;
		}
	}

	rss_hf = 0;
	rss_field = sxe_hw_rss_field_get(hw);
	
	if (rss_field & SXE_MRQC_RSS_FIELD_IPV4) {
		rss_hf |= ETH_RSS_IPV4;
	}

	if (rss_field & SXE_MRQC_RSS_FIELD_IPV4_TCP) {
		rss_hf |= ETH_RSS_NONFRAG_IPV4_TCP;
	}

	if (rss_field & SXE_MRQC_RSS_FIELD_IPV4_UDP) {
		rss_hf |= ETH_RSS_NONFRAG_IPV4_UDP;
	}

	if (rss_field & SXE_MRQC_RSS_FIELD_IPV6) {
		rss_hf |= ETH_RSS_IPV6;
	}

	if (rss_field & SXE_MRQC_RSS_FIELD_IPV6_TCP) {
		rss_hf |= ETH_RSS_NONFRAG_IPV6_TCP;
	}

	if (rss_field & SXE_MRQC_RSS_FIELD_IPV6_UDP) {
		rss_hf |= ETH_RSS_NONFRAG_IPV6_UDP;
	}

	LOG_DEBUG("got rss hash func=%lld\n", rss_hf);
	rss_conf->rss_hf = rss_hf;

	return 0;
}

#ifdef HAVE_AF_XDP_ZERO_COPY
static void sxe_hw_rx_buffer_configure(struct sxe_hw *hw,
				u8 reg_idx, unsigned long state,
				bool is_xsk_on, u32 xsk_buf_len)
#else
static void sxe_hw_rx_buffer_configure(struct sxe_hw *hw,
				u8 reg_idx, unsigned long state)
#endif
{
	u32 pkg_buf_len;
#ifdef HAVE_AF_XDP_ZERO_COPY
	if (is_xsk_on) {
		pkg_buf_len = xsk_buf_len;
	} else if (test_bit(SXE_RX_3K_BUFFER, &state)) {
#else
	if (test_bit(SXE_RX_3K_BUFFER, &state)) {
#endif
		pkg_buf_len = SXE_RXBUFFER_3K;
	} else {
		pkg_buf_len = SXE_RXBUFFER_2K;
	}

	hw->dma.ops->rx_rcv_ctl_configure(hw, reg_idx,
				SXE_RX_HDR_SIZE, pkg_buf_len);

	return;
}

static void sxe_rx_ring_reg_configure(struct sxe_adapter *adapter,
				struct sxe_ring *ring)
{
	struct sxe_hw *hw = &adapter->hw;
	u64 desc_dma_addr = ring->desc.dma;
	u8 reg_idx = ring->reg_idx;
#ifdef HAVE_AF_XDP_ZERO_COPY
	u32 xsk_buf_len = 0;
#endif
	u32 desc_mem_len;

	hw->dma.ops->rx_ring_switch(hw, reg_idx, false);

	desc_mem_len = ring->depth * sizeof(union sxe_rx_data_desc);
	hw->dma.ops->rx_ring_desc_configure(hw, desc_mem_len, desc_dma_addr, reg_idx);

	ring->desc.tail = adapter->hw.reg_base_addr + SXE_RDT(reg_idx);

#ifdef HAVE_AF_XDP_ZERO_COPY
	if (ring->xsk_pool) {
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
		xsk_buf_len = xsk_pool_get_rx_frame_size(ring->xsk_pool);
#else
		xsk_buf_len = ring->xsk_pool->chunk_size_nohr -
						  XDP_PACKET_HEADROOM;
#endif
		ring->rx_buf_len = xsk_buf_len;
	}
#endif

#ifdef HAVE_AF_XDP_ZERO_COPY
	sxe_hw_rx_buffer_configure(hw, reg_idx, ring->state,
					!!ring->xsk_pool, xsk_buf_len);
#else
	sxe_hw_rx_buffer_configure(hw, reg_idx, ring->state);
#endif

	sxe_rx_ring_lro_configure(adapter, ring);

	hw->dma.ops->rx_desc_thresh_set(hw, reg_idx);

	hw->dma.ops->rx_ring_switch(hw, reg_idx, true);

	return;
}

#ifdef HAVE_AF_XDP_ZERO_COPY
static inline void sxe_rx_ring_xdp_mem_mode_init(
					struct sxe_adapter *adapter,
					struct sxe_ring *ring)
{
	xdp_rxq_info_unreg_mem_model(&ring->xdp_rxq);
	ring->xsk_pool = sxe_xsk_pool_get(adapter, ring);
	if (ring->xsk_pool) {
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
		WARN_ON(xdp_rxq_info_reg_mem_model(&ring->xdp_rxq,
						   MEM_TYPE_XSK_BUFF_POOL,
						   NULL));
		xsk_pool_set_rxq_info(ring->xsk_pool, &ring->xdp_rxq);
#else
		ring->zca.free = sxe_zca_free;
		WARN_ON(xdp_rxq_info_reg_mem_model(&ring->xdp_rxq,
						   MEM_TYPE_ZERO_COPY,
						   &ring->zca));
#endif
	} else {
		WARN_ON(xdp_rxq_info_reg_mem_model(&ring->xdp_rxq,
						   MEM_TYPE_PAGE_SHARED, NULL));
	}

	return;
}
#endif
static inline void sxe_rx_desc_init(struct sxe_ring *rx_ring)
{
	union sxe_rx_data_desc *desc;

	desc = SXE_RX_DESC(rx_ring, 0);
	desc->wb.upper.length = 0;

	return;
}

static inline u32 sxe_rx_offset(struct sxe_ring *rx_ring)
{
	return ring_uses_build_skb(rx_ring) ? SXE_SKB_PAD : 0;
}

static inline void sxe_rx_offset_init(struct sxe_ring *rx_ring)
{
	rx_ring->rx_offset = sxe_rx_offset(rx_ring);

	return;
}

void sxe_rx_ring_attr_configure(struct sxe_adapter *adapter,
					struct sxe_ring *ring)
{
#ifdef HAVE_AF_XDP_ZERO_COPY
	sxe_rx_ring_xdp_mem_mode_init(adapter, ring);
#endif

	sxe_ring_lro_configure(adapter, ring);

	sxe_rx_buffer_size_set(adapter, ring);

	sxe_rx_ring_reg_configure(adapter, ring);
	sxe_rx_ring_buffer_init(ring);
	sxe_rx_desc_init(ring);
	sxe_rx_offset_init(ring);
#ifdef HAVE_AF_XDP_ZERO_COPY
	if (ring->xsk_pool) {
		sxe_zc_rx_ring_buffers_alloc(ring, sxe_desc_unused(ring));
	} else {
		sxe_rx_ring_buffers_alloc(ring, sxe_desc_unused(ring));
	}
#else
	sxe_rx_ring_buffers_alloc(ring, sxe_desc_unused(ring));
#endif
	return;
}

static void sxe_rx_ring_configure(struct sxe_adapter *adapter)
{
	u32 i;
	u8 tcs = sxe_dcb_tc_get(adapter);
	struct sxe_hw *hw = &adapter->hw;
	u16 mask = sxe_pool_mask_get(adapter);
	bool enable = !!(adapter->cap & SXE_SRIOV_ENABLE);
	bool is_4Q;
	struct sxe_ring **ring = adapter->rx_ring_ctxt.ring;

	if (mask == SXE_4Q_PER_POOL_MASK) {
		is_4Q = true;
	} else {
		is_4Q = false;
	}

	hw->dbu.ops->rx_multi_ring_configure(hw, tcs, is_4Q, enable);

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		sxe_rx_ring_attr_configure(adapter, ring[i]);
	}

	return;
}

static void sxe_vlan_restore(struct sxe_adapter *adapter)
{
	u16 vid = 1;

	sxe_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q), 0);

	for_each_set_bit_from(vid, adapter->vlan_ctxt.active_vlans, VLAN_N_VID)
		sxe_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q), vid);

	return;
}

static void sxe_fnav_configure(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;

	if (adapter->cap & SXE_FNAV_SAMPLE_ENABLE) {
		hash_init(adapter->fnav_ctxt.sample_list);
		adapter->fnav_ctxt.is_sample_table_overflowed = false;
		hw->dbu.ops->fnav_mode_init(hw,
				adapter->fnav_ctxt.rules_table_size,
				SXE_FNAV_SAMPLE_MODE);
	} else if (adapter->cap & SXE_FNAV_SPECIFIC_ENABLE) {
		hw->dbu.ops->fnav_mode_init(hw,
				adapter->fnav_ctxt.rules_table_size,
				SXE_FNAV_SPECIFIC_MODE);
		sxe_fnav_rules_restore(adapter);
	}

	return;
}

void sxe_hw_rx_configure(struct sxe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	sxe_fnav_configure(adapter);

	sxe_hw_rx_buf_configure(adapter);

	__sxe_set_rx_mode(netdev, true);

	sxe_vlan_restore(adapter);

	sxe_rx_hw_dma_ctrl_init(adapter);

	sxe_hw_rss_configure(adapter);

	sxe_lro_configure(adapter);

	sxe_rx_ring_configure(adapter);

	return;
}

s32 sxe_test_rx_configure(struct sxe_adapter *adapter, struct sxe_ring *ring)
{
	s32 ret;
	u32 reg_data;
	union sxe_rx_data_desc *desc;
	struct sxe_hw *hw = &adapter->hw;

	ring->depth   = SXE_DEFAULT_DESC_CNT;
	ring->idx     = 0;
	ring->dev     = &adapter->pdev->dev;
	ring->netdev  = adapter->netdev;
	ring->reg_idx = adapter->rx_ring_ctxt.ring[0]->reg_idx;

	ret = sxe_rx_ring_alloc(adapter, ring);
	if (ret) {
		goto l_end;
	}

	sxe_hw_rx_buf_configure(adapter);

	hw->dbu.ops->rx_cap_switch_off(hw);

	desc = SXE_RX_DESC(ring, 0);
	desc->wb.upper.length = 0;

	sxe_rx_buffer_size_set(adapter, ring);

	sxe_rx_ring_reg_configure(adapter, ring);
	sxe_rx_ring_buffer_init(ring);
	sxe_rx_ring_buffers_alloc(ring, sxe_desc_unused(ring));

	reg_data = hw->filter.mac.ops->rx_mode_get(hw);
	reg_data |= SXE_FCTRL_BAM | SXE_FCTRL_SBP | SXE_FCTRL_MPE;
	hw->filter.mac.ops->rx_mode_set(hw, reg_data);

	hw->dbu.ops->rx_cap_switch_on(hw);

l_end:
	return ret;
}

s32 sxe_rx_configure(struct sxe_adapter *adapter)
{
	s32 ret;
	u16 queues;
	struct net_device *netdev = adapter->netdev;

	ret = sxe_rx_resources_alloc(adapter);
	if (ret) {
		LOG_ERROR_BDF("allocation for Rx resources failed:Out of memory\n");
		goto l_ret;
	}

	sxe_hw_rx_configure(adapter);

	queues = adapter->rx_ring_ctxt.num;
	ret = netif_set_real_num_rx_queues(netdev, queues);
	if (ret) {
		goto l_err_clean;
	}

	return 0;

l_err_clean:
	sxe_rx_release(adapter);

l_ret:
	return ret;
}

static void sxe_rx_dma_sync(struct sxe_ring *rx_ring,
				struct sk_buff *skb)
{
#ifdef HAVE_DMA_ATTRS_STRUCT
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif

	if (ring_uses_build_skb(rx_ring)) {
		unsigned long mask = (unsigned long)sxe_rx_pg_size(rx_ring) - 1;
		unsigned long offset = (unsigned long)(skb->data) & mask;

		dma_sync_single_range_for_cpu(rx_ring->dev,
						SXE_CTRL_BUFFER(skb)->dma,
						offset,
						skb_headlen(skb),
						DMA_FROM_DEVICE);
	} else {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[0];

		dma_sync_single_range_for_cpu(rx_ring->dev,
					SXE_CTRL_BUFFER(skb)->dma,
					skb_frag_off(frag),
					skb_frag_size(frag),
					DMA_FROM_DEVICE);
	}

	if (unlikely(SXE_CTRL_BUFFER(skb)->page_released)) {
		dma_unmap_page_attrs(rx_ring->dev, SXE_CTRL_BUFFER(skb)->dma,
					sxe_rx_pg_size(rx_ring),
					DMA_FROM_DEVICE,
#ifdef HAVE_DMA_ATTRS_STRUCT
					&attrs);
#else
					SXE_RX_DMA_ATTR);
#endif
	}

	return;
}

static struct sxe_rx_buffer *sxe_rx_buffer_get(struct sxe_ring *rx_ring,
					union sxe_rx_data_desc *rx_desc,
					struct sk_buff **skb,
					const u32 size,
					s32 *rx_buffer_pgcnt)
{
	struct sxe_rx_buffer *rx_buffer;
	bool is_end_pkt = !!sxe_status_err_check(rx_desc, SXE_RXD_STAT_EOP);

	rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];

	*rx_buffer_pgcnt =
#if (PAGE_SIZE < SXE_PAGE_SIZE_8KB)
		page_count(rx_buffer->page);
#else
		0;
#endif

	prefetchw(rx_buffer->page);
	*skb = rx_buffer->skb;

	if (!is_end_pkt) {
		if (!*skb) {
			LOG_DEBUG("start a chain or lro rcv on ring[%u]\n",
				rx_ring->idx);
			goto l_skip_sync;
		}
	} else {
		if (*skb) {
			sxe_rx_dma_sync(rx_ring, *skb);
		}
	}

	dma_sync_single_range_for_cpu(rx_ring->dev,
				      rx_buffer->dma,
				      rx_buffer->page_offset,
				      size,
				      DMA_FROM_DEVICE);
l_skip_sync:
	rx_buffer->pagecnt_bias--;

	return rx_buffer;
}

static void sxe_add_rx_frag_to_skb(struct sxe_ring *rx_ring,
			      struct sxe_rx_buffer *rx_buffer,
			      struct sk_buff *skb,
			      u32 size)
{
#if (PAGE_SIZE < SXE_PAGE_SIZE_8KB)
	u32 truesize = sxe_rx_pg_size(rx_ring) / 2;
#else
	u32 truesize = ring_uses_build_skb(rx_ring) ?
				SKB_DATA_ALIGN(rx_ring->rx_offset + size) :
				SKB_DATA_ALIGN(size);
#endif
	LOG_DEBUG("rx_ring[%u] add data in page[%p], offset[%u], size"
			  "to skb frag[%u],use truesize[%u]\n",
			  rx_ring->idx, rx_buffer->page,
			  rx_buffer->page_offset, size, truesize);
	skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, rx_buffer->page,
			rx_buffer->page_offset, size, truesize);
#if (PAGE_SIZE < SXE_PAGE_SIZE_8KB)
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
STATIC struct sk_buff *sxe_skb_build(struct sxe_ring *rx_ring,
				       struct sxe_rx_buffer *rx_buffer,
				       struct xdp_buff *xdp,
				       union sxe_rx_data_desc *rx_desc)
{
#ifdef HAVE_XDP_BUFF_DATA_META
	u32 metasize = xdp->data - xdp->data_meta;
#endif

#if (PAGE_SIZE < SXE_PAGE_SIZE_8KB)
	u32 truesize = sxe_rx_pg_size(rx_ring) / 2;
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

	if (!sxe_status_err_check(rx_desc, SXE_RXD_STAT_EOP)) {
		SXE_CTRL_BUFFER(skb)->dma = rx_buffer->dma;
	}

#if (PAGE_SIZE < SXE_PAGE_SIZE_8KB)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif

l_ret:
	return skb;
}
#endif  

STATIC struct sk_buff *sxe_skb_construct(struct sxe_ring *rx_ring,
					   struct sxe_rx_buffer *rx_buffer,
					   struct xdp_buff *xdp,
					   union sxe_rx_data_desc *rx_desc)
{
	u32 size = xdp->data_end - xdp->data;
#if (PAGE_SIZE < SXE_PAGE_SIZE_8KB)
	u32 truesize = sxe_rx_pg_size(rx_ring) / 2;
#else
	u32 truesize = SKB_DATA_ALIGN(xdp->data_end -
					       xdp->data_hard_start);
#endif
	struct sk_buff *skb;

	net_prefetch(xdp->data);


	skb = napi_alloc_skb(&rx_ring->irq_data->napi, SXE_RX_HDR_SIZE);
	if (unlikely(!skb)) {
		goto l_ret;
	}

	if (size > SXE_RX_HDR_SIZE) {
		if (!sxe_status_err_check(rx_desc, SXE_RXD_STAT_EOP)) {
			SXE_CTRL_BUFFER(skb)->dma = rx_buffer->dma;
		}

		skb_add_rx_frag(skb, 0, rx_buffer->page,
				xdp->data - page_address(rx_buffer->page),
				size, truesize);
#if (PAGE_SIZE < SXE_PAGE_SIZE_8KB)
		rx_buffer->page_offset ^= truesize;
#else
		rx_buffer->page_offset += truesize;
#endif
	} else {
		memcpy(__skb_put(skb, size),
			xdp->data, ALIGN(size, sizeof(long)));
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
STATIC bool sxe_is_rx_page_can_reuse(struct sxe_rx_buffer *rx_buffer,
					s32 rx_buffer_pgcnt)
{
	u16 pagecnt_bias = rx_buffer->pagecnt_bias;
	struct page *page = rx_buffer->page;

	if (!dev_page_is_reusable(page)) {
		LOG_INFO("page[%p] can not reuse since it is reserved page, "
			"page_numa_id=%d, cpu_numa_id=%d, pfmemalloc:%s\n",
			page, page_to_nid(page), numa_mem_id(),
			page_is_pfmemalloc(page) ? "yes" : "no");
		goto l_false;
	}

#if (PAGE_SIZE < SXE_PAGE_SIZE_8KB)
	if (unlikely((rx_buffer_pgcnt - pagecnt_bias) > 1)) {
		LOG_INFO("page[%p] can not reuse page_ref=%d --- bias=%d\n",
			page, rx_buffer_pgcnt, pagecnt_bias);
		goto l_false;
	}
#else

#define SXE_LAST_OFFSET \
	(SKB_WITH_OVERHEAD(PAGE_SIZE) - SXE_RXBUFFER_3K)
	if (rx_buffer->page_offset > SXE_LAST_OFFSET) {
		LOG_INFO("page[%p] can not reuse rx_buffer->page_offset:%u > %u\n",
			page, rx_buffer->page_offset, SXE_LAST_OFFSET);
		goto l_false;
	}
#endif

	if (unlikely(pagecnt_bias == 1)) {
		page_ref_add(page, USHRT_MAX - 1);
		rx_buffer->pagecnt_bias = USHRT_MAX;
	}

	return true;

l_false:
	return false;
}

static void sxe_rx_page_reuse(struct sxe_ring *rx_ring,
				struct sxe_rx_buffer *old_buff)
{
	struct sxe_rx_buffer *new_buff;
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

static inline void sxe_rx_page_release(struct sxe_ring *rx_ring,
				struct sxe_rx_buffer *rx_buffer,
				struct sk_buff *skb)
{
#ifdef HAVE_DMA_ATTRS_STRUCT
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif
	if (!IS_ERR(skb) && SXE_CTRL_BUFFER(skb)->dma == rx_buffer->dma) {
		SXE_CTRL_BUFFER(skb)->page_released = true;
	} else {
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
					sxe_rx_pg_size(rx_ring),
					DMA_FROM_DEVICE,
#ifdef HAVE_DMA_ATTRS_STRUCT
					&attrs);
#else
					SXE_RX_DMA_ATTR);
#endif
	}

	__page_frag_cache_drain(rx_buffer->page,
				rx_buffer->pagecnt_bias);

	return;
}

static void sxe_rx_buffer_put(struct sxe_ring *rx_ring,
				struct sxe_rx_buffer *rx_buffer,
				struct sk_buff *skb,
				s32 rx_buffer_pgcnt)
{
	if (sxe_is_rx_page_can_reuse(rx_buffer, rx_buffer_pgcnt)) {
		sxe_rx_page_reuse(rx_ring, rx_buffer);
	} else {
		LOG_DEBUG("ring[%u], rx_buffer[%p]'s page[%p] can release\n",
			rx_ring->idx, rx_buffer, rx_buffer->page);
		sxe_rx_page_release(rx_ring, rx_buffer, skb);
	}

	rx_buffer->page = NULL;
	rx_buffer->skb = NULL;

	return;
}

static bool sxe_is_non_eop(struct sxe_ring *rx_ring,
			     union sxe_rx_data_desc *rx_desc,
			     struct sk_buff *skb)
{
	u32 ntc = rx_ring->next_to_clean + 1;

	ntc = (ntc < rx_ring->depth) ? ntc : 0;
	rx_ring->next_to_clean = ntc;
	LOG_DEBUG("next to clean:ntc[%u]\n", ntc);

	prefetch(SXE_RX_DESC(rx_ring, ntc));

	if (ring_is_lro_enabled(rx_ring)) {
		__le32 lro_enabled = rx_desc->wb.lower.lo_dword.data &
				     cpu_to_le32(SXE_RXDADV_LROCNT_MASK);
		if (unlikely(lro_enabled)) {
			u32 lro_cnt = le32_to_cpu(lro_enabled);

			lro_cnt >>= SXE_RXDADV_LROCNT_SHIFT;
			SXE_CTRL_BUFFER(skb)->lro_cnt += lro_cnt - 1;

			ntc = le32_to_cpu(rx_desc->wb.upper.status_error);
			ntc &= SXE_RXDADV_NEXTP_MASK;
			ntc >>= SXE_RXDADV_NEXTP_SHIFT;
			LOG_DEBUG("enter lro static lro_cnt=%u, ntc =%u\n",
				lro_cnt, ntc);
		}
	}

	if (likely(sxe_status_err_check(rx_desc, SXE_RXD_STAT_EOP))) {
		LOG_DEBUG("the rx_desc[%p]'s last packet arrived\n", rx_desc);
		goto l_false;
	}

	rx_ring->rx_buffer_info[ntc].skb = skb;
	rx_ring->rx_stats.non_eop_descs++;
	LOG_DEBUG("in chain mode, ntc[%u]\n", ntc);

	return true;

l_false:
	return false;
}

#ifdef ETH_GET_HEADLEN_API_NEED_2_PARAM
static inline u32
eth_get_headlen_compat(const struct net_device __always_unused *dev, void *data,
		     unsigned int len)
{
	return eth_get_headlen(data, len);
}

#define eth_get_headlen(dev, data, len) eth_get_headlen_compat(dev, data, len)
#endif
static void sxe_tail_pull(struct sxe_ring *rx_ring,
			    struct sk_buff *skb)
{
	skb_frag_t *frag = &skb_shinfo(skb)->frags[0];
	unsigned char *va;
	u32 pull_len;

	va = skb_frag_address(frag);

	pull_len = eth_get_headlen(skb->dev, va, SXE_RX_HDR_SIZE);

	skb_copy_to_linear_data(skb, va, ALIGN(pull_len, sizeof(long)));

	skb_frag_size_sub(frag, pull_len);
	skb_frag_off_add(frag, pull_len);

	skb->data_len -= pull_len;
	skb->tail += pull_len;

	return;
}

bool sxe_headers_cleanup(struct sxe_ring *rx_ring,
			   union sxe_rx_data_desc *rx_desc,
			   struct sk_buff *skb)
{
	bool ret = false;

	if (IS_ERR(skb)) {
		ret = true;
		goto l_ret;
	}

	if (!skb_headlen(skb)) {
		LOG_DEBUG("ring[%u] place header in linear portion in skb\n",
				rx_ring->idx);
		sxe_tail_pull(rx_ring, skb);
	}

	if (eth_skb_pad(skb)) {
		ret = true;
		goto l_ret;
	}

l_ret:
	return ret;
}

static void sxe_lro_gso_size_set(struct sxe_ring *ring,
				   struct sk_buff *skb)
{
	u16 hdr_len = skb_headlen(skb);

	skb_shinfo(skb)->gso_size = DIV_ROUND_UP((skb->len - hdr_len),
					SXE_CTRL_BUFFER(skb)->lro_cnt);
	skb_shinfo(skb)->gso_type = SKB_GSO_TCPV4;

	return;
}

static void sxe_lro_stats_update(struct sxe_ring *rx_ring,
				   struct sk_buff *skb)
{
	if (!SXE_CTRL_BUFFER(skb)->lro_cnt) {
		goto l_end;
	}

	rx_ring->rx_stats.lro_count += SXE_CTRL_BUFFER(skb)->lro_cnt;
	rx_ring->rx_stats.lro_flush++;

	LOG_DEBUG("ring[%u], lro_count=%llu, lro_flush=%llu\n", rx_ring->idx,
		rx_ring->rx_stats.lro_count, rx_ring->rx_stats.lro_flush);
	sxe_lro_gso_size_set(rx_ring, skb);

	SXE_CTRL_BUFFER(skb)->lro_cnt = 0;

l_end:
	return;
}

static inline void sxe_rx_hash_set(struct sxe_ring *ring,
				 union sxe_rx_data_desc *rx_desc,
				 struct sk_buff *skb)
{
	u16 rss_type;

	if (!(ring->netdev->features & NETIF_F_RXHASH)) {
		goto l_end;
	}

	rss_type = le16_to_cpu(rx_desc->wb.lower.lo_dword.hs_rss.pkt_info) &
		   SXE_RXDADV_RSSTYPE_MASK;
	if (!rss_type) {
		goto l_end;
	}
	LOG_DEBUG("rss_type:%u, rss hash value:%u\n", rss_type,
				le32_to_cpu(rx_desc->wb.lower.hi_dword.rss));

	skb_set_hash(skb, le32_to_cpu(rx_desc->wb.lower.hi_dword.rss),
		     (SXE_RSS_L4_TYPES_MASK & (1ul << rss_type)) ?
		     PKT_HASH_TYPE_L4 : PKT_HASH_TYPE_L3);
l_end:
	return;
}

void sxe_skb_fields_process(struct sxe_ring *rx_ring,
			      union sxe_rx_data_desc *rx_desc,
			      struct sk_buff *skb)
{
	struct net_device *dev = rx_ring->netdev;
	u32 cap = rx_ring->irq_data->adapter->cap;

	sxe_rx_hash_set(rx_ring, rx_desc, skb);

	sxe_lro_stats_update(rx_ring, skb);

	sxe_rx_csum_verify(rx_ring, rx_desc, skb);

	if ((dev->features & NETIF_F_HW_VLAN_CTAG_RX) &&
	    sxe_status_err_check(rx_desc, SXE_RXD_STAT_VP)) {
		u16 vid = le16_to_cpu(rx_desc->wb.upper.vlan);
		LOG_DEBUG("rx vlan id=%u\n", vid);
		__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), vid);
	}

	if (unlikely(cap & SXE_RX_HWTSTAMP_ENABLED)) {
		sxe_ptp_rx_hwtstamp_process(rx_ring, rx_desc, skb);
	}

#ifdef SXE_IPSEC_CONFIGURE
	sxe_rx_ipsec_proc(rx_ring, rx_desc, skb);
#endif

	if (netif_is_sxe(dev)) {
		skb_record_rx_queue(skb, rx_ring->idx);
	} else {
		macvlan_count_rx(netdev_priv(dev), skb->len + ETH_HLEN, true,
			 false);
	}

	skb->protocol = eth_type_trans(skb, dev);

	return;
}

void sxe_rx_skb_deliver(struct sxe_irq_data *irq_data,
		  struct sk_buff *skb)
{
	napi_gro_receive(&irq_data->napi, skb);
	return;
}

static inline u32 sxe_rx_frame_truesize(struct sxe_ring *rx_ring, u32 size)
{
	u32 truesize;

#if (PAGE_SIZE < 8192)
	truesize = sxe_rx_pg_size(rx_ring) / 2;
#else
	truesize = rx_ring->rx_offset ?
		SKB_DATA_ALIGN(rx_ring->rx_offset + size)
#ifdef HAVE_XDP_BUFF_FRAME_SIZE
		+ SKB_DATA_ALIGN(sizeof(struct skb_shared_info))
#endif
		: SKB_DATA_ALIGN(size);
#endif
	return truesize;
}

void sxe_rx_buffer_page_offset_update(struct sxe_ring *rx_ring,
				 struct sxe_rx_buffer *rx_buffer,
				 u32 size)
{
	u32 truesize = sxe_rx_frame_truesize(rx_ring, size);

#if (PAGE_SIZE < SXE_PAGE_SIZE_8KB)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif

	return;
}

u32 sxe_rx_ring_irq_clean(struct sxe_irq_data *irq_data,
			       struct sxe_ring *rx_ring,
			       const u32 budget)
{
	u32 total_rx_bytes = 0;
	u32 total_rx_packets = 0;
	u32 rx_offset = rx_ring->rx_offset;
	struct sxe_adapter *adapter = irq_data->adapter;
	u16 cleaned_count = sxe_desc_unused(rx_ring);
	u32 xdp_xmit = 0;
	struct xdp_buff xdp;
	struct sxe_ring_stats stats;

#ifdef HAVE_XDP_BUFF_FRAME_SIZE
	u32 frame_sz = 0;
#if (PAGE_SIZE < SXE_PAGE_SIZE_8KB)
	frame_sz = sxe_rx_frame_truesize(rx_ring, 0);
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

	LOG_DEBUG_BDF("entry rx irq clean:irq=%u, ring_idx=%u, ring_reg_idx=%u, "
		"ring_tc_idx=%u, next_to_clean=%u, next_to_use=%u, budget=%u\n",
		irq_data->irq_idx, rx_ring->idx, rx_ring->reg_idx, rx_ring->tc_idx,
		rx_ring->next_to_clean, rx_ring->next_to_use, budget);

	while (likely(total_rx_packets < budget)) {
		s32 rx_buffer_pgcnt;
		union sxe_rx_data_desc *rx_desc;
		struct sxe_rx_buffer *rx_buffer;
		struct sk_buff *skb;
		u32 size;

		if (cleaned_count >= SXE_RX_BUFFER_WRITE) {
			sxe_rx_ring_buffers_alloc(rx_ring, cleaned_count);
			cleaned_count = 0;
		}

#ifdef SXE_DRIVER_TRACE
		SXE_TRACE_RX(rx_ring->idx, SXE_TRACE_LAB_RX_START);
#endif

		rx_desc = SXE_RX_DESC(rx_ring, rx_ring->next_to_clean);
		size = le16_to_cpu(rx_desc->wb.upper.length);
		if (!size) {
			break;
		}

		LOG_DEBUG_BDF("process rx_desc[%u], write back info:"
				"status_error=0x%x, length=%u, vlan=%u\n",
				rx_ring->next_to_clean,
				le16_to_cpu(rx_desc->wb.upper.status_error),
				le16_to_cpu(rx_desc->wb.upper.length),
				le16_to_cpu(rx_desc->wb.upper.vlan));

		dma_rmb();

		rx_buffer = sxe_rx_buffer_get(rx_ring, rx_desc, &skb,
						size, &rx_buffer_pgcnt);

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
#if (PAGE_SIZE > SXE_PAGE_SIZE_4KB)
			xdp.frame_sz = sxe_rx_frame_truesize(rx_ring, size);
#endif
#endif
			skb = sxe_xdp_run(adapter, rx_ring, &xdp);
		}

		if (IS_ERR(skb)) {
			unsigned int xdp_res = - PTR_ERR(skb);
			LOG_DEBUG("get in xdp process\n");

			if (xdp_res & (SXE_XDP_TX | SXE_XDP_REDIR)) {
				xdp_xmit |= xdp_res;
				sxe_rx_buffer_page_offset_update(rx_ring,
							rx_buffer, size);
			} else {
				rx_buffer->pagecnt_bias++;
			}
			total_rx_packets++;
			total_rx_bytes += size;
		} else if (skb) {
			sxe_add_rx_frag_to_skb(rx_ring, rx_buffer, skb, size);
#ifndef HAVE_NO_SWIOTLB_SKIP_CPU_SYNC
		} else if (ring_uses_build_skb(rx_ring)) {
			skb = sxe_skb_build(rx_ring, rx_buffer,
					      &xdp, rx_desc);
#endif
		} else {
			skb = sxe_skb_construct(rx_ring, rx_buffer,
						  &xdp, rx_desc);
		}

		if (!skb) {
			LOG_INFO("skb is NULL, failed to process\n");
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			rx_buffer->pagecnt_bias++;
			break;
		}

		sxe_rx_buffer_put(rx_ring, rx_buffer, skb, rx_buffer_pgcnt);
		cleaned_count++;

#ifdef SXE_DRIVER_TRACE
		SXE_TRACE_RX(rx_ring->idx, SXE_TRACE_LAB_RX_END);
#endif

		if (sxe_is_non_eop(rx_ring, rx_desc, skb)) {
			continue;
		}

		if (sxe_headers_cleanup(rx_ring, rx_desc, skb)) {
			continue;
		}

		total_rx_bytes += skb->len;

		SKB_DUMP(skb);

		sxe_skb_fields_process(rx_ring, rx_desc, skb);

		LOG_DEBUG("in loop[%u], process total bytes:%u\n",
			  total_rx_packets + 1, skb->len);
		sxe_rx_skb_deliver(irq_data, skb);

		total_rx_packets++;
	}

#ifdef HAVE_XDP_SUPPORT
	if (xdp_xmit & SXE_XDP_REDIR) {
		xdp_do_flush_map();
	}

	if (xdp_xmit & SXE_XDP_TX) {
		struct sxe_ring *ring = sxe_xdp_tx_ring_pick(adapter);
		sxe_xdp_ring_tail_update_locked(ring);
	}
#endif

	stats.packets = total_rx_packets;
	stats.bytes = total_rx_bytes;
	sxe_rx_pkt_stats_update(rx_ring, &irq_data->rx.irq_rate, &stats);

	return total_rx_packets;
}

void sxe_hw_rx_disable(struct sxe_adapter *adapter)
{
	u8 reg_idx;
	u32 i, wait_loop, rxdctl;
	struct sxe_ring *ring;
	unsigned long wait_delay, delay_interval;
	struct	sxe_hw *hw = &adapter->hw;

	hw->dbu.ops->rx_cap_switch_off(hw);

	if (sxe_is_hw_fault(hw)) {
		goto l_end;
	}

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		ring = adapter->rx_ring_ctxt.ring[i];
		reg_idx = ring->reg_idx;

		hw->dma.ops->rx_ring_switch_not_polling(hw, reg_idx, false);
	}

	delay_interval = sxe_pcie_timeout_poll(adapter->pdev, hw);
	wait_delay = delay_interval;

	wait_loop = SXE_MAX_TXRX_DESC_POLL;
	while (wait_loop--) {
		usleep_range(wait_delay, wait_delay + 10);
		wait_delay += delay_interval * 2;
		rxdctl = 0;

		for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
			ring = adapter->rx_ring_ctxt.ring[i];
			reg_idx = ring->reg_idx;

			rxdctl |= hw->dma.ops->rx_desc_ctrl_get(hw, reg_idx);
		}

		if (!(rxdctl & SXE_RXDCTL_ENABLE)) {
			goto l_end;
		}
	}

	LOG_MSG_ERR(drv, "RXDCTL.ENABLE for one or more queues not cleared within the polling period\n");

l_end:
	return;

}
