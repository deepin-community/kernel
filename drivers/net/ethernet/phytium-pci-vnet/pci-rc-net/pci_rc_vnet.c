// SPDX-License-Identifier: GPL-2.0
/**
 * Virtual network driver based on the pcie interface in RC
 *
 * Copyright (C) 2023 Phytium Corporation
 * Author: netgroup@phytium.com.cn
 */

#include "pci_rc_vnet.h"

static int debug = 3;
static const char dev_addr[6] = {0x04, 0x0c, 0x00, 0x00, 0x0d, 0x05};

static const char pci_rc_vnet_gstrings_stats[][ETH_GSTRING_LEN] = {
	"rx_packets", "tx_packets", "rx_bytes", "tx_bytes", "rx_errors",
	"tx_errors", "rx_dropped", "tx_dropped", "multicast", "collisions",
	"rx_length_errors", "rx_over_errors", "rx_crc_errors",
	"rx_frame_errors", "rx_fifo_errors", "rx_missed_errors",
	"tx_aborted_errors", "tx_carrier_errors", "tx_fifo_errors",
	"tx_heartbeat_errors", "tx_window_errors",
};

#define PCI_RC_VNET_NET_STATS_LEN	21
#define PCI_RC_VNET_STATS_LEN	ARRAY_SIZE(pci_rc_vnet_gstrings_stats)

static const char pci_rc_vnet_gstrings_test[][ETH_GSTRING_LEN] = {
};

#define PCI_RC_VNET_TEST_LEN	ARRAY_SIZE(pci_rc_vnet_gstrings_test)

static inline int mul32(int size)
{
	int ret = 0;

	if (size % 32)
		ret = size  + (32 - size % 32);
	else
		ret = size;

	return ret;
}

static struct sk_buff *rc_build_skb(struct rc_rx_buffer *rx_buffer,
				    unsigned int size)
{
	struct sk_buff *skb;
	unsigned int truesize;
	void *va;

#if (PAGE_SIZE < 8192)
	truesize = RC_RX_PAGE_SIZE / 2;
#else
	truesize = SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
		   SKB_DATA_ALIGN(RC_SKB_PAD + size);
#endif

	va = page_address(rx_buffer->page) + rx_buffer->page_offset;
	/* prefetch first cache line of first page */
	prefetch(va);

	/* build an skb around the page buffer */
	skb = build_skb(va - RC_SKB_PAD, truesize);
	if (unlikely(!skb))
		return NULL;

	/* update pointers within the skb to store the data */
	skb_reserve(skb, RC_SKB_PAD);
	__skb_put(skb, size);

	/* update buffer offset */
#if (PAGE_SIZE < 8192)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif

	return skb;
}

static int pci_rc_vnet_tx_poll(struct pci_rc_vnet_private *tp, int budget)
{
	int len;
	int work_done = 0;
	u64 ctrl;
	int tx_reclaim_start = 0;
	int tx_reclaim_num = 0;
	struct pci_rc_vnet_dma_desc *tx_desc_ring;
	unsigned long tx_reclaim_lock_flags;

	struct pci_dev *pdev = tp->pci_dev;
	struct device *dev = &pdev->dev;
	int ring_size = tp->params.tx_ring.count;

	if (!(READ_ONCE(tp->tx_queue->flags) & PCI_RC_VNET_FLAGS_EP_LINK)) {
		netdev_info(tp->netdev, "EP Device not online!\n");
		return 0;
	}
	tx_desc_ring = tp->tx_queue->desc_ring;
	spin_lock_irqsave(&tp->tx_reclaim_lock, tx_reclaim_lock_flags);
	tx_reclaim_start = tp->tx_reclaim_start;
	tx_reclaim_num = tp->tx_reclaim_num;

	/* Ensure ctrl is at least as up-to-date as used */
	dma_rmb();
	ctrl = tx_desc_ring[tx_reclaim_start].ctrl;
	while ((work_done < budget) && (work_done < tx_reclaim_num) &&
	       (!(ctrl & PCI_RC_VNET_DMA_DESC_CTRL_FIELD_MASK(USED)))) {
		len = PCI_RC_VNET_DMA_DESC_FIELD_GET(PKT_LEN, ctrl);
		dma_unmap_single(dev, tp->tx_phys_addr_list[tx_reclaim_start],
				 mul32(len), DMA_TO_DEVICE);
		dev_kfree_skb_any(tp->tx_skbuff[tx_reclaim_start]);
		tp->tx_skbuff[tx_reclaim_start] = NULL;
		work_done++;
		tx_reclaim_start++;
		if (tx_reclaim_start == ring_size)
			tx_reclaim_start = 0;

		if (ctrl & PCI_RC_VNET_DMA_DESC_CTRL_FIELD_MASK(ERR)) {
			tp->netdev->stats.tx_dropped++;
		} else {
			tp->netdev->stats.tx_packets++;
			tp->netdev->stats.tx_bytes += len;
		}
		dma_rmb();
		ctrl = tx_desc_ring[tx_reclaim_start].ctrl;
	}

	tp->tx_reclaim_start = tx_reclaim_start;
	tp->tx_reclaim_num -= work_done;
	spin_unlock_irqrestore(&tp->tx_reclaim_lock, tx_reclaim_lock_flags);

	if (netif_queue_stopped(tp->netdev) && (tp->tx_queue->nb_desc - tp->tx_reclaim_num >=
	    PCI_RC_VNET_TX_QUEUE_RESTART_THRESHOLD)) {
		netif_wake_queue(tp->netdev);
	}

	return work_done;
}

