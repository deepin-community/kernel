// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/in.h>
#include <linux/interrupt.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/sctp.h>
#include <linux/pkt_sched.h>
#include <linux/ipv6.h>
#include <linux/slab.h>
#include <net/checksum.h>
#include <net/ip6_checksum.h>
#include <linux/ethtool.h>
#include <linux/if.h>
#include <linux/if_vlan.h>
#include <linux/prefetch.h>
#include <net/xdp_sock_drv.h>
#ifdef CONFIG_NET_NCSI
#include <net/ncsi.h>
#endif

#include "rnpgbevf.h"

char rnpgbevf_driver_name[] = "rnpgbevf";
static const char rnpgbevf_driver_string[] =
	"Mucse(R) 1 Gigabit PCI Express Virtual Function Network Driver";

#define DRV_VERSION "0.2.3"
const char rnpgbevf_driver_version[] = DRV_VERSION;
static const char rnpgbevf_copyright[] =
	"Copyright (c) 2020 - 2024 Mucse Corporation.";

static const struct rnpgbevf_info *rnpgbevf_info_tbl[] = {
	[board_n500] = &rnp_n500_vf_info,
	[board_n210] = &rnp_n210_vf_info,
};

#define N500_BOARD board_n500
#define N210_BOARD board_n210

#ifdef CONFIG_NET_NCSI
static unsigned int be_ncsi_mc;
module_param(be_ncsi_mc, uint, 0000);
#endif

static unsigned int fix_eth_name;
module_param(fix_eth_name, uint, 0000);
MODULE_PARM_DESC(fix_eth_name, "set eth adapter name to rnpvfXX");
static struct pci_device_id rnpgbevf_pci_tbl[] = {
	{ PCI_DEVICE(0x8848, 0x8309), .driver_data = N500_BOARD },
	{ PCI_DEVICE(0x8848, 0x8209), .driver_data = N210_BOARD },
	/* required last entry */
	{
		0,
	},
};

MODULE_DEVICE_TABLE(pci, rnpgbevf_pci_tbl);
MODULE_AUTHOR("Mucse Corporation, <mucse@mucse.com>");
MODULE_DESCRIPTION("Mucse(R) N500 Virtual Function Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

#define DEFAULT_MSG_ENABLE (NETIF_MSG_DRV | NETIF_MSG_PROBE | NETIF_MSG_LINK)
static int debug = -1;
module_param(debug, int, 0000);
MODULE_PARM_DESC(debug, "Debug level (0=none,...,16=all)");

static int pci_using_hi_dma;

/* forward decls */
static void rnpgbevf_free_all_rx_resources(struct rnpgbevf_adapter *adapter);

#define RNPVF_XDP_PASS 0
#define RNPVF_XDP_CONSUMED 1
#define RNPVF_XDP_TX 2

static void rnpgbevf_pull_tail(struct sk_buff *skb);
#ifdef OPTM_WITH_LPAGE
static bool rnpgbevf_alloc_mapped_page(struct rnpgbevf_ring *rx_ring,
				       struct rnpgbevf_rx_buffer *bi,
				       union rnp_rx_desc *rx_desc, u16 bufsz,
				       u64 fun_id);
static void rnpgbevf_put_rx_buffer(struct rnpgbevf_ring *rx_ring,
				   struct rnpgbevf_rx_buffer *rx_buffer);
#else /* OPTM_WITH_LPAGE */
static bool rnpgbevf_alloc_mapped_page(struct rnpgbevf_ring *rx_ring,
				       struct rnpgbevf_rx_buffer *bi);
static void rnpgbevf_put_rx_buffer(struct rnpgbevf_ring *rx_ring,
				   struct rnpgbevf_rx_buffer *rx_buffer,
				   struct sk_buff *skb);
#endif /* OPTM_WITH_LPAGE */

/**
 * rnpgbevf_set_ivar - set IVAR registers - maps interrupt causes to vectors
 * @adapter: pointer to adapter struct
 * @direction: 0 for Rx, 1 for Tx, -1 for other causes
 * @queue: queue to map the corresponding interrupt to
 * @msix_vector: the vector to map to the corresponding queue
 */
static void rnpgbevf_set_ring_vector(struct rnpgbevf_adapter *adapter,
				     u8 rnpgbevf_queue, u8 rnpgbevf_msix_vector)
{
	struct rnpgbevf_hw *hw = &adapter->hw;
	u32 data = 0;

	data = hw->vfnum << 24;
	data |= (rnpgbevf_msix_vector << 8);
	data |= (rnpgbevf_msix_vector << 0);
	DPRINTK(IFUP, INFO,
		"Set Ring-Vector queue:%d (reg:0x%x) <-- Rx-MSIX:%d, Tx-MSIX:%d\n",
		rnpgbevf_queue, RING_VECTOR(rnpgbevf_queue),
		rnpgbevf_msix_vector, rnpgbevf_msix_vector);

	rnpgbevf_wr_reg(hw->ring_msix_base + RING_VECTOR(rnpgbevf_queue), data);
}

void rnpgbevf_unmap_and_free_tx_resource(struct rnpgbevf_ring *ring,
					 struct rnpgbevf_tx_buffer *tx_buffer)
{
	if (tx_buffer->skb) {
		dev_kfree_skb_any(tx_buffer->skb);
		if (dma_unmap_len(tx_buffer, len))
			dma_unmap_single(ring->dev,
					 dma_unmap_addr(tx_buffer, dma),
					 dma_unmap_len(tx_buffer, len),
					 DMA_TO_DEVICE);
	} else if (dma_unmap_len(tx_buffer, len)) {
		dma_unmap_page(ring->dev, dma_unmap_addr(tx_buffer, dma),
			       dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
	}
	tx_buffer->next_to_watch = NULL;
	tx_buffer->skb = NULL;
	dma_unmap_len_set(tx_buffer, len, 0);
	/* tx_buffer must be completely set up in the transmit path */
}

static void rnpgbevf_tx_timeout(struct net_device *netdev);

/**
 * rnpgbevf_clean_tx_irq - Reclaim resources after transmit completes
 * @q_vector: board private structure
 * @tx_ring: tx ring to clean
 **/
static bool rnpgbevf_clean_tx_irq(struct rnpgbevf_q_vector *q_vector,
				  struct rnpgbevf_ring *tx_ring)
{
	struct rnpgbevf_adapter *adapter = q_vector->adapter;
	struct rnpgbevf_tx_buffer *tx_buffer;
	struct rnp_tx_desc *tx_desc;
	unsigned int total_bytes = 0, total_packets = 0;
	unsigned int budget = adapter->tx_work_limit;
	unsigned int i = tx_ring->next_to_clean;

	if (test_bit(__RNPVF_DOWN, &adapter->state))
		return true;
	tx_ring->tx_stats.poll_count++;
	tx_buffer = &tx_ring->tx_buffer_info[i];
	tx_desc = RNPVF_TX_DESC(tx_ring, i);
	i -= tx_ring->count;

	do {
		struct rnp_tx_desc *eop_desc = tx_buffer->next_to_watch;

		/* if next_to_watch is not set then there is no work pending */
		if (!eop_desc)
			break;

		/* prevent any other reads prior to eop_desc */
		rmb();

		/* if eop DD is not set pending work has not been completed */
		if (!(eop_desc->cmd & cpu_to_le16(RNPGBE_TXD_STAT_DD)))
			break;

		/* clear next_to_watch to prevent false hangs */
		tx_buffer->next_to_watch = NULL;

		/* update the statistics for this packet */
		total_bytes += tx_buffer->bytecount;
		total_packets += tx_buffer->gso_segs;

		/* free the skb */
		dev_kfree_skb_any(tx_buffer->skb);

		/* unmap skb header data */
		dma_unmap_single(tx_ring->dev, dma_unmap_addr(tx_buffer, dma),
				 dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);

		/* clear tx_buffer data */
		tx_buffer->skb = NULL;
		dma_unmap_len_set(tx_buffer, len, 0);

		/* unmap remaining buffers */
		while (tx_desc != eop_desc) {
			tx_buffer++;
			tx_desc++;
			i++;
			if (unlikely(!i)) {
				i -= tx_ring->count;
				tx_buffer = tx_ring->tx_buffer_info;
				tx_desc = RNPVF_TX_DESC(tx_ring, 0);
			}

			/* unmap any remaining paged data */
			if (dma_unmap_len(tx_buffer, len)) {
				dma_unmap_page(tx_ring->dev,
					       dma_unmap_addr(tx_buffer, dma),
					       dma_unmap_len(tx_buffer, len),
					       DMA_TO_DEVICE);
				dma_unmap_len_set(tx_buffer, len, 0);
			}
		}

		/* move us one more past the eop_desc for start of next pkt */
		tx_buffer++;
		tx_desc++;
		i++;
		if (unlikely(!i)) {
			i -= tx_ring->count;
			tx_buffer = tx_ring->tx_buffer_info;
			tx_desc = RNPVF_TX_DESC(tx_ring, 0);
		}

		/* issue prefetch for next Tx descriptor */
		prefetch(tx_desc);

		/* update budget accounting */
		budget--;
	} while (likely(budget));

	i += tx_ring->count;
	tx_ring->next_to_clean = i;
	u64_stats_update_begin(&tx_ring->syncp);
	tx_ring->stats.bytes += total_bytes;
	tx_ring->stats.packets += total_packets;
	u64_stats_update_end(&tx_ring->syncp);
	q_vector->tx.total_bytes += total_bytes;
	q_vector->tx.total_packets += total_packets;

	netdev_tx_completed_queue(txring_txq(tx_ring), total_packets,
				  total_bytes);

	if (!(q_vector->vector_flags & RNPVF_QVECTOR_FLAG_REDUCE_TX_IRQ_MISS)) {
#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)
		if (unlikely(total_packets &&
			     netif_carrier_ok(tx_ring->netdev) &&
			     (rnpgbevf_desc_unused(tx_ring) >=
			      TX_WAKE_THRESHOLD))) {
			/* Make sure that anybody stopping the queue after this
			 * sees the new next_to_clean.
			 */
			smp_mb();
			if (__netif_subqueue_stopped(tx_ring->netdev,
						     tx_ring->queue_index) &&
			    !test_bit(__RNPVF_DOWN, &adapter->state)) {
				netif_wake_subqueue(tx_ring->netdev,
						    tx_ring->queue_index);
				++tx_ring->tx_stats.restart_queue;
			}
		}
	}

	return !!budget;
}

static inline void rnpgbevf_rx_hash(struct rnpgbevf_ring *ring,
				    union rnp_rx_desc *rx_desc,
				    struct sk_buff *skb)
{
	int rss_type;

	if (!(ring->netdev->features & NETIF_F_RXHASH))
		return;

#define RNPVF_RSS_TYPE_MASK 0xc0
	rss_type = rx_desc->wb.cmd & RNPVF_RSS_TYPE_MASK;
	skb_set_hash(skb, le32_to_cpu(rx_desc->wb.rss_hash),
		     rss_type ? PKT_HASH_TYPE_L4 : PKT_HASH_TYPE_L3);
}

/**
 * rnpgbevf_rx_checksum - indicate in skb if hw indicated a good cksum
 * @ring: structure containing ring specific data
 * @rx_desc: current Rx descriptor being processed
 * @skb: skb currently being received and modified
 **/
static inline void rnpgbevf_rx_checksum(struct rnpgbevf_ring *ring,
					union rnp_rx_desc *rx_desc,
					struct sk_buff *skb)
{
	bool encap_pkt = false;

	skb_checksum_none_assert(skb);

	/* Rx csum disabled */
	if (!(ring->netdev->features & NETIF_F_RXCSUM))
		return;

	/* vxlan packet handle ? */

	if (!(ring->ring_flags & RNPVF_RING_NO_TUNNEL_SUPPORT)) {
		if (rnpgbevf_get_stat(rx_desc, RNPGBE_RXD_STAT_TUNNEL_MASK) ==
		    RNPGBE_RXD_STAT_TUNNEL_VXLAN) {
			encap_pkt = true;
			skb->encapsulation = 1;
			skb->ip_summed = CHECKSUM_NONE;
		}
	}

	/* if L3/L4  error:ignore errors from veb(other vf) */
	if (unlikely(
		    rnpgbevf_test_staterr(rx_desc, RNPGBE_RXD_STAT_ERR_MASK))) {
		ring->rx_stats.csum_err++;
		return;
	}
	ring->rx_stats.csum_good++;
	/* It must be a TCP or UDP packet with a valid checksum */
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	if (encap_pkt) {
		/* If we checked the outer header let the stack know */
		skb->csum_level = 1;
	}
}

static inline void rnpgbevf_update_rx_tail(struct rnpgbevf_ring *rx_ring,
					   u32 val)
{
	rx_ring->next_to_use = val;

	/* update next to alloc since we have filled the ring */
	rx_ring->next_to_alloc = val;
	/*
	 * Force memory writes to complete before letting h/w
	 * know there are new descriptors to fetch.  (Only
	 * applicable for weak-ordered memory model archs,
	 * such as IA-64).
	 */
	wmb();
	rnpgbevf_wr_reg(rx_ring->tail, val);
}

#ifndef OPTM_WITH_LPAGE
/**
 * rnpgbevf_alloc_rx_buffers - Replace used receive buffers
 * @rx_ring: ring to place buffers on
 * @cleaned_count: number of buffers to replace
 **/
void rnpgbevf_alloc_rx_buffers(struct rnpgbevf_ring *rx_ring, u16 cleaned_count)
{
	union rnp_rx_desc *rx_desc;
	struct rnpgbevf_rx_buffer *bi;
	u16 i = rx_ring->next_to_use;
	u64 fun_id = ((u64)(rx_ring->vfnum) << (32 + 24));
	u16 bufsz;
	/* nothing to do */
	if (!cleaned_count)
		return;

	rx_desc = RNPVF_RX_DESC(rx_ring, i);
	BUG_ON(rx_desc == NULL);
	bi = &rx_ring->rx_buffer_info[i];
	BUG_ON(bi == NULL);

	i -= rx_ring->count;
	bufsz = rnpgbevf_rx_bufsz(rx_ring);

	do {
		if (!rnpgbevf_alloc_mapped_page(rx_ring, bi))
			break;

		dma_sync_single_range_for_device(rx_ring->dev, bi->dma,
						 bi->page_offset, bufsz,
						 DMA_FROM_DEVICE);

		/*
		 * Refresh the desc even if buffer_addrs didn't change
		 * because each write-back erases this info.
		 */
		rx_desc->pkt_addr =
			cpu_to_le64(bi->dma + bi->page_offset + fun_id);
		/* clean dd */
		rx_desc->cmd = 0;

		rx_desc++;
		bi++;
		i++;
		if (unlikely(!i)) {
			rx_desc = RNPVF_RX_DESC(rx_ring, 0);
			bi = rx_ring->rx_buffer_info;
			i -= rx_ring->count;
		}

		/* clear the hdr_addr for the next_to_use descriptor */
		cleaned_count--;
	} while (cleaned_count);

	i += rx_ring->count;

	if (rx_ring->next_to_use != i)
		rnpgbevf_update_rx_tail(rx_ring, i);
}
#endif

/**
 * rnpgbevf_reuse_rx_page - page flip buffer and store it back on the ring
 * @rx_ring: rx descriptor ring to store buffers on
 * @old_buff: donor buffer to have page reused
 *
 * Synchronizes page for reuse by the adapter
 **/
static void rnpgbevf_reuse_rx_page(struct rnpgbevf_ring *rx_ring,
				   struct rnpgbevf_rx_buffer *old_buff)
{
	struct rnpgbevf_rx_buffer *new_buff;
	u16 nta = rx_ring->next_to_alloc;

	new_buff = &rx_ring->rx_buffer_info[nta];

	/* update, and store next to alloc */
	nta++;
	rx_ring->next_to_alloc = (nta < rx_ring->count) ? nta : 0;

	/*
	 * Transfer page from old buffer to new buffer.
	 * Move each member individually to avoid possible store
	 * forwarding stalls and unnecessary copy of skb.
	 */
	new_buff->dma = old_buff->dma;
	new_buff->page = old_buff->page;
	new_buff->page_offset = old_buff->page_offset;
	new_buff->pagecnt_bias = old_buff->pagecnt_bias;
}

static inline bool rnpgbevf_page_is_reserved(struct page *page)
{
	return (page_to_nid(page) != numa_mem_id()) || page_is_pfmemalloc(page);
}

static bool rnpgbevf_can_reuse_rx_page(struct rnpgbevf_rx_buffer *rx_buffer)
{
	unsigned int pagecnt_bias = rx_buffer->pagecnt_bias;
	struct page *page = rx_buffer->page;

#ifdef OPTM_WITH_LPAGE
	return false;
#endif
	/* avoid re-using remote pages */
	if (unlikely(rnpgbevf_page_is_reserved(page)))
		return false;

#if (PAGE_SIZE < 8192)
		/* if we are only owner of page we can reuse it */
	if (unlikely((page_ref_count(page) - pagecnt_bias) > 1))
		return false;
#else

	/*
	 * The last offset is a bit aggressive in that we assume the
	 * worst case of FCoE being enabled and using a 3K buffer.
	 * However this should have minimal impact as the 1K extra is
	 * still less than one buffer in size.
	 */
#define RNPVF_LAST_OFFSET (SKB_WITH_OVERHEAD(PAGE_SIZE) - RNPVF_RXBUFFER_2K)
	if (rx_buffer->page_offset > RNPVF_LAST_OFFSET)
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

#if (PAGE_SIZE < 8192)
#define RNPVF_MAX_2K_FRAME_BUILD_SKB (RNPVF_RXBUFFER_1536 - NET_IP_ALIGN)
#define RNPVF_2K_TOO_SMALL_WITH_PADDING                                        \
	((NET_SKB_PAD + RNPVF_RXBUFFER_1536) >                                 \
	 SKB_WITH_OVERHEAD(RNPVF_RXBUFFER_2K))

static inline int rnpgbevf_compute_pad(int rx_buf_len)
{
	int page_size, pad_size;

	page_size = ALIGN(rx_buf_len, PAGE_SIZE / 2);
	pad_size = SKB_WITH_OVERHEAD(page_size) - rx_buf_len;

	return pad_size;
}

static inline int rnpgbevf_skb_pad(void)
{
	int rx_buf_len;

	/* If a 2K buffer cannot handle a standard Ethernet frame then
	 * optimize padding for a 3K buffer instead of a 1.5K buffer.
	 *
	 * For a 3K buffer we need to add enough padding to allow for
	 * tailroom due to NET_IP_ALIGN possibly shifting us out of
	 * cache-line alignment.
	 */
	if (RNPVF_2K_TOO_SMALL_WITH_PADDING)
		rx_buf_len = RNPVF_RXBUFFER_3K + SKB_DATA_ALIGN(NET_IP_ALIGN);
	else
		rx_buf_len = RNPVF_RXBUFFER_1536;

	/* if needed make room for NET_IP_ALIGN */
	rx_buf_len -= NET_IP_ALIGN;
	return rnpgbevf_compute_pad(rx_buf_len);
}

#define RNPVF_SKB_PAD rnpgbevf_skb_pad()
#else /* PAGE_SIZE < 8192 */
#define RNPVF_SKB_PAD (NET_SKB_PAD + NET_IP_ALIGN)
#endif

/**
 * rnp_clean_rx_ring - Free Rx Buffers per Queue
 * @rx_ring: ring to free buffers from
 **/
static void rnpgbevf_clean_rx_ring(struct rnpgbevf_ring *rx_ring)
{
	u16 i = rx_ring->next_to_clean;
	struct rnpgbevf_rx_buffer *rx_buffer = &rx_ring->rx_buffer_info[i];

	/* Free all the Rx ring sk_buffs */
	while (i != rx_ring->next_to_alloc) {
		if (rx_buffer->skb) {
			struct sk_buff *skb = rx_buffer->skb;

			dev_kfree_skb(skb);
			rx_buffer->skb = NULL;
		}

		/* Invalidate cache lines that may have been written to by
		 * device so that we avoid corrupting memory.
		 */
		dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma,
					      rx_buffer->page_offset,
					      rnpgbevf_rx_bufsz(rx_ring),
					      DMA_FROM_DEVICE);

		/* free resources associated with mapping */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnpgbevf_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE,
				     RNPVF_RX_DMA_ATTR);

		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
		/* now this page is not used */
		rx_buffer->page = NULL;
		i++;
		rx_buffer++;
		if (i == rx_ring->count) {
			i = 0;
			rx_buffer = rx_ring->rx_buffer_info;
		}
	}

	rx_ring->next_to_alloc = 0;
	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
}

static inline unsigned int rnpgbevf_rx_offset(struct rnpgbevf_ring *rx_ring)
{
	return ring_uses_build_skb(rx_ring) ? RNPVF_SKB_PAD : 0;
}

#ifdef OPTM_WITH_LPAGE
static bool rnpgbevf_alloc_mapped_page(struct rnpgbevf_ring *rx_ring,
				       struct rnpgbevf_rx_buffer *bi,
				       union rnp_rx_desc *rx_desc, u16 bufsz,
				       u64 fun_id)
{
	struct page *page = bi->page;
	dma_addr_t dma;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page))
		return true;

	page = dev_alloc_pages(RNPVF_ALLOC_PAGE_ORDER);
	if (unlikely(!page)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	bi->page_offset = rnpgbevf_rx_offset(rx_ring);

	/* map page for use */
	dma = dma_map_page_attrs(rx_ring->dev, page, bi->page_offset, bufsz,
				 DMA_FROM_DEVICE,
				 RNPVF_RX_DMA_ATTR);

	/*
	 * if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		__free_pages(page, RNPVF_ALLOC_PAGE_ORDER);
		printk(KERN_DEBUG "map failed\n");

		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}
	bi->dma = dma;
	bi->page = page;
	bi->page_offset = rnpgbevf_rx_offset(rx_ring);
	page_ref_add(page, USHRT_MAX - 1);
	bi->pagecnt_bias = USHRT_MAX;
	rx_ring->rx_stats.alloc_rx_page++;

	/* sync the buffer for use by the device */
	dma_sync_single_range_for_device(rx_ring->dev, bi->dma, 0, bufsz,
					 DMA_FROM_DEVICE);

	/*
	 * Refresh the desc even if buffer_addrs didn't change
	 * because each write-back erases this info.
	 */
	rx_desc->pkt_addr = cpu_to_le64(bi->dma + fun_id);

	return true;
}

static void rnpgbevf_put_rx_buffer(struct rnpgbevf_ring *rx_ring,
				   struct rnpgbevf_rx_buffer *rx_buffer)
{
	if (rnpgbevf_can_reuse_rx_page(rx_buffer)) {
		/* hand second half of page back to the ring */
		rnpgbevf_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		/* we are not reusing the buffer so unmap it */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnpgbevf_rx_bufsz(rx_ring),
				     DMA_FROM_DEVICE,
				     RNPVF_RX_DMA_ATTR);
		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
	}

	/* clear contents of rx_buffer */
	rx_buffer->page = NULL;
}

/**
 * rnpgbevf_alloc_rx_buffers - Replace used receive buffers
 * @rx_ring: ring to place buffers on
 * @cleaned_count: number of buffers to replace
 **/
void rnpgbevf_alloc_rx_buffers(struct rnpgbevf_ring *rx_ring, u16 cleaned_count)
{
	union rnp_rx_desc *rx_desc;
	struct rnpgbevf_rx_buffer *bi;
	u16 i = rx_ring->next_to_use;
	u64 fun_id = ((u64)(rx_ring->vfnum) << (32 + 24));
	u16 bufsz;
	/* nothing to do */
	if (!cleaned_count)
		return;

	rx_desc = RNPVF_RX_DESC(rx_ring, i);

	BUG_ON(rx_desc == NULL);

	bi = &rx_ring->rx_buffer_info[i];

	BUG_ON(bi == NULL);

	i -= rx_ring->count;
	bufsz = rnpgbevf_rx_bufsz(rx_ring);

	do {
		int count = 1;
		struct page *page;

		if (!rnpgbevf_alloc_mapped_page(rx_ring, bi, rx_desc, bufsz,
						fun_id))
			break;
		page = bi->page;

		rx_desc->cmd = 0;

		rx_desc++;
		i++;
		bi++;

		if (unlikely(!i)) {
			rx_desc = RNPVF_RX_DESC(rx_ring, 0);
			bi = rx_ring->rx_buffer_info;
			i -= rx_ring->count;
		}

		rx_desc->cmd = 0;

		cleaned_count--;

		while (count < rx_ring->rx_page_buf_nums && cleaned_count) {
			dma_addr_t dma;

			bi->page_offset = rx_ring->rx_per_buf_mem * count +
					  rnpgbevf_rx_offset(rx_ring);
			/* map page for use */
			dma = dma_map_page_attrs(rx_ring->dev, page,
						 bi->page_offset, bufsz,
						 DMA_FROM_DEVICE,
						 RNPVF_RX_DMA_ATTR);

			if (dma_mapping_error(rx_ring->dev, dma)) {
				printk(KERN_DEBUG "map second error\n");
				rx_ring->rx_stats.alloc_rx_page_failed++;
				break;
			}

			bi->dma = dma;
			bi->page = page;
			page_ref_add(page, USHRT_MAX);
			bi->pagecnt_bias = USHRT_MAX;

			/* sync the buffer for use by the device */
			dma_sync_single_range_for_device(rx_ring->dev, bi->dma,
							 0, bufsz,
							 DMA_FROM_DEVICE);

			/*
			 * Refresh the desc even if buffer_addrs didn't change
			 * because each write-back erases this info.
			 */
			rx_desc->pkt_addr = cpu_to_le64(bi->dma + fun_id);
			/* clean dd */
			rx_desc->cmd = 0;

			rx_desc++;
			bi++;
			i++;
			if (unlikely(!i)) {
				rx_desc = RNPVF_RX_DESC(rx_ring, 0);
				bi = rx_ring->rx_buffer_info;
				i -= rx_ring->count;
			}
			count++;
			/* clear the hdr_addr for the next_to_use descriptor */
			cleaned_count--;
		}
	} while (cleaned_count);

	i += rx_ring->count;

	if (rx_ring->next_to_use != i)
		rnpgbevf_update_rx_tail(rx_ring, i);
}

#else
static bool rnpgbevf_alloc_mapped_page(struct rnpgbevf_ring *rx_ring,
				       struct rnpgbevf_rx_buffer *bi)
{
	struct page *page = bi->page;
	dma_addr_t dma;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page))
		return true;

	page = dev_alloc_pages(rnpgbevf_rx_pg_order(rx_ring));
	if (unlikely(!page)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	/* map page for use */
	dma = dma_map_page_attrs(rx_ring->dev, page, 0,
				 rnpgbevf_rx_pg_size(rx_ring), DMA_FROM_DEVICE,
				 RNPVF_RX_DMA_ATTR);

	/*
	 * if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		__free_pages(page, rnpgbevf_rx_pg_order(rx_ring));
		printk(KERN_DEBUG "map failed\n");

		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}
	bi->dma = dma;
	bi->page = page;
	bi->page_offset = rnpgbevf_rx_offset(rx_ring);
	page_ref_add(page, USHRT_MAX - 1);
	bi->pagecnt_bias = USHRT_MAX;
	rx_ring->rx_stats.alloc_rx_page++;

	return true;
}

static void rnpgbevf_put_rx_buffer(struct rnpgbevf_ring *rx_ring,
				   struct rnpgbevf_rx_buffer *rx_buffer,
				   struct sk_buff *skb)
{
	if (rnpgbevf_can_reuse_rx_page(rx_buffer)) {
		/* hand second half of page back to the ring */
		rnpgbevf_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		/* we are not reusing the buffer so unmap it */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnpgbevf_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE,
				     RNPVF_RX_DMA_ATTR);
		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
	}

	/* clear contents of rx_buffer */
	rx_buffer->page = NULL;
	rx_buffer->skb = NULL;
}

