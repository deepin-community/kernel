// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include "txrx.h"

int ne6x_setup_tx_descriptors(struct ne6x_ring *tx_ring)
{
	struct device *dev = tx_ring->dev;
	int bi_size;

	if (!dev)
		return -ENOMEM;

	/* warn if we are about to overwrite the pointer */
	WARN_ON(tx_ring->tx_buf);
	bi_size = sizeof(struct ne6x_tx_buf) * tx_ring->count;
	tx_ring->tx_buf = kzalloc(bi_size, GFP_KERNEL);
	if (!tx_ring->tx_buf)
		goto err;

	/* round up to nearest 4K */
	tx_ring->size = tx_ring->count * sizeof(struct ne6x_tx_desc);
	tx_ring->size = ALIGN(tx_ring->size, 4096);
	tx_ring->desc = dma_alloc_coherent(dev, tx_ring->size, &tx_ring->dma, GFP_KERNEL);
	if (!tx_ring->desc) {
		dev_info(dev, "Unable to allocate memory for the Tx descriptor ring, size=%d\n",
			 tx_ring->size);
		goto err;
	}

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;
	tx_ring->cq_last_expect = 0;

	return 0;

err:
	kfree(tx_ring->tx_buf);
	tx_ring->tx_buf = NULL;

	return -ENOMEM;
}

int ne6x_setup_cq_descriptors(struct ne6x_ring *cq_ring)
{
	struct device *dev = cq_ring->dev;

	if (!dev)
		return -ENOMEM;

	/* round up to nearest 4K */
	cq_ring->size = cq_ring->count * sizeof(struct ne6x_cq_desc);
	cq_ring->size = ALIGN(cq_ring->size, 4096);
	cq_ring->desc = dma_alloc_coherent(dev, cq_ring->size, &cq_ring->dma, GFP_KERNEL);
	if (!cq_ring->desc) {
		dev_info(dev, "Unable to allocate memory for the Tx descriptor ring, size=%d\n",
			 cq_ring->size);
		goto err;
	}

	cq_ring->next_to_use = 0;
	cq_ring->next_to_clean = 0;

	return 0;

err:
	return -ENOMEM;
}

int ne6x_setup_tg_descriptors(struct ne6x_ring *tg_ring)
{
	struct device *dev = tg_ring->dev;

	if (!dev)
		return -ENOMEM;

	/* round up to nearest 4K */
	tg_ring->size = tg_ring->count * sizeof(struct ne6x_tx_tag);
	tg_ring->size = ALIGN(tg_ring->size, 4096);
	tg_ring->desc = dma_alloc_coherent(dev, tg_ring->size, &tg_ring->dma, GFP_KERNEL);
	if (!tg_ring->desc) {
		dev_info(dev, "Unable to allocate memory for the Tx descriptor ring, size=%d\n",
			 tg_ring->size);
		goto err;
	}

	tg_ring->next_to_use = 0;
	tg_ring->next_to_clean = 0;

	return 0;

err:
	return -ENOMEM;
}

int ne6x_setup_rx_descriptors(struct ne6x_ring *rx_ring)
{
	struct device *dev = rx_ring->dev;
	int err = -ENOMEM;
	int bi_size;

	/* warn if we are about to overwrite the pointer */
	WARN_ON(rx_ring->rx_buf);
	bi_size = sizeof(struct ne6x_rx_buf) * rx_ring->count;
	rx_ring->rx_buf = kzalloc(bi_size, GFP_KERNEL);
	if (!rx_ring->rx_buf)
		goto err;

	u64_stats_init(&rx_ring->syncp);

	/* Round up to nearest 4K */
	rx_ring->size = rx_ring->count * sizeof(union ne6x_rx_desc);
	rx_ring->size = ALIGN(rx_ring->size, 4096);
	rx_ring->desc = dma_alloc_coherent(dev, rx_ring->size, &rx_ring->dma, GFP_KERNEL);

	if (!rx_ring->desc)
		goto err;

	rx_ring->next_to_alloc = 0;
	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
	rx_ring->cq_last_expect = 0;

	return 0;

err:
	kfree(rx_ring->rx_buf);
	rx_ring->rx_buf = NULL;

	return err;
}

int ne6x_setup_tx_sgl(struct ne6x_ring *tx_ring)
{
	struct device *dev = tx_ring->dev;

	if (!dev)
		return -ENOMEM;
	tx_ring->sgl = kzalloc(sizeof(*tx_ring->sgl), GFP_KERNEL);

	if (!tx_ring->sgl)
		goto err;

	return 0;
err:
	return -ENOMEM;
}

static inline unsigned int ne6x_txd_use_count(unsigned int size)
{
	return ((size * 85) >> 20) + 1;
}

bool __ne6x_chk_linearize(struct sk_buff *skb);
static inline bool ne6x_chk_linearize(struct sk_buff *skb, int count)
{
	/* Both TSO and single send will work if count is less than 8 */
	if (likely(count < NE6X_MAX_BUFFER_TXD))
		return false;

	if (skb_is_gso(skb))
		return __ne6x_chk_linearize(skb);

	/* we can support up to 8 data buffers for a single send */
	return count != NE6X_MAX_BUFFER_TXD;
}

int __ne6x_maybe_stop_tx(struct ne6x_ring *tx_ring, int size);

static inline int ne6x_maybe_stop_tx(struct ne6x_ring *tx_ring, int size)
{
	if (likely(NE6X_DESC_UNUSED(tx_ring) >= size))
		return 0;

	return __ne6x_maybe_stop_tx(tx_ring, size);
}

static inline bool ne6x_rx_is_programming_status(u8 status)
{
	return status & 0x20;
}

static void ne6x_reuse_rx_page(struct ne6x_ring *rx_ring, struct ne6x_rx_buf *old_buff)
{
	u16 nta = rx_ring->next_to_alloc;
	struct ne6x_rx_buf *new_buff;

	new_buff = &rx_ring->rx_buf[nta];

	/* update, and store next to alloc */
	nta++;
	rx_ring->next_to_alloc = (nta < rx_ring->count) ? nta : 0;

	/* transfer page from old buffer to new buffer */
	new_buff->dma = old_buff->dma;
	new_buff->page = old_buff->page;
	new_buff->page_offset = old_buff->page_offset;
	new_buff->pagecnt_bias = old_buff->pagecnt_bias;
}

static void ne6x_clean_programming_status(struct ne6x_ring *rx_ring,
					  union ne6x_rx_desc *rx_desc,
					  u8 status)
{
	u32 ntc = rx_ring->next_to_clean;
	struct ne6x_rx_buf *rx_buffer;

	/* fetch, update, and store next to clean */
	rx_buffer = &rx_ring->rx_buf[ntc++];
	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;

	prefetch(NE6X_RX_DESC(rx_ring, ntc));

	/* place unused page back on the ring */
	ne6x_reuse_rx_page(rx_ring, rx_buffer);
	rx_ring->rx_stats.page_reuse_count++;

	/* clear contents of buffer_info */
	rx_buffer->page = NULL;
}

static struct ne6x_rx_buf *ne6x_get_rx_buffer(struct ne6x_ring *rx_ring, const unsigned int size)
{
	struct ne6x_rx_buf *rx_buffer;

	rx_buffer = &rx_ring->rx_buf[rx_ring->next_to_clean];
	prefetchw(rx_buffer->page);

