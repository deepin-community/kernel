/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2021 Motorcomm Corporation. */

#include "fuxi-gmac.h"
#include "fuxi-gmac-reg.h"

static void fxgmac_unmap_desc_data(struct fxgmac_pdata *pdata,
				   struct fxgmac_desc_data *desc_data)
{
	if (desc_data->skb_dma) {
		if (desc_data->mapped_as_page) {
			dma_unmap_page(pdata->dev, desc_data->skb_dma,
				       desc_data->skb_dma_len, DMA_TO_DEVICE);
		} else {
			dma_unmap_single(pdata->dev, desc_data->skb_dma,
					 desc_data->skb_dma_len, DMA_TO_DEVICE);
		}
		desc_data->skb_dma = 0;
		desc_data->skb_dma_len = 0;
	}

	if (desc_data->rx.buf.dma_base) {
		dma_unmap_single(pdata->dev, desc_data->rx.buf.dma_base,
				 pdata->rx_buf_size, DMA_FROM_DEVICE);
		desc_data->rx.buf.dma_base = 0;
	}

	if (desc_data->skb) {
		dev_kfree_skb_any(desc_data->skb);
		desc_data->skb = NULL;
	}

	memset(&desc_data->tx, 0, sizeof(desc_data->tx));
	memset(&desc_data->rx, 0, sizeof(desc_data->rx));

	desc_data->mapped_as_page = 0;

	if (desc_data->state_saved) {
		desc_data->state_saved = 0;
		desc_data->state.skb = NULL;
		desc_data->state.len = 0;
		desc_data->state.error = 0;
	}
}

static void fxgmac_free_ring(struct fxgmac_pdata *pdata,
			     struct fxgmac_ring *ring)
{
	struct fxgmac_desc_data *desc_data;
	unsigned int i;

	if (!ring)
		return;

	if (ring->desc_data_head) {
		for (i = 0; i < ring->dma_desc_count; i++) {
			desc_data = FXGMAC_GET_DESC_DATA(ring, i);
			fxgmac_unmap_desc_data(pdata, desc_data);
		}

		kfree(ring->desc_data_head);
		ring->desc_data_head = NULL;
	}

	if (ring->dma_desc_head) {
		dma_free_coherent(
			pdata->dev,
			(sizeof(struct fxgmac_dma_desc) * ring->dma_desc_count),
			ring->dma_desc_head, ring->dma_desc_head_addr);
		ring->dma_desc_head = NULL;
	}
}

static int fxgmac_init_ring(struct fxgmac_pdata *pdata,
			    struct fxgmac_ring *ring,
			    unsigned int dma_desc_count)
{
	if (!ring)
		return 0;
	/* Descriptors */
	ring->dma_desc_count = dma_desc_count;
	ring->dma_desc_head = dma_alloc_coherent(
		pdata->dev, (sizeof(struct fxgmac_dma_desc) * dma_desc_count),
		&ring->dma_desc_head_addr, GFP_KERNEL);
	if (!ring->dma_desc_head)
		return -ENOMEM;

	/* Array of descriptor data */
	ring->desc_data_head = kcalloc(
		dma_desc_count, sizeof(struct fxgmac_desc_data), GFP_KERNEL);
	if (!ring->desc_data_head)
		return -ENOMEM;

	netif_dbg(
		pdata, drv, pdata->netdev,
		"dma_desc_head=%p, dma_desc_head_addr=%pad, desc_data_head=%p\n",
		ring->dma_desc_head, &ring->dma_desc_head_addr,
		ring->desc_data_head);

	return 0;
}

static void fxgmac_free_rings(struct fxgmac_pdata *pdata)
{
	struct fxgmac_channel *channel;
	unsigned int i;

	if (!pdata->channel_head)
		return;

	channel = pdata->channel_head;
	for (i = 0; i < pdata->channel_count; i++, channel++) {
		fxgmac_free_ring(pdata, channel->tx_ring);
		fxgmac_free_ring(pdata, channel->rx_ring);
	}
}

