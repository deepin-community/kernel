// SPDX-License-Identifier: GPL-2.0
/**
 * Virtual network driver based on the pcie interface in EP
 *
 * Copyright (C) 2023 Phytium Corporation
 * Author: netgroup@phytium.com.cn
 */

#include <linux/crc32.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/random.h>

#include <linux/pci-epc.h>
#include <linux/pci-epf.h>
#include <linux/pci_regs.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
#include <net/ip6_checksum.h>
#include <linux/dma-mapping.h>
#include <linux/prefetch.h>
#include <linux/skbuff.h>
#include <linux/circ_buf.h>
#include <linux/ioctl.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include "pci_epf_vnet.h"

char dev_addr[6] = {0x04, 0x05, 0x06, 0x00, 0x00, 0x16};

static int debug = 3;
module_param(debug, int, 0);

static struct cdev_mdata epf_cdev_mdata;
static struct epf_ioctl_data ioctl_data;

static struct sk_buff *ep_build_skb(struct ep_rx_buffer *rx_buffer,
				    unsigned int size)
{
	struct sk_buff *skb;
	unsigned int truesize;
	void *va;

#if (PAGE_SIZE < 8192)
	truesize = EP_RX_PAGE_SIZE / 2;
#else
	truesize = SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
				  SKB_DATA_ALIGN(EP_SKB_PAD + size);
#endif

	va = page_address(rx_buffer->page) + rx_buffer->page_offset;
	/* prefetch first cache line of first page */
	prefetch(va);

	/* build an skb around the page buffer */
	skb = build_skb(va - EP_SKB_PAD, truesize);
	if (unlikely(!skb))
		return NULL;

	/* update pointers within the skb to store the data */
	skb_reserve(skb, EP_SKB_PAD);
	__skb_put(skb, size);

	/* update buffer offset */
#if (PAGE_SIZE < 8192)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif

	return skb;
}

static inline int mul32(int size)
{
	int ret = 0;

	if (size % 32)
		ret = size  + (32 - size % 32);
	else
		ret = size;

	return ret;
}

static inline struct device *vnet_to_dev(struct pci_epf_vnet *vnet)
{
	return &vnet->epf->dev;
}

static inline unsigned int vnet_tx_ring_wrap(struct pci_epf_vnet *vnet,
					     unsigned int index)
{
	return index & (vnet->tx_ring_size - 1);
}

static inline struct pci_ep_dma_desc *vnet_tx_desc(struct pci_epf_vnet *vnet,
						   unsigned int index)
{
	struct pci_ep_queue *queue = vnet->queue;
	struct ep_queue *tx_queue = &queue->ep_tx_queue;
	/* Obtain the index of descriptors */
	index = vnet_tx_ring_wrap(vnet, index);
	return &tx_queue->dma_desc_base[index];
}

static inline unsigned int vnet_rx_ring_wrap(struct pci_epf_vnet *vnet,
					     unsigned int index)
{
	return index & (vnet->rx_ring_size - 1);
}

static inline struct pci_ep_dma_desc *vnet_rx_desc(struct pci_epf_vnet *vnet,
						   unsigned int index)
{
	struct pci_ep_queue *queue = vnet->queue;
	struct ep_queue *rx_queue = &queue->ep_rx_queue;
	/* Obtain the index of descriptors */
	index = vnet_rx_ring_wrap(vnet, index);
	return &rx_queue->dma_desc_base[index];
}

static inline int pci_epf_vnet_ep2rc_dma(struct pci_epf_vnet *vnet,
					 dma_addr_t buffer_dma_addr,
					 u64 addr, u32 data_size,
					 enum dma_data_direction dir)
{
	int ret;
	struct pci_epc *epc = vnet->epf->epc;

	dma_sync_single_for_device(epc->dev.parent, buffer_dma_addr, data_size,
				   dir);

	ret = pci_epc_start_dma(epc, vnet->epf->func_no, buffer_dma_addr, addr,
				data_size, DMA_WRITE);
	if (ret) {
		dev_err(&epc->dev, "ep2rc pci_epc_start_dma fail! ret %d\n",
			ret);
		return -EIO;
	}

	return 0;
}

static inline int pci_epf_vnet_rc2ep_dma(struct pci_epf_vnet *vnet,
					 dma_addr_t buffer_dma_addr,
					 u64 addr, u32 data_size)
{
	int ret;
	struct pci_epc *epc = vnet->epf->epc;

	ret = pci_epc_start_dma(epc, vnet->epf->func_no, buffer_dma_addr, addr,
				data_size, DMA_READ);
	if (ret) {
		dev_err(&epc->dev, "rc2ep pci_epc_start_dma fail! ret %d\n",
			ret);
		return -EIO;
	}

	return 0;
}

static inline bool vnet_tx_desc_queue_full(struct pci_epf_vnet *vnet)
{
	u32 tail, head;
	struct pci_ep_queue *queue = vnet->queue;
	struct ep_queue *tx_queue = &queue->ep_tx_queue;

	head = READ_ONCE(tx_queue->head);
	tail = READ_ONCE(tx_queue->tail);

	if (head == (tail + 1) % vnet->tx_ring_size)
		return true;
	else
		return false;
}

static inline int vnet_get_tx_desc_tail_idx(struct pci_epf_vnet *vnet)
{
	u32 tail;
	struct pci_ep_queue *queue = vnet->queue;
	struct ep_queue *tx_queue = &queue->ep_tx_queue;

	tail = READ_ONCE(tx_queue->tail);
	return tail;
}

static inline int vnet_adj_tx_desc_tail_idx(struct pci_epf_vnet *vnet)
{
	struct pci_ep_queue *queue = vnet->queue;
	struct ep_queue *tx_queue = &queue->ep_tx_queue;
	u32 tail = READ_ONCE(tx_queue->tail);

	/* Move tail pointer */
	tail = (tail + 1) % vnet->tx_ring_size;
	WRITE_ONCE(tx_queue->tail, tail);

	return 0;
}

static bool ep_alloc_mapped_page(struct pci_epf_vnet *vnet,
				 struct ep_rx_buffer *rx_buffer_info)
{
	dma_addr_t paddr;
	struct pci_epf *epf = vnet->epf;
	struct pci_epc *epc = epf->epc;
	struct page *page = rx_buffer_info->page;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page))
		return true;

	page = __dev_alloc_pages(EP_GFP_FLAGS, EP_RX_PAGE_ORDER);
	if (unlikely(!page)) {
		netdev_err(vnet->netdev, "rx alloc page failed\n");
		rx_buffer_info->page = NULL;
		return false;
	}

	paddr = dma_map_page_attrs(epc->dev.parent, page, 0,
				   EP_RX_PAGE_SIZE,
				   DMA_FROM_DEVICE, EP_DMA_ATTR);
	if (dma_mapping_error(epc->dev.parent, paddr)) {
		__free_pages(page, EP_RX_PAGE_ORDER);
		rx_buffer_info->page = NULL;
		return false;
	}

	rx_buffer_info->addr = paddr;
	rx_buffer_info->page = page;
	rx_buffer_info->pagecnt_bias = 1;
	rx_buffer_info->page_offset = EP_SKB_PAD;

	return true;
}