static netdev_tx_t pci_rc_vnet_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	u32 tail, head;
	u32 crc32;
	u64 ctrl;
	int len;
	char *data;
	unsigned long tx_lock_flags;
	unsigned long tx_reclaim_lock_flags;
	dma_addr_t phys_addr;
	struct pci_rc_vnet_private *tp = netdev_priv(netdev);
	struct pci_rc_vnet_tx_queue *tx_queue = tp->tx_queue;
	struct pci_dev *pdev = tp->pci_dev;
	struct device *dev = &pdev->dev;
	int ring_size = tp->params.tx_ring.count;

	if (!(READ_ONCE(tp->tx_queue->flags) & PCI_RC_VNET_FLAGS_EP_LINK)) {
		netdev_info(tp->netdev, "EP Device not online!\n");
		return NETDEV_TX_BUSY;
	}

	data = skb->data;
	len = skb->len;

	if (unlikely(tx_queue->nb_desc - tp->tx_reclaim_num <=
		     PCI_RC_VNET_TX_QUEUE_STOP_THRESHOLD)) {
		netif_stop_queue(tp->netdev);
		netdev_info(tp->netdev,
			    "desc queue reclaim is not finished\n");
		return NETDEV_TX_BUSY;
	}

	spin_lock_irqsave(&tp->tx_lock, tx_lock_flags);

	tail = READ_ONCE(tx_queue->tail);
	head = READ_ONCE(tx_queue->head);

	crc32 = crc32_le(~0, data, len);

	phys_addr = dma_map_single(dev, data, mul32(len), DMA_TO_DEVICE);
	if (dma_mapping_error(dev, phys_addr)) {
		spin_unlock_irqrestore(&tp->tx_lock, tx_lock_flags);
		netdev_err(tp->netdev, "tx map failed\n");
		return NETDEV_TX_BUSY;
	}

	dma_sync_single_range_for_device(dev, phys_addr, 0, mul32(len), DMA_TO_DEVICE);

	ctrl = PCI_RC_VNET_DMA_DESC_SET(USED, 1) |
	       PCI_RC_VNET_DMA_DESC_SET(PKT_LEN, len) |
	       PCI_RC_VNET_DMA_DESC_SET(CHECKSUM, crc32);


	if (unlikely(head == (tail + 1) % tx_queue->nb_desc)) {
		spin_unlock_irqrestore(&tp->tx_lock, tx_lock_flags);
		netif_stop_queue(tp->netdev);
		dma_unmap_single(dev, phys_addr, mul32(len), DMA_TO_DEVICE);
		netdev_info(tp->netdev, "tx_desc_queue_is_full\n");
		return NETDEV_TX_BUSY;
	}

	tx_queue->desc_ring[tail].addr = phys_addr;
	tx_queue->desc_ring[tail].ctrl = ctrl;

	/* Make descriptor updates visible to device */
	wmb();

	spin_lock_irqsave(&tp->tx_reclaim_lock, tx_reclaim_lock_flags);
	tp->tx_reclaim_num++;
	spin_unlock_irqrestore(&tp->tx_reclaim_lock, tx_reclaim_lock_flags);

	tp->tx_skbuff[tail] = skb;
	tp->tx_phys_addr_list[tail] = phys_addr;

	tail = (tail + 1) % ring_size;
	WRITE_ONCE(tx_queue->tail, tail);

	writel(SEND_MSI_IRQ, tp->msi_irq_addr);

	spin_unlock_irqrestore(&tp->tx_lock, tx_lock_flags);

	return NETDEV_TX_OK;
}

static bool rc_alloc_mapped_page(struct pci_rc_vnet_private *tp,
				 struct rc_rx_buffer *rx_buffer_info)
{
	dma_addr_t paddr;
	struct page *page = rx_buffer_info->page;
	struct pci_rc_vnet_dma_desc *rx_desc_ring;