static int fxgmac_alloc_rings(struct fxgmac_pdata *pdata)
{
	struct fxgmac_channel *channel;
	unsigned int i;
	int ret;

	channel = pdata->channel_head;
	for (i = 0; i < pdata->channel_count; i++, channel++) {
		netif_dbg(pdata, drv, pdata->netdev, "%s - Tx ring:\n",
			  channel->name);

		if (i < pdata->tx_ring_count) {
			ret = fxgmac_init_ring(pdata, channel->tx_ring,
					       pdata->tx_desc_count);

			if (ret) {
				netdev_alert(pdata->netdev,
					     "error initializing Tx ring");
				goto err_init_ring;
			}
		}

		netif_dbg(pdata, drv, pdata->netdev, "%s - Rx ring:\n",
			  channel->name);

		ret = fxgmac_init_ring(pdata, channel->rx_ring,
				       pdata->rx_desc_count);
		if (ret) {
			netdev_alert(pdata->netdev,
				     "error initializing Rx ring\n");
			goto err_init_ring;
		}
		if (netif_msg_drv(pdata))
			DPRINTK("fxgmac_alloc_ring..ch=%u, tx_desc_cnt=%u, rx_desc_cnt=%u\n",
				i, pdata->tx_desc_count, pdata->rx_desc_count);
	}
	if (netif_msg_drv(pdata))
		DPRINTK("alloc_rings callout ok\n");

	return 0;

err_init_ring:
	fxgmac_free_rings(pdata);

	DPRINTK("alloc_rings callout err,%d\n", ret);
	return ret;
}

static void fxgmac_free_channels(struct fxgmac_pdata *pdata)
{
	if (!pdata->channel_head)
		return;
	if (netif_msg_drv(pdata))
		DPRINTK("free_channels, tx_ring=%p\n",
			pdata->channel_head->tx_ring);
	kfree(pdata->channel_head->tx_ring);
	pdata->channel_head->tx_ring = NULL;

	if (netif_msg_drv(pdata))
		DPRINTK("free_channels, rx_ring=%p\n",
			pdata->channel_head->rx_ring);
	kfree(pdata->channel_head->rx_ring);
	pdata->channel_head->rx_ring = NULL;

	if (netif_msg_drv(pdata))
		DPRINTK("free_channels, channel=%p\n", pdata->channel_head);
	kfree(pdata->channel_head);

	pdata->channel_head = NULL;
}

