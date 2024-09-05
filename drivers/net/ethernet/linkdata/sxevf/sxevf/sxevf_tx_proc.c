
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
#include <net/mpls.h>
#include <linux/vmalloc.h>

#include "sxevf.h"
#ifdef HAVE_NO_OVERFLOW_H
#include <sxe_compat_overflow.h>
#else
#include <linux/overflow.h>
#endif
#include "sxe_log.h"
#include "sxevf_tx_proc.h"
#include "sxevf_irq.h"
#include "sxevf_hw.h"
#include "sxevf_pci.h"
#include "sxevf_monitor.h"
#include "sxevf_csum.h"
#include "sxevf_ipsec.h"
#include "sxevf_debug.h"

#define SXEVF_IPV4     4
#define SXEVF_SKB_MINI_LEN 17

void sxevf_tx_ring_buffer_clean(struct sxevf_ring *ring)
{
	union  sxevf_tx_data_desc *eop_desc, *tx_desc;
	u16    ntc                      = ring->next_to_clean;
	struct sxevf_tx_buffer *tx_buffer = &ring->tx_buffer_info[ntc];

	while (ntc != ring->next_to_use) {
#ifdef HAVE_XDP_SUPPORT
		if (vf_ring_is_xdp(ring)) {
			page_frag_free(tx_buffer->data);
		} else {
			dev_kfree_skb_any(tx_buffer->skb);
		}
#else
		dev_kfree_skb_any(tx_buffer->skb);
#endif
		dma_unmap_single(ring->dev, dma_unmap_addr(tx_buffer, dma),
				dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);

		eop_desc = tx_buffer->next_to_watch;
		tx_desc  = SXEVF_TX_DESC(ring, ntc);

		while (tx_desc != eop_desc) {
			tx_buffer++;
			tx_desc++;
			ntc++;
			if (unlikely(ntc == ring->depth)) {
				ntc = 0;
				tx_buffer = ring->tx_buffer_info;
				tx_desc   = SXEVF_TX_DESC(ring, 0);
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

	ring->next_to_use   = 0;
	ring->next_to_clean = 0;

	return;
}

void sxevf_tx_ring_free(struct sxevf_ring *ring)
{
	sxevf_tx_ring_buffer_clean(ring);

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

void sxevf_tx_resources_free(struct sxevf_adapter *adapter)
{
	u32 i;
	struct sxevf_ring **tx_ring = adapter->tx_ring_ctxt.ring;
	struct sxevf_ring **xdp_ring = adapter->xdp_ring_ctxt.ring;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		if (tx_ring[i]->desc.base_addr) {
			sxevf_tx_ring_free(tx_ring[i]);
		}
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		if (xdp_ring[i]->desc.base_addr) {
			sxevf_tx_ring_free(xdp_ring[i]);
		}
	}

	return;
}

static inline void sxevf_tx_buffer_init(struct sxevf_ring *ring)
{
	memset(ring->tx_buffer_info, 0,
	       sizeof(struct sxevf_tx_buffer) * ring->depth);
	return;
}

s32 sxevf_tx_ring_alloc(struct sxevf_ring *ring)
{
	s32 ret;
	u32 size = sizeof(struct sxevf_tx_buffer) * ring->depth;
	struct sxevf_adapter *adapter = netdev_priv(ring->netdev);

	ring->tx_buffer_info = vmalloc(size);
	if (!ring->tx_buffer_info) {
		ret = -ENOMEM;
		goto l_err;
	}

	u64_stats_init(&ring->syncp);

	ring->size = ring->depth * sizeof(union sxevf_tx_data_desc);
	ring->size = ALIGN(ring->size, SXEVF_DESC_ALIGN_4K);

	ring->desc.base_addr = dma_alloc_coherent(ring->dev, ring->size,
					&ring->desc.dma, GFP_KERNEL);
	if (!ring->desc.base_addr) {
		ret = -ENOMEM;
		goto l_free;
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

static s32 sxevf_tx_resources_alloc(struct sxevf_adapter *adapter)
{
	s32 ret;
	u32 i, j;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		ret = sxevf_tx_ring_alloc(adapter->tx_ring_ctxt.ring[i]);
		if (ret < 0) {
			LOG_DEV_ERR("allocation for Tx Queue %d failed\n", i);
			goto l_tx_free;
		}
	}

	for (j = 0; j < adapter->xdp_ring_ctxt.num; j++) {
		ret = sxevf_tx_ring_alloc(adapter->xdp_ring_ctxt.ring[j]);
		if (ret < 0) {
			LOG_DEV_ERR("allocation for xdp Queue %d failed\n", j);
			goto l_xdp_free;
		}
	}

	return 0;

l_xdp_free:
	while (j--) {
		sxevf_tx_ring_free(adapter->xdp_ring_ctxt.ring[j]);
	}

l_tx_free:
	while (i--) {
		sxevf_tx_ring_free(adapter->tx_ring_ctxt.ring[i]);
	}

	return ret;
}

s32 sxevf_tx_ring_depth_reset(struct sxevf_adapter *adapter, u32 tx_cnt)
{
	s32 ret;
	u32 i, j, tx_ring_cnt;
	struct sxevf_ring *temp_ring;
	struct sxevf_ring **tx_ring   = adapter->tx_ring_ctxt.ring;
	struct sxevf_ring **xdp_ring  = adapter->xdp_ring_ctxt.ring;

	tx_ring_cnt = adapter->tx_ring_ctxt.num + adapter->xdp_ring_ctxt.num;
	temp_ring = vmalloc(array_size(tx_ring_cnt, sizeof(struct sxevf_ring)));
	if (!temp_ring) {
		LOG_ERROR("vmalloc failed, size=%lu\n",
			array_size(tx_ring_cnt, sizeof(struct sxevf_ring)));
		ret = -ENOMEM;
		goto l_end;
	}

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		memcpy(&temp_ring[i], tx_ring[i], sizeof(struct sxevf_ring));
		temp_ring[i].depth = tx_cnt;
		ret = sxevf_tx_ring_alloc(&temp_ring[i]);
		if (ret < 0) {
			LOG_ERROR("tx ring alloc failed, tx ring idx=%d\n", i);
			goto l_tx_free;
		}
	}

	for (j = 0; j < adapter->xdp_ring_ctxt.num; j++, i++) {
		memcpy(&temp_ring[i], xdp_ring[j], sizeof(struct sxevf_ring));
		temp_ring[i].depth = tx_cnt;
		ret = sxevf_tx_ring_alloc(&temp_ring[i]);
		if (ret < 0) {
			LOG_ERROR("xdp ring alloc failed, xdp ring idx=%d\n", j);
			goto l_tx_free;
		}
	}

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		sxevf_tx_ring_free(tx_ring[i]);
		memcpy(tx_ring[i], &temp_ring[i], sizeof(struct sxevf_ring));
	}

	for (j = 0; j < adapter->xdp_ring_ctxt.num; j++, i++) {
		sxevf_tx_ring_free(xdp_ring[j]);
		memcpy(xdp_ring[j], &temp_ring[i], sizeof(struct sxevf_ring));
	}

	adapter->tx_ring_ctxt.depth  = tx_cnt;
	adapter->xdp_ring_ctxt.depth = tx_cnt;
	ret = 0;
	goto l_temp_free;

l_tx_free:
	while (i--) {
		sxevf_tx_ring_free(&temp_ring[i]);
	}

l_temp_free:
	vfree(temp_ring);

l_end:
	return ret;
}

static void sxevf_tx_ring_reg_configure(struct sxevf_adapter *adapter,
				struct sxevf_ring *ring)
{
	u32 reg_idx = ring->reg_idx;
	struct sxevf_hw *hw = &adapter->hw;
	u32 dma_len = ring->depth * sizeof(union sxevf_tx_data_desc);
	u32 hthresh, pthresh;
	u32 wthresh = 0;

	ring->desc.tail = adapter->hw.reg_base_addr + SXEVF_TDT(reg_idx);

	hw->dma.ops->tx_writeback_off(hw, reg_idx);

	if (adapter->irq_ctxt.rx_irq_interval) {
		wthresh = SXEVF_TX_DESC_WRITEBACK_THRESH_8;
	}

	hthresh = SXEVF_TX_DESC_HOST_THRESH_1;
	pthresh = SXEVF_TX_DESC_PREFETCH_THRESH_32;

	hw->dma.ops->tx_desc_thresh_set(hw, reg_idx, wthresh, hthresh, pthresh);

	hw->dma.ops->tx_ring_desc_configure(hw, dma_len,
				(u64)ring->desc.dma, reg_idx);

	hw->dma.ops->tx_ring_switch(hw, reg_idx, true);
	return;
}

void sxevf_hw_tx_configure(struct sxevf_adapter *adapter)
{
	u32 i;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		clear_bit(SXEVF_HANG_CHECK_ARMED,
				&adapter->tx_ring_ctxt.ring[i]->state);
		clear_bit(SXEVF_TX_XDP_RING_PRIMED,
				&adapter->tx_ring_ctxt.ring[i]->state);
		sxevf_tx_buffer_init(adapter->tx_ring_ctxt.ring[i]);
		sxevf_tx_ring_reg_configure(adapter,adapter->tx_ring_ctxt.ring[i]);
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++){
		clear_bit(SXEVF_HANG_CHECK_ARMED,
				&adapter->xdp_ring_ctxt.ring[i]->state);
		clear_bit(SXEVF_TX_XDP_RING_PRIMED,
				&adapter->xdp_ring_ctxt.ring[i]->state);
		sxevf_tx_buffer_init(adapter->xdp_ring_ctxt.ring[i]);
		sxevf_tx_ring_reg_configure(adapter,adapter->xdp_ring_ctxt.ring[i]);
	}

	return;
}

s32 sxevf_tx_configure(struct sxevf_adapter *adapter)
{
	s32 ret;

	ret = sxevf_tx_resources_alloc(adapter);
	if (ret) {
		LOG_ERROR("tx ring init failed, ret = %d\n", ret);
		goto l_err;
	}

	sxevf_hw_tx_configure(adapter);

	ret = netif_set_real_num_tx_queues(adapter->netdev,
				adapter->tx_ring_ctxt.num);
	if (ret) {
		LOG_ERROR("netif_set_real_num_tx_queues failed, ret = %d\n", ret);
		sxevf_tx_resources_free(adapter);
	}

l_err:
	return ret;
}

static void sxevf_tx_ctxt_desc_set(struct sxevf_ring *tx_ring,
			struct sxevf_tx_context_desc *ctxt_desc)
{
	struct sxevf_tx_context_desc *context_desc;
	u16 i = tx_ring->next_to_use;

	context_desc = SXEVF_TX_CTXTDESC(tx_ring, i);

	i++;
	tx_ring->next_to_use = (i < tx_ring->depth) ? i : 0;

	ctxt_desc->type_tucmd_mlhl |= SXEVF_TXD_DTYP_CTXT;

	context_desc->vlan_macip_lens	= cpu_to_le32(ctxt_desc->vlan_macip_lens);
	context_desc->sa_idx            = cpu_to_le32(ctxt_desc->sa_idx);
	context_desc->type_tucmd_mlhl	= cpu_to_le32(ctxt_desc->type_tucmd_mlhl);
	context_desc->mss_l4len_idx	= cpu_to_le32(ctxt_desc->mss_l4len_idx);

	LOG_DEBUG("contxt desc, ring=%u, ntu=%u, ntc=%u, vlan_macip_lens=%#x, "
		"sa_idx=%#x, type_tucmd_mlhl=%#x, mss_l4len_idx=%x\n",
		tx_ring->idx, tx_ring->next_to_use, tx_ring->next_to_clean,
		context_desc->vlan_macip_lens,
		context_desc->sa_idx,
		context_desc->type_tucmd_mlhl,
		context_desc->mss_l4len_idx);

	return;
}

static s32 sxevf_tso(struct sxevf_ring *tx_ring,
			struct sxevf_tx_buffer *first_buff,
			struct sxevf_tx_context_desc *ctxt_desc,
			u8 *hdr_len)
{
	s32 ret;
	u16 tucmd;
	union sxevf_ip_hdr ip;
	union sxevf_l4_hdr l4;
	u8 *csum_start, *trans_start;
	u32 mss_l4len, paylen, l4_offset, len;
	struct sk_buff *skb = first_buff->skb;

	if (skb->ip_summed != CHECKSUM_PARTIAL || !skb_is_gso(skb)) {
		ret = 0;
		goto  l_end;
	}

	LOG_DEBUG("tso start, ring[%d]\n", tx_ring->idx);

	ret = skb_cow_head(skb, 0);
	if (ret < 0) {
		LOG_ERROR("skb cow head failed, ret=%d\n", ret);
		goto l_end;
	}

	if (eth_p_mpls(first_buff->protocol)) {
		ip.hdr = skb_inner_network_header(skb);
	} else {
		ip.hdr = skb_network_header(skb);
	}

	tucmd = SXEVF_TX_CTXTD_TUCMD_L4T_TCP;

	if (ip.v4->version == SXEVF_IPV4) {
		csum_start = skb_checksum_start(skb);
		trans_start = ip.hdr + (ip.v4->ihl * 4);
		len = csum_start - trans_start;

		ip.v4->check = (skb_shinfo(skb)->gso_type & SKB_GSO_PARTIAL) ?
					csum_fold(csum_partial(trans_start,
					len, 0)) : 0;

		LOG_INFO("tso ipv4 ip.v4->check=%u, gso_type=%x\n",
				ip.v4->check, skb_shinfo(skb)->gso_type);
		tucmd |= SXEVF_TX_CTXTD_TUCMD_IPV4;

		ip.v4->tot_len = 0;
		first_buff->tx_features |= SXEVF_TX_FEATURE_TSO |
				   SXEVF_TX_FEATURE_CSUM |
				   SXEVF_TX_FEATURE_IPV4;
	} else {
		ip.v6->payload_len = 0;
		first_buff->tx_features |= SXEVF_TX_FEATURE_TSO |
				   SXEVF_TX_FEATURE_CSUM;
	}

	l4.hdr = skb_checksum_start(skb);
	l4_offset = l4.hdr - skb->data;

	*hdr_len = (l4.tcp->doff * 4) + l4_offset;

	paylen = skb->len - l4_offset;
	csum_replace_by_diff(&l4.tcp->check, (__force __wsum)htonl(paylen));

	first_buff->gso_segs = skb_shinfo(skb)->gso_segs;
	first_buff->bytecount += (first_buff->gso_segs - 1) * (*hdr_len);

	sxevf_ctxt_desc_iplen_set(ctxt_desc, (l4.hdr - ip.hdr));
	sxevf_ctxt_desc_maclen_set(ctxt_desc, (ip.hdr - skb->data));
	sxevf_ctxt_desc_tucmd_set(ctxt_desc, tucmd);
	mss_l4len = (*hdr_len - l4_offset) << SXEVF_TX_CTXTD_L4LEN_SHIFT;
	mss_l4len |= skb_shinfo(skb)->gso_size << SXEVF_TX_CTXTD_MSS_SHIFT;
	sxevf_ctxt_desc_mss_l4len_set(ctxt_desc, mss_l4len);

	ret = 1;
l_end:
	return ret;
}

static inline u16 sxevf_desc_unused_count(struct sxevf_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->depth) + ntc - ntu - 1;
}