static void vnet_rx_refill(struct pci_epf_vnet *vnet, bool init)
{
	struct pci_epf *epf = vnet->epf;
	struct pci_epc *epc = epf->epc;
	unsigned int entry, space;
	struct ep_rx_buffer *rx_buffer_info;

	if (init)
		space = vnet->rx_ring_size - 1;
	else
		space = CIRC_SPACE(vnet->rx_refill_start,
				   READ_ONCE(vnet->queue->ep_rx_queue.head),
				   vnet->rx_ring_size);
	while (space > 0) {
		entry = vnet_rx_ring_wrap(vnet, vnet->rx_refill_start);

		rx_buffer_info = &vnet->rx_buffer_info[entry];
		if (!ep_alloc_mapped_page(vnet, rx_buffer_info))
			break;
		/* sync the buffer for use by the device */
		dma_sync_single_range_for_device(epc->dev.parent,
						 rx_buffer_info->addr,
						 rx_buffer_info->page_offset,
						 vnet->rx_buffer_len,
						 DMA_FROM_DEVICE);
		vnet->rx_refill_start = (vnet->rx_refill_start + 1) %
					 vnet->rx_ring_size;
		space--;
	}

	vnet->rx_next_to_alloc = vnet->rx_refill_start;
}

static int vnet_tx_fill(struct pci_epf_vnet *vnet, u32 pktnum)
{
	struct page *page;
	dma_addr_t paddr;
	struct pci_epf *epf = vnet->epf;
	struct pci_epc *epc = epf->epc;
	int count;
	void *buffer_virt;

	for (count = 0; count < pktnum; count++) {
		page = __dev_alloc_pages(EP_GFP_FLAGS, EP_TX_PAGE_ORDER);
		if (unlikely(!page)) {
			netdev_err(vnet->netdev, "tx alloc page failed\n");
			break;
		}

		buffer_virt = page_to_virt(page);

		paddr = dma_map_page_attrs(epc->dev.parent, page, 0,
					   EP_TX_PAGE_SIZE,
					   DMA_FROM_DEVICE, EP_DMA_ATTR);
		if (dma_mapping_error(epc->dev.parent, paddr)) {
			netdev_err(vnet->netdev, "tx map page failed\n");
			break;
		}

		vnet->tx_buffer_info[count].page = page;
		vnet->tx_buffer_info[count].addr = paddr;
		vnet->tx_buffer_info[count].vaddr = buffer_virt;
	}

	return count;
}

static void vnet_free_rx_skb_ring(struct pci_epf_vnet *vnet, int count)
{
	int i;
	struct pci_epf *epf = vnet->epf;
	struct pci_epc *epc = epf->epc;
	struct ep_rx_buffer *rx_buffer_info;

	for (i = 0; i < count; i++) {
		rx_buffer_info = &vnet->rx_buffer_info[i];
		if (!rx_buffer_info->page)
			continue;

		dma_unmap_page_attrs(epc->dev.parent, rx_buffer_info->addr,
				     EP_RX_PAGE_SIZE, DMA_FROM_DEVICE, EP_DMA_ATTR);

		__page_frag_cache_drain(rx_buffer_info->page,
					rx_buffer_info->pagecnt_bias);
	}

	vfree(vnet->rx_buffer_info);
	vnet->rx_buffer_info = NULL;
}

static inline int ep_calc_rx_buf_len(void)
{
#if (PAGE_SIZE < 8192)
	return rounddown(EP_MAX_FRAME_BUILD_SKB, RX_BUFFER_MULTIPLE);
#endif
	return rounddown(EP_RXBUFFER_2048, RX_BUFFER_MULTIPLE);
}

static int vnet_alloc_rx_skb_ring(struct pci_epf_vnet *vnet)
{
	int size;

	vnet->rx_buffer_len = ep_calc_rx_buf_len();

	/* Allocate pointer array for rx buffers */
	size = vnet->rx_ring_size * sizeof(struct ep_rx_buffer);
	vnet->rx_buffer_info = vzalloc(size);
	if (!vnet->rx_buffer_info) {
		netdev_err(vnet->netdev,
			   "%s:Unable to allocate buffer memory\n", __func__);
		return -ENOMEM;
	}

	netdev_dbg(vnet->netdev,
		   "Allocated %d RX struct buffer entries at %p\n",
		   vnet->rx_ring_size, vnet->rx_skb);

	/* Pre allocated rx skb */
	vnet_rx_refill(vnet, true);

	return 0;
}

static void vnet_free_tx_skb_ring(struct pci_epf_vnet *vnet, int count)
{
	int i;
	struct pci_epf *epf = vnet->epf;
	struct pci_epc *epc = epf->epc;
	struct ep_tx_buffer *tx_buffer_info;

	for (i = 0; i < count; i++) {
		tx_buffer_info = &vnet->tx_buffer_info[i];
		if (!tx_buffer_info->page)
			continue;

		dma_unmap_page_attrs(epc->dev.parent, tx_buffer_info->addr,
				     EP_RX_PAGE_SIZE, DMA_FROM_DEVICE, EP_DMA_ATTR);

		__page_frag_cache_drain(tx_buffer_info->page,
					tx_buffer_info->pagecnt_bias);
	}

	vfree(vnet->tx_buffer_info);
	vnet->tx_buffer_info = NULL;
}

static int vnet_alloc_tx_skb_ring(struct pci_epf_vnet *vnet)
{
	int size;
	int count;

	size = vnet->tx_ring_size * sizeof(struct ep_tx_buffer);
	vnet->tx_buffer_info = vzalloc(size);
	if (!vnet->tx_buffer_info) {
		netdev_err(vnet->netdev,
			   "%s:Unable to allocate buffer memory\n", __func__);
		return -ENOMEM;
	}

	netdev_dbg(vnet->netdev,
		   "Allocated %d TX struct buffer entries at %p\n",
		   vnet->tx_ring_size, vnet->tx_buffer_info);

	/* Pre allocated tx buffer */
	count = vnet_tx_fill(vnet, vnet->tx_ring_size);
	if (count != vnet->tx_ring_size) {
		netdev_err(vnet->netdev,
			   "%s:There is that page allocation failed!\n",
			   __func__);
		vnet_free_tx_skb_ring(vnet, count);
		return -ENOMEM;
	}

	return 0;
}

static int vnet_down(struct net_device *netdev)
{
	struct pci_epf_vnet *vnet = netdev_priv(netdev);

	napi_disable(&vnet->rx_napi);

	netif_stop_queue(netdev);

	netif_carrier_off(netdev);

	vnet_free_rx_skb_ring(vnet, vnet->rx_ring_size);
	vnet_free_tx_skb_ring(vnet, vnet->tx_ring_size);

	return 0;
}