	/* we are reusing so sync this buffer for CPU use */
	dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma, rx_buffer->page_offset, size,
				      DMA_FROM_DEVICE);

	/* We have pulled a buffer for use, so decrement pagecnt_bias */
	rx_buffer->pagecnt_bias--;

	return rx_buffer;
}

static void ne6x_add_rx_frag(struct ne6x_ring *rx_ring, struct ne6x_rx_buf *rx_buffer,
			     struct sk_buff *skb, unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = ne6x_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize = SKB_DATA_ALIGN(size);
#endif

	skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, rx_buffer->page, rx_buffer->page_offset,
			size, truesize);

	/* page is being used so we must update the page offset */
#if (PAGE_SIZE < 8192)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif
}

static struct sk_buff *ne6x_construct_skb(struct ne6x_ring *rx_ring,
					  struct ne6x_rx_buf *rx_buffer,
					  unsigned int size)
{
	void *page_addr = page_address(rx_buffer->page) + rx_buffer->page_offset;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = ne6x_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize = SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
				SKB_DATA_ALIGN(size);
#endif
	unsigned int headlen;
	struct sk_buff *skb;

	/* prefetch first cache line of first page */
	prefetch(page_addr);
#if L1_CACHE_BYTES < 128
	prefetch((void *)((u8 *)page_addr + L1_CACHE_BYTES));
#endif

	/* allocate a skb to store the frags */
	skb = __napi_alloc_skb(&rx_ring->q_vector->napi, NE6X_RX_HDR_SIZE,
			       GFP_ATOMIC | __GFP_NOWARN);
	if (unlikely(!skb))
		return NULL;

	/* Determine available headroom for copy */
	headlen = size;
	if (headlen > NE6X_RX_HDR_SIZE)
		headlen = eth_get_headlen(skb->dev, page_addr, NE6X_RX_HDR_SIZE);

	/* align pull length to size of long to optimize memcpy performance */
	memcpy(__skb_put(skb, headlen), page_addr, ALIGN(headlen, sizeof(long)));

	/* update all of the pointers */
	size -= headlen;
	if (size) {
		skb_add_rx_frag(skb, 0, rx_buffer->page, rx_buffer->page_offset + headlen, size,
				truesize);

		/* buffer is used by skb, update page_offset */
#if (PAGE_SIZE < 8192)
		rx_buffer->page_offset ^= truesize;
#else
		rx_buffer->page_offset += truesize;
#endif
	} else {
		/* buffer is unused, reset bias back to rx_buffer */
		rx_buffer->pagecnt_bias++;
	}

	return skb;
}

static inline bool ne6x_page_is_reusable(struct page *page)
{
	return (page_to_nid(page) == numa_mem_id()) && !page_is_pfmemalloc(page);
}

static bool ne6x_can_reuse_rx_page(struct ne6x_rx_buf *rx_buffer)
{
	unsigned int pagecnt_bias = rx_buffer->pagecnt_bias;
	struct page *page = rx_buffer->page;

	/* Is any reuse possible? */
	if (unlikely(!ne6x_page_is_reusable(page)))
		return false;

#if (PAGE_SIZE < 8192)
	/* if we are only owner of page we can reuse it */
	if (unlikely((page_count(page) - pagecnt_bias) > 1))
		return false;
#else
#define NE6X_LAST_OFFSET (SKB_WITH_OVERHEAD(PAGE_SIZE) - NE6X_RXBUFFER_4096)
	if (rx_buffer->page_offset > NE6X_LAST_OFFSET)
		return false;
#endif

	/* If we have drained the page fragment pool we need to update
	 * the pagecnt_bias and page count so that we fully restock the
	 * number of references the driver holds.
	 */
	if (unlikely(pagecnt_bias == 1)) {
		page_ref_add(page, USHRT_MAX - 1);
		rx_buffer->pagecnt_bias = USHRT_MAX;
	}

	return true;
}

static void ne6x_put_rx_buffer(struct ne6x_ring *rx_ring, struct ne6x_rx_buf *rx_buffer)
{
	if (ne6x_can_reuse_rx_page(rx_buffer)) {
		/* hand second half of page back to the ring */
		ne6x_reuse_rx_page(rx_ring, rx_buffer);
		rx_ring->rx_stats.page_reuse_count++;
	} else {
		/* we are not reusing the buffer so unmap it */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma, ne6x_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE, NE6X_RX_DMA_ATTR);
		__page_frag_cache_drain(rx_buffer->page, rx_buffer->pagecnt_bias);
	}

	/* clear contents of buffer_info */
	rx_buffer->page = NULL;
}

static inline bool ne6x_test_staterr(union ne6x_rx_desc *rx_desc, const u8 stat_err_bits)
{
	return !!(rx_desc->wb.u.val & stat_err_bits);
}

static bool ne6x_is_non_eop(struct ne6x_ring *rx_ring, union ne6x_rx_desc *rx_desc,
			    struct sk_buff *skb)
{
	u32 ntc = rx_ring->next_to_clean + 1;

	/* fetch, update, and store next to clean */
	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;

	prefetch(NE6X_RX_DESC(rx_ring, ntc));

	/* if we are the last buffer then there is nothing else to do */
#define NE6X_RXD_EOF BIT(NE6X_RX_DESC_STATUS_EOF_SHIFT)
	if (likely(ne6x_test_staterr(rx_desc, NE6X_RXD_EOF)))
		return false;

	rx_ring->rx_stats.non_eop_descs++;
	rx_desc->wb.u.val = 0;

	return true;
}

static bool ne6x_cleanup_headers(struct ne6x_ring *rx_ring, struct sk_buff *skb,
				 union ne6x_rx_desc *rx_desc)
{
	if (unlikely(ne6x_test_staterr(rx_desc, BIT(NE6X_RX_DESC_STATUS_ERR_SHIFT)))) {
		dev_kfree_skb_any(skb);
		rx_ring->rx_stats.rx_mem_error++;
		return true;
	}

	/* if eth_skb_pad returns an error the skb was freed */
	if (eth_skb_pad(skb))
		return true;

	return false;
}

static inline void ne6x_rx_hash(struct ne6x_ring *ring, union ne6x_rx_desc *rx_desc,
				struct sk_buff *skb, struct rx_hdr_info *rx_hdr)
{
	if (!(ring->netdev->features & NETIF_F_RXHASH))
		return;

	if (rx_hdr->ol_flag.flag_bits.rx_rss_hash)
		skb_set_hash(skb, rx_hdr->rss_hash, PKT_HASH_TYPE_NONE);
}

static inline void ne6x_rx_checksum(struct ne6x_ring *rx_ring, struct sk_buff *skb,
				    union ne6x_rx_desc *rx_desc,
				    struct rx_hdr_info *rx_hdr)
{
	skb->ip_summed = CHECKSUM_NONE;
	skb->csum_level = 0;
	skb_checksum_none_assert(skb);

	if (!(rx_ring->netdev->features & NETIF_F_RXCSUM))
		return;