#endif /* OPTM_WITH_LPAGE */

/* drop this packets if error */
static bool rnpgbevf_check_csum_error(struct rnpgbevf_ring *rx_ring,
				      union rnp_rx_desc *rx_desc,
				      unsigned int size,
				      unsigned int *driver_drop_packets)
{
	bool err = false;

	struct net_device *netdev = rx_ring->netdev;
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);

	if ((netdev->features & NETIF_F_RXCSUM) &&
	    (!(adapter->priv_flags & RNPVF_PRIV_FLAG_FCS_ON))) {
		if (unlikely(rnpgbevf_test_staterr(rx_desc,
						   RNPGBE_RXD_STAT_ERR_MASK))) {
			/* push this packet to stack if in promisc mode */
			rx_ring->rx_stats.csum_err++;

			if ((!(netdev->flags & IFF_PROMISC) &&
			     (!(netdev->features & NETIF_F_RXALL)))) {
				if (rx_ring->ring_flags &
				    RNPVF_RING_CHKSM_FIX) {
					err = true;

					goto skip_fix;
				}
				if (unlikely(rnpgbevf_test_staterr(
						     rx_desc,
						     RNPGBE_RXD_STAT_L4_MASK) &&
					     (!(rx_desc->wb.rev1 &
						RNPGBE_RX_L3_TYPE_MASK)))) {
					rx_ring->rx_stats.csum_err--;
					goto skip_fix;
				}

				if (unlikely(rnpgbevf_test_staterr(
					    rx_desc,
					    RNPGBE_RXD_STAT_SCTP_MASK))) {
					if (size > 60) {
						err = true;
					} else {
						/* sctp less than 60 hw report err by mistake */
						rx_ring->rx_stats.csum_err--;
					}
				} else {
					err = true;
				}
			}
		}
	}

skip_fix:
	if (err) {
		u32 ntc = rx_ring->next_to_clean + 1;
		struct rnpgbevf_rx_buffer *rx_buffer;
#if (PAGE_SIZE < 8192)
		unsigned int truesize = rnpgbevf_rx_pg_size(rx_ring) / 2;
#else
		unsigned int truesize =
			ring_uses_build_skb(rx_ring) ?
				SKB_DATA_ALIGN(RNPVF_SKB_PAD + size) :
				SKB_DATA_ALIGN(size);
#endif

		if (likely(rnpgbevf_test_staterr(rx_desc, RNPGBE_RXD_STAT_EOP)))
			*driver_drop_packets = *driver_drop_packets + 1;

		/* we are reusing so sync this buffer for CPU use */
		rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
		dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma,
					      rx_buffer->page_offset, size,
					      DMA_FROM_DEVICE);
		/* we should clean it since we used all info in it */
		rx_desc->wb.cmd = 0;

#if (PAGE_SIZE < 8192)
		rx_buffer->page_offset ^= truesize;
#else
		rx_buffer->page_offset += truesize;
#endif
#ifdef OPTM_WITH_LPAGE
		rnpgbevf_put_rx_buffer(rx_ring, rx_buffer);
#else
		rnpgbevf_put_rx_buffer(rx_ring, rx_buffer, NULL);
#endif
		ntc = (ntc < rx_ring->count) ? ntc : 0;
		rx_ring->next_to_clean = ntc;
	}
	return err;
}

/**
 * rnpgbevf_process_skb_fields - Populate skb header fields from Rx descriptor
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @rx_desc: pointer to the EOP Rx descriptor
 * @skb: pointer to current skb being populated
 *
 * This function checks the ring, descriptor, and packet information in
 * order to populate the hash, checksum, VLAN, timestamp, protocol, and
 * other fields within the skb.
 **/
static void rnpgbevf_process_skb_fields(struct rnpgbevf_ring *rx_ring,
					union rnp_rx_desc *rx_desc,
					struct sk_buff *skb)
{
	struct net_device *dev = rx_ring->netdev;
	struct rnpgbevf_adapter *adapter = netdev_priv(dev);
	struct rnpgbevf_hw *hw = &adapter->hw;

	rnpgbevf_rx_hash(rx_ring, rx_desc, skb);
	rnpgbevf_rx_checksum(rx_ring, rx_desc, skb);

	if (hw->pf_feature & PF_NCSI_EN) {
		/* if pf setup vf vlan, check it */
		if (adapter->flags & RNPVF_FLAG_PF_SET_VLAN) {
			u16 vid_pf;
			u8 header[ETH_ALEN + ETH_ALEN];
			u8 *data = skb->data;

			if (__vlan_get_tag(skb, &vid_pf))
				goto skip_vf_vlan;

			if (vid_pf == adapter->vf_vlan) {
				memcpy(header, data, ETH_ALEN + ETH_ALEN);
				memcpy(skb->data + 4, header, ETH_ALEN + ETH_ALEN);
				skb->len -= 4;
				skb->data += 4;
				goto skip_vf_vlan;
			}
		}
	}
	/* remove vlan if pf set a vlan */
	if (((dev->features & NETIF_F_HW_VLAN_CTAG_RX)
	     || (dev->features & NETIF_F_HW_VLAN_STAG_RX)) &&
	    rnpgbevf_test_staterr(rx_desc, RNPGBE_RXD_STAT_VLAN_VALID) &&
	    !(cpu_to_le16(rx_desc->wb.rev1) & VEB_VF_IGNORE_VLAN)) {
		u16 vid = le16_to_cpu(rx_desc->wb.vlan);
		if (rx_ring->ring_flags & RNPVF_RING_STAGS_SUPPORT) {
			if (rnpgbevf_test_staterr(rx_desc,
						  RNPGBE_RXD_STAT_STAG)) {
				__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021AD),
						       vid);

			} else {
				if ((adapter->vf_vlan) &&
				    (adapter->vf_vlan == vid))
					goto skip_vf_vlan;
				__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
						       vid);
			}
		} else {
			if ((adapter->vf_vlan) && (adapter->vf_vlan == vid))
				goto skip_vf_vlan;
			__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), vid);
		}
		rx_ring->rx_stats.vlan_remove++;
	}
skip_vf_vlan:
	skb_record_rx_queue(skb, rx_ring->queue_index);

	skb->protocol = eth_type_trans(skb, dev);
}

static void rnpgbevf_rx_skb(struct rnpgbevf_q_vector *q_vector,
			    struct sk_buff *skb)
{
	struct rnpgbevf_adapter *adapter = q_vector->adapter;

	if (!(adapter->flags & RNPVF_FLAG_IN_NETPOLL))
		napi_gro_receive(&q_vector->napi, skb);
	else
		netif_rx(skb);
}

/**
 * rnpgbevf_get_headlen - determine size of header for RSC/LRO/GRO/FCOE
 * @data: pointer to the start of the headers
 * @max_len: total length of section to find headers in
 *
 * This function is meant to determine the length of headers that will
 * be recognized by hardware for LRO, GRO, and RSC offloads.  The main
 * motivation of doing this is to only perform one pull for IPv4 TCP
 * packets so that we can do basic things like calculating the gso_size
 * based on the average data per packet.
 **/
static unsigned int rnpgbevf_get_headlen(unsigned char *data,
					 unsigned int max_len)
{
	union {
		unsigned char *network;
		/* l2 headers */
		struct ethhdr *eth;
		struct vlan_hdr *vlan;
		/* l3 headers */
		struct iphdr *ipv4;
		struct ipv6hdr *ipv6;
	} hdr;
	__be16 protocol;
	u8 nexthdr = 0; /* default to not TCP */
	u8 hlen;

	/* this should never happen, but better safe than sorry */
	if (max_len < ETH_HLEN)
		return max_len;

	/* initialize network frame pointer */
	hdr.network = data;

	/* set first protocol and move network header forward */
	protocol = hdr.eth->h_proto;
	hdr.network += ETH_HLEN;

	/* handle any vlan tag if present */
	if (protocol == htons(ETH_P_8021Q)) {
		if ((hdr.network - data) > (max_len - VLAN_HLEN))
			return max_len;

		protocol = hdr.vlan->h_vlan_encapsulated_proto;
		hdr.network += VLAN_HLEN;
	}

	/* handle L3 protocols */
	if (protocol == htons(ETH_P_IP)) {
		if ((hdr.network - data) > (max_len - sizeof(struct iphdr)))
			return max_len;

		/* access ihl as a u8 to avoid unaligned access on ia64 */
		hlen = (hdr.network[0] & 0x0F) << 2;

		/* verify hlen meets minimum size requirements */
		if (hlen < sizeof(struct iphdr))
			return hdr.network - data;

		/* record next protocol if header is present */
		if (!(hdr.ipv4->frag_off & htons(IP_OFFSET)))
			nexthdr = hdr.ipv4->protocol;
	} else if (protocol == htons(ETH_P_IPV6)) {
		if ((hdr.network - data) > (max_len - sizeof(struct ipv6hdr)))
			return max_len;

		/* record next protocol */
		nexthdr = hdr.ipv6->nexthdr;
		hlen = sizeof(struct ipv6hdr);
	} else {
		return hdr.network - data;
	}

	/* relocate pointer to start of L4 header */
	hdr.network += hlen;

	/* finally sort out TCP/UDP */
	if (nexthdr == IPPROTO_TCP) {
		if ((hdr.network - data) > (max_len - sizeof(struct tcphdr)))
			return max_len;

		/* access doff as a u8 to avoid unaligned access on ia64 */
		hlen = (hdr.network[12] & 0xF0) >> 2;

		/* verify hlen meets minimum size requirements */
		if (hlen < sizeof(struct tcphdr))
			return hdr.network - data;

		hdr.network += hlen;
	} else if (nexthdr == IPPROTO_UDP) {
		if ((hdr.network - data) > (max_len - sizeof(struct udphdr)))
			return max_len;

		hdr.network += sizeof(struct udphdr);
	}

	/*
	 * If everything has gone correctly hdr.network should be the
	 * data section of the packet and will be the end of the header.
	 * If not then it probably represents the end of the last recognized
	 * header.
	 */
	if ((hdr.network - data) < max_len)
		return hdr.network - data;
	else
		return max_len;
}

/**
 * rnpgbevf_pull_tail - rnp specific version of skb_pull_tail
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @skb: pointer to current skb being adjusted
 *
 * This function is an rnp specific version of __pskb_pull_tail.  The
 * main difference between this version and the original function is that
 * this function can make several assumptions about the state of things
 * that allow for significant optimizations versus the standard function.
 * As a result we can do things like drop a frag and maintain an accurate
 * truesize for the skb.
 */
static void rnpgbevf_pull_tail(struct sk_buff *skb)
{
	skb_frag_t *frag = &skb_shinfo(skb)->frags[0];
	unsigned char *va;
	unsigned int pull_len;

	/*
	 * it is valid to use page_address instead of kmap since we are
	 * working with pages allocated out of the lomem pool per
	 * alloc_page(GFP_ATOMIC)
	 */
	va = skb_frag_address(frag);

	/*
	 * we need the header to contain the greater of either ETH_HLEN or
	 * 60 bytes if the skb->len is less than 60 for skb_pad.
	 */
	pull_len = rnpgbevf_get_headlen(va, RNPVF_RX_HDR_SIZE);

	/* align pull length to size of long to optimize memcpy performance */
	skb_copy_to_linear_data(skb, va, ALIGN(pull_len, sizeof(long)));

	/* update all of the pointers */
	skb_frag_size_sub(frag, pull_len);
	skb_frag_off_add(frag, pull_len);
	skb->data_len -= pull_len;
	skb->tail += pull_len;
}

/**
 * rnpgbevf_cleanup_headers - Correct corrupted or empty headers
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @rx_desc: pointer to the EOP Rx descriptor
 * @skb: pointer to current skb being fixed
 *
 * Check for corrupted packet headers caused by senders on the local L2
 * embedded NIC switch not setting up their Tx Descriptors right.  These
 * should be very rare.
 *
 * Also address the case where we are pulling data in on pages only
 * and as such no data is present in the skb header.
 *
 * In addition if skb is not at least 60 bytes we need to pad it so that
 * it is large enough to qualify as a valid Ethernet frame.
 *
 * Returns true if an error was encountered and skb was freed.
 **/
static bool rnpgbevf_cleanup_headers(struct rnpgbevf_ring *rx_ring,
				     union rnp_rx_desc *rx_desc,
				     struct sk_buff *skb)
{
#ifdef OPTM_WITH_LPAGE
#else
	/* XDP packets use error pointer so abort at this point */
	if (IS_ERR(skb))
		return true;
#endif

	/* place header in linear portion of buffer */
	if (!skb_headlen(skb))
		rnpgbevf_pull_tail(skb);

	if (eth_skb_pad(skb))
		return true;
	return false;
}

/**
 * rnpgbevf_add_rx_frag - Add contents of Rx buffer to sk_buff
 * @rx_ring: rx descriptor ring to transact packets on
 * @rx_buffer: buffer containing page to add
 * @skb: sk_buff to place the data into
 * @size: size of data
 *
 * This function will add the data contained in rx_buffer->page to the skb.
 * This is done either through a direct copy if the data in the buffer is
 * less than the skb header size, otherwise it will just attach the page as
 * a frag to the skb.
 *
 * The function will then update the page offset if necessary and return
 * true if the buffer can be reused by the adapter.
 **/
static void rnpgbevf_add_rx_frag(struct rnpgbevf_ring *rx_ring,
				 struct rnpgbevf_rx_buffer *rx_buffer,
				 struct sk_buff *skb, unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnpgbevf_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize = ring_uses_build_skb(rx_ring) ?
					SKB_DATA_ALIGN(RNPVF_SKB_PAD + size) :
					SKB_DATA_ALIGN(size);
#endif

	skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, rx_buffer->page,
			rx_buffer->page_offset, size, truesize);

#if (PAGE_SIZE < 8192)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif
}

#ifdef OPTM_WITH_LPAGE
static struct sk_buff *rnpgbevf_build_skb(struct rnpgbevf_ring *rx_ring,
					  struct rnpgbevf_rx_buffer *rx_buffer,
					  union rnp_rx_desc *rx_desc,
					  unsigned int size)
{
	void *va = page_address(rx_buffer->page) + rx_buffer->page_offset;
	unsigned int truesize = SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
				SKB_DATA_ALIGN(size + RNPVF_SKB_PAD);
	struct sk_buff *skb;

	/* prefetch first cache line of first page */
	prefetch(va);
#if L1_CACHE_BYTES < 128
	prefetch(va + L1_CACHE_BYTES);
#endif

	/* build an skb around the page buffer */
	skb = build_skb(va - RNPVF_SKB_PAD, truesize);
	if (unlikely(!skb))
		return NULL;

	/* update pointers within the skb to store the data */
	skb_reserve(skb, RNPVF_SKB_PAD);
	__skb_put(skb, size);

	return skb;
}

static struct rnpgbevf_rx_buffer *
rnpgbevf_get_rx_buffer(struct rnpgbevf_ring *rx_ring,
		       union rnp_rx_desc *rx_desc, const unsigned int size)
{
	struct rnpgbevf_rx_buffer *rx_buffer;

	rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
	prefetchw(rx_buffer->page);

	rx_buf_dump("rx buf",
		    page_address(rx_buffer->page) + rx_buffer->page_offset,
		    rx_desc->wb.len);

	/* we are reusing so sync this buffer for CPU use */
	dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma, 0, size,
				      DMA_FROM_DEVICE);
	/* skip_sync: */
	rx_buffer->pagecnt_bias--;

	return rx_buffer;
}

/**
 * rnpgbevf_is_non_eop - process handling of non-EOP buffers
 * @rx_ring: Rx ring being processed
 * @rx_desc: Rx descriptor for current buffer
 * @skb: Current socket buffer containing buffer in progress
 *
 * This function updates next to clean.  If the buffer is an EOP buffer
 * this function exits returning false, otherwise it will place the
 * sk_buff in the next buffer to be chained and return true indicating
 * that this is in fact a non-EOP buffer.
 **/
static bool rnpgbevf_is_non_eop(struct rnpgbevf_ring *rx_ring,
				union rnp_rx_desc *rx_desc)
{
	u32 ntc = rx_ring->next_to_clean + 1;
	/* fetch, update, and store next to clean */
	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;

	prefetch(RNPVF_RX_DESC(rx_ring, ntc));

	/* if we are the last buffer then there is nothing else to do */
	if (likely(rnpgbevf_test_staterr(rx_desc, RNPGBE_RXD_STAT_EOP)))
		return false;
	/* place skb in next buffer to be received */
	/* we should clean it since we used all info in it */
	rx_desc->wb.cmd = 0;

	return true;
}

static struct sk_buff *
rnpgbevf_construct_skb(struct rnpgbevf_ring *rx_ring,
		       struct rnpgbevf_rx_buffer *rx_buffer,
		       union rnp_rx_desc *rx_desc, unsigned int size)
{
	void *va = page_address(rx_buffer->page) + rx_buffer->page_offset;
	unsigned int truesize = SKB_DATA_ALIGN(size);
	unsigned int headlen;
	struct sk_buff *skb;

	/* prefetch first cache line of first page */
	prefetch(va);
#if L1_CACHE_BYTES < 128
	prefetch(va + L1_CACHE_BYTES);
#endif

	/* allocate a skb to store the frags */
	skb = napi_alloc_skb(&rx_ring->q_vector->napi, RNPVF_RX_HDR_SIZE);
	if (unlikely(!skb))
		return NULL;

	prefetchw(skb->data);

	/* Determine available headroom for copy */
	headlen = size;
	if (headlen > RNPVF_RX_HDR_SIZE)
		headlen = rnpgbevf_get_headlen(va, RNPVF_RX_HDR_SIZE);

	/* align pull length to size of long to optimize memcpy performance */
	memcpy(__skb_put(skb, headlen), va, ALIGN(headlen, sizeof(long)));

	/* update all of the pointers */
	size -= headlen;

	if (size) {
		skb_add_rx_frag(skb, 0, rx_buffer->page,
				(va + headlen) - page_address(rx_buffer->page),
				size, truesize);
		rx_buffer->page_offset += truesize;
	} else {
		rx_buffer->pagecnt_bias++;
	}

	return skb;
}

/**
 * rnp_clean_rx_irq - Clean completed descriptors from Rx ring - bounce buf
 * @q_vector: structure containing interrupt and ring information
 * @rx_ring: rx descriptor ring to transact packets on
 * @budget: Total limit on number of packets to process
 *
 * This function provides a "bounce buffer" approach to Rx interrupt
 * processing.  The advantage to this is that on systems that have
 * expensive overhead for IOMMU access this provides a means of avoiding
 * it by maintaining the mapping of the page to the syste.
 *
 * Returns amount of work completed.
 **/

static int rnpgbevf_clean_rx_irq(struct rnpgbevf_q_vector *q_vector,
				 struct rnpgbevf_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	unsigned int err_packets = 0;
	unsigned int driver_drop_packets = 0;
	struct sk_buff *skb = rx_ring->skb;
	struct rnpgbevf_adapter *adapter = q_vector->adapter;
	u16 cleaned_count = rnpgbevf_desc_unused(rx_ring);

	while (likely(total_rx_packets < budget)) {
		union rnp_rx_desc *rx_desc;
		struct rnpgbevf_rx_buffer *rx_buffer;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= RNPVF_RX_BUFFER_WRITE) {
			rnpgbevf_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}
		rx_desc = RNPVF_RX_DESC(rx_ring, rx_ring->next_to_clean);
		rx_buf_dump("rx-desc:", rx_desc, sizeof(*rx_desc));
		rx_debug_printk("  dd set: %s\n",
				(rx_desc->wb.cmd & RNPGBE_RXD_STAT_DD) ? "Yes" :
									 "No");

		if (!rnpgbevf_test_staterr(rx_desc, RNPGBE_RXD_STAT_DD))
			break;

		rx_debug_printk(
			"queue:%d  rx-desc:%d has-data len:%d next_to_clean %d\n",
			rx_ring->rnp_queue_idx, rx_ring->next_to_clean,
			rx_desc->wb.len, rx_ring->next_to_clean);

		/* handle padding */
		if ((adapter->priv_flags & RNPVF_PRIV_FLAG_FT_PADDING) &&
		    (!(adapter->priv_flags & RNPVF_PRIV_FLAG_PADDING_DEBUG))) {
			if (likely(rnpgbevf_test_staterr(
				    rx_desc, RNPGBE_RXD_STAT_EOP))) {
				size = le16_to_cpu(rx_desc->wb.len) -
				       le16_to_cpu(rx_desc->wb.padding_len);
			} else {
				size = le16_to_cpu(rx_desc->wb.len);
			}
		} else {
			/* size should not zero */
			size = le16_to_cpu(rx_desc->wb.len);
		}

		if (!size)
			break;

		/*
		 * should check csum err
		 * maybe one packet use mutiple descs
		 * no problems hw set all csum_err in mutiple descs
		 * maybe BUG if the last sctp desc less than 60
		 */
		if (rnpgbevf_check_csum_error(rx_ring, rx_desc, size,
					      &driver_drop_packets)) {
			cleaned_count++;
			err_packets++;
			if (err_packets + total_rx_packets > budget)
				break;
			continue;
		}
		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();

		rx_buffer = rnpgbevf_get_rx_buffer(rx_ring, rx_desc, size);

		if (skb) {
			rnpgbevf_add_rx_frag(rx_ring, rx_buffer, skb, size);
		} else if (ring_uses_build_skb(rx_ring)) {
			skb = rnpgbevf_build_skb(rx_ring, rx_buffer, rx_desc,
						 size);
		} else {
			skb = rnpgbevf_construct_skb(rx_ring, rx_buffer,
						     rx_desc, size);
		}

		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			rx_buffer->pagecnt_bias++;
			break;
		}
		rnpgbevf_put_rx_buffer(rx_ring, rx_buffer);
		cleaned_count++;

		/* place incomplete frames back on ring for completion */
		if (rnpgbevf_is_non_eop(rx_ring, rx_desc))
			continue;

		/* verify the packet layout is correct */
		if (rnpgbevf_cleanup_headers(rx_ring, rx_desc, skb)) {
			/* we should clean it since we used all info in it */
			rx_desc->wb.cmd = 0;
			skb = NULL;
			continue;
		}

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, timestamp, VLAN, and protocol */
		rnpgbevf_process_skb_fields(rx_ring, rx_desc, skb);

		/* we should clean it since we used all info in it */
		rx_desc->wb.cmd = 0;
		rnpgbevf_rx_skb(q_vector, skb);
		skb = NULL;

		/* update budget accounting */
		total_rx_packets++;
	}

	rx_ring->skb = skb;

	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	rx_ring->rx_stats.driver_drop_packets += driver_drop_packets;
	u64_stats_update_end(&rx_ring->syncp);
	q_vector->rx.total_packets += total_rx_packets;
	q_vector->rx.total_bytes += total_rx_bytes;

	if (total_rx_packets >= budget)
		rx_ring->rx_stats.poll_again_count++;

	return total_rx_packets;
}

#else

/**
 * rnpgbevf_is_non_eop - process handling of non-EOP buffers
 * @rx_ring: Rx ring being processed
 * @rx_desc: Rx descriptor for current buffer
 * @skb: Current socket buffer containing buffer in progress
 *
 * This function updates next to clean.  If the buffer is an EOP buffer
 * this function exits returning false, otherwise it will place the
 * sk_buff in the next buffer to be chained and return true indicating
 * that this is in fact a non-EOP buffer.
 **/
static bool rnpgbevf_is_non_eop(struct rnpgbevf_ring *rx_ring,
				union rnp_rx_desc *rx_desc, struct sk_buff *skb)
{
	u32 ntc = rx_ring->next_to_clean + 1;

	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;
	prefetch(RNPVF_RX_DESC(rx_ring, ntc));

	/* if we are the last buffer then there is nothing else to do */
	if (likely(rnpgbevf_test_staterr(rx_desc, RNPGBE_RXD_STAT_EOP)))
		return false;
	/* place skb in next buffer to be received */
	rx_ring->rx_buffer_info[ntc].skb = skb;
	/* we should clean it since we used all info in it */
	rx_desc->wb.cmd = 0;
	rx_ring->rx_stats.non_eop_descs++;

	return true;
}

static struct sk_buff *rnpgbevf_build_skb(struct rnpgbevf_ring *rx_ring,
					  struct rnpgbevf_rx_buffer *rx_buffer,
					  struct xdp_buff *xdp,
					  union rnp_rx_desc *rx_desc)
{
	unsigned int metasize = xdp->data - xdp->data_meta;
	void *va = xdp->data_meta;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnpgbevf_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize =
		SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
		SKB_DATA_ALIGN(xdp->data_end - xdp->data_hard_start);
#endif
	struct sk_buff *skb;

	/* prefetch first cache line of first page */
	prefetch(va);
#if L1_CACHE_BYTES < 128
	prefetch(va + L1_CACHE_BYTES);
#endif

	/* build an skb around the page buffer */
	skb = build_skb(xdp->data_hard_start, truesize);
	if (unlikely(!skb))
		return NULL;

	/* update pointers within the skb to store the data */
	skb_reserve(skb, xdp->data - xdp->data_hard_start);
	__skb_put(skb, xdp->data_end - xdp->data);
	if (metasize)
		skb_metadata_set(skb, metasize);
		/* update buffer offset */
#if (PAGE_SIZE < 8192)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif

	return skb;
}

static void rnpgbevf_rx_buffer_flip(struct rnpgbevf_ring *rx_ring,
				    struct rnpgbevf_rx_buffer *rx_buffer,
				    unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnpgbevf_rx_pg_size(rx_ring) / 2;

	rx_buffer->page_offset ^= truesize;
#else
	unsigned int truesize = ring_uses_build_skb(rx_ring) ?
					SKB_DATA_ALIGN(RNPVF_SKB_PAD + size) :
					SKB_DATA_ALIGN(size);

	rx_buffer->page_offset += truesize;
#endif
}

static struct rnpgbevf_rx_buffer *
rnpgbevf_get_rx_buffer(struct rnpgbevf_ring *rx_ring,
		       union rnp_rx_desc *rx_desc, struct sk_buff **skb,
		       const unsigned int size)
{
	struct rnpgbevf_rx_buffer *rx_buffer;

	rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
	prefetchw(rx_buffer->page);
	*skb = rx_buffer->skb;

	rx_buf_dump("rx buf",
		    page_address(rx_buffer->page) + rx_buffer->page_offset,
		    rx_desc->wb.len);

	/* we are reusing so sync this buffer for CPU use */
	dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma,
				      rx_buffer->page_offset, size,
				      DMA_FROM_DEVICE);
	/* skip_sync: */
	rx_buffer->pagecnt_bias--;

	return rx_buffer;
}

static struct sk_buff *
rnpgbevf_construct_skb(struct rnpgbevf_ring *rx_ring,
		       struct rnpgbevf_rx_buffer *rx_buffer,
		       struct xdp_buff *xdp, union rnp_rx_desc *rx_desc)
{
	unsigned int size = xdp->data_end - xdp->data;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnpgbevf_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize =
		SKB_DATA_ALIGN(xdp->data_end - xdp->data_hard_start);
#endif
	struct sk_buff *skb;