static int sxevf_maybe_stop_tx(struct sxevf_ring *ring, u16 size)
{
	int ret = 0;

	netif_stop_subqueue(ring->netdev, ring->idx);

	smp_mb();

	if (likely(sxevf_desc_unused_count(ring) < size)) {
		ret = -EBUSY;
		goto l_end;
	}

	netif_start_subqueue(ring->netdev, ring->idx);

	++ring->tx_stats.restart_queue;

l_end:
	return ret;
}

STATIC netdev_tx_t sxevf_ring_maybe_stop_tx(struct sk_buff *skb,
					struct sxevf_ring *tx_ring)
{
	u16 need_num;
	netdev_tx_t ret = NETDEV_TX_OK;
	u16  desc_cnt = SXEVF_TX_DESC_USE_COUNT(skb_headlen(skb));
#if PAGE_SIZE > SXEVF_DATA_PER_DESC_SIZE_MAX
	u16 i;


	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		desc_cnt += SXEVF_TX_DESC_USE_COUNT(
				skb_frag_size(&skb_shinfo(skb)->frags[i]));
	}
#else
	desc_cnt += skb_shinfo(skb)->nr_frags;
#endif 
	need_num = desc_cnt + SXEVF_TX_NON_DATA_DESC_NUM;

	if (unlikely(sxevf_desc_unused_count(tx_ring) < need_num)) {
		if (sxevf_maybe_stop_tx(tx_ring, need_num) < 0) {
			ret = NETDEV_TX_BUSY;
		}

		tx_ring->tx_stats.tx_busy++;
	}

	return ret;
}