	if (rx_hdr->ol_flag.flag_bits.rx_ip_cksum_bad ||
	    rx_hdr->ol_flag.flag_bits.rx_l4_cksum_bad ||
	    rx_hdr->ol_flag.flag_bits.rx_inner_ip_cksum_bad ||
	    rx_hdr->ol_flag.flag_bits.rx_inner_l4_cksum_bad) {
		rx_ring->rx_stats.csum_err++;
	} else if (rx_hdr->ol_flag.flag_bits.rx_ip_cksum_good ||
		   rx_hdr->ol_flag.flag_bits.rx_l4_cksum_good ||
		   rx_hdr->ol_flag.flag_bits.rx_inner_ip_cksum_good ||
		   rx_hdr->ol_flag.flag_bits.rx_inner_l4_cksum_good) {
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		skb->csum_level = 1;
	}
}

static inline void ne6x_process_skb_fields(struct ne6x_ring *rx_ring,
					   union ne6x_rx_desc *rx_desc,
					   struct sk_buff *skb,
					   struct rx_hdr_info *rx_hdr)
{
	netdev_features_t features = rx_ring->netdev->features;
	bool non_zero_vlan = false;

	ne6x_rx_hash(rx_ring, rx_desc, skb, rx_hdr);
	rx_hdr->vlan_tci = ntohs(rx_hdr->vlan_tci);
	rx_hdr->vlan_tci_outer = ntohs(rx_hdr->vlan_tci_outer);

	if (features & NETIF_F_HW_VLAN_CTAG_RX) {
		if (rx_hdr->ol_flag.flag_bits.rx_vlan_striped) {
			non_zero_vlan = !!(rx_hdr->vlan_tci_outer & VLAN_VID_MASK);
			if (non_zero_vlan) {
				__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
						       (rx_hdr->vlan_tci_outer));
			}
		}
	} else if (features & NETIF_F_HW_VLAN_STAG_RX) {
		if (rx_hdr->ol_flag.flag_bits.rx_qinq_striped) {
			non_zero_vlan = !!(rx_hdr->vlan_tci_outer & VLAN_VID_MASK);
			if (non_zero_vlan) {
				__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021AD),
						       (rx_hdr->vlan_tci_outer));
			}
		}
	}

	ne6x_rx_checksum(rx_ring, skb, rx_desc, rx_hdr);
	skb_record_rx_queue(skb, rx_ring->queue_index);

	/* modifies the skb - consumes the enet header */
	skb->protocol = eth_type_trans(skb, rx_ring->netdev);
}

static void ne6x_receive_skb(struct ne6x_ring *rx_ring, struct sk_buff *skb)
{
	struct ne6x_q_vector *q_vector = rx_ring->q_vector;

	napi_gro_receive(&q_vector->napi, skb);
}

static bool ne6x_alloc_mapped_page(struct ne6x_ring *rx_ring, struct ne6x_rx_buf *bi)
{
	struct page *page = bi->page;
	dma_addr_t dma;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page)) {
		rx_ring->rx_stats.page_reuse_count++;
		return true;
	}

	/* alloc new page for storage */
	page = dev_alloc_pages(ne6x_rx_pg_order(rx_ring));
	if (unlikely(!page)) {
		rx_ring->rx_stats.alloc_page_failed++;
		return false;
	}

	/* map page for use */
	dma = dma_map_page_attrs(rx_ring->dev, page, 0, ne6x_rx_pg_size(rx_ring), DMA_FROM_DEVICE,
				 NE6X_RX_DMA_ATTR);

	/* if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		__free_pages(page, ne6x_rx_pg_order(rx_ring));
		rx_ring->rx_stats.alloc_page_failed++;
		return false;
	}

	bi->dma = dma;
	bi->page = page;
	bi->page_offset = 0;

	page_ref_add(page, USHRT_MAX - 1);
	bi->pagecnt_bias = USHRT_MAX;

	return true;
}

void ne6x_tail_update(struct ne6x_ring *ring, int val)
{
	int i;

	for (i = 0; i < NE6X_TAIL_REG_NUM; i++)
		writeq(val, ring->tail + i);
}

static inline void ne6x_release_rx_desc(struct ne6x_ring *rx_ring, u32 val)
{
	rx_ring->next_to_use = val;

	/* update next to alloc since we have filled the ring */
	rx_ring->next_to_alloc = val;

	/* Force memory writes to complete before letting h/w
	 * know there are new descriptors to fetch.  (Only
	 * applicable for weak-ordered memory model archs,
	 * such as IA-64).
	 */
	wmb();
	ne6x_tail_update(rx_ring, val);
}

bool ne6x_alloc_rx_buffers(struct ne6x_ring *rx_ring, u16 cleaned_count)
{
	u16 ntu = rx_ring->next_to_use;
	union ne6x_rx_desc *rx_desc;
	struct ne6x_rx_buf *bi;

	/* do nothing if no valid netdev defined */
	if (!rx_ring->netdev || !cleaned_count)
		return false;

	rx_desc = NE6X_RX_DESC(rx_ring, ntu);
	bi = &rx_ring->rx_buf[ntu];

	do {
		if (!ne6x_alloc_mapped_page(rx_ring, bi))
			goto no_buffers;

		/* sync the buffer for use by the device */
		dma_sync_single_range_for_device(rx_ring->dev, bi->dma, bi->page_offset,
						 rx_ring->rx_buf_len, DMA_FROM_DEVICE);

		/* Refresh the desc even if buffer_addrs didn't change
		 * because each write-back erases this info.
		 */
		rx_desc->wb.u.val = 0;
		rx_desc->w.buffer_mop_addr = cpu_to_le64(bi->dma + bi->page_offset);
		rx_desc->w.buffer_sop_addr = 0;
		rx_desc->w.mop_mem_len = rx_ring->rx_buf_len;
		rx_desc->wb.pkt_len = 0;
		rx_desc->w.vp = rx_ring->reg_idx;

		rx_desc++;
		bi++;
		ntu++;
		if (unlikely(ntu == rx_ring->count)) {
			rx_desc = NE6X_RX_DESC(rx_ring, 0);
			bi = rx_ring->rx_buf;
			ntu = 0;
		}

		/* clear the status bits for the next_to_use descriptor */
		rx_desc->wb.u.val = 0;

		cleaned_count--;
	} while (cleaned_count);

	if (rx_ring->next_to_use != ntu)
		ne6x_release_rx_desc(rx_ring, ntu);

	return false;

no_buffers:
	if (rx_ring->next_to_use != ntu)
		ne6x_release_rx_desc(rx_ring, ntu);

	/* make sure to come back via polling to try again after
	 * allocation failure
	 */
	return true;
}