	/* prefetch first cache line of first page */
	prefetch(xdp->data);
#if L1_CACHE_BYTES < 128
	prefetch(xdp->data + L1_CACHE_BYTES);
#endif

	/* allocate a skb to store the frags */
	skb = napi_alloc_skb(&rx_ring->q_vector->napi, RNPVF_RX_HDR_SIZE);
	if (unlikely(!skb))
		return NULL;

	prefetchw(skb->data);

	if (size > RNPVF_RX_HDR_SIZE) {
		skb_add_rx_frag(skb, 0, rx_buffer->page,
				xdp->data - page_address(rx_buffer->page), size,
				truesize);
#if (PAGE_SIZE < 8192)
		rx_buffer->page_offset ^= truesize;
#else
		rx_buffer->page_offset += truesize;
#endif
	} else {
		memcpy(__skb_put(skb, size), xdp->data,
		       ALIGN(size, sizeof(long)));
		rx_buffer->pagecnt_bias++;
	}

	return skb;
}

/**
 * rnp_clean_rx_irq - Clean completed descriptors from Rx ring - bounce buf
 * @q_vector: structure containing interrupt and ring information
 * @rx_ring: rx descriptor ring to transact packets on
 * @budget: Total limit on number of packets to process
 *
 * This function provides a "bounce buffer" approach to Rx interrupt
 * processing.  The advantage to this is that on systems that have
 * expensive overhead for IOMMU access this provides a means of avoiding
 * it by maintaining the mapping of the page to the syste.
 *
 * Returns amount of work completed.
 **/
static int rnpgbevf_clean_rx_irq(struct rnpgbevf_q_vector *q_vector,
				 struct rnpgbevf_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	unsigned int err_packets = 0;
	unsigned int driver_drop_packets = 0;
	struct rnpgbevf_adapter *adapter = q_vector->adapter;
	u16 cleaned_count = rnpgbevf_desc_unused(rx_ring);
	bool xdp_xmit = false;
	struct xdp_buff xdp;

	xdp.data = NULL;
	xdp.data_end = NULL;

	while (likely(total_rx_packets < budget)) {
		union rnp_rx_desc *rx_desc;
		struct rnpgbevf_rx_buffer *rx_buffer;
		struct sk_buff *skb;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= RNPVF_RX_BUFFER_WRITE) {
			rnpgbevf_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}
		rx_desc = RNPVF_RX_DESC(rx_ring, rx_ring->next_to_clean);

		rx_buf_dump("rx-desc:", rx_desc, sizeof(*rx_desc));
		rx_debug_printk("  dd set: %s\n",
				(rx_desc->wb.cmd & RNPGBE_RXD_STAT_DD) ? "Yes" :
									 "No");

		if (!rnpgbevf_test_staterr(rx_desc, RNPGBE_RXD_STAT_DD))
			break;

		rx_debug_printk(
			"queue:%d  rx-desc:%d has-data len:%d next_to_clean %d\n",
			rx_ring->rnpgbevf_queue_idx, rx_ring->next_to_clean,
			rx_desc->wb.len, rx_ring->next_to_clean);

		/* handle padding */
		if ((adapter->priv_flags & RNPVF_PRIV_FLAG_FT_PADDING) &&
		    (!(adapter->priv_flags & RNPVF_PRIV_FLAG_PADDING_DEBUG))) {
			if (likely(rnpgbevf_test_staterr(
				    rx_desc, RNPGBE_RXD_STAT_EOP))) {
				size = le16_to_cpu(rx_desc->wb.len) -
				       le16_to_cpu(rx_desc->wb.padding_len);
			} else {
				size = le16_to_cpu(rx_desc->wb.len);
			}
		} else {
			/* size should not zero */
			size = le16_to_cpu(rx_desc->wb.len);
		}

		if (!size)
			break;

		/*
		 * should check csum err
		 * maybe one packet use mutiple descs
		 * no problems hw set all csum_err in mutiple descs
		 * maybe BUG if the last sctp desc less than 60
		 */
		if (rnpgbevf_check_csum_error(rx_ring, rx_desc, size,
					      &driver_drop_packets)) {
			cleaned_count++;
			err_packets++;
			if (err_packets + total_rx_packets > budget)
				break;
			continue;
		}
		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();

		rx_buffer =
			rnpgbevf_get_rx_buffer(rx_ring, rx_desc, &skb, size);

		if (!skb) {
			xdp.data = page_address(rx_buffer->page) +
				   rx_buffer->page_offset;
			xdp.data_meta = xdp.data;
			xdp.data_hard_start =
				xdp.data - rnpgbevf_rx_offset(rx_ring);
			xdp.data_end = xdp.data + size;
		}

		if (IS_ERR(skb)) {
			if (PTR_ERR(skb) == -RNPVF_XDP_TX) {
				xdp_xmit = true;
				rnpgbevf_rx_buffer_flip(rx_ring, rx_buffer,
							size);
			} else {
				rx_buffer->pagecnt_bias++;
			}
			total_rx_packets++;
			total_rx_bytes += size;
		} else if (skb) {
			rnpgbevf_add_rx_frag(rx_ring, rx_buffer, skb, size);
		} else if (ring_uses_build_skb(rx_ring)) {
			skb = rnpgbevf_build_skb(rx_ring, rx_buffer, &xdp,
						 rx_desc);
		} else {
			skb = rnpgbevf_construct_skb(rx_ring, rx_buffer, &xdp,
						     rx_desc);
		}

		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			rx_buffer->pagecnt_bias++;
			break;
		}

		rnpgbevf_put_rx_buffer(rx_ring, rx_buffer, skb);
		cleaned_count++;

		/* place incomplete frames back on ring for completion */
		if (rnpgbevf_is_non_eop(rx_ring, rx_desc, skb))
			continue;

		/* verify the packet layout is correct */
		if (rnpgbevf_cleanup_headers(rx_ring, rx_desc, skb)) {
			/* we should clean it since we used all info in it */
			rx_desc->wb.cmd = 0;
			continue;
		}

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, timestamp, VLAN, and protocol */
		rnpgbevf_process_skb_fields(rx_ring, rx_desc, skb);

		/* we should clean it since we used all info in it */
		rx_desc->wb.cmd = 0;

		rnpgbevf_rx_skb(q_vector, skb);

		/* update budget accounting */
		total_rx_packets++;
	}

	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	rx_ring->rx_stats.driver_drop_packets += driver_drop_packets;
	u64_stats_update_end(&rx_ring->syncp);
	q_vector->rx.total_packets += total_rx_packets;
	q_vector->rx.total_bytes += total_rx_bytes;

	if (total_rx_packets >= budget)
		rx_ring->rx_stats.poll_again_count++;
	return total_rx_packets;
}
#endif

/**
 * rnpgbevf_configure_msix - Configure MSI-X hardware
 * @adapter: board private structure
 *
 * rnpgbevf_configure_msix sets up the hardware to properly generate MSI-X
 * interrupts.
 **/
static void rnpgbevf_configure_msix(struct rnpgbevf_adapter *adapter)
{
	struct rnpgbevf_q_vector *q_vector;
	int i;

	/*
	 * configure ring-msix Registers table
	 */
	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnpgbevf_ring *ring;

		q_vector = adapter->q_vector[i];

		rnpgbevf_for_each_ring(ring, q_vector->rx)
		{
			rnpgbevf_set_ring_vector(adapter,
						 ring->rnpgbevf_msix_off,
						 q_vector->v_idx);
		}
	}
}

enum latency_range {
	lowest_latency = 0,
	low_latency = 1,
	bulk_latency = 2,
	latency_invalid = 255
};

static inline void
rnpgbevf_irq_enable_queues(struct rnpgbevf_q_vector *q_vector)
{
	struct rnpgbevf_ring *ring;

	rnpgbevf_for_each_ring(ring, q_vector->rx) {
		rnpgbevf_wr_reg(ring->dma_int_clr, RX_INT_MASK | TX_INT_MASK);
		/* we need this */
		wmb();
		ring_wr32(ring, RNPGBE_DMA_INT_TRIG,
			  (0x3 << 16) | TX_INT_MASK | RX_INT_MASK);
		rnpgbevf_wr_reg(ring->dma_int_mask,
				~(RX_INT_MASK | TX_INT_MASK));
	}
}

static inline void
rnpgbevf_irq_disable_queues(struct rnpgbevf_q_vector *q_vector)
{
	struct rnpgbevf_ring *ring;

	rnpgbevf_for_each_ring(ring, q_vector->tx)
	{
		ring_wr32(ring, RNPGBE_DMA_INT_TRIG,
			  (0x3 << 16) | (~TX_INT_MASK | RX_INT_MASK));
		rnpgbevf_wr_reg(ring->dma_int_mask,
				(RX_INT_MASK | TX_INT_MASK));
	}
}

/**
 * rnpgbevf_irq_enable - Enable default interrupt generation settings
 * @adapter: board private structure
 **/
static inline void rnpgbevf_irq_enable(struct rnpgbevf_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_q_vectors; i++)
		rnpgbevf_irq_enable_queues(adapter->q_vector[i]);
}

static irqreturn_t rnpgbevf_msix_other(int irq, void *data)
{
	struct rnpgbevf_adapter *adapter = data;
	struct rnpgbevf_hw *hw = &adapter->hw;

	dbg("\n\n !!! %s irq-comming !!!\n", __func__);

	/* link is down by pf */
	if (test_bit(__RNPVF_MBX_POLLING, &adapter->state))
		goto NO_WORK_DONE;
	if (!hw->mbx.ops.check_for_rst(hw, false)) {
		if (test_bit(__RNPVF_REMOVE, &adapter->state)) {
			printk(KERN_DEBUG "rnpvf is removed\n");
		}
	}
NO_WORK_DONE:

	return IRQ_HANDLED;
}

static void rnpgbevf_htimer_start(struct rnpgbevf_q_vector *q_vector)
{
	unsigned long ns = q_vector->irq_check_usecs * NSEC_PER_USEC / 2;

	hrtimer_start_range_ns(&q_vector->irq_miss_check_timer, ns_to_ktime(ns),
			       ns, HRTIMER_MODE_REL);
}

static void rnpgbevf_htimer_stop(struct rnpgbevf_q_vector *q_vector)
{
	hrtimer_cancel(&q_vector->irq_miss_check_timer);
}

static irqreturn_t rnpgbevf_intr(int irq, void *data)
{
	struct rnpgbevf_adapter *adapter = data;
	struct rnpgbevf_q_vector *q_vector = adapter->q_vector[0];
	struct rnpgbevf_hw *hw = &adapter->hw;

	/* handle data */
	if (q_vector->vector_flags & RNPVF_QVECTOR_FLAG_IRQ_MISS_CHECK)
		rnpgbevf_htimer_stop(q_vector);

	/*  disabled interrupts (on this vector) for us */
	rnpgbevf_irq_disable_queues(q_vector);

	if (q_vector->rx.ring || q_vector->tx.ring)
		napi_schedule_irqoff(&q_vector->napi);

	dbg("\n\n !!! %s irq-comming !!!\n", __func__);

	/* link is down by pf */
	if (test_bit(__RNPVF_MBX_POLLING, &adapter->state))
		goto WORK_DONE;
	if (!hw->mbx.ops.check_for_rst(hw, false)) {
		if (test_bit(__RNPVF_REMOVE, &adapter->state)) {
			printk(KERN_DEBUG "rnpvf is removed\n");
		}
	}
WORK_DONE:
	return IRQ_HANDLED;
}

void rnpgbevf_write_eitr_rx(struct rnpgbevf_q_vector *q_vector)
{
	struct rnpgbevf_adapter *adapter = q_vector->adapter;
	struct rnpgbevf_hw *hw = &adapter->hw;
	u32 new_itr_rx = q_vector->rx.itr;
	u32 old_itr_rx = q_vector->rx.itr;
	struct rnpgbevf_ring *ring;

	new_itr_rx = new_itr_rx * hw->usecstocount;
	rnpgbevf_for_each_ring(ring, q_vector->rx) {
		ring_wr32(ring, RNPGBE_DMA_REG_RX_INT_DELAY_TIMER, new_itr_rx);
		if (ring->ring_flags & RNPVF_RING_LOWER_ITR) {
			if (q_vector->itr_rx == RNP_LOWEREST_ITR)
				continue;
			ring_wr32(ring, RNPGBE_DMA_REG_RX_INT_DELAY_PKTCNT, 1);
			ring_wr32(ring, RNPGBE_DMA_REG_RX_INT_DELAY_TIMER,
				  RNP_LOWEREST_ITR);
			q_vector->itr_rx = RNP_LOWEREST_ITR;
		} else {
			if (new_itr_rx == q_vector->itr_rx)
				continue;
			ring_wr32(ring, RNPGBE_DMA_REG_RX_INT_DELAY_TIMER,
				  new_itr_rx);
			ring_wr32(ring, RNPGBE_DMA_REG_RX_INT_DELAY_PKTCNT,
				  adapter->rx_frames);
			q_vector->itr_rx = old_itr_rx;
		}
	}
}

static irqreturn_t rnpgbevf_msix_clean_rings(int irq, void *data)
{
	struct rnpgbevf_q_vector *q_vector = data;

	if (q_vector->vector_flags & RNPVF_QVECTOR_FLAG_IRQ_MISS_CHECK)
		rnpgbevf_htimer_stop(q_vector);
	/*  disabled interrupts (on this vector) for us */
	rnpgbevf_irq_disable_queues(q_vector);

	rnpgbevf_write_eitr_rx(q_vector);

	if (q_vector->rx.ring || q_vector->tx.ring)
		napi_schedule(&q_vector->napi);

	return IRQ_HANDLED;
}

void rnpgbevf_update_rx_count(int cleaned, struct rnpgbevf_q_vector *q_vector)
{
	struct rnpgbevf_adapter *adapter = q_vector->adapter;

	if ((cleaned) && (cleaned != q_vector->new_rx_count)) {
		if (cleaned < 5) {
			q_vector->small_times = 0;
			q_vector->large_times = 0;
			q_vector->too_small_times++;
			if (q_vector->too_small_times >= 2)
				q_vector->new_rx_count = 1;
		} else if (cleaned < 30) {
			q_vector->too_small_times = 0;
			q_vector->middle_time++;
			if (cleaned < q_vector->new_rx_count) {
				q_vector->small_times = 0;
				q_vector->new_rx_count -=
					(1 << (q_vector->large_times++));
				if (q_vector->new_rx_count < 0)
					q_vector->new_rx_count = 1;
			} else {
				q_vector->large_times = 0;

				if (cleaned > 30) {
					if (q_vector->new_rx_count ==
					    (cleaned - 4)) {
					} else {
						q_vector->new_rx_count +=
							(1
							 << (q_vector->small_times++));
					}
					if (q_vector->new_rx_count >= cleaned) {
						q_vector->new_rx_count =
							cleaned - 4;
						q_vector->small_times = 0;
					}

				} else {
					if (q_vector->new_rx_count ==
					    (cleaned - 1)) {
					} else {
						q_vector->new_rx_count +=
							(1
							 << (q_vector->small_times++));
					}
					if (q_vector->new_rx_count >= cleaned) {
						q_vector->new_rx_count =
							cleaned - 1;
						q_vector->small_times = 0;
					}
				}
			}
		} else {
			q_vector->too_small_times = 0;
			q_vector->new_rx_count =
				max_t(int, 64, adapter->rx_frames);
			q_vector->small_times = 0;
			q_vector->large_times = 0;
		}
	}
}

static void rnpgbevf_check_restart_tx(struct rnpgbevf_q_vector *q_vector,
				      struct rnpgbevf_ring *tx_ring)
{
	struct rnpgbevf_adapter *adapter = q_vector->adapter;
#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)
	if (likely(netif_carrier_ok(tx_ring->netdev) &&
		   (rnpgbevf_desc_unused(tx_ring) >= TX_WAKE_THRESHOLD))) {
		/* Make sure that anybody stopping the queue after this
		 * sees the new next_to_clean.
		 */
		smp_mb();
		if (__netif_subqueue_stopped(tx_ring->netdev,
					     tx_ring->queue_index) &&
		    !test_bit(__RNPVF_DOWN, &adapter->state)) {
			netif_wake_subqueue(tx_ring->netdev,
					    tx_ring->queue_index);
			++tx_ring->tx_stats.restart_queue;
		}
	}
}

__maybe_unused static void
rnpgbevf_update_ring_itr_rx(struct rnpgbevf_q_vector *q_vector)
{
	int new_val = q_vector->itr_rx;
	int avg_wire_size = 0;
	struct rnpgbevf_adapter *adapter = q_vector->adapter;
	unsigned int packets;

	/* For non-gigabit speeds, just fix the interrupt rate at 4000
         * ints/sec - ITR timer value of 120 ticks.
         */
	switch (adapter->link_speed) {
	case RNPGBE_LINK_SPEED_10_FULL:
	case RNPGBE_LINK_SPEED_100_FULL:
		new_val = RNP_4K_ITR;
		goto set_itr_val;
	default:
		break;
	}

	packets = q_vector->rx.total_packets;
	if (packets)
		avg_wire_size = max_t(u32, avg_wire_size,
				      q_vector->rx.total_bytes / packets);

	/* if avg_wire_size isn't set no work was done */
	if (!avg_wire_size)
		goto clear_counts;

	/* Add 24 bytes to size to account for CRC, preamble, and gap */
	avg_wire_size += 24;

	/* Don't starve jumbo frames */
	avg_wire_size = min(avg_wire_size, 3000);

	/* Give a little boost to mid-size frames */
	if ((avg_wire_size > 300) && (avg_wire_size < 1200))
		new_val = avg_wire_size / 3;
	else
		new_val = avg_wire_size / 2;

	new_val = new_val / 2;

	if (new_val < RNP_LOWEREST_ITR)
		new_val = RNP_LOWEREST_ITR;

set_itr_val:
	if (q_vector->rx.itr != new_val) {
		q_vector->rx.update_count++;
		if (q_vector->rx.update_count >= 2) {
			q_vector->rx.itr = new_val;
			q_vector->rx.update_count = 0;
		}
	} else
		q_vector->rx.update_count = 0;

clear_counts:
	q_vector->rx.total_bytes = 0;
	q_vector->rx.total_packets = 0;
}

/**
 * rnpgbevf_poll - NAPI polling calback
 * @napi: napi struct with our devices info in it
 * @budget: amount of work driver is allowed to do this pass, in packets
 *
 * This function will clean more than one or more rings associated with a
 * q_vector.
 **/
static int rnpgbevf_poll(struct napi_struct *napi, int budget)
{
	struct rnpgbevf_q_vector *q_vector =
		container_of(napi, struct rnpgbevf_q_vector, napi);
	struct rnpgbevf_adapter *adapter = q_vector->adapter;
	struct rnpgbevf_ring *ring;
	int per_ring_budget, work_done = 0;
	bool clean_complete = true;
	int cleaned_total = 0;

	rnpgbevf_for_each_ring(ring, q_vector->tx) clean_complete &=
		!!rnpgbevf_clean_tx_irq(q_vector, ring);

	/* attempt to distribute budget to each queue fairly, but don't allow
	 * the budget to go below 1 because we'll exit polling
	 */
	if (q_vector->rx.count > 1)
		per_ring_budget = max(budget / q_vector->rx.count, 1);
	else
		per_ring_budget = budget;

	rnpgbevf_for_each_ring(ring, q_vector->rx) {
		int cleaned = 0;

		cleaned =
			rnpgbevf_clean_rx_irq(q_vector, ring, per_ring_budget);

		work_done += cleaned;
		cleaned_total += cleaned;

		if (cleaned >= per_ring_budget)
			clean_complete = false;
	}

	/* force irq stop */
	if (test_bit(__RNPVF_DOWN, &adapter->state))
		clean_complete = true;

	if (!(q_vector->vector_flags & RNPVF_QVECTOR_FLAG_ITR_FEATURE))
		rnpgbevf_update_rx_count(cleaned_total, q_vector);

	/* If all work not completed, return budget and keep polling */
	if (!clean_complete)
		return budget;

	/* all work done, exit the polling mode */
	if (likely(napi_complete_done(napi, work_done))) {
		/* try to do itr handle */
		if (q_vector->vector_flags & RNPVF_QVECTOR_FLAG_ITR_FEATURE)
			rnpgbevf_update_ring_itr_rx(q_vector);

		if (!test_bit(__RNPVF_DOWN, &adapter->state)) {
			rnpgbevf_irq_enable_queues(q_vector);
			/* we need this */
			smp_mb();
			/* we need this to ensure irq start before tx start */
			if (q_vector->vector_flags &
			    RNPVF_QVECTOR_FLAG_REDUCE_TX_IRQ_MISS) {
				rnpgbevf_for_each_ring(ring, q_vector->tx) {
					rnpgbevf_check_restart_tx(q_vector,
								  ring);
					if (q_vector->new_rx_count !=
					    q_vector->old_rx_count) {
						ring_wr32(
							ring,
							RNPGBE_DMA_REG_RX_INT_DELAY_PKTCNT,
							q_vector->new_rx_count);
						q_vector->old_rx_count =
							q_vector->new_rx_count;
					}
				}
			}
		}
	}

	if (!test_bit(__RNPVF_DOWN, &adapter->state)) {
		if (q_vector->vector_flags & RNPVF_QVECTOR_FLAG_IRQ_MISS_CHECK)
			rnpgbevf_htimer_start(q_vector);
	}
	return 0;
}

/**
 * rnpgbevf_request_msix_irqs - Initialize MSI-X interrupts
 * @adapter: board private structure
 *
 * rnpgbevf_request_msix_irqs allocates MSI-X vectors and requests
 * interrupts from the kernel.
 **/
static int rnpgbevf_request_msix_irqs(struct rnpgbevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int err;
	int i = 0;

	DPRINTK(IFUP, INFO, "num_q_vectors:%d\n", adapter->num_q_vectors);

	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnpgbevf_q_vector *q_vector = adapter->q_vector[i];
		struct msix_entry *entry =
			&adapter->msix_entries[i + adapter->vector_off];

		if (q_vector->tx.ring && q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name) - 1,
				 "%s-%s-%d-%d", netdev->name, "TxRx", i,
				 q_vector->v_idx);
		} else {
			WARN(!(q_vector->tx.ring && q_vector->rx.ring),
			     "%s vector%d tx rx is null, v_idx:%d\n",
			     netdev->name, i, q_vector->v_idx);
			/* skip this unused q_vector */
			continue;
		}
		err = request_irq(entry->vector, &rnpgbevf_msix_clean_rings, 0,
				  q_vector->name, q_vector);
		if (err) {
			rnpgbevf_err(
				"%s:request_irq failed for MSIX interrupt:%d "
				"Error: %d\n",
				netdev->name, entry->vector, err);
			goto free_queue_irqs;
		}
		irq_set_affinity_hint(entry->vector, &q_vector->affinity_mask);
	}

	return 0;

free_queue_irqs:
	while (i) {
		i--;
		irq_set_affinity_hint(
			adapter->msix_entries[i + adapter->vector_off].vector,
			NULL);
		free_irq(adapter->msix_entries[i + adapter->vector_off].vector,
			 adapter->q_vector[i]);
	}
	return err;
}

static int rnpgbevf_free_msix_irqs(struct rnpgbevf_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnpgbevf_q_vector *q_vector = adapter->q_vector[i];
		struct msix_entry *entry =
			&adapter->msix_entries[i + adapter->vector_off];

		/* free only the irqs that were actually requested */
		if (!q_vector->rx.ring && !q_vector->tx.ring)
			continue;

		/* clear the affinity_mask in the IRQ descriptor */
		irq_set_affinity_hint(entry->vector, NULL);
		DPRINTK(IFDOWN, INFO, "free irq %s\n", q_vector->name);
		free_irq(entry->vector, q_vector);
	}

	return 0;
}

/**
 * rnpgbevf_request_irq - initialize interrupts
 * @adapter: board private structure
 *
 * Attempts to configure interrupts using the best available
 * capabilities of the hardware and kernel.
 **/
static int rnpgbevf_request_irq(struct rnpgbevf_adapter *adapter)
{
	int err;

	if (adapter->flags & RNPVF_FLAG_MSIX_ENABLED) {
		err = rnpgbevf_request_msix_irqs(adapter);
	} else if (adapter->flags & RNPVF_FLAG_MSI_ENABLED) {
		/* in this case one for all */
		err = request_irq(adapter->pdev->irq, rnpgbevf_intr, 0,
				  adapter->netdev->name, adapter);
	} else {
		err = request_irq(adapter->pdev->irq, rnpgbevf_intr,
				  IRQF_SHARED, adapter->netdev->name, adapter);
	}
	if (err)
		rnpgbevf_err("request_irq failed, Error %d\n", err);

	return err;
}

static void rnpgbevf_free_irq(struct rnpgbevf_adapter *adapter)
{
	if (adapter->flags & RNPVF_FLAG_MSIX_ENABLED) {
		rnpgbevf_free_msix_irqs(adapter);
	} else if (adapter->flags & RNPVF_FLAG_MSI_ENABLED) {
		/* in this case one for all */
		free_irq(adapter->pdev->irq, adapter);
	} else {
		free_irq(adapter->pdev->irq, adapter);
	}
}

/**
 * rnpgbevf_irq_disable - Mask off interrupt generation on the NIC
 * @adapter: board private structure
 **/
static inline void rnpgbevf_irq_disable(struct rnpgbevf_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		rnpgbevf_irq_disable_queues(adapter->q_vector[i]);
		if (adapter->flags & RNPVF_FLAG_MSIX_ENABLED) {
			synchronize_irq(
				adapter->msix_entries[i + adapter->vector_off]
					.vector);
		} else {
			synchronize_irq(adapter->pdev->irq);
		}
	}
}

/**
 * rnpgbevf_configure_tx_ring - Configure 8259x Tx ring after Reset
 * @adapter: board private structure
 * @ring: structure containing ring specific data
 *
 * Configure the Tx descriptor ring after a reset.
 **/
void rnpgbevf_configure_tx_ring(struct rnpgbevf_adapter *adapter,
				struct rnpgbevf_ring *ring)
{
	struct rnpgbevf_hw *hw = &adapter->hw;

	/* disable queue to avoid issues while updating state */
	if (!(ring->ring_flags & RNPVF_RING_SKIP_TX_START))
		ring_wr32(ring, RNPGBE_DMA_TX_START, 0);

	ring_wr32(ring, RNPGBE_DMA_REG_TX_DESC_BUF_BASE_ADDR_LO,
		  (u32)ring->dma);
	/* dma high address is used for vfnum */
	ring_wr32(ring, RNPGBE_DMA_REG_TX_DESC_BUF_BASE_ADDR_HI,
		  (u32)(((u64)ring->dma) >> 32) | (hw->vfnum << 24));
	ring_wr32(ring, RNPGBE_DMA_REG_TX_DESC_BUF_LEN, ring->count);

	ring->next_to_clean = ring_rd32(ring, RNPGBE_DMA_REG_TX_DESC_BUF_HEAD);
	ring->next_to_use = ring->next_to_clean;
	ring->tail = ring->ring_addr + RNPGBE_DMA_REG_TX_DESC_BUF_TAIL;
	rnpgbevf_wr_reg(ring->tail, ring->next_to_use);

