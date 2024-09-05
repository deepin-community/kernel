#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
#include <net/mpls.h>
#include <linux/vmalloc.h>

#include "sxe.h"
#ifdef HAVE_NO_OVERFLOW_H
#include <sxe_compat_overflow.h>
#else
#include <linux/overflow.h>
#endif
#include "sxe_log.h"
#include "sxe_tx_proc.h"
#include "sxe_irq.h"
#include "sxe_hw.h"
#include "sxe_pci.h"
#include "sxe_debug.h"
#include "sxe_csum.h"
#include "sxe_ptp.h"
#include "sxe_monitor.h"
#include "sxe_filter.h"
#include "sxe_ipsec.h"
#include "sxe_xdp.h"

#ifdef SXE_DRIVER_TRACE
#include "sxe_trace.h"
#endif

#define SXE_SKB_MIN_LEN   17   

#ifdef NEED_SKB_FRAG_SIZE_API
#define skb_frag_size(frag)	skb_frag_size_compat(frag)
static inline unsigned int skb_frag_size_compat(const skb_frag_t *frag)
{
	return frag->size;
}
#endif 

void sxe_tx_ring_buffer_clean(struct sxe_ring *ring)
{
	union  sxe_tx_data_desc *eop_desc, *tx_desc;
	u16    ntc                      = ring->next_to_clean;
	struct sxe_tx_buffer *tx_buffer = &ring->tx_buffer_info[ntc];
#ifdef HAVE_AF_XDP_ZERO_COPY
	if (ring->xsk_pool) {
		sxe_xsk_tx_ring_clean(ring);
		goto l_out;
	}
#endif
	while (ntc != ring->next_to_use) {
#ifdef HAVE_XDP_SUPPORT
		if (ring_is_xdp(ring)) {
			xdp_return_frame(tx_buffer->xdpf);
		} else {
			dev_kfree_skb_any(tx_buffer->skb);
		}
#else
		dev_kfree_skb_any(tx_buffer->skb);
#endif
		dma_unmap_single(ring->dev, dma_unmap_addr(tx_buffer, dma),
				dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);

		eop_desc = tx_buffer->next_to_watch;
		tx_desc  = SXE_TX_DESC(ring, ntc);

		while (tx_desc != eop_desc) {
			tx_buffer++;
			tx_desc++;
			ntc++;
			if (unlikely(ntc == ring->depth)) {
				ntc = 0;
				tx_buffer = ring->tx_buffer_info;
				tx_desc   = SXE_TX_DESC(ring, 0);
			}

			if (dma_unmap_len(tx_buffer, len)) {
				dma_unmap_page(ring->dev,
					       dma_unmap_addr(tx_buffer, dma),
					       dma_unmap_len(tx_buffer, len),
					       DMA_TO_DEVICE);
			}
		}

		tx_buffer++;
		ntc++;
		if (unlikely(ntc == ring->depth)) {
			ntc = 0;
			tx_buffer = ring->tx_buffer_info;
		}
	}

#ifdef HAVE_XDP_SUPPORT
	if (!ring_is_xdp(ring))
#endif
		netdev_tx_reset_queue(netdev_get_tx_queue(ring->netdev, ring->idx));
#ifdef HAVE_AF_XDP_ZERO_COPY
l_out:
#endif
	ring->next_to_use   = 0;
	ring->next_to_clean = 0;
	return;
}

void sxe_tx_ring_free(struct sxe_ring *ring)
{
	sxe_tx_ring_buffer_clean(ring);

	if (ring->tx_buffer_info) {
		vfree(ring->tx_buffer_info);
		ring->tx_buffer_info = NULL;
	}

	if (ring->desc.base_addr) {
		dma_free_coherent(ring->dev, ring->size,
			ring->desc.base_addr, ring->desc.dma);
		ring->desc.base_addr = NULL;
	}

	return ;
}

void sxe_tx_resources_free(struct sxe_adapter *adapter)
{
	u32 i;
	struct sxe_ring **tx_ring = adapter->tx_ring_ctxt.ring;
	struct sxe_ring **xdp_ring = adapter->xdp_ring_ctxt.ring;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		if (tx_ring[i]->desc.base_addr) {
			sxe_tx_ring_free(tx_ring[i]);
		}
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		if (xdp_ring[i]->desc.base_addr) {
			sxe_tx_ring_free(xdp_ring[i]);
		}
	}

	return;
}

s32 sxe_tx_ring_alloc(struct sxe_ring *ring)
{
	s32    ret;
	s32    node        = NUMA_NO_NODE;
	u32    size        = sizeof(struct sxe_tx_buffer) * ring->depth;
	struct device *dev = ring->dev;
	s32    orig_node   = dev_to_node(dev);
	struct sxe_adapter *adapter = netdev_priv(ring->netdev);

	if (ring->irq_data) {
		node = ring->irq_data->numa_node;
	}

	ring->tx_buffer_info = vmalloc_node(size, node);
	if (!ring->tx_buffer_info) {
		ring->tx_buffer_info = vmalloc(size);
		if (!ring->tx_buffer_info) {
			ret = -ENOMEM;
			goto l_err;
		}
	}

	ring->size = ring->depth * sizeof(union sxe_tx_data_desc);
	ring->size = ALIGN(ring->size, SXE_ALIGN_4K);

	set_dev_node(dev, node);
	ring->desc.base_addr = dma_alloc_coherent(dev, ring->size,
					&ring->desc.dma, GFP_KERNEL);
	set_dev_node(dev, orig_node);
	if (!ring->desc.base_addr) {
		ring->desc.base_addr = dma_alloc_coherent(dev, ring->size,
					&ring->desc.dma, GFP_KERNEL);
		if (!ring->desc.base_addr) {
			ret = -ENOMEM;
			goto l_free;
		}
	}

	ring->next_to_use   = 0;
	ring->next_to_clean = 0;
	return 0;

l_free:
	vfree(ring->tx_buffer_info);
	ring->tx_buffer_info = NULL;
l_err:
	LOG_DEV_ERR("unable to allocate memory for the Tx descriptor ring\n");
	return ret;
}

static s32 sxe_tx_resources_alloc(struct sxe_adapter *adapter)
{
	s32 ret;
	u32 i, j;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		ret = sxe_tx_ring_alloc(adapter->tx_ring_ctxt.ring[i]);
		if (ret < 0) {
			LOG_MSG_ERR(probe, "allocation for Tx Queue %d failed\n", i);
			goto l_tx_free;
		}
	}

	for (j = 0; j < adapter->xdp_ring_ctxt.num; j++) {
		ret = sxe_tx_ring_alloc(adapter->xdp_ring_ctxt.ring[j]);
		if (ret < 0) {
			LOG_MSG_ERR(probe, "allocation for xdp Queue %d failed\n", j);
			goto l_xdp_free;
		}
	}

	return 0;

l_xdp_free:
	while (j--) {
		sxe_tx_ring_free(adapter->xdp_ring_ctxt.ring[j]);
	}

l_tx_free:
	while (i--) {
		sxe_tx_ring_free(adapter->tx_ring_ctxt.ring[i]);
	}

	return ret;
}