static void ne6x_get_rx_head_info(struct sk_buff *skb, struct rx_hdr_info *rx_hdr)
{
	skb_frag_t *frag;
	void *page_addr;
	u32 temp_len, i;

	if (skb->data_len == 0) {
		memcpy(rx_hdr, &skb->data[skb->len - 16], sizeof(struct rx_hdr_info));
	} else {
		if (skb_shinfo(skb)->nr_frags > 1) {
			i = skb_shinfo(skb)->nr_frags - 1;
			frag = &skb_shinfo(skb)->frags[i];
			if (skb_frag_size(frag) >= 16) {
				page_addr = skb_frag_address(frag) + skb_frag_size(frag) - 16;
				memcpy(rx_hdr, page_addr, sizeof(struct rx_hdr_info));
			} else if (skb_frag_size(frag) > 4) {
				page_addr = skb_frag_address(frag);
				temp_len = skb_frag_size(frag);
				memcpy((char *)rx_hdr + 16 - temp_len, page_addr, temp_len - 4);
				frag = &skb_shinfo(skb)->frags[i - 1];
				page_addr = skb_frag_address(frag) + skb_frag_size(frag) - 16 +
					    temp_len;
				memcpy(rx_hdr, page_addr, 16 - temp_len);
			} else {
				page_addr = skb_frag_address(frag);
				temp_len = skb_frag_size(frag);
				frag = &skb_shinfo(skb)->frags[i - 1];
				page_addr = skb_frag_address(frag) + skb_frag_size(frag) - 16 +
					    temp_len;
				memcpy(rx_hdr, page_addr, sizeof(struct rx_hdr_info));
			}
		} else {
			frag = &skb_shinfo(skb)->frags[0];
			if (skb_frag_size(frag) >= 16) {
				page_addr = skb_frag_address(frag) + skb_frag_size(frag) - 16;
				memcpy(rx_hdr, page_addr, sizeof(struct rx_hdr_info));
			} else if (skb_frag_size(frag) > 4) {
				page_addr = skb_frag_address(frag);
				temp_len = skb_frag_size(frag);
				memcpy((char *)rx_hdr + 16 - temp_len, page_addr, temp_len - 4);
				page_addr = &skb->data[skb->len - skb->data_len - 16 + temp_len];
				memcpy(rx_hdr, page_addr, 16 - temp_len);
			} else {
				page_addr = skb_frag_address(frag);
				temp_len = skb_frag_size(frag);
				page_addr = &skb->data[skb->len - skb->data_len - 16 + temp_len];
				memcpy(rx_hdr, page_addr, sizeof(struct rx_hdr_info));
			}
		}
	}
}

static void ne6x_clean_tx_desc(struct ne6x_tx_desc *tx_desc, struct ne6x_ring *ring)
{
	if (tx_desc->u.flags.tx_drop_addr)
		ring->tx_stats.tx_drop_addr++;

	if (tx_desc->u.flags.tx_ecc_err)
		ring->tx_stats.tx_ecc_err++;

	if (tx_desc->u.flags.tx_pcie_read_err) {
		ring->tx_stats.tx_pcie_read_err++;
		dev_info(ring->dev, "**** tx_desc: flag[0x%x], vp[%d], et[%d], ch[%d], tt[%d], sopv[%d], eopv[%d], tso[%d], l3chk[%d], l3oft[%d], l4chk[%d], l4oft[%d], pld[%d], mop[%d], sop[%d], mss[%d],mopa[%lld],sopa[%lld]\n",
			 tx_desc->u.val, tx_desc->vp, tx_desc->event_trigger, tx_desc->chain,
			 tx_desc->transmit_type, tx_desc->sop_valid, tx_desc->eop_valid,
			 tx_desc->tso, tx_desc->l3_csum, tx_desc->l3_ofst, tx_desc->l4_csum,
			 tx_desc->l4_ofst, tx_desc->pld_ofst, tx_desc->mop_cnt, tx_desc->sop_cnt,
			 tx_desc->mss, tx_desc->buffer_mop_addr, tx_desc->buffer_sop_addr);
	}

	tx_desc->u.val = 0;
	tx_desc->vp = 0;
	tx_desc->event_trigger = 0;
	tx_desc->chain = 0;
	tx_desc->transmit_type = 0;
	tx_desc->sop_valid = 0;
	tx_desc->eop_valid = 0;
	tx_desc->tso = 0;
	tx_desc->l3_csum = 0;
	tx_desc->l3_ofst = 0;
	tx_desc->l4_csum = 0;
	tx_desc->l4_ofst = 0;
	tx_desc->pld_ofst = 0;
	tx_desc->mop_cnt = 0;
	tx_desc->sop_cnt = 0;
	tx_desc->mss = 0;
	tx_desc->buffer_mop_addr = 0;
	tx_desc->buffer_sop_addr = 0;
}

int ne6x_clean_cq_irq(struct ne6x_q_vector *q_vector, struct ne6x_ring *cq_ring, int napi_budget)
{
	struct ne6x_cq_desc *cq_desc = NULL;
	struct ne6x_tx_desc *tx_desc = NULL;
	struct ne6x_ring *clean_ring = NULL;
	union ne6x_rx_desc *rx_desc = NULL;
	int i, cq_num, off_idx, ntc;
	int budget = napi_budget;
	int last_expect = 0;
	int total = 0;

	do {
		cq_desc = NE6X_CQ_DESC(cq_ring, cq_ring->next_to_use);
		cq_num = cq_desc->num;
		if (!cq_num)
			break;

		dma_rmb();
		cq_ring->stats.packets += cq_num;

		if (cq_desc->ctype) {
			clean_ring = q_vector->rx.ring;
			last_expect = clean_ring->cq_last_expect;
			for (i = 0; i < cq_num; i++) {
				off_idx = cq_desc->payload.rx_cq[i].cq_rx_offset;
				if (unlikely(off_idx != last_expect)) {
					netdev_err(cq_ring->netdev, "ne6xpf: cqrx err, need debug! cq: %d, rx: %d\n",
						   off_idx, last_expect);
					netdev_err(cq_ring->netdev, "ne6xpf: queue: %d, vp: %d, rxq: %d\n",
						   cq_ring->queue_index, cq_ring->reg_idx,
						   clean_ring->queue_index);
				}

				rx_desc = NE6X_RX_DESC(clean_ring, off_idx);
				rx_desc->wb.u.val = cq_desc->payload.rx_cq[i].cq_rx_stats;
				rx_desc->wb.pkt_len = cq_desc->payload.rx_cq[i].cq_rx_len;
				if (rx_desc->wb.pkt_len > clean_ring->rx_buf_len) {
					if (!rx_desc->wb.u.flags.rx_eop)
						rx_desc->wb.pkt_len = clean_ring->rx_buf_len;
					else
						rx_desc->wb.pkt_len = rx_desc->wb.pkt_len %
								      clean_ring->rx_buf_len ?
								      rx_desc->wb.pkt_len %
								      clean_ring->rx_buf_len :
								      clean_ring->rx_buf_len;
				}

				last_expect++;
				last_expect = (last_expect < clean_ring->count) ? last_expect : 0;
			}

			cq_ring->cq_stats.rx_num += cq_num;
		} else {
			clean_ring = q_vector->tx.ring;
			last_expect = clean_ring->cq_last_expect;
			for (i = 0; i < cq_num; i++) {
				off_idx = cq_desc->payload.tx_cq[i].cq_tx_offset;
				if (unlikely(off_idx != last_expect)) {
					netdev_info(cq_ring->netdev, "ne6xpf: cqtx err, need debug! cq: %d, tx: %d\n",
						    off_idx, last_expect);
					netdev_info(cq_ring->netdev, "ne6xpf: queue: %d, vp: %d, txq: %d\n",
						    cq_ring->queue_index, cq_ring->reg_idx,
						    clean_ring->queue_index);
				}

				tx_desc = NE6X_TX_DESC(clean_ring, off_idx);
				tx_desc->u.val = cq_desc->payload.tx_cq[i].cq_tx_stats;
				last_expect++;
				last_expect = (last_expect < clean_ring->count) ? last_expect : 0;
			}

			cq_ring->cq_stats.tx_num += cq_num;
		}

		clean_ring->cq_last_expect = last_expect;
		cq_ring->cq_stats.cq_num++;

		/*  clean cq desc */
		cq_desc->num = 0;
		ntc = cq_ring->next_to_use + 1;
		ntc = (ntc < cq_ring->count) ? ntc : 0;
		cq_ring->next_to_use = ntc;
		prefetch(NE6X_CQ_DESC(cq_ring, ntc));

		budget--;
		total++;
	} while (likely(budget));

	if (NE6X_DESC_UNUSED(cq_ring) < 1024) {
		cq_ring->next_to_clean = cq_ring->next_to_use;
		/* memory barrier updating cq ring tail */
		wmb();
		writeq(cq_ring->next_to_clean, cq_ring->tail);
	}

	return total;
}