	ring_wr32(ring, RNPGBE_DMA_REG_TX_DESC_FETCH_CTRL,
		  (8 << 0) /*max_water_flow*/
			  | (TSRN10_TX_DEFAULT_BURST
			     << 16)); /* max-num_descs_peer_read */

	ring_wr32(ring, RNPGBE_DMA_REG_TX_INT_DELAY_TIMER,
		  adapter->tx_usecs * hw->usecstocount);
	ring_wr32(ring, RNPGBE_DMA_REG_TX_INT_DELAY_PKTCNT,
		  adapter->tx_frames);

	if (!(ring->ring_flags & RNPVF_RING_SKIP_TX_START)) {
		/* n500 should wait tx_ready before open tx start */
		int timeout = 0;
		u32 status = 0;

		do {
			status = ring_rd32(ring, RNPGBE_DMA_TX_READY);
			usleep_range(100, 200);
			timeout++;
			rnpgbevf_dbg("wait %d tx ready to 1\n",
				     ring->rnpgbevf_queue_idx);
		} while ((status != 1) && (timeout < 100));

		if (timeout >= 100)
			printk("wait tx ready timeout\n");
		ring_wr32(ring, RNPGBE_DMA_TX_START, 1);
	}
}

/**
 * rnpgbevf_configure_tx - Configure 82599 VF Transmit Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Tx unit of the MAC after a reset.
 **/
static void rnpgbevf_configure_tx(struct rnpgbevf_adapter *adapter)
{
	u32 i;

	/* Setup the HW Tx Head and Tail descriptor pointers */
	for (i = 0; i < (adapter->num_tx_queues); i++)
		rnpgbevf_configure_tx_ring(adapter, adapter->tx_ring[i]);
}

#define RNPGBE_SRRCTL_BSIZEHDRSIZE_SHIFT 2

void rnpgbevf_disable_rx_queue(struct rnpgbevf_adapter *adapter,
			       struct rnpgbevf_ring *ring)
{
	ring_wr32(ring, RNPGBE_DMA_RX_START, 0);
}

void rnpgbevf_enable_rx_queue(struct rnpgbevf_adapter *adapter,
			      struct rnpgbevf_ring *ring)
{
	ring_wr32(ring, RNPGBE_DMA_RX_START, 1);
}

void rnpgbevf_configure_rx_ring(struct rnpgbevf_adapter *adapter,
				struct rnpgbevf_ring *ring)
{
	struct rnpgbevf_hw *hw = &adapter->hw;
	u64 desc_phy = ring->dma;

	/* disable queue to avoid issues while updating state */
	rnpgbevf_disable_rx_queue(adapter, ring);

	/* set descripts registers*/
	ring_wr32(ring, RNPGBE_DMA_REG_RX_DESC_BUF_BASE_ADDR_LO, (u32)desc_phy);
	/* dma address high bits is used */
	ring_wr32(ring, RNPGBE_DMA_REG_RX_DESC_BUF_BASE_ADDR_HI,
		  ((u32)(desc_phy >> 32)) | (hw->vfnum << 24));
	ring_wr32(ring, RNPGBE_DMA_REG_RX_DESC_BUF_LEN, ring->count);

	ring->tail = ring->ring_addr + RNPGBE_DMA_REG_RX_DESC_BUF_TAIL;
	ring->next_to_clean = ring_rd32(ring, RNPGBE_DMA_REG_RX_DESC_BUF_HEAD);
	ring->next_to_use = ring->next_to_clean;

#define SCATER_SIZE (96)
	if (ring->ring_flags & RNPVF_RING_SCATER_SETUP) {
		ring_wr32(ring, PCI_DMA_REG_RX_SCATTER_LENGH, SCATER_SIZE);
	}

	ring_wr32(ring, RNPGBE_DMA_REG_RX_DESC_FETCH_CTRL,
		  0 | (TSRN10_RX_DEFAULT_LINE << 0) /*rx-desc-flow*/
			  |
			  (TSRN10_RX_DEFAULT_BURST << 16) /*max-read-desc-cnt*/
	);

	ring_wr32(ring, RNPGBE_DMA_REG_RX_INT_DELAY_TIMER,
		  adapter->rx_usecs * hw->usecstocount);
	ring_wr32(ring, RNPGBE_DMA_REG_RX_INT_DELAY_PKTCNT, adapter->rx_frames);
	rnpgbevf_alloc_rx_buffers(ring, rnpgbevf_desc_unused(ring));
}

static void rnpgbevf_set_rx_buffer_len(struct rnpgbevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int max_frame = netdev->mtu + ETH_HLEN + ETH_FCS_LEN * 3;
	struct rnpgbevf_ring *rx_ring;
	int i;

	if (max_frame < (ETH_FRAME_LEN + ETH_FCS_LEN))
		max_frame = (ETH_FRAME_LEN + ETH_FCS_LEN);

	for (i = 0; i < adapter->num_rx_queues; i++) {
		rx_ring = adapter->rx_ring[i];
		clear_bit(__RNPVF_RX_3K_BUFFER, &rx_ring->state);
		clear_bit(__RNPVF_RX_BUILD_SKB_ENABLED, &rx_ring->state);
		set_bit(__RNPVF_RX_BUILD_SKB_ENABLED, &rx_ring->state);

#ifdef OPTM_WITH_LPAGE
		rx_ring->rx_page_buf_nums = RNPVF_PAGE_BUFFER_NUMS(rx_ring);
		rx_ring->rx_per_buf_mem = RNPVF_RXBUFFER_2K;
#endif
	}
}

/**
 * rnpgbevf_configure_rx - Configure 82599 VF Receive Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Rx unit of the MAC after a reset.
 **/
static void rnpgbevf_configure_rx(struct rnpgbevf_adapter *adapter)
{
	int i;

	/* set_rx_buffer_len must be called before ring initialization */
	rnpgbevf_set_rx_buffer_len(adapter);

	/*
	 * Setup the HW Rx Head and Tail Descriptor Pointers and
	 * the Base and Length of the Rx Descriptor Ring
	 */
	for (i = 0; i < adapter->num_rx_queues; i++)
		rnpgbevf_configure_rx_ring(adapter, adapter->rx_ring[i]);
}

static int rnpgbevf_vlan_rx_add_vid(struct net_device *netdev,
				    __always_unused __be16 proto, u16 vid)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct rnpgbevf_hw *hw = &adapter->hw;
	struct rnp_mbx_info *mbx = &hw->mbx;
	int err = 0;

	if ((vid) && (adapter->vf_vlan) && (vid != adapter->vf_vlan)) {
		dev_err(&adapter->pdev->dev,
			"only 1 vlan for vf or pf set vlan already\n");
		return 0;
	}

	if ((vid) && (!adapter->vf_vlan)) {
		spin_lock_bh(&adapter->mbx_lock);
		set_bit(__RNPVF_MBX_POLLING, &adapter->state);
		/* add VID to filter table */
		err = hw->mac.ops.set_vfta(hw, vid, 0, true);
		clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
		spin_unlock_bh(&adapter->mbx_lock);
	}

	/* translate error return types so error makes sense */
	if (err == RNPGBE_ERR_MBX) {

		return -EIO;
	}

	if (err == RNPGBE_ERR_INVALID_ARGUMENT) {
		return -EACCES;
	}
	set_bit(vid, adapter->active_vlans);

	if (vid)
		hw->ops.set_veb_vlan(hw, vid, VFNUM(mbx, hw->vfnum));

#ifdef CONFIG_NET_NCSI
	if (adapter->ncsi_dev)
		ncsi_vlan_rx_add_vid(netdev, proto, vid);
#endif

	return err;
}

static int rnpgbevf_vlan_rx_kill_vid(struct net_device *netdev,
				     __always_unused __be16 proto, u16 vid)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct rnpgbevf_hw *hw = &adapter->hw;
	struct rnp_mbx_info *mbx = &hw->mbx;
	int err = -EOPNOTSUPP;

	if (!test_bit(__RNPVF_DOWN, &adapter->state))
		rnpgbevf_irq_disable(adapter);

	if (vid) {
		spin_lock_bh(&adapter->mbx_lock);
		set_bit(__RNPVF_MBX_POLLING, &adapter->state);
		/* remove VID from filter table */
		err = hw->mac.ops.set_vfta(hw, vid, 0, false);
		clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
		spin_unlock_bh(&adapter->mbx_lock);
		hw->ops.set_veb_vlan(hw, 0, VFNUM(mbx, hw->vfnum));
	}

	clear_bit(vid, adapter->active_vlans);

#ifdef CONFIG_NET_NCSI
	if (adapter->ncsi_dev)
		ncsi_vlan_rx_kill_vid(netdev, proto, vid);
#endif /* CONFIG_NET_NCSI */

	return 0;
}

/**
 * rnpgbevf_vlan_strip_disable - helper to disable hw vlan stripping
 * @adapter: driver data
 */
__maybe_unused static void
rnpgbevf_vlan_strip_disable(struct rnpgbevf_adapter *adapter)
{
	struct rnpgbevf_hw *hw = &adapter->hw;

	spin_lock_bh(&adapter->mbx_lock);
	set_bit(__RNPVF_MBX_POLLING, &adapter->state);
	hw->mac.ops.set_vlan_strip(hw, false);
	clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
	spin_unlock_bh(&adapter->mbx_lock);
}

/**
 * rnpgbevf_vlan_strip_enable - helper to enable hw vlan stripping
 * @adapter: driver data
 */
__maybe_unused static s32
rnpgbevf_vlan_strip_enable(struct rnpgbevf_adapter *adapter)
{
	struct rnpgbevf_hw *hw = &adapter->hw;
	int err;

	spin_lock_bh(&adapter->mbx_lock);
	set_bit(__RNPVF_MBX_POLLING, &adapter->state);
	err = hw->mac.ops.set_vlan_strip(hw, true);
	clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
	spin_unlock_bh(&adapter->mbx_lock);

	return err;
}

static void rnpgbevf_restore_vlan(struct rnpgbevf_adapter *adapter)
{
	u16 vid;

	rnpgbevf_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q), 0);

	for_each_set_bit (vid, adapter->active_vlans, VLAN_N_VID) {
		rnpgbevf_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q),
					 vid);
	}
}

static int rnpgbevf_write_uc_addr_list(struct net_device *netdev)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct rnpgbevf_hw *hw = &adapter->hw;
	int count = 0;

	if ((netdev_uc_count(netdev)) > 10) {
		pr_err("Too many unicast filters - No Space\n");
		return -ENOSPC;
	}

	if (!netdev_uc_empty(netdev)) {
		struct netdev_hw_addr *ha;

		netdev_for_each_uc_addr (ha, netdev) {
			spin_lock_bh(&adapter->mbx_lock);
			set_bit(__RNPVF_MBX_POLLING, &adapter->state);
			hw->mac.ops.set_uc_addr(hw, ++count, ha->addr);
			clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
			spin_unlock_bh(&adapter->mbx_lock);
			udelay(200);
		}
	} else {
		/*
		 * If the list is empty then send message to PF driver to
		 * clear all macvlans on this VF.
		 */
		spin_lock_bh(&adapter->mbx_lock);
		set_bit(__RNPVF_MBX_POLLING, &adapter->state);
		hw->mac.ops.set_uc_addr(hw, 0, NULL);
		clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
		spin_unlock_bh(&adapter->mbx_lock);
		udelay(200);
	}

	return count;
}

/**
 * rnpgbevf_set_rx_mode - Multicast and unicast set
 * @netdev: network interface device structure
 *
 * The set_rx_method entry point is called whenever the multicast address
 * list, unicast address list or the network interface flags are updated.
 * This routine is responsible for configuring the hardware for proper
 * multicast mode and configuring requested unicast filters.
 **/
static void rnpgbevf_set_rx_mode(struct net_device *netdev)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct rnpgbevf_hw *hw = &adapter->hw;

	netdev_features_t features = netdev->features;
	spin_lock_bh(&adapter->mbx_lock);
	set_bit(__RNPVF_MBX_POLLING, &adapter->state);
	/* reprogram multicast list */
	hw->mac.ops.update_mc_addr_list(hw, netdev);
	clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
	spin_unlock_bh(&adapter->mbx_lock);

	rnpgbevf_write_uc_addr_list(netdev);

	if (features & NETIF_F_HW_VLAN_CTAG_RX)
		rnpgbevf_vlan_strip_enable(adapter);
	else
		rnpgbevf_vlan_strip_disable(adapter);

	/* stags */
	if ((features & NETIF_F_HW_VLAN_STAG_RX) ||
	    (adapter->flags & RNPVF_FLAG_PF_SET_VLAN))
		rnpgbevf_vlan_strip_enable(adapter);
	else
		rnpgbevf_vlan_strip_disable(adapter);
}

static void rnpgbevf_napi_enable_all(struct rnpgbevf_adapter *adapter)
{
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++)
		napi_enable(&adapter->q_vector[q_idx]->napi);
}

static void rnpgbevf_napi_disable_all(struct rnpgbevf_adapter *adapter)
{
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++)
		napi_disable(&adapter->q_vector[q_idx]->napi);
}

static void rnpgbevf_configure_veb(struct rnpgbevf_adapter *adapter)
{
	struct rnpgbevf_hw *hw = &adapter->hw;
	struct rnp_mbx_info *mbx = &hw->mbx;
	u8 vfnum = VFNUM(mbx, hw->vfnum);
	u32 ring;
	u8 *mac;

	if (is_valid_ether_addr(hw->mac.addr))
		mac = hw->mac.addr;
	else
		mac = hw->mac.perm_addr;

	ring = adapter->rx_ring[0]->rnpgbevf_queue_idx;
	ring |= ((0x80 | vfnum) << 8);

	hw->ops.set_veb_mac(hw, mac, vfnum, ring);
}

static void rnpgbevf_configure(struct rnpgbevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	rnpgbevf_set_rx_mode(netdev);
	rnpgbevf_restore_vlan(adapter);
	rnpgbevf_configure_tx(adapter);
	rnpgbevf_configure_rx(adapter);
	rnpgbevf_configure_veb(adapter);
}

#define RNPGBE_MAX_RX_DESC_POLL 10

static void rnpgbevf_save_reset_stats(struct rnpgbevf_adapter *adapter)
{
	/* Only save pre-reset stats if there are some */
	if (adapter->stats.vfgprc || adapter->stats.vfgptc) {
		adapter->stats.saved_reset_vfgprc +=
			adapter->stats.vfgprc - adapter->stats.base_vfgprc;
		adapter->stats.saved_reset_vfgptc +=
			adapter->stats.vfgptc - adapter->stats.base_vfgptc;
		adapter->stats.saved_reset_vfgorc +=
			adapter->stats.vfgorc - adapter->stats.base_vfgorc;
		adapter->stats.saved_reset_vfgotc +=
			adapter->stats.vfgotc - adapter->stats.base_vfgotc;
		adapter->stats.saved_reset_vfmprc +=
			adapter->stats.vfmprc - adapter->stats.base_vfmprc;
	}
}

static void rnpgbevf_up_complete(struct rnpgbevf_adapter *adapter)
{
	struct rnpgbevf_hw *hw = &adapter->hw;
	int i;

	rnpgbevf_configure_msix(adapter);

	spin_lock_bh(&adapter->mbx_lock);
	set_bit(__RNPVF_MBX_POLLING, &adapter->state);

	if (is_valid_ether_addr(hw->mac.addr))
		hw->mac.ops.set_rar(hw, 0, hw->mac.addr, 0);
	else
		hw->mac.ops.set_rar(hw, 0, hw->mac.perm_addr, 0);

	clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
	spin_unlock_bh(&adapter->mbx_lock);

	rnpgbevf_napi_enable_all(adapter);

	/* clear any pending interrupts */
	rnpgbevf_irq_enable(adapter);

	/* enable transmits */
	netif_tx_start_all_queues(adapter->netdev);

	rnpgbevf_save_reset_stats(adapter);

	hw->mac.get_link_status = 1;
	mod_timer(&adapter->watchdog_timer, jiffies);

	clear_bit(__RNPVF_DOWN, &adapter->state);
	for (i = 0; i < adapter->num_rx_queues; i++)
		rnpgbevf_enable_rx_queue(adapter, adapter->rx_ring[i]);
}

void rnpgbevf_reinit_locked(struct rnpgbevf_adapter *adapter)
{
	WARN_ON(in_interrupt());

	while (test_and_set_bit(__RNPVF_RESETTING, &adapter->state))
		usleep_range(1000, 2000);

	rnpgbevf_down(adapter);

	rnpgbevf_reset(adapter);

	rnpgbevf_up(adapter);

	clear_bit(__RNPVF_RESETTING, &adapter->state);
}

void rnpgbevf_up(struct rnpgbevf_adapter *adapter)
{
	rnpgbevf_configure(adapter);

	rnpgbevf_up_complete(adapter);
}

void rnpgbevf_reset(struct rnpgbevf_adapter *adapter)
{
	struct rnpgbevf_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;

	set_bit(__RNPVF_MBX_POLLING, &adapter->state);
	if (hw->mac.ops.reset_hw(hw))
		hw_dbg(hw, "PF still resetting\n");
	else
		hw->mac.ops.init_hw(hw);

	clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
	if (is_valid_ether_addr(adapter->hw.mac.addr)) {
		eth_hw_addr_set(netdev, adapter->hw.mac.addr);
		memcpy(netdev->perm_addr, adapter->hw.mac.addr,
		       netdev->addr_len);
	}
}

/**
 * rnpgbevf_clean_tx_ring - Free Tx Buffers
 * @adapter: board private structure
 * @tx_ring: ring to be cleaned
 **/
static void rnpgbevf_clean_tx_ring(struct rnpgbevf_adapter *adapter,
				   struct rnpgbevf_ring *tx_ring)
{
	struct rnpgbevf_tx_buffer *tx_buffer_info;
	unsigned long size;
	u16 i;

	BUG_ON(tx_ring == NULL);

	/* ring already cleared, nothing to do */
	if (!tx_ring->tx_buffer_info)
		return;

	/* Free all the Tx ring sk_buffs */
	for (i = 0; i < tx_ring->count; i++) {
		tx_buffer_info = &tx_ring->tx_buffer_info[i];
		rnpgbevf_unmap_and_free_tx_resource(tx_ring, tx_buffer_info);
	}

	netdev_tx_reset_queue(txring_txq(tx_ring));

	size = sizeof(struct rnpgbevf_tx_buffer) * tx_ring->count;
	memset(tx_ring->tx_buffer_info, 0, size);

	/* Zero out the descriptor ring */
	memset(tx_ring->desc, 0, tx_ring->size);

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;
}

/**
 * rnpgbevf_clean_all_rx_rings - Free Rx Buffers for all queues
 * @adapter: board private structure
 **/
static void rnpgbevf_clean_all_rx_rings(struct rnpgbevf_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		rnpgbevf_clean_rx_ring(adapter->rx_ring[i]);
}

/**
 * rnpgbevf_clean_all_tx_rings - Free Tx Buffers for all queues
 * @adapter: board private structure
 **/
static void rnpgbevf_clean_all_tx_rings(struct rnpgbevf_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		rnpgbevf_clean_tx_ring(adapter, adapter->tx_ring[i]);
}

void rnpgbevf_down(struct rnpgbevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int i;

	/* signal that we are down to the interrupt handler */
	set_bit(__RNPVF_DOWN, &adapter->state);
	set_bit(__RNPVF_LINK_DOWN, &adapter->state);

	/* disable all enabled rx queues */
	for (i = 0; i < adapter->num_rx_queues; i++)
		rnpgbevf_disable_rx_queue(adapter, adapter->rx_ring[i]);

	usleep_range(1000, 2000);

	netif_tx_stop_all_queues(netdev);

	/* call carrier off first to avoid false dev_watchdog timeouts */
	netif_carrier_off(netdev);

	netif_tx_disable(netdev);

	rnpgbevf_irq_disable(adapter);

	rnpgbevf_napi_disable_all(adapter);

	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct rnpgbevf_ring *tx_ring = adapter->tx_ring[i];

		if (!(tx_ring->ring_flags & RNPVF_RING_SKIP_TX_START)) {
			int head, tail;
			int timeout = 0;

			head = ring_rd32(tx_ring,
					 RNPGBE_DMA_REG_TX_DESC_BUF_HEAD);
			tail = ring_rd32(tx_ring,
					 RNPGBE_DMA_REG_TX_DESC_BUF_TAIL);

			while (head != tail) {
				usleep_range(10000, 20000);

				head = ring_rd32(
					tx_ring,
					RNPGBE_DMA_REG_TX_DESC_BUF_HEAD);
				tail = ring_rd32(
					tx_ring,
					RNPGBE_DMA_REG_TX_DESC_BUF_TAIL);
				timeout++;
				if (timeout >= 100) {
					printk(KERN_DEBUG
					       "vf wait tx done timeout\n");
					break;
				}
			}
		}
	}

	/* disable transmits in the hardware now that interrupts are off */
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct rnpgbevf_ring *tx_ring = adapter->tx_ring[i];

		if (!(tx_ring->ring_flags & RNPVF_RING_SKIP_TX_START))
			ring_wr32(tx_ring, RNPGBE_DMA_TX_START, 0);
	}

	netif_carrier_off(netdev);
	rnpgbevf_clean_all_tx_rings(adapter);
	rnpgbevf_clean_all_rx_rings(adapter);
}

static netdev_features_t rnpgbevf_fix_features(struct net_device *netdev,
					       netdev_features_t features)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct rnpgbevf_hw *hw = &adapter->hw;

	/* If Rx checksum is disabled, then RSC/LRO should also be disabled */
	if (!(features & NETIF_F_RXCSUM)) {
		features &= ~NETIF_F_LRO;
		adapter->flags &= (~RNPVF_FLAG_RX_CHKSUM_ENABLED);
	} else
		adapter->flags |= RNPVF_FLAG_RX_CHKSUM_ENABLED;

	/* vf not support change vlan filter */
	if ((netdev->features & NETIF_F_HW_VLAN_CTAG_FILTER) !=
	    (features & NETIF_F_HW_VLAN_CTAG_FILTER)) {
		if (netdev->features & NETIF_F_HW_VLAN_CTAG_FILTER)
			features |= NETIF_F_HW_VLAN_CTAG_FILTER;
		else
			features &= ~NETIF_F_HW_VLAN_CTAG_FILTER;
	}

	if ((netdev->features & NETIF_F_HW_VLAN_STAG_FILTER) !=
	    (features & NETIF_F_HW_VLAN_STAG_FILTER)) {
		if (netdev->features & NETIF_F_HW_VLAN_STAG_FILTER)
			features |= NETIF_F_HW_VLAN_STAG_FILTER;
		else
			features &= ~NETIF_F_HW_VLAN_STAG_FILTER;
	}

	if (adapter->flags & RNPVF_FLAG_PF_SET_VLAN) {
		/* if in this mode , close tx/rx vlan offload */
		if (features & NETIF_F_HW_VLAN_CTAG_RX)
			adapter->priv_flags |= RNPVF_FLAG_RX_CVLAN_OFFLOAD;
		else
			adapter->priv_flags &= ~RNPVF_FLAG_RX_CVLAN_OFFLOAD;

		if (!(hw->pf_feature & PF_NCSI_EN))
			features |= NETIF_F_HW_VLAN_CTAG_RX;
		if (features & NETIF_F_HW_VLAN_CTAG_TX)
			adapter->priv_flags |= RNPVF_FLAG_TX_CVLAN_OFFLOAD;
		else
			adapter->priv_flags &= ~RNPVF_FLAG_TX_CVLAN_OFFLOAD;

		features &= ~NETIF_F_HW_VLAN_CTAG_TX;

		if (features & NETIF_F_HW_VLAN_STAG_RX)
			adapter->priv_flags |= RNPVF_FLAG_RX_SVLAN_OFFLOAD;
		else
			adapter->priv_flags &= ~RNPVF_FLAG_RX_SVLAN_OFFLOAD;

		if (!(hw->pf_feature & PF_NCSI_EN))
			features |= NETIF_F_HW_VLAN_STAG_RX;
		if (features & NETIF_F_HW_VLAN_STAG_TX)
			adapter->priv_flags |= RNPVF_FLAG_TX_SVLAN_OFFLOAD;
		else
			adapter->priv_flags &= ~RNPVF_FLAG_TX_SVLAN_OFFLOAD;

		features &= ~NETIF_F_HW_VLAN_STAG_TX;

	} else {
		if (!(features & NETIF_F_HW_VLAN_CTAG_RX)) {
			if (hw->feature_flags & RNPVF_NET_FEATURE_STAG_OFFLOAD)
				features &= ~NETIF_F_HW_VLAN_STAG_RX;
		}

		if (hw->feature_flags & RNPVF_NET_FEATURE_STAG_OFFLOAD) {
			if (!(features & NETIF_F_HW_VLAN_STAG_RX)) {
				features &= ~NETIF_F_HW_VLAN_CTAG_RX;
			}
		}

		if (!(features & NETIF_F_HW_VLAN_CTAG_TX)) {
			if (hw->feature_flags & RNPVF_NET_FEATURE_STAG_OFFLOAD)
				features &= ~NETIF_F_HW_VLAN_STAG_TX;
		}

		if (hw->feature_flags & RNPVF_NET_FEATURE_STAG_OFFLOAD) {
			if (!(features & NETIF_F_HW_VLAN_STAG_TX)) {
				features &= ~NETIF_F_HW_VLAN_CTAG_TX;
			}
		}
	}
	return features;
}

static int rnpgbevf_set_features(struct net_device *netdev,
				 netdev_features_t features)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	netdev_features_t changed = netdev->features ^ features;
	bool need_reset = false;
	int err = 0;

	netdev->features = features;
	if (changed & NETIF_F_HW_VLAN_CTAG_RX) {
		if (features & NETIF_F_HW_VLAN_CTAG_RX) {
			if ((!rnpgbevf_vlan_strip_enable(adapter)))
				features &= ~NETIF_F_HW_VLAN_CTAG_RX;
		} else {
			rnpgbevf_vlan_strip_disable(adapter);
		}
	}

	netdev->features = features;

	if (need_reset)
		rnpgbevf_reset(adapter);

	return err;
}

/**
 * rnpgbevf_tx_timeout - Respond to a Tx Hang
 * @netdev: network interface device structure
 **/
__maybe_unused static void rnpgbevf_tx_timeout(struct net_device *netdev)
{
	/* Do the reset outside of interrupt context */
}

/**
 * rnpgbevf_sw_init - Initialize general software structures
 * (struct rnpgbevf_adapter)
 * @adapter: board private structure to initialize
 *
 * rnpgbevf_sw_init initializes the Adapter private data structure.
 * Fields are initialized based on PCI device information and
 * OS network device settings (MTU size).
 **/