	rx_desc_ring = tp->rx_queue->desc_ring;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page))
		return true;

	page = __dev_alloc_pages(RC_GFP_FLAGS, RC_RX_PAGE_ORDER);
	if (unlikely(!page)) {
		netdev_err(tp->netdev, "rx alloc page failed\n");
		rx_buffer_info->page = NULL;
		return false;
	}

	paddr = dma_map_page_attrs(&tp->pci_dev->dev, page, 0,
				   RC_RX_PAGE_SIZE,
				   DMA_FROM_DEVICE, RC_RX_DMA_ATTR);
	if (dma_mapping_error(&tp->pci_dev->dev, paddr)) {
		__free_pages(page, RC_RX_PAGE_ORDER);
		rx_buffer_info->page = NULL;
		return false;
	}

	rx_buffer_info->addr = paddr;
	rx_buffer_info->page = page;
	rx_buffer_info->pagecnt_bias = 1;
	rx_buffer_info->page_offset = RC_SKB_PAD;

	return true;
}

static inline int rc_calc_rx_buf_len(void)
{
#if (PAGE_SIZE < 8192)
	return rounddown(RC_MAX_FRAME_BUILD_SKB, RX_BUFFER_MULTIPLE);
#endif
	return rounddown(RC_RXBUFFER_2048, RX_BUFFER_MULTIPLE);
}

static void pci_rc_vnet_rx_refill(struct pci_rc_vnet_private *tp, bool init)
{
	unsigned int entry, space;
	struct rc_rx_buffer *rx_buffer_info;
	struct pci_rc_vnet_dma_desc *rx_desc_ring;
	int ring_size = tp->params.rx_ring.count;

	rx_desc_ring = tp->rx_queue->desc_ring;

	if (init)
		space = ring_size - 1;
	else
		space = CIRC_SPACE(tp->rx_refill_start,
				   READ_ONCE(tp->queue->rx_queue.head),
				   ring_size);

	while (space > 0) {
		entry = tp->rx_refill_start & (ring_size - 1);
		rx_buffer_info = &tp->rx_buffer_info[entry];
		if (!rc_alloc_mapped_page(tp, rx_buffer_info))
			break;
		/* sync the buffer for use by the device */
		dma_sync_single_range_for_device(&tp->pci_dev->dev, rx_buffer_info->addr,
						 rx_buffer_info->page_offset,
						 tp->rx_buffer_len, DMA_FROM_DEVICE);
		rx_desc_ring[tp->rx_refill_start].addr =
			rx_buffer_info->addr + rx_buffer_info->page_offset;
		rx_desc_ring[tp->rx_refill_start].ctrl = 0;

		dma_wmb();

		tp->rx_refill_start = (tp->rx_refill_start + 1) & (ring_size - 1);
		space--;
	}

	tp->rx_next_to_alloc = tp->rx_refill_start;
}

static int pci_rc_vnet_rx_buff_alloc(struct pci_rc_vnet_private *tp)
{
	int ring_size = tp->params.rx_ring.count;
	struct pci_rc_vnet_dma_desc *desc_ring;

	tp->rx_buffer_len = rc_calc_rx_buf_len();
	desc_ring = tp->rx_queue->desc_ring;
	tp->rx_buffer_info = vzalloc(ring_size * sizeof(struct rc_rx_buffer));
	if (!tp->rx_buffer_info) {
		netdev_err(tp->netdev, "Unable to allocate rx_buffer_info memory\n");
		return -ENOMEM;
	}

	/* Pre allocated rx skb */
	pci_rc_vnet_rx_refill(tp, true);

	return 0;
}

static void pci_rc_vnet_rx_buff_free(struct pci_rc_vnet_private *tp)
{
	int i;
	int ring_size = tp->params.rx_ring.count;

	for (i = 0; i < ring_size; i++) {
		if (tp->rx_buffer_info) {
			if (tp->rx_buffer_info[i].page) {
				dma_sync_single_range_for_cpu(&tp->pci_dev->dev,
							      tp->rx_buffer_info[i].addr,
							      0, RC_RX_PAGE_SIZE,
							      DMA_FROM_DEVICE);
				/* free resources associated with mapping */
				dma_unmap_page_attrs(&tp->pci_dev->dev,
						     tp->rx_buffer_info[i].addr,
						     RC_RX_PAGE_SIZE,
						     DMA_FROM_DEVICE,
						     RC_RX_DMA_ATTR);
				__page_frag_cache_drain(tp->rx_buffer_info[i].page,
							tp->rx_buffer_info[i].pagecnt_bias);
			}
		}
	}

	vfree(tp->rx_buffer_info);
	tp->rx_buffer_info = NULL;
}