static int fxgmac_alloc_channels(struct fxgmac_pdata *pdata)
{
	struct fxgmac_channel *channel_head, *channel;
	struct fxgmac_ring *tx_ring, *rx_ring;
	int ret = -ENOMEM;
	unsigned int i;

#ifdef CONFIG_PCI_MSI
	u32 msix = FXGMAC_GET_REG_BITS(pdata->expansion.int_flags,
				       FXGMAC_FLAG_MSIX_POS,
				       FXGMAC_FLAG_MSIX_LEN);
#endif

	channel_head = kcalloc(pdata->channel_count,
			       sizeof(struct fxgmac_channel), GFP_KERNEL);
	if (netif_msg_drv(pdata))
		DPRINTK("alloc_channels, channel_head=%p, size=%d*%ld\n",
			channel_head, pdata->channel_count,
			sizeof(struct fxgmac_channel));

	if (!channel_head)
		return ret;

	netif_dbg(pdata, drv, pdata->netdev, "channel_head=%p\n", channel_head);

	tx_ring = kcalloc(pdata->tx_ring_count, sizeof(struct fxgmac_ring),
			  GFP_KERNEL);
	if (!tx_ring)
		goto err_tx_ring;

	if (netif_msg_drv(pdata))
		DPRINTK("alloc_channels, tx_ring=%p, size=%d*%ld\n", tx_ring,
			pdata->tx_ring_count, sizeof(struct fxgmac_ring));
	rx_ring = kcalloc(pdata->rx_ring_count, sizeof(struct fxgmac_ring),
			  GFP_KERNEL);
	if (!rx_ring)
		goto err_rx_ring;

	if (netif_msg_drv(pdata))
		DPRINTK("alloc_channels, rx_ring=%p, size=%d*%ld\n", rx_ring,
			pdata->rx_ring_count, sizeof(struct fxgmac_ring));

	for (i = 0, channel = channel_head; i < pdata->channel_count;
	     i++, channel++) {
		snprintf(channel->name, sizeof(channel->name), "channel-%u", i);
		channel->pdata = pdata;
		channel->queue_index = i;
		channel->dma_regs =
			pdata->mac_regs + DMA_CH_BASE + (DMA_CH_INC * i);

		if (pdata->per_channel_irq) {
			/* Get the per DMA interrupt */
#ifdef CONFIG_PCI_MSI
			if (msix) {
				pdata->channel_irq[i] =
					pdata->expansion.msix_entries[i].vector;
				if (FXGMAC_IS_CHANNEL_WITH_TX_IRQ(i)) {
					pdata->channel_irq
						[FXGMAC_MAX_DMA_CHANNELS] =
						pdata->expansion
							.msix_entries
								[FXGMAC_MAX_DMA_CHANNELS]
							.vector;

					if (pdata->channel_irq
						    [FXGMAC_MAX_DMA_CHANNELS] <
					    0) {
						netdev_err(
							pdata->netdev,
							"get_irq %u for tx failed\n",
							i + 1);
						goto err_irq;
					}

					channel->expansion.dma_irq_tx =
						pdata->channel_irq
							[FXGMAC_MAX_DMA_CHANNELS];
					DPRINTK("fxgmac_alloc_channels, for MSIx, channel %d dma_irq_tx=%u\n",
						i,
						channel->expansion.dma_irq_tx);
				}
			}
#endif
			ret = pdata->channel_irq[i];
			if (ret < 0) {
				netdev_err(pdata->netdev, "get_irq %u failed\n",
					   i + 1);
				goto err_irq;
			}
			channel->dma_irq = ret;
			DPRINTK("fxgmac_alloc_channels, for MSIx, channel %d dma_irq=%u\n",
				i, channel->dma_irq);
		}

		if (i < pdata->tx_ring_count)
			channel->tx_ring = tx_ring++;

		if (i < pdata->rx_ring_count)
			channel->rx_ring = rx_ring++;

		netif_dbg(pdata, drv, pdata->netdev,
			  "%s: dma_regs=%p, tx_ring=%p, rx_ring=%p\n",
			  channel->name, channel->dma_regs, channel->tx_ring,
			  channel->rx_ring);
	}

	pdata->channel_head = channel_head;

	if (netif_msg_drv(pdata))
		DPRINTK("alloc_channels callout ok\n");
	return 0;

err_irq:
	kfree(rx_ring);

err_rx_ring:
	kfree(tx_ring);

err_tx_ring:
	kfree(channel_head);

	DPRINTK("fxgmac alloc_channels callout err,%d\n", ret);
	return ret;
}

static void fxgmac_free_channels_and_rings(struct fxgmac_pdata *pdata)
{
	fxgmac_free_rings(pdata);

	fxgmac_free_channels(pdata);
}

static int fxgmac_alloc_channels_and_rings(struct fxgmac_pdata *pdata)
{
	int ret;

	ret = fxgmac_alloc_channels(pdata);
	if (ret)
		goto err_alloc;

	ret = fxgmac_alloc_rings(pdata);
	if (ret)
		goto err_alloc;

	return 0;

err_alloc:
	fxgmac_free_channels_and_rings(pdata);

	return ret;
}

static int fxgmac_map_rx_buffer(struct fxgmac_pdata *pdata,
				struct fxgmac_ring *ring,
				struct fxgmac_desc_data *desc_data)
{
	struct sk_buff *skb;
	skb = __netdev_alloc_skb_ip_align(pdata->netdev, pdata->rx_buf_size,
					  GFP_ATOMIC);
	if (!skb) {
		netdev_err(pdata->netdev, "%s: Rx init fails; skb is NULL\n",
			   __func__);
		return -ENOMEM;
	}

	desc_data->skb = skb;
	desc_data->rx.buf.dma_base = dma_map_single(
		pdata->dev, skb->data, pdata->rx_buf_size, DMA_FROM_DEVICE);
	if (dma_mapping_error(pdata->dev, desc_data->rx.buf.dma_base)) {
		netdev_err(pdata->netdev, "%s: DMA mapping error\n", __func__);
		dev_kfree_skb_any(skb);
		return -EINVAL;
	}

	return 0;
}