static int rnpgbevf_sw_init(struct rnpgbevf_adapter *adapter)
{
	struct rnpgbevf_hw *hw = &adapter->hw;
	struct pci_dev *pdev = adapter->pdev;
	struct net_device *netdev = adapter->netdev;
	int err;

	/* PCI config space info */
	hw->pdev = pdev;

	hw->vendor_id = pdev->vendor;
	hw->device_id = pdev->device;
	hw->subsystem_vendor_id = pdev->subsystem_vendor;
	hw->subsystem_device_id = pdev->subsystem_device;

	hw->mbx.ops.init_params(hw);

	/*initialization default pause flow */
	hw->fc.requested_mode = rnp_fc_none;
	hw->fc.current_mode = rnp_fc_none;

	/* now vf other irq handler is not regist */
	err = hw->mac.ops.reset_hw(hw);
	if (err) {
		dev_info(&pdev->dev,
			 "PF still in reset state.  Is the PF interface up?\n");
		hw->adapter_stopped = false;
		hw->link = false;
		hw->speed = 0;
		hw->usecstocount = 500;
		return err;
	}
	err = hw->mac.ops.init_hw(hw);
	if (err) {
		pr_err("init_shared_code failed: %d\n", err);
		goto out;
	}
	err = hw->mac.ops.get_mac_addr(hw, hw->mac.addr);
	if (err)
		dev_info(&pdev->dev, "Error reading MAC address\n");
	else if (is_zero_ether_addr(adapter->hw.mac.addr))
		dev_info(&pdev->dev,
			 "MAC address not assigned by administrator.\n");
	eth_hw_addr_set(netdev, hw->mac.addr);

	if (!is_valid_ether_addr(netdev->dev_addr)) {
		dev_info(&pdev->dev, "Assigning random MAC address\n");
		eth_hw_addr_random(netdev);
		memcpy(hw->mac.addr, netdev->dev_addr, netdev->addr_len);
	}
	/* get info from pf */
	err = hw->mac.ops.get_queues(hw);
	if (err) {
		dev_info(&pdev->dev, "Get queue info error, use default one\n");
		hw->mac.max_tx_queues = MAX_TX_QUEUES;
		hw->mac.max_rx_queues = MAX_RX_QUEUES;
		hw->queue_ring_base = (hw->vfnum & VF_NUM_MASK) * MAX_RX_QUEUES;
	}

	dev_info(&pdev->dev, "queue_ring_base %d num %d\n", hw->queue_ring_base,
		 hw->mac.max_tx_queues);
	err = hw->mac.ops.get_mtu(hw);
	if (err) {
		dev_info(&pdev->dev, "Get mtu error ,use default one\n");
		hw->mtu = 1500;
	}
	/* lock to protect mailbox accesses */
	spin_lock_init(&adapter->mbx_lock);

	/* Enable dynamic interrupt throttling rates */
	/* set default ring sizes */
	adapter->tx_ring_item_count = hw->tx_items_count;
	adapter->rx_ring_item_count = hw->rx_items_count;
	adapter->dma_channels =
		min_t(int, hw->mac.max_tx_queues, hw->mac.max_rx_queues);
	DPRINTK(PROBE, INFO, "tx parameters %d, rx parameters %d\n",
		adapter->tx_ring_item_count, adapter->rx_ring_item_count);

	/* set default tx/rx soft count */
	adapter->adaptive_rx_coal = 1;
	adapter->adaptive_tx_coal = 1;
	adapter->napi_budge = RNPVF_DEFAULT_RX_WORK;
	adapter->tx_work_limit = RNPVF_DEFAULT_TX_WORK;
	adapter->rx_usecs = RNPVF_PKT_TIMEOUT;
	adapter->rx_frames = RNPVF_RX_PKT_POLL_BUDGET;
	adapter->tx_usecs = RNPVF_PKT_TIMEOUT_TX;
	adapter->tx_frames = RNPVF_TX_PKT_POLL_BUDGET;

	set_bit(__RNPVF_DOWN, &adapter->state);
	return 0;

out:
	return err;
}

static int rnpgbevf_acquire_msix_vectors(struct rnpgbevf_adapter *adapter,
					 int vectors)
{
	int err = 0;
	int vector_threshold;

	vector_threshold = MIN_MSIX_COUNT;

	/* The more we get, the more we will assign to Tx/Rx Cleanup
	 * for the separate queues...where Rx Cleanup >= Tx Cleanup.
	 * Right now, we simply care about how many we'll get; we'll
	 * set them up later while requesting irq's.
	 */
	err = pci_enable_msix_range(adapter->pdev, adapter->msix_entries,
				    vectors, vectors);
	if (err > 0) { /* Success or a nasty failure. */
		vectors = err;
		err = 0;
	}
	DPRINTK(PROBE, INFO, "err:%d, vectors:%d\n", err, vectors);
	if (err < 0) {
		dev_err(&adapter->pdev->dev,
			"Unable to allocate MSI-X interrupts\n");
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;
	} else {
		/*
		 * Adjust for only the vectors we'll use, which is minimum
		 * of max_msix_q_vectors + NON_Q_VECTORS, or the number of
		 * vectors we were allocated.
		 */
		adapter->num_msix_vectors = vectors;
	}

	return err;
}

/**
 * rnpgbevf_set_num_queues - Allocate queues for device, feature dependent
 * @adapter: board private structure to initialize
 *
 * This is the top level queue allocation routine.  The order here is very
 * important, starting with the "most" number of features turned on at once,
 * and ending with the smallest set of features.  This way large combinations
 * can be allocated if they're turned on, and smaller combinations are the
 * fallthrough conditions.
 *
 **/
static void rnpgbevf_set_num_queues(struct rnpgbevf_adapter *adapter)
{
	/* Start with base case */
	adapter->num_rx_queues = adapter->dma_channels;
	adapter->num_tx_queues = adapter->dma_channels;
}

/**
 * rnpgbevf_set_interrupt_capability - set MSI-X or FAIL if not supported
 * @adapter: board private structure to initialize
 *
 * Attempt to configure the interrupts using the best available
 * capabilities of the hardware and the kernel.
 **/
static int rnpgbevf_set_interrupt_capability(struct rnpgbevf_adapter *adapter)
{
	int err = 0;
	int vector, v_budget;
	int irq_mode_back = adapter->irq_mode;
	/*
	 * It's easy to be greedy for MSI-X vectors, but it really
	 * doesn't do us much good if we have a lot more vectors
	 * than CPU's.  So let's be conservative and only ask for
	 * (roughly) the same number of vectors as there are CPU's.
	 * The default is to use pairs of vectors.
	 */
	v_budget = max(adapter->num_rx_queues, adapter->num_tx_queues);
	v_budget = min_t(int, v_budget, num_online_cpus());
	v_budget += NON_Q_VECTORS;
	v_budget = min_t(int, v_budget, MAX_MSIX_VECTORS);

	if (adapter->irq_mode == irq_mode_msix) {
		/* A failure in MSI-X entry allocation isn't fatal, but it does
		 * mean we disable MSI-X capabilities of the adapter.
		 */
		adapter->msix_entries = kcalloc(
			v_budget, sizeof(struct msix_entry), GFP_KERNEL);
		if (!adapter->msix_entries) {
			err = -ENOMEM;
			goto out;
		}

		for (vector = 0; vector < v_budget; vector++)
			adapter->msix_entries[vector].entry = vector;

		err = rnpgbevf_acquire_msix_vectors(adapter, v_budget);
		if (!err) {
			adapter->vector_off = NON_Q_VECTORS;
			adapter->num_q_vectors =
				adapter->num_msix_vectors - NON_Q_VECTORS;
			DPRINTK(PROBE, INFO,
				"adapter%d alloc vectors: cnt:%d [%d~%d] num_msix_vectors:%d\n",
				adapter->bd_number, v_budget,
				adapter->vector_off,
				adapter->vector_off + v_budget - 1,
				adapter->num_msix_vectors);
			adapter->flags |= RNPVF_FLAG_MSIX_ENABLED;
			goto out;
		}
		kfree(adapter->msix_entries);

		if (adapter->flags & RNPVF_FLAG_MSI_CAPABLE) {
			adapter->irq_mode = irq_mode_msi;
			pr_info("acquire msix failed, try to use msi\n");
		}

	} else {
		pr_info("adapter not in msix mode\n");
	}

	if (adapter->irq_mode == irq_mode_msi) {
		err = pci_enable_msi(adapter->pdev);
		if (err) {
			pr_info("Failed to allocate MSI interrupt, falling back to legacy. Error");
		} else {
			/* msi mode use only 1 irq */
			adapter->flags |= RNPVF_FLAG_MSI_ENABLED;
		}
	}
	/* write back origin irq_mode */
	adapter->irq_mode = irq_mode_back;
	/* legacy and msi only 1 vectors */
	adapter->num_q_vectors = 1;

out:
	return err;
}

static void rnpgbevf_add_ring(struct rnpgbevf_ring *ring,
			      struct rnpgbevf_ring_container *head)
{
	ring->next = head->ring;
	head->ring = ring;
	head->count++;
}

static enum hrtimer_restart irq_miss_check(struct hrtimer *hrtimer)
{
	struct rnpgbevf_q_vector *q_vector;
	struct rnpgbevf_ring *ring;
	struct rnp_tx_desc *eop_desc;
	struct rnpgbevf_adapter *adapter;
	struct rnpgbevf_hw *hw;

	int tx_next_to_clean;
	int tx_next_to_use;

	struct rnpgbevf_tx_buffer *tx_buffer;
	union rnp_rx_desc *rx_desc;

	q_vector = container_of(hrtimer, struct rnpgbevf_q_vector,
				irq_miss_check_timer);
	adapter = q_vector->adapter;
	hw = &adapter->hw;

	/* If we're already down or resetting, just bail */
	if (test_bit(__RNPVF_DOWN, &adapter->state) ||
	    test_bit(__RNPVF_RESETTING, &adapter->state))
		goto do_self_napi;

	rnpgbevf_irq_disable_queues(q_vector);
	rnpgbevf_for_each_ring(ring, q_vector->tx) {
		tx_next_to_clean = ring->next_to_clean;
		tx_next_to_use = ring->next_to_use;
		if (tx_next_to_use == tx_next_to_clean)
			continue;

		tx_buffer = &ring->tx_buffer_info[tx_next_to_clean];
		eop_desc = tx_buffer->next_to_watch;
		if (eop_desc) {
			if ((eop_desc->cmd & cpu_to_le16(RNPGBE_TXD_STAT_DD))) {
				if (q_vector->new_rx_count !=
				    q_vector->old_rx_count) {
					ring_wr32(
						ring,
						RNPGBE_DMA_REG_RX_INT_DELAY_PKTCNT,
						q_vector->new_rx_count);
					q_vector->old_rx_count =
						q_vector->new_rx_count;
				}
				napi_schedule_irqoff(&q_vector->napi);
				goto do_self_napi;
			}
		}
	}

	/* check rx irq */
	rnpgbevf_for_each_ring(ring, q_vector->rx) {
		rx_desc = RNPVF_RX_DESC(ring, ring->next_to_clean);

		if (!rx_desc)
			continue;
		if (rnpgbevf_test_staterr(rx_desc, RNPGBE_RXD_STAT_DD)) {
			unsigned int size;

			size = le16_to_cpu(rx_desc->wb.len) -
			       le16_to_cpu(rx_desc->wb.padding_len);

			if (size) {
				if (q_vector->new_rx_count !=
				    q_vector->old_rx_count) {
					ring_wr32(
						ring,
						RNPGBE_DMA_REG_RX_INT_DELAY_PKTCNT,
						q_vector->new_rx_count);
					q_vector->old_rx_count =
						q_vector->new_rx_count;
				}
				napi_schedule_irqoff(&q_vector->napi);
			} else
				adapter->flags |= RNPVF_FLAG_PF_RESET_REQ;
			goto do_self_napi;
		}
	}
	rnpgbevf_irq_enable_queues(q_vector);
do_self_napi:
	return HRTIMER_NORESTART;
}

static int rnpgbevf_alloc_q_vector(struct rnpgbevf_adapter *adapter,
				   int eth_queue_idx, int rnpgbevf_vector,
				   int rnpgbevf_queue, int r_count, int step)
{
	struct rnpgbevf_q_vector *q_vector;
	struct rnpgbevf_ring *ring;
	struct rnpgbevf_hw *hw = &adapter->hw;
	int node = NUMA_NO_NODE;
	int cpu = -1;
	int ring_count, size;
	int txr_count, rxr_count, idx;
	int rxr_idx = rnpgbevf_queue, txr_idx = rnpgbevf_queue;

	DPRINTK(PROBE, INFO,
		"eth_queue_idx:%d rnpgbevf_vector:%d(off:%d) ring:%d "
		"ring_cnt:%d, step:%d\n",
		eth_queue_idx, rnpgbevf_vector, adapter->vector_off,
		rnpgbevf_queue, r_count, step);

	txr_count = rxr_count = r_count;

	ring_count = txr_count + rxr_count;
	size = sizeof(struct rnpgbevf_q_vector) +
	       (sizeof(struct rnpgbevf_ring) * ring_count);

	if (cpu_online(rnpgbevf_vector)) {
		cpu = rnpgbevf_vector;
		node = cpu_to_node(cpu);
	}

	/* allocate q_vector and rings */
	q_vector = kzalloc_node(size, GFP_KERNEL, node);
	if (!q_vector)
		q_vector = kzalloc(size, GFP_KERNEL);
	if (!q_vector)
		return -ENOMEM;

	/* setup affinity mask and node */
	if (cpu != -1)
		cpumask_set_cpu(cpu, &q_vector->affinity_mask);
	q_vector->numa_node = node;

	netif_napi_add_weight(adapter->netdev, &q_vector->napi, rnpgbevf_poll,
			      adapter->napi_budge);

	/* tie q_vector and adapter together */
	adapter->q_vector[rnpgbevf_vector - adapter->vector_off] = q_vector;
	q_vector->adapter = adapter;
	q_vector->v_idx = rnpgbevf_vector;

	/* initialize pointer to rings */
	ring = q_vector->ring;

	for (idx = 0; idx < txr_count; idx++) {
		/* assign generic ring traits */
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;

		/* configure backlink on ring */
		ring->q_vector = q_vector;

		/* update q_vector Tx values */
		rnpgbevf_add_ring(ring, &q_vector->tx);

		/* apply Tx specific ring traits */
		ring->count = adapter->tx_ring_item_count;
		ring->queue_index = eth_queue_idx + idx;
		ring->rnpgbevf_queue_idx = txr_idx;

		if (hw->board_type == rnp_board_n10) {
			ring->ring_flags |= RNPVF_RING_SKIP_TX_START;
			ring->ring_addr = hw->hw_addr + RNPGBE_RING_BASE_N10 +
					  RNPGBE_RING_OFFSET(txr_idx);
			ring->rnpgbevf_msix_off = txr_idx;
		} else if (hw->board_type == rnp_board_n500) {
			/* n500 not support tunnel */
			ring->ring_flags |= RNPVF_RING_NO_TUNNEL_SUPPORT;
			/* n500 fixed ring size change from large to small */
			ring->ring_flags |= RNPVF_RING_SIZE_CHANGE_FIX;
			/* n500 vf use this */
			ring->ring_addr = hw->hw_addr + RNPGBE_RING_BASE_N500;
			ring->ring_flags |= RNPVF_RING_VEB_MULTI_FIX;
			ring->rnpgbevf_msix_off = 0;
		} else if (hw->board_type == rnp_board_n210) {
			/* n210 not support tunnel */
			ring->ring_flags |= RNPVF_RING_NO_TUNNEL_SUPPORT;
			/* n210 fixed ring size change from large to small */
			ring->ring_flags |= RNPVF_RING_SIZE_CHANGE_FIX;
			/* n210 vf use this */
			ring->ring_addr = hw->hw_addr + RNPGBE_RING_BASE_N500;
			ring->ring_flags |= RNPVF_RING_VEB_MULTI_FIX;
			ring->rnpgbevf_msix_off = 0;
		}
		ring->dma_int_stat = ring->ring_addr + RNPGBE_DMA_INT_STAT;
		ring->dma_int_mask = ring->dma_int_stat + 4;
		ring->dma_int_clr = ring->dma_int_stat + 8;
		ring->device_id = adapter->pdev->device;
		ring->vfnum = hw->vfnum;
		/* assign ring to adapter */
		adapter->tx_ring[ring->queue_index] = ring;
		dbg("adapter->tx_ringp[%d] <= %p\n", ring->queue_index, ring);

		/* update count and index */
		txr_idx += step;

		DPRINTK(PROBE, INFO,
			"vector[%d] <--RNP TxRing:%d, eth_queue:%d\n",
			rnpgbevf_vector, ring->rnpgbevf_queue_idx,
			ring->queue_index);

		/* push pointer to next ring */
		ring++;
	}

	for (idx = 0; idx < rxr_count; idx++) {
		/* assign generic ring traits */
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;

		/* configure backlink on ring */
		ring->q_vector = q_vector;

		/* update q_vector Rx values */
		rnpgbevf_add_ring(ring, &q_vector->rx);

		/* apply Rx specific ring traits */
		ring->count = adapter->rx_ring_item_count;
		ring->queue_index = eth_queue_idx + idx;
		ring->rnpgbevf_queue_idx = rxr_idx;

		if (hw->board_type == rnp_board_n10) {
			ring->ring_addr = hw->hw_addr + RNPGBE_RING_BASE_N10 +
					  RNPGBE_RING_OFFSET(rxr_idx);
			ring->rnpgbevf_msix_off = rxr_idx;

		} else if (hw->board_type == rnp_board_n500) {
			/* n500 fixed ring size change from large to small */
			ring->ring_flags |= RNPVF_RING_SIZE_CHANGE_FIX;
			ring->ring_flags |= RNPVF_RING_SCATER_SETUP;
			ring->ring_flags |= RNPVF_RING_NO_TUNNEL_SUPPORT;
			ring->ring_flags |= RNPVF_RING_STAGS_SUPPORT;
			ring->ring_flags |= RNPVF_RING_VEB_MULTI_FIX;
			ring->ring_flags |= RNPVF_RING_IRQ_MISS_FIX;
			ring->ring_flags |= RNPVF_RING_CHKSM_FIX;
			ring->ring_addr = hw->hw_addr + RNPGBE_RING_BASE_N500;
			ring->rnpgbevf_msix_off = 0;
		} else if (hw->board_type == rnp_board_n210) {
			/* n210 fixed ring size change from large to small */
			ring->ring_flags |= RNPVF_RING_SIZE_CHANGE_FIX;
			ring->ring_flags |= RNPVF_RING_SCATER_SETUP;
			ring->ring_flags |= RNPVF_RING_NO_TUNNEL_SUPPORT;
			ring->ring_flags |= RNPVF_RING_STAGS_SUPPORT;
			ring->ring_flags |= RNPVF_RING_VEB_MULTI_FIX;
			ring->ring_flags |= RNPVF_RING_IRQ_MISS_FIX;
			ring->ring_flags |= RNPVF_RING_CHKSM_FIX;
			ring->ring_addr = hw->hw_addr + RNPGBE_RING_BASE_N500;
			ring->rnpgbevf_msix_off = 0;
		}
		ring->dma_int_stat = ring->ring_addr + RNPGBE_DMA_INT_STAT;
		ring->dma_int_mask = ring->dma_int_stat + 4;
		ring->dma_int_clr = ring->dma_int_stat + 8;
		ring->device_id = adapter->pdev->device;
		ring->vfnum = hw->vfnum;

		/* assign ring to adapter */
		adapter->rx_ring[ring->queue_index] = ring;
		DPRINTK(PROBE, INFO,
			"vector[%d] <--RNP RxRing:%d, eth_queue:%d\n",
			rnpgbevf_vector, ring->rnpgbevf_queue_idx,
			ring->queue_index);

		/* update count and index */
		rxr_idx += step;

		/* push pointer to next ring */
		ring++;
	}

	if (hw->board_type == rnp_board_n10) {
		q_vector->vector_flags |= RNPVF_QVECTOR_FLAG_IRQ_MISS_CHECK;
		q_vector->vector_flags |= RNPVF_QVECTOR_FLAG_REDUCE_TX_IRQ_MISS;
		/* initialize timer */
		q_vector->irq_check_usecs = 1000;
		hrtimer_init(&q_vector->irq_miss_check_timer, CLOCK_MONOTONIC,
			     HRTIMER_MODE_REL);
		q_vector->irq_miss_check_timer.function =
			irq_miss_check; /* initialize NAPI */
	} else if (hw->board_type == rnp_board_n500) {
		q_vector->vector_flags |= RNPVF_QVECTOR_FLAG_ITR_FEATURE;

	} else if (hw->board_type == rnp_board_n210) {
		q_vector->vector_flags |= RNPVF_QVECTOR_FLAG_ITR_FEATURE;
	}

	return 0;
}

static void rnpgbevf_free_q_vector(struct rnpgbevf_adapter *adapter, int v_idx)
{
	struct rnpgbevf_q_vector *q_vector;
	struct rnpgbevf_ring *ring;

	dbg("v_idx:%d\n", v_idx);

	q_vector = adapter->q_vector[v_idx];

	rnpgbevf_for_each_ring(ring, q_vector->tx)
		adapter->tx_ring[ring->queue_index] = NULL;

	rnpgbevf_for_each_ring(ring, q_vector->rx)
		adapter->rx_ring[ring->queue_index] = NULL;

	adapter->q_vector[v_idx] = NULL;
	netif_napi_del(&q_vector->napi);

	if (q_vector->vector_flags & RNPVF_QVECTOR_FLAG_IRQ_MISS_CHECK)
		rnpgbevf_htimer_stop(q_vector);

	/*
	 * rnpgbevf_get_stats64() might access the rings on this vector,
	 * we must wait a grace period before freeing it.
	 */
	kfree_rcu(q_vector, rcu);
}

/**
 * rnpgbevf_alloc_q_vectors - Allocate memory for interrupt vectors
 * @adapter: board private structure to initialize
 *
 * We allocate one q_vector per queue interrupt.  If allocation fails we
 * return -ENOMEM.
 **/
static int rnpgbevf_alloc_q_vectors(struct rnpgbevf_adapter *adapter)
{
	int vector_idx = adapter->vector_off;
	int ring_idx = adapter->hw.queue_ring_base;
	int ring_remaing =
		min_t(int, adapter->num_tx_queues, adapter->num_rx_queues);
	int ring_step = 1;
	int err, ring_cnt,
		vector_remaing = adapter->num_msix_vectors - NON_Q_VECTORS;
	int eth_queue_idx = 0;

	BUG_ON(ring_remaing == 0);
	BUG_ON(vector_remaing == 0);

	for (; ring_remaing > 0 && vector_remaing > 0; vector_remaing--) {
		ring_cnt = DIV_ROUND_UP(ring_remaing, vector_remaing);

		err = rnpgbevf_alloc_q_vector(adapter, eth_queue_idx,
					      vector_idx, ring_idx, ring_cnt,
					      ring_step);
		if (err)
			goto err_out;

		ring_idx += ring_step * ring_cnt;
		ring_remaing -= ring_cnt;
		vector_idx++;
		eth_queue_idx += ring_cnt;
	}

	return 0;

err_out:
	vector_idx -= adapter->vector_off;
	while (vector_idx--)
		rnpgbevf_free_q_vector(adapter, vector_idx);
	return -ENOMEM;
}

/**
 * rnpgbevf_free_q_vectors - Free memory allocated for interrupt vectors
 * @adapter: board private structure to initialize
 *
 * This function frees the memory allocated to the q_vectors.  In addition if
 * NAPI is enabled it will delete any references to the NAPI struct prior
 * to freeing the q_vector.
 **/
static void rnpgbevf_free_q_vectors(struct rnpgbevf_adapter *adapter)
{
	int i, v_idx = adapter->num_q_vectors;

	adapter->num_rx_queues = 0;
	adapter->num_tx_queues = 0;
	adapter->num_q_vectors = 0;

	for (i = 0; i < v_idx; i++)
		rnpgbevf_free_q_vector(adapter, i);
}

/**
 * rnpgbevf_reset_interrupt_capability - Reset MSIX setup
 * @adapter: board private structure
 *
 **/
static void
rnpgbevf_reset_interrupt_capability(struct rnpgbevf_adapter *adapter)
{
	if (adapter->flags & RNPVF_FLAG_MSIX_ENABLED) {
		pci_disable_msix(adapter->pdev);
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;
	} else if (adapter->flags & RNPVF_FLAG_MSI_ENABLED) {
		pci_disable_msi(adapter->pdev);
	}
}

/**
 * rnpgbevf_init_interrupt_scheme - Determine if MSIX is supported and init
 * @adapter: board private structure to initialize
 *
 **/
int rnpgbevf_init_interrupt_scheme(struct rnpgbevf_adapter *adapter)
{
	int err;

	/* Number of supported queues */
	rnpgbevf_set_num_queues(adapter);

	err = rnpgbevf_set_interrupt_capability(adapter);
	if (err) {
		hw_dbg(&adapter->hw,
		       "Unable to setup interrupt capabilities\n");
		goto err_set_interrupt;
	}

	err = rnpgbevf_alloc_q_vectors(adapter);
	if (err) {
		hw_dbg(&adapter->hw, "Unable to allocate memory for queue "
				     "vectors\n");
		goto err_alloc_q_vectors;
	}

	hw_dbg(&adapter->hw,
	       "Multiqueue %s: Rx Queue count = %u, "
	       "Tx Queue count = %u\n",
	       (adapter->num_rx_queues > 1) ? "Enabled" : "Disabled",
	       adapter->num_rx_queues, adapter->num_tx_queues);

	set_bit(__RNPVF_DOWN, &adapter->state);

	return 0;
err_alloc_q_vectors:
	rnpgbevf_reset_interrupt_capability(adapter);
err_set_interrupt:
	return err;
}

/**
 * rnpgbevf_clear_interrupt_scheme - Clear the current interrupt scheme settings
 * @adapter: board private structure to clear interrupt scheme on
 *
 * We go through and clear interrupt specific resources and reset the structure
 * to pre-load conditions
 **/
void rnpgbevf_clear_interrupt_scheme(struct rnpgbevf_adapter *adapter)
{
	adapter->num_tx_queues = 0;
	adapter->num_rx_queues = 0;

	rnpgbevf_free_q_vectors(adapter);
	rnpgbevf_reset_interrupt_capability(adapter);
}

#define UPDATE_VF_COUNTER_32bit(reg, last_counter, counter)                    \
	{                                                                      \
		u32 current_counter = RNPGBE_READ_REG(hw, reg);                \
		if (current_counter < last_counter)                            \
			counter += 0x100000000LL;                              \
		last_counter = current_counter;                                \
		counter &= 0xFFFFFFFF00000000LL;                               \
		counter |= current_counter;                                    \
	}

#define UPDATE_VF_COUNTER_36bit(reg_lsb, reg_msb, last_counter, counter)       \
	{                                                                      \
		u64 current_counter_lsb = RNPGBE_READ_REG(hw, reg_lsb);        \
		u64 current_counter_msb = RNPGBE_READ_REG(hw, reg_msb);        \
		u64 current_counter =                                          \
			(current_counter_msb << 32) | current_counter_lsb;     \
		if (current_counter < last_counter)                            \
			counter += 0x1000000000LL;                             \
		last_counter = current_counter;                                \
		counter &= 0xFFFFFFF000000000LL;                               \
		counter |= current_counter;                                    \
	}

/**
 * rnpgbevf_update_stats - Update the board statistics counters.
 * @adapter: board private structure
 **/