static int pci_rc_vnet_tx_buff_alloc(struct pci_rc_vnet_private *tp)
{
	int ret = 0, i;
	int ring_size = tp->params.tx_ring.count;
	dma_addr_t *tx_phys_addr_list;
	struct sk_buff **tx_skbuff;

	tx_skbuff = kcalloc(ring_size, sizeof(struct sk_buff *), GFP_KERNEL);
	if (!tx_skbuff) {
		ret = -ENOMEM;
		goto err_failed_alloc_skbuff;
	}
	for (i = 0; i < ring_size; i++)
		tx_skbuff[i] = NULL;

	tp->tx_skbuff = tx_skbuff;

	tx_phys_addr_list = kmalloc_array(ring_size, sizeof(dma_addr_t), GFP_KERNEL);
	if (!tx_phys_addr_list) {
		ret = -ENOMEM;
		goto err_free_skbuff;
	}
	tp->tx_phys_addr_list = tx_phys_addr_list;
	return 0;

err_free_skbuff:
	kfree(tp->tx_skbuff);
err_failed_alloc_skbuff:
	return ret;
}

static void pci_rc_vnet_tx_buff_clean(struct pci_rc_vnet_private *tp)
{
	int len, i;
	int ring_size = tp->params.tx_ring.count;
	struct pci_dev *pdev = tp->pci_dev;
	struct device *dev = &pdev->dev;
	unsigned long tx_lock_flag;
	unsigned long tx_reclaim_lock_flag;
	u32 tx_tail;

	spin_lock_irqsave(&tp->tx_lock, tx_lock_flag);
	spin_lock_irqsave(&tp->tx_reclaim_lock, tx_reclaim_lock_flag);

	for (i = 0; i < ring_size; i++) {
		if (tp->tx_skbuff[i]) {
			len = tp->tx_skbuff[i]->len;
			dma_unmap_single(dev, tp->tx_phys_addr_list[i],
					 mul32(len), DMA_TO_DEVICE);
			tp->tx_queue->desc_ring[i].ctrl = 0;
			dev_kfree_skb_any(tp->tx_skbuff[i]);
		}
	}
	tp->tx_reclaim_num = 0;
	tx_tail = READ_ONCE(tp->tx_queue->tail);
	tp->tx_reclaim_start = tx_tail;
	WRITE_ONCE(tp->tx_queue->head, tx_tail);

	spin_unlock_irqrestore(&tp->tx_reclaim_lock, tx_reclaim_lock_flag);
	spin_unlock_irqrestore(&tp->tx_lock, tx_lock_flag);
}

static void pci_rc_vnet_tx_buff_free(struct pci_rc_vnet_private *tp)
{
	kfree(tp->tx_skbuff);
	tp->tx_skbuff = NULL;
	kfree(tp->tx_phys_addr_list);
	tp->tx_phys_addr_list = NULL;
}

static int pci_rc_vnet_up(struct pci_rc_vnet_private *tp)
{
	int err = 0;

	/* Apply for DMA buffer */
	if (pci_rc_vnet_tx_buff_alloc(tp)) {
		err = -ENOMEM;
		netdev_err(tp->netdev, "Failed to allocate tx buffer\n");
		goto err_failed_alloc_tx_buff;
	}
	if (pci_rc_vnet_rx_buff_alloc(tp)) {
		err = -ENOMEM;
		netdev_err(tp->netdev, "Failed to allocate rx buffer\n");
		goto err_free_tx_buff;
	}

	tp->netdev->flags |= IFF_UP;

	netif_wake_queue(tp->netdev);
	return err;

err_free_tx_buff:
	pci_rc_vnet_tx_buff_free(tp);

err_failed_alloc_tx_buff:
	return err;
}

static void pci_rc_vnet_down(struct pci_rc_vnet_private *tp)
{
	netif_stop_queue(tp->netdev);

	netif_carrier_off(tp->netdev);

	msleep(200);
	pci_rc_vnet_tx_buff_clean(tp);
	pci_rc_vnet_tx_buff_free(tp);
	pci_rc_vnet_rx_buff_free(tp);
}

static bool rc_can_reuse_rx_page(struct rc_rx_buffer *rx_buffer)
{
	unsigned int pagecnt_bias = rx_buffer->pagecnt_bias;
	struct page *page = rx_buffer->page;

	/* avoid re-using remote and pfmemalloc pages */
	if (!dev_page_is_reusable(page))
		return false;

#if (PAGE_SIZE < 8192)
	/* if we are only owner of page we can reuse it */
	if (unlikely((page_ref_count(page) - pagecnt_bias) > 1))
		return false;
#else
#define RC_LAST_OFFSET \
	(SKB_WITH_OVERHEAD(PAGE_SIZE) - RC_RXBUFFER_2048)

	if (rx_buffer->page_offset > RC_LAST_OFFSET)
		return false;
#endif

	/* If we have drained the page fragment pool we need to update
	 * the pagecnt_bias and page count so that we fully restock the
	 * number of references the driver holds.
	 */
	if (unlikely(!pagecnt_bias)) {
		page_ref_add(page, USHRT_MAX);
		rx_buffer->pagecnt_bias = USHRT_MAX;
	}

	return true;
}