int ne6x_clean_rx_irq(struct ne6x_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	u16 cleaned_count = NE6X_DESC_UNUSED(rx_ring);
	struct ne6x_rx_buf *rx_buffer = NULL;
	struct sk_buff *skb = rx_ring->skb;
	union ne6x_rx_desc *rx_desc = NULL;
	struct rx_hdr_info rx_hdr;
	bool failure = false;
	unsigned int size;
	u8 rx_status;

	while (likely(total_rx_packets < (unsigned int)budget)) {
		if (cleaned_count >= NE6X_RX_BUFFER_WRITE) {
			failure = failure || ne6x_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}
		rx_desc = NE6X_RX_DESC(rx_ring, rx_ring->next_to_clean);

		rx_status = rx_desc->wb.u.val;
		if (!rx_status)
			break;

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we have
		 * verified the descriptor has been written back.
		 */
		dma_rmb();

		if (unlikely(ne6x_rx_is_programming_status(rx_status))) {
			rx_ring->rx_stats.rx_err++;
			ne6x_clean_programming_status(rx_ring, rx_desc, rx_status);
			cleaned_count++;
			continue;
		}

		size = rx_desc->wb.pkt_len;
		rx_buffer = ne6x_get_rx_buffer(rx_ring, size);

		/* retrieve a buffer from the ring */
		if (skb)
			ne6x_add_rx_frag(rx_ring, rx_buffer, skb, size);
		else
			skb = ne6x_construct_skb(rx_ring, rx_buffer, size);

		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			rx_ring->rx_stats.alloc_buf_failed++;
			rx_buffer->pagecnt_bias++;
			break;
		}

		ne6x_put_rx_buffer(rx_ring, rx_buffer);
		cleaned_count++;

		if (ne6x_is_non_eop(rx_ring, rx_desc, skb))
			continue;

		if (ne6x_cleanup_headers(rx_ring, skb, rx_desc)) {
			skb = NULL;
			continue;
		}

		ne6x_get_rx_head_info(skb, &rx_hdr);
		pskb_trim(skb, skb->len - 16);
		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, VLAN, and protocol */
		ne6x_process_skb_fields(rx_ring, rx_desc, skb, &rx_hdr);

		ne6x_receive_skb(rx_ring, skb);
		skb = NULL;

		rx_desc->wb.u.val = 0;

		/* update budget accounting */
		total_rx_packets++;
	}

	rx_ring->skb = skb;

	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	u64_stats_update_end(&rx_ring->syncp);

	/* guarantee a trip back through this routine if there was a failure */
	return failure ? budget : (int)total_rx_packets;
}

int ne6x_clean_tx_irq(struct ne6x_adapt_comm *comm, struct ne6x_ring *tx_ring, int napi_budget)
{
	unsigned int total_bytes = 0, total_packets = 0;
	struct ne6x_tx_desc *eop_desc = NULL;
	u16 i = tx_ring->next_to_clean;
	struct ne6x_tx_desc *tx_desc;
	struct ne6x_tx_buf *tx_buf;
	unsigned int budget = 256;

	tx_buf = &tx_ring->tx_buf[i];
	tx_desc = NE6X_TX_DESC(tx_ring, i);

	if (unlikely(tx_buf->jumbo_frame)) {
		tx_buf->napi_budget += napi_budget;
		if (!tx_buf->jumbo_finsh)
			return !!budget;

		napi_budget = tx_buf->napi_budget;
	}

	do {
		eop_desc = tx_buf->next_to_watch;
		if (!eop_desc)
			break;

		prefetchw(&tx_buf->skb->users);

		if (!eop_desc->u.val)
			break;

		dma_rmb();

		/* clear next_to_watch to prevent false hangs */
		tx_buf->next_to_watch = NULL;
		tx_buf->jumbo_frame = 0;
		tx_buf->jumbo_finsh = 0;

		/* update the statistics for this packet */
		total_bytes += tx_buf->bytecount;
		total_packets += tx_buf->gso_segs;

		/* free the skb/XDP data */
		ne6x_clean_tx_desc(tx_desc, tx_ring);

		/* free the skb */
		napi_consume_skb(tx_buf->skb, napi_budget);

		/* unmap skb header data */
		dma_unmap_single(tx_ring->dev, dma_unmap_addr(tx_buf, dma),
				 dma_unmap_len(tx_buf, len), DMA_TO_DEVICE);

		/* clear tx_buffer data */
		tx_buf->skb = NULL;
		dma_unmap_len_set(tx_buf, len, 0);

		/* unmap remaining buffers */
		while (tx_desc != eop_desc) {
			tx_buf++;
			tx_desc++;
			i++;
			if (i == tx_ring->count) {
				i = 0;
				tx_buf = tx_ring->tx_buf;
				tx_desc = NE6X_TX_DESC(tx_ring, 0);
			}

			/* unmap any remaining paged data */
			if (dma_unmap_len(tx_buf, len)) {
				dma_unmap_page(tx_ring->dev, dma_unmap_addr(tx_buf, dma),
					       dma_unmap_len(tx_buf, len), DMA_TO_DEVICE);
				dma_unmap_len_set(tx_buf, len, 0);
			}

			/* free the skb/XDP data */
			ne6x_clean_tx_desc(tx_desc, tx_ring);
		}

		/* move us one more past the eop_desc for start of next pkt */
		tx_buf++;
		tx_desc++;
		i++;
		if (i == tx_ring->count) {
			i = 0;
			tx_buf = tx_ring->tx_buf;
			tx_desc = NE6X_TX_DESC(tx_ring, 0);
		}

		if (unlikely(tx_buf->jumbo_frame && !tx_buf->jumbo_finsh))
			break;

		prefetch(tx_desc);

		/* update budget accounting */
		budget--;
	} while (likely(budget));

	if (total_packets) {
		tx_ring->next_to_clean = i;
		u64_stats_update_begin(&tx_ring->syncp);
		tx_ring->stats.bytes += total_bytes;
		tx_ring->stats.packets += total_packets;
		u64_stats_update_end(&tx_ring->syncp);

		/* notify netdev of completed buffers */
		netdev_tx_completed_queue(txring_txq(tx_ring), total_packets, total_bytes);

#define TX_WAKE_THRESHOLD ((s16)(DESC_NEEDED * 2))
		if (unlikely(total_packets && netif_carrier_ok(tx_ring->netdev) &&
			     (NE6X_DESC_UNUSED(tx_ring) >= TX_WAKE_THRESHOLD))) {
			/* Make sure that anybody stopping the queue after this
			 * sees the new next_to_clean.
			 */
			smp_mb();
			if (__netif_subqueue_stopped(tx_ring->netdev, tx_ring->queue_index) &&
			    !test_bit(NE6X_ADPT_DOWN, comm->state)) {
				netif_wake_subqueue(tx_ring->netdev, tx_ring->queue_index);
				++tx_ring->tx_stats.restart_q;
			}
		}
	}

	return !!budget;
}