void rnpgbevf_update_stats(struct rnpgbevf_adapter *adapter)
{
	struct rnpgbevf_hw_stats_own *hw_stats = &adapter->hw_stats;
	int i;
	struct net_device_stats *net_stats = &adapter->netdev->stats;

	net_stats->tx_packets = 0;
	net_stats->tx_bytes = 0;
	net_stats->rx_packets = 0;
	net_stats->rx_bytes = 0;
	net_stats->rx_dropped = 0;
	net_stats->rx_errors = 0;
	hw_stats->vlan_add_cnt = 0;
	hw_stats->vlan_strip_cnt = 0;
	hw_stats->csum_err = 0;
	hw_stats->csum_good = 0;
	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnpgbevf_ring *ring;
		struct rnpgbevf_q_vector *q_vector = adapter->q_vector[i];

		rnpgbevf_for_each_ring(ring, q_vector->tx) {
			hw_stats->vlan_add_cnt += ring->tx_stats.vlan_add;
			net_stats->tx_packets += ring->stats.packets;
			net_stats->tx_bytes += ring->stats.bytes;
		}

		rnpgbevf_for_each_ring(ring, q_vector->rx) {
			hw_stats->csum_err += ring->rx_stats.csum_err;
			hw_stats->csum_good += ring->rx_stats.csum_good;
			hw_stats->vlan_strip_cnt += ring->rx_stats.vlan_remove;
			net_stats->rx_packets += ring->stats.packets;
			net_stats->rx_bytes += ring->stats.bytes;
			net_stats->rx_errors += ring->rx_stats.csum_err;
		}
	}
}

static void rnpgbevf_reset_pf_request(struct rnpgbevf_adapter *adapter)
{
	struct rnpgbevf_hw *hw = &adapter->hw;

	if (!(adapter->flags & RNPVF_FLAG_PF_RESET_REQ))
		return;

	adapter->flags &= (~RNPVF_FLAG_PF_RESET_REQ);
	spin_lock_bh(&adapter->mbx_lock);
	set_bit(__RNPVF_MBX_POLLING, &adapter->state);
	hw->mac.ops.req_reset_pf(hw);
	clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
	spin_unlock_bh(&adapter->mbx_lock);
}

static int rnpgbevf_reset_subtask(struct rnpgbevf_adapter *adapter)
{
	if (!(adapter->flags & RNPVF_FLAG_PF_RESET))
		return 0;
	/* If we're already down or resetting, just bail */
	if (test_bit(__RNPVF_DOWN, &adapter->state) ||
	    test_bit(__RNPVF_RESETTING, &adapter->state))
		return 0;

	adapter->tx_timeout_count++;

	rtnl_lock();
	rnpgbevf_reinit_locked(adapter);
	rtnl_unlock();

	adapter->flags &= (~RNPVF_FLAG_PF_RESET);

	return 1;
}

/**
 * rnpgbevf_watchdog - Timer Call-back
 * @data: pointer to adapter cast into an unsigned long
 **/
static void rnpgbevf_watchdog(struct timer_list *t)
{
	struct rnpgbevf_adapter *adapter =
		from_timer(adapter, t, watchdog_timer);

	/*
	 * Do the watchdog outside of interrupt context due to the lovely
	 * delays that some of the newer hardware requires
	 */

	if (test_bit(__RNPVF_DOWN, &adapter->state))
		goto watchdog_short_circuit;

watchdog_short_circuit:
	if (!test_bit(__RNPVF_REMOVE, &adapter->state))
		schedule_work(&adapter->watchdog_task);
}

__maybe_unused static void rnpgbevf_reset_task(struct work_struct *work)
{
	struct rnpgbevf_adapter *adapter;

	adapter = container_of(work, struct rnpgbevf_adapter, reset_task);

	/* If we're already down or resetting, just bail */
	if (test_bit(__RNPVF_DOWN, &adapter->state) ||
	    test_bit(__RNPVF_RESETTING, &adapter->state))
		return;

	adapter->tx_timeout_count++;

	rnpgbevf_reinit_locked(adapter);
}

static void rnpgbevf_check_hang_subtask(struct rnpgbevf_adapter *adapter)
{
	int i;
	struct rnpgbevf_ring *tx_ring;
	u64 tx_next_to_clean_old;
	u64 tx_next_to_clean;
	u64 tx_next_to_use;
	struct rnpgbevf_ring *rx_ring;
	u64 rx_next_to_clean_old;
	u64 rx_next_to_clean;
	union rnp_rx_desc *rx_desc;

	/* If we're down or resetting, just bail */
	if (test_bit(__RNPVF_DOWN, &adapter->state) ||
	    test_bit(__RNPVF_RESETTING, &adapter->state))
		return;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		tx_ring = adapter->tx_ring[i];
		/* get the last next_to_clean */
		tx_next_to_clean_old = tx_ring->tx_stats.tx_next_to_clean;
		tx_next_to_clean = tx_ring->next_to_clean;
		tx_next_to_use = tx_ring->next_to_use;

		/* if we have tx desc to clean */
		if (tx_next_to_use != tx_next_to_clean) {
			if (tx_next_to_clean == tx_next_to_clean_old) {
				tx_ring->tx_stats.tx_equal_count++;
				if (tx_ring->tx_stats.tx_equal_count > 2) {
					/* maybe not so good */
					struct rnpgbevf_q_vector *q_vector =
						tx_ring->q_vector;

					/* stats */
					if (q_vector->rx.ring ||
					    q_vector->tx.ring)
						napi_schedule_irqoff(
							&q_vector->napi);

					tx_ring->tx_stats.tx_irq_miss++;
					tx_ring->tx_stats.tx_equal_count = 0;
				}
			} else {
				tx_ring->tx_stats.tx_equal_count = 0;
			}
			/* update */
			/* record this next_to_clean */
			tx_ring->tx_stats.tx_next_to_clean = tx_next_to_clean;
		} else {
			/* clean record to -1 */
			tx_ring->tx_stats.tx_next_to_clean = -1;
		}
	}
	/* check if we lost rx irq */
	for (i = 0; i < adapter->num_rx_queues; i++) {
		rx_ring = adapter->rx_ring[i];
		/* get the last next_to_clean */
		rx_next_to_clean_old = rx_ring->rx_stats.rx_next_to_clean;
		/* get the now clean */
		rx_next_to_clean = rx_ring->next_to_clean;

		if (rx_next_to_clean != rx_next_to_clean_old) {
			rx_ring->rx_stats.rx_equal_count = 0;
			rx_ring->rx_stats.rx_next_to_clean = rx_next_to_clean;
			continue;
		}
		rx_ring->rx_stats.rx_equal_count++;

		if ((rx_ring->rx_stats.rx_equal_count > 2) &&
		    (rx_ring->rx_stats.rx_equal_count < 5)) {
			rx_desc =
				RNPVF_RX_DESC(rx_ring, rx_ring->next_to_clean);
			if (rnpgbevf_test_staterr(rx_desc,
						  RNPGBE_RXD_STAT_DD)) {
				struct rnpgbevf_q_vector *q_vector =
					rx_ring->q_vector;
				unsigned int size;

				size = le16_to_cpu(rx_desc->wb.len) -
				       le16_to_cpu(rx_desc->wb.padding_len);
				if (size) {
					rx_ring->rx_stats.rx_irq_miss++;
					if (q_vector->rx.ring ||
					    q_vector->tx.ring)
						napi_schedule_irqoff(
							&q_vector->napi);
				}
			}
		}
		if (rx_ring->rx_stats.rx_equal_count > 1000)
			rx_ring->rx_stats.rx_equal_count = 0;
		rx_ring->rx_stats.rx_next_to_clean = rx_next_to_clean;
	}
}

/* just modify rx itr */
__maybe_unused static void
rnpgbevf_auto_itr_moderation(struct rnpgbevf_adapter *adapter)
{
	int i;
	struct rnpgbevf_ring *rx_ring;
	u64 period = (u64)(jiffies - adapter->last_moder_jiffies);

	// update jiffies
	adapter->last_moder_jiffies = jiffies;

	if (!adapter->adaptive_rx_coal ||
	    period < adapter->sample_interval * HZ) {
		return;
	}

	/* it is time to check moderation */
	for (i = 0; i < adapter->num_rx_queues; i++) {
		u64 x, y, rate;
		u64 rx_packets, packets, rx_pkt_diff;

		rx_ring = adapter->rx_ring[i];
		rx_packets = READ_ONCE(rx_ring->stats.packets);
		rx_pkt_diff = rx_packets -
			      adapter->last_moder_packets[rx_ring->queue_index];
		packets = rx_pkt_diff;

		x = packets * HZ;
		y = do_div(x, period);
		rate = x;

		if (rate == 0) {
		} else if (rate < 20000) {
			rx_ring->ring_flags |= RNPVF_RING_LOWER_ITR;
		} else {
			rx_ring->ring_flags &= (~RNPVF_RING_LOWER_ITR);
		}

		/* write back new count */
		adapter->last_moder_packets[rx_ring->queue_index] = rx_packets;
	}
}

/**
 * rnpgbevf_watchdog_task - worker thread to bring link up
 * @work: pointer to work_struct containing our data
 **/
static void rnpgbevf_watchdog_task(struct work_struct *work)
{
	struct rnpgbevf_adapter *adapter =
		container_of(work, struct rnpgbevf_adapter, watchdog_task);
	struct net_device *netdev = adapter->netdev;
	struct rnpgbevf_hw *hw = &adapter->hw;
	u32 link_speed = adapter->link_speed;
	bool link_up = adapter->link_up;
	s32 need_reset;

	adapter->flags |= RNPVF_FLAG_IN_WATCHDOG_TASK;

	rnpgbevf_reset_pf_request(adapter);

	if (rnpgbevf_reset_subtask(adapter)) {
		adapter->flags &= ~RNPVF_FLAG_PF_UPDATE_MTU;
		adapter->flags &= ~RNPVF_FLAG_PF_UPDATE_VLAN;
		goto pf_has_reset;
	}

	need_reset = hw->mac.ops.check_link(hw, &link_speed, &link_up, false);

	if (need_reset) {
		adapter->link_up = link_up;
		adapter->link_speed = link_speed;
		netif_carrier_off(netdev);
		netif_tx_stop_all_queues(netdev);
		schedule_work(&adapter->reset_task);
		goto pf_has_reset;
	}
	adapter->link_up = link_up;
	adapter->link_speed = link_speed;

	if (test_bit(__RNPVF_DOWN, &adapter->state)) {
		if (test_bit(__RNPVF_LINK_DOWN, &adapter->state)) {
			clear_bit(__RNPVF_LINK_DOWN, &adapter->state);
			dev_info(&adapter->pdev->dev, "NIC Link is Down\n");
		}
		goto skip_link_check;
	}

	if (link_up) {
		if (!netif_carrier_ok(netdev)) {
			char *link_speed_string;

			switch (link_speed) {
			case RNPGBE_LINK_SPEED_40GB_FULL:
				link_speed_string = "40 Gbps";
				break;
			case RNPGBE_LINK_SPEED_25GB_FULL:
				link_speed_string = "25 Gbps";
				break;
			case RNPGBE_LINK_SPEED_10GB_FULL:
				link_speed_string = "10 Gbps";
				break;
			case RNPGBE_LINK_SPEED_1GB_FULL:
				link_speed_string = "1 Gbps";
				break;
			case RNPGBE_LINK_SPEED_100_FULL:
				link_speed_string = "100 Mbps";
				break;
			default:
				link_speed_string = "unknown speed";
				break;
			}
			dev_info(&adapter->pdev->dev, "NIC Link is Up, %s\n",
				 link_speed_string);
			netif_carrier_on(netdev);
			netif_tx_wake_all_queues(netdev);
		}
	} else {
		adapter->link_up = false;
		adapter->link_speed = 0;
		if (netif_carrier_ok(netdev)) {
			dev_info(&adapter->pdev->dev, "NIC Link is Down\n");
			netif_carrier_off(netdev);
			netif_tx_stop_all_queues(netdev);
		}
	}
skip_link_check:
	if (adapter->flags & RNPVF_FLAG_PF_UPDATE_MTU) {
		adapter->flags &= ~RNPVF_FLAG_PF_UPDATE_MTU;
		if (netdev->mtu > hw->mtu) {
			netdev->mtu = hw->mtu;
			rtnl_lock();
			call_netdevice_notifiers(NETDEV_CHANGEMTU,
						 adapter->netdev);
			rtnl_unlock();
		}
	}
	if (adapter->flags & RNPVF_FLAG_PF_UPDATE_VLAN) {
		adapter->flags &= ~RNPVF_FLAG_PF_UPDATE_VLAN;
		rnpgbevf_set_rx_mode(adapter->netdev);
	}

	rnpgbevf_check_hang_subtask(adapter);
	rnpgbevf_update_stats(adapter);
	rnpgbevf_auto_itr_moderation(adapter);

pf_has_reset:
	/* Reset the timer */
	mod_timer(&adapter->watchdog_timer, round_jiffies(jiffies + (2 * HZ)));
	adapter->flags &= ~RNPVF_FLAG_IN_WATCHDOG_TASK;
}

/**
 * rnpgbevf_free_tx_resources - Free Tx Resources per Queue
 * @adapter: board private structure
 * @tx_ring: Tx descriptor ring for a specific queue
 *
 * Free all transmit software resources
 **/
void rnpgbevf_free_tx_resources(struct rnpgbevf_adapter *adapter,
				struct rnpgbevf_ring *tx_ring)
{
	BUG_ON(tx_ring == NULL);

	rnpgbevf_clean_tx_ring(adapter, tx_ring);

	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;

	/* if not set, then don't free */
	if (!tx_ring->desc)
		return;

	dma_free_coherent(tx_ring->dev, tx_ring->size, tx_ring->desc,
			  tx_ring->dma);

	tx_ring->desc = NULL;
}

/**
 * rnpgbevf_free_all_tx_resources - Free Tx Resources for All Queues
 * @adapter: board private structure
 *
 * Free all transmit software resources
 **/
static void rnpgbevf_free_all_tx_resources(struct rnpgbevf_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		rnpgbevf_free_tx_resources(adapter, adapter->tx_ring[i]);
}

/**
 * rnpgbevf_setup_tx_resources - allocate Tx resources (Descriptors)
 * @adapter: board private structure
 * @tx_ring:    tx descriptor ring (for a specific queue) to setup
 *
 * Return 0 on success, negative on failure
 **/
int rnpgbevf_setup_tx_resources(struct rnpgbevf_adapter *adapter,
				struct rnpgbevf_ring *tx_ring)
{
	struct device *dev = tx_ring->dev;
	int orig_node = dev_to_node(dev);
	int numa_node = NUMA_NO_NODE;
	int size;

	size = sizeof(struct rnpgbevf_tx_buffer) * tx_ring->count;

	if (tx_ring->q_vector)
		numa_node = tx_ring->q_vector->numa_node;

	dbg("%s size:%d count:%d\n", __func__, size, tx_ring->count);
	tx_ring->tx_buffer_info = vzalloc_node(size, numa_node);
	if (!tx_ring->tx_buffer_info)
		tx_ring->tx_buffer_info = vzalloc(size);
	if (!tx_ring->tx_buffer_info)
		goto err_buffer;

	/* round up to nearest 4K */
	tx_ring->size = tx_ring->count * sizeof(struct rnp_tx_desc);
	tx_ring->size = ALIGN(tx_ring->size, 4096);

	set_dev_node(dev, numa_node);
	tx_ring->desc = dma_alloc_coherent(dev, tx_ring->size, &tx_ring->dma,
					   GFP_KERNEL);
	set_dev_node(dev, orig_node);
	if (!tx_ring->desc)
		tx_ring->desc = dma_alloc_coherent(dev, tx_ring->size,
						   &tx_ring->dma, GFP_KERNEL);
	if (!tx_ring->desc)
		goto err;
	memset(tx_ring->desc, 0, tx_ring->size);

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;

	DPRINTK(IFUP, INFO,
		"%d TxRing:%d, vector:%d ItemCounts:%d "
		"desc:%p(0x%llx) node:%d\n",
		tx_ring->queue_index, tx_ring->rnpgbevf_queue_idx,
		tx_ring->q_vector->v_idx, tx_ring->count, tx_ring->desc,
		tx_ring->dma, numa_node);
	return 0;

err:
	rnpgbevf_err(
		"%s [SetupTxResources] ERROR: #%d TxRing:%d, vector:%d ItemCounts:%d\n",
		tx_ring->netdev->name, tx_ring->queue_index,
		tx_ring->rnpgbevf_queue_idx, tx_ring->q_vector->v_idx,
		tx_ring->count);
	vfree(tx_ring->tx_buffer_info);
err_buffer:
	tx_ring->tx_buffer_info = NULL;
	dev_err(dev, "Unable to allocate memory for the Tx descriptor ring\n");
	return -ENOMEM;
}

/**
 * rnpgbevf_setup_all_tx_resources - allocate all queues Tx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int rnpgbevf_setup_all_tx_resources(struct rnpgbevf_adapter *adapter)
{
	int i, err = 0;

	dbg("adapter->num_tx_queues:%d, adapter->tx_ring[0]:%p\n",
	    adapter->num_tx_queues, adapter->tx_ring[0]);

	for (i = 0; i < adapter->num_tx_queues; i++) {
		BUG_ON(adapter->tx_ring[i] == NULL);
		err = rnpgbevf_setup_tx_resources(adapter, adapter->tx_ring[i]);
		if (!err)
			continue;
		hw_dbg(&adapter->hw, "Allocation for Tx Queue %u failed\n", i);
		goto err_setup_tx;
	}

	return 0;

err_setup_tx:
	/* rewind the index freeing the rings as we go */
	while (i--)
		rnpgbevf_free_tx_resources(adapter, adapter->tx_ring[i]);
	return err;
}

/**
 * rnpgbevf_setup_rx_resources - allocate Rx resources (Descriptors)
 * @adapter: board private structure
 * @rx_ring:    rx descriptor ring (for a specific queue) to setup
 *
 * Returns 0 on success, negative on failure
 **/
int rnpgbevf_setup_rx_resources(struct rnpgbevf_adapter *adapter,
				struct rnpgbevf_ring *rx_ring)
{
	struct device *dev = rx_ring->dev;
	int orig_node = dev_to_node(dev);
	int numa_node = -1;
	int size;

	BUG_ON(rx_ring == NULL);

	size = sizeof(struct rnpgbevf_rx_buffer) * rx_ring->count;

	if (rx_ring->q_vector)
		numa_node = rx_ring->q_vector->numa_node;

	rx_ring->rx_buffer_info = vzalloc_node(size, numa_node);
	if (!rx_ring->rx_buffer_info)
		rx_ring->rx_buffer_info = vzalloc(size);
	if (!rx_ring->rx_buffer_info)
		goto alloc_buffer;

	/* Round up to nearest 4K */
	rx_ring->size = rx_ring->count * sizeof(union rnp_rx_desc);
	rx_ring->size = ALIGN(rx_ring->size, 4096);

	set_dev_node(dev, numa_node);
	rx_ring->desc = dma_alloc_coherent(&adapter->pdev->dev, rx_ring->size,
					   &rx_ring->dma, GFP_KERNEL);
	set_dev_node(dev, orig_node);
	if (!rx_ring->desc) {
		vfree(rx_ring->rx_buffer_info);
		rx_ring->rx_buffer_info = NULL;
		goto alloc_failed;
	}

	memset(rx_ring->desc, 0, rx_ring->size);
	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;

	DPRINTK(IFUP, INFO,
		"%d RxRing:%d, vector:%d ItemCounts:%d "
		"desc:%p(0x%llx) node:%d\n",
		rx_ring->queue_index, rx_ring->rnpgbevf_queue_idx,
		rx_ring->q_vector->v_idx, rx_ring->count, rx_ring->desc,
		rx_ring->dma, numa_node);

	return 0;
alloc_failed:
	rnpgbevf_err(
		"%s [SetupTxResources] ERROR: #%d RxRing:%d, vector:%d ItemCounts:%d\n",
		rx_ring->netdev->name, rx_ring->queue_index,
		rx_ring->rnpgbevf_queue_idx, rx_ring->q_vector->v_idx,
		rx_ring->count);
	vfree(rx_ring->tx_buffer_info);
alloc_buffer:
	rx_ring->tx_buffer_info = NULL;
	dev_err(dev, "Unable to allocate memory for the Rx descriptor ring\n");

	return -ENOMEM;
}

/**
 * rnpgbevf_setup_all_rx_resources - allocate all queues Rx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int rnpgbevf_setup_all_rx_resources(struct rnpgbevf_adapter *adapter)
{
	int i, err = 0;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		BUG_ON(adapter->rx_ring[i] == NULL);

		err = rnpgbevf_setup_rx_resources(adapter, adapter->rx_ring[i]);
		if (!err)
			continue;
		hw_dbg(&adapter->hw, "Allocation for Rx Queue %u failed\n", i);
		goto err_setup_rx;
	}

	return 0;

err_setup_rx:
	/* rewind the index freeing the rings as we go */
	while (i--)
		rnpgbevf_free_rx_resources(adapter, adapter->rx_ring[i]);
	return err;
}

/**
 * rnpgbevf_free_rx_resources - Free Rx Resources
 * @adapter: board private structure
 * @rx_ring: ring to clean the resources from
 *
 * Free all receive software resources
 **/
void rnpgbevf_free_rx_resources(struct rnpgbevf_adapter *adapter,
				struct rnpgbevf_ring *rx_ring)
{
	struct pci_dev *pdev = adapter->pdev;

	rnpgbevf_clean_rx_ring(rx_ring);

	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;

	/* if not set, then don't free */
	if (!rx_ring->desc)
		return;

	dma_free_coherent(&pdev->dev, rx_ring->size, rx_ring->desc,
			  rx_ring->dma);

	rx_ring->desc = NULL;
}

/**
 * rnpgbevf_free_all_rx_resources - Free Rx Resources for All Queues
 * @adapter: board private structure
 *
 * Free all receive software resources
 **/
static void rnpgbevf_free_all_rx_resources(struct rnpgbevf_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		rnpgbevf_free_rx_resources(adapter, adapter->rx_ring[i]);
}

/**
 * rnpgbevf_change_mtu - Change the Maximum Transfer Unit
 * @netdev: network interface device structure
 * @new_mtu: new value for maximum frame size
 *
 * Returns 0 on success, negative on failure
 **/
static int rnpgbevf_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct rnpgbevf_hw *hw = &adapter->hw;

	if (new_mtu > hw->mtu) {
		dev_info(&adapter->pdev->dev,
			 "PF limit vf mtu setup too large %d \n", hw->mtu);
		return -EINVAL;

	} else {
		hw_dbg(&adapter->hw, "changing MTU from %d to %d\n",
		       netdev->mtu, new_mtu);
		/* must set new MTU before calling down or up */
		netdev->mtu = new_mtu;
	}

	if (netif_running(netdev))
		rnpgbevf_reinit_locked(adapter);

	return 0;
}

/**
 * rnpgbevf_open - Called when a network interface is made active
 * @netdev: network interface device structure
 *
 * Returns 0 on success, negative value on failure
 *
 * The open entry point is called when a network interface is made
 * active by the system (IFF_UP).  At this point all resources needed
 * for transmit and receive operations are allocated, the interrupt
 * handler is registered with the OS, the watchdog timer is started,
 * and the stack is notified that the interface is ready.
 **/
int rnpgbevf_open(struct net_device *netdev)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct rnpgbevf_hw *hw = &adapter->hw;
	int err;

	DPRINTK(IFUP, INFO, "ifup\n");

	/* A previous failure to open the device because of a lack of
	 * available MSIX vector resources may have reset the number
	 * of msix vectors variable to zero.  The only way to recover
	 * is to unload/reload the driver and hope that the system has
	 * been able to recover some MSIX vector resources.
	 */
	if (!adapter->num_msix_vectors)
		return -ENOMEM;

	/* disallow open during test */
	if (test_bit(__RNPVF_TESTING, &adapter->state))
		return -EBUSY;

	if (hw->adapter_stopped) {
		rnpgbevf_reset(adapter);
		/* if adapter is still stopped then PF isn't up and
		 * the vf can't start.
		 */
		if (hw->adapter_stopped) {
			err = RNPGBE_ERR_MBX;
			dev_err(&hw->pdev->dev,
				"%s(%s):error: Unable to start - perhaps the PF Driver isn't "
				"up yet\n",
				adapter->name, netdev->name);
			goto err_setup_reset;
		}
	}

	netif_carrier_off(netdev);

	/* allocate transmit descriptors */
	err = rnpgbevf_setup_all_tx_resources(adapter);
	if (err)
		goto err_setup_tx;

	/* allocate receive descriptors */
	err = rnpgbevf_setup_all_rx_resources(adapter);
	if (err)
		goto err_setup_rx;

	rnpgbevf_configure(adapter);

	/* clear any pending interrupts, may auto mask */
	err = rnpgbevf_request_irq(adapter);
	if (err)
		goto err_req_irq;

	/* Notify the stack of the actual queue counts. */
	err = netif_set_real_num_tx_queues(netdev, adapter->num_tx_queues);
	if (err)
		goto err_set_queues;

	err = netif_set_real_num_rx_queues(netdev, adapter->num_rx_queues);
	if (err)
		goto err_set_queues;

	rnpgbevf_up_complete(adapter);

#ifdef CONFIG_NET_NCSI
	if (adapter->ncsi_dev) {
		err = ncsi_start_dev(adapter->ncsi_dev);
		if (err)
			netdev_err(netdev, "ncsi start faild!\n");
	}
#endif

	return 0;

err_set_queues:
	rnpgbevf_free_irq(adapter);
err_req_irq:

err_setup_rx:
	rnpgbevf_free_all_rx_resources(adapter);
err_setup_tx:
	rnpgbevf_free_all_tx_resources(adapter);

err_setup_reset:

	return err;
}

/**
 * rnpgbevf_close - Disables a network interface
 * @netdev: network interface device structure
 *
 * Returns 0, this is not allowed to fail
 *
 * The close entry point is called when an interface is de-activated
 * by the OS.  The hardware is still under the drivers control, but
 * needs to be disabled.  A global MAC reset is issued to stop the
 * hardware, and all transmit and receive resources are freed.
 **/
int rnpgbevf_close(struct net_device *netdev)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);

	DPRINTK(IFDOWN, INFO, "ifdown\n");

#ifdef CONFIG_NET_NCSI
	if (adapter->ncsi_dev)
		ncsi_stop_dev(adapter->ncsi_dev);
#endif

	rnpgbevf_down(adapter);
	rnpgbevf_free_irq(adapter);

	rnpgbevf_free_all_tx_resources(adapter);
	rnpgbevf_free_all_rx_resources(adapter);

	return 0;
}

void rnpgbevf_tx_ctxtdesc(struct rnpgbevf_ring *tx_ring, u16 mss_seg_len,
			  u8 l4_hdr_len, u8 tunnel_hdr_len, int ignore_vlan,
			  u16 type_tucmd, bool crc_pad)
{
	struct rnp_tx_ctx_desc *context_desc;
	u16 i = tx_ring->next_to_use;
	struct rnpgbevf_adapter *adapter = RING2ADAPT(tx_ring);
	struct rnpgbevf_hw *hw = &adapter->hw;
	struct rnp_mbx_info *mbx = &hw->mbx;
	u8 vfnum = VFNUM(mbx, hw->vfnum);

	context_desc = RNPVF_TX_CTXTDESC(tx_ring, i);

	i++;
	tx_ring->next_to_use = (i < tx_ring->count) ? i : 0;

	/* set bits to identify this as an advanced context descriptor */
	type_tucmd |= RNPGBE_TXD_CTX_CTRL_DESC;

	if (adapter->priv_flags & RNPVF_PRIV_FLAG_TX_PADDING) {
		if (!crc_pad)
			type_tucmd |= RNPGBE_TXD_MTI_CRC_PAD_CTRL;
	}

	context_desc->mss_len = cpu_to_le16(mss_seg_len);
	context_desc->vfnum = 0x80 | vfnum;
	context_desc->l4_hdr_len = l4_hdr_len;

	if (ignore_vlan)
		context_desc->vf_veb_flags |= VF_IGNORE_VLAN;

	context_desc->tunnel_hdr_len = tunnel_hdr_len;
	context_desc->rev = 0;
	context_desc->cmd = cpu_to_le16(type_tucmd);
	buf_dump_line("ctx  ", __LINE__, context_desc, sizeof(*context_desc));
}