static void rc_reuse_rx_page(struct pci_rc_vnet_private *tp,
			     struct rc_rx_buffer *old_buff)
{
	struct rc_rx_buffer *new_buff;
	u16 nta = tp->rx_next_to_alloc;
	int ring_size = tp->params.rx_ring.count;
	struct pci_rc_vnet_dma_desc *rx_desc_ring;

	rx_desc_ring = tp->rx_queue->desc_ring;

	new_buff = &tp->rx_buffer_info[nta & (ring_size - 1)];

	/* update, and store next to alloc */
	nta++;
	tp->rx_next_to_alloc = (nta < ring_size) ? nta : 0;

	/* Transfer page from old buffer to new buffer.
	 * Move each member individually to avoid possible store
	 * forwarding stalls.
	 */
	new_buff->addr		= old_buff->addr;
	new_buff->page		= old_buff->page;
	new_buff->page_offset	= old_buff->page_offset;
	new_buff->pagecnt_bias	= old_buff->pagecnt_bias;
}

static void rc_put_rx_buffer(struct pci_rc_vnet_private *tp,
			     struct rc_rx_buffer *rx_buffer_info)
{
	if (rc_can_reuse_rx_page(rx_buffer_info)) {
		/* hand second half of page back to the ring */
		rc_reuse_rx_page(tp, rx_buffer_info);
	} else {
		dma_unmap_page_attrs(&tp->pci_dev->dev,
				     rx_buffer_info->addr,
				     RC_RX_PAGE_SIZE,
				     DMA_FROM_DEVICE,
				     RC_RX_DMA_ATTR);
		__page_frag_cache_drain(rx_buffer_info->page,
					rx_buffer_info->pagecnt_bias);
	}

	/* clear contents of rx_buffer */
	rx_buffer_info->page = NULL;
}

static int pci_rc_vnet_rx(struct pci_rc_vnet_private *tp, u32 head, u32 tail)
{
	u32 desc_crc, pkt_crc;
	int used;
	u64 ctrl;
	int pkt_len;
	void *pkt_data;

	struct rc_rx_buffer *rx_buffer_info;
	struct pci_rc_vnet_rx_queue *rx_queue;
	struct pci_rc_vnet_dma_desc *rx_desc_ring;
	struct sk_buff	*skb;
	int ring_size = tp->params.tx_ring.count;
	unsigned int pkt_num, count = 0;

	rx_queue = tp->rx_queue;
	rx_desc_ring = rx_queue->desc_ring;

	if (tail >= head)
		pkt_num = tail - head;
	else
		pkt_num = ring_size - head + tail;

	for (count = 0; count < pkt_num; count++) {
		/* Ensure ctrl is at least as up-to-date as rxused */
		dma_rmb();

		ctrl = rx_desc_ring[head].ctrl;
		used = PCI_RC_VNET_DMA_DESC_FIELD_GET(USED, ctrl);

		pkt_len = PCI_RC_VNET_DMA_DESC_FIELD_GET(PKT_LEN, ctrl);

		/* No package to receive */
		if (!used)
			break;

		/* get rx buffer */
		rx_buffer_info = &tp->rx_buffer_info[head & (ring_size - 1)];
		if (!rx_buffer_info->page) {
			netdev_err(tp->netdev,
				   "%s: The page used is null!\n", __func__);
			break;
		}
		rx_buffer_info->pagecnt_bias--;

		dma_sync_single_range_for_cpu(&tp->pci_dev->dev, rx_buffer_info->addr,
					      rx_buffer_info->page_offset,
					      pkt_len, DMA_FROM_DEVICE);

		pkt_data = page_address(rx_buffer_info->page) +
			   rx_buffer_info->page_offset - RC_SKB_PAD;
		pkt_crc = crc32_le(~0, (unsigned char *)pkt_data, pkt_len);
		desc_crc = PCI_RC_VNET_DMA_DESC_FIELD_GET(CHECKSUM, ctrl);
		if (desc_crc != pkt_crc) {
			tp->netdev->stats.rx_errors++;
			rx_buffer_info->pagecnt_bias++;
			rc_put_rx_buffer(tp, rx_buffer_info);
			goto refresh;
		}

		skb = rc_build_skb(rx_buffer_info, pkt_len);
		if (unlikely(!skb)) {
			netdev_err(tp->netdev, "rx build skb failed\n");
			tp->netdev->stats.rx_dropped++;
			rx_buffer_info->pagecnt_bias++;
			break;
		}

		rc_put_rx_buffer(tp, rx_buffer_info);

		skb->protocol = eth_type_trans(skb, tp->netdev);
		/* Upload protocol stack */
		netif_receive_skb(skb);

		tp->netdev->stats.rx_packets++;
		tp->netdev->stats.rx_bytes += pkt_len;
refresh:
		head = (head + 1) % ring_size;
	}

	WRITE_ONCE(rx_queue->head, head);

	pci_rc_vnet_rx_refill(tp, false);

	return count;
}