static int vnet_up(struct net_device *netdev)
{
	struct pci_epf_vnet *vnet = netdev_priv(netdev);
	struct ep_queue *rx_queue;
	struct ep_queue *tx_queue;
	int err;

	rx_queue = &vnet->queue->ep_rx_queue;
	tx_queue = &vnet->queue->ep_tx_queue;

	err = vnet_alloc_rx_skb_ring(vnet);
	if (err) {
		netdev_err(vnet->netdev, "%s: failed to alloc rx buffer!!\n",
			   __func__);
		return err;
	}

	err = vnet_alloc_tx_skb_ring(vnet);
	if (err) {
		netdev_err(vnet->netdev, "%s: failed to alloc tx buffer!!\n",
			   __func__);
		return err;
	}

	netdev->flags |= IFF_RUNNING | IFF_UP;

	netif_wake_queue(netdev);

	napi_enable(&vnet->rx_napi);

	return 0;
}

static void vnet_get_drvinfo(struct net_device *netdev,
			     struct ethtool_drvinfo *drvinfo)
{
	strscpy(drvinfo->driver, DRV_MODULE_NAME, sizeof(drvinfo->driver));
	strscpy(drvinfo->version, DRV_VERSION, sizeof(drvinfo->version));
}

static u32 vnet_get_msglevel(struct net_device *netdev)
{
	struct pci_epf_vnet *vnet = netdev_priv(netdev);

	return vnet->msg_enable;
}

static void vnet_set_msglevel(struct net_device *netdev, u32 value)
{
	struct pci_epf_vnet *vnet = netdev_priv(netdev);

	vnet->msg_enable = value;
}

static u32 vnet_get_link(struct net_device *netdev)
{
	struct pci_epf_vnet *vnet = netdev_priv(netdev);
	struct pci_ep_queue *queue = vnet->queue;
	struct ep_queue *tx_queue = &queue->ep_tx_queue;

	return tx_queue->flags & PCI_EP_VNET_FLAGS_BIT(RC_LINK_ACTIVE);
}

static const char vnet_gstrings_stats[][ETH_GSTRING_LEN] = {
	"rx_packets",	    "tx_packets",	   "rx_bytes",
	"tx_bytes",	    "rx_errors",	   "tx_errors",
	"rx_dropped",	    "tx_dropped",	   "multicast",
	"collisions",	    "rx_length_errors",	   "rx_over_errors",
	"rx_crc_errors",    "rx_frame_errors",	   "rx_fifo_errors",
	"rx_missed_errors", "tx_aborted_errors",   "tx_carrier_errors",
	"tx_fifo_errors",   "tx_heartbeat_errors", "tx_window_errors",
};

#define VNET_NET_STATS_LEN ARRAY_SIZE(vnet_gstrings_stats)

static int vnet_get_sset_count(struct net_device *netdev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return VNET_NET_STATS_LEN;
	default:
		return -EOPNOTSUPP;
	}
}

static void vnet_get_ethtool_stats(struct net_device *netdev,
				   struct ethtool_stats *stats, u64 *data)
{
	int i;

	for (i = 0; i < VNET_NET_STATS_LEN; i++)
		data[i] = ((unsigned long *)&netdev->stats)[i];
}

static void vnet_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
	switch (stringset) {
	case ETH_SS_STATS:
		memcpy(data, *vnet_gstrings_stats, sizeof(vnet_gstrings_stats));
		break;
	}
}

static const struct ethtool_ops vnet_ethtool_ops = {
	.get_drvinfo = vnet_get_drvinfo,
	.get_msglevel = vnet_get_msglevel,
	.set_msglevel = vnet_set_msglevel,
	.get_link = vnet_get_link,
	.get_strings = vnet_get_strings,
	.get_ethtool_stats = vnet_get_ethtool_stats,
	.get_sset_count = vnet_get_sset_count,
};

static void pci_ep_vnet_get_defaults(struct pci_epf_vnet *vnet)
{
	struct param_range rxd_size = { .min = VNET_MIN_RXD,
					.max = VNET_MAX_RXD,
					.count = VNET_DEFAULT_RXD };
	struct param_range txd_size = { .min = VNET_MIN_TXD,
					.max = VNET_MAX_TXD,
					.count = VNET_DEFAULT_TXD };

	vnet->rx_refill_start = 0;
	vnet->rx_refill_num = 0;

	/* RX buffers initialization */
	vnet->rx_buffer_size = VNET_RXBUFFER_2048;

	vnet->rx_ring_size = rxd_size.count;
	vnet->tx_ring_size = txd_size.count;

	vnet->rxd_size = rxd_size;
	vnet->txd_size = txd_size;
}

static bool ep_can_reuse_rx_page(struct ep_rx_buffer *rx_buffer)
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
#define EP_LAST_OFFSET \
	(SKB_WITH_OVERHEAD(PAGE_SIZE) - EP_RXBUFFER_2048)

	if (rx_buffer->page_offset > EP_LAST_OFFSET)
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

static void ep_reuse_rx_page(struct pci_epf_vnet *vnet,
			     struct ep_rx_buffer *old_buff)
{
	struct ep_rx_buffer *new_buff;
	u16 nta = vnet->rx_next_to_alloc;

	new_buff = &vnet->rx_buffer_info[nta & (vnet->rx_ring_size - 1)];

	/* update, and store next to alloc */
	nta++;
	vnet->rx_next_to_alloc = (nta < vnet->rx_ring_size) ? nta : 0;

	/* Transfer page from old buffer to new buffer.
	 * Move each member individually to avoid possible store
	 * forwarding stalls.
	 */
	new_buff->addr		= old_buff->addr;
	new_buff->page		= old_buff->page;
	new_buff->page_offset	= old_buff->page_offset;
	new_buff->pagecnt_bias	= old_buff->pagecnt_bias;
}

static void ep_put_rx_buffer(struct pci_epf_vnet *vnet,
			     struct ep_rx_buffer *rx_buffer_info)
{
	struct pci_epf *epf = vnet->epf;
	struct pci_epc *epc = epf->epc;

	if (ep_can_reuse_rx_page(rx_buffer_info)) {
		/* hand second half of page back to the ring */
		ep_reuse_rx_page(vnet, rx_buffer_info);
	} else {
		dma_unmap_page_attrs(epc->dev.parent, rx_buffer_info->addr,
				     EP_RX_PAGE_SIZE, DMA_FROM_DEVICE, EP_DMA_ATTR);

		__page_frag_cache_drain(rx_buffer_info->page,
					rx_buffer_info->pagecnt_bias);
	}

	/* clear contents of rx_buffer */
	rx_buffer_info->page = NULL;
}