static inline struct sxevf_tx_buffer *sxevf_tx_first_buffer_get(
					struct sk_buff *skb,
					struct sxevf_ring *ring)
{
	struct sxevf_tx_buffer *first_buff;

	first_buff            = &ring->tx_buffer_info[ring->next_to_use];
	first_buff->skb       = skb;
	first_buff->bytecount = skb->len;
	first_buff->gso_segs  = 1;

	return first_buff;
}

static u32 sxevf_tx_cmd_type(struct sk_buff *skb, u32 features)
{
	u32 cmd_type = SXEVF_TX_DESC_TYPE_DATA |
				SXEVF_TX_DESC_IFCS;

	if (features & SXEVF_TX_FEATURE_VLAN) {
		cmd_type |= cpu_to_le32(SXEVF_TXD_DCMD_VLE);
	}

	if (features & SXEVF_TX_FEATURE_TSO) {
		cmd_type |= cpu_to_le32(SXEVF_TXD_DCMD_TSE);
	}

	return cmd_type;
}

static void sxevf_tx_desc_offload_setup(u32 flags,
					unsigned int paylen,
					union sxevf_tx_data_desc *tx_desc)
{
	u32 olinfo_status = paylen << SXEVF_TX_DESC_PAYLEN_SHIFT;

	if (flags & SXEVF_TX_FEATURE_CSUM) {
		olinfo_status |= cpu_to_le32(SXEVF_TXD_POPTS_TXSM);
	}