s32 sxe_tx_ring_depth_reset(struct sxe_adapter *adapter, u32 tx_cnt)
{
	s32 ret;
	u32 i, j, tx_ring_cnt;
	struct sxe_ring *temp_ring;
	struct sxe_ring **tx_ring   = adapter->tx_ring_ctxt.ring;
	struct sxe_ring **xdp_ring  = adapter->xdp_ring_ctxt.ring;

	tx_ring_cnt = adapter->tx_ring_ctxt.num + adapter->xdp_ring_ctxt.num;
	temp_ring = vmalloc(array_size(tx_ring_cnt, sizeof(struct sxe_ring)));
	if (!temp_ring) {
		LOG_ERROR_BDF("vmalloc failed, size=%lu\n",
			array_size(tx_ring_cnt, sizeof(struct sxe_ring)));
		ret = -ENOMEM;
		goto l_end;
	}

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		memcpy(&temp_ring[i], tx_ring[i], sizeof(struct sxe_ring));
		temp_ring[i].depth = tx_cnt;
		ret = sxe_tx_ring_alloc(&temp_ring[i]);
		if (ret < 0) {
			LOG_ERROR_BDF("tx ring alloc failed, tx ring idx=%d\n", i);
			goto l_tx_free;
		}
	}

	for (j = 0; j < adapter->xdp_ring_ctxt.num; j++, i++) {
		memcpy(&temp_ring[i], xdp_ring[j], sizeof(struct sxe_ring));
		temp_ring[i].depth = tx_cnt;
		ret = sxe_tx_ring_alloc(&temp_ring[i]);
		if (ret < 0) {
			LOG_ERROR_BDF("xdp ring alloc failed, xdp ring idx=%d\n", j);
			goto l_tx_free;
		}
	}

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		sxe_tx_ring_free(tx_ring[i]);
		memcpy(tx_ring[i], &temp_ring[i], sizeof(struct sxe_ring));
	}

	for (j = 0; j < adapter->xdp_ring_ctxt.num; j++, i++) {
		sxe_tx_ring_free(xdp_ring[j]);
		memcpy(xdp_ring[j], &temp_ring[i], sizeof(struct sxe_ring));
	}

	adapter->tx_ring_ctxt.depth  = tx_cnt;
	adapter->xdp_ring_ctxt.depth = tx_cnt;
	ret = 0;
	goto l_temp_free;

l_tx_free:
	while (i--) {
		sxe_tx_ring_free(&temp_ring[i]);
	}

l_temp_free:
	vfree(temp_ring);

l_end:
	return ret;
}

static void sxe_tx_ring_fnav_configure(struct sxe_adapter *adapter,
					struct sxe_ring *ring)
{
	if (adapter->cap & SXE_FNAV_SAMPLE_ENABLE) {
		ring->fnav_sample_rate = adapter->fnav_ctxt.sample_rate;
		ring->fnav_sample_count = 0;
		set_bit(SXE_TX_FNAV_INIT_DONE, &ring->state);
	} else {
		ring->fnav_sample_rate = 0;
	}

	return;
}

void sxe_tx_ring_reg_configure(struct sxe_adapter *adapter,
				struct sxe_ring *ring)
{
	u32 host_thresh, prefetch_thresh;
	u32 reg_idx = ring->reg_idx;
	struct sxe_hw *hw = &adapter->hw;
	u32 dma_len = ring->depth * sizeof(union sxe_tx_data_desc);
	u32 wb_thresh = 0;

	hw->dma.ops->tx_ring_desc_configure(hw, dma_len,
				(u64)ring->desc.dma, reg_idx);
	ring->desc.tail = adapter->hw.reg_base_addr + SXE_TDT(reg_idx);

	if (!ring->irq_data || \
			ring->irq_data->irq_interval < SXE_IRQ_ITR_100K) {
		if (adapter->irq_ctxt.rx_irq_interval) {
			wb_thresh = SXE_TX_DESC_PREFETCH_THRESH_8;
		}
	} else {
		wb_thresh = SXE_TX_DESC_PREFETCH_THRESH_8;
	}
	host_thresh = SXE_TX_DESC_HOST_THRESH;
	prefetch_thresh = SXE_TX_DESC_WRITEBACK_THRESH;
	hw->dma.ops->tx_desc_thresh_set(hw, reg_idx,
				wb_thresh, host_thresh, prefetch_thresh);

	hw->dma.ops->tx_ring_tail_init(hw, reg_idx);

	hw->dma.ops->tx_ring_switch(hw, reg_idx, true);
	return;
}

static inline bool sxe_is_xdp_use_extra_pkg_buf(struct sxe_adapter *adapter)
{
	u16 num = adapter->tx_ring_ctxt.num + adapter->xdp_ring_ctxt.num;

	if (adapter->xdp_prog && (num > SXE_FNAV_RING_NUM_MAX)) {
		return true;
	}

	return false;
}

static void sxe_hw_tx_buf_configure(struct sxe_adapter *adapter)
{
	u8 tcs = sxe_dcb_tc_get(adapter);
	struct sxe_hw *hw = &adapter->hw;

	if (sxe_is_xdp_use_extra_pkg_buf(adapter)) {
		LOG_INFO_BDF("nr cpu=%u, xdp use rings large then 64\n", nr_cpu_ids);
		tcs = 4;
	}

	hw->dbu.ops->tx_pkt_buf_switch(hw, false);

	hw->dbu.ops->tx_pkt_buf_size_configure(hw, tcs);

	if (adapter->cap & SXE_DCB_ENABLE) {
		hw->dma.ops->tx_pkt_buf_thresh_configure(hw, tcs, true);
	} else {
		hw->dma.ops->tx_pkt_buf_thresh_configure(hw, tcs, false);
	}

	hw->dbu.ops->tx_pkt_buf_switch(hw, true);

	return;
}

static void sxe_tx_multi_ring_configure(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	u8 tcs = sxe_dcb_tc_get(adapter);
	bool enable = !!(adapter->cap & SXE_SRIOV_ENABLE);
	u16 mask = sxe_pool_mask_get(adapter);
	u16 num = adapter->tx_ring_ctxt.num + adapter->xdp_ring_ctxt.num;

	hw->dma.ops->tx_multi_ring_configure(hw, tcs,
					mask, enable, num);

	return;
}

static void sxe_tx_ring_xps_set(struct sxe_ring *ring)
{
	if (!test_and_set_bit(SXE_TX_XPS_INIT_DONE, &ring->state)) {
		struct sxe_irq_data *irq_data = ring->irq_data;

		if (irq_data) {
			netif_set_xps_queue(ring->netdev,
					    &irq_data->affinity_mask,
					    ring->idx);
		}
	}

	return;
}

void sxe_tx_ring_attr_configure(struct sxe_adapter *adapter,
					struct sxe_ring *ring)
{
#ifdef HAVE_AF_XDP_ZERO_COPY
	ring->xsk_pool = NULL;
	if (ring_is_xdp(ring)) {
		LOG_DEBUG_BDF("ring[%u] is xdp and begin get xsk pool\n", ring->idx);
		ring->xsk_pool = sxe_xsk_pool_get(adapter, ring);
	}
#endif
	sxe_tx_ring_xps_set(ring);
	clear_bit(SXE_HANG_CHECK_ARMED, &ring->state);
	sxe_tx_buffer_init(ring);

	sxe_tx_ring_fnav_configure(adapter, ring);

	sxe_tx_ring_reg_configure(adapter, ring);
	return;
}

void sxe_hw_tx_configure(struct sxe_adapter *adapter)
{
	u32 i;
	struct sxe_hw *hw = &adapter->hw;

	sxe_tx_multi_ring_configure(adapter);

	sxe_hw_tx_buf_configure(adapter);

	hw->mac.ops->pad_enable(hw);

	hw->dma.ops->tx_enable(hw);

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		sxe_tx_ring_attr_configure(adapter, adapter->tx_ring_ctxt.ring[i]);
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++){
		sxe_tx_ring_attr_configure(adapter, adapter->xdp_ring_ctxt.ring[i]);
	}

	return;
}