static int pci_epf_vnet_rx(struct pci_epf_vnet *vnet, int budget)
{
	struct pci_epf *epf = vnet->epf;
	struct pci_epc *epc = epf->epc;
	struct ep_queue *rx_queue = &vnet->queue->ep_rx_queue;
	u64 src_addr, dst_addr;
	struct sk_buff *skb;
	unsigned int len;
	struct pci_ep_dma_desc *desc;
	int count;
	u32 desc_crc, pkt_crc;
	void *pkt_data;
	unsigned int entry;
	int ret = 0;
	int timeout;
	struct ep_rx_buffer *rx_buffer_info;
	u32 rx_head, rx_tail;

	for (count = 0; count < budget; count++) {
		rx_head = READ_ONCE(rx_queue->head);
		rx_tail = READ_ONCE(rx_queue->tail);
		/* Obtain descriptor index */
		entry = vnet_rx_ring_wrap(vnet, rx_head);

		/* Obtain descriptors for bar space */
		desc = vnet_rx_desc(vnet, entry);

		/* No package to receive */
		if (rx_head == rx_tail)
			break;

		/* Make hw descriptor updates visible to CPU */
		dma_rmb();
		/* Determine if it is a valid package */
		if (!(desc->ctrl & PCI_EP_DMA_DESC_CTRL_FIELD_MASK(USED)))
			break;

		rx_buffer_info = &vnet->rx_buffer_info[entry & (vnet->rx_ring_size - 1)];
		if (!rx_buffer_info->page) {
			netdev_err(vnet->netdev, "%s:The page used is null!\n", __func__);
			break;
		}

		rx_buffer_info->pagecnt_bias--;

		/* Obtain packet length from descriptor */
		len = PCI_EP_DMA_DESC_FIELD_GET(LEN, desc->ctrl);

		/* Obtain the DMA address of the package in RC from the descriptor */
		src_addr = desc->addr;
		dst_addr = rx_buffer_info->addr + rx_buffer_info->page_offset;

		spin_lock(&vnet->rx_lock);

		/* Start DMA transmission */
		ret = pci_epf_vnet_rc2ep_dma(vnet, dst_addr,
					     src_addr, mul32(len));
		if (ret) {
			spin_unlock(&vnet->rx_lock);
			netdev_err(vnet->netdev,
				   "dma transfer failure(err %d)\n", ret);
			vnet->netdev->stats.rx_dropped++;
			rx_buffer_info->pagecnt_bias++;
			ep_put_rx_buffer(vnet, rx_buffer_info);

			desc->ctrl |= PCI_EP_DMA_DESC_CTRL_PKT_ERR;
			desc->ctrl &= ~PCI_EP_DMA_DESC_CTRL_PKT_USED;
			dma_wmb();
			goto next_cycle;
		}

		timeout = (mul32(len) / DIVISOR + 1) * 10 * 8;
		while (!(pci_epc_dma_status(epc, epf->func_no, DMA_READ) &
			 DMA_STATUS_DONE)) {
			timeout -= 5;
			udelay(1);
			if (timeout <= 0) {
				spin_unlock(&vnet->rx_lock);
				netdev_err(vnet->netdev,
					   "%s:dma transfer timeout!\n",
					   __func__);
				vnet->netdev->stats.rx_dropped++;
				rx_buffer_info->pagecnt_bias++;
				ep_put_rx_buffer(vnet, rx_buffer_info);

				desc->ctrl |= PCI_EP_DMA_DESC_CTRL_PKT_ERR;
				desc->ctrl &= ~PCI_EP_DMA_DESC_CTRL_PKT_USED;
				dma_wmb();
				goto next_cycle;
			}
		}

		spin_unlock(&vnet->rx_lock);

		dma_sync_single_range_for_cpu(epc->dev.parent,
					      rx_buffer_info->addr,
					      rx_buffer_info->page_offset,
					      len, DMA_FROM_DEVICE);

		pkt_data = page_address(rx_buffer_info->page) +
					rx_buffer_info->page_offset - EP_SKB_PAD;
		pkt_crc = crc32_le(~0, (unsigned char *)pkt_data, len);
		desc_crc = PCI_EP_DMA_DESC_FIELD_GET(CHECKSUM, desc->ctrl);

		/* Compare the CRC of RC and EP */
		if (pkt_crc != desc_crc) {
			vnet->netdev->stats.rx_errors++;
			rx_buffer_info->pagecnt_bias++;
			ep_put_rx_buffer(vnet, rx_buffer_info);

			desc->ctrl |= PCI_EP_DMA_DESC_CTRL_PKT_ERR;
			desc->ctrl &= ~PCI_EP_DMA_DESC_CTRL_PKT_USED;
			dma_wmb();
			goto next_cycle;
		}

		skb = ep_build_skb(rx_buffer_info, len);
		if (unlikely(!skb)) {
			netdev_err(vnet->netdev, "rx build skb failed\n");
			vnet->netdev->stats.rx_dropped++;
			rx_buffer_info->pagecnt_bias++;
			break;
		}

		ep_put_rx_buffer(vnet, rx_buffer_info);

		/* Modify the Ctrl key in the descriptor */
		desc->ctrl &= ~PCI_EP_DMA_DESC_CTRL_PKT_USED;
		dma_wmb();

		skb->protocol = eth_type_trans(skb, vnet->netdev);
		skb->ip_summed = CHECKSUM_UNNECESSARY;

		/* Upload protocol stack */
		netif_receive_skb(skb);

		vnet->netdev->stats.rx_packets++;
		vnet->netdev->stats.rx_bytes += skb->len;

next_cycle:
		/* Move the SKB position in the ring backwards */
		rx_head = (rx_head + 1) % vnet->rx_ring_size;
		WRITE_ONCE(rx_queue->head, rx_head);
	}

	vnet_rx_refill(vnet, false);

	return count;
}

int pci_epf_vnet_clean_ep2rc_irq(struct pci_epf_vnet *vnet, int budget)
{
	int ret = 0;

	ret = pci_epf_vnet_rx(vnet, budget);
	return ret;
}

void pci_epf_vnet_set_irq(struct phytium_pcie_ep *priv, struct pci_epf_vnet *vnet)
{
	u32 bar1_lo_addr, bar1_hi_addr;
	struct pci_epf *epf = vnet->epf;

	/* Use BAR1 as the area to trigger the EP interrupt */
	bar1_lo_addr = readl(priv->reg_base +
			     PHYTIUM_PCIE_FUNC_BASE(epf->func_no) +
			     PHYTIUM_PCI_WIN0_TRSL_ADDR0(BAR_1));
	bar1_hi_addr = readl(priv->reg_base +
			     PHYTIUM_PCIE_FUNC_BASE(epf->func_no) +
			     PHYTIUM_PCI_WIN0_TRSL_ADDR1(BAR_1));

	writel(bar1_lo_addr, priv->hpb_base + PHYTIUM_HPB_REG_MSI64_LO_ADDR);
	writel(bar1_hi_addr, priv->hpb_base + PHYTIUM_HPB_REG_MSI64_HI_ADDR);

	/* Enable Interrupt */
	writel(HPB_ERR_EVT_LOG_EN, priv->hpb_base +
	       PHYTIUM_HPB_REG_ERR_EVT_LOG_EN);
	writel(HPB_ERR_EVT_INT_EN, priv->hpb_base +
	       PHYTIUM_HPB_REG_ERR_EVT_INT_EN);
	writel(HPB_MSI_EN, priv->hpb_base + PHYTIUM_HPB_REG_MSI_EN);
	writel(HPB_NS_MSI_SPI_EN, priv->hpb_base +
	       PHYTIUM_HPB_REG_NS_MSI_SPI_EN);
}