static void pci_rc_vnet_get_drvinfo(struct net_device *netdev, struct ethtool_drvinfo *info)
{
	struct pci_rc_vnet_private *tp = netdev_priv(netdev);

	strscpy(info->driver, DRV_MODULE_NAME, sizeof(info->driver));
	strscpy(info->version, DRV_VERSION, sizeof(info->version));
	strscpy(info->bus_info, pci_name(tp->pci_dev),
		sizeof(info->bus_info));
}

static u32 pci_rc_vnet_get_msglevel(struct net_device *netdev)
{
	struct pci_rc_vnet_private *tp = netdev_priv(netdev);

	return tp->msg_enable;
}

static void pci_rc_vnet_set_msglevel(struct net_device *netdev, u32 value)
{
	struct pci_rc_vnet_private *tp = netdev_priv(netdev);

	tp->msg_enable = value;
}

static u32 pci_rc_vnet_get_link(struct net_device *netdev)
{
	struct pci_rc_vnet_private *tp = netdev_priv(netdev);
	u32 tx_flags;

	tx_flags = READ_ONCE(tp->tx_queue->flags);
	return tx_flags & PCI_RC_VNET_FLAGS_EP_LINK;
}

static void pci_rc_vnet_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
	switch (stringset) {
	case ETH_SS_TEST:
		memcpy(data, *pci_rc_vnet_gstrings_test, sizeof(pci_rc_vnet_gstrings_test));
		break;
	case ETH_SS_STATS:
		memcpy(data, *pci_rc_vnet_gstrings_stats, sizeof(pci_rc_vnet_gstrings_stats));
		break;
	}
}

static int pci_rc_vnet_get_sset_count(struct net_device *netdev, int sset)
{
	switch (sset) {
	case ETH_SS_TEST:
		return PCI_RC_VNET_TEST_LEN;
	case ETH_SS_STATS:
		return PCI_RC_VNET_STATS_LEN;
	default:
		return -EOPNOTSUPP;
	}
}

static void pci_rc_vnet_get_ethtool_stats(struct net_device *netdev,
					  struct ethtool_stats *stats, u64 *data)
{
	int i;

	for (i = 0; i < PCI_RC_VNET_NET_STATS_LEN; i++)
		data[i] = ((unsigned long *)&netdev->stats)[i];
}

static const struct ethtool_ops pci_rc_vnet_ethtool_ops = {
	.get_drvinfo		= pci_rc_vnet_get_drvinfo,
	.get_msglevel		= pci_rc_vnet_get_msglevel,
	.set_msglevel		= pci_rc_vnet_set_msglevel,
	.get_link		= pci_rc_vnet_get_link,
	.get_strings		= pci_rc_vnet_get_strings,
	.get_ethtool_stats	= pci_rc_vnet_get_ethtool_stats,
	.get_sset_count	= pci_rc_vnet_get_sset_count,
};

static int pci_rc_vnet_open(struct net_device *dev)
{
	int err;
	struct pci_rc_vnet_private *tp = netdev_priv(dev);
	struct pci_rc_vnet_rx_queue *rx_queue;
	u32 rx_flags;

	rx_queue = tp->rx_queue;
	WRITE_ONCE(rx_queue->head, READ_ONCE(rx_queue->tail));

	netif_carrier_off(dev);
	err = pci_rc_vnet_up(tp);
	if (err)
		goto err;

	hrtimer_start(&tp->link_detect_timer, ns_to_ktime(PCI_RC_VNET_LINK_DETECT_PERIOD_NS),
		      HRTIMER_MODE_REL_SOFT);
	hrtimer_start(&tp->receive_timer, ns_to_ktime(PCI_RC_VNET_RECEIVE_PERIOD_NS),
		      HRTIMER_MODE_REL_SOFT);
	hrtimer_start(&tp->tx_poll_timer, ns_to_ktime(PCI_RC_VNET_TX_POLL_PERIOD_NS),
		      HRTIMER_MODE_REL_SOFT);
	rx_flags |= PCI_RC_VNET_FLAGS_RC_LINK;
	WRITE_ONCE(tp->rx_queue->flags, rx_flags);

	return 0;

err:
	return err;
}