s32 sxe_test_tx_configure(struct sxe_adapter *adapter, struct sxe_ring *ring)
{
	s32 ret;
	struct sxe_hw *hw = &adapter->hw;

	ring->depth   = SXE_DEFAULT_DESC_CNT;
	ring->idx     = 0;
	ring->dev     = &adapter->pdev->dev;
	ring->netdev  = adapter->netdev;
	ring->reg_idx = adapter->tx_ring_ctxt.ring[0]->reg_idx;

	ret = sxe_tx_ring_alloc(ring);
	if (ret) {
		LOG_ERROR_BDF("test tx ring alloc failed, ret=%d", ret);
		goto l_end;
	}

	sxe_hw_tx_buf_configure(adapter);

	hw->dma.ops->tx_enable(hw);

	sxe_tx_ring_xps_set(ring);
	clear_bit(SXE_HANG_CHECK_ARMED, &ring->state);
	sxe_tx_buffer_init(ring);

	sxe_tx_ring_reg_configure(adapter, ring);

	hw->dma.ops->tx_desc_wb_thresh_clear(hw, ring->reg_idx);

l_end:
	return ret;
}

s32 sxe_tx_configure(struct sxe_adapter *adapter)
{
	s32 ret;

	ret = sxe_tx_resources_alloc(adapter);
	if (ret) {
		LOG_ERROR_BDF("tx ring init failed, ret = %d\n", ret);
		goto l_err;
	}

	sxe_hw_tx_configure(adapter);

	ret = netif_set_real_num_tx_queues(adapter->netdev,
				adapter->tx_ring_ctxt.num);
	if (ret) {
		LOG_ERROR_BDF("netif_set_real_num_tx_queues failed, ret = %d\n", ret);
		sxe_tx_resources_free(adapter);
	}

l_err:
	return ret;
}

static void sxe_tx_ctxt_desc_set(struct sxe_ring *tx_ring,
			struct sxe_tx_context_desc *ctxt_desc)
{
	struct sxe_tx_context_desc *context_desc;
	u16 i = tx_ring->next_to_use;
	struct sxe_adapter *adapter = netdev_priv(tx_ring->netdev);

	context_desc = SXE_TX_CTXTDESC(tx_ring, i);

	i++;
	tx_ring->next_to_use = (i < tx_ring->depth) ? i : 0;

	ctxt_desc->type_tucmd_mlhl |= SXE_TXD_DTYP_CTXT;

	context_desc->vlan_macip_lens = cpu_to_le32(ctxt_desc->vlan_macip_lens);
	context_desc->sa_idx           = cpu_to_le32(ctxt_desc->sa_idx);
	context_desc->type_tucmd_mlhl = cpu_to_le32(ctxt_desc->type_tucmd_mlhl);
	context_desc->mss_l4len_idx   = cpu_to_le32(ctxt_desc->mss_l4len_idx);

	LOG_DEBUG_BDF("contxt desc, ring=%u, ntu=%u, ntc=%u, vlan_macip_lens=%#x, "
		"sa_idx=%#x, type_tucmd_mlhl=%#x, mss_l4len_idx=%x\n",
		tx_ring->idx, tx_ring->next_to_use, tx_ring->next_to_clean,
		context_desc->vlan_macip_lens,
		context_desc->sa_idx,
		context_desc->type_tucmd_mlhl,
		context_desc->mss_l4len_idx);
	return;
}

STATIC s32 sxe_tso(struct sxe_ring *tx_ring,
			struct sxe_tx_buffer *first_buff,
			struct sxe_tx_context_desc *ctxt_desc,
			u8 *hdr_len)
{
	s32 ret;
	u16 tucmd;
	union sxe_ip_hdr ip;
	union sxe_l4_hdr l4;
	u8 *csum_start, *trans_start;
	u32 mss_l4len, paylen, l4_offset, len;
	struct sk_buff *skb = first_buff->skb;
	struct sxe_adapter *adapter = netdev_priv(tx_ring->netdev);

	if (skb->ip_summed != CHECKSUM_PARTIAL || !skb_is_gso(skb)) {
		ret = 0;
		goto  l_end;
	}

	LOG_DEBUG_BDF("tso start, ring[%d]\n", tx_ring->idx);

	ret = skb_cow_head(skb, 0);
	if (ret < 0) {
		LOG_ERROR_BDF("skb cow head failed, ret=%d\n", ret);
		goto l_end;
	}

	if (eth_p_mpls(first_buff->protocol)) {
		ip.hdr = skb_inner_network_header(skb);
	} else {
		ip.hdr = skb_network_header(skb);
	}

	tucmd = SXE_TX_CTXTD_TUCMD_L4T_TCP;

	if (ip.v4->version == SXE_IPV4) {
		csum_start = skb_checksum_start(skb);
		trans_start = ip.hdr + (ip.v4->ihl * 4);
		len = csum_start - trans_start;

		ip.v4->check = (skb_shinfo(skb)->gso_type & SKB_GSO_PARTIAL) ?
					csum_fold(csum_partial(trans_start,
					len, 0)) : 0;

		LOG_INFO_BDF("tso ipv4 ip.v4->check=%u, gso_type=%x\n",
				ip.v4->check, skb_shinfo(skb)->gso_type);
		tucmd |= SXE_TX_CTXTD_TUCMD_IPV4;

		ip.v4->tot_len = 0;
		first_buff->tx_features |= SXE_TX_FEATURE_TSO |
				   SXE_TX_FEATURE_CSUM |
				   SXE_TX_FEATURE_IPV4;
	} else {
		ip.v6->payload_len = 0;
		first_buff->tx_features |= SXE_TX_FEATURE_TSO |
				   SXE_TX_FEATURE_CSUM;
	}

	l4.hdr = skb_checksum_start(skb);
	l4_offset = l4.hdr - skb->data;

	*hdr_len = (l4.tcp->doff * 4) + l4_offset;

	paylen = skb->len - l4_offset;
	csum_replace_by_diff(&l4.tcp->check, (__force __wsum)htonl(paylen));

	first_buff->gso_segs = skb_shinfo(skb)->gso_segs;
	first_buff->bytecount += (first_buff->gso_segs - 1) * (*hdr_len);

	sxe_ctxt_desc_iplen_set(ctxt_desc, (l4.hdr - ip.hdr));
	sxe_ctxt_desc_maclen_set(ctxt_desc, (ip.hdr - skb->data));
	sxe_ctxt_desc_tucmd_set(ctxt_desc, tucmd);
	mss_l4len = (*hdr_len - l4_offset) << SXE_TX_CTXTD_L4LEN_SHIFT;
	mss_l4len |= skb_shinfo(skb)->gso_size << SXE_TX_CTXTD_MSS_SHIFT;
	sxe_ctxt_desc_mss_l4len_set(ctxt_desc, mss_l4len);

	ret = 1;
l_end:
	return ret;
}

static inline u16 sxe_desc_unused_count(struct sxe_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->depth) + ntc - ntu - 1;
}

STATIC s32 sxe_maybe_stop_tx(struct sxe_ring *ring, u16 size)
{
	s32 ret = 0;

	netif_stop_subqueue(ring->netdev, ring->idx);

	smp_mb();

	if (likely(sxe_desc_unused_count(ring) < size)) {
		ret = -EBUSY;
		goto l_end;
	}

	netif_start_subqueue(ring->netdev, ring->idx);

	++ring->tx_stats.restart_queue;

l_end:
	return ret;
}