static inline int ne6x_xmit_descriptor_count(struct sk_buff *skb)
{
	int count = 0;

	count = 1;
	count += skb_shinfo(skb)->nr_frags;

	return count;
}

int __ne6x_maybe_stop_tx(struct ne6x_ring *tx_ring, int size)
{
	netif_stop_subqueue(tx_ring->netdev, tx_ring->queue_index);
	/* Memory barrier before checking head and tail */
	smp_mb();

	/* Check again in a case another CPU has just made room available. */
	if (likely(NE6X_DESC_UNUSED(tx_ring) < size))
		return -EBUSY;

	/* A reprieve! - use start_queue because it doesn't call schedule */
	netif_start_subqueue(tx_ring->netdev, tx_ring->queue_index);

	return 0;
}

static inline u16 ne6x_gso_get_seg_hdrlen(struct sk_buff *skb)
{
	u16 gso_hdr_len;

	gso_hdr_len = skb_transport_offset(skb) + tcp_hdrlen(skb);
	if (unlikely(skb->encapsulation))
		gso_hdr_len = skb_inner_transport_offset(skb) + inner_tcp_hdrlen(skb);

	return gso_hdr_len;
}

static int ne6x_tso(struct ne6x_ring *tx_ring, struct ne6x_tx_buf *first,
		    struct ne6x_tx_tag *ptx_tag)
{
	struct sk_buff *skb = first->skb;
	u8 hdrlen = 0;
	int err;

	if (skb->ip_summed != CHECKSUM_PARTIAL || !skb_is_gso(skb))
		return 0;

	hdrlen = ne6x_gso_get_seg_hdrlen(skb);

	err = skb_cow_head(skb, 0);
	if (err < 0)
		return err;

	/* update gso_segs and bytecount */
	first->gso_segs = skb_shinfo(skb)->gso_segs;
	first->bytecount += (first->gso_segs - 1) * hdrlen;

	ptx_tag->tag_mss = skb_shinfo(skb)->gso_size;

	return 1;
}

static void ne6x_tx_prepare_vlan_flags(struct ne6x_ring *tx_ring,
				       struct ne6x_tx_buf *first,
				       struct ne6x_tx_tag  *ptx_tag)
{
	struct sk_buff *skb = first->skb;

	/* nothing left to do, software offloaded VLAN */
	if (!skb_vlan_tag_present(skb) && eth_type_vlan(skb->protocol))
		return;

	/* the VLAN ethertype/tpid is determined by adapter configuration and netdev
	 * feature flags, which the driver only allows either 802.1Q or 802.1ad
	 * VLAN offloads exclusively so we only care about the VLAN ID here
	 */
	if (skb_vlan_tag_present(skb)) {
		if (tx_ring->netdev->features & NETIF_F_HW_VLAN_CTAG_TX)
			ptx_tag->tag_vlan2 = cpu_to_be16(skb_vlan_tag_get(skb));
		else if (tx_ring->netdev->features & NETIF_F_HW_VLAN_STAG_TX)
			ptx_tag->tag_vlan1 = cpu_to_be16(skb_vlan_tag_get(skb));
	}
}

static int ne6x_tx_csum(struct ne6x_ring *tx_ring, struct ne6x_tx_buf *first,
			struct ne6x_tx_tag *ptx_tag)
{
	tx_ring->tx_stats.csum_good++;
	return 1;
}

static inline void  ne6x_tx_desc_push(struct ne6x_tx_desc *tx_desc,
				      dma_addr_t dma, u32 size)
{
	tx_desc->buffer_mop_addr = cpu_to_le64(dma);
	tx_desc->mop_cnt = size;
	tx_desc->event_trigger = 1;
}