static int pci_rc_vnet_close(struct net_device *netdev)
{
	struct pci_rc_vnet_private *tp = netdev_priv(netdev);
	u32 rx_flags;

	rx_flags = READ_ONCE(tp->rx_queue->flags);
	rx_flags &= ~PCI_RC_VNET_FLAGS_RC_LINK;
	WRITE_ONCE(tp->rx_queue->flags, rx_flags);

	hrtimer_cancel(&tp->link_detect_timer);
	hrtimer_cancel(&tp->receive_timer);
	hrtimer_cancel(&tp->tx_poll_timer);

	pci_rc_vnet_down(tp);

	return 0;
}

static const struct net_device_ops pci_rc_netdev_ops = {
	.ndo_open		= pci_rc_vnet_open,
	.ndo_stop		= pci_rc_vnet_close,
	.ndo_start_xmit		= pci_rc_vnet_start_xmit,
};

static void pci_rc_vnet_get_defaults(struct pci_rc_vnet_private *tp)
{
	struct param_range rx_ring = { .min = 64, .max = 2048, .count = 512 };
	struct param_range tx_ring = { .min = 64, .max = 2048, .count = 512 };

	tp->params.rx_ring = rx_ring;
	tp->params.tx_ring = tx_ring;
	tp->rx_buffer_len = PCI_RC_VNET_RXBUFFER_2048;

	tp->tx_reclaim_start = READ_ONCE(tp->tx_queue->tail);
	tp->tx_reclaim_num = 0;
	tp->rx_refill_start = READ_ONCE(tp->rx_queue->head);
	tp->rx_next_to_alloc = tp->rx_refill_start;

	tp->params.tx_ring.count = tp->tx_queue->nb_desc;
	tp->params.rx_ring.count = tp->rx_queue->nb_desc;
}

static enum hrtimer_restart link_detect_timer_handle(struct hrtimer *timer)
{
	struct pci_rc_vnet_private *tp =
		container_of(timer, struct pci_rc_vnet_private, link_detect_timer);
	int ret;

	ret = READ_ONCE(tp->tx_queue->flags) & PCI_RC_VNET_FLAGS_EP_LINK;
	if (ret && !(netif_carrier_ok(tp->netdev)))
		netif_carrier_on(tp->netdev);
	else if (!ret && netif_carrier_ok(tp->netdev))
		netif_carrier_off(tp->netdev);

	hrtimer_forward_now(timer, ns_to_ktime(PCI_RC_VNET_LINK_DETECT_PERIOD_NS));
	return HRTIMER_RESTART;
}

static enum hrtimer_restart pci_epf_vnet_tx_poll_callback(struct hrtimer *timer)
{
	struct pci_rc_vnet_private *tp =
		container_of(timer, struct pci_rc_vnet_private, tx_poll_timer);
	int tx_reclaim_start, head, budget;
	int ring_size = tp->params.tx_ring.count;

	tx_reclaim_start = READ_ONCE(tp->tx_reclaim_start);
	head = READ_ONCE(tp->tx_queue->head);

	if (tx_reclaim_start != head) {
		if (tx_reclaim_start < head)
			budget = head - tx_reclaim_start;
		else
			budget = ring_size - tx_reclaim_start + head;

		pci_rc_vnet_tx_poll(tp, budget);
	}

	hrtimer_forward_now(timer, ns_to_ktime(PCI_RC_VNET_TX_POLL_PERIOD_NS));
	return HRTIMER_RESTART;
}

static enum hrtimer_restart pci_epf_vnet_ep2rc_callback(struct hrtimer *timer)
{
	u32 head;
	u32 tail;
	int ret = 0;
	struct pci_rc_vnet_private *tp =
		container_of(timer, struct pci_rc_vnet_private, receive_timer);
	struct pci_rc_vnet_rx_queue *rx_queue;

	rx_queue = tp->rx_queue;
	head = READ_ONCE(rx_queue->head);
	tail = READ_ONCE(rx_queue->tail);

	/* Receive packets */
	ret = READ_ONCE(tp->tx_queue->flags) & PCI_RC_VNET_FLAGS_EP_LINK;
	if (ret) {
		if (head != tail) {
			ret = pci_rc_vnet_rx(tp, head, tail);
			if (!ret) {
				netdev_err(tp->netdev, "data packet not received\n");
				goto next_cycle;
			}
		}
	}

next_cycle:
	hrtimer_forward_now(timer, ns_to_ktime(PCI_RC_VNET_RECEIVE_PERIOD_NS));
	return HRTIMER_RESTART;
}