STATIC netdev_tx_t sxe_ring_maybe_stop_tx(struct sk_buff *skb,
					struct sxe_ring *tx_ring)
{
	u16 i, need_num;
	netdev_tx_t ret = NETDEV_TX_OK;
	u16  desc_cnt = SXE_TX_DESC_USE_COUNT(skb_headlen(skb));

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		desc_cnt += SXE_TX_DESC_USE_COUNT(
				skb_frag_size(&skb_shinfo(skb)->frags[i]));
	}

	need_num = desc_cnt + SXE_TX_NON_DATA_DESC_NUM;

	if (unlikely(sxe_desc_unused_count(tx_ring) < need_num)) {
		if (sxe_maybe_stop_tx(tx_ring, need_num) < 0) {
			ret = NETDEV_TX_BUSY;
		}

		tx_ring->tx_stats.tx_busy++;
	}

	return ret;
}

static u32 sxe_tx_cmd_type(struct sk_buff *skb, u32 flags)
{
	u32 cmd_type = SXE_TX_DESC_TYPE_DATA |
				SXE_TX_DESC_IFCS;

	cmd_type |= SXE_TX_SET_FLAG(flags, SXE_TX_FEATURE_HW_VLAN,
				   SXE_TX_DESC_VLE);

	cmd_type |= SXE_TX_SET_FLAG(flags, SXE_TX_FEATURE_TSTAMP,
				   SXE_TX_DESC_TSTAMP);

	cmd_type |= SXE_TX_SET_FLAG(flags, SXE_TX_FEATURE_TSO,
				   SXE_TXD_DCMD_TSE);

	cmd_type ^= SXE_TX_SET_FLAG(skb->no_fcs, 1, SXE_TX_DESC_IFCS);

	return cmd_type;
}

static void sxe_tx_desc_offload_setup(u32 flags,
					unsigned int paylen,
					union sxe_tx_data_desc *tx_desc)
{
	u32 olinfo_status = paylen << SXE_TX_DESC_PAYLEN_SHIFT;

	olinfo_status |= SXE_TX_SET_FLAG(flags,
					SXE_TX_FEATURE_CSUM,
					SXE_TXD_POPTS_TXSM);

	olinfo_status |= SXE_TX_SET_FLAG(flags,
					SXE_TX_FEATURE_IPV4,
					SXE_TXD_POPTS_IXSM);

#ifdef SXE_IPSEC_CONFIGURE
	olinfo_status |= SXE_TX_SET_FLAG(flags,
					SXE_TX_FEATURE_IPSEC,
					SXE_TXD_POPTS_IPSEC);
#endif

	tx_desc->read.olinfo_status = cpu_to_le32(olinfo_status);

	return;
}

static inline void sxe_tx_desc_update(
				struct sxe_ring *ring,
				union sxe_tx_data_desc **desc,
				u16 *next_to_use)
{
	++(*next_to_use);
	++(*desc);
	if (ring->depth == *next_to_use) {
		*desc = SXE_TX_DESC(ring, 0);
		*next_to_use = 0;
	}
	(*desc)->read.olinfo_status = 0;

	return;
}