	if (flags & SXEVF_TX_FEATURE_IPV4) {
		olinfo_status |= cpu_to_le32(SXEVF_TXD_POPTS_IXSM);
	}

	olinfo_status |= cpu_to_le32(SXEVF_TXD_CC);

	tx_desc->read.olinfo_status = olinfo_status;

	return;
}

static inline void sxevf_tx_desc_update(
				struct sxevf_ring *ring,
				union sxevf_tx_data_desc **desc,
				u16 *next_to_use)
{
	++(*next_to_use);
	++(*desc);
	if (ring->depth == *next_to_use) {
		*desc = SXEVF_TX_DESC(ring, 0);
		*next_to_use = 0;
	}
	(*desc)->read.olinfo_status = 0;

	return;
}

static void sxevf_tx_dma_err(struct sxevf_ring *ring,
			struct sxevf_tx_buffer *first_buffer,
			u16    next_to_use)
{
	struct sxevf_tx_buffer *tx_buffer;

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

static s32 sxevf_tx_desc_setup(struct sxevf_ring *ring,
				struct sk_buff *skb,
				struct sxevf_tx_buffer *first_buffer,
				union sxevf_tx_data_desc **desc,
				u16    *next_to_use)
{
	struct sxevf_adapter *adapter = netdev_priv(ring->netdev);
	dma_addr_t dma;
	skb_frag_t *frag;
	u32 map_size       = skb_headlen(skb); 
	u32 remaining_size = skb->data_len;    
	u32 cmd_type       = sxevf_tx_cmd_type(skb, first_buffer->tx_features);
	struct sxevf_tx_buffer *tx_buffer = first_buffer;

	LOG_DEBUG("skb dma map start, line_size=%u,"
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

		while (unlikely(map_size > SXEVF_DATA_PER_DESC_SIZE_MAX)) {
			(*desc)->read.cmd_type_len = \
			cpu_to_le32(cmd_type ^ SXEVF_DATA_PER_DESC_SIZE_MAX);

			sxevf_tx_desc_update(ring, desc, next_to_use);
			dma            += SXEVF_DATA_PER_DESC_SIZE_MAX;
			map_size       -= SXEVF_DATA_PER_DESC_SIZE_MAX;

			(*desc)->read.buffer_addr = cpu_to_le64(dma);
		}

		if (likely(!remaining_size)) {
			cmd_type |= map_size | SXEVF_TX_DESC_CMD;
			(*desc)->read.cmd_type_len = cpu_to_le32(cmd_type);
			LOG_DEBUG("skb dma map, current_map_size=%u, remaining_size=%u, "
			"desc_ptr=%p, dma_addr=%#llx, desc.buffer_addr = %#llx, "
			"desc.cmdtype=%#x, desc.olinfo_status=%#x\n",
				map_size, remaining_size, *desc,
				(U64)dma, (*desc)->read.buffer_addr,
				(*desc)->read.cmd_type_len,
				(*desc)->read.olinfo_status);
			break;
		}

		(*desc)->read.cmd_type_len = cpu_to_le32(cmd_type ^ map_size);

		LOG_DEBUG("skb dma map, current_map_size=%u, remaining_size=%u, "
			"desc_ptr=%p, dma_addr=%#llx, desc.buffer_addr = %#llx, "
			"desc.cmdtype=%#x, desc.olinfo_status=%#x\n",
				map_size, remaining_size, *desc,
				(U64)dma, (*desc)->read.buffer_addr,
				(*desc)->read.cmd_type_len,
				(*desc)->read.olinfo_status);

		sxevf_tx_desc_update(ring, desc, next_to_use);

		map_size        = skb_frag_size(frag);
		remaining_size -= map_size;
		dma = skb_frag_dma_map(ring->dev, frag, 0,
						map_size, DMA_TO_DEVICE);

		tx_buffer = &ring->tx_buffer_info[*next_to_use];
	}

	LOG_DEBUG("skb dma map end\n");
	return 0;
l_dma_err:
	sxevf_tx_dma_err(ring, first_buffer, *next_to_use);
	return -ENOMEM;
}

static s32 sxevf_xmit_pkt(struct sxevf_ring *ring,
			struct sxevf_tx_buffer *first_buffer,
			const u8 hdr_len)
{
	s32 ret;
	struct sk_buff *skb = first_buffer->skb;
	u32    tx_features     = first_buffer->tx_features;
	u16    ntu          = ring->next_to_use;
	union sxevf_tx_data_desc *desc = SXEVF_TX_DESC(ring, ntu);

	sxevf_tx_desc_offload_setup(tx_features, skb->len - hdr_len, desc);

	ret = sxevf_tx_desc_setup(ring, skb, first_buffer, &desc, &ntu);
	if (ret) {
		goto l_end;
	}

	first_buffer->time_stamp = jiffies;
	skb_tx_timestamp(skb);

	wmb();

	first_buffer->next_to_watch = desc;

	ntu++;
	if (ntu == ring->depth) {
		ntu = 0;
	}
	ring->next_to_use = ntu;

	if (unlikely(sxevf_desc_unused_count(ring) < SXEVF_TX_DESC_NEEDED)) {
	 	ret = sxevf_maybe_stop_tx(ring, SXEVF_TX_DESC_NEEDED);
		if (ret < 0) {
			LOG_WARN("the desc is not enough in the ring[%u],"
				"to stop the ring, "
				"desc_cnt < SXEVF_TX_DESC_NEEDED[%ld]\n",
					ring->idx, SXEVF_TX_DESC_NEEDED);
		}
	}

	writel(ntu, ring->desc.tail);
	LOG_DEBUG("send directly, ring[%u]\n", ring->idx);

	LOG_DEBUG("tx end: ring idx=%u, next_to_use=%d, "
		"next_to_clean=%d, next_to_watch=%p\n",
		ring->idx, ring->next_to_use, ring->next_to_clean,
		first_buffer->next_to_watch);

	return 0;

l_end:
	return ret;
}

STATIC void sxevf_tx_vlan_process(struct sk_buff *skb,
		struct sxevf_tx_buffer *first_buffer,
		struct sxevf_tx_context_desc *ctxt_desc)
{
	u32 tx_features = 0;

	if (skb_vlan_tag_present(skb)) {
		tx_features |= SXEVF_TX_FEATURE_VLAN;
		sxevf_ctxt_desc_vlan_tag_set(ctxt_desc, skb_vlan_tag_get(skb));
		sxevf_ctxt_desc_maclen_set(ctxt_desc, skb_network_offset(skb));
	}

	first_buffer->tx_features = tx_features;
	first_buffer->protocol = vlan_get_protocol(skb);

	return;
}

static s32 sxevf_tx_feature_offload(struct sxevf_ring *ring,
				struct sk_buff *skb,
				struct sxevf_tx_buffer *first_buffer,
				u8  *hdr_len)
{
	s32 ret = 0;
	s32 need_tso;
	struct sxevf_tx_context_desc ctxt_desc = {0};

	sxevf_tx_vlan_process(skb, first_buffer, &ctxt_desc);

#ifdef SXE_IPSEC_CONFIGURE
	ret = sxevf_tx_ipsec_offload(ring, first_buffer, &ctxt_desc);
	if (ret) {
		LOG_ERROR("ring[%u] tx ipsec offload failed.(err:%d)\n",
			  ring->idx, ret);
		goto l_end;
	}
#endif

	need_tso = sxevf_tso(ring, first_buffer, &ctxt_desc, hdr_len);
	if (need_tso < 0) {
		LOG_ERROR("tso deal failed, ring->idx=%u\n", ring->idx);
		ret = need_tso;
		goto l_end;
	} else if (!need_tso) {
		sxevf_tx_csum_offload(ring, first_buffer, &ctxt_desc);
	}

	if (first_buffer->tx_features &
		(SXEVF_TX_FEATURE_VLAN |
		 SXEVF_TX_FEATURE_CSUM |
		 SXEVF_TX_FEATURE_TSO)) {

		sxevf_tx_ctxt_desc_set(ring, &ctxt_desc);
	}

l_end:
	return ret;
}

STATIC netdev_tx_t sxevf_ring_xmit(struct sk_buff *skb,
				struct sxevf_ring *ring)
{
	s32 res;
	u8  hdr_len = 0;
	netdev_tx_t ret = NETDEV_TX_OK;
	struct sxevf_tx_buffer *first_buffer;
	u8 *dst_mac = skb_header_pointer(skb, 0, 0, NULL);

	if (!dst_mac || is_link_local_ether_addr(dst_mac)) {
		LOG_ERROR("dst mac addr err\n");
		dev_kfree_skb_any(skb);
		goto l_end;
	}

	ret = sxevf_ring_maybe_stop_tx(skb, ring);
	if (ret != NETDEV_TX_OK) {
		LOG_ERROR("tx busy, ring idx=%u\n", ring->idx);
		goto l_end;
	}

	first_buffer = sxevf_tx_first_buffer_get(skb, ring);

	res = sxevf_tx_feature_offload(ring, skb, first_buffer, &hdr_len);
	if (res < 0) {
		LOG_ERROR("tx offload failed, ring->idx=%u\n", ring->idx);
		goto l_free;
	}

	if (sxevf_xmit_pkt(ring, first_buffer, hdr_len)) {
		LOG_ERROR("tx dma mapping err, ring idx=%u\n", ring->idx);
	}

	return NETDEV_TX_OK;

l_free:
	dev_kfree_skb_any(first_buffer->skb);
	first_buffer->skb = NULL;
l_end:
	return ret;
}

netdev_tx_t sxevf_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	netdev_tx_t ret = NETDEV_TX_OK;
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	struct sxevf_ring *tx_ring;

	if (skb->len <= 0) {
		dev_kfree_skb_any(skb);
		goto  l_end;
	}

	if (skb_put_padto(skb, SXEVF_SKB_MINI_LEN)) {
		goto l_end;
	}

	tx_ring = adapter->tx_ring_ctxt.ring[skb_get_queue_mapping(skb)];
	LOG_DEBUG("sxe xmit start, ring idx=%u\n", tx_ring->idx);

	 SKB_DUMP(skb);

	ret = sxevf_ring_xmit(skb, tx_ring);
	if (ret) {
		LOG_ERROR("sxe xmit failed, ring idx=%u, status=%x\n",
						tx_ring->idx, ret);
	} else {
		LOG_DEBUG("sxe xmit end, ring idx=%u\n", tx_ring->idx);
	}

l_end:
	return ret;
}