void pci_epf_vnet_enable_irq(struct phytium_pcie_ep *priv)
{
	/* Enable interrupt reporting */
	writel(HPB_ERR_EVT_INT_EN, priv->hpb_base +
	       PHYTIUM_HPB_REG_ERR_EVT_INT_EN);
}

void pci_epf_vnet_disable_irq(struct phytium_pcie_ep *priv)
{
	/* Close interrupt reporting */
	writel(HPB_ERR_EVT_INT_CLR, priv->hpb_base +
	       PHYTIUM_HPB_REG_ERR_EVT_INT_EN);
}

static int pci_epf_vnet_rx_poll(struct napi_struct *napi, int budget)
{
	int work_done;
	struct pci_epf_vnet *vnet = container_of(napi,
						 struct pci_epf_vnet,
						 rx_napi);
	struct pci_epf *epf = vnet->epf;
	struct pci_epc *epc = epf->epc;
	struct phytium_pcie_ep *priv = epc_get_drvdata(epc);

	work_done = vnet->clean_rc2ep(vnet, budget);
	if (work_done < budget) {
		napi_complete_done(napi, work_done);
		/* Enable interrupt */
		pci_epf_vnet_enable_irq(priv);
	}
	return work_done;
}

static irqreturn_t pci_epf_vnet_rx_irqhandler(int irq, void *data)
{
	struct pci_epf_vnet *vnet = (struct pci_epf_vnet *)data;
	struct pci_epf *epf = vnet->epf;
	struct pci_epc *epc = epf->epc;
	struct phytium_pcie_ep *priv = epc_get_drvdata(epc);

	if (napi_schedule_prep(&vnet->rx_napi)) {
		/* Disable interrupt */
		pci_epf_vnet_disable_irq(priv);
		__napi_schedule(&vnet->rx_napi);
	}

	readl(priv->hpb_base + PHYTIUM_HPB_REG_NS_MSI_DATA);
	return IRQ_HANDLED;
}

static netdev_tx_t vnet_start_xmit(struct sk_buff *skb,
				   struct net_device *netdev)
{
	u32 tail;
	u32 crc32;
	int timeout;
	int len;
	char *data;
	dma_addr_t src_addr, dst_addr = 0;
	int ret;
	unsigned long tx_lock_flags;
	struct pci_ep_dma_desc *desc;
	u64 ctrl = 0;
	u32 tx_flags;

	struct pci_epf_vnet *vnet = netdev_priv(netdev);
	struct pci_ep_queue *queue = vnet->queue;
	struct pci_epf *epf = vnet->epf;
	struct pci_epc *epc = epf->epc;
	struct ep_queue *tx_queue = &queue->ep_tx_queue;

	data = skb->data;
	len = skb->len;

	/* Check if RC is online */
	tx_flags = READ_ONCE(tx_queue->flags);
	if (!(tx_flags & PCI_EP_VNET_FLAGS_BIT(RC_LINK_ACTIVE)))
		return NETDEV_TX_BUSY;

	/* Calculate the CRC of the package */
	crc32 = crc32_le(~0, data, len);

	spin_lock_irqsave(&vnet->tx_lock, tx_lock_flags);

	/* Check if the bar space descriptor ring is full */
	if (vnet_tx_desc_queue_full(vnet)) {
		spin_unlock_irqrestore(&vnet->tx_lock, tx_lock_flags);
		netdev_err(vnet->netdev, "tx desc queue is full!!!\n");
		return NETDEV_TX_BUSY;
	}

	/* Obtain the tail index of the bar descriptor ring */
	tail = vnet_get_tx_desc_tail_idx(vnet);

	desc = vnet_tx_desc(vnet, tail);

	/* Obtain the DMA address of the buffer on the RC end */
	dst_addr = desc->addr;
	if (!dst_addr) {
		spin_unlock_irqrestore(&vnet->tx_lock, tx_lock_flags);
		return NETDEV_TX_BUSY;
	}

	memcpy(vnet->tx_buffer_info[tail].vaddr, data, len);

	src_addr = vnet->tx_buffer_info[tail].addr;

	ctrl |= PCI_EP_DMA_DESC_CTRL_PKT_USED |
		PCI_EP_DMA_DESC_FIELD_SET(LEN, len) |
		PCI_EP_DMA_DESC_FIELD_SET(CHECKSUM, (u64)crc32);

	/* Start DMA transmission */
	ret = pci_epf_vnet_ep2rc_dma(vnet, src_addr, dst_addr, mul32(len),
				     DMA_TO_DEVICE);
	if (ret) {
		spin_unlock_irqrestore(&vnet->tx_lock, tx_lock_flags);
		dev_kfree_skb_any(skb);
		/* Packet loss statistics */
		vnet->netdev->stats.tx_dropped++;
		netdev_err(vnet->netdev, "dma failed(%d)!\n", ret);
		return NETDEV_TX_OK;
	}

	timeout = (mul32(len) / DIVISOR + 1) * 10 * 8;
	while (!(pci_epc_dma_status(epc, epf->func_no, DMA_WRITE) &
		 DMA_STATUS_DONE)) {
		timeout -= 5;
		udelay(1);
		if (timeout <= 0) {
			netdev_err(vnet->netdev, "%s:dma transfer timeout!\n", __func__);
			spin_unlock_irqrestore(&vnet->tx_lock, tx_lock_flags);
			dev_kfree_skb_any(skb);
			vnet->netdev->stats.tx_dropped++;
			return NETDEV_TX_OK;
		}
	}

	/* Move the tail pointer of the ep2rc descriptor */
	vnet_adj_tx_desc_tail_idx(vnet);

	/* Modify the Ctrl key in the descriptor */
	desc->ctrl = ctrl;

	/* Make descriptor updates visible to device */
	dma_wmb();

	spin_unlock_irqrestore(&vnet->tx_lock, tx_lock_flags);

	vnet->netdev->stats.tx_packets++;
	vnet->netdev->stats.tx_bytes += skb->len;

	dev_kfree_skb_any(skb);

	return NETDEV_TX_OK;
}