static void sxe_tx_dma_err(struct sxe_ring *ring,
			struct sxe_tx_buffer *first_buffer,
			u16    next_to_use)
{
	struct sxe_tx_buffer *tx_buffer;

	for (;;) {
		tx_buffer = &ring->tx_buffer_info[next_to_use];
		if (dma_unmap_len(tx_buffer, len)) {
			dma_unmap_page(ring->dev,dma_unmap_addr(tx_buffer, dma),
				dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
		}
		dma_unmap_len_set(tx_buffer, len, 0);

		if (tx_buffer == first_buffer) {
			break;
		}

		if (next_to_use == 0) {
			next_to_use += ring->depth;
		}

		--next_to_use;
	}

	dev_kfree_skb_any(first_buffer->skb);
	first_buffer->skb = NULL;

	ring->next_to_use = next_to_use;

	return;
}

STATIC s32 sxe_tx_desc_setup(struct sxe_ring *ring,
				struct sk_buff *skb,
				struct sxe_tx_buffer *first_buffer,
				union sxe_tx_data_desc **desc,
				u16    *next_to_use)
{
	dma_addr_t dma;
	skb_frag_t *frag;
	u32 map_size       = skb_headlen(skb); 
	u32 remaining_size = skb->data_len;    
	u32 cmd_type       = sxe_tx_cmd_type(skb, first_buffer->tx_features);
	struct sxe_tx_buffer *tx_buffer = first_buffer;
	struct sxe_adapter *adapter = netdev_priv(ring->netdev);

	LOG_DEBUG_BDF("skb dma map start, line_size=%u,"
			" total_frag_len=%u, skb_len=%u\n",
			skb_headlen(skb), skb->data_len, skb->len);

	dma = dma_map_single(ring->dev, skb->data, map_size, DMA_TO_DEVICE);

	for (frag = &skb_shinfo(skb)->frags[0];; frag++) {
		if (dma_mapping_error(ring->dev, dma)) {
			LOG_DEV_ERR("tx dma map failed\n");
			goto l_dma_err;
		}
		dma_unmap_len_set(tx_buffer, len, map_size);
		dma_unmap_addr_set(tx_buffer, dma, dma);

		(*desc)->read.buffer_addr = cpu_to_le64(dma);

		while (unlikely(map_size > SXE_DATA_PER_DESC_SIZE_MAX)) {
			(*desc)->read.cmd_type_len = \
			cpu_to_le32(cmd_type ^ SXE_DATA_PER_DESC_SIZE_MAX);

			sxe_tx_desc_update(ring, desc, next_to_use);
			dma            += SXE_DATA_PER_DESC_SIZE_MAX;
			map_size       -= SXE_DATA_PER_DESC_SIZE_MAX;

			(*desc)->read.buffer_addr = cpu_to_le64(dma);
		}

		if (likely(!remaining_size)) {
			cmd_type |= map_size | SXE_TX_DESC_CMD;
			(*desc)->read.cmd_type_len = cpu_to_le32(cmd_type);
			LOG_DEBUG_BDF("skb dma map, current_map_size=%u, remaining_size=%u, "
			"desc_ptr=%p, dma_addr=%#llx, desc.buffer_addr = %#llx, "
			"desc.cmdtype=%#x, desc.olinfo_status=%#x\n",
				map_size, remaining_size, *desc,
				(U64)dma, (*desc)->read.buffer_addr,
				(*desc)->read.cmd_type_len,
				(*desc)->read.olinfo_status);
			break;
		}

		(*desc)->read.cmd_type_len = cpu_to_le32(cmd_type ^ map_size);

		LOG_DEBUG_BDF("skb dma map, current_map_size=%u, remaining_size=%u, "
			"desc_ptr=%p, dma_addr=%#llx, desc.buffer_addr = %#llx, "
			"desc.cmdtype=%#x, desc.olinfo_status=%#x\n",
				map_size, remaining_size, *desc,
				(U64)dma, (*desc)->read.buffer_addr,
				(*desc)->read.cmd_type_len,
				(*desc)->read.olinfo_status);

		sxe_tx_desc_update(ring, desc, next_to_use);

		map_size        = skb_frag_size(frag);
		remaining_size -= map_size;
		dma = skb_frag_dma_map(ring->dev, frag, 0,
						map_size, DMA_TO_DEVICE);

		tx_buffer = &ring->tx_buffer_info[*next_to_use];
	}

	LOG_DEBUG_BDF("skb dma map end\n");
	return 0;
l_dma_err:
	sxe_tx_dma_err(ring, first_buffer, *next_to_use);
	return -ENOMEM;
}

#ifdef NETDEV_XMIT_MORE_WORK_AROUND
#define netdev_xmit_more_workaround()   (skb->xmit_more)
#endif

STATIC s32 sxe_xmit_pkt(struct sxe_ring *ring,
			struct sxe_tx_buffer *first_buffer,
			const u8 hdr_len)
{
	s32 ret;
	struct netdev_queue *queue;
	struct sk_buff *skb = first_buffer->skb;
	u32    tx_features  = first_buffer->tx_features;
	u16    ntu          = ring->next_to_use;
	union sxe_tx_data_desc *desc = SXE_TX_DESC(ring, ntu);
	struct sxe_adapter *adapter = netdev_priv(ring->netdev);

	sxe_tx_desc_offload_setup(tx_features, skb->len - hdr_len, desc);

	ret = sxe_tx_desc_setup(ring, skb, first_buffer, &desc, &ntu);
	if (ret) {
		goto l_end;
	}

	queue = netdev_get_tx_queue(ring->netdev, ring->idx);
	netdev_tx_sent_queue(queue, first_buffer->bytecount);

	first_buffer->time_stamp = jiffies;
	skb_tx_timestamp(skb);

	wmb();

	first_buffer->next_to_watch = desc;

	ntu++;
	if (ntu == ring->depth) {
		ntu = 0;
	}
	ring->next_to_use = ntu;

	if (unlikely(sxe_desc_unused_count(ring) < SXE_TX_DESC_NEEDED)) {
	 	ret = sxe_maybe_stop_tx(ring, SXE_TX_DESC_NEEDED);
		if (ret < 0) {
			LOG_WARN_BDF("the desc is not enough in the ring[%u],"
				"to stop the ring, "
				"desc_cnt < SXE_TX_DESC_NEEDED[%u]\n",
					ring->idx, (u32)SXE_TX_DESC_NEEDED);
		}
	}

	if (netif_xmit_stopped(queue) || 
#ifdef NETDEV_XMIT_MORE_WORK_AROUND
		!netdev_xmit_more_workaround()
#else
		!netdev_xmit_more()
#endif
	) {
		writel(ntu, ring->desc.tail);
		LOG_DEBUG_BDF("send directly, ring[%u]\n", ring->idx);
	}

	LOG_DEBUG_BDF("tx end: ring idx=%u, ring reg=%u, next_to_use=%d, "
		"next_to_clean=%d, next_to_watch=%p\n",
		ring->idx, ring->reg_idx, ring->next_to_use,
		ring->next_to_clean, first_buffer->next_to_watch);

	return 0;

l_end:
	return ret;
}

static inline struct sxe_tx_buffer *sxe_tx_first_buffer_get(
					struct sk_buff *skb,
					struct sxe_ring *ring)
{
	struct sxe_tx_buffer *first_buff;

	first_buff              = &ring->tx_buffer_info[ring->next_to_use];
	first_buff->skb         = skb;
	first_buff->bytecount   = skb->len;
	first_buff->gso_segs    = 1;
	first_buff->protocol    = skb->protocol;
	first_buff->tx_features = 0;

	return first_buff;
}

STATIC s32 sxe_tx_vlan_process(struct sk_buff *skb,
		struct sxe_tx_buffer *first_buffer,
		struct sxe_tx_context_desc *ctxt_desc)
{
	s32    ret = 0;
	__be16 protocol = skb->protocol;
	u32    vlan_tag = 0;

	if (skb_vlan_tag_present(skb)) {
		vlan_tag  = skb_vlan_tag_get(skb);
		first_buffer->tx_features |= SXE_TX_FEATURE_HW_VLAN;

	} else if (protocol == htons(ETH_P_8021Q)) {
		struct vlan_hdr *vhdr, _vhdr;
		vhdr = skb_header_pointer(skb, ETH_HLEN, sizeof(_vhdr), &_vhdr);
		if (!vhdr) {
			ret = -SXE_ERR_PARAM;
			goto l_ret;
		}

		vlan_tag = ntohs(vhdr->h_vlan_TCI);
		first_buffer->tx_features |=  SXE_TX_FEATURE_SW_VLAN;
	}
	protocol = vlan_get_protocol(skb);

	first_buffer->protocol = protocol;

	first_buffer->tx_features |= vlan_tag << SXE_TX_FEATURE_VLAN_SHIFT;

	sxe_ctxt_desc_vlan_tag_set(ctxt_desc, vlan_tag);
	sxe_ctxt_desc_maclen_set(ctxt_desc, skb_network_offset(skb));

l_ret:
	return ret;
}

#if 0 

static u8 *sxe_ptp_header_parse(struct sk_buff *skb, unsigned int type)
{
	u8 *ptp_hdr = NULL;
	u8 *data = skb_mac_header(skb);
	u32 offset = 0;

	if (type & PTP_CLASS_VLAN) {
		offset += VLAN_HLEN;
	}

	switch (type & PTP_CLASS_PMASK) {
	case PTP_CLASS_IPV4:
		offset += ETH_HLEN + IPV4_HLEN(data + offset) + UDP_HLEN;
		break;
	case PTP_CLASS_IPV6:
		offset += ETH_HLEN + IP6_HLEN + UDP_HLEN;
		break;
	case PTP_CLASS_L2:
		offset += ETH_HLEN;
		break;
	default:
		goto l_ret;
	}

	if (skb->len + ETH_HLEN < offset + 34) {
		goto l_ret;
	}

	ptp_hdr = data + offset;
l_ret:
	return ptp_hdr;
}

static void sxe_tx_ptp_one_step_mode_check(struct sxe_adapter *adapter,
						struct sk_buff *skb)
{
	u32 ptp_type;
	u8 *ptp_hdr;
	u8 ptp_msgtype;
	u16 ptp_flagfield;
	u32 onestep;

	ptp_type = ptp_classify_raw(skb);
	if (ptp_type != PTP_CLASS_NONE) {
		ptp_hdr = sxe_ptp_header_parse(skb, ptp_type);
		if (ptp_hdr) {
			ptp_flagfield = *(__be16 *)(ptp_hdr +
				SXE_PTP_FLAGFIELD_OFFSET);

			onestep = !!!(ptp_flagfield & SXE_PTP_FLAGFIELD_TWOSTEP);
			if (onestep) {
				ptp_msgtype = *(u8 *)ptp_hdr;
				if (ptp_msgtype == SXE_PTP_MSG_TYPE_SYNC) {
					adapter->cap |= SXE_1588V2_ONE_STEP;
				}
			}
		}
	}

	return;
}
#endif

static void sxe_tx_ptp_process(struct sxe_adapter *adapter,
				struct sk_buff *skb,
				struct sxe_tx_buffer *first_buffer)
{
	if (unlikely(skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) &&
	    adapter->ptp_ctxt.ptp_clock) {
	    	LOG_DEBUG_BDF("need ptp stamp and ptp_clock=0x%p\n",
			adapter->ptp_ctxt.ptp_clock);
		if ((adapter->ptp_ctxt.tstamp_config.tx_type == HWTSTAMP_TX_ON) &&
		    (!test_and_set_bit_lock(SXE_PTP_TX_IN_PROGRESS,
		    			&adapter->state))) {
			LOG_DEBUG_BDF("there is no other tx in ptp mode\n");
			skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
			first_buffer->tx_features |= SXE_TX_FEATURE_TSTAMP;

			adapter->ptp_ctxt.ptp_tx_skb = skb_get(skb);
			adapter->ptp_ctxt.ptp_tx_start = jiffies;
			schedule_work(&adapter->ptp_ctxt.ptp_tx_work);
		} else {
			adapter->stats.sw.tx_hwtstamp_skipped++;
		}
	}

	return;
}

#ifdef SXE_DCB_CONFIGURE
STATIC s32 sxe_tx_dcb_process(struct sxe_adapter *adatper,
		struct sk_buff *skb,
		struct sxe_tx_buffer *first_buffer,
		struct sxe_tx_context_desc *ctxt_desc)
{
	s32 ret = 0;
	u32 vlan_tag;
	struct vlan_ethhdr *vhdr;

	if (!(adatper->cap & SXE_DCB_ENABLE)) {
		goto l_end;
	}

	if ((first_buffer->tx_features &
		(SXE_TX_FEATURE_HW_VLAN | SXE_TX_FEATURE_SW_VLAN)) ||
			(skb->priority != TC_PRIO_CONTROL)) {

		first_buffer->tx_features &= ~SXE_TX_FEATURE_VLAN_PRIO_MASK;
		first_buffer->tx_features |= (skb->priority & 0x7) <<
					SXE_TX_FEATURE_VLAN_PRIO_SHIFT;
		vlan_tag = first_buffer->tx_features >> \
						SXE_TX_FEATURE_VLAN_SHIFT;

		if (first_buffer->tx_features & SXE_TX_FEATURE_SW_VLAN) {
			if (skb_cow_head(skb, 0)) {
				LOG_ERROR("skb head reallocation failed\n");
				ret = -ENOMEM;
				goto l_end;
			}

			vhdr = (struct vlan_ethhdr *)skb->data;
			vhdr->h_vlan_TCI = htons(vlan_tag);
		} else {
			first_buffer->tx_features |= SXE_TX_FEATURE_HW_VLAN;
			sxe_ctxt_desc_vlan_tag_set(ctxt_desc, vlan_tag);
			sxe_ctxt_desc_maclen_set(ctxt_desc, skb_network_offset(skb));
		}

		LOG_DEBUG("dcb enable needs to handle vlan tag, "
			"vlan_tag=%x, priority=%d, tx_features=%x\n",
			vlan_tag, skb->priority, first_buffer->tx_features);
	}

l_end:
	return ret;
}
#endif

STATIC s32 sxe_tx_feature_offload(struct sxe_adapter *adapter,
				struct sxe_ring *ring,
				struct sk_buff *skb,
				struct sxe_tx_buffer *first_buffer,
				u8  *hdr_len)
{
	s32 ret;
	s32 need_tso;
	struct sxe_tx_context_desc ctxt_desc = {0};


	ret = sxe_tx_vlan_process(skb, first_buffer, &ctxt_desc);
	if (ret < 0) {
		LOG_ERROR_BDF("ring[%u] vlan process failed\n", ring->idx);
		goto l_end;
	}

#ifdef SXE_DCB_CONFIGURE
	ret = sxe_tx_dcb_process(adapter, skb, first_buffer, &ctxt_desc);
	if (ret) {
		LOG_ERROR_BDF("ring[%u] dcb process failed\n", ring->idx);
		goto l_end;
	}
#endif

#ifdef SXE_IPSEC_CONFIGURE
	ret = sxe_tx_ipsec_offload(ring, first_buffer, &ctxt_desc);
	if (ret) {
		LOG_ERROR_BDF("ring[%u] tx ipsec offload failed.(err:%d)\n",
			  ring->idx, ret);
		goto l_end;
	}
#endif

	need_tso = sxe_tso(ring, first_buffer, &ctxt_desc, hdr_len);
	if (need_tso < 0) {
		LOG_ERROR_BDF("tso deal failed, ring->idx=%u\n", ring->idx);
		ret = need_tso;
		goto l_end;
	} else if (!need_tso) {
		sxe_tx_csum_offload(ring, first_buffer, &ctxt_desc);
	}

	if (first_buffer->tx_features &
		(SXE_TX_FEATURE_HW_VLAN |
		 SXE_TX_FEATURE_CC |
		 SXE_TX_FEATURE_CSUM |
		 SXE_TX_FEATURE_IPSEC |
		 SXE_TX_FEATURE_TSO)) {

		sxe_tx_ctxt_desc_set(ring, &ctxt_desc);
	}

l_end:
	return ret;
}

netdev_tx_t sxe_ring_xmit(struct sk_buff *skb,
				struct net_device *netdev,
				struct sxe_ring *ring)
{
	s32 res;
	u8 hdr_len = 0;
	netdev_tx_t ret = NETDEV_TX_OK;
	struct sxe_tx_buffer *first_buffer;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	ret = sxe_ring_maybe_stop_tx(skb, ring);
	if (ret != NETDEV_TX_OK) {
		LOG_ERROR_BDF("tx busy, ring idx=%u\n", ring->idx);
		goto l_end;
	}

	first_buffer = sxe_tx_first_buffer_get(skb, ring);

	res = sxe_tx_feature_offload(adapter, ring, skb, first_buffer, &hdr_len);
	if (res < 0) {
		LOG_ERROR_BDF("tx offload failed, ring->idx=%u\n", ring->idx);
		goto l_free;
	}

	sxe_tx_ptp_process(adapter, skb, first_buffer);

	if (test_bit(SXE_TX_FNAV_INIT_DONE, &ring->state)) {
		sxe_fnav_sample_rule_get(ring, first_buffer);
	}

	if (sxe_xmit_pkt(ring, first_buffer, hdr_len)) {
		LOG_ERROR_BDF("tx dma mapping err, ring idx=%u\n", ring->idx);
		goto l_cleanup_tx_timestamp;
	}
	return NETDEV_TX_OK;

l_free:
	dev_kfree_skb_any(first_buffer->skb);
	first_buffer->skb = NULL;
l_cleanup_tx_timestamp:
	if (unlikely(first_buffer->tx_features & SXE_TX_FEATURE_TSTAMP)) {
		dev_kfree_skb_any(adapter->ptp_ctxt.ptp_tx_skb);
		adapter->ptp_ctxt.ptp_tx_skb = NULL;
		cancel_work_sync(&adapter->ptp_ctxt.ptp_tx_work);
		clear_bit_unlock(SXE_PTP_TX_IN_PROGRESS, &adapter->state);
	}
l_end:
	return ret;
}

netdev_tx_t sxe_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	netdev_tx_t ret;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_ring *tx_ring;

	if (skb_put_padto(skb, SXE_SKB_MIN_LEN)) {
		ret = NETDEV_TX_OK;
		goto l_end;
	}

	tx_ring = adapter->tx_ring_ctxt.ring[skb_get_queue_mapping(skb)];
	LOG_DEBUG_BDF("sxe xmit start, ring idx=%u, ring_reg=%u\n",
				tx_ring->idx, tx_ring->reg_idx);

#ifdef SXE_DRIVER_TRACE
	SXE_TRACE_TX(tx_ring->idx, SXE_TRACE_LAB_TX_START);
#endif

	SKB_DUMP(skb);

	ret = sxe_ring_xmit(skb, netdev, tx_ring);
	if (ret) {
		LOG_ERROR_BDF("sxe xmit failed, ring idx=%u, status=%x\n",
						tx_ring->idx, ret);
	} else {
		LOG_DEBUG_BDF("sxe xmit end, ring idx=%u\n", tx_ring->idx);
	}

#ifdef SXE_DRIVER_TRACE
	if(!ret) {
		SXE_TRACE_TX(tx_ring->idx, SXE_TRACE_LAB_TX_END);
	}
#endif

l_end:
	return ret;
}