static inline void sxevf_tx_skb_unmap(struct sxevf_ring *ring,
				struct sxevf_tx_buffer *tx_buffer)
{
	dma_unmap_single(ring->dev,
			dma_unmap_addr(tx_buffer, dma),
			dma_unmap_len(tx_buffer, len),
			DMA_TO_DEVICE);
	dma_unmap_len_set(tx_buffer, len, 0);

	return;
}

static inline void sxevf_tx_desc_buf_update(
					struct sxevf_ring *ring,
					struct sxevf_tx_buffer **tx_buffer,
					union sxevf_tx_data_desc **tx_desc,
					u32 *next_to_clean)
{
	(*tx_buffer)++;
	(*tx_desc)++;
	++(*next_to_clean);
	if (unlikely(!(*next_to_clean))) {
		*next_to_clean -= ring->depth;
		*tx_buffer      = ring->tx_buffer_info;
		*tx_desc        = SXEVF_TX_DESC(ring, 0);
	}

	return;
}

STATIC void sxevf_tx_ring_unmap(struct sxevf_ring *ring,
				s32 napi_budget,
				u16 *budget,
				struct sxevf_ring_stats *ring_stats)
{
	union sxevf_tx_data_desc *tx_desc;
	union sxevf_tx_data_desc *eop_desc;
	u32 next_to_clean = ring->next_to_clean;
	struct sxevf_tx_buffer *tx_buffer;