static int pci_rc_vnet_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int err;
	int rc;
	enum pci_barno bar;
	struct net_device *netdev;
	resource_size_t mmio_start, mmio_len;
	struct pci_rc_vnet_private *tp;
	uintptr_t msi_irq_addr;
	enum pci_barno bar_index = BAR_2;

	if (pci_is_bridge(pdev))
		return -ENODEV;

	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (err) {
		dev_err(&pdev->dev, "dma_set_mask failed\n");
		return err;
	}

	netdev = alloc_etherdev(sizeof(struct pci_rc_vnet_private));

	if (!netdev)
		return -ENOMEM;

	mmio_start = pci_resource_start(pdev, 0);
	mmio_len = pci_resource_len(pdev, 0);

	SET_NETDEV_DEV(netdev, &pdev->dev);

	netdev->mem_start = mmio_start;
	netdev->mem_end = mmio_start + mmio_len;

	netdev->features &= ~NETIF_F_HW_CSUM;

	dev_addr_set(netdev, dev_addr);

	netdev->netdev_ops = &pci_rc_netdev_ops;
	netdev->ethtool_ops = &pci_rc_vnet_ethtool_ops;

	netdev->min_mtu = ETH_ZLEN - ETH_HLEN;
	netdev->max_mtu = MAX_JUMBO_FRAME_SIZE - (ETH_HLEN + ETH_FCS_LEN);

	tp = netdev_priv(netdev);
	tp->pci_dev = pdev;
	tp->netdev = netdev;
	tp->msg_enable = (1 << debug) - 1;

	err = pci_enable_device(pdev);
	if (err) {
		netdev_err(tp->netdev, "Cannot enable PCI device\n");
		goto err_free_device;
	}

	err = pci_request_regions(pdev, DRV_MODULE_NAME);
	if (err) {
		netdev_err(tp->netdev, "Cannot obtain PCI resources\n");
		goto err_disable_pdev;
	}

	pci_set_master(pdev);

	for (bar = BAR_0; bar <= BAR_5; bar++) {
		if (pci_resource_flags(pdev, bar) & IORESOURCE_MEM)
			tp->bar[bar] = pci_ioremap_bar(pdev, bar);
	}

	tp->queue = tp->bar[bar_index];
	msi_irq_addr = (uintptr_t)tp->bar[BAR_1];
	tp->msi_irq_addr = (void *)msi_irq_addr;

	tp->tx_queue = &tp->queue->tx_queue;
	if (!tp->tx_queue) {
		err = -ENOMEM;
		netdev_err(tp->netdev,
			   "Cannot perform PCI tp without BAR%d\n", bar_index);
		goto err_iounmap;
	}
	tp->rx_queue = &tp->queue->rx_queue;
	if (!tp->rx_queue) {
		err = -ENOMEM;
		netdev_err(tp->netdev,
			   "Cannot perform PCI tp without BAR%d\n", bar_index);
		goto err_iounmap;
	}

	pci_set_drvdata(pdev, tp);

	/* Register as a network device */
	rc = register_netdev(netdev);

	if (rc) {
		netdev_err(tp->netdev, "Unable to register_netdev\n");
		goto err_iounmap;
	}
	pci_rc_vnet_get_defaults(tp);

	spin_lock_init(&tp->tx_lock);
	spin_lock_init(&tp->tx_reclaim_lock);
	spin_lock_init(&tp->rx_lock);

	hrtimer_init(&tp->link_detect_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_SOFT);
	tp->link_detect_timer.function = link_detect_timer_handle;

	hrtimer_init(&tp->receive_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_SOFT);
	tp->receive_timer.function = pci_epf_vnet_ep2rc_callback;

	hrtimer_init(&tp->tx_poll_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_SOFT);
	tp->tx_poll_timer.function = pci_epf_vnet_tx_poll_callback;

	return 0;

err_iounmap:
	for (bar = BAR_0; bar <= BAR_5; bar++) {
		if (tp->bar[bar])
			pci_iounmap(pdev, tp->bar[bar]);
	}

err_disable_pdev:
	pci_disable_device(pdev);

err_free_device:
	free_netdev(netdev);
	return err;
}

static void pci_rc_vnet_remove(struct pci_dev *pdev)
{
	enum pci_barno bar;
	struct pci_rc_vnet_private *tp = pci_get_drvdata(pdev);
	struct net_device *net_dev = tp->netdev;

	unregister_netdev(net_dev);

	for (bar = BAR_0; bar <= BAR_5; bar++) {
		if (tp->bar[bar])
			pci_iounmap(pdev, tp->bar[bar]);
	}

	pci_release_regions(pdev);
	free_netdev(net_dev);
	pci_disable_device(pdev);
}

static const struct pci_device_id pci_rc_vnet_tbl[] = {
	{ PCI_DEVICE(0x16c3, 0xedda) },
	{ }
};

static struct pci_driver pci_rc_vnet_driver = {
	.name = DRV_MODULE_NAME,
	.id_table = pci_rc_vnet_tbl,
	.probe = pci_rc_vnet_probe,
	.remove = pci_rc_vnet_remove,
};

module_pci_driver(pci_rc_vnet_driver);
MODULE_DESCRIPTION("Virtual network driver based on the pcie interface in RC");
MODULE_AUTHOR("litongfeng1497@phytium.com.cn");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