static inline void sxe_tx_skb_unmap(struct sxe_ring *ring,
				struct sxe_tx_buffer *tx_buffer)
{
	dma_unmap_single(ring->dev,
			dma_unmap_addr(tx_buffer, dma),
			dma_unmap_len(tx_buffer, len),
			DMA_TO_DEVICE);
	dma_unmap_len_set(tx_buffer, len, 0);

	return;
}

static inline void sxe_tx_desc_buf_update(
					struct sxe_ring *ring,
					struct sxe_tx_buffer **tx_buffer,
					union sxe_tx_data_desc **tx_desc,
					u32 *next_to_clean)
{
	(*tx_buffer)++;
	(*tx_desc)++;
	++(*next_to_clean);
	if (unlikely(!(*next_to_clean))) {
		*next_to_clean -= ring->depth;
		*tx_buffer      = ring->tx_buffer_info;
		*tx_desc        = SXE_TX_DESC(ring, 0);
	}

	return;
}

static void sxe_tx_ring_unmap(struct sxe_ring *ring,
				s32 napi_budget,
				u16 *budget,
				struct sxe_ring_stats *ring_stats)
{
	union sxe_tx_data_desc *tx_desc;
	union sxe_tx_data_desc *eop_desc;
	u32 next_to_clean = ring->next_to_clean;
	struct sxe_tx_buffer *tx_buffer;
	struct sxe_adapter *adapter = netdev_priv(ring->netdev);