	tx_buffer = &ring->tx_buffer_info[next_to_clean];
	tx_desc = SXEVF_TX_DESC(ring, next_to_clean);
	next_to_clean -= ring->depth;

	LOG_DEBUG("tx ring clean start: ring idx=%u, ring_reg_idx=%u, " 	"next_to_use=%d, next_to_clean=%d, budget=%d, "
		"next_to_watch=%p, "
		"desc.wb.nxtseq_seed=%#08x, desc.wb.status=%#08x\n",
		ring->idx, ring->reg_idx, ring->next_to_use, ring->next_to_clean,
		*budget, tx_buffer->next_to_watch,
		tx_desc->wb.nxtseq_seed, tx_desc->wb.status);

	do {
		eop_desc = tx_buffer->next_to_watch;
		if (!eop_desc) {
			break;
		}

		smp_rmb();

		if (!(eop_desc->wb.status & cpu_to_le32(SXEVF_TX_DESC_STAT_DD))) {
			break;
		}

		tx_buffer->next_to_watch = NULL;

		ring_stats->bytes   += tx_buffer->bytecount;
		ring_stats->packets += tx_buffer->gso_segs;

#ifdef HAVE_XDP_SUPPORT
		if (vf_ring_is_xdp(ring)) {
			page_frag_free(tx_buffer->data);
		} else {
			napi_consume_skb(tx_buffer->skb, napi_budget);
		}
#else
		napi_consume_skb(tx_buffer->skb, napi_budget);
#endif
		LOG_DEBUG("tx ring clean: budget=%d, bytes=%llu, packet=%llu\n",
			*budget, ring_stats->bytes, ring_stats->packets);

		sxevf_tx_skb_unmap(ring, tx_buffer);
		while (tx_desc != eop_desc) {
			sxevf_tx_desc_buf_update(ring, &tx_buffer,
					&tx_desc, &next_to_clean);

			if (dma_unmap_len(tx_buffer, len)) {
				sxevf_tx_skb_unmap(ring, tx_buffer);
			}
		}
		sxevf_tx_desc_buf_update(ring, &tx_buffer,
					&tx_desc, &next_to_clean);

		prefetch(tx_desc);

		--*budget;
	}while (likely(*budget));