void ne6x_unmap_and_free_tx_resource(struct ne6x_ring *ring,
				     struct ne6x_tx_buf *tx_buffer)
{
	if (tx_buffer->skb) {
		dev_kfree_skb_any(tx_buffer->skb);
		if (dma_unmap_len(tx_buffer, len))
			dma_unmap_single(ring->dev, dma_unmap_addr(tx_buffer, dma),
					 dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
	} else if (dma_unmap_len(tx_buffer, len)) {
		dma_unmap_page(ring->dev, dma_unmap_addr(tx_buffer, dma),
			       dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
	}

	tx_buffer->next_to_watch = NULL;
	tx_buffer->skb = NULL;
	dma_unmap_len_set(tx_buffer, len, 0);
}

static inline void ne6x_fill_gso_sg(void *p, u16 offset, u16 len, struct ne6x_sg_info *sg)
{
	sg->p = p;
	sg->offset = offset;
	sg->len = len;
}

int ne6x_fill_jumbo_sgl(struct ne6x_ring *tx_ring, struct sk_buff *skb)
{
	u16 sg_max_dlen = 0, dlen = 0, len = 0, offset = 0, send_dlen = 0, total_dlen = 0;
	u16 subframe = 0, send_subframe = 0, sg_avail = 0, i = 0, j = 0;
	u16 gso_hdr_len = ne6x_gso_get_seg_hdrlen(skb);
	struct ne6x_sg_list  *sgl = tx_ring->sgl;

	WARN_ON(!sgl);

	memset(sgl, 0, sizeof(struct ne6x_sg_list));
	dlen = skb_headlen(skb) - gso_hdr_len;
	sgl->mss = skb_shinfo(skb)->gso_size;
	sg_max_dlen = NE6X_MAX_DATA_PER_TXD - gso_hdr_len;
	sg_max_dlen = ((u16)(sg_max_dlen / sgl->mss)) * sgl->mss;
	total_dlen = skb->data_len + dlen;
	sgl->sgl_mss_cnt = sg_max_dlen / sgl->mss;
	subframe = total_dlen / sg_max_dlen;
	subframe += total_dlen % sg_max_dlen ? 1 : 0;
	ne6x_fill_gso_sg(skb->data, 0, gso_hdr_len, &sgl->sg[i]);
	sgl->sg[i].flag |= NE6X_SG_FST_SG_FLAG | NE6X_SG_SOP_FLAG | NE6X_SG_JUMBO_FLAG;
	offset = gso_hdr_len;
	sg_avail = sg_max_dlen;
	++send_subframe;
	i++;
	while (dlen) {
		len = dlen > sg_avail ? sg_avail : dlen;
		ne6x_fill_gso_sg(skb->data, offset, len, &sgl->sg[i]);
		offset += len;
		dlen -= len;
		send_dlen += len;
		sg_avail -= len;
		if (send_dlen == total_dlen)
			goto end;

		if (!(send_dlen % sg_max_dlen)) {
			sgl->sg[i].flag |= NE6X_SG_EOP_FLAG;
			++i;
			if (unlikely(i > NE6X_MAX_DESC_NUM_PER_SKB))
				goto err;

			ne6x_fill_gso_sg(skb->data, 0, gso_hdr_len, &sgl->sg[i]);

			sgl->sg[i].flag |= NE6X_SG_SOP_FLAG | NE6X_SG_JUMBO_FLAG;
			sgl->sg[i].base_mss_no = send_subframe * sgl->sgl_mss_cnt;

			if (++send_subframe == subframe)
				sgl->sg[i].flag |= NE6X_SG_LST_SG_FLAG;

			sgl->sg[i].base_mss_no = send_subframe * sgl->sgl_mss_cnt;

			sg_avail = sg_max_dlen;
		}
		++i;
		if (unlikely(i > NE6X_MAX_DESC_NUM_PER_SKB))
			goto err;
	}

	for (j = 0; j < skb_shinfo(skb)->nr_frags; j++) {
		skb_frag_t *f = &skb_shinfo(skb)->frags[j];

		dlen = skb_frag_size(f);
		offset = 0;
		while (dlen) {
			len = dlen > sg_avail ? sg_avail : dlen;
			ne6x_fill_gso_sg(f, offset, len, &sgl->sg[i]);
			sgl->sg[i].flag |= NE6X_SG_FRAG_FLAG;

			offset += len;
			dlen -= len;
			send_dlen += len;
			sg_avail -= len;
			if (send_dlen == total_dlen)
				goto end;
			if (!(send_dlen % sg_max_dlen)) {
				sgl->sg[i].flag |= NE6X_SG_EOP_FLAG;
				++i;
				if (unlikely(i > NE6X_MAX_DESC_NUM_PER_SKB))
					goto err;
				ne6x_fill_gso_sg(skb->data, 0, gso_hdr_len, &sgl->sg[i]);
				sgl->sg[i].flag |= NE6X_SG_SOP_FLAG | NE6X_SG_JUMBO_FLAG;
				sgl->sg[i].base_mss_no = send_subframe * sgl->sgl_mss_cnt;

				if (++send_subframe  == subframe)
					sgl->sg[i].flag |= NE6X_SG_LST_SG_FLAG;
				sg_avail = sg_max_dlen;
			}
			++i;
			if (unlikely(i > NE6X_MAX_DESC_NUM_PER_SKB))
				goto err;
		}
		offset = 0;
	}
end:
	sgl->sg[i].flag |= NE6X_SG_EOP_FLAG;
	sgl->sg_num = ++i;
	return 0;
err:
	return -1;
}

void ne6x_fill_tx_desc(struct ne6x_tx_desc *tx_desc, u8 vp, dma_addr_t tag_dma,
		       dma_addr_t dma, struct ne6x_sg_info *sg)
{
	memset(tx_desc, 0, NE6X_TX_DESC_SIZE);
	tx_desc->buffer_mop_addr = cpu_to_le64(dma);
	tx_desc->buffer_sop_addr = (sg->flag & NE6X_SG_SOP_FLAG) ? cpu_to_le64(tag_dma) : 0;
	tx_desc->mop_cnt = sg->len;
	tx_desc->event_trigger = 1;
	tx_desc->vp = vp;
	tx_desc->sop_valid = (sg->flag & NE6X_SG_SOP_FLAG) ? 1u : 0u;
	tx_desc->eop_valid = (sg->flag & NE6X_SG_EOP_FLAG) ? 1u : 0u;
	tx_desc->sop_cnt =  (sg->flag & NE6X_SG_SOP_FLAG) ? 32 : 0;
	if (tx_desc->eop_valid) {
		tx_desc->sop_cnt = tx_desc->mop_cnt;
		tx_desc->buffer_sop_addr = tx_desc->buffer_mop_addr;
		tx_desc->mop_cnt = 4;
	}
}

void ne6x_fill_tx_priv_tag(struct ne6x_ring *tx_ring, struct ne6x_tx_tag *tx_tag,
			   int mss, struct ne6x_sg_info *sg)
{
	struct ne6x_adapt_comm *comm = (struct ne6x_adapt_comm *)tx_ring->adpt;

	tx_tag->tag_pi1 = (comm->port_info & 0x2) ? 1 : 0;
	tx_tag->tag_pi0 = (comm->port_info & 0x1) ? 1 : 0;
	tx_tag->tag_vport = (comm->port_info >> 8) & 0xFF;
	tx_tag->tag_mss = cpu_to_be16(mss);
	tx_tag->tag_num = sg->base_mss_no | (sg->flag & NE6X_SG_JUMBO_FLAG) |
			  (sg->flag & NE6X_SG_LST_SG_FLAG) |
			  (sg->flag & NE6X_SG_FST_SG_FLAG);
	tx_tag->tag_num = cpu_to_be16(tx_tag->tag_num);
}

void ne6x_xmit_jumbo(struct ne6x_ring *tx_ring, struct ne6x_tx_buf *first,
		     struct ne6x_ring *tag_ring, struct ne6x_tx_tag *tx_tag)
{
	int j = 0;
	struct ne6x_sg_list *sgl = tx_ring->sgl;
	struct ne6x_sg_info *sg;
	dma_addr_t dma, tag_dma;
	struct sk_buff *skb = first->skb;
	struct ne6x_tx_buf *tx_bi;
	struct ne6x_tx_tag *tag_desc = tx_tag;
	u32 i = tx_ring->next_to_use;
	struct ne6x_tx_desc *tx_desc = NE6X_TX_DESC(tx_ring, i);

	for (; j < sgl->sg_num; j++) {
		sg = &sgl->sg[j];
		if (likely(sg->flag & NE6X_SG_FRAG_FLAG)) {
			dma = skb_frag_dma_map(tx_ring->dev, sg->p, sg->offset, sg->len,
					       DMA_TO_DEVICE);
		} else {
			dma = dma_map_single(tx_ring->dev, sg->p + sg->offset, sg->len,
					     DMA_TO_DEVICE);
		}

		if (dma_mapping_error(tx_ring->dev, dma))
			goto dma_error;

		tx_bi = &tx_ring->tx_buf[i];

		dma_unmap_len_set(tx_bi, len, sg->len);

		dma_unmap_addr_set(tx_bi, dma, dma);

		if (sg->flag & NE6X_SG_SOP_FLAG) {
			tag_dma = tag_ring->dma + tag_ring->next_to_use * NE6X_TX_PRIV_TAG_SIZE;
			tag_desc = NE6X_TX_TAG(tag_ring, tag_ring->next_to_use);
			ne6x_fill_tx_priv_tag(tx_ring, tag_desc, sgl->mss, sg);
			if (++tag_ring->next_to_use == tag_ring->count)
				tag_ring->next_to_use = 0;
		} else {
			tag_dma = 0;
		}

		tx_desc = NE6X_TX_DESC(tx_ring, i);
		ne6x_fill_tx_desc(tx_desc, tx_ring->reg_idx, tag_dma, dma, sg);
		if (++i == tx_ring->count)
			i = 0;
	}
	tx_ring->next_to_use = i;
	ne6x_maybe_stop_tx(tx_ring, DESC_NEEDED);

	skb_tx_timestamp(skb);

	/* Force memory writes to complete before letting h/w know there
	 * are new descriptors to fetch.
	 *
	 * We also use this memory barrier to make certain all of the
	 * status bits have been updated before next_to_watch is written.
	 */
	wmb();

	/* set next_to_watch value indicating a packet is present */
	first->next_to_watch = tx_desc;
	/* notify HW of packet */
	if (netif_xmit_stopped(txring_txq(tx_ring)) || !netdev_xmit_more())
		ne6x_tail_update(tx_ring, i);

	netdev_tx_sent_queue(txring_txq(tx_ring), first->bytecount);
	first->jumbo_finsh = 1u;

	return;

dma_error:
	dev_info(tx_ring->dev, "TX DMA map failed\n");

	/* clear dma mappings for failed tx_bi map */
	for (;;) {
		tx_bi = &tx_ring->tx_buf[i];
		ne6x_unmap_and_free_tx_resource(tx_ring, tx_bi);
		if (tx_bi == first)
			break;

		if (i == 0)
			i = tx_ring->count;

		i--;
	}

	tx_ring->next_to_use = i;
}

void ne6x_xmit_simple(struct ne6x_ring *tx_ring, struct ne6x_tx_buf *first,
		      struct ne6x_ring *tag_ring, struct ne6x_tx_tag *tx_tag)
{
	struct sk_buff *skb = first->skb;
	struct ne6x_adapt_comm *comm = (struct ne6x_adapt_comm *)tx_ring->adpt;
	struct ne6x_tx_desc *tx_desc, *first_desc;
	unsigned int size = skb_headlen(skb);
	u32 i = tx_ring->next_to_use;
	struct ne6x_tx_tag *ttx_desc;
	struct ne6x_tx_buf *tx_bi;
	bool is_first = true;
	int send_len = 0;
	skb_frag_t *frag;
	dma_addr_t dma;
	__le64 mss = 0;

	dma = dma_map_single(tx_ring->dev, skb->data, size, DMA_TO_DEVICE);

	first_desc = NE6X_TX_DESC(tx_ring, i);
	tx_desc = NE6X_TX_DESC(tx_ring, i);
	mss = tx_desc->mss;
	tx_desc->sop_valid = 1;
	tx_desc->eop_valid = 0;
	tx_bi = first;

	ttx_desc = (struct ne6x_tx_tag  *)tx_tag;
	ttx_desc->tag_pi1 = (comm->port_info & 0x2) ? 1 : 0;
	ttx_desc->tag_pi0 = (comm->port_info & 0x1) ? 1 : 0;
	ttx_desc->tag_vport = (comm->port_info >> 8) & 0xFF;
	ttx_desc->tag_mss = tx_tag->tag_mss;
	ttx_desc->tag_num = 0x0;
	send_len += size;

	for (frag = &skb_shinfo(skb)->frags[0];; frag++) {
		if (dma_mapping_error(tx_ring->dev, dma))
			goto dma_error;

		/* record length, and DMA address */
		dma_unmap_len_set(tx_bi, len, size);
		dma_unmap_addr_set(tx_bi, dma, dma);

		ne6x_tx_desc_push(tx_desc, dma, size);
		tx_desc->vp = tx_ring->reg_idx;
		tx_desc->tso = 0x0;
		tx_desc->l3_csum = 0x00;
		tx_desc->l3_ofst = 0x00;
		tx_desc->l4_csum = 0x00;
		tx_desc->l4_ofst = 0x00;
		tx_desc->pld_ofst = 0x00;
		tx_desc->u.val = 0x0;
		tx_desc->rsv4 = 0;
		if (is_first) {
			tx_desc->sop_valid = 1u;
			is_first = false;
			tx_desc->sop_cnt = 32;
			tx_desc->buffer_sop_addr = cpu_to_le64(first->tag_dma);
		}

		if (send_len == skb->len) {
			tx_desc->eop_valid = 1u;
			break;
		}
		if (++i == tx_ring->count)
			i = 0;

		tx_desc = NE6X_TX_DESC(tx_ring, i);

		size = skb_frag_size(frag);
		send_len += size;
		dma = skb_frag_dma_map(tx_ring->dev, frag, 0, size, DMA_TO_DEVICE);

		tx_bi = &tx_ring->tx_buf[i];
	}

	netdev_tx_sent_queue(txring_txq(tx_ring), first->bytecount);

	if (++i == tx_ring->count)
		i = 0;

	tx_ring->next_to_use = i;
	if (++tag_ring->next_to_use == tag_ring->count)
		tag_ring->next_to_use = 0;

	ne6x_maybe_stop_tx(tx_ring, DESC_NEEDED);

	/* timestamp the skb as late as possible, just prior to notifying
	 * the MAC that it should transmit this packet
	 */
	skb_tx_timestamp(skb);

	/* Force memory writes to complete before letting h/w know there
	 * are new descriptors to fetch.
	 *
	 * We also use this memory barrier to make certain all of the
	 * status bits have been updated before next_to_watch is written.
	 */
	wmb();

	/* set next_to_watch value indicating a packet is present */
	first->next_to_watch = tx_desc;
	/* notify HW of packet */
	if (netif_xmit_stopped(txring_txq(tx_ring)) || !netdev_xmit_more())
		ne6x_tail_update(tx_ring, i);

	return;

dma_error:
	dev_info(tx_ring->dev, "TX DMA map failed\n");

	/* clear dma mappings for failed tx_bi map */
	for (;;) {
		tx_bi = &tx_ring->tx_buf[i];
		ne6x_unmap_and_free_tx_resource(tx_ring, tx_bi);
		if (tx_bi == first)
			break;

		if (i == 0)
			i = tx_ring->count;

		i--;
	}

	tx_ring->next_to_use = i;
}

netdev_tx_t ne6x_xmit_frame_ring(struct sk_buff *skb, struct ne6x_ring *tx_ring,
				 struct ne6x_ring *tag_ring, bool jumbo_frame)
{
	struct ne6x_tx_tag *tx_tagx = NE6X_TX_TAG(tag_ring, tag_ring->next_to_use);
	struct ne6x_tx_buf *first;
	int tso, count;

	/* prefetch the data, we'll need it later */
	prefetch(tx_tagx);
	prefetch(skb->data);

	if (!jumbo_frame) {
		count = ne6x_xmit_descriptor_count(skb);
	} else {
		if (ne6x_fill_jumbo_sgl(tx_ring, skb)) {
			dev_kfree_skb_any(skb);
			return NETDEV_TX_OK;
		}
		count = tx_ring->sgl->sg_num;
	}
	/* reserve 5 descriptors to avoid tail over-write */
	if (ne6x_maybe_stop_tx(tx_ring, count + 4 + 1)) {
		/* this is a hard error */
		tx_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}

	/* record the location of the first descriptor for this packet */
	first = &tx_ring->tx_buf[tx_ring->next_to_use];
	first->skb = skb;
	first->bytecount = skb->len;
	first->gso_segs = 1;
	/* record initial flags and protocol */

	first->jumbo_frame = 0;
	first->jumbo_finsh = 0;
	first->tag_dma = tag_ring->dma + tag_ring->next_to_use * sizeof(struct ne6x_tx_tag);
	memset(tx_tagx, 0x00, sizeof(*tx_tagx));

	ne6x_tx_prepare_vlan_flags(tx_ring, first, tx_tagx);

	tso = ne6x_tso(tx_ring, first, tx_tagx);
	if (tso < 0)
		goto out_drop;

	tso = ne6x_tx_csum(tx_ring, first, tx_tagx);
	if (tso < 0)
		goto out_drop;

	tx_tagx->tag_mss = cpu_to_be16(tx_tagx->tag_mss);

	if (!jumbo_frame) {
		ne6x_xmit_simple(tx_ring, first, tag_ring, tx_tagx);
	} else  {
		first->jumbo_frame = true;
		ne6x_xmit_jumbo(tx_ring, first, tag_ring, tx_tagx);
	}

	return NETDEV_TX_OK;

out_drop:
	ne6x_unmap_and_free_tx_resource(tx_ring, first);

	return NETDEV_TX_OK;
}