static int rnpgbevf_tso(struct rnpgbevf_ring *tx_ring,
			struct rnpgbevf_tx_buffer *first, u8 *hdr_len)
{
	struct sk_buff *skb = first->skb;
	struct net_device *netdev = tx_ring->netdev;
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	union {
		struct iphdr *v4;
		struct ipv6hdr *v6;
		unsigned char *hdr;
	} ip;
	union {
		struct tcphdr *tcp;
		struct udphdr *udp;
		unsigned char *hdr;
	} l4;
	u32 paylen, l4_offset;
	int err;
	u8 *inner_mac;
	u16 gso_segs, gso_size;
	u16 gso_need_pad;

	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return 0;

	if (!skb_is_gso(skb))
		return 0;

	err = skb_cow_head(skb, 0);
	if (err < 0)
		return err;

	inner_mac = skb->data;
	ip.hdr = skb_network_header(skb);
	l4.hdr = skb_transport_header(skb);

	/* initialize outer IP header fields */
	if (ip.v4->version == 4) {
		/* IP header will have to cancel out any data that
		 * is not a part of the outer IP header
		 */
		ip.v4->check = 0x0000;
	} else {
		ip.v6->payload_len = 0;
	}
	if (skb_shinfo(skb)->gso_type &
	    (SKB_GSO_GRE |
	     SKB_GSO_GRE_CSUM |
	     SKB_GSO_UDP_TUNNEL | SKB_GSO_UDP_TUNNEL_CSUM)) {
		if (!(skb_shinfo(skb)->gso_type & SKB_GSO_PARTIAL) &&
		    (skb_shinfo(skb)->gso_type & SKB_GSO_UDP_TUNNEL_CSUM)) {
		}
		inner_mac = skb_inner_mac_header(skb);
		first->tunnel_hdr_len = inner_mac - skb->data;

		if (skb_shinfo(skb)->gso_type &
		    (SKB_GSO_UDP_TUNNEL | SKB_GSO_UDP_TUNNEL_CSUM)) {
			first->cmd_flags |= RNPGBE_TXD_TUNNEL_VXLAN;
			l4.udp->check = 0;
		} else {
			first->cmd_flags |= RNPGBE_TXD_TUNNEL_NVGRE;
		}
		dbg("set outer l4.udp to 0\n");

		/* reset pointers to inner headers */
		ip.hdr = skb_inner_network_header(skb);
		l4.hdr = skb_inner_transport_header(skb);
	}

	if (ip.v4->version == 4) {
		/* IP header will have to cancel out any data that
		 * is not a part of the outer IP header
		 */
		ip.v4->check = 0x0000;

	} else {
		ip.v6->payload_len = 0;
		/* set ipv6 type */
		first->cmd_flags |= (RNPGBE_TXD_FLAG_IPv6);
	}

	/* determine offset of inner transport header */
	l4_offset = l4.hdr - skb->data;

	paylen = skb->len - l4_offset;
	dbg("before l4 checksum is %x\n", l4.tcp->check);

	if (skb->csum_offset == offsetof(struct tcphdr, check)) {
		dbg("tcp before l4 checksum is %x\n", l4.tcp->check);
		first->cmd_flags |= RNPGBE_TXD_L4_TYPE_TCP;
		/* compute length of segmentation header */
		*hdr_len = (l4.tcp->doff * 4) + l4_offset;
		csum_replace_by_diff(&l4.tcp->check,
				     (__force __wsum)htonl(paylen));
		dbg("tcp l4 checksum is %x\n", l4.tcp->check);
		l4.tcp->psh = 0;
	} else {
		dbg("paylen is %x\n", paylen);
		first->cmd_flags |= RNPGBE_TXD_L4_TYPE_UDP;
		/* compute length of segmentation header */
		dbg("udp before l4 checksum is %x\n", l4.udp->check);
		*hdr_len = sizeof(*l4.udp) + l4_offset;
		csum_replace_by_diff(&l4.udp->check,
				     (__force __wsum)htonl(paylen));
		dbg("udp l4 checksum is %x\n", l4.udp->check);
	}

	dbg("l4 checksum is %x\n", l4.tcp->check);

	first->mac_ip_len = l4.hdr - ip.hdr;
	first->mac_ip_len |= (ip.hdr - inner_mac) << 9;

	/* compute header lengths */
	/* pull values out of skb_shinfo */
	gso_size = skb_shinfo(skb)->gso_size;
	gso_segs = skb_shinfo(skb)->gso_segs;

	if (adapter->priv_flags & RNPVF_PRIV_FLAG_TX_PADDING) {
		gso_need_pad = (first->skb->len - *hdr_len) % gso_size;
		if (gso_need_pad) {
			if ((gso_need_pad + *hdr_len) <= 60) {
				gso_need_pad = 60 - (gso_need_pad + *hdr_len);
				first->gso_need_padding = !!gso_need_pad;
			}
		}
	}

	/* update gso size and bytecount with header size */
	/* to fix tx status */
	first->gso_segs = gso_segs;
	first->bytecount += (first->gso_segs - 1) * *hdr_len;
	first->mss_len_vf_num |= (gso_size | ((l4.tcp->doff * 4) << 24));
	first->cmd_flags |=
		RNPGBE_TXD_FLAG_TSO | RNPGBE_TXD_IP_CSUM | RNPGBE_TXD_L4_CSUM;
	first->ctx_flag = true;
	return 1;
}

static int rnpgbevf_tx_csum(struct rnpgbevf_ring *tx_ring,
			    struct rnpgbevf_tx_buffer *first)
{
	struct sk_buff *skb = first->skb;
	u8 l4_proto = 0;
	u8 ip_len = 0;
	u8 mac_len = 0;
	u8 *inner_mac = skb->data;
	u8 *exthdr;
	__be16 frag_off;
	union {
		struct iphdr *v4;
		struct ipv6hdr *v6;
		unsigned char *hdr;
	} ip;
	union {
		struct tcphdr *tcp;
		struct udphdr *udp;
		unsigned char *hdr;
	} l4;

	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return 0;

	ip.hdr = skb_network_header(skb);
	l4.hdr = skb_transport_header(skb);

	inner_mac = skb->data;

	/* outer protocol */
	if (skb->encapsulation) {
		/* define outer network header type */
		if (ip.v4->version == 4) {
			l4_proto = ip.v4->protocol;
		} else {
			exthdr = ip.hdr + sizeof(*ip.v6);
			l4_proto = ip.v6->nexthdr;
			if (l4.hdr != exthdr)
				ipv6_skip_exthdr(skb, exthdr - skb->data,
						 &l4_proto, &frag_off);
		}

		/* define outer transport */
		switch (l4_proto) {
		case IPPROTO_UDP:
			l4.udp->check = 0;
			first->cmd_flags |= RNPGBE_TXD_TUNNEL_VXLAN;

			break;
		case IPPROTO_GRE:

			first->cmd_flags |= RNPGBE_TXD_TUNNEL_NVGRE;
			/* There was a long-standing issue in GRE where GSO
			 * was not setting the outer transport header unless
			 * a GRE checksum was requested. This was fixed in
			 * the 4.6 version of the kernel.  In the 4.7 kernel
			 * support for GRE over IPv6 was added to GSO.  So we
			 * can assume this workaround for all IPv4 headers
			 * without impacting later versions of the GRE.
			 */
			if (ip.v4->version == 4)
				l4.hdr = ip.hdr + (ip.v4->ihl * 4);
			break;
		default:
			skb_checksum_help(skb);
			return -1;
		}

		/* switch IP header pointer from outer to inner header */
		ip.hdr = skb_inner_network_header(skb);
		l4.hdr = skb_inner_transport_header(skb);

		inner_mac = skb_inner_mac_header(skb);
		first->tunnel_hdr_len = inner_mac - skb->data;
		first->ctx_flag = true;
		dbg("tunnel length is %d\n", first->tunnel_hdr_len);
	}

	mac_len = (ip.hdr - inner_mac);
	dbg("inner checksum needed %d", skb_checksum_start_offset(skb));
	dbg("skb->encapsulation %d\n", skb->encapsulation);
	ip_len = (l4.hdr - ip.hdr);
	if (ip.v4->version == 4) {
		l4_proto = ip.v4->protocol;
	} else {
		exthdr = ip.hdr + sizeof(*ip.v6);
		l4_proto = ip.v6->nexthdr;
		if (l4.hdr != exthdr)
			ipv6_skip_exthdr(skb, exthdr - skb->data, &l4_proto,
					 &frag_off);
		first->cmd_flags |= RNPGBE_TXD_FLAG_IPv6;
	}
	/* Enable L4 checksum offloads */
	switch (l4_proto) {
	case IPPROTO_TCP:
		first->cmd_flags |= RNPGBE_TXD_L4_TYPE_TCP | RNPGBE_TXD_L4_CSUM;
		break;
	case IPPROTO_SCTP:
		first->cmd_flags |=
			RNPGBE_TXD_L4_TYPE_SCTP | RNPGBE_TXD_L4_CSUM;
		break;
	case IPPROTO_UDP:
		first->cmd_flags |= RNPGBE_TXD_L4_TYPE_UDP | RNPGBE_TXD_L4_CSUM;
		break;
	default:
		skb_checksum_help(skb);
		return 0;
	}
	if ((tx_ring->ring_flags & RNPVF_RING_NO_TUNNEL_SUPPORT) &&
	    (first->ctx_flag)) {
		/* if not support tunnel */
		first->cmd_flags &= (~RNPGBE_TXD_TUNNEL_MASK);
		mac_len += first->tunnel_hdr_len;
		first->tunnel_hdr_len = 0;
		first->ctx_flag = false;
	}

	dbg("mac length is %d\n", mac_len);
	dbg("ip length is %d\n", ip_len);
	first->mac_ip_len = (mac_len << 9) | ip_len;
	return 0;
}

static void rnpgbevf_tx_map(struct rnpgbevf_ring *tx_ring,
			    struct rnpgbevf_tx_buffer *first, const u8 hdr_len)
{
	struct sk_buff *skb = first->skb;
	struct rnpgbevf_tx_buffer *tx_buffer;
	struct rnp_tx_desc *tx_desc;
	skb_frag_t *frag;
	dma_addr_t dma;
	unsigned int data_len, size;
	u16 vlan = first->vlan;
	u16 cmd = first->cmd_flags;
	u16 i = tx_ring->next_to_use;
	u64 fun_id = ((u64)(tx_ring->vfnum) << (32 + 24));

	tx_desc = RNPVF_TX_DESC(tx_ring, i);
	tx_desc->blen = cpu_to_le16(skb->len - hdr_len); /* maybe no-use */
	tx_desc->vlan = cpu_to_le16(vlan);
	tx_desc->cmd = cpu_to_le16(cmd);
	tx_desc->mac_ip_len = first->mac_ip_len;

	size = skb_headlen(skb);
	data_len = skb->data_len;

	dma = dma_map_single(tx_ring->dev, skb->data, size, DMA_TO_DEVICE);

	tx_buffer = first;

	for (frag = &skb_shinfo(skb)->frags[0];; frag++) {
		if (dma_mapping_error(tx_ring->dev, dma))
			goto dma_error;

		/* record length, and DMA address */
		dma_unmap_len_set(tx_buffer, len, size);
		dma_unmap_addr_set(tx_buffer, dma, dma);

		/* 1st desc */
		tx_desc->pkt_addr = cpu_to_le64(dma | fun_id);

		while (unlikely(size > RNPVF_MAX_DATA_PER_TXD)) {
			tx_desc->cmd = cpu_to_le16(cmd);
			tx_desc->blen = cpu_to_le16(RNPVF_MAX_DATA_PER_TXD);
			/* ==== desc== */
			buf_dump_line("tx0  ", __LINE__, tx_desc,
				      sizeof(*tx_desc));
			i++;
			tx_desc++;
			if (i == tx_ring->count) {
				tx_desc = RNPVF_TX_DESC(tx_ring, 0);
				i = 0;
			}

			dma += RNPVF_MAX_DATA_PER_TXD;
			size -= RNPVF_MAX_DATA_PER_TXD;

			tx_desc->pkt_addr = cpu_to_le64(dma | fun_id);
		}

		buf_dump_line("tx1  ", __LINE__, tx_desc, sizeof(*tx_desc));
		if (likely(!data_len))
			break;
		tx_desc->cmd = cpu_to_le16(cmd);
		tx_desc->blen = cpu_to_le16(size);
		buf_dump_line("tx2  ", __LINE__, tx_desc, sizeof(*tx_desc));

		/* ==== frag== */
		i++;
		tx_desc++;
		if (i == tx_ring->count) {
			tx_desc = RNPVF_TX_DESC(tx_ring, 0);
			i = 0;
		}
		tx_desc->cmd = RNPGBE_TXD_CMD_RS;
		tx_desc->mac_ip_len = 0;

		size = skb_frag_size(frag);

		data_len -= size;

		dma = skb_frag_dma_map(tx_ring->dev, frag, 0, size,
				       DMA_TO_DEVICE);

		tx_buffer = &tx_ring->tx_buffer_info[i];
	}

	/* write last descriptor with RS and EOP bits */
	tx_desc->cmd =
		cpu_to_le16(cmd | RNPGBE_TXD_CMD_EOP | RNPGBE_TXD_CMD_RS);
	tx_desc->blen = cpu_to_le16(size);
	buf_dump_line("tx3  ", __LINE__, tx_desc, sizeof(*tx_desc));
	netdev_tx_sent_queue(txring_txq(tx_ring), first->bytecount);

	/* set the timestamp */
	first->time_stamp = jiffies;

	/*
	 * Force memory writes to complete before letting h/w know there
	 * are new descriptors to fetch.  (Only applicable for weak-ordered
	 * memory model archs, such as IA-64).
	 *
	 * We also need this memory barrier to make certain all of the
	 * status bits have been updated before next_to_watch is written.
	 */
	wmb();

	/* set next_to_watch value indicating a packet is present */
	first->next_to_watch = tx_desc;

	buf_dump_line("tx4  ", __LINE__, tx_desc, sizeof(*tx_desc));
	i++;
	if (i == tx_ring->count)
		i = 0;

	tx_ring->next_to_use = i;

	/* notify HW of packet */
	rnpgbevf_wr_reg(tx_ring->tail, i);

	return;
dma_error:
	dev_err(tx_ring->dev, "TX DMA map failed\n");

	/* clear dma mappings for failed tx_buffer_info map */
	for (;;) {
		tx_buffer = &tx_ring->tx_buffer_info[i];
		rnpgbevf_unmap_and_free_tx_resource(tx_ring, tx_buffer);
		if (tx_buffer == first)
			break;
		if (i == 0)
			i = tx_ring->count;
		i--;
	}

	tx_ring->next_to_use = i;
}

static int __rnpgbevf_maybe_stop_tx(struct rnpgbevf_ring *tx_ring, int size)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(tx_ring->netdev);

	dbg("stop subqueue\n");
	netif_stop_subqueue(tx_ring->netdev, tx_ring->queue_index);
	/* Herbert's original patch had:
	 *  smp_mb__after_netif_stop_queue();
	 * but since that doesn't exist yet, just open code it.
	 */
	smp_mb();

	/* We need to check again in a case another CPU has just
	 * made room available.
	 */
	if (likely(rnpgbevf_desc_unused(tx_ring) < size))
		return -EBUSY;

	/* A reprieve! - use start_queue because it doesn't call schedule */
	netif_start_subqueue(tx_ring->netdev, tx_ring->queue_index);
	++adapter->restart_queue;
	return 0;
}

void rnpgbevf_maybe_tx_ctxtdesc(struct rnpgbevf_ring *tx_ring,
				struct rnpgbevf_tx_buffer *first,
				int ignore_vlan, u16 type_tucmd)
{
	if (first->ctx_flag) {
		rnpgbevf_tx_ctxtdesc(tx_ring, first->mss_len, first->l4_hdr_len,
				     first->tunnel_hdr_len, ignore_vlan,
				     type_tucmd, first->gso_need_padding);
	}
}

static int rnpgbevf_maybe_stop_tx(struct rnpgbevf_ring *tx_ring, int size)
{
	if (likely(RNPVF_DESC_UNUSED(tx_ring) >= size))
		return 0;
	return __rnpgbevf_maybe_stop_tx(tx_ring, size);
}

netdev_tx_t rnpgbevf_xmit_frame_ring(struct sk_buff *skb,
				     struct rnpgbevf_adapter *adapter,
				     struct rnpgbevf_ring *tx_ring,
				     bool tx_padding)
{
	struct rnpgbevf_tx_buffer *first;
	int tso;
	u16 cmd = RNPGBE_TXD_CMD_RS;
	u16 vlan = 0;
	unsigned short f;
	u16 count = TXD_USE_COUNT(skb_headlen(skb));
	__be16 protocol = skb->protocol;
	u8 hdr_len = 0;
	int ignore_vlan = 0;

	dbg("=== begin ====\n");

	rnpgbevf_skb_dump(skb, true);

	dbg("skb:%p, skb->len:%d  headlen:%d, data_len:%d, tx_ring->next_to_use:%d "
	    "count:%d\n",
	    skb, skb->len, skb_headlen(skb), skb->data_len,
	    tx_ring->next_to_use, tx_ring->count);
	/*
	 * need: 1 descriptor per page * PAGE_SIZE/RNPVF_MAX_DATA_PER_TXD,
	 *       + 1 desc for skb_headlen/RNPVF_MAX_DATA_PER_TXD,
	 *       + 2 desc gap to keep tail from touching head,
	 *       + 1 desc for context descriptor,
	 * otherwise try next time
	 */
	for (f = 0; f < skb_shinfo(skb)->nr_frags; f++) {
		skb_frag_t *frag_temp = &skb_shinfo(skb)->frags[f];
		count += TXD_USE_COUNT(skb_frag_size(frag_temp));
		dbg(" #%d frag: size:%d\n", f, skb_shinfo(skb)->frags[f].size);
	}

	if (rnpgbevf_maybe_stop_tx(tx_ring, count + 3)) {
		tx_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}
	dbg("xx %p\n", tx_ring->tx_buffer_info);

	/* record the location of the first descriptor for this packet */
	first = &tx_ring->tx_buffer_info[tx_ring->next_to_use];
	first->skb = skb;
	first->bytecount = skb->len;
	first->gso_segs = 1;

	first->mss_len_vf_num = 0;
	first->inner_vlan_tunnel_len = 0;

	if (adapter->priv_flags & RNPVF_PRIV_FLAG_TX_PADDING) {
		first->ctx_flag = true;
		first->gso_need_padding = tx_padding;
	}

	/* if we have a HW VLAN tag being added default to the HW one */
	if (adapter->flags & RNPVF_FLAG_PF_SET_VLAN) {
		/* in this mode , driver insert vlan */
		vlan |= adapter->vf_vlan;
		cmd |= RNPGBE_TXD_VLAN_VALID | RNPGBE_TXD_VLAN_CTRL_INSERT_VLAN;

	} else {
		if (skb_vlan_tag_present(skb)) {
			if (skb->vlan_proto != htons(ETH_P_8021Q)) {
				/* veb only use ctags */
				vlan |= skb_vlan_tag_get(skb);
				cmd |= RNPGBE_TXD_SVLAN_TYPE |
				       RNPGBE_TXD_VLAN_CTRL_INSERT_VLAN;
			} else {
				vlan |= skb_vlan_tag_get(skb);
				cmd |= RNPGBE_TXD_VLAN_VALID |
				       RNPGBE_TXD_VLAN_CTRL_INSERT_VLAN;
			}
			tx_ring->tx_stats.vlan_add++;
			/* else if it is a SW VLAN check the next protocol and store the tag */
		} else if (protocol == __constant_htons(ETH_P_8021Q)) {
			struct vlan_hdr *vhdr, _vhdr;

			vhdr = skb_header_pointer(skb, ETH_HLEN, sizeof(_vhdr),
						  &_vhdr);
			if (!vhdr)
				goto out_drop;

			protocol = vhdr->h_vlan_encapsulated_proto;
			vlan = ntohs(vhdr->h_vlan_TCI);
			cmd |= RNPGBE_TXD_VLAN_VALID | RNPGBE_TXD_VLAN_CTRL_NOP;
			ignore_vlan = 1;
		}
	}

	/* record initial flags and protocol */
	first->cmd_flags = cmd;
	first->vlan = vlan;
	first->protocol = protocol;
	/* default len should not 0 (hw request) */
	first->mac_ip_len = 20;
	first->tunnel_hdr_len = 0;

	tso = rnpgbevf_tso(tx_ring, first, &hdr_len);
	if (tso < 0)
		goto out_drop;
	else if (!tso)
		rnpgbevf_tx_csum(tx_ring, first);
	/* vf should always send ctx with vf_num*/
	first->ctx_flag = true;
	/* add control desc */
	rnpgbevf_maybe_tx_ctxtdesc(tx_ring, first, ignore_vlan, 0);
	rnpgbevf_tx_map(tx_ring, first, hdr_len);
	rnpgbevf_maybe_stop_tx(tx_ring, DESC_NEEDED);

	dbg("=== end ====\n\n\n\n");
	return NETDEV_TX_OK;

out_drop:
	dev_kfree_skb_any(first->skb);
	first->skb = NULL;

	return NETDEV_TX_OK;
}

static bool check_sctp_no_padding(struct sk_buff *skb)
{
	bool no_padding = false;
	u8 l4_proto = 0;
	u8 *exthdr;
	__be16 frag_off;
	union {
		struct iphdr *v4;
		struct ipv6hdr *v6;
		unsigned char *hdr;
	} ip;
	union {
		struct tcphdr *tcp;
		struct udphdr *udp;
		unsigned char *hdr;
	} l4;

	ip.hdr = skb_network_header(skb);
	l4.hdr = skb_transport_header(skb);

	if (ip.v4->version == 4) {
		l4_proto = ip.v4->protocol;
	} else {
		exthdr = ip.hdr + sizeof(*ip.v6);
		l4_proto = ip.v6->nexthdr;
		if (l4.hdr != exthdr)
			ipv6_skip_exthdr(skb, exthdr - skb->data, &l4_proto,
					 &frag_off);
	}
	switch (l4_proto) {
	case IPPROTO_SCTP:
		no_padding = true;
		break;
	default:

		break;
	}
	return no_padding;
}

static int rnpgbevf_xmit_frame(struct sk_buff *skb, struct net_device *netdev)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct rnpgbevf_ring *tx_ring;
	bool tx_padding = false;

	/*
	 * The minimum packet size for olinfo paylen is 17 so pad the skb
	 * in order to meet this minimum size requirement.
	 */
	/* for sctp packet, padding 0 change the crc32c */
	/* padding is done by hw
	 */

	if (!netif_carrier_ok(netdev)) {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}
	if (adapter->priv_flags & RNPVF_PRIV_FLAG_TX_PADDING) {
		if (skb->len < 60) {
			if (!check_sctp_no_padding(skb)) {
				if (skb_put_padto(skb, 60))
					return NETDEV_TX_OK;

			} else {
				tx_padding = true;
			}
		}
	} else {
		if (skb_put_padto(skb, 17))
			return NETDEV_TX_OK;
	}

	tx_ring = adapter->tx_ring[skb->queue_mapping];
	dbg("xmi:queue_mapping:%d ring:%p\n", skb->queue_mapping, tx_ring);
	return rnpgbevf_xmit_frame_ring(skb, adapter, tx_ring, tx_padding);
}

/**
 * rnpgbevf_set_mac - Change the Ethernet Address of the NIC
 * @netdev: network interface device structure
 * @p: pointer to an address structure
 *
 * Returns 0 on success, negative on failure
 **/
static int rnpgbevf_set_mac(struct net_device *netdev, void *p)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	struct rnpgbevf_hw *hw = &adapter->hw;
	struct sockaddr *addr = p;
	s32 ret_val;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;
	spin_lock_bh(&adapter->mbx_lock);
	set_bit(__RNPVF_MBX_POLLING, &adapter->state);
	ret_val = hw->mac.ops.set_rar(hw, 0, addr->sa_data, 0);
	clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
	spin_unlock_bh(&adapter->mbx_lock);
	if (ret_val != 0) {
		/* set mac failed */
		dev_err(&adapter->pdev->dev, "pf not allowed reset mac\n");
		return -EADDRNOTAVAIL;
	}
	eth_hw_addr_set(netdev, addr->sa_data);
	memcpy(hw->mac.addr, addr->sa_data, netdev->addr_len);
	rnpgbevf_configure_veb(adapter);

	return 0;
}

void rnpgbevf_remove_mbx_irq(struct rnpgbevf_adapter *adapter)
{
	u32 msgbuf[2];
	struct rnpgbevf_hw *hw = &adapter->hw;

	spin_lock_bh(&adapter->mbx_lock);
	set_bit(__RNPVF_MBX_POLLING, &adapter->state);
	msgbuf[0] = RNPGBE_PF_REMOVE;
	adapter->hw.mbx.ops.write_posted(hw, msgbuf, 1, false);
	clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
	spin_unlock_bh(&adapter->mbx_lock);

	mdelay(100);

	/* mbx */
	if (adapter->flags & RNPVF_FLAG_MSIX_ENABLED) {
		adapter->hw.mbx.ops.configure(
			&adapter->hw, adapter->msix_entries[0].entry, false);
		free_irq(adapter->msix_entries[0].vector, adapter);
	}
}

static void rnp_get_link_status(struct rnpgbevf_adapter *adapter)
{
	struct rnpgbevf_hw *hw = &adapter->hw;
	u32 msgbuf[3];
	s32 ret_val = -1;

	spin_lock_bh(&adapter->mbx_lock);
	set_bit(__RNPVF_MBX_POLLING, &adapter->state);
	msgbuf[0] = RNPGBE_PF_GET_LINK;
	adapter->hw.mbx.ops.write_posted(hw, msgbuf, 1, false);
	mdelay(2);
	ret_val = adapter->hw.mbx.ops.read_posted(hw, msgbuf, 2, false);
	if (ret_val == 0) {
		if (msgbuf[1] & RNPGBE_PF_LINK_UP) {
			hw->link = true;
			hw->speed = msgbuf[1] & 0xffff;

		} else {
			hw->link = false;
			hw->speed = 0;
		}
	} else {
		printk(KERN_DEBUG "[rpnvf] error! mbx GET_LINK faild!\n");
	}
	clear_bit(__RNPVF_MBX_POLLING, &adapter->state);
	spin_unlock_bh(&adapter->mbx_lock);
}

int rnpgbevf_register_mbx_irq(struct rnpgbevf_adapter *adapter)
{
	struct rnpgbevf_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	int err = 0;

	/* for mbx:vector0 */
	if (adapter->flags & RNPVF_FLAG_MSIX_ENABLED) {
		err = request_irq(adapter->msix_entries[0].vector,
				  rnpgbevf_msix_other, 0, netdev->name,
				  adapter);
		if (err) {
			dev_err(&adapter->pdev->dev,
				"request_irq for msix_other failed: %d\n", err);
			goto err_mbx;
		}
		hw->mbx.ops.configure(hw, adapter->msix_entries[0].entry, true);
	}

	rnp_get_link_status(adapter);
err_mbx:
	return err;
}