static void pci_epf_vnet_link_detect_handle(struct timer_list *timer)
{
	struct pci_epf_vnet *vnet = container_of(timer, struct pci_epf_vnet,
						 link_detect_timer);
	struct ep_queue *tx_queue = &vnet->queue->ep_tx_queue;
	struct net_device *netdev = vnet->netdev;
	u32 tx_flags = READ_ONCE(tx_queue->flags);

	/* Link detection */
	if ((tx_flags & PCI_EP_VNET_FLAGS_BIT(RC_LINK_ACTIVE)) &&
	    !(netif_carrier_ok(netdev))) {
		netif_carrier_on(netdev);
	} else if (!(tx_flags & PCI_EP_VNET_FLAGS_BIT(RC_LINK_ACTIVE)) &&
		   (netif_carrier_ok(netdev))) {
		netif_carrier_off(netdev);
	}

	mod_timer(timer, jiffies + msecs_to_jiffies(LINK_DETECT_PERIOD));
}

static int vnet_close(struct net_device *netdev)
{
	struct pci_epf_vnet *vnet = netdev_priv(netdev);
	struct ep_queue *rx_queue;
	struct ep_queue *tx_queue;
	struct pci_epf *epf = vnet->epf;
	struct pci_epc *epc = epf->epc;
	struct phytium_pcie_ep *priv = epc_get_drvdata(epc);
	u32 rx_flags;

	rx_queue = &vnet->queue->ep_rx_queue;
	tx_queue = &vnet->queue->ep_tx_queue;

	dma_unmap_page_attrs(epc->dev.parent, epf_cdev_mdata.phys_addr,
			     MEM_MAX, DMA_FROM_DEVICE, EP_DMA_ATTR);

	pci_epf_vnet_disable_irq(priv);
	/* Set EP end offline */
	rx_flags = READ_ONCE(rx_queue->flags);
	rx_flags &= ~PCI_EP_VNET_FLAGS_BIT(EP_LINK_ACTIVE);
	WRITE_ONCE(rx_queue->flags, rx_flags);

	del_timer(&vnet->link_detect_timer);

	vnet_down(netdev);

	devm_free_irq(&priv->pdev->dev, vnet->rx_irq, vnet);

	return 0;
}

static int vnet_open(struct net_device *netdev)
{
	struct pci_epf_vnet *vnet = netdev_priv(netdev);
	struct pci_epf *epf = vnet->epf;
	struct pci_epc *epc;
	struct phytium_pcie_ep *priv;
	struct ep_queue *rx_queue;
	u32 rx_flags;
	int ret;

	/* Prevent opening network card devices before binding EPF and EPC */
	if (!epf->epc)
		return -ENXIO;
	if (!vnet->bind_success)
		return -ENXIO;

	epc = epf->epc;
	priv = epc_get_drvdata(epc);

	rx_queue = &vnet->queue->ep_rx_queue;

	/* Obtain interrupt number */
	vnet->rx_irq = platform_get_irq(priv->pdev, 1);
	if (vnet->rx_irq < 0) {
		dev_err(&priv->pdev->dev,
			"Failed to obtain interrupt from device tree!\n");
		return vnet->rx_irq;
	}

	/* Apply for interruption */
	ret = devm_request_irq(&priv->pdev->dev, vnet->rx_irq,
			       pci_epf_vnet_rx_irqhandler, IRQF_SHARED,
			       "pci_ep_vnet", vnet);
	if (ret < 0) {
		dev_err(&priv->pdev->dev,
			"The EP interruption application failed\n");
		return ret;
	}

	netif_carrier_off(netdev);

	ret = vnet_up(netdev);
	if (ret) {
		netdev_err(netdev, "pci_ep_vnet: Cannot open interface, aborting");
		return ret;
	}

	timer_setup(&vnet->link_detect_timer, pci_epf_vnet_link_detect_handle, 0);
	add_timer(&vnet->link_detect_timer);

	/* Set EP end online */
	rx_flags = READ_ONCE(rx_queue->flags);
	rx_flags |= PCI_EP_VNET_FLAGS_BIT(EP_LINK_ACTIVE);
	WRITE_ONCE(rx_queue->flags, rx_flags);

	pci_epf_vnet_set_irq(priv, vnet);

	epf_cdev_mdata.phys_addr = dma_map_page_attrs(epc->dev.parent, epf_cdev_mdata.page, 0,
						      MEM_MAX, DMA_FROM_DEVICE, EP_DMA_ATTR);

	return 0;
}

static int vnet_init(struct net_device *netdev)
{
	struct pci_epf_vnet *vnet = netdev_priv(netdev);

	dev_info(vnet_to_dev(vnet), "virtual net device initialized.\n");
	return 0;
}

static const struct net_device_ops vnet_ops = {
	.ndo_init = vnet_init,
	.ndo_open = vnet_open,
	.ndo_stop = vnet_close,
	.ndo_start_xmit = vnet_start_xmit,
};

static const struct pci_epf_device_id pci_epf_vnet_ids[] = {
	{
		.name = "pci_epf_vnet",
	},
	{},
};

static int epf_cdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long pfn = epf_cdev_mdata.phys_addr >> PAGE_SHIFT;
	unsigned long size = vma->vm_end - vma->vm_start;

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static int epf_cdev_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int epf_read_rc_mem(u64 rc_addr, u64 len)
{
	int ret;
	int timeout;
	unsigned long lock_flags;

	if (len < 0 || len > MEM_MAX)
		return -EINVAL;

	spin_lock_irqsave(&epf_cdev_mdata.cdev_vnet->rx_lock, lock_flags);

	ret = pci_epf_vnet_rc2ep_dma(epf_cdev_mdata.cdev_vnet, epf_cdev_mdata.phys_addr,
				     rc_addr, len);
	if (ret) {
		spin_unlock_irqrestore(&epf_cdev_mdata.cdev_vnet->rx_lock, lock_flags);
		netdev_err(epf_cdev_mdata.cdev_vnet->netdev,
			   "dma transfer failure(err %d)\n", ret);
		return ret;
	}

	timeout = (mul32(len) / DIVISOR + 1) * 20 * 8;
	while (!(pci_epc_dma_status(epf_cdev_mdata.cdev_vnet->epf->epc,
				    epf_cdev_mdata.cdev_vnet->epf->func_no, DMA_READ) &
				    DMA_STATUS_DONE)) {
		timeout -= 5;
		udelay(1);
		if (timeout <= 0) {
			spin_unlock_irqrestore(&epf_cdev_mdata.cdev_vnet->rx_lock, lock_flags);
			netdev_err(epf_cdev_mdata.cdev_vnet->netdev,
				   "%s:dma transfer timeout!\n",
				   __func__);
			return -ETIME;
		}
	}

	spin_unlock_irqrestore(&epf_cdev_mdata.cdev_vnet->rx_lock, lock_flags);

	return 0;
}