	tx_buffer = &ring->tx_buffer_info[next_to_clean];
	tx_desc = SXE_TX_DESC(ring, next_to_clean);
	next_to_clean -= ring->depth;

	LOG_DEBUG_BDF("tx ring clean start: ring idx=%u, reg_idx=%u, next_to_use=%d, "
		"next_to_clean=%d, budget=%d, next_to_watch=%p, "
		"desc.wb.nxtseq_seed=%#08x, desc.wb.status=%#08x\n",
		ring->idx, ring->idx, ring->next_to_use, ring->next_to_clean,
		*budget, tx_buffer->next_to_watch, tx_desc->wb.nxtseq_seed,
		tx_desc->wb.status);

	do {
		eop_desc = tx_buffer->next_to_watch;
		if (!eop_desc) {
			break;
		}

		smp_rmb();

		if (!(eop_desc->wb.status & cpu_to_le32(SXE_TX_DESC_STAT_DD))) {
			break;
		}

		tx_buffer->next_to_watch = NULL;

		ring_stats->bytes   += tx_buffer->bytecount;
		ring_stats->packets += tx_buffer->gso_segs;
#ifdef SXE_IPSEC_CONFIGURE
		struct sxe_adapter *adapter = netdev_priv(ring->netdev);
		if (tx_buffer->tx_features & SXE_TX_FEATURE_IPSEC) {
			adapter->stats.sw.tx_ipsec++;
		}
#endif

#ifdef HAVE_XDP_SUPPORT
		if (ring_is_xdp(ring)) {
			xdp_return_frame(tx_buffer->xdpf);
		} else {
			napi_consume_skb(tx_buffer->skb, napi_budget);
		}
#else
		napi_consume_skb(tx_buffer->skb, napi_budget);
#endif
		LOG_DEBUG_BDF("tx ring clean: budget=%d, bytes=%llu, packet=%llu\n",
			*budget, ring_stats->bytes, ring_stats->packets);

		sxe_tx_skb_unmap(ring, tx_buffer);
		while (tx_desc != eop_desc) {
			sxe_tx_desc_buf_update(ring, &tx_buffer,
					&tx_desc, &next_to_clean);

			if (dma_unmap_len(tx_buffer, len)) {
				sxe_tx_skb_unmap(ring, tx_buffer);
			}
		}
		sxe_tx_desc_buf_update(ring, &tx_buffer,
					&tx_desc, &next_to_clean);

		prefetch(tx_desc);

		--*budget;
	}while (likely(*budget));

	next_to_clean += ring->depth;
	ring->next_to_clean = next_to_clean;
	LOG_DEBUG_BDF("tx ring clean end: ring idx=%u, reg_idx=%u, next_to_use=%d, "
			"next_to_clean=%d, budget=%d\n",
		ring->idx, ring->reg_idx, ring->next_to_use,
		ring->next_to_clean, *budget);

	return;
}

bool sxe_tx_ring_pending(struct sxe_adapter *adapter)
{
	u32 i;
	bool ret = false;
	struct sxe_ring **tx_ring = adapter->tx_ring_ctxt.ring;
	struct sxe_ring **xdp_ring = adapter->xdp_ring_ctxt.ring;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		if (tx_ring[i]->next_to_use != tx_ring[i]->next_to_clean) {
			ret = true;
			LOG_DEV_DEBUG("tx ring %d, next_to_use %d, next_to_clean %d, pending.\n",
				i, tx_ring[i]->next_to_use, tx_ring[i]->next_to_clean);
			goto l_end;
		}
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		if (xdp_ring[i]->next_to_use != xdp_ring[i]->next_to_clean) {
			ret = true;
			LOG_DEV_DEBUG("tx ring %d, next_to_use %d, next_to_clean %d, pending.\n",
				i, xdp_ring[i]->next_to_use, xdp_ring[i]->next_to_clean);
			goto l_end;
		}
	}

l_end:
	return ret;
}

static u64 sxe_tx_ring_pending_get(struct sxe_ring *ring)
{
	u32 head, tail;

	head = ring->next_to_clean;
	tail = ring->next_to_use;

	return ((head <= tail) ? tail : tail + ring->depth) - head;
}

static inline bool sxe_detect_tx_hang(struct sxe_ring *ring)
{
	bool ret;
	u32 tx_done     = ring->stats.packets;
	u32 tx_done_old = ring->tx_stats.tx_done_old;
	u32 tx_pending  = sxe_tx_ring_pending_get(ring);

	SXE_TX_HANG_CHECK_COMPLETE(ring);

	if (tx_done_old == tx_done && tx_pending) {
		ret = test_and_set_bit(SXE_HANG_CHECK_ARMED, &ring->state);
		goto l_end;
	}

	ring->tx_stats.tx_done_old = tx_done;

	clear_bit(SXE_HANG_CHECK_ARMED, &ring->state);

	ret = false;
l_end:
	return ret;
}

static void sxe_tx_timeout_reset(struct sxe_adapter *adapter)
{
	if (!test_bit(SXE_DOWN, &adapter->state)) {
		set_bit(SXE_RESET_REQUESTED, &adapter->monitor_ctxt.state);
		LOG_MSG_WARN(drv, "initiating reset due to tx timeout\n");
		sxe_monitor_work_schedule(adapter);
	}

	return;
}

static bool sxe_blocked_tx_ring_find(struct sxe_adapter *adapter)
{
	u32 i;
	struct net_device *netdev = adapter->netdev;
	struct netdev_queue *q;
	unsigned long trans_start;

	for (i = 0; i < netdev->num_tx_queues; i++) {
		q = netdev_get_tx_queue(netdev, i);
		trans_start = q->trans_start;
		if (netif_xmit_stopped(q) && \
			time_after(jiffies,
				(trans_start + netdev->watchdog_timeo))) {
			LOG_WARN_BDF("tx_timeout: netdev hang queue %d found\n", i);
			return true;
		}
	}

	LOG_INFO_BDF("tx_timeout: no netdev hang queue found\n");
	return false;
}

#ifdef HAVE_TIMEOUT_TXQUEUE_IDX
void sxe_tx_timeout(struct net_device *netdev, u32 __always_unused txqueue)
#else
void sxe_tx_timeout(struct net_device *netdev)
#endif
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	if (!sxe_blocked_tx_ring_find(adapter)) {
		goto l_end;
	}

	sxe_tx_timeout_reset(adapter);