	next_to_clean += ring->depth;
	ring->next_to_clean = next_to_clean;
	LOG_DEBUG("tx ring clean end: ring idx=%u, next_to_use=%d, "
			"next_to_clean=%d, budget=%d\n",
		ring->idx, ring->next_to_use, ring->next_to_clean, *budget);

	return;
}

static u32 sxevf_hw_tx_ring_pending_get(struct sxevf_ring *ring)
{
	u32 ret = 0;
	u32 head, tail;
	struct sxevf_adapter *adapter = netdev_priv(ring->netdev);
	struct sxevf_hw *hw = &adapter->hw;

	hw->dma.ops->tx_ring_info_get(hw, ring->idx, &head, &tail);

	if (head != tail) {
		ret = (head < tail) ?
			(tail - head) : (tail + ring->depth - head);
	}

	return ret;
}

static inline bool sxevf_detect_tx_hang(struct sxevf_ring *ring)
{
	bool ret;
	u32 tx_done     = ring->stats.packets;
	u32 tx_done_old = ring->tx_stats.tx_done_old;
	u32 tx_pending  = sxevf_hw_tx_ring_pending_get(ring);

	SXEVF_TX_HANG_CHECK_COMPLETE(ring);

	if (tx_done_old == tx_done && tx_pending) {
		LOG_INFO("ring[%u] hang \n", ring->idx);
		ret = test_and_set_bit(SXEVF_HANG_CHECK_ARMED, &ring->state);
		goto l_end;
	}

	ring->tx_stats.tx_done_old = tx_done;

	clear_bit(SXEVF_HANG_CHECK_ARMED, &ring->state);

	ret = false;
l_end:
	return ret;
}

static void sxevf_tx_timeout_reset(struct sxevf_adapter *adapter)
{
	if (!test_bit(SXEVF_DOWN, &adapter->state)) {
		set_bit(SXEVF_RESET_REQUESTED, &adapter->monitor_ctxt.state);
		sxevf_monitor_work_schedule(adapter);
		LOG_INFO("tx timeout reset adapter\n");
	}

	return;
}