int measure_ddr(phys_addr_t hostddr, uint8_t *bmcaddr, u32 size)
{
	int ret = 0;
	u32 offset = hostddr & (PAGE_SIZE - 1);
	u64 new_base = hostddr - offset;
	u64 new_len = size + offset;
	u64 read_len = 0;
	u8 *ori_addr = bmcaddr;
	void *vaddr = memremap(epf_cdev_mdata.phys_addr, MEM_MAX, MEMREMAP_WB);

	while (new_len) {
		if (new_len >= MEM_MAX)
			read_len = MEM_MAX;
		else
			read_len = new_len;

		ret = epf_read_rc_mem(new_base, read_len);
		if (ret)
			return ret;

		new_len -= read_len;
		new_base += read_len;

		memcpy(ori_addr, (u8 *)vaddr + offset, read_len - offset);
		ori_addr += read_len - offset;

		if (offset)
			offset = 0;
	}

	memunmap(vaddr);

	return 0;
}
EXPORT_SYMBOL(measure_ddr);

static long epf_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case IOCTL_EPF_READ_MEM:
		if (copy_from_user(&ioctl_data, (struct epf_ioctl_data __user *)arg,
				   sizeof(struct epf_ioctl_data)))
			return -EFAULT;

		pr_debug("Received ioctl data: phy_addr=%llx, len=%d\n",
			 ioctl_data.ktext_addr, ioctl_data.ktext_len);
		return epf_read_rc_mem(ioctl_data.ktext_addr, ioctl_data.ktext_len);
	default:
		return -EINVAL;
	}
	return 0;
}

static int epf_cdev_release(struct inode *inode, struct file *file)
{
	pr_debug("epf_char_release\n");
	return 0;
}

static const struct file_operations epf_fops = {
	.owner = THIS_MODULE,
	.open = epf_cdev_open,
	.release = epf_cdev_release,
	.unlocked_ioctl = epf_cdev_ioctl,
	.mmap = epf_cdev_mmap
};

static int pci_epf_vnet_probe(struct pci_epf *epf,
			      const struct pci_epf_device_id *id)
{
	struct pci_epf_vnet *vnet;
	struct net_device *netdev;
	struct device *dev = &epf->dev;
	int err;

	netdev = devm_alloc_etherdev(dev, sizeof(*vnet));
	if (!netdev)
		return -ENOMEM;

	netdev->netdev_ops = &vnet_ops;
	netdev->ethtool_ops = &vnet_ethtool_ops;

	dev_addr_set(netdev, dev_addr);

	/* MTU range: 46 - 3982 */
	netdev->min_mtu = ETH_ZLEN - ETH_HLEN;
	netdev->max_mtu = MAX_JUMBO_FRAME_SIZE - (ETH_HLEN + ETH_FCS_LEN);

	vnet = netdev_priv(netdev);

	epf->header = &vnet_header;
	vnet->epf = epf;
	vnet->netdev = netdev;

	vnet->msg_enable = (1 << debug) - 1;
	vnet->link_detect_timer.expires = jiffies + msecs_to_jiffies(LINK_DETECT_PERIOD);
	vnet->bind_success = false;

	epf_set_drvdata(epf, vnet);

	pci_ep_vnet_get_defaults(vnet);

	spin_lock_init(&vnet->tx_lock);
	spin_lock_init(&vnet->rx_lock);

	err = register_netdev(netdev);
	if (err) {
		dev_err(&epf->dev, "Cannot register net device, aborting.\n");
		goto netdev_reg_failed;
	}

	epf_cdev_mdata.cdev_vnet = vnet;

	netif_napi_add(netdev, &vnet->rx_napi, pci_epf_vnet_rx_poll);
	vnet->clean_rc2ep = pci_epf_vnet_clean_ep2rc_irq;

	/* cdev register */
	err = alloc_chrdev_region(&epf_cdev_mdata.dev_num, 0, 1, DEVICE_NAME);
	if (err < 0)
		goto err_unregister_netdev;

	epf_cdev_mdata.epf_cdev = cdev_alloc();
	if (!epf_cdev_mdata.epf_cdev) {
		dev_err(&epf->dev, "Failed to allocate cdev\n");
		unregister_chrdev_region(epf_cdev_mdata.dev_num, 1);
		err = -ENOMEM;
		goto err_unregister_chrdev;
	}

	epf_cdev_mdata.epf_cdev->owner = THIS_MODULE;

	cdev_init(epf_cdev_mdata.epf_cdev, &epf_fops);
	if (cdev_add(epf_cdev_mdata.epf_cdev, epf_cdev_mdata.dev_num, 1) == -1)
		goto err_cdev_del;

	epf_cdev_mdata.epf_class = class_create("epf_class");
	if (IS_ERR(epf_cdev_mdata.epf_class)) {
		err = PTR_ERR(epf_cdev_mdata.epf_class);
		goto err_cdev_del;
	}

	epf_cdev_mdata.epf_device = device_create(epf_cdev_mdata.epf_class,
						  NULL, epf_cdev_mdata.dev_num,
						  NULL, DEVICE_NAME);
	if (IS_ERR(epf_cdev_mdata.epf_device)) {
		err = PTR_ERR(epf_cdev_mdata.epf_device);
		goto err_class_destroy;
	}

	epf_cdev_mdata.page = alloc_pages(GFP_KERNEL | __GFP_DMA32, EP_CDEV_PAGE_ORDER);
	if (!epf_cdev_mdata.page) {
		dev_err(&epf->dev, "failed to allocate %d pages\n", EP_CDEV_PAGE_ORDER);
		goto err_device_destroy;
	}

	return 0;

err_device_destroy:
	device_destroy(epf_cdev_mdata.epf_class, epf_cdev_mdata.dev_num);
err_class_destroy:
	class_destroy(epf_cdev_mdata.epf_class);
err_cdev_del:
	cdev_del(epf_cdev_mdata.epf_cdev);
err_unregister_chrdev:
	unregister_chrdev_region(epf_cdev_mdata.dev_num, 1);
err_unregister_netdev:
	unregister_netdev(netdev);
netdev_reg_failed:
	free_netdev(netdev);
	return err;
}

static void pci_epf_vnet_remove(struct pci_epf *epf)
{
	struct pci_epf_vnet *vnet = epf_get_drvdata(epf);
	struct net_device *netdev = vnet->netdev;

	netif_napi_del(&vnet->rx_napi);

	unregister_netdev(netdev);
	free_netdev(netdev);

	device_destroy(epf_cdev_mdata.epf_class, epf_cdev_mdata.dev_num);
	class_destroy(epf_cdev_mdata.epf_class);
	cdev_del(epf_cdev_mdata.epf_cdev);
	unregister_chrdev_region(epf_cdev_mdata.dev_num, 1);

	__free_pages(epf_cdev_mdata.page, EP_CDEV_PAGE_ORDER);
}