static void fxgmac_tx_desc_init(struct fxgmac_pdata *pdata)
{
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
	struct fxgmac_desc_data *desc_data;
	struct fxgmac_dma_desc *dma_desc;
	struct fxgmac_channel *channel;
	struct fxgmac_ring *ring;
	dma_addr_t dma_desc_addr;
	unsigned int i, j;

	channel = pdata->channel_head;
	for (i = 0; i < pdata->channel_count; i++, channel++) {
		ring = channel->tx_ring;
		if (!ring)
			break;

		/* reset the tx timer status. 20220104 */
		channel->tx_timer_active = 0;

		dma_desc = ring->dma_desc_head;
		dma_desc_addr = ring->dma_desc_head_addr;

		for (j = 0; j < ring->dma_desc_count; j++) {
			desc_data = FXGMAC_GET_DESC_DATA(ring, j);

			desc_data->dma_desc = dma_desc;
			desc_data->dma_desc_addr = dma_desc_addr;

			dma_desc++;
			dma_desc_addr += sizeof(struct fxgmac_dma_desc);
		}

		ring->cur = 0;
		ring->dirty = 0;
		memset(&ring->tx, 0, sizeof(ring->tx));

		hw_ops->tx_desc_init(channel);
	}
}

static void fxgmac_rx_desc_init(struct fxgmac_pdata *pdata)
{
	struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
	struct fxgmac_desc_data *desc_data;
	struct fxgmac_dma_desc *dma_desc;
	struct fxgmac_channel *channel;
	struct fxgmac_ring *ring;
	dma_addr_t dma_desc_addr;
	unsigned int i, j;

	channel = pdata->channel_head;
	for (i = 0; i < pdata->channel_count; i++, channel++) {
		ring = channel->rx_ring;
		if (!ring)
			break;

		dma_desc = ring->dma_desc_head;
		dma_desc_addr = ring->dma_desc_head_addr;

		for (j = 0; j < ring->dma_desc_count; j++) {
			desc_data = FXGMAC_GET_DESC_DATA(ring, j);

			desc_data->dma_desc = dma_desc;
			desc_data->dma_desc_addr = dma_desc_addr;

			if (fxgmac_map_rx_buffer(pdata, ring, desc_data))
				break;

			dma_desc++;
			dma_desc_addr += sizeof(struct fxgmac_dma_desc);
		}

		ring->cur = 0;
		ring->dirty = 0;

		hw_ops->rx_desc_init(channel);
	}
}

static int fxgmac_map_tx_skb(struct fxgmac_channel *channel,
			     struct sk_buff *skb)
{
	struct fxgmac_pdata *pdata = channel->pdata;
	struct fxgmac_ring *ring = channel->tx_ring;
	unsigned int start_index, cur_index;
	struct fxgmac_desc_data *desc_data;
	unsigned int offset, datalen, len;
	struct fxgmac_pkt_info *pkt_info;
	skb_frag_t *frag;
	unsigned int tso, vlan;
	dma_addr_t skb_dma;
	unsigned int i;

	offset = 0;
	start_index = ring->cur;
	cur_index = ring->cur;

	pkt_info = &ring->pkt_info;
	pkt_info->desc_count = 0;
	pkt_info->length = 0;

	tso = FXGMAC_GET_REG_BITS(pkt_info->attributes,
				  TX_PACKET_ATTRIBUTES_TSO_ENABLE_POS,
				  TX_PACKET_ATTRIBUTES_TSO_ENABLE_LEN);
	vlan = FXGMAC_GET_REG_BITS(pkt_info->attributes,
				   TX_PACKET_ATTRIBUTES_VLAN_CTAG_POS,
				   TX_PACKET_ATTRIBUTES_VLAN_CTAG_LEN);

	/* Save space for a context descriptor if needed */
	if ((tso && (pkt_info->mss != ring->tx.cur_mss)) ||
	    (vlan && (pkt_info->vlan_ctag != ring->tx.cur_vlan_ctag))) {
		cur_index = FXGMAC_GET_ENTRY(cur_index, ring->dma_desc_count);
	}
	desc_data = FXGMAC_GET_DESC_DATA(ring, cur_index);

	if (tso) {
		/* Map the TSO header */
		skb_dma = dma_map_single(pdata->dev, skb->data,
					 pkt_info->header_len, DMA_TO_DEVICE);
		if (dma_mapping_error(pdata->dev, skb_dma)) {
			netdev_alert(pdata->netdev, "dma_map_single failed\n");
			goto err_out;
		}
		desc_data->skb_dma = skb_dma;
		desc_data->skb_dma_len = pkt_info->header_len;
		netif_dbg(pdata, tx_queued, pdata->netdev,
			  "skb header: index=%u, dma=%pad, len=%u\n", cur_index,
			  &skb_dma, pkt_info->header_len);

		offset = pkt_info->header_len;

		pkt_info->length += pkt_info->header_len;

		cur_index = FXGMAC_GET_ENTRY(cur_index, ring->dma_desc_count);
		desc_data = FXGMAC_GET_DESC_DATA(ring, cur_index);
	}