#ifdef HAVE_TIMEOUT_TXQUEUE_IDX
void sxevf_tx_timeout(struct net_device *netdev, u32 __always_unused txqueue)
#else
void sxevf_tx_timeout(struct net_device *netdev)
#endif
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	sxevf_tx_timeout_reset(adapter);

	return;
}

static inline bool sxevf_tx_hang_handle(struct sxevf_adapter *adapter,
				struct sxevf_ring *ring)
{
	u32 tdh, tdt;
	bool ret = false;
	struct sxevf_hw *hw = &adapter->hw;

	if (SXEVF_DETECT_TX_HANG_NEED(ring) && sxevf_detect_tx_hang(ring)) {
#ifdef HAVE_XDP_SUPPORT
		if (!vf_ring_is_xdp(ring))
#endif
			netif_stop_subqueue(ring->netdev, ring->idx);

		hw->dma.ops->tx_ring_info_get(hw, ring->reg_idx, &tdh, &tdt);
		LOG_DEV_ERR("detected Tx hang %s\n"
			"  Tx ring              <%u>\n"
			"  ring reg             <%u>\n"
			"  TDH, TDT             <%u>, <%u>\n"
			"  next_to_use          <%u>\n"
			"  next_to_clean        <%u>\n"
			"tx_buffer_info[next_to_clean]\n"
			"  time_stamp           <%lx>\n"
			"  jiffies              <%llx>\n",
#ifdef HAVE_XDP_SUPPORT
			vf_ring_is_xdp(ring) ? " XDP" : "",
#else
			"",
#endif
			ring->idx, ring->reg_idx,
			tdh, tdt,
			ring->next_to_use, ring->next_to_clean,
			ring->tx_buffer_info[ring->next_to_clean].time_stamp,
			(U64)jiffies);

		sxevf_tx_timeout_reset(adapter);

		ret = true;
	}

	return ret;
}

static inline void sxevf_tx_pkt_stats_update(struct sxevf_irq_rate *irq_rate,
					struct sxevf_ring *ring,
					struct sxevf_ring_stats *stats)
{
	u64_stats_update_begin(&ring->syncp);
	ring->stats.bytes += stats->bytes;
	ring->stats.packets += stats->packets;
	u64_stats_update_end(&ring->syncp);

	irq_rate->total_bytes += stats->bytes;
	irq_rate->total_packets += stats->packets;

	return;
}

bool sxevf_tx_ring_irq_clean(struct sxevf_irq_data *irq,
				struct sxevf_ring *ring, s32 napi_budget)
{
	bool ret;
	struct sxevf_ring_stats ring_stats = {};
	u16 budget = ring->depth / 2;
	struct sxevf_adapter *adapter = irq->adapter;

	if (test_bit(SXEVF_DOWN, &adapter->state)) {
		ret = true;
		goto l_end;
	}

	sxevf_tx_ring_unmap(ring, napi_budget, &budget, &ring_stats);

	sxevf_tx_pkt_stats_update(&irq->tx.irq_rate, ring, &ring_stats);

	if (sxevf_tx_hang_handle(adapter, ring)) {
		ret = true;
		goto l_end;
	}

#ifdef HAVE_XDP_SUPPORT
	if (vf_ring_is_xdp(ring)) {
		LOG_INFO("xdp ring[%u] xmit finish ,return\n", ring->idx);
		ret =  !!budget;
		goto l_end;
	}
#endif

	if (unlikely(ring_stats.packets &&
			netif_carrier_ok(ring->netdev) &&
			(sxevf_desc_unused_count(ring) >= \
				SXEVF_TX_WAKE_THRESHOLD))) {

		smp_mb();
		if (__netif_subqueue_stopped(ring->netdev, ring->idx) && \
				!test_bit(SXEVF_DOWN, &adapter->state)) {
			netif_wake_subqueue(ring->netdev, ring->idx);
			++ring->tx_stats.restart_queue;
			LOG_DEBUG("\n\n ring idx=%u, wake_up\n\n", ring->idx);
		}
	}

	ret =  !!budget;

l_end:
	return ret;
}

bool sxevf_xdp_ring_irq_clean(struct sxevf_irq_data *irq,
				struct sxevf_ring *xdp_ring, s32 napi_budget)
{
	return sxevf_tx_ring_irq_clean(irq, xdp_ring, napi_budget);

}

void sxevf_hw_tx_disable(struct sxevf_adapter *adapter)
{
	u32 i;
	u8 reg_idx;
	struct sxevf_hw *hw = &adapter->hw;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		reg_idx = adapter->tx_ring_ctxt.ring[i]->reg_idx;

		hw->dma.ops->tx_ring_switch(hw, reg_idx, false);
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		reg_idx = adapter->xdp_ring_ctxt.ring[i]->reg_idx;

		hw->dma.ops->tx_ring_switch(hw, reg_idx, false);
	}

	return;
}