static int pci_epf_vnet_set_bar(struct pci_epf *epf)
{
	int ret;
	struct pci_epf_bar *epf_bar;
	struct pci_epc *epc = epf->epc;
	struct device *dev = &epf->dev;
	struct pci_epf_vnet *vnet = epf_get_drvdata(epf);
	enum pci_barno vnet_reg_barno = vnet->vnet_reg_barno;

	/* Set BAR1 */
	epf_bar = &epf->bar[BAR_1];

	ret = pci_epc_set_bar(epc, epf->func_no, epf->vfunc_no, epf_bar);
	if (ret) {
		pci_epf_free_space(epf, vnet->reg[vnet_reg_barno],
				   vnet_reg_barno, PRIMARY_INTERFACE);
		dev_err(dev, "Failed to set BAR%d\n", vnet_reg_barno);
	}

	/* Set BAR2 */
	epf_bar = &epf->bar[vnet_reg_barno];

	epf_bar->flags |= upper_32_bits(epf_bar->size) ?
					PCI_BASE_ADDRESS_MEM_TYPE_64 :
					PCI_BASE_ADDRESS_MEM_TYPE_32;

	ret = pci_epc_set_bar(epc, epf->func_no, epf->vfunc_no, epf_bar);
	if (ret) {
		pci_epf_free_space(epf, vnet->reg[vnet_reg_barno],
				   vnet_reg_barno, PRIMARY_INTERFACE);
		dev_err(dev, "Failed to set BAR%d\n", vnet_reg_barno);
	}

	return 0;
}

static int pci_epf_vnet_alloc_space(struct pci_epf *epf)
{
	struct pci_epf_vnet *vnet = epf_get_drvdata(epf);
	struct device *dev = &epf->dev;
	void *base;
	enum pci_barno vnet_reg_barno = vnet->vnet_reg_barno;
	u64 offset;
	size_t alloc_size = 64 * 1024;

	/* Allocate BAR1 space */
	base = pci_epf_alloc_space(epf, alloc_size, BAR_1,
				   0, PRIMARY_INTERFACE);
	if (!base) {
		dev_err(dev, "Failed to allocated register BAR%d space\n", BAR_1);
		return -ENOMEM;
	}
	vnet->reg[BAR_1] = base;

	/* Allocate BAR2 space */
	alloc_size = sizeof(struct pci_ep_queue) % PAGE_SIZE ?
		     (sizeof(struct pci_ep_queue) / PAGE_SIZE * 4 * PAGE_SIZE) :
		     (sizeof(struct pci_ep_queue) * 2);
	base = pci_epf_alloc_space(epf, alloc_size, vnet_reg_barno,
				   0, PRIMARY_INTERFACE);
	if (!base) {
		dev_err(dev, "Failed to allocated register BAR%d space\n",
			vnet_reg_barno);
		return -ENOMEM;
	}
	offset = epf->bar[vnet_reg_barno].phys_addr
		& (epf->bar[vnet_reg_barno].size / 2 - 1);
	if (offset)
		offset = epf->bar[vnet_reg_barno].size / 2 - offset;
	epf->bar[vnet_reg_barno].phys_addr =
		epf->bar[vnet_reg_barno].phys_addr + offset;
	vnet->reg[vnet_reg_barno] = (void *)((u64)base + offset);

	return 0;
}

static int pci_epf_vnet_bind(struct pci_epf *epf)
{
	int ret;
	struct pci_epf_vnet *vnet = epf_get_drvdata(epf);
	struct pci_epf_header *header = epf->header;
	struct pci_epc *epc = epf->epc;
	struct phytium_pcie_ep *priv = epc_get_drvdata(epc);
	struct device *dev = &epf->dev;
	struct pci_ep_queue *queue;
	struct ep_queue	*rx_queue;
	struct ep_queue *tx_queue;

	if (WARN_ON_ONCE(!epc))
		return -EINVAL;

	if (!vnet)
		return -EINVAL;

	vnet->vnet_reg_barno = BAR_2;

	ret = pci_epc_write_header(epc, epf->func_no, epf->vfunc_no, header);
	if (ret) {
		dev_err(dev, "Configuration header write failed\n");
		return ret;
	}

	ret = pci_epf_vnet_alloc_space(epf);
	if (ret)
		return ret;

	ret = pci_epf_vnet_set_bar(epf);
	if (ret)
		return ret;

	queue = vnet->reg[vnet->vnet_reg_barno];
	vnet->queue = queue;
	rx_queue = &queue->ep_rx_queue;
	tx_queue = &queue->ep_tx_queue;

	/* Initialize variables in the bar space */
	WRITE_ONCE(rx_queue->head, 0);
	WRITE_ONCE(rx_queue->tail, 0);
	WRITE_ONCE(rx_queue->flags, 0);
	rx_queue->nb_desc = vnet->rx_ring_size;

	WRITE_ONCE(tx_queue->head, 0);
	WRITE_ONCE(tx_queue->tail, 0);
	WRITE_ONCE(tx_queue->flags, 0);
	tx_queue->nb_desc = vnet->tx_ring_size;
	vnet->bind_success = true;

	/* Used to apply for interruption */
	if (!priv->pdev) {
		dev_err(dev, "Failed to obtain pdev!\n");
		return -ENODEV;
	}

	return 0;
}

static void pci_epf_vnet_unbind(struct pci_epf *epf)
{
	struct pci_epf_vnet *vnet = epf_get_drvdata(epf);
	struct pci_epc *epc = epf->epc;
	struct pci_epf_bar *epf_bar;
	int bar = vnet->vnet_reg_barno;

	pci_epc_stop(epc);
	epf_bar = &epf->bar[bar];

	if (vnet->reg[bar]) {
		pci_epf_free_space(epf, vnet->reg[bar], bar, PRIMARY_INTERFACE);
		pci_epc_clear_bar(epc, epf->func_no, epf->vfunc_no, epf_bar);
	}

	bar = BAR_1;
	epf_bar = &epf->bar[bar];

	if (vnet->reg[bar]) {
		pci_epf_free_space(epf, vnet->reg[bar], bar, PRIMARY_INTERFACE);
		pci_epc_clear_bar(epc, epf->func_no, epf->vfunc_no, epf_bar);
	}
}

static struct pci_epf_ops vnet_epf_ops = {
	.unbind = pci_epf_vnet_unbind,
	.bind = pci_epf_vnet_bind,
};

static struct pci_epf_driver pci_epf_vnet = {
	.driver.name = "pci_epf_vnet",
	.probe = pci_epf_vnet_probe,
	.remove = pci_epf_vnet_remove,
	.id_table = pci_epf_vnet_ids,
	.ops = &vnet_epf_ops,
	.owner = THIS_MODULE,
};

static int __init pci_epf_vnet_init(void)
{
	int ret;

	ret = pci_epf_register_driver(&pci_epf_vnet);
	if (ret) {
		pr_err("Failed to register pci epf test driver --> %d\n", ret);
		return ret;
	}

	return 0;
}
module_init(pci_epf_vnet_init);

static void __exit pci_epf_vnet_exit(void)
{
	pci_epf_unregister_driver(&pci_epf_vnet);
}
module_exit(pci_epf_vnet_exit);

MODULE_DESCRIPTION("Virtual network driver based on the pcie interface in EP");
MODULE_AUTHOR("litongfeng1497@phytium.com.cn");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