	/* Map the (remainder of the) packet */
	for (datalen = skb_headlen(skb) - offset; datalen;) {
		len = min_t(unsigned int, datalen, FXGMAC_TX_MAX_BUF_SIZE);

		skb_dma = dma_map_single(pdata->dev, skb->data + offset, len,
					 DMA_TO_DEVICE);
		if (dma_mapping_error(pdata->dev, skb_dma)) {
			netdev_alert(pdata->netdev, "dma_map_single failed\n");
			goto err_out;
		}
		desc_data->skb_dma = skb_dma;
		desc_data->skb_dma_len = len;
		netif_dbg(pdata, tx_queued, pdata->netdev,
			  "skb data: index=%u, dma=%pad, len=%u\n", cur_index,
			  &skb_dma, len);

		datalen -= len;
		offset += len;

		pkt_info->length += len;

		cur_index = FXGMAC_GET_ENTRY(cur_index, ring->dma_desc_count);
		desc_data = FXGMAC_GET_DESC_DATA(ring, cur_index);
	}

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		netif_dbg(pdata, tx_queued, pdata->netdev, "mapping frag %u\n",
			  i);
		frag = &skb_shinfo(skb)->frags[i];
		offset = 0;

		for (datalen = skb_frag_size(frag); datalen;) {
			len = min_t(unsigned int, datalen,
				    FXGMAC_TX_MAX_BUF_SIZE);

			skb_dma = skb_frag_dma_map(pdata->dev, frag, offset,
						   len, DMA_TO_DEVICE);

			if (dma_mapping_error(pdata->dev, skb_dma)) {
				netdev_alert(pdata->netdev,
					     "skb_frag_dma_map failed\n");
				goto err_out;
			}
			desc_data->skb_dma = skb_dma;
			desc_data->skb_dma_len = len;
			desc_data->mapped_as_page = 1;
			netif_dbg(pdata, tx_queued, pdata->netdev,
				  "skb frag: index=%u, dma=%pad, len=%u\n",
				  cur_index, &skb_dma, len);

			datalen -= len;
			offset += len;

			pkt_info->length += len;

			cur_index = FXGMAC_GET_ENTRY(cur_index,
						     ring->dma_desc_count);
			desc_data = FXGMAC_GET_DESC_DATA(ring, cur_index);
		}
	}

	/* Save the skb address in the last entry. We always have some data
	 * that has been mapped so desc_data is always advanced past the last
	 * piece of mapped data - use the entry pointed to by cur_index - 1.
	 */
	desc_data = FXGMAC_GET_DESC_DATA(
		ring, (cur_index - 1) & (ring->dma_desc_count - 1));
	desc_data->skb = skb;

	/* Save the number of descriptor entries used */
	if (start_index <= cur_index)
		pkt_info->desc_count = cur_index - start_index;
	else
		pkt_info->desc_count =
			ring->dma_desc_count - start_index + cur_index;

	return pkt_info->desc_count;

err_out:
	while (start_index < cur_index) {
		desc_data = FXGMAC_GET_DESC_DATA(ring, start_index);
		start_index =
			FXGMAC_GET_ENTRY(start_index, ring->dma_desc_count);
		fxgmac_unmap_desc_data(pdata, desc_data);
	}

	return 0;
}

void fxgmac_init_desc_ops(struct fxgmac_desc_ops *desc_ops)
{
	desc_ops->alloc_channles_and_rings = fxgmac_alloc_channels_and_rings;
	desc_ops->free_channels_and_rings = fxgmac_free_channels_and_rings;
	desc_ops->map_tx_skb = fxgmac_map_tx_skb;
	desc_ops->map_rx_buffer = fxgmac_map_rx_buffer;
	desc_ops->unmap_desc_data = fxgmac_unmap_desc_data;
	desc_ops->tx_desc_init = fxgmac_tx_desc_init;
	desc_ops->rx_desc_init = fxgmac_rx_desc_init;
}