static int rnpgbevf_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct rnpgbevf_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
#ifdef CONFIG_PM
	int retval = 0;
#endif

	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		rtnl_lock();
		rnpgbevf_down(adapter);
		rnpgbevf_free_irq(adapter);
		rnpgbevf_free_all_tx_resources(adapter);
		rnpgbevf_free_all_rx_resources(adapter);
		rtnl_unlock();
	}

	rnpgbevf_remove_mbx_irq(adapter);
	rnpgbevf_clear_interrupt_scheme(adapter);

#ifdef CONFIG_PM
	retval = pci_save_state(pdev);
	if (retval)
		return retval;

#endif
	pci_disable_device(pdev);

	return 0;
}

#ifdef CONFIG_PM
static int rnpgbevf_resume(struct pci_dev *pdev)
{
	struct rnpgbevf_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
	u32 err;

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
	/*
	 * pci_restore_state clears dev->state_saved so call
	 * pci_save_state to restore it.
	 */
	pci_save_state(pdev);

	err = pcim_enable_device(pdev);
	if (err) {
		dev_err(&pdev->dev, "Cannot enable PCI device from suspend\n");
		return err;
	}
	pci_set_master(pdev);

	rtnl_lock();
	err = rnpgbevf_init_interrupt_scheme(adapter);
	rtnl_unlock();
	rnpgbevf_register_mbx_irq(adapter);

	if (err) {
		dev_err(&pdev->dev, "Cannot initialize interrupts\n");
		return err;
	}

	rnpgbevf_reset(adapter);

	if (netif_running(netdev)) {
		err = rnpgbevf_open(netdev);
		if (err)
			return err;
	}

	netif_device_attach(netdev);

	return err;
}
#endif /* CONFIG_PM */

static void rnpgbevf_shutdown(struct pci_dev *pdev)
{
	rnpgbevf_suspend(pdev, PMSG_SUSPEND);
}

static void rnpgbevf_get_stats64(struct net_device *netdev,
				 struct rtnl_link_stats64 *stats)
{
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);
	int i;
	u64 ring_csum_err = 0;
	u64 ring_csum_good = 0;

	rcu_read_lock();
	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct rnpgbevf_ring *ring = adapter->rx_ring[i];
		u64 bytes, packets;
		unsigned int start;

		if (ring) {
			do {
				start = u64_stats_fetch_begin(&ring->syncp);
				packets = ring->stats.packets;
				bytes = ring->stats.bytes;
				ring_csum_err += ring->rx_stats.csum_err;
				ring_csum_good += ring->rx_stats.csum_good;
			} while (u64_stats_fetch_retry(&ring->syncp, start));
			stats->rx_packets += packets;
			stats->rx_bytes += bytes;
		}
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct rnpgbevf_ring *ring = adapter->tx_ring[i];
		u64 bytes, packets;
		unsigned int start;

		if (ring) {
			do {
				start = u64_stats_fetch_begin(&ring->syncp);
				packets = ring->stats.packets;
				bytes = ring->stats.bytes;
			} while (u64_stats_fetch_retry(&ring->syncp, start));
			stats->tx_packets += packets;
			stats->tx_bytes += bytes;
		}
	}
	rcu_read_unlock();
	/* following stats updated by rnp_watchdog_task() */
	stats->multicast = netdev->stats.multicast;
	stats->rx_errors = netdev->stats.rx_errors;
	stats->rx_length_errors = netdev->stats.rx_length_errors;
	stats->rx_crc_errors = netdev->stats.rx_crc_errors;
	stats->rx_missed_errors = netdev->stats.rx_missed_errors;

}

#define RNPGBE_MAX_TUNNEL_HDR_LEN 80
#define RNPGBE_MAX_MAC_HDR_LEN 127
#define RNPGBE_MAX_NETWORK_HDR_LEN 511

static netdev_features_t rnpgbevf_features_check(struct sk_buff *skb,
						 struct net_device *dev,
						 netdev_features_t features)
{
	unsigned int network_hdr_len, mac_hdr_len;

	/* Make certain the headers can be described by a context descriptor */
	mac_hdr_len = skb_network_header(skb) - skb->data;
	if (unlikely(mac_hdr_len > RNPGBE_MAX_MAC_HDR_LEN))
		return features &
		       ~(NETIF_F_HW_CSUM | NETIF_F_SCTP_CRC |
			 NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_TSO | NETIF_F_TSO6);

	network_hdr_len = skb_checksum_start(skb) - skb_network_header(skb);
	if (unlikely(network_hdr_len > RNPGBE_MAX_NETWORK_HDR_LEN))
		return features & ~(NETIF_F_HW_CSUM | NETIF_F_SCTP_CRC |
				    NETIF_F_TSO | NETIF_F_TSO6);

	/* We can only support IPV4 TSO in tunnels if we can mangle the
	 * inner IP ID field, so strip TSO if MANGLEID is not supported.
	 */
	if (skb->encapsulation && !(features & NETIF_F_TSO_MANGLEID))
		features &= ~NETIF_F_TSO;

	return features;
}

static const struct net_device_ops rnpgbevf_netdev_ops = {
	.ndo_open = rnpgbevf_open,
	.ndo_stop = rnpgbevf_close,
	.ndo_start_xmit = rnpgbevf_xmit_frame,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_get_stats64 = rnpgbevf_get_stats64,
	.ndo_set_rx_mode = rnpgbevf_set_rx_mode,
	.ndo_set_mac_address = rnpgbevf_set_mac,
	.ndo_change_mtu = rnpgbevf_change_mtu,
	.ndo_vlan_rx_add_vid = rnpgbevf_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = rnpgbevf_vlan_rx_kill_vid,
	.ndo_features_check = rnpgbevf_features_check,
	.ndo_set_features = rnpgbevf_set_features,
	.ndo_fix_features = rnpgbevf_fix_features,
};

void rnpgbevf_assign_netdev_ops(struct net_device *dev)
{
	/* different hw can assign difference fun */
	dev->netdev_ops = &rnpgbevf_netdev_ops;
	rnpgbevf_set_ethtool_ops(dev);
	dev->watchdog_timeo = 5 * HZ;
}

static u8 rnpgbevf_vfnum_n500(struct rnpgbevf_hw *hw)
{
	u16 vf_num;

	vf_num = readl(hw->hw_addr + VF_NUM_REG_N500);
#define VF_NUM_MASK_N500 (0xff)

	return (vf_num & VF_NUM_MASK_N500);
}

#ifdef CONFIG_NET_NCSI
static void rnpgbevf_ncsi_notify_handler(struct ncsi_dev *nd)
{
	if (unlikely(nd->state != ncsi_dev_state_functional))
		return;

	netdev_dbg(nd->dev, "NCSI interface %s\n", nd->link_up ? "up" : "down");
}
#endif

static inline unsigned long rnpgbevf_tso_features(struct rnpgbevf_hw *hw)
{
	unsigned long features = 0;

	if (hw->feature_flags & RNPVF_NET_FEATURE_TSO)
		features |= NETIF_F_TSO;
	if (hw->feature_flags & RNPVF_NET_FEATURE_TSO)
		features |= NETIF_F_TSO6;
	features |= NETIF_F_GSO_PARTIAL;
	if (hw->feature_flags & RNPVF_NET_FEATURE_TX_UDP_TUNNEL)
		features |= RNPVF_GSO_PARTIAL_FEATURES;

	return features;
}

static int rnpgbevf_add_adpater(struct pci_dev *pdev,
				const struct rnpgbevf_info *ii,
				struct rnpgbevf_adapter **padapter)
{
	int err = 0;
	struct rnpgbevf_adapter *adapter = NULL;
	struct net_device *netdev;
	struct rnpgbevf_hw *hw;
	unsigned int queues = MAX_TX_QUEUES;
	static int pf0_cards_found;
	static int pf1_cards_found;
	static int pf2_cards_found;
	static int pf3_cards_found;

	pr_info("====  add adapter queues:%d ====", queues);

	netdev = alloc_etherdev_mq(sizeof(struct rnpgbevf_adapter), queues);
	if (!netdev)
		return -ENOMEM;

	SET_NETDEV_DEV(netdev, &pdev->dev);

	adapter = netdev_priv(netdev);
	adapter->netdev = netdev;
	adapter->pdev = pdev;
	/* setup some status */

	if (padapter)
		*padapter = adapter;
	pci_set_drvdata(pdev, adapter);

	hw = &adapter->hw;
	hw->back = adapter;
	hw->pdev = pdev;
	hw->board_type = ii->board_type;
	adapter->msg_enable = netif_msg_init(debug, NETIF_MSG_DRV
#ifdef MSG_PROBE_ENABLE
							    | NETIF_MSG_PROBE
#endif
#ifdef MSG_IFUP_ENABLE
							    | NETIF_MSG_IFUP
#endif
#ifdef MSG_IFDOWN_ENABLE
							    | NETIF_MSG_IFDOWN
#endif
	);

	switch (ii->mac) {
	case rnp_mac_2port_10G:
		hw->mode = MODE_NIC_MODE_2PORT_10G;
		break;
	case rnp_mac_2port_40G:
		hw->mode = MODE_NIC_MODE_2PORT_40G;
		break;
	case rnp_mac_4port_10G:
		hw->mode = MODE_NIC_MODE_4PORT_10G;
		break;
	case rnp_mac_8port_10G:
		hw->mode = MODE_NIC_MODE_8PORT_10G;
		break;
	default:
		break;
	}

	switch (hw->board_type) {
	case rnp_board_n500:
#define RNPGBE_N500_BAR 2
		hw->hw_addr = pcim_iomap(pdev, RNPGBE_N500_BAR, 0);
		if (!hw->hw_addr) {
			err = -EIO;
			goto err_ioremap;
		}
		dev_info(&pdev->dev, "[bar%d]:%p %llx len=%d kB\n",
			 RNPGBE_N500_BAR, hw->hw_addr,
			 (unsigned long long)pci_resource_start(
				 pdev, RNPGBE_N500_BAR),
			 (int)pci_resource_len(pdev, RNPGBE_N500_BAR) / 1024);
		hw->vfnum = rnpgbevf_vfnum_n500(hw);
		hw->ring_msix_base = hw->hw_addr + 0x24700;

		switch ((hw->vfnum & 0x60) >> 5) {
		case 0x00:
			adapter->port = adapter->bd_number = pf0_cards_found++;
			if (pf0_cards_found == 1000)
				pf0_cards_found = 0;
			break;
		case 0x01:
			adapter->port = adapter->bd_number = pf1_cards_found++;
			if (pf1_cards_found == 1000)
				pf1_cards_found = 0;
			break;
		case 0x02:
			adapter->port = adapter->bd_number = pf2_cards_found++;
			if (pf2_cards_found == 1000)
				pf2_cards_found = 0;
			break;
		case 0x03:
			adapter->port = adapter->bd_number = pf3_cards_found++;
			if (pf3_cards_found == 1000)
				pf3_cards_found = 0;
			break;
		}
		snprintf(adapter->name, sizeof(netdev->name), "%s%d%d",
			 rnpgbevf_driver_name, (hw->vfnum & 0x60) >> 5,
			 adapter->bd_number);

		adapter->irq_mode = irq_mode_msix;
		break;

	case rnp_board_n210:
#define RNPGBE_N210_BAR 2
		hw->hw_addr = pcim_iomap(pdev, RNPGBE_N210_BAR, 0);
		if (!hw->hw_addr) {
			err = -EIO;
			goto err_ioremap;
		}
		dev_info(&pdev->dev, "[bar%d]:%p %llx len=%d kB\n",
			 RNPGBE_N210_BAR, hw->hw_addr,
			 (unsigned long long)pci_resource_start(
				 pdev, RNPGBE_N210_BAR),
			 (int)pci_resource_len(pdev, RNPGBE_N210_BAR) / 1024);
		hw->vfnum = rnpgbevf_vfnum_n500(hw);
		hw->ring_msix_base = hw->hw_addr + 0x25000;

		switch ((hw->vfnum & 0x60) >> 5) {
		case 0x00:
			adapter->port = adapter->bd_number = pf0_cards_found++;
			if (pf0_cards_found == 1000)
				pf0_cards_found = 0;
			break;
		case 0x01:
			adapter->port = adapter->bd_number = pf1_cards_found++;
			if (pf1_cards_found == 1000)
				pf1_cards_found = 0;
			break;
		case 0x02:
			adapter->port = adapter->bd_number = pf2_cards_found++;
			if (pf2_cards_found == 1000)
				pf2_cards_found = 0;
			break;
		case 0x03:
			adapter->port = adapter->bd_number = pf3_cards_found++;
			if (pf3_cards_found == 1000)
				pf3_cards_found = 0;
			break;
		}
		snprintf(adapter->name, sizeof(netdev->name), "%s%d%d",
			 rnpgbevf_driver_name, (hw->vfnum & 0x60) >> 5,
			 adapter->bd_number);

		adapter->irq_mode = irq_mode_msix;
		break;
	default:
		printk("card type error\n");
		goto err_ioremap;

		break;
	}

	pr_info("%s %s: vfnum:0x%x\n", adapter->name, pci_name(pdev),
		hw->vfnum);

	rnpgbevf_assign_netdev_ops(netdev);
	strncpy(netdev->name, adapter->name, sizeof(netdev->name) - 1);

	/* Setup hw api */
	memcpy(&hw->mac.ops, ii->mac_ops, sizeof(hw->mac.ops));
	hw->mac.type = ii->mac;

	ii->get_invariants(hw);

	memcpy(&hw->mbx.ops, &rnpgbevf_mbx_ops,
	       sizeof(struct rnp_mbx_operations));

	/* setup the private structure */
	err = rnpgbevf_sw_init(adapter);
	if (err)
		goto err_sw_init;

	/* The HW MAC address was set and/or determined in sw_init */
	if (!is_valid_ether_addr(netdev->dev_addr)) {
		pr_err("invalid MAC address\n");
		err = -EIO;
		goto err_sw_init;
	}
	/* MTU range: 68 - 9710 */
	netdev->min_mtu = hw->min_length;
	netdev->max_mtu = hw->max_length - (ETH_HLEN + 2 * ETH_FCS_LEN);

	netdev->mtu = hw->mtu;

	if (hw->feature_flags & RNPVF_NET_FEATURE_SG)
		netdev->features |= NETIF_F_SG;
	if (hw->feature_flags & RNPVF_NET_FEATURE_TSO)
		netdev->features |= NETIF_F_TSO | NETIF_F_TSO6;
	if (hw->feature_flags & RNPVF_NET_FEATURE_RX_HASH)
		netdev->features |= NETIF_F_RXHASH;
	if (hw->feature_flags & RNPVF_NET_FEATURE_RX_CHECKSUM) {
		netdev->features |= NETIF_F_RXCSUM;
		adapter->flags |= RNPVF_FLAG_RX_CHKSUM_ENABLED;
	}
	if (hw->feature_flags & RNPVF_NET_FEATURE_TX_CHECKSUM)
		netdev->features |= NETIF_F_HW_CSUM | NETIF_F_SCTP_CRC;
	if (hw->feature_flags & RNPVF_NET_FEATURE_USO)
		netdev->features |= NETIF_F_GSO_UDP_L4;

	netdev->features |= NETIF_F_HIGHDMA;

	if (hw->feature_flags & RNPVF_NET_FEATURE_TX_UDP_TUNNEL) {
		netdev->gso_partial_features = RNPVF_GSO_PARTIAL_FEATURES;
		netdev->features |=
			NETIF_F_GSO_PARTIAL | RNPVF_GSO_PARTIAL_FEATURES;
	}

	netdev->hw_features |= netdev->features;

	if (hw->feature_flags & RNPVF_NET_FEATURE_VLAN_FILTER) {
		netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_FILTER;
		netdev->hw_features |= NETIF_F_HW_VLAN_STAG_FILTER;
	}
	if (hw->feature_flags & RNPVF_NET_FEATURE_VLAN_OFFLOAD) {
		if (!(hw->pf_feature & PF_NCSI_EN)) {
			netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_RX;
			netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_TX;
		}
	}

	if (hw->feature_flags & RNPVF_NET_FEATURE_STAG_OFFLOAD) {
		if (!(hw->pf_feature & PF_NCSI_EN)) {
			netdev->hw_features |= NETIF_F_HW_VLAN_STAG_RX;
			netdev->hw_features |= NETIF_F_HW_VLAN_STAG_TX;
		}
	}
	netdev->hw_features |= NETIF_F_RXALL;
	if (hw->feature_flags & RNPVF_NET_FEATURE_RX_NTUPLE_FILTER)
		netdev->hw_features |= NETIF_F_NTUPLE;
	if (hw->feature_flags & RNPVF_NET_FEATURE_RX_FCS)
		netdev->hw_features |= NETIF_F_RXFCS;

	netdev->vlan_features |= netdev->features | NETIF_F_TSO_MANGLEID;
	netdev->hw_enc_features |= netdev->vlan_features;
	netdev->mpls_features |= NETIF_F_HW_CSUM;

	/* some fixed feature control by pf */
	if (hw->pf_feature & PF_FEATURE_VLAN_FILTER) {
		netdev->features |= NETIF_F_HW_VLAN_CTAG_FILTER;
		netdev->features |= NETIF_F_HW_VLAN_STAG_FILTER;
	}

	if (hw->feature_flags & RNPVF_NET_FEATURE_VLAN_OFFLOAD) {
		if (!(hw->pf_feature & PF_NCSI_EN)) {
			netdev->features |= NETIF_F_HW_VLAN_CTAG_RX;
			netdev->features |= NETIF_F_HW_VLAN_CTAG_TX;
		}
	}

	if (hw->feature_flags & RNPVF_NET_FEATURE_STAG_OFFLOAD) {
		if (!(hw->pf_feature & PF_NCSI_EN)) {
			netdev->features |= NETIF_F_HW_VLAN_STAG_RX;
			netdev->features |= NETIF_F_HW_VLAN_STAG_TX;
		}
	}

	netdev->priv_flags |= IFF_UNICAST_FLT;
	netdev->priv_flags |= IFF_SUPP_NOFCS;


	netdev->priv_flags |= IFF_UNICAST_FLT;
	netdev->priv_flags |= IFF_SUPP_NOFCS;

	timer_setup(&adapter->watchdog_timer, rnpgbevf_watchdog, 0);
	INIT_WORK(&adapter->watchdog_task, rnpgbevf_watchdog_task);
	err = rnpgbevf_init_interrupt_scheme(adapter);
	if (err)
		goto err_sw_init;

	err = rnpgbevf_register_mbx_irq(adapter);
	if (err)
		goto err_register;

	if (fix_eth_name) {
		strncpy(netdev->name, adapter->name, sizeof(netdev->name) - 1);
	} else {
		strscpy(netdev->name, pci_name(pdev), sizeof(netdev->name));
		strscpy(netdev->name, "eth%d", sizeof(netdev->name));
	}
	err = register_netdev(netdev);
	if (err) {
		rnpgbevf_err("register_netdev faild!\n");
		dev_err(&pdev->dev,
			"%s %s: vfnum:0x%x. register_netdev faild!\n",
			adapter->name, pci_name(pdev), hw->vfnum);
		goto err_register;
	}

	/* carrier off reporting is important to ethtool even BEFORE open */
	netif_carrier_off(netdev);

	rnpgbevf_sysfs_init(netdev);

#ifdef CONFIG_NET_NCSI
	if (be_ncsi_mc &&
	    (hw->vfnum & BIT(6)) == 0) {
		adapter->ncsi_dev =
			ncsi_register_dev(netdev, rnpgbevf_ncsi_notify_handler);
		if (adapter->ncsi_dev == NULL) {
			dev_err(&pdev->dev, "ncsi register faild!\n");
			goto err_register;
		}
	}
#endif

	/* print the MAC address */
	hw_dbg(hw, "%pM\n", netdev->dev_addr);

	hw_dbg(hw, "Mucse(R) n10 Virtual Function\n");

	return 0;
err_register:
	rnpgbevf_remove_mbx_irq(adapter);
	rnpgbevf_clear_interrupt_scheme(adapter);
err_sw_init:
err_ioremap:
	free_netdev(netdev);

	dev_err(&pdev->dev, "%s faild. err:%d\n", __func__, err);
	return err;
}

static int rnpgbevf_rm_adpater(struct rnpgbevf_adapter *adapter)
{
	struct net_device *netdev;

	if (!adapter)
		return -EINVAL;

	rnpgbevf_info("= remove adapter:%s =\n", adapter->name);
	netdev = adapter->netdev;

#ifdef CONFIG_NET_NCSI
	if (adapter->ncsi_dev) {
		ncsi_unregister_dev(adapter->ncsi_dev);
		adapter->ncsi_dev = NULL;
	}
#endif

	if (netdev) {
		netif_carrier_off(netdev);
		rnpgbevf_sysfs_exit(netdev);
	}

	set_bit(__RNPVF_REMOVE, &adapter->state);
	del_timer_sync(&adapter->watchdog_timer);

	cancel_work_sync(&adapter->watchdog_task);

#ifdef CONFIG_NET_NCSI
	if (adapter->ncsi_dev)
		ncsi_unregister_dev(adapter->ncsi_dev);
#endif

	if (netdev) {
		if (netdev->reg_state == NETREG_REGISTERED)
			unregister_netdev(netdev);
	}

	rnpgbevf_remove_mbx_irq(adapter);
	rnpgbevf_clear_interrupt_scheme(adapter);
	rnpgbevf_reset_interrupt_capability(adapter);

	free_netdev(netdev);

	rnpgbevf_info("remove %s  complete\n", adapter->name);

	return 0;
}

/**
 * rnpgbevf_probe - Device Initialization Routine
 * @pdev: PCI device information struct
 * @ent: entry in rnpgbevf_pci_tbl
 *
 * Returns 0 on success, negative on failure
 *
 * rnpgbevf_probe initializes an adapter identified by a pci_dev structure.
 * The OS initialization, configuring of the adapter private structure,
 * and a hardware reset occur.
 **/
static int rnpgbevf_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct rnpgbevf_adapter *adapter = NULL;
	const struct rnpgbevf_info *ii = rnpgbevf_info_tbl[ent->driver_data];
	int err;

	err = pci_enable_device_mem(pdev);
	if (err)
		return err;

	if (!dma_set_mask(&pdev->dev, DMA_BIT_MASK(56)) &&
	    !dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(56))) {
		pci_using_hi_dma = 1;
	} else {
		err = dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));
		if (err) {
			err = dma_set_coherent_mask(&pdev->dev,
						    DMA_BIT_MASK(32));
			if (err) {
				dev_err(&pdev->dev,
					"No usable DMA "
					"configuration, aborting\n");
				goto err_dma;
			}
		}
		pci_using_hi_dma = 0;
	}

	err = pci_request_mem_regions(pdev, rnpgbevf_driver_name);
	if (err) {
		dev_err(&pdev->dev,
			"pci_request_selected_regions failed 0x%x\n", err);
		goto err_pci_reg;
	}

	pci_set_master(pdev);
	pci_save_state(pdev);

	err = rnpgbevf_add_adpater(pdev, ii, &adapter);
	if (err) {
		dev_err(&pdev->dev, "ERROR %s: %d\n", __func__, __LINE__);
		goto err_regions;
	}

	return 0;

err_regions:
	pci_release_mem_regions(pdev);
err_dma:
err_pci_reg:
	return err;
}

/**
 * rnpgbevf_remove - Device Removal Routine
 * @pdev: PCI device information struct
 *
 * rnpgbevf_remove is called by the PCI subsystem to alert the driver
 * that it should release a PCI device.  The could be caused by a
 * Hot-Plug event, or because the driver is going to be removed from
 * memory.
 **/
static void rnpgbevf_remove(struct pci_dev *pdev)
{
	struct rnpgbevf_adapter *adapter = pci_get_drvdata(pdev);

	rnpgbevf_rm_adpater(adapter);
	pci_release_mem_regions(pdev);
	pci_disable_device(pdev);
}

/**
 * rnpgbevf_io_error_detected - called when PCI error is detected
 * @pdev: Pointer to PCI device
 * @state: The current pci connection state
 *
 * This function is called after a PCI bus error affecting
 * this device has been detected.
 */
static pci_ers_result_t rnpgbevf_io_error_detected(struct pci_dev *pdev,
						   pci_channel_state_t state)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);

	netif_device_detach(netdev);

	if (state == pci_channel_io_perm_failure)
		return PCI_ERS_RESULT_DISCONNECT;

	if (netif_running(netdev))
		rnpgbevf_down(adapter);

	pci_disable_device(pdev);

	/* Request a slot reset. */
	return PCI_ERS_RESULT_NEED_RESET;
}

/**
 * rnpgbevf_io_slot_reset - called after the pci bus has been reset.
 * @pdev: Pointer to PCI device
 *
 * Restart the card from scratch, as if from a cold-boot. Implementation
 * resembles the first-half of the rnpgbevf_resume routine.
 */
static pci_ers_result_t rnpgbevf_io_slot_reset(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);

	if (pci_enable_device_mem(pdev)) {
		dev_err(&pdev->dev,
			"Cannot re-enable PCI device after reset.\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}

	pci_set_master(pdev);

	rnpgbevf_reset(adapter);

	return PCI_ERS_RESULT_RECOVERED;
}

/**
 * rnpgbevf_io_resume - called when traffic can start flowing again.
 * @pdev: Pointer to PCI device
 *
 * This callback is called when the error recovery driver tells us that
 * its OK to resume normal operation. Implementation resembles the
 * second-half of the rnpgbevf_resume routine.
 */
static void rnpgbevf_io_resume(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct rnpgbevf_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev))
		rnpgbevf_up(adapter);

	netif_device_attach(netdev);
}

/* PCI Error Recovery (ERS) */
static const struct pci_error_handlers rnpgbevf_err_handler = {
	.error_detected = rnpgbevf_io_error_detected,
	.slot_reset = rnpgbevf_io_slot_reset,
	.resume = rnpgbevf_io_resume,
};

static struct pci_driver rnpgbevf_driver = {
	.name = rnpgbevf_driver_name,
	.id_table = rnpgbevf_pci_tbl,
	.probe = rnpgbevf_probe,
	.remove = rnpgbevf_remove,
#ifdef CONFIG_PM
	/* Power Management Hooks */
	.suspend = rnpgbevf_suspend,
	.resume = rnpgbevf_resume,
#endif
	.shutdown = rnpgbevf_shutdown,
	.err_handler = &rnpgbevf_err_handler,
};

/**
 * rnpgbevf_init_module - Driver Registration Routine
 *
 * rnpgbevf_init_module is the first routine called when the driver is
 * loaded. All it does is register with the PCI subsystem.
 **/
static int __init rnpgbevf_init_module(void)
{
	int ret;

	pr_info("%s - version %s\n", rnpgbevf_driver_string,
		rnpgbevf_driver_version);

	pr_info("%s\n", rnpgbevf_copyright);

	ret = pci_register_driver(&rnpgbevf_driver);
	return ret;
}

module_init(rnpgbevf_init_module);

/**
 * rnpgbevf_exit_module - Driver Exit Cleanup Routine
 *
 * rnpgbevf_exit_module is called just before the driver is removed
 * from memory.
 **/
static void __exit rnpgbevf_exit_module(void)
{
	pci_unregister_driver(&rnpgbevf_driver);
}

module_exit(rnpgbevf_exit_module);