l_end:
	return;
}

static inline bool sxe_tx_hang_handle(struct sxe_adapter *adapter,
				struct sxe_ring *ring)
{
	u32 tdh, tdt;
	bool ret = false;
	struct sxe_hw *hw = &adapter->hw;

	if (SXE_DETECT_TX_HANG_NEED(ring) && sxe_detect_tx_hang(ring)) {
#ifdef HAVE_XDP_SUPPORT
		if (!ring_is_xdp(ring))
#endif
			netif_stop_subqueue(ring->netdev, ring->idx);

		hw->dma.ops->tx_ring_info_get(hw, ring->reg_idx, &tdh, &tdt);
		LOG_MSG_ERR(drv, "detected Tx hang %s\n"
			"  Tx ring              <%u>\n"
			"  ring reg             <%u>\n"
			"  TDH, TDT             <%u>, <%u>\n"
			"  next_to_use          <%u>\n"
			"  next_to_clean        <%u>\n"
			"tx_buffer_info[next_to_clean]\n"
			"  time_stamp           <%lx>\n"
			"  jiffies              <%llx>\n",
			ring_is_xdp(ring) ? "(XDP)" : "",
			ring->idx, ring->reg_idx,
			tdh, tdt,
			ring->next_to_use, ring->next_to_clean,
			ring->tx_buffer_info[ring->next_to_clean].time_stamp,
			(U64)jiffies);
		LOG_MSG_INFO(probe, "tx hang %llu detected on queue %d, resetting adapter\n",
			adapter->stats.sw.reset_work_trigger_cnt + 1,
			ring->idx);

		sxe_tx_timeout_reset(adapter);

		ret = true;
	}

	return ret;
}

static inline void sxe_tx_pkt_stats_update(struct sxe_irq_rate *irq_rate,
					struct sxe_ring *ring,
					struct sxe_ring_stats *stats)
{
	u64_stats_update_begin(&ring->syncp);
	ring->stats.bytes += stats->bytes;
	ring->stats.packets += stats->packets;
	u64_stats_update_end(&ring->syncp);
	irq_rate->total_bytes += stats->bytes;
	irq_rate->total_packets += stats->packets;

#ifdef SXE_IPSEC_CONFIGURE
	struct sxe_adapter *adapter = netdev_priv(ring->netdev);
	adapter->ipsec.rx_ipsec += adapter->stats.sw.rx_ipsec;
#endif

	return;
}

bool sxe_tx_ring_irq_clean(struct sxe_irq_data *irq,
				struct sxe_ring *ring, s32 napi_budget)
{
	bool ret;
	struct sxe_adapter *adapter = irq->adapter;
	struct netdev_queue *queue;
	struct sxe_ring_stats ring_stats = {};
	u16 budget = irq->tx.work_limit;

	if (test_bit(SXE_DOWN, &adapter->state)) {
		ret = true;
		goto l_end;
	}

	sxe_tx_ring_unmap(ring, napi_budget, &budget, &ring_stats);

	sxe_tx_pkt_stats_update(&irq->tx.irq_rate, ring, &ring_stats);

	if (sxe_tx_hang_handle(adapter, ring)) {
		ret = true;
		goto l_end;
	}

#ifdef HAVE_XDP_SUPPORT
	if (ring_is_xdp(ring)) {
		ret = !!budget;
		goto l_end;
	}
#endif
	queue = netdev_get_tx_queue(ring->netdev, ring->idx);
	netdev_tx_completed_queue(queue, ring_stats.packets, ring_stats.bytes);

	if (unlikely(ring_stats.packets &&
				netif_carrier_ok(ring->netdev) &&
				(sxe_desc_unused_count(ring) >= \
					SXE_TX_WAKE_THRESHOLD))) {
		smp_mb();

		if (__netif_subqueue_stopped(ring->netdev, ring->idx)
			&& !test_bit(SXE_DOWN, &adapter->state)) {
			netif_wake_subqueue(ring->netdev, ring->idx);
			++ring->tx_stats.restart_queue;
			LOG_WARN_BDF("\n\n ring idx=%u, wake_up\n\n", ring->idx);
		}
	}

	ret = !!budget;

l_end:
	return ret;
}

void sxe_hw_tx_disable(struct sxe_adapter *adapter)
{
	u8 reg_idx;
	bool link_up;
	u32 i, wait_loop, txdctl, link_speed;
	struct sxe_ring *ring;
	unsigned long wait_delay, delay_interval;
	struct sxe_hw *hw = &adapter->hw;

	LOG_DEBUG_BDF("tx hw disable\n");

	if (sxe_is_hw_fault(hw)) {
		goto l_end;
	}

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		ring = adapter->tx_ring_ctxt.ring[i];
		reg_idx = ring->reg_idx;

		hw->dma.ops->tx_ring_switch_not_polling(hw, reg_idx, false);
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		ring = adapter->xdp_ring_ctxt.ring[i];
		reg_idx = ring->reg_idx;

		hw->dma.ops->tx_ring_switch_not_polling(hw, reg_idx, false);
	}

	sxe_link_info_get(adapter, &link_speed, &link_up);
	if (!link_up) {
		goto l_end;
	}

	delay_interval = sxe_pcie_timeout_poll(adapter->pdev, hw);
	wait_delay = delay_interval;

	wait_loop = SXE_MAX_TXRX_DESC_POLL;
	while (wait_loop--) {
		usleep_range(wait_delay, wait_delay + 10);
		wait_delay += delay_interval * 2;
		txdctl = 0;

		for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
			ring = adapter->tx_ring_ctxt.ring[i];
			reg_idx = ring->reg_idx;

			txdctl |= hw->dma.ops->tx_desc_ctrl_get(hw, reg_idx);
		}
		for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
			ring = adapter->xdp_ring_ctxt.ring[i];
			reg_idx = ring->reg_idx;

			txdctl |= hw->dma.ops->tx_desc_ctrl_get(hw, reg_idx);
		}

		if (!(txdctl & SXE_TXDCTL_ENABLE)) {
			goto l_end;
		}
	}

	LOG_MSG_ERR(drv, "TXDCTL.ENABLE for one or more queues not cleared within the polling period\n");

l_end:
	return;
}

void sxe_tx_buffer_dump(struct sxe_adapter *adapter)
{
	u32 i;
	struct sxe_ring *ring;
	struct sxe_tx_buffer *tx_buffer;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		ring = adapter->tx_ring_ctxt.ring[i];
		tx_buffer = &ring->tx_buffer_info[ring->next_to_clean];

		LOG_DEV_INFO(" %5d %5X %5X %016llX %08X %p %016llX\n",
		i, ring->next_to_use, ring->next_to_clean,
		(u64)dma_unmap_addr(tx_buffer, dma),
		dma_unmap_len(tx_buffer, len),
		tx_buffer->next_to_watch,
		(u64)tx_buffer->time_stamp);
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		ring = adapter->xdp_ring_ctxt.ring[i];
		tx_buffer = &ring->tx_buffer_info[ring->next_to_clean];

		LOG_DEV_INFO(" %5d %5X %5X %016llX %08X %p %016llX\n",
		i, ring->next_to_use, ring->next_to_clean,
		(u64)dma_unmap_addr(tx_buffer, dma),
		dma_unmap_len(tx_buffer, len),
		tx_buffer->next_to_watch,
		(u64)tx_buffer->time_stamp);
	}

	return;
}
