#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>
#include <linux/pagemap.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/poll.h>
//#include <linux/version.h>

#include <linux/tcp.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#ifdef NETIF_F_TSO
#include <net/checksum.h>
#endif
#include <linux/ethtool.h>
#include <linux/prefetch.h>

#include "grtnic.h"
#include "grtnic_nvm.h"
#include "grtnic_macphy.h"

/* only works for sizes that are powers of 2 */
#define GRTNIC_ROUNDUP_SIZE(i, size) (  (size) - ((i) & ((size) - 1)) )

static void grtnic_clean_tx_ring(struct grtnic_ring *tx_ring);
static void grtnic_clean_rx_ring(struct grtnic_ring *rx_ring);

#ifdef NETIF_F_RXHASH
static inline void grtnic_rx_hash(struct grtnic_ring *ring, union grtnic_rx_desc *rx_desc, struct sk_buff *skb)
{
	u16 rss_type; 

	if (!(netdev_ring(ring)->features & NETIF_F_RXHASH))
		return;

	rss_type = le16_to_cpu(rx_desc->wb.lower.lo_dword.hs_rss.pkt_info) & 0x0f;

	if (!rss_type)
		return;

	skb_set_hash(skb, le32_to_cpu(rx_desc->wb.lower.hi_dword.rss),
		     (rss_type & 0xc0) ? //tcp or udp
		     PKT_HASH_TYPE_L4 : PKT_HASH_TYPE_L3);
}
#endif /* NETIF_F_RXHASH */

static int grtnic_desc_unused(struct grtnic_ring *ring)
{
	if (ring->next_to_clean > ring->next_to_use)
		return ring->next_to_clean - ring->next_to_use - 1;

	return ring->count + ring->next_to_clean - ring->next_to_use - 1;
}

static inline void grtnic_release_rx_desc(struct grtnic_ring *rx_ring, u32 val)
{
	rx_ring->next_to_use = val;
#ifndef CONFIG_DISABLE_PACKET_SPLIT

	/* update next to alloc since we have filled the ring */
	rx_ring->next_to_alloc = val;
#endif
	/*
	 * Force memory writes to complete before letting h/w
	 * know there are new descriptors to fetch.  (Only
	 * applicable for weak-ordered memory model archs,
	 * such as IA-64).
	 */
  wmb();
  writel(val, rx_ring->tail); //rx_ring->tail, 这个地方别忘记设置，desc要在clean_rx_irq里面清0
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef CONFIG_DISABLE_PACKET_SPLIT
static bool grtnic_alloc_mapped_skb(struct grtnic_ring *rx_ring, struct grtnic_rx_buffer *buffer_info)
{
  struct sk_buff *skb = buffer_info->skb;
  dma_addr_t dma = buffer_info->dma;

	if (unlikely(dma))
		return true;

  if (likely(!skb)) {
    skb = netdev_alloc_skb_ip_align(netdev_ring(rx_ring), rx_ring->rx_buffer_len); 

		if (unlikely(!skb)) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			return false;
		}
    buffer_info->skb = skb;
  }

  dma = dma_map_single(rx_ring->dev, skb->data, rx_ring->rx_buffer_len, DMA_FROM_DEVICE);
	/*
	 * if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		dev_kfree_skb_any(skb);
		buffer_info->skb = NULL;

		rx_ring->rx_stats.alloc_rx_buff_failed++;
		return false;
	}

  buffer_info->dma = dma;
  buffer_info->length = rx_ring->rx_buffer_len;
  return true;
}

#else /* CONFIG_DISABLE_PACKET_SPLIT */

static inline unsigned int grtnic_rx_offset(struct grtnic_ring *rx_ring)
{
	return ring_uses_build_skb(rx_ring) ? GRTNIC_SKB_PAD : 0;
}

static bool grtnic_alloc_mapped_page(struct grtnic_ring *rx_ring, struct grtnic_rx_buffer *buffer_info)
{
  struct page *page = buffer_info->page;
  dma_addr_t dma;
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
	DEFINE_DMA_ATTRS(attrs);

	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif

  /* since we are recycling buffers we should seldom need to alloc */
  if (likely(page))
    return true;

  /* alloc new page for storage */
	page = dev_alloc_pages(grtnic_rx_pg_order(rx_ring));
	if (unlikely(!page)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

  /* map page for use */
	dma = dma_map_page_attrs(rx_ring->dev, page, 0, grtnic_rx_pg_size(rx_ring),
				 DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
				 &attrs);
#else
				 GRTNIC_RX_DMA_ATTR);
#endif

	/*
	 * if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		__free_pages(page, grtnic_rx_pg_order(rx_ring));

		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

  buffer_info->dma = dma;
  buffer_info->page = page;
  buffer_info->page_offset = grtnic_rx_offset(rx_ring);
#ifdef HAVE_PAGE_COUNT_BULK_UPDATE
	page_ref_add(page, USHRT_MAX - 1);
	buffer_info->pagecnt_bias = USHRT_MAX;
#else
	buffer_info->pagecnt_bias = 1;
#endif
	rx_ring->rx_stats.alloc_rx_page++;
//  buffer_info->length = grtnic_rx_bufsz(rx_ring);
//  buffer_info->length = GRTNIC_RX_BUFSZ; //注意，这里告知asic缓冲区大小不是整个page，因为整个page可能有几个缓冲区

//  printk("offset = %d, length = %d\n", buffer_info->page_offset, buffer_info->length);

  return true;
}
#endif /* CONFIG_DISABLE_PACKET_SPLIT */
/////////////////////////////////////////////////////////////////////////////////////////////////////

void grtnic_alloc_rx_buffers(struct grtnic_ring *rx_ring, u16 cleaned_count)
{
  union grtnic_rx_desc *rx_desc;
  struct grtnic_rx_buffer *buffer_info;
  u16 i = rx_ring->next_to_use;
#ifndef CONFIG_DISABLE_PACKET_SPLIT
	u16 bufsz;
#endif

  /* nothing to do */
  if (!cleaned_count)
    return;

  rx_desc = GRTNIC_RX_DESC(*rx_ring, i);
  buffer_info = &rx_ring->rx_buffer_info[i];
  i -= rx_ring->count;
#ifndef CONFIG_DISABLE_PACKET_SPLIT
	bufsz = grtnic_rx_bufsz(rx_ring);
#endif

  do {
#ifdef CONFIG_DISABLE_PACKET_SPLIT
    if (!grtnic_alloc_mapped_skb(rx_ring, buffer_info))
      break;
#else
    if (!grtnic_alloc_mapped_page(rx_ring, buffer_info))
			break;

		/* sync the buffer for use by the device */
		dma_sync_single_range_for_device(rx_ring->dev, buffer_info->dma,
						 buffer_info->page_offset, bufsz,
						 DMA_FROM_DEVICE);
#endif /* CONFIG_DISABLE_PACKET_SPLIT */

    /*
     * Refresh the desc even if buffer_addrs didn't change
     * because each write-back erases this info.
     */
#ifdef CONFIG_DISABLE_PACKET_SPLIT
    rx_desc->read.src_addr = cpu_to_le64(buffer_info->dma);
    rx_desc->read.len_ctl.len = cpu_to_le16(buffer_info->length);
#else
    rx_desc->read.src_addr = cpu_to_le64(buffer_info->dma + buffer_info->page_offset);
    rx_desc->read.len_ctl.len = cpu_to_le16(bufsz);
#endif
    rx_desc->read.len_ctl.desc_num = 0;
    rx_desc->read.len_ctl.chl = 0;
    rx_desc->read.len_ctl.cmp = 0;
    rx_desc->read.len_ctl.sop = 0;
    rx_desc->read.len_ctl.eop = 0;

    rx_desc++;
    buffer_info++;
    i++;

    if (unlikely(!i)) {
      rx_desc = GRTNIC_RX_DESC(*rx_ring, 0);
      buffer_info = &rx_ring->rx_buffer_info[0];
      i -= rx_ring->count;
    }

    cleaned_count--;
  } while (cleaned_count);

  i += rx_ring->count;

  if (rx_ring->next_to_use != i)
		grtnic_release_rx_desc(rx_ring, i);
}

static inline bool grtnic_container_is_rx(struct grtnic_q_vector *q_vector, struct grtnic_ring_container *rc)
{
	return &q_vector->rx == rc;
}
/**
 * ixgbe_update_itr - update the dynamic ITR value based on statistics
 * @q_vector: structure containing interrupt and ring information
 * @ring_container: structure containing ring performance data
 *
 *      Stores a new ITR value based on packets and byte
 *      counts during the last interrupt.  The advantage of per interrupt
 *      computation is faster updates and more accurate ITR for the current
 *      traffic pattern.  Constants in this function were computed
 *      based on theoretical maximum wire speed and thresholds were set based
 *      on testing data as well as attempting to minimize response time
 *      while increasing bulk throughput.
 **/
static void grtnic_update_itr(struct grtnic_q_vector *q_vector, struct grtnic_ring_container *ring_container)
{
	unsigned int itr = ITR_ADAPTIVE_MIN_USECS | ITR_ADAPTIVE_LATENCY;
	unsigned int avg_wire_size, packets, bytes;
	unsigned long next_update = jiffies;

	/* If we don't have any rings just leave ourselves set for maximum
	 * possible latency so we take ourselves out of the equation.
	 */
	if (!ring_container->ring)
		return;

	/* If we didn't update within up to 1 - 2 jiffies we can assume
	 * that either packets are coming in so slow there hasn't been
	 * any work, or that there is so much work that NAPI is dealing
	 * with interrupt moderation and we don't need to do anything.
	 */
	if (time_after(next_update, ring_container->next_update))
		goto clear_counts;

	packets = ring_container->total_packets;
	bytes = ring_container->total_bytes;

	if (grtnic_container_is_rx(q_vector, ring_container)) {
		/* If Rx and there are 1 to 23 packets and bytes are less than
		 * 12112 assume insufficient data to use bulk rate limiting
		 * approach. Instead we will focus on simply trying to target
		 * receiving 8 times as much data in the next interrupt.
		 */
		if (packets && packets < 24 && bytes < 12112) {
			itr = ITR_ADAPTIVE_LATENCY;
			avg_wire_size = (bytes + packets * 24) * 2;
			avg_wire_size = clamp_t(unsigned int, avg_wire_size, 2560, 12800);
			goto adjust_for_speed;
		}
	}

	/* Less than 48 packets we can assume that our current interrupt delay
	 * is only slightly too low. As such we should increase it by a small
	 * fixed amount.
	 */
	if (packets < 48) {
		itr = (q_vector->itr >> 2) + ITR_ADAPTIVE_MIN_INC;
		if (itr > ITR_ADAPTIVE_MAX_USECS)
			itr = ITR_ADAPTIVE_MAX_USECS;

		/* If sample size is 0 - 7 we should probably switch
		 * to latency mode instead of trying to control
		 * things as though we are in bulk.
		 *
		 * Otherwise if the number of packets is less than 48
		 * we should maintain whatever mode we are currently
		 * in. The range between 8 and 48 is the cross-over
		 * point between latency and bulk traffic.
		 */
		if (packets < 8)
			itr += ITR_ADAPTIVE_LATENCY;
		else
			itr += ring_container->itr & ITR_ADAPTIVE_LATENCY;
		goto clear_counts;
	}

	/* Between 48 and 96 is our "goldilocks" zone where we are working
	 * out "just right". Just report that our current ITR is good for us.
	 */
	if (packets < 96) {
		itr = q_vector->itr >> 2;
		goto clear_counts;
	}

	/* If packet count is 96 or greater we are likely looking at a slight
	 * overrun of the delay we want. Try halving our delay to see if that
	 * will cut the number of packets in half per interrupt.
	 */
	if (packets < 256) {
		itr = q_vector->itr >> 3;
		if (itr < ITR_ADAPTIVE_MIN_USECS)
			itr = ITR_ADAPTIVE_MIN_USECS;
		goto clear_counts;
	}

	/* The paths below assume we are dealing with a bulk ITR since number
	 * of packets is 256 or greater. We are just going to have to compute
	 * a value and try to bring the count under control, though for smaller
	 * packet sizes there isn't much we can do as NAPI polling will likely
	 * be kicking in sooner rather than later.
	 */
	itr = ITR_ADAPTIVE_BULK;

	/* If packet counts are 256 or greater we can assume we have a gross
	 * overestimation of what the rate should be. Instead of trying to fine
	 * tune it just use the formula below to try and dial in an exact value
	 * give the current packet size of the frame.
	 */
	avg_wire_size = bytes / packets;

	/* The following is a crude approximation of:
	 *  wmem_default / (size + overhead) = desired_pkts_per_int
	 *  rate / bits_per_byte / (size + ethernet overhead) = pkt_rate
	 *  (desired_pkt_rate / pkt_rate) * usecs_per_sec = ITR value
	 *
	 * Assuming wmem_default is 212992 and overhead is 640 bytes per
	 * packet, (256 skb, 64 headroom, 320 shared info), we can reduce the
	 * formula down to
	 *
	 *  (170 * (size + 24)) / (size + 640) = ITR
	 *
	 * We first do some math on the packet size and then finally bitshift
	 * by 8 after rounding up. We also have to account for PCIe link speed
	 * difference as ITR scales based on this.
	 */
	if (avg_wire_size <= 60) {
		/* Start at 50k ints/sec */
		avg_wire_size = 5120;
	} else if (avg_wire_size <= 316) {
		/* 50K ints/sec to 16K ints/sec */
		avg_wire_size *= 40;
		avg_wire_size += 2720;
	} else if (avg_wire_size <= 1084) {
		/* 16K ints/sec to 9.2K ints/sec */
		avg_wire_size *= 15;
		avg_wire_size += 11452;
	} else if (avg_wire_size <= 1980) {
		/* 9.2K ints/sec to 8K ints/sec */
		avg_wire_size *= 5;
		avg_wire_size += 22420;
	} else {
		/* plateau at a limit of 8K ints/sec */
		avg_wire_size = 32256;
	}

adjust_for_speed:
	/* Resultant value is 256 times larger than it needs to be. This
	 * gives us room to adjust the value as needed to either increase
	 * or decrease the value based on link speeds of 10G, 2.5G, 1G, etc.
	 *
	 * Use addition as we have already recorded the new latency flag
	 * for the ITR value.
	 */

	if (q_vector->adapter->speed == 1) //10G
		itr += DIV_ROUND_UP(avg_wire_size, ITR_ADAPTIVE_MIN_INC * 256) * ITR_ADAPTIVE_MIN_INC;
	else //1G
		itr += DIV_ROUND_UP(avg_wire_size, ITR_ADAPTIVE_MIN_INC * 64) * ITR_ADAPTIVE_MIN_INC;

	/* In the case of a latency specific workload only allow us to
	 * reduce the ITR by at most 2us. By doing this we should dial
	 * in so that our number of interrupts is no more than 2x the number
	 * of packets for the least busy workload. So for example in the case
	 * of a TCP worload the ack packets being received would set the
	 * the interrupt rate as they are a latency specific workload.
	 */
	if ((itr & ITR_ADAPTIVE_LATENCY) && itr < ring_container->itr)
		itr = ring_container->itr - ITR_ADAPTIVE_MIN_INC;

clear_counts:
	/* write back value */
	ring_container->itr = itr;

	/* next update should occur within next jiffy */
	ring_container->next_update = next_update + 1;

	ring_container->total_bytes = 0;
	ring_container->total_packets = 0;
}

void grtnic_write_itr (struct grtnic_q_vector *q_vector)
{
	struct grtnic_adapter *adapter = q_vector->adapter;
	struct grtnic_hw *hw = &adapter->hw;
	int v_idx = q_vector->v_idx;
	u32 itr_reg = q_vector->itr & MAX_EITR;

	GRTNIC_WRITE_REG(hw, ((TARGET_IRQ<<12) + ADDR_INTR_ITR*4), (v_idx<<16 | itr_reg), 1);
}


static void grtnic_set_itr(struct grtnic_q_vector *q_vector)
{
	u32 new_itr;

	grtnic_update_itr(q_vector, &q_vector->tx);
	grtnic_update_itr(q_vector, &q_vector->rx);

	/* use the smallest value of new ITR delay calculations */
	new_itr = min(q_vector->rx.itr, q_vector->tx.itr);

	/* Clear latency flag if set, shift into correct position */
	new_itr &= ITR_ADAPTIVE_MASK_USECS;
	new_itr <<= 2;

	if (new_itr != q_vector->itr) {
		/* save the algorithm value here */
		q_vector->itr = new_itr;
//		printk("new_itr  = %d\n", new_itr);
		grtnic_write_itr(q_vector);
	}
}

#ifdef CONFIG_DISABLE_PACKET_SPLIT
/**
 * ixgbe_merge_active_tail - merge active tail into lro skb
 * @tail: pointer to active tail in frag_list
 *
 * This function merges the length and data of an active tail into the
 * skb containing the frag_list.  It resets the tail's pointer to the head,
 * but it leaves the heads pointer to tail intact.
 **/
static inline struct sk_buff *grtnic_merge_active_tail(struct sk_buff *tail)
{
	struct sk_buff *head = GRTNIC_CB(tail)->head;

	if (!head)
		return tail;

	head->len += tail->len;
	head->data_len += tail->len;
	head->truesize += tail->truesize;

	GRTNIC_CB(tail)->head = NULL;

	return head;
}

/**
 * ixgbe_add_active_tail - adds an active tail into the skb frag_list
 * @head: pointer to the start of the skb
 * @tail: pointer to active tail to add to frag_list
 *
 * This function adds an active tail to the end of the frag list.  This tail
 * will still be receiving data so we cannot yet ad it's stats to the main
 * skb.  That is done via ixgbe_merge_active_tail.
 **/
static inline void grtnic_add_active_tail(struct sk_buff *head, struct sk_buff *tail)
{
	struct sk_buff *old_tail = GRTNIC_CB(head)->tail;

	if (old_tail) {
		grtnic_merge_active_tail(old_tail);
		old_tail->next = tail;
	} else {
		skb_shinfo(head)->frag_list = tail;
	}

	GRTNIC_CB(tail)->head = head;
	GRTNIC_CB(head)->tail = tail;
}

/**
 * ixgbe_close_active_frag_list - cleanup pointers on a frag_list skb
 * @head: pointer to head of an active frag list
 *
 * This function will clear the frag_tail_tracker pointer on an active
 * frag_list and returns true if the pointer was actually set
 **/
static inline bool grtnic_close_active_frag_list(struct sk_buff *head)
{
	struct sk_buff *tail = GRTNIC_CB(head)->tail;

	if (!tail)
		return false;

	grtnic_merge_active_tail(tail);

	GRTNIC_CB(head)->tail = NULL;

	return true;
}

#endif


static void grtnic_process_skb_fields(struct grtnic_ring *rx_ring, union grtnic_rx_desc *rx_desc, struct sk_buff *skb)
{
  struct net_device *netdev = netdev_ring(rx_ring);
  u8 TCPCS, UDPCS, IPCS, CSUM_OK, UDP_CSUM_FLAG;

#ifdef NETIF_F_RXHASH
	grtnic_rx_hash(rx_ring, rx_desc, skb);
#endif /* NETIF_F_RXHASH */

  CSUM_OK       = rx_desc->wb.upper.rx_info.csum_ok;
  IPCS          = rx_desc->wb.upper.rx_info.ipcs;
  TCPCS         = rx_desc->wb.upper.rx_info.tcpcs;
  UDPCS         = rx_desc->wb.upper.rx_info.udpcs;
  UDP_CSUM_FLAG = rx_desc->wb.upper.rx_info.udp_csum_flag;

//  printk("CSUM_OK=%d, IPCS=%d, TCPCS=%d, UDPCS=%d, UDP_CSUM_FLAG=%d\n", CSUM_OK, IPCS, TCPCS, UDPCS, UDP_CSUM_FLAG);

  if((netdev->features & NETIF_F_RXCSUM) && IPCS) //is ip protocol
  {
    if((TCPCS & CSUM_OK) || (UDPCS & CSUM_OK & UDP_CSUM_FLAG)) //UDP_CSUM_FLAG means: udp checksum not is 0
    {
        skb->ip_summed = CHECKSUM_UNNECESSARY;
    }
    else if(TCPCS || (UDPCS & UDP_CSUM_FLAG))
    {
				printk("CSUM_OK=%d, IPCS=%d, TCPCS=%d, UDPCS=%d, UDP_CSUM_FLAG=%d\n", CSUM_OK, IPCS, TCPCS, UDPCS, UDP_CSUM_FLAG);
				rx_ring->rx_stats.csum_err++;
    }
  }

 	skb_record_rx_queue(skb, ring_queue_index(rx_ring));

  skb->protocol = eth_type_trans(skb, netdev_ring(rx_ring));
}


void grtnic_rx_skb(struct grtnic_q_vector *q_vector,
		  struct grtnic_ring *rx_ring,
		  union grtnic_rx_desc *rx_desc,
		  struct sk_buff *skb)
{
#ifdef HAVE_NDO_BUSY_POLL
	skb_mark_napi_id(skb, &q_vector->napi);

	if (grtnic_qv_busy_polling(q_vector) || q_vector->netpoll_rx) {
		netif_receive_skb(skb);
		/* exit early if we busy polled */
		return;
	}
#endif

	napi_gro_receive(&q_vector->napi, skb);

#ifndef NETIF_F_GRO
	netdev_ring(rx_ring)->last_rx = jiffies;
#endif
}


static bool grtnic_is_non_eop(struct grtnic_ring *rx_ring, union grtnic_rx_desc *rx_desc, struct sk_buff *skb)
{
#ifdef CONFIG_DISABLE_PACKET_SPLIT
	struct sk_buff *next_skb;
#endif

  u32 ntc = rx_ring->next_to_clean + 1;

  rx_desc->wb.upper.len_ctl.cmp = 0;

  /* fetch, update, and store next to clean */
  ntc = (ntc < rx_ring->count) ? ntc : 0;
  rx_ring->next_to_clean = ntc;

  prefetch(GRTNIC_RX_DESC(*rx_ring, ntc));

  if (likely(rx_desc->wb.upper.len_ctl.eop))
    return false;

	/* place skb in next buffer to be received */
#ifdef CONFIG_DISABLE_PACKET_SPLIT
	next_skb = rx_ring->rx_buffer_info[ntc].skb;

	grtnic_add_active_tail(skb, next_skb);
	GRTNIC_CB(next_skb)->head = skb;
#else
	rx_ring->rx_buffer_info[ntc].skb = skb;
#endif
	rx_ring->rx_stats.non_eop_descs++;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef CONFIG_DISABLE_PACKET_SPLIT
/* grtnic_clean_rx_irq -- * legacy */
static int grtnic_clean_rx_irq(struct grtnic_q_vector *q_vector, int budget)
{
	struct grtnic_ring *rx_ring = q_vector->rx.ring;
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
//#if IS_ENABLED(CONFIG_FCOE)
//	int ddp_bytes;
//	unsigned int mss = 0;
//#endif /* CONFIG_FCOE */
	u16 len = 0;
  u16 cleaned_count = grtnic_desc_unused(rx_ring);

	while (likely(total_rx_packets < budget)) {
    struct grtnic_rx_buffer *rx_buffer;
    union grtnic_rx_desc *rx_desc;
		struct sk_buff *skb;
		u16 ntc;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= GRTNIC_RX_BUFFER_WRITE) {
			grtnic_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}

		ntc = rx_ring->next_to_clean;
    rx_desc = GRTNIC_RX_DESC(*rx_ring, ntc);
		rx_buffer = &rx_ring->rx_buffer_info[ntc];

    if (!rx_desc->wb.upper.len_ctl.cmp)
      break;

//		printk("rx len = %d, desc_num = %d, chl = %d, cmp = %d, rs = %d, irq = %d, eop = %d, sop = %d\n", rx_desc->len_ctl.len,
//						rx_desc->len_ctl.desc_num,rx_desc->len_ctl.chl,rx_desc->len_ctl.cmp,rx_desc->len_ctl.rs,rx_desc->len_ctl.irq,
//						rx_desc->len_ctl.eop,rx_desc->len_ctl.sop);

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();

		skb = rx_buffer->skb;

		prefetch(skb->data);

		len = le16_to_cpu(rx_desc->wb.upper.len_ctl.len);
		/* pull the header of the skb in */
		__skb_put(skb, len);

//		printk("rx len = %d\n", len);
		/*
		 * Delay unmapping of the first packet. It carries the
		 * header information, HW may still access the header after
		 * the writeback.  Only unmap it when EOP is reached
		 */
		if (!GRTNIC_CB(skb)->head) {
			GRTNIC_CB(skb)->dma = rx_buffer->dma;
		} else {
			skb = grtnic_merge_active_tail(skb);
			dma_unmap_single(rx_ring->dev, rx_buffer->dma, rx_ring->rx_buffer_len, DMA_FROM_DEVICE);
		}

		/* clear skb reference in buffer info structure */
		rx_buffer->skb = NULL;
		rx_buffer->dma = 0;

		cleaned_count++;

		if (grtnic_is_non_eop(rx_ring, rx_desc, skb))
			continue;

		dma_unmap_single(rx_ring->dev, GRTNIC_CB(skb)->dma, rx_ring->rx_buffer_len, DMA_FROM_DEVICE);
		GRTNIC_CB(skb)->dma = 0;

		if (grtnic_close_active_frag_list(skb) && !GRTNIC_CB(skb)->append_cnt) {
			/* if we got here without RSC the packet is invalid */
			dev_kfree_skb_any(skb);
			continue;
		}

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

    /* populate checksum, timestamp, VLAN, and protocol */
    grtnic_process_skb_fields(rx_ring, rx_desc, skb);

		grtnic_rx_skb(q_vector, rx_ring, rx_desc, skb);

		/* update budget accounting */
		total_rx_packets++;
  }

	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	q_vector->rx.total_packets += total_rx_packets;
	q_vector->rx.total_bytes += total_rx_bytes;

	if (cleaned_count)
		grtnic_alloc_rx_buffers(rx_ring, cleaned_count);

	return total_rx_packets;
}

#else /* CONFIG_DISABLE_PACKET_SPLIT */

static void grtnic_reuse_rx_page(struct grtnic_ring *rx_ring, struct grtnic_rx_buffer *old_buff)
{
  struct grtnic_rx_buffer *new_buff;
  u16 nta = rx_ring->next_to_alloc;

  new_buff = &rx_ring->rx_buffer_info[nta];

  /* update, and store next to alloc */
  nta++;
  rx_ring->next_to_alloc = (nta < rx_ring->count) ? nta : 0;

	/* Transfer page from old buffer to new buffer.
	 * Move each member individually to avoid possible store
	 * forwarding stalls and unnecessary copy of skb.
	 */
	new_buff->dma		= old_buff->dma;
	new_buff->page		= old_buff->page;
	new_buff->page_offset	= old_buff->page_offset;
	new_buff->pagecnt_bias	= old_buff->pagecnt_bias;
}

static inline bool grtnic_page_is_reserved(struct page *page)
{
	return (page_to_nid(page) != numa_mem_id()) || page_is_pfmemalloc(page);
}

static bool grtnic_can_reuse_rx_page(struct grtnic_rx_buffer *rx_buffer)
{
	unsigned int pagecnt_bias = rx_buffer->pagecnt_bias;
	struct page *page = rx_buffer->page;

	/* avoid re-using remote pages */
	if (unlikely(grtnic_page_is_reserved(page)))
		return false;

#if (PAGE_SIZE < 8192)
	/* if we are only owner of page we can reuse it */
#ifdef HAVE_PAGE_COUNT_BULK_UPDATE
	if (unlikely((page_ref_count(page) - pagecnt_bias) > 1))
#else
	if (unlikely((page_count(page) - pagecnt_bias) > 1))
#endif
		return false;
#else
	/* The last offset is a bit aggressive in that we assume the
	 * worst case of FCoE being enabled and using a 3K buffer.
	 * However this should have minimal impact as the 1K extra is
	 * still less than one buffer in size.
	 */
#define GRTNIC_LAST_OFFSET \
	(SKB_WITH_OVERHEAD(PAGE_SIZE) - GRTNIC_RXBUFFER_3K)
	if (rx_buffer->page_offset > GRTNIC_LAST_OFFSET)
		return false;
#endif

#ifdef HAVE_PAGE_COUNT_BULK_UPDATE
	/* If we have drained the page fragment pool we need to update
	 * the pagecnt_bias and page count so that we fully restock the
	 * number of references the driver holds.
	 */
	if (unlikely(pagecnt_bias == 1)) {
		page_ref_add(page, USHRT_MAX - 1);
		rx_buffer->pagecnt_bias = USHRT_MAX;
	}
#else
	/* Even if we own the page, we are not allowed to use atomic_set()
	 * This would break get_page_unless_zero() users.
	 */
	if (likely(!pagecnt_bias)) {
		page_ref_inc(page);
		rx_buffer->pagecnt_bias = 1;
	}
#endif

	return true;
}

static void grtnic_add_rx_frag(struct grtnic_ring *rx_ring, struct grtnic_rx_buffer *rx_buffer, struct sk_buff *skb, unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = grtnic_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize = ring_uses_build_skb(rx_ring) ?
				SKB_DATA_ALIGN(GRTNIC_SKB_PAD + size) :
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

static void grtnic_dma_sync_frag(struct grtnic_ring *rx_ring, struct sk_buff *skb)
{
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
	DEFINE_DMA_ATTRS(attrs);

	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);

#endif
	/* if the page was released unmap it, else just sync our portion */
	if (unlikely(GRTNIC_CB(skb)->page_released)) {
		dma_unmap_page_attrs(rx_ring->dev, GRTNIC_CB(skb)->dma,
				     grtnic_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
				     &attrs);
#else
				     GRTNIC_RX_DMA_ATTR);
#endif
	} else if (ring_uses_build_skb(rx_ring)) {
		unsigned long offset = (unsigned long)(skb->data) & ~PAGE_MASK;

		dma_sync_single_range_for_cpu(rx_ring->dev,
					      GRTNIC_CB(skb)->dma,
					      offset,
					      skb_headlen(skb),
					      DMA_FROM_DEVICE);
	}	else {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[0];

		dma_sync_single_range_for_cpu(rx_ring->dev,
					      GRTNIC_CB(skb)->dma,
					      skb_frag_off(frag),
					      skb_frag_size(frag),
					      DMA_FROM_DEVICE);
	}
}


///////////////////////////////////////////////////////////////

static struct grtnic_rx_buffer *grtnic_get_rx_buffer(struct grtnic_ring *rx_ring,
		   union grtnic_rx_desc *rx_desc, struct sk_buff **skb, const unsigned int size)
{
	struct grtnic_rx_buffer *rx_buffer;

  rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
	prefetchw(rx_buffer->page);
	*skb = rx_buffer->skb;

	/* Delay unmapping of the first packet. It carries the header
	 * information, HW may still access the header after the writeback.
	 * Only unmap it when EOP is reached
	 */
	if (!likely(rx_desc->wb.upper.len_ctl.eop)) {
		if (!*skb)
			goto skip_sync;
	} else {
		if (*skb)
			grtnic_dma_sync_frag(rx_ring, *skb);
	}

	/* we are reusing so sync this buffer for CPU use */
	dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma,
				      rx_buffer->page_offset,
				      size,
				      DMA_FROM_DEVICE);
skip_sync:
	rx_buffer->pagecnt_bias--;

	return rx_buffer;
}

static void grtnic_put_rx_buffer(struct grtnic_ring *rx_ring, struct grtnic_rx_buffer *rx_buffer, struct sk_buff *skb)
{
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
	DEFINE_DMA_ATTRS(attrs);

	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);

#endif
	if (grtnic_can_reuse_rx_page(rx_buffer)) {
		/* hand second half of page back to the ring */
		grtnic_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		if (!IS_ERR(skb) && GRTNIC_CB(skb)->dma == rx_buffer->dma) {
			/* the page has been released from the ring */
			GRTNIC_CB(skb)->page_released = true;
		} else {
			/* we are not reusing the buffer so unmap it */
			dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
					     grtnic_rx_pg_size(rx_ring),
					     DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
					     &attrs);
#else
					     GRTNIC_RX_DMA_ATTR);
#endif
		}
		__page_frag_cache_drain(rx_buffer->page, rx_buffer->pagecnt_bias);
	}

	/* clear contents of rx_buffer */
	rx_buffer->page = NULL;
	rx_buffer->skb = NULL;
}

static struct sk_buff *grtnic_construct_skb(struct grtnic_ring *rx_ring,
					   struct grtnic_rx_buffer *rx_buffer,
					   union grtnic_rx_desc *rx_desc,
					   unsigned int size)
{

	void *va = page_address(rx_buffer->page) + rx_buffer->page_offset;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = grtnic_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize = SKB_DATA_ALIGN(GRTNIC_SKB_PAD + size);
#endif
	struct sk_buff *skb;

	/* prefetch first cache line of first page */
	prefetch(va);
#if L1_CACHE_BYTES < 128
	prefetch(va + L1_CACHE_BYTES);
#endif

	/* allocate a skb to store the frags */
	skb = napi_alloc_skb(&rx_ring->q_vector->napi, GRTNIC_RX_HDR_SIZE);
	if (unlikely(!skb))
		return NULL;

	if (size > GRTNIC_RX_HDR_SIZE) {
		if (!likely(rx_desc->wb.upper.len_ctl.eop))
			GRTNIC_CB(skb)->dma = rx_buffer->dma;

		skb_add_rx_frag(skb, 0, rx_buffer->page, rx_buffer->page_offset, size, truesize);
#if (PAGE_SIZE < 8192)
		rx_buffer->page_offset ^= truesize;
#else
		rx_buffer->page_offset += truesize;
#endif
	} else {
		memcpy(__skb_put(skb, size), va, ALIGN(size, sizeof(long)));
		rx_buffer->pagecnt_bias++;
	}

	return skb;
}

#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC
static struct sk_buff *grtnic_build_skb(struct grtnic_ring *rx_ring,
				       struct grtnic_rx_buffer *rx_buffer,
				       union grtnic_rx_desc *rx_desc,
				       unsigned int size)
{
	void *va = page_address(rx_buffer->page) + rx_buffer->page_offset;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = grtnic_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize = SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
				SKB_DATA_ALIGN(GRTNIC_SKB_PAD + size);
#endif
	struct sk_buff *skb;

	/* prefetch first cache line of first page */
	prefetch(va);
#if L1_CACHE_BYTES < 128
	prefetch(va + L1_CACHE_BYTES);
#endif

	/* build an skb around the page buffer */
	skb = build_skb(va - GRTNIC_SKB_PAD, truesize);
	if (unlikely(!skb))
		return NULL;

	/* update pointers within the skb to store the data */
	skb_reserve(skb, GRTNIC_SKB_PAD);
	__skb_put(skb, size);

	/* record DMA address if this is the start of a chain of buffers */
	if (!likely(rx_desc->wb.upper.len_ctl.eop))
		GRTNIC_CB(skb)->dma = rx_buffer->dma;

	/* update buffer offset */
#if (PAGE_SIZE < 8192)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif

	return skb;
}

#endif  /* HAVE_SWIOTLB_SKIP_CPU_SYNC */

static void grtnic_pull_tail(struct sk_buff *skb)
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
	pull_len = eth_get_headlen(skb->dev, va, GRTNIC_RX_HDR_SIZE);
//	pull_len = eth_get_headlen(va, GRTNIC_RX_HDR_SIZE);

	/* align pull length to size of long to optimize memcpy performance */
	skb_copy_to_linear_data(skb, va, ALIGN(pull_len, sizeof(long)));

	/* update all of the pointers */
	skb_frag_size_sub(frag, pull_len);
	skb_frag_off_add(frag, pull_len);
	skb->data_len -= pull_len;
	skb->tail += pull_len;
}

static bool grtnic_cleanup_headers(struct grtnic_ring *rx_ring, union grtnic_rx_desc *rx_desc, struct sk_buff *skb)
{

	/* place header in linear portion of buffer */
	if (!skb_headlen(skb))
		grtnic_pull_tail(skb);

	/* if eth_skb_pad returns an error the skb was freed */
	if (eth_skb_pad(skb))
		return true;

	return false;
}

/* grtnic_clean_rx_irq -- * packet split */
static int grtnic_clean_rx_irq(struct grtnic_q_vector *q_vector, int budget)
{
	struct grtnic_ring *rx_ring = q_vector->rx.ring;
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
//#if IS_ENABLED(CONFIG_FCOE)
//	int ddp_bytes;
//	unsigned int mss = 0;
//#endif /* CONFIG_FCOE */
  u16 cleaned_count = grtnic_desc_unused(rx_ring);

	while (likely(total_rx_packets < budget)) {
    union grtnic_rx_desc *rx_desc;
		struct grtnic_rx_buffer *rx_buffer;
		struct sk_buff *skb;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= GRTNIC_RX_BUFFER_WRITE) {
      grtnic_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}

    rx_desc = GRTNIC_RX_DESC(*rx_ring, rx_ring->next_to_clean);
    if (!rx_desc->wb.upper.len_ctl.cmp)
      break;

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();

  	size = le16_to_cpu(rx_desc->wb.upper.len_ctl.len);
		rx_buffer = grtnic_get_rx_buffer(rx_ring, rx_desc, &skb, size);

		/* retrieve a buffer from the ring */
		if (skb) {
			grtnic_add_rx_frag(rx_ring, rx_buffer, skb, size);
#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC
		} else if (ring_uses_build_skb(rx_ring)) {
			skb = grtnic_build_skb(rx_ring, rx_buffer, rx_desc, size);
#endif
		} else {
			skb = grtnic_construct_skb(rx_ring, rx_buffer, rx_desc, size);
		}

		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			rx_buffer->pagecnt_bias++;
			break;
		}

		grtnic_put_rx_buffer(rx_ring, rx_buffer, skb);
		cleaned_count++;

		/* place incomplete frames back on ring for completion */
		if (grtnic_is_non_eop(rx_ring, rx_desc, skb))
			continue;

		/* verify the packet layout is correct */
		if (grtnic_cleanup_headers(rx_ring, rx_desc, skb))
			continue;

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, timestamp, VLAN, and protocol */
		grtnic_process_skb_fields(rx_ring, rx_desc, skb);

		grtnic_rx_skb(q_vector, rx_ring, rx_desc, skb);

		/* update budget accounting */
		total_rx_packets++;
	}

	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	u64_stats_update_end(&rx_ring->syncp);
	q_vector->rx.total_packets += total_rx_packets;
	q_vector->rx.total_bytes += total_rx_bytes;

	return total_rx_packets;
}

#endif /* CONFIG_DISABLE_PACKET_SPLIT */
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_NDO_BUSY_POLL
/* must be called with local_bh_disable()d */
static int grtnic_busy_poll_recv(struct napi_struct *napi)
{
	struct grtnic_q_vector *q_vector =
			container_of(napi, struct grtnic_q_vector, napi);
	struct grtnic_adapter *adapter = q_vector->adapter;
	int found = 0;

	if (test_bit(__GRTNIC_DOWN, &adapter->state))
		return LL_FLUSH_FAILED;

	if (!grtnic_qv_lock_poll(q_vector))
		return LL_FLUSH_BUSY;

		found = grtnic_clean_rx_irq(q_vector, 4);
#ifdef BP_EXTENDED_STATS
		if (found)
			q_vector->rx.ring->stats.cleaned += found;
		else
			q_vector->rx.ring->stats.misses++;
#endif
//		if (found)
//			break;

	grtnic_qv_unlock_poll(q_vector);

	return found;
}

#endif /* HAVE_NDO_BUSY_POLL */

static bool grtnic_clean_tx_irq_reg(struct grtnic_q_vector *q_vector, int napi_budget)
{
	struct grtnic_adapter *adapter = q_vector->adapter;
	struct grtnic_ring *tx_ring = q_vector->tx.ring;
  struct grtnic_tx_buffer *tx_buffer;
	union grtnic_tx_desc *tx_desc;
	unsigned int total_bytes = 0, total_packets = 0;
	unsigned int budget = q_vector->tx.work_limit;
	unsigned int i = tx_ring->next_to_clean;

	if (test_bit(__GRTNIC_DOWN, &adapter->state))
		return true;

	tx_buffer = &tx_ring->tx_buffer_info[i];
	tx_desc = GRTNIC_TX_DESC(*tx_ring, i);
	i -= tx_ring->count;

	do {
		union grtnic_tx_desc *eop_desc = tx_buffer->next_to_watch;

		/* if next_to_watch is not set then there is no work pending */
		if (!eop_desc)
			break;

		/* prevent any other reads prior to eop_desc */
		smp_rmb();

    if (!eop_desc->wb.len_ctl.cmp)
      break;

//		printk("tx len = %d, desc_num = %d, chl = %d, cmp = %d, rs = %d, irq = %d, eop = %d, sop = %d\n", tx_desc->len_ctl.len,
//						tx_desc->len_ctl.desc_num,tx_desc->len_ctl.chl,tx_desc->len_ctl.cmp,tx_desc->len_ctl.rs,tx_desc->len_ctl.irq,
//						tx_desc->len_ctl.eop,tx_desc->len_ctl.sop);


		/* clear next_to_watch to prevent false hangs */
		tx_buffer->next_to_watch = NULL;

		/* update the statistics for this packet */
		total_bytes += tx_buffer->bytecount;
		total_packets += tx_buffer->gso_segs;

		/* free the skb */
		napi_consume_skb(tx_buffer->skb, napi_budget);

		/* unmap skb header data */
		dma_unmap_single(tx_ring->dev,
				 dma_unmap_addr(tx_buffer, dma),
				 dma_unmap_len(tx_buffer, len),
				 DMA_TO_DEVICE);

		/* clear tx_buffer data */
		dma_unmap_len_set(tx_buffer, len, 0);

		/* unmap remaining buffers */
		while (tx_desc != eop_desc) {
			tx_buffer++;
			tx_desc++;
			i++;
			if (unlikely(!i)) {
				i -= tx_ring->count;
				tx_buffer = tx_ring->tx_buffer_info;
				tx_desc = GRTNIC_TX_DESC(*tx_ring, 0);
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
			tx_desc = GRTNIC_TX_DESC(*tx_ring, 0);
		}

		/* issue prefetch for next Tx descriptor */
		prefetch(tx_desc);

		/* update budget accounting */
		budget--;
	} while (likely(budget));

	i += tx_ring->count;
	tx_ring->next_to_clean = i;

//	printk("next_to_clean = %d\n", i);

	u64_stats_update_begin(&tx_ring->syncp);
	tx_ring->stats.bytes += total_bytes;
	tx_ring->stats.packets += total_packets;
	u64_stats_update_end(&tx_ring->syncp);
	q_vector->tx.total_bytes += total_bytes;
	q_vector->tx.total_packets += total_packets;

	netdev_tx_completed_queue(txring_txq(tx_ring), total_packets, total_bytes);

#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)
	if (unlikely(total_packets && netif_carrier_ok(netdev_ring(tx_ring)) &&
		     (grtnic_desc_unused(tx_ring) >= TX_WAKE_THRESHOLD))) {
		/* Make sure that anybody stopping the queue after this
		 * sees the new next_to_clean.
		 */
		smp_mb();
#ifdef HAVE_TX_MQ
		if (__netif_subqueue_stopped(netdev_ring(tx_ring),
					     ring_queue_index(tx_ring))
		    && !test_bit(__GRTNIC_DOWN, &q_vector->adapter->state)) {
			netif_wake_subqueue(netdev_ring(tx_ring),
					    ring_queue_index(tx_ring));
			++tx_ring->tx_stats.restart_queue;
		}
#else
		if (netif_queue_stopped(netdev_ring(tx_ring)) &&
		    !test_bit(__GRTNIC_DOWN, &q_vector->adapter->state)) {
			netif_wake_queue(netdev_ring(tx_ring));
			++tx_ring->tx_stats.restart_queue;
		}
#endif
	}

	return !!budget;
}


/**
 *  grtnic_poll - NAPI Rx polling callback
 *  @napi: napi polling structure
 *  @budget: count of how many packets we should handle
 **/
int grtnic_poll(struct napi_struct *napi, int budget)
{
	struct grtnic_q_vector *q_vector = container_of(napi, struct grtnic_q_vector, napi);
	struct grtnic_adapter *adapter = q_vector->adapter;
	struct grtnic_hw *hw = &adapter->hw;
	int work_done = 0;
	bool clean_complete = true;
	u32 var;

//	bool clean_complete = true;
//	int work_done = 0;
//	int cleaned = 0;

#if 0
//#if IS_ENABLED(CONFIG_DCA)
	if (adapter->flags & GRTNIC_FLAG_DCA_ENABLED)
		grtnic_update_dca(q_vector);
#endif /* CONFIG_DCA */


	if (q_vector->tx.ring)
	{
		if(!grtnic_clean_tx_irq_reg(q_vector, budget))
			clean_complete = false;
	}

#ifdef HAVE_NDO_BUSY_POLL
	if (test_bit(NAPI_STATE_NPSVC, &napi->state))
		return budget;

	/* Exit if we are called by netpoll or busy polling is active */
	if ((budget <= 0) || !grtnic_qv_lock_napi(q_vector))
		return budget;
#else
	/* Exit if we are called by netpoll */
	if (budget <= 0)
		return budget;
#endif

	if (q_vector->rx.ring)
	{
		int cleaned = grtnic_clean_rx_irq(q_vector, budget);
		work_done += cleaned;

		if (cleaned >= budget)
			clean_complete = false;
	}


#ifdef HAVE_NDO_BUSY_POLL
	grtnic_qv_unlock_napi(q_vector);
#endif
#ifndef HAVE_NETDEV_NAPI_LIST
	if (!netif_running(adapter->netdev))
		clean_complete = true;
#endif

	/* If all work not completed, return budget and keep polling */
	if (!clean_complete)
		return budget;
	/* all work done, exit the polling mode */
	if (likely(napi_complete_done(napi, work_done))) {
		if (adapter->rx_itr_setting == 1)
			grtnic_set_itr(q_vector);
		if (!test_bit(__GRTNIC_DOWN, &adapter->state))
		{
			if (adapter->flags & GRTNIC_FLAG_MSIX_ENABLED)
				var = q_vector->eims_value;
			else
				var = ~0;
	
			GRTNIC_WRITE_REG(hw, ((TARGET_IRQ<<12) + ADDR_INTR_IMS*4), var, 1);
		}
	}
	return min(work_done, budget - 1);
}


static void grtnic_trigger_lsc(struct grtnic_adapter *adapter)
{
	struct grtnic_hw *hw = &adapter->hw;
	GRTNIC_WRITE_REG(hw, ((TARGET_IRQ<<12) + ADDR_INTR_ICS*4), adapter->eims_other, 1); //trigger user interrupt
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int grtnic_setup_tx_resources(struct grtnic_ring *tx_ring)
{
	struct device *dev = tx_ring->dev;
	int orig_node = dev_to_node(dev);
	int node = -1;
	int size;

	size = sizeof(struct grtnic_tx_buffer) * tx_ring->count;

	if (tx_ring->q_vector)
		node = tx_ring->q_vector->node;

	tx_ring->tx_buffer_info = vmalloc_node(size, node);
	if (!tx_ring->tx_buffer_info)
		tx_ring->tx_buffer_info = vmalloc(size);
	if (!tx_ring->tx_buffer_info)
		goto err_tx_buffer;

	/* round up to nearest 4K */
	tx_ring->size = tx_ring->count * sizeof(union grtnic_tx_desc);
	tx_ring->size = ALIGN(tx_ring->size, 4096);

	set_dev_node(dev, node);
	tx_ring->desc = dma_alloc_coherent(dev,
					   tx_ring->size,
					   &tx_ring->dma,
					   GFP_KERNEL);
	set_dev_node(dev, orig_node);
	if (!tx_ring->desc)
		tx_ring->desc = dma_alloc_coherent(dev, tx_ring->size,
						   &tx_ring->dma, GFP_KERNEL);
	if (!tx_ring->desc)
		goto err_tx_ring_dma;


	set_dev_node(dev, node);
	tx_ring->desc_wb = dma_alloc_coherent(dev, sizeof(struct grtnic_desc_wb), &tx_ring->desc_wb_dma, GFP_KERNEL);
	set_dev_node(dev, orig_node);
	if (!tx_ring->desc_wb)
		tx_ring->desc_wb = dma_alloc_coherent(dev, sizeof(struct grtnic_desc_wb), &tx_ring->desc_wb_dma, GFP_KERNEL);

	if (!tx_ring->desc_wb)
		goto err_tx_ring_wb;

	((struct grtnic_desc_wb *) tx_ring->desc_wb)->desc_hw_ptr = 0;

//	tx_ring->next_to_use = 0; 	//检查一下这里，其他地方设置了，这里就不需要了
//	tx_ring->next_to_clean = 0;
//
//#ifndef CONFIG_DISABLE_PACKET_SPLIT
//	tx_ring->next_to_alloc = 0;
//#endif

	return 0;
err_tx_ring_wb:
	dma_free_coherent(dev, tx_ring->size, tx_ring->desc, tx_ring->dma);
err_tx_ring_dma:
	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;
err_tx_buffer:	
	printk("Unable to allocate memory for the transmit descriptor ring\n");
	return -ENOMEM;
}

int grtnic_setup_rx_resources(struct grtnic_ring *rx_ring)
{
	struct device *dev = rx_ring->dev;
	int orig_node = dev_to_node(dev);
	int node = -1;
	int size;

	size = sizeof(struct grtnic_rx_buffer) * rx_ring->count;

	if (rx_ring->q_vector)
		node = rx_ring->q_vector->node;

	rx_ring->rx_buffer_info = vmalloc_node(size, node);
	if (!rx_ring->rx_buffer_info)
		rx_ring->rx_buffer_info = vmalloc(size);
	if (!rx_ring->rx_buffer_info)
		goto err_rx_buffer;

	/* Round up to nearest 4K */
	rx_ring->size = rx_ring->count * sizeof(union grtnic_rx_desc);
	rx_ring->size = ALIGN(rx_ring->size, 4096);

	set_dev_node(dev, node);
	rx_ring->desc = dma_alloc_coherent(dev,
					   rx_ring->size,
					   &rx_ring->dma,
					   GFP_KERNEL);
	set_dev_node(dev, orig_node);
	if (!rx_ring->desc)
		rx_ring->desc = dma_alloc_coherent(dev, rx_ring->size,
						   &rx_ring->dma, GFP_KERNEL);
	if (!rx_ring->desc)
		goto err_rx_ring_dma;

//	rx_ring->next_to_clean = 0; //检查一下这里，其他地方设置了，这里就不需要了
//	rx_ring->next_to_use = 0;
//
//#ifndef CONFIG_DISABLE_PACKET_SPLIT
//	rx_ring->next_to_alloc = 0;
//#endif

	return 0;

err_rx_ring_dma:
	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;
err_rx_buffer:
	printk("Unable to allocate memory for the receive descriptor ring\n");
	return -ENOMEM;
}


void grtnic_free_tx_resources(struct grtnic_ring *tx_ring)
{
	grtnic_clean_tx_ring(tx_ring);

	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;

	/* if not set, then don't free */
	if (!tx_ring->desc)
		return;

	dma_free_coherent(tx_ring->dev, tx_ring->size,
			  tx_ring->desc, tx_ring->dma);
	tx_ring->desc = NULL;

	dma_free_coherent(tx_ring->dev, sizeof(struct grtnic_desc_wb),
			  tx_ring->desc_wb, tx_ring->desc_wb_dma);
	tx_ring->desc = NULL;
}

void grtnic_free_rx_resources(struct grtnic_ring *rx_ring)
{
	grtnic_clean_rx_ring(rx_ring);

	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;

	/* if not set, then don't free */
	if (!rx_ring->desc)
		return;

	dma_free_coherent(rx_ring->dev, rx_ring->size, rx_ring->desc, rx_ring->dma);
	rx_ring->desc = NULL;
}

/**
 * grtnic_setup_all_tx_resources - allocate all queues Tx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int grtnic_setup_all_tx_resources(struct grtnic_adapter *adapter)
{
	int i, err = 0;

	for (i = 0; i < adapter->num_tx_queues; i++) {


		err = grtnic_setup_tx_resources(adapter->tx_ring[i]);
		if (!err)
			continue;

		e_err(probe, "Allocation for Tx Queue %u failed\n", i);
		goto err_setup_tx;
	}

	return 0;
err_setup_tx:
	/* rewind the index freeing the rings as we go */
	while (i--)
		grtnic_free_tx_resources(adapter->tx_ring[i]);
	return err;
}

/**
 * grtnic_setup_all_rx_resources - allocate all queues Rx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int grtnic_setup_all_rx_resources(struct grtnic_adapter *adapter)
{
	int i, err = 0;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		err = grtnic_setup_rx_resources(adapter->rx_ring[i]);
		if (!err)
			continue;

		e_err(probe, "Allocation for Rx Queue %u failed\n", i);
		goto err_setup_rx;
	}

	return 0;

err_setup_rx:
	/* rewind the index freeing the rings as we go */
	while (i--)
		grtnic_free_rx_resources(adapter->rx_ring[i]);
	return err;
}


/**
 * grtnic_free_all_tx_resources - Free Tx Resources for All Queues
 * @adapter: board private structure
 *
 * Free all transmit software resources
 **/
static void grtnic_free_all_tx_resources(struct grtnic_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		grtnic_free_tx_resources(adapter->tx_ring[i]);
}


/**
 * grtnic_free_all_rx_resources - Free Rx Resources for All Queues
 * @adapter: board private structure
 *
 * Free all receive software resources
 **/
static void grtnic_free_all_rx_resources(struct grtnic_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		grtnic_free_rx_resources(adapter->rx_ring[i]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * grtnic_configure_tx_ring - Configure 8259x Tx ring after Reset
 * @adapter: board private structure
 * @ring: structure containing ring specific data
 *
 * Configure the Tx descriptor ring after a reset.
 **/
void grtnic_configure_tx_ring(struct grtnic_adapter *adapter, struct grtnic_ring *ring)
{
	struct grtnic_hw *hw = &adapter->hw;
	u32 w;
	u32 txdctl = (1u << 25);			/* LWTHRESH */
	u8 reg_idx = ring->reg_idx;

  /* flush pending descriptor writebacks to memory */
//	GRTNIC_WRITE_REG(hw, ((TARGET_H2C<<12) + (reg_idx<<8) + ADDR_INT_DELAY*4), (TX_INT_DELAY | GRTNIC_TIDV_FPD), 1);
  /* execute the writes immediately */
	GRTNIC_WRITE_FLUSH(hw);

	/* write lower 32-bit of bus address of transfer first descriptor */
	w = cpu_to_le32(PCI_DMA_L(ring->dma));
	GRTNIC_WRITE_REG(hw, ((TARGET_H2C<<12) + (reg_idx<<8) + ADDR_SG_ADDRLO*4), w, 1);
	/* write upper 32-bit of bus address of transfer first descriptor */
	w = cpu_to_le32(PCI_DMA_H(ring->dma));
	GRTNIC_WRITE_REG(hw, ((TARGET_H2C<<12) + (reg_idx<<8) + ADDR_SG_ADDRHI*4), w, 1);
	/* write lower 32-bit of bus address of desc write back address*/
	w = cpu_to_le32(PCI_DMA_L(ring->desc_wb_dma));
	GRTNIC_WRITE_REG(hw, ((TARGET_H2C<<12) + (reg_idx<<8) + ADDR_SG_WBADDRLO*4), w, 1);
	/* write upper 32-bit of bus address of desc write back address*/
	w = cpu_to_le32(PCI_DMA_H(ring->desc_wb_dma));
	GRTNIC_WRITE_REG(hw, ((TARGET_H2C<<12) + (reg_idx<<8) + ADDR_SG_WBADDRHI*4), w, 1);

	/* setup max SG num */
	GRTNIC_WRITE_REG(hw, ((TARGET_H2C<<12) + (reg_idx<<8) + ADDR_SG_MAXNUM*4), ring->count, 1);
//  /* Set the Tx Interrupt Delay register  TIDV */ 前面为了flush，已经设置过了，这里就不用了
	GRTNIC_WRITE_REG(hw, ((TARGET_H2C<<12) + (reg_idx<<8) + ADDR_INT_DELAY*4), TX_INT_DELAY, 1);
//  write_register(tx_int_delay, adapter->dma_bar+ (TARGET_H2C<<12) + (reg_idx<<8) + ADDR_INT_DELAY*4);

	ring->tail = hw->dma_bar + (TARGET_H2C<<12) + (reg_idx <<8) + (ADDR_SG_SWPT*4);

	/* reset ntu and ntc to place SW in sync with hardwdare */
	ring->next_to_clean = 0;
	ring->next_to_use = 0;

	/*
	 * set WTHRESH to encourage burst writeback, it should not be set
	 * higher than 1 when:
	 * - ITR is 0 as it could cause false TX hangs
	 * - ITR is set to > 100k int/sec and BQL is enabled
	 *
	 * In order to avoid issues WTHRESH + PTHRESH should always be equal
	 * to or less than the number of on chip descriptors, which is
	 * currently 40.
	 */
	if (!ring->q_vector || (ring->q_vector->itr < GRTNIC_100K_ITR))
		txdctl |= (1 << 16);	/* WTHRESH = 1 */
	else
		txdctl |= (8 << 16);	/* WTHRESH = 8 */

	/*
	 * Setting PTHRESH to 32 both improves performance
	 * and avoids a TX hang with DFP enabled
	 */
	txdctl |= (1 << 8) |	/* HTHRESH = 1 */
		   32;		/* PTHRESH = 32 */

  //PTHRESH=32, HTHRESH=1, WTHRESH=1,LWTHRESH=1 预读的方式就是等待其它都不忙的时候才进行描述符指令发出
//  write_register(GRTNIC_TXDCTL_DMA_BURST_ENABLE, adapter->dma_bar+ (TARGET_H2C<<12) + (reg_idx<<8) + ADDR_DESC_CTRL*4);
	GRTNIC_WRITE_REG(hw, ((TARGET_H2C<<12) + (reg_idx<<8) + ADDR_DESC_CTRL*4), txdctl, 1);
//	GRTNIC_WRITE_REG(hw, ((TARGET_H2C<<12) + (reg_idx<<8) + ADDR_DESC_CTRL*4), GRTNIC_TXDCTL_DMA_BURST_ENABLE, 1);

	/* initialize XPS */
	if (!test_and_set_bit(__GRTNIC_TX_XPS_INIT_DONE, &ring->state)) {
		struct grtnic_q_vector *q_vector = ring->q_vector;

		if (q_vector)
			netif_set_xps_queue(adapter->netdev, get_cpu_mask(reg_idx % adapter->rss_queues), ring->queue_index);
//			netif_set_xps_queue(adapter->netdev, &q_vector->affinity_mask, ring->queue_index);
	}

	clear_bit(__GRTNIC_HANG_CHECK_ARMED, &ring->state);

	/* reinitialize tx_buffer_info */
	memset(ring->tx_buffer_info, 0,
	       sizeof(struct grtnic_tx_buffer) * ring->count);

	/* TX dma engine start */
	GRTNIC_WRITE_REG(hw, ((TARGET_H2C<<12) + (reg_idx<<8) + ADDR_ENGINE_CTRL*4), 0x01, 1);
}

/**
 * grtnic_configure_tx - Configure 8259x Transmit Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Tx unit of the MAC after a reset.
 **/

static void grtnic_configure_tx(struct grtnic_adapter *adapter)
{
	u32 i;

	/* Setup the HW Tx Head and Tail descriptor pointers */
	for (i = 0; i < adapter->num_tx_queues; i++)
		grtnic_configure_tx_ring(adapter, adapter->tx_ring[i]);
}


void grtnic_configure_rx_ring(struct grtnic_adapter *adapter, struct grtnic_ring *ring)
{
	struct grtnic_hw *hw = &adapter->hw;
	u32 w;
	u32 rxdctl = (1u << 25);			/* LWTHRESH */
	u8 reg_idx = ring->reg_idx;

  union grtnic_rx_desc *rx_desc;

  /* flush pending descriptor writebacks to memory */
//	GRTNIC_WRITE_REG(hw, ((TARGET_C2H<<12) + (reg_idx<<8) + ADDR_INT_DELAY*4), (RX_INT_DELAY | GRTNIC_RDTR_FPD), 1);
  /* execute the writes immediately */
	GRTNIC_WRITE_FLUSH(hw);

	w = cpu_to_le32(PCI_DMA_L(ring->dma));
	GRTNIC_WRITE_REG(hw, ((TARGET_C2H<<12) + (reg_idx<<8) + ADDR_SG_ADDRLO*4), w, 1);
	/* write upper 32-bit of bus address of transfer first descriptor */
	w = cpu_to_le32(PCI_DMA_H(ring->dma));
	GRTNIC_WRITE_REG(hw, ((TARGET_C2H<<12) + (reg_idx<<8) + ADDR_SG_ADDRHI*4), w, 1);

	/* setup max SG num */
	GRTNIC_WRITE_REG(hw, ((TARGET_C2H<<12) + (reg_idx<<8) + ADDR_SG_MAXNUM*4), ring->count, 1);
//  /* set the Receive Delay Timer Register RDTR  #define BURST_RDTR      0x20 */ /*前面为了flush，已经执行过了，这些就不再执行了*/
	GRTNIC_WRITE_REG(hw, ((TARGET_C2H<<12) + (reg_idx<<8) + ADDR_INT_DELAY*4), RX_INT_DELAY, 1);
//  write_register(rx_int_delay, adapter->dma_bar+ (TARGET_C2H<<12) + (channel<<8) + ADDR_INT_DELAY*4);

	ring->tail = hw->dma_bar + (TARGET_C2H<<12) + (reg_idx <<8) + (ADDR_SG_SWPT*4);

	/* reset ntu and ntc to place SW in sync with hardwdare */
	ring->next_to_clean = 0;
	ring->next_to_use = 0;
#ifndef CONFIG_DISABLE_PACKET_SPLIT
	ring->next_to_alloc = 0;
#endif

	/* initialize rx_buffer_info */
	memset(ring->rx_buffer_info, 0,
	       sizeof(struct grtnic_rx_buffer) * ring->count);

	/* initialize Rx descriptor 0 */
  rx_desc = GRTNIC_RX_DESC(*ring, 0);
  rx_desc->wb.upper.len_ctl.cmp = 0;

	rxdctl = GRTNIC_RXDCTL_DMA_BURST_ENABLE;
  //PTHRESH=32, HTHRESH=4, WTHRESH=4, LWTHRESH=1 这与intel功能不同，当描述符数量低于LWTHRESH时候，优先级最高，立刻进行读描述符，不关系bus是不是busy，否则采取预读的方式，优先级最低
	GRTNIC_WRITE_REG(hw, ((TARGET_C2H<<12) + (reg_idx<<8) + ADDR_DESC_CTRL*4), rxdctl, 1);

	/* RX dma engine start */
	GRTNIC_WRITE_REG(hw, ((TARGET_C2H<<12) + (reg_idx<<8) + ADDR_ENGINE_CTRL*4), 0x01, 1);

	grtnic_alloc_rx_buffers(ring, grtnic_desc_unused(ring));
}


static void grtnic_set_rx_buffer_len(struct grtnic_adapter *adapter)
{
	struct grtnic_ring *rx_ring;
	int i;

#if defined(CONFIG_DISABLE_PACKET_SPLIT) || (defined (HAVE_SWIOTLB_SKIP_CPU_SYNC) && (PAGE_SIZE < 8192)) 
	int max_frame	= adapter->max_frame_size;
#endif	

#ifdef CONFIG_DISABLE_PACKET_SPLIT
	max_frame += VLAN_HLEN;
	if(max_frame <= MAXIMUM_ETHERNET_VLAN_SIZE)
		max_frame = MAXIMUM_ETHERNET_VLAN_SIZE;
	else
		max_frame = ALIGN(max_frame, 1024);
#endif

	for (i = 0; i < adapter->num_rx_queues; i++) {
		rx_ring = adapter->rx_ring[i];

#ifndef CONFIG_DISABLE_PACKET_SPLIT
		clear_bit(__GRTNIC_RX_3K_BUFFER, &rx_ring->state);
		clear_bit(__GRTNIC_RX_BUILD_SKB_ENABLED, &rx_ring->state);
		rx_ring->rx_buffer_len = GRTNIC_RXBUFFER_2K;

#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC
		set_bit(__GRTNIC_RX_BUILD_SKB_ENABLED, &rx_ring->state);

#if (PAGE_SIZE < 8192)
		if (GRTNIC_2K_TOO_SMALL_WITH_PADDING || (max_frame > (ETH_FRAME_LEN + ETH_FCS_LEN)))
		{
			set_bit(__GRTNIC_RX_3K_BUFFER, &rx_ring->state);
			rx_ring->rx_buffer_len = GRTNIC_RXBUFFER_3K;
		}

#endif /* PAGE_SIZE < 8192*/
#endif /* HAVE_SWIOTLB_SKIP_CPU_SYNC */
#else /* CONFIG_IXGBE_DISABLE_PACKET_SPLIT */
		rx_ring->rx_buffer_len = max_frame;
#endif /*!CONFIG_DISABLE_PACKET_SPLIT*/
	}
}

/**
 * grtnic_configure_rx - Configure 8259x Receive Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Rx unit of the MAC after a reset.
 **/
static void grtnic_configure_rx(struct grtnic_adapter *adapter)
{
	int i;

	/* Program registers for the distribution of queues */
	grtnic_setup_mrqc(adapter);

	/* set_rx_buffer_len must be called before ring initialization */
	grtnic_set_rx_buffer_len(adapter);

	/*
	 * Setup the HW Rx Head and Tail Descriptor Pointers and
	 * the Base and Length of the Rx Descriptor Ring
	 */
	for (i = 0; i < adapter->num_rx_queues; i++)
		grtnic_configure_rx_ring(adapter, adapter->rx_ring[i]);

	/* enable all receives */
	grtnic_SetRx(adapter->netdev, 1); 	//start rx
}


static void grtnic_configure(struct grtnic_adapter *adapter)
{

//#if IS_ENABLED(CONFIG_DCA)
//	/* configure DCA */
//	if (adapter->flags & FLAG_DCA_CAPABLE)
//		grtnic_setup_dca(adapter);
//#endif

	grtnic_configure_tx(adapter);
	grtnic_configure_rx(adapter);
}

static void grtnic_up_complete(struct grtnic_adapter *adapter)
{
	struct grtnic_hw *hw = &adapter->hw;
	u32 phy_addr = hw->phy_addr;
	u16 temp;

//	ixgbe_get_hw_control(adapter);
//	ixgbe_setup_gpie(adapter);

	if (adapter->flags & GRTNIC_FLAG_MSIX_ENABLED)
		grtnic_configure_msix(adapter);
	else
		grtnic_configure_msi_and_legacy(adapter);

	if(adapter->ei->type == board_902T_GRT_FF)
	{
		grtnic_PhyRead(adapter->netdev, phy_addr, 0x00, &temp); //prtad_reg
	  grtnic_PhyWrite(adapter->netdev, phy_addr, 0x00, temp | PHY_RESET); //rst phy
	}
	else
	{
		/* enable the optics for 82599 SFP+ fiber */
		GRTNIC_WRITE_REG(hw, PHY_TX_DISABLE, 0x00, 0); //enable laser;
	}

	smp_mb__before_atomic();
	clear_bit(__GRTNIC_DOWN, &adapter->state);
	grtnic_napi_enable_all(adapter);
//#ifndef IXGBE_NO_LLI
//	grtnic_configure_lli(adapter);
//#endif

	/* clear any pending interrupts, may auto mask */
	GRTNIC_READ_REG(hw, ((TARGET_IRQ<<12) + ADDR_INTR_VECTOR*4), 1);
	grtnic_irq_enable(adapter);

	/* enable transmits */
	netif_tx_start_all_queues(adapter->netdev);

	/* bring the link up in the watchdog, this could race with our first
	 * link up interrupt but shouldn't be a problem */
	adapter->flags |= GRTNIC_FLAG_NEED_LINK_UPDATE;
	adapter->link_check_timeout = jiffies;
	mod_timer(&adapter->service_timer, jiffies);

//
//	ixgbe_clear_vf_stats_counters(adapter);
//	/* Set PF Reset Done bit so PF/VF Mail Ops can work */
//	ctrl_ext = IXGBE_READ_REG(hw, IXGBE_CTRL_EXT);
//	ctrl_ext |= IXGBE_CTRL_EXT_PFRSTD;
//	IXGBE_WRITE_REG(hw, IXGBE_CTRL_EXT, ctrl_ext);
//
//	/* update setting rx tx for all active vfs */
//	ixgbe_set_all_vfs(adapter);
}

void grtnic_reset(struct grtnic_adapter *adapter)
{
	struct grtnic_hw *hw = &adapter->hw;
	GRTNIC_READ_REG(hw, ((TARGET_CONFIG<<12) + ADDR_FUNC_RST*4), 1); //function reset;
}

int grtnic_open(struct net_device *netdev)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	int err;

	/* disallow open during test */
	if (test_bit(__GRTNIC_TESTING, &adapter->state))
		return -EBUSY;

	grtnic_SetRx(netdev, 0); 	//stop rx

	netif_carrier_off(netdev);

	/* allocate transmit descriptors */
	err = grtnic_setup_all_tx_resources(adapter);
	if (err)
		goto err_setup_tx;

	/* allocate receive descriptors */
	err = grtnic_setup_all_rx_resources(adapter);
	if (err)
		goto err_setup_rx;

	grtnic_configure(adapter);

	err = grtnic_request_irq(adapter);
	if (err)
		goto err_req_irq;


	/* Notify the stack of the actual queue counts. */
	err = netif_set_real_num_tx_queues(netdev, adapter->num_tx_queues);
	if (err)
		goto err_set_queues;

	err = netif_set_real_num_rx_queues(netdev, adapter->num_rx_queues);
	if (err)
		goto err_set_queues;


	grtnic_up_complete(adapter);

	grtnic_SetTx(netdev, 1); 	//start tx
	grtnic_trigger_lsc(adapter); //Fire a link status change interrupt to start the watchdog.

	return GRTNIC_SUCCESS;


err_set_queues:
	grtnic_free_irq(adapter);
err_req_irq:
	grtnic_free_all_rx_resources(adapter);
err_setup_rx:
	grtnic_free_all_tx_resources(adapter);
err_setup_tx:
	grtnic_reset(adapter);

	return err;
}
///////////////////////////////////////////////////////////////////////////////


void grtnic_disable_rx_queue(struct grtnic_adapter *adapter)
{
	struct grtnic_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	int i;

	/* disable receives */
	grtnic_SetRx(netdev, 0); 	//stop rx

	/* disable all enabled Rx queues */
	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct grtnic_ring *ring = adapter->rx_ring[i];
		u8 reg_idx = ring->reg_idx;
	  /* flush pending descriptor writebacks to memory */
		GRTNIC_WRITE_REG(hw, ((TARGET_C2H<<12) + (reg_idx<<8) + ADDR_INT_DELAY*4), (RX_INT_DELAY | GRTNIC_RDTR_FPD), 1);
		/* channel stop */
		GRTNIC_WRITE_REG(hw, ((TARGET_C2H<<12) + (reg_idx<<8) + ADDR_ENGINE_CTRL*4), 0x00, 1);
	}
}

void grtnic_disable_tx_queue(struct grtnic_adapter *adapter)
{
	struct grtnic_hw *hw = &adapter->hw;
	int i;

	/* disable all enabled Tx queues */
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct grtnic_ring *ring = adapter->tx_ring[i];
		u8 reg_idx = ring->reg_idx;

	  /* flush pending descriptor writebacks to memory */
		GRTNIC_WRITE_REG(hw, ((TARGET_H2C<<12) + (reg_idx<<8) + ADDR_INT_DELAY*4), (TX_INT_DELAY | GRTNIC_TIDV_FPD), 1);
		/* channel stop */
		GRTNIC_WRITE_REG(hw, ((TARGET_H2C<<12) + (reg_idx<<8) + ADDR_ENGINE_CTRL*4), 0x00, 1);
	}
}

/**
 *  grtnic_clean_tx_ring - Free Tx Buffers
 *  @tx_ring: ring to be cleaned
 **/
static void grtnic_clean_tx_ring(struct grtnic_ring *tx_ring)
{
	u16 i = tx_ring->next_to_clean;
	struct grtnic_tx_buffer *tx_buffer = &tx_ring->tx_buffer_info[i];
	unsigned int size;


	while (i != tx_ring->next_to_use) {
		union grtnic_tx_desc *eop_desc, *tx_desc;

		/* Free all the Tx ring sk_buffs */
		dev_kfree_skb_any(tx_buffer->skb);

		/* unmap skb header data */
		dma_unmap_single(tx_ring->dev, dma_unmap_addr(tx_buffer, dma), dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);

		/* check for eop_desc to determine the end of the packet */
		eop_desc = tx_buffer->next_to_watch;
		tx_desc = GRTNIC_TX_DESC(*tx_ring, i);
	
		/* unmap remaining buffers */
		while (tx_desc != eop_desc) {
			tx_buffer++;
			tx_desc++;
			i++;
			if (unlikely(i == tx_ring->count)) {
				i = 0;
				tx_buffer = tx_ring->tx_buffer_info;
				tx_desc = GRTNIC_TX_DESC(*tx_ring, 0);
			}

			/* unmap any remaining paged data */
			if (dma_unmap_len(tx_buffer, len))
				dma_unmap_page(tx_ring->dev, dma_unmap_addr(tx_buffer, dma), dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
		}

		/* move us one more past the eop_desc for start of next pkt */
		tx_buffer++;
		i++;
		if (unlikely(i == tx_ring->count)) {
			i = 0;
			tx_buffer = tx_ring->tx_buffer_info;
		}
	}

	/* reset BQL for queue */
	netdev_tx_reset_queue(txring_txq(tx_ring));

	size = sizeof(struct grtnic_tx_buffer) * tx_ring->count;
	memset(tx_ring->tx_buffer_info, 0, size);
	/* Zero out the descriptor ring */
	memset(tx_ring->desc, 0, tx_ring->size);

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;
}


/**
 *  grtnic_clean_rx_ring - Free Rx Buffers per Queue
 *  @rx_ring: ring to free buffers from
 **/
static void grtnic_clean_rx_ring(struct grtnic_ring *rx_ring)
{
	u16 i = rx_ring->next_to_clean;
	struct grtnic_rx_buffer *rx_buffer = &rx_ring->rx_buffer_info[i];
	unsigned int size;
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
	DEFINE_DMA_ATTRS(attrs);

	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif


	/* Free all the Rx ring sk_buffs */
#ifdef CONFIG_DISABLE_PACKET_SPLIT
	while (i != rx_ring->next_to_use) {
#else
	while (i != rx_ring->next_to_alloc) {
#endif
		if (rx_buffer->skb) {
			struct sk_buff *skb = rx_buffer->skb;
#ifndef CONFIG_DISABLE_PACKET_SPLIT
			if (GRTNIC_CB(skb)->page_released)
				dma_unmap_page_attrs(rx_ring->dev,
						     GRTNIC_CB(skb)->dma,
						     grtnic_rx_pg_size(rx_ring),
						     DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
						     &attrs);
#else
						     GRTNIC_RX_DMA_ATTR);
#endif
#else
			/* We need to clean up RSC frag lists */
			skb = grtnic_merge_active_tail(skb);
			if (grtnic_close_active_frag_list(skb))
				dma_unmap_single(rx_ring->dev, GRTNIC_CB(skb)->dma, rx_ring->rx_buffer_len, DMA_FROM_DEVICE);
			GRTNIC_CB(skb)->dma = 0;
#endif /* CONFIG_DISABLE_PACKET_SPLIT */
			dev_kfree_skb(skb);
			rx_buffer->skb = NULL;
		}

#ifndef CONFIG_DISABLE_PACKET_SPLIT
		/* Invalidate cache lines that may have been written to by
		 * device so that we avoid corrupting memory.
		 */
		dma_sync_single_range_for_cpu(rx_ring->dev,
					      rx_buffer->dma,
					      rx_buffer->page_offset,
					      grtnic_rx_bufsz(rx_ring),
					      DMA_FROM_DEVICE);

		/* free resources associated with mapping */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     grtnic_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
				     &attrs);
#else
				     GRTNIC_RX_DMA_ATTR);
#endif

		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
#else /* CONFIG_DISABLE_PACKET_SPLIT */
		if (rx_buffer->dma) {
			dma_unmap_single(rx_ring->dev, rx_buffer->dma, rx_ring->rx_buffer_len, DMA_FROM_DEVICE);
			rx_buffer->dma = 0;
		}
#endif /* CONFIG_DISABLE_PACKET_SPLIT */
		i++;
		rx_buffer++;
		if (i == rx_ring->count) {
			i = 0;
			rx_buffer = rx_ring->rx_buffer_info;
		}
	}

	size = sizeof(struct grtnic_rx_buffer) * rx_ring->count;
	memset(rx_ring->rx_buffer_info, 0, size);

	/* Zero out the descriptor ring */
	memset(rx_ring->desc, 0, rx_ring->size);

#ifndef CONFIG_DISABLE_PACKET_SPLIT
	rx_ring->next_to_alloc = 0;
#endif

	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
}

/**
 *  grtnic_clean_all_tx_rings - Free Tx Buffers for all queues
 *  @adapter: board private structure
 **/
static void grtnic_clean_all_tx_rings(struct grtnic_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		grtnic_clean_tx_ring(adapter->tx_ring[i]);
}

/**
 *  grtnic_clean_all_rx_rings - Free Rx Buffers for all queues
 *  @adapter: board private structure
 **/
static void grtnic_clean_all_rx_rings(struct grtnic_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		grtnic_clean_rx_ring(adapter->rx_ring[i]);
}


void grtnic_down(struct grtnic_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct grtnic_hw *hw = &adapter->hw;
	u32 phy_addr = hw->phy_addr;
	u16 temp;

	/* signal that we are down to the interrupt handler */
	if (test_and_set_bit(__GRTNIC_DOWN, &adapter->state))
		return; /* do nothing if already down */

	/* Shut off incoming Tx traffic */
	netif_tx_stop_all_queues(netdev);

	/* call carrier off first to avoid false dev_watchdog timeouts */
	netif_carrier_off(netdev);
	netif_tx_disable(netdev);


	/* Disable Rx */
	grtnic_disable_rx_queue(adapter);

	grtnic_irq_disable(adapter);

	grtnic_napi_disable_all(adapter);

	clear_bit(__GRTNIC_RESET_REQUESTED, &adapter->state);
	adapter->flags &= ~GRTNIC_FLAG_NEED_LINK_UPDATE;

	del_timer_sync(&adapter->service_timer);

	/* disable transmits in the hardware now that interrupts are off */
	grtnic_disable_tx_queue(adapter);

#ifdef HAVE_PCI_ERS
	if (!pci_channel_offline(adapter->pdev))
#endif
		grtnic_reset(adapter);

	if(adapter->ei->type == board_902T_GRT_FF)
	{
		grtnic_PhyRead(netdev, phy_addr, 0x00, &temp); //prtad_reg
	  grtnic_PhyWrite(netdev, phy_addr, 0x00, temp | PHY_POWER_DOWN); //power down
	}
	else
	{
	/* power down the optics for 82599 SFP+ fiber */
		GRTNIC_WRITE_REG(hw, PHY_TX_DISABLE, 0x01, 0); //disable laser;
	}

	grtnic_clean_all_tx_rings(adapter);
	grtnic_clean_all_rx_rings(adapter);
}


void grtnic_up(struct grtnic_adapter *adapter)
{

	/* hardware has been reset, we need to reload some things */
	grtnic_configure(adapter);

	grtnic_up_complete(adapter);
}

void grtnic_reinit_locked(struct grtnic_adapter *adapter)
{
	WARN_ON(in_interrupt());
	/* put off any impending NetWatchDogTimeout */
#ifdef HAVE_NETIF_TRANS_UPDATE
	netif_trans_update(adapter->netdev);
#else
	adapter->netdev->trans_start = jiffies;
#endif

	while (test_and_set_bit(__GRTNIC_RESETTING, &adapter->state))
		usleep_range(1000, 2000);

	grtnic_down(adapter);
	grtnic_up(adapter);
	clear_bit(__GRTNIC_RESETTING, &adapter->state);
}

void grtnic_do_reset(struct net_device *netdev)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev))
		grtnic_reinit_locked(adapter);
	else
		grtnic_reset(adapter);
}

/**
 * grtnic_close_suspend - actions necessary to both suspend and close flows
 * @adapter: the private adapter struct
 *
 * This function should contain the necessary work common to both suspending
 * and closing of the device.
 */
void grtnic_close_suspend(struct grtnic_adapter *adapter)
{
	grtnic_down(adapter);
	grtnic_free_irq(adapter);

	grtnic_free_all_rx_resources(adapter);
	grtnic_free_all_tx_resources(adapter);
}


int grtnic_close(struct net_device *netdev)
{
  struct grtnic_adapter *adapter        = netdev_priv(netdev);

	if (netif_device_present(netdev))
		grtnic_close_suspend(adapter);

	return 0;
}

static int __grtnic_maybe_stop_tx(struct grtnic_ring *tx_ring, u16 size)
{
	netif_stop_subqueue(tx_ring->netdev, tx_ring->queue_index);

	/* Herbert's original patch had:
	 *  smp_mb__after_netif_stop_queue();
	 * but since that doesn't exist yet, just open code it.
	 */
	smp_mb();

	/* We need to check again in a case another CPU has just
	 * made room available.
	 */
	if (likely(grtnic_desc_unused(tx_ring) < size))
		return -EBUSY;

	/* A reprieve! - use start_queue because it doesn't call schedule */
	netif_start_subqueue(tx_ring->netdev, tx_ring->queue_index);
	++tx_ring->tx_stats.restart_queue;
	return 0;
}

static inline int grtnic_maybe_stop_tx(struct grtnic_ring *tx_ring, u16 size)
{
	if (likely(grtnic_desc_unused(tx_ring) >= size))
		return 0;

	return __grtnic_maybe_stop_tx(tx_ring, size);
}


netdev_tx_t grtnic_xmit_frame_ring (struct sk_buff *skb,
	struct grtnic_adapter __maybe_unused *adapter,
	struct grtnic_ring *tx_ring)

{
	struct grtnic_tx_buffer *first, *tx_buffer;
  union grtnic_tx_desc *tx_desc;
	unsigned int i, f;
	skb_frag_t *frag;
	dma_addr_t dma;
	unsigned int data_len, size;
	u16 count = TXD_USE_COUNT(skb_headlen(skb));
	unsigned int csum_info = 0;

////////////////////////////////////////////////////////

	/*
	 * need: 1 descriptor per page * PAGE_SIZE/IXGBE_MAX_DATA_PER_TXD,
	 *       + 1 desc for skb_headlen/IXGBE_MAX_DATA_PER_TXD,
	 *       + 2 desc gap to keep tail from touching head,
	 *       + 1 desc for context descriptor,
	 * otherwise try next time
	 */
	for (f = 0; f < skb_shinfo(skb)->nr_frags; f++)
		count += TXD_USE_COUNT(skb_frag_size(&skb_shinfo(skb)->frags[f]));

	if (grtnic_maybe_stop_tx(tx_ring, count + 3)) {
		tx_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}

	/* record the location of the first descriptor for this packet */
	i = tx_ring->next_to_use;
	first = &tx_ring->tx_buffer_info[i];

	first->skb = skb;
	first->bytecount = skb->len;
	first->gso_segs = 1;
///////////////////////////////////////////////////////////////////
	tx_desc = GRTNIC_TX_DESC(*tx_ring, i);

	memset(&tx_desc->read.len_ctl, 0, sizeof(tx_desc->read.len_ctl));

	if (skb->ip_summed == CHECKSUM_PARTIAL)
	{
		unsigned int csum_start = skb_checksum_start_offset(skb);
		unsigned int csum_offset = skb->csum_offset;

		if (csum_start > 255 || csum_offset > 127)
		{
			if (skb_checksum_help(skb)) //soft calc csum
			csum_info = 0; //disable hw csum
		}
		else
		{
			csum_info = (csum_offset << 8) | (csum_start);
		}
	}
	else
	{
		csum_info = 0;
	}

	tx_desc->read.len_ctl.sop = 1;
	tx_desc->read.tx_info.csum_info = csum_info;

//////////////////////////////////////////////////////////////////////////////
	size = skb_headlen(skb);
	data_len = skb->data_len;
	dma = dma_map_single(tx_ring->dev, skb->data, size, DMA_TO_DEVICE);

	tx_buffer = first;

	for (frag = &skb_shinfo(skb)->frags[0];; frag++) {
		if (dma_mapping_error(tx_ring->dev, dma))
			goto dma_error;

		/* record length, and DMA address */
		tx_buffer->tx_flags = 0;

		dma_unmap_len_set(tx_buffer, len, size);
		dma_unmap_addr_set(tx_buffer, dma, dma);

		tx_desc->read.src_addr 	 = cpu_to_le64(dma);

		while (unlikely(size > GRTNIC_MAX_DATA_PER_TXD)) {
	    tx_desc->read.len_ctl.len = cpu_to_le32(GRTNIC_MAX_DATA_PER_TXD);

			i++;
			tx_desc++;
			if (i == tx_ring->count) {
				tx_desc = GRTNIC_TX_DESC(*tx_ring, 0);
				i = 0;
			}
			memset(&tx_desc->read.len_ctl, 0, sizeof(tx_desc->read.len_ctl));
//			tx_desc->read.olinfo_status = 0;

			dma += GRTNIC_MAX_DATA_PER_TXD;
			size -= GRTNIC_MAX_DATA_PER_TXD;

			tx_desc->read.src_addr 	 = cpu_to_le64(dma);
		}

		if (likely(!data_len))
			break;

    tx_desc->read.len_ctl.len = cpu_to_le32(size);

		i++;
		tx_desc++;
		if (i == tx_ring->count) {
			tx_desc = GRTNIC_TX_DESC(*tx_ring, 0);
			i = 0;
		}
		memset(&tx_desc->read.len_ctl, 0, sizeof(tx_desc->read.len_ctl));
//		tx_desc->read.olinfo_status = 0;

		size = skb_frag_size(frag);
		data_len -= size;

		dma = skb_frag_dma_map(tx_ring->dev, frag, 0, size, DMA_TO_DEVICE);

		tx_buffer = &tx_ring->tx_buffer_info[i];
	}

	/* write last descriptor with RS and EOP bits */
	tx_desc->read.len_ctl.eop = 1;
	tx_desc->read.len_ctl.irq = 1;
	tx_desc->read.len_ctl.rs  = 1;
  tx_desc->read.len_ctl.len = cpu_to_le32(size);


	netdev_tx_sent_queue(txring_txq(tx_ring), first->bytecount);

	/* set the timestamp */
	first->time_stamp = jiffies;

#ifndef HAVE_TRANS_START_IN_QUEUE
	netdev_ring(tx_ring)->trans_start = first->time_stamp;
#endif


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

	i++;
	if (i == tx_ring->count)
		i = 0;

	tx_ring->next_to_use = i;

	grtnic_maybe_stop_tx(tx_ring, DESC_NEEDED);

	if (netif_xmit_stopped(txring_txq(tx_ring)) || !netdev_xmit_more()) {
		writel(i, tx_ring->tail);
	
//		printk("next_to_use = %d\n", i);

#ifndef SPIN_UNLOCK_IMPLIES_MMIOWB

		/* The following mmiowb() is required on certain
		 * architechtures (IA64/Altix in particular) in order to
		 * synchronize the I/O calls with respect to a spin lock. This
		 * is because the wmb() on those architectures does not
		 * guarantee anything for posted I/O writes.
		 *
		 * Note that the associated spin_unlock() is not within the
		 * driver code, but in the networking core stack.
		 */
		mmiowb();
#endif /* SPIN_UNLOCK_IMPLIES_MMIOWB */
	}

	return 0;

dma_error:
	dev_err(tx_ring->dev, "TX DMA map failed\n");

	/* clear dma mappings for failed tx_buffer_info map */
	for (;;) {
		tx_buffer = &tx_ring->tx_buffer_info[i];
		if (dma_unmap_len(tx_buffer, len))
			dma_unmap_page(tx_ring->dev,
				       dma_unmap_addr(tx_buffer, dma),
				       dma_unmap_len(tx_buffer, len),
				       DMA_TO_DEVICE);
		dma_unmap_len_set(tx_buffer, len, 0);
		if (tx_buffer == first)
			break;
		if (i == 0)
			i += tx_ring->count;
		i--;
	}

	dev_kfree_skb_any(first->skb);
	first->skb = NULL;

	tx_ring->next_to_use = i;

	return -1;
}

static netdev_tx_t grtnic_xmit_frame(struct sk_buff *skb, struct net_device *netdev)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	struct grtnic_ring *tx_ring;
#ifdef HAVE_TX_MQ
	unsigned int r_idx = skb->queue_mapping;
#endif

	if (!netif_carrier_ok(netdev)) {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	if (skb_put_padto(skb, ETH_ZLEN)) 
		return NETDEV_TX_OK;

#ifdef HAVE_TX_MQ
	if (r_idx >= adapter->num_tx_queues)
		r_idx = r_idx % adapter->num_tx_queues;
	tx_ring = adapter->tx_ring[r_idx];
#else
	tx_ring = adapter->tx_ring[0];
#endif

	return grtnic_xmit_frame_ring(skb, adapter, tx_ring);

}

static void grtnic_check_lsc(struct grtnic_adapter *adapter)
{
	adapter->lsc_int++;
//	printk("lsc = %d\n", adapter->lsc_int);
	adapter->flags |= GRTNIC_FLAG_NEED_LINK_UPDATE;
	adapter->link_check_timeout = jiffies;
	if (!test_bit(__GRTNIC_DOWN, &adapter->state))
		grtnic_service_event_schedule(adapter);
}


irqreturn_t grtnic_isr (int __always_unused irq, void *data)
{
	struct grtnic_adapter *adapter  = data;
	struct grtnic_hw *hw = &adapter->hw;
	struct grtnic_q_vector *q_vector = adapter->q_vector[0];
	u32 irq_vector;

	/* read ICR disables interrupts using IAM */
	irq_vector = GRTNIC_READ_REG(hw, ((TARGET_IRQ<<12) + ADDR_INTR_VECTOR*4), 1);

	if (!(adapter->flags & GRTNIC_FLAG_MSI_CAPABLE)) //legacy int
	{
		if(!(irq_vector & (1<<31)))
			return IRQ_NONE;	/* Not our interrupt */
	}

  if (irq_vector & adapter->eims_other) //link status change
		grtnic_check_lsc(adapter);

  else if (((irq_vector & 0x7FFFFFFF) & ~(adapter->eims_other)) == 0)
  {
 		GRTNIC_WRITE_REG(hw, ((TARGET_IRQ<<12) + ADDR_INTR_IMS*4), ~0, 1); //打开所有的中断
    goto exit_int;
  }

	/* would disable interrupts here but EIAM disabled it */
	napi_schedule_irqoff(&q_vector->napi);

exit_int:
	return IRQ_HANDLED;
}

irqreturn_t grtnic_msix_other(int __always_unused irq, void *data)
{
	struct grtnic_adapter *adapter = data;
	struct grtnic_hw *hw = &adapter->hw;

	grtnic_check_lsc(adapter);

	/* re-enable the original interrupt state, no lsc, no queues */
	if (!test_bit(__GRTNIC_DOWN, &adapter->state))
		GRTNIC_WRITE_REG(hw, ((TARGET_IRQ<<12) + ADDR_INTR_IMS*4), adapter->eims_other, 1); //打开相应的中断,user_interrupt

	return IRQ_HANDLED;
}

irqreturn_t grtnic_msix_ring(int __always_unused irq, void *data)
{
	struct grtnic_q_vector *q_vector = data;

	/* EIAM disabled interrupts (on this vector) for us */

	if (q_vector->rx.ring || q_vector->tx.ring)
		napi_schedule_irqoff(&q_vector->napi);

	return IRQ_HANDLED;
}

///////////////////////////////////////////////////////////////////////////////////////////////
static int grtnic_mdio_read(struct net_device *netdev, int prtad, int devad,
			   u16 addr)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;
	u16 value;

	if (prtad != hw->phy_addr)
		return -EINVAL;

	if(adapter->speed) //10G
	{
		grtnic_SetPhyAddr(netdev, prtad, devad, addr); //only for 10G phy
		grtnic_PhyRead(netdev, prtad, devad, &value);
	}
	else
	{
		grtnic_PhyRead(netdev, prtad, addr, &value);
	}

	return value;
}

static int grtnic_mdio_write(struct net_device *netdev, int prtad, int devad,
			    u16 addr, u16 value)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	if (prtad != hw->phy_addr)
		return -EINVAL;

	if(adapter->speed) //10G
	{
		grtnic_SetPhyAddr(netdev, prtad, devad, addr); //only for 10G phy
		grtnic_PhyWrite(netdev, prtad, devad, value);
	}
	else
	{
		grtnic_PhyWrite(netdev, prtad, addr, value);
	}
	return 0;
}

static int grtnic_mii_ioctl(struct net_device *netdev, struct ifreq *ifr,
			   int cmd)
{
	struct mii_ioctl_data *mii = (struct mii_ioctl_data *) &ifr->ifr_data;
	int prtad, devad, ret;

	prtad = (mii->phy_id & MDIO_PHY_ID_PRTAD) >> 5;
	devad = (mii->phy_id & MDIO_PHY_ID_DEVAD);

	if (cmd == SIOCGMIIREG) {
		ret = grtnic_mdio_read(netdev, prtad, devad, mii->reg_num);
		if (ret < 0)
			return ret;
		mii->val_out = ret;
		return 0;
	} else {
		return grtnic_mdio_write(netdev, prtad, devad, mii->reg_num,
					mii->val_in);
	}
}

static int grtnic_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	switch (cmd) {
#ifdef ETHTOOL_OPS_COMPAT
	case SIOCETHTOOL:
		return ethtool_ioctl(ifr);
#endif
	case SIOCGMIIREG:
	case SIOCSMIIREG:
		return grtnic_mii_ioctl(netdev, ifr, cmd);
	default:
		return -EOPNOTSUPP;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
static int grtnic_set_mac(struct net_device *netdev, void *p)
{
//  struct xdmanet_port *xdmanet_port = netdev_priv(netdev);
	struct sockaddr *addr = p;

	if (!is_valid_ether_addr(addr->sa_data))
		return (-EADDRNOTAVAIL);
//	memcpy(netdev->dev_addr, addr->sa_data, netdev->addr_len);
	eth_hw_addr_set(netdev, addr->sa_data);

	grtnic_SetMacAddress(netdev, netdev->dev_addr); //added

	grtnic_SetMacPauseAddress(netdev, addr->sa_data);

	write_flash_macaddr(netdev);

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int grtnic_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
//#ifndef HAVE_NETDEVICE_MIN_MAX_MTU
	int max_frame = new_mtu + ETH_HLEN + ETH_FCS_LEN;
//#endif

#ifndef HAVE_NETDEVICE_MIN_MAX_MTU
	/* MTU < 68 is an error and causes problems on some kernels */
	if ((new_mtu < 68) || (max_frame > GRTNIC_MAX_JUMBO_FRAME_SIZE))
		return -EINVAL;

#endif

	e_info(probe, "changing MTU from %d to %d\n", netdev->mtu, new_mtu);

	adapter->max_frame_size = max_frame;

	grtnic_SetMaxFrameLen(netdev, max_frame);

	/* must set new MTU before calling down or up */
	netdev->mtu = new_mtu;

	if (netif_running(netdev))
		grtnic_reinit_locked(adapter);

	return 0;
}


static u32 hash_mc_addr(struct net_device *netdev, u8 *mc_addr)
{
	struct grtnic_adapter *adapter 		= netdev_priv(netdev);
	struct grtnic_hw *hw 							= &adapter->hw;
	struct grtnic_mac_info *mac 			= &hw->mac;

	u32 hash_value, hash_mask;
	u8 bit_shift = 0;

//	printk("add=%02x:%02x:%02x:%02x:%02x:%02x\n", mc_addr[5],mc_addr[4],mc_addr[3],mc_addr[2],mc_addr[1],mc_addr[0]);

	/* Register count multiplied by bits per register */
	hash_mask = (mac->mta_reg_count * 32) - 1;

	/* For a mc_filter_type of 0, bit_shift is the number of left-shifts
	 * where 0xFF would still fall within the hash mask.
	 */
	while (hash_mask >> bit_shift != 0xFF)
		bit_shift++;

	/* The portion of the address that is used for the hash table
	 * is determined by the mc_filter_type setting.
	 * The algorithm is such that there is a total of 8 bits of shifting.
	 * The bit_shift for a mc_filter_type of 0 represents the number of
	 * left-shifts where the MSB of mc_addr[5] would still fall within
	 * the hash_mask.  Case 0 does this exactly.  Since there are a total
	 * of 8 bits of shifting, then mc_addr[4] will shift right the
	 * remaining number of bits. Thus 8 - bit_shift.  The rest of the
	 * cases are a variation of this algorithm...essentially raising the
	 * number of bits to shift mc_addr[5] left, while still keeping the
	 * 8-bit shifting total.
	 *
	 * For example, given the following Destination MAC Address and an
	 * mta register count of 128 (thus a 4096-bit vector and 0xFFF mask),
	 * we can see that the bit_shift for case 0 is 4.  These are the hash
	 * values resulting from each mc_filter_type...
	 * [0] [1] [2] [3] [4] [5]
	 * 01  AA  00  12  34  56
	 * LSB           MSB
	 *
	 * case 0: hash_value = ((0x34 >> 4) | (0x56 << 4)) & 0xFFF = 0x563
	 * case 1: hash_value = ((0x34 >> 3) | (0x56 << 5)) & 0xFFF = 0xAC6
	 * case 2: hash_value = ((0x34 >> 2) | (0x56 << 6)) & 0xFFF = 0x163
	 * case 3: hash_value = ((0x34 >> 0) | (0x56 << 8)) & 0xFFF = 0x634
	 */
/*	switch (hw->mac.mc_filter_type) {
	default:
	case 0:
		break;
	case 1:
		bit_shift += 1;
		break;
	case 2:
		bit_shift += 2;
		break;
	case 3:
		bit_shift += 4;
		break;
	}*/

	hash_value = hash_mask & (((mc_addr[4] >> (8 - bit_shift)) | (((u16)mc_addr[5]) << bit_shift)));

	return hash_value;
}

void update_mc_addr_list(struct net_device *netdev, u8 *mc_addr_list, u32 mc_addr_count)
{
	struct grtnic_adapter *adapter 		= netdev_priv(netdev);
	struct grtnic_hw *hw 							= &adapter->hw;
	struct grtnic_mac_info *mac 			= &hw->mac;

	u32 hash_value, hash_bit, hash_reg;
	int i;

	mac->mta_reg_count = 128;

	/* clear mta_shadow */
	memset(&mac->mta_shadow, 0, sizeof(mac->mta_shadow));

	/* update mta_shadow from mc_addr_list */
	for (i = 0; (u32)i < mc_addr_count; i++) {
		hash_value = hash_mc_addr(netdev, mc_addr_list);

		hash_reg = (hash_value >> 5) & (mac->mta_reg_count - 1);
		hash_bit = hash_value & 0x1F;

		mac->mta_shadow[hash_reg] |= (1 << hash_bit);
		mc_addr_list += (ETH_ALEN);
	}

	GRTNIC_WRITE_REG(hw, MAC_HASH_TABLE_START, 0, 0);

	/* replace the entire MTA table */
	for (i = 0; i< mac->mta_reg_count; i++)
		GRTNIC_WRITE_REG(hw, MAC_HASH_TABLE_WR, mac->mta_shadow[i], 0);
}



static int grtnic_write_mc_addr_list(struct net_device *netdev)
{

#ifdef NETDEV_HW_ADDR_T_MULTICAST
	struct netdev_hw_addr *ha;
#else
	struct dev_mc_list *ha;
#endif
	u8 *mta_list;
	int i;

	if (netdev_mc_empty(netdev)) {
		/* nothing to program, so clear mc list */
		update_mc_addr_list(netdev, NULL, 0);
		return 0;
	}

	mta_list = kzalloc(netdev_mc_count(netdev) * ETH_ALEN, GFP_ATOMIC);
	if (!mta_list)
		return -ENOMEM;

	/* update_mc_addr_list expects a packed array of only addresses. */
	i = 0;
	netdev_for_each_mc_addr(ha, netdev)
#ifdef NETDEV_HW_ADDR_T_MULTICAST
	    memcpy(mta_list + (i++ * ETH_ALEN), ha->addr, ETH_ALEN);
#else
	    memcpy(mta_list + (i++ * ETH_ALEN), ha->dmi_addr, ETH_ALEN);
#endif

	update_mc_addr_list(netdev, mta_list, i);
	kfree(mta_list);

	return netdev_mc_count(netdev);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void grtnic_set_rx_mode(struct net_device *netdev)
{
	int count;
	u32 rctl, multicast_mode, all_multicast_mode, promisc_mode;

  promisc_mode        = 1;
  all_multicast_mode  = 2;
	multicast_mode      = 4;

	rctl = grtnic_GetAdrsFilter(netdev);
	rctl &= 0x0000000f;

	/* clear the affected bits */
	rctl &= ~(multicast_mode | all_multicast_mode| promisc_mode); //muliticast & all multicast & promisc

	/* Check for Promiscuous and All Multicast modes */

	if (netdev->flags & IFF_PROMISC)
	{
		rctl |= promisc_mode; //promisc
	}

	else
	{
		if (netdev->flags & IFF_ALLMULTI)
		{
			rctl |= all_multicast_mode;
		} 
		else if (!netdev_mc_empty(netdev))
		{
      count = netdev_mc_count(netdev);
      rctl |= multicast_mode;
      count = grtnic_write_mc_addr_list(netdev);
      if (count < 0)
				rctl |= all_multicast_mode;
		}
	}
	grtnic_SetAdrsFilter(netdev, rctl);
}



/**
 *  grtnic_update_stats - Update the board statistics counters
 *  @adapter: board private structure
 **/

void grtnic_update_stats(struct grtnic_adapter *adapter)
{
#ifdef HAVE_NETDEV_STATS_IN_NETDEV
	struct net_device_stats *net_stats = &adapter->netdev->stats;
#else
	struct net_device_stats *net_stats = &adapter->net_stats;
#endif /* HAVE_NETDEV_STATS_IN_NETDEV */
	struct grtnic_hw_stats *hwstats = &adapter->stats;
	u32 temp_val;

	u32 i;
	u64 non_eop_descs = 0, restart_queue = 0, tx_busy = 0;
	u64 alloc_rx_page_failed = 0, alloc_rx_buff_failed = 0;
	u64 alloc_rx_page = 0;
	u64 bytes = 0, packets = 0, hw_csum_rx_error = 0;

	if (test_bit(__GRTNIC_DOWN, &adapter->state) ||
	    test_bit(__GRTNIC_RESETTING, &adapter->state))
		return;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct grtnic_ring *rx_ring = adapter->rx_ring[i];
		non_eop_descs += rx_ring->rx_stats.non_eop_descs;
		alloc_rx_page += rx_ring->rx_stats.alloc_rx_page;
		alloc_rx_page_failed += rx_ring->rx_stats.alloc_rx_page_failed;
		alloc_rx_buff_failed += rx_ring->rx_stats.alloc_rx_buff_failed;
		hw_csum_rx_error += rx_ring->rx_stats.csum_err;
		bytes += rx_ring->stats.bytes;
		packets += rx_ring->stats.packets;

	}
	adapter->non_eop_descs = non_eop_descs;
	adapter->alloc_rx_page = alloc_rx_page;
	adapter->alloc_rx_page_failed = alloc_rx_page_failed;
	adapter->alloc_rx_buff_failed = alloc_rx_buff_failed;
	adapter->hw_csum_rx_error = hw_csum_rx_error;
	net_stats->rx_bytes = bytes;
	net_stats->rx_packets = packets;

	bytes = 0;
	packets = 0;
	/* gather some stats to the adapter struct that are per queue */
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct grtnic_ring *tx_ring = adapter->tx_ring[i];
		restart_queue += tx_ring->tx_stats.restart_queue;
		tx_busy += tx_ring->tx_stats.tx_busy;
		bytes += tx_ring->stats.bytes;
		packets += tx_ring->stats.packets;
	}
	adapter->restart_queue = restart_queue;
	adapter->tx_busy = tx_busy;
	net_stats->tx_bytes = bytes;
	net_stats->tx_packets = packets;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	temp_val = GRTNIC_READ_REG(&adapter->hw, MAC_RX_OVERFLOW_FRAME, 0);
	if(temp_val < hwstats->mpc)
		hwstats->mpc = 0x100000000 + temp_val;
	else
		hwstats->mpc = temp_val;

	net_stats->rx_missed_errors = hwstats->mpc;
	
	hwstats->ruc = grtnic_get_statistics_cnt(adapter, 0x210, hwstats->ruc);
	hwstats->roc = grtnic_get_statistics_cnt(adapter, 0x250, hwstats->roc);
	hwstats->rfc = grtnic_get_statistics_cnt(adapter, 0x218, hwstats->rfc); //crc error(<64)
	hwstats->crcerrs = grtnic_get_statistics_cnt(adapter, 0x298, hwstats->crcerrs); //crc error(>=64)
	hwstats->rlec = grtnic_get_statistics_cnt(adapter, 0x2B8, hwstats->rlec);
	hwstats->badopcode = grtnic_get_statistics_cnt(adapter, 0x2D0, hwstats->badopcode);
	hwstats->algnerrc = grtnic_get_statistics_cnt(adapter, 0x340, hwstats->algnerrc);

	net_stats->rx_errors = hwstats->rfc + 
		hwstats->crcerrs + 
		hwstats->algnerrc + 
		hwstats->ruc + 
		hwstats->roc + 
		hwstats->rlec +
		hwstats->badopcode;
	
	net_stats->rx_length_errors = hwstats->ruc + hwstats->roc + hwstats->rlec;
	net_stats->rx_crc_errors = hwstats->rfc + hwstats->crcerrs;
	net_stats->rx_frame_errors = hwstats->algnerrc;

	hwstats->ecol = grtnic_get_statistics_cnt(adapter, 0x330, hwstats->ecol);
	hwstats->latecol = grtnic_get_statistics_cnt(adapter, 0x328, hwstats->latecol);
	hwstats->tx_underrun = grtnic_get_statistics_cnt(adapter, 0x2F0, hwstats->tx_underrun);

	net_stats->tx_errors = hwstats->ecol + hwstats->latecol + hwstats->tx_underrun;
	net_stats->tx_aborted_errors = hwstats->ecol;
	net_stats->tx_window_errors = hwstats->latecol;
	net_stats->tx_carrier_errors = hwstats->tx_underrun;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	hwstats->gprc = grtnic_get_statistics_cnt(adapter, 0x290, hwstats->gprc);
	hwstats->gorc = grtnic_get_statistics_cnt(adapter, 0x200, hwstats->gorc);
	hwstats->bprc = grtnic_get_statistics_cnt(adapter, 0x2A0, hwstats->bprc);
	hwstats->mprc = grtnic_get_statistics_cnt(adapter, 0x2A8, hwstats->mprc);

	hwstats->prc64 	= grtnic_get_statistics_cnt(adapter, 0x220, 	hwstats->prc64);
	hwstats->prc127 	= grtnic_get_statistics_cnt(adapter, 0x228, hwstats->prc127);
	hwstats->prc255 	= grtnic_get_statistics_cnt(adapter, 0x230, hwstats->prc255);
	hwstats->prc511 	= grtnic_get_statistics_cnt(adapter, 0x238, hwstats->prc511);
	hwstats->prc1023 = grtnic_get_statistics_cnt(adapter, 0x240, 	hwstats->prc1023);
	hwstats->prc1522 = grtnic_get_statistics_cnt(adapter, 0x248, 	hwstats->prc1522);
	hwstats->prcoversize = grtnic_get_statistics_cnt(adapter, 0x250, hwstats->prcoversize);

	hwstats->scc 		= grtnic_get_statistics_cnt(adapter, 0x310, 	hwstats->scc);
	hwstats->mcc 		= grtnic_get_statistics_cnt(adapter, 0x318, 	hwstats->mcc);
	hwstats->dc 			= grtnic_get_statistics_cnt(adapter, 0x320, hwstats->dc);
	hwstats->rxpause 	= grtnic_get_statistics_cnt(adapter, 0x2C8, hwstats->rxpause);
	hwstats->txpause 	= grtnic_get_statistics_cnt(adapter, 0x308, hwstats->txpause);

	hwstats->gptc 		= grtnic_get_statistics_cnt(adapter, 0x2D8, 	hwstats->gptc);
	hwstats->gotc 		= grtnic_get_statistics_cnt(adapter, 0x208, 	hwstats->gotc);
	hwstats->bptc 			= grtnic_get_statistics_cnt(adapter, 0x2E0, hwstats->bptc);
	hwstats->mptc 			= grtnic_get_statistics_cnt(adapter, 0x2E8, hwstats->mptc);

	hwstats->ptc64 			= grtnic_get_statistics_cnt(adapter, 0x258, hwstats->ptc64);
	hwstats->ptc127 		= grtnic_get_statistics_cnt(adapter, 0x260, hwstats->ptc127);
	hwstats->ptc255 		= grtnic_get_statistics_cnt(adapter, 0x268, hwstats->ptc255);
	hwstats->ptc511 		= grtnic_get_statistics_cnt(adapter, 0x270, hwstats->ptc511);
	hwstats->ptc1023 		= grtnic_get_statistics_cnt(adapter, 0x278, hwstats->ptc1023);
	hwstats->ptc1522 		= grtnic_get_statistics_cnt(adapter, 0x280, hwstats->ptc1522);
	hwstats->ptcoversize 		= grtnic_get_statistics_cnt(adapter, 0x288, hwstats->ptcoversize);
}


#ifdef HAVE_NDO_GET_STATS64
static void grtnic_get_ring_stats64(struct rtnl_link_stats64 *stats, struct grtnic_ring *ring)
{
	u64 bytes, packets;
	unsigned int start;

	if (ring) {
		do {
			start = u64_stats_fetch_begin(&ring->syncp);
			packets = ring->stats.packets;
			bytes   = ring->stats.bytes;
		} while (u64_stats_fetch_retry(&ring->syncp, start));
		stats->tx_packets += packets;
		stats->tx_bytes   += bytes;
	}
}

/**
 * grtnic_get_stats64 - Get System Network Statistics
 * @netdev: network interface device structure
 * @stats: storage space for 64bit statistics
 *
 * Returns 64bit statistics, for use in the ndo_get_stats64 callback. This
 * function replaces ixgbe_get_stats for kernels which support it.
 */
#ifdef HAVE_VOID_NDO_GET_STATS64
static void grtnic_get_stats64(struct net_device *netdev, struct rtnl_link_stats64 *stats)
#else
static struct rtnl_link_stats64 *
grtnic_get_stats64(struct net_device *netdev, struct rtnl_link_stats64 *stats)
#endif
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);
	int i;

	rcu_read_lock();
	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct grtnic_ring *ring = READ_ONCE(adapter->rx_ring[i]);
		u64 bytes, packets;
		unsigned int start;

		if (ring) {
			do {
				start = u64_stats_fetch_begin(&ring->syncp);
				packets = ring->stats.packets;
				bytes   = ring->stats.bytes;
			} while (u64_stats_fetch_retry(&ring->syncp, start));
			stats->rx_packets += packets;
			stats->rx_bytes   += bytes;
		}
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct grtnic_ring *ring = READ_ONCE(adapter->tx_ring[i]);

		grtnic_get_ring_stats64(stats, ring);
	}

	rcu_read_unlock();

	/* following stats updated by grtnic_watchdog_task() */
//	stats->multicast	= netdev->stats.multicast;
//	stats->rx_errors	= netdev->stats.rx_errors;
//	stats->rx_length_errors	= netdev->stats.rx_length_errors;
//	stats->rx_crc_errors	= netdev->stats.rx_crc_errors;
//	stats->rx_missed_errors	= netdev->stats.rx_missed_errors;
#ifndef HAVE_VOID_NDO_GET_STATS64

	return stats;
#endif
}

#else

/**
 *  grtnic_get_stats - Get System Network Statistics
 *  @netdev: network interface device structure
 *
 *  Returns the address of the device statistics structure.
 *  The statistics are updated here and also from the timer callback.
 **/
static struct net_device_stats *grtnic_get_stats(struct net_device *netdev)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);

	/* update the stats data */
	grtnic_update_stats(adapter);

#ifdef HAVE_NETDEV_STATS_IN_NETDEV
	/* only return the current stats */
	return &netdev->stats;
#else
	/* only return the current stats */
	return &adapter->net_stats;
#endif /* HAVE_NETDEV_STATS_IN_NETDEV */
}
#endif //HAVE_NDO_GET_STATS64

/////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_NETDEV_SELECT_QUEUE
#ifdef CONFIG_USER_QUEUE

#if defined(HAVE_NDO_SELECT_QUEUE_FALLBACK_REMOVED)
static u16 grtnic_select_queue(struct net_device *dev, struct sk_buff *skb,
			      struct net_device *sb_dev)
#elif defined(HAVE_NDO_SELECT_QUEUE_SB_DEV)
static u16 grtnic_select_queue(struct net_device *dev, struct sk_buff *skb,
			      __always_unused struct net_device *sb_dev,
			      select_queue_fallback_t fallback)
#elif defined(HAVE_NDO_SELECT_QUEUE_ACCEL_FALLBACK)
static u16 grtnic_select_queue(struct net_device *dev, struct sk_buff *skb,
			      __always_unused void *accel,
			      select_queue_fallback_t fallback)
#elif defined(HAVE_NDO_SELECT_QUEUE_ACCEL)
static u16 grtnic_select_queue(struct net_device *dev, struct sk_buff *skb,
			      __always_unused void *accel)
#else
static u16 grtnic_select_queue(struct net_device *dev, struct sk_buff *skb)
#endif /* HAVE_NDO_SELECT_QUEUE_ACCEL_FALLBACK */
{
	struct grtnic_adapter *adapter 	= netdev_priv(dev);
	int card_type 									= adapter->ei->type;
	struct sock *sk 								= skb->sk;
	int queue_index 								= sk_tx_queue_get(sk);
	int new_index 									= -1;

	if (queue_index < 0 || skb->ooo_okay || queue_index >= dev->real_num_tx_queues)
	{
		if (skb_rx_queue_recorded(skb))
		{
			new_index = skb_get_rx_queue(skb);
			while (unlikely(new_index >= dev->real_num_tx_queues))
				new_index -= dev->real_num_tx_queues;
		
			if (queue_index != new_index && sk && sk_fullsock(sk) && rcu_access_pointer(sk->sk_dst_cache))
				sk_tx_queue_set(sk, new_index);

			return new_index;
		}
	}

#if defined(HAVE_NDO_SELECT_QUEUE_FALLBACK_REMOVED)
		return netdev_pick_tx(dev, skb, sb_dev);
#elif defined(HAVE_NDO_SELECT_QUEUE_SB_DEV)
		return fallback(dev, skb, sb_dev);
#elif defined(HAVE_NDO_SELECT_QUEUE_ACCEL_FALLBACK)
		return fallback(dev, skb);
#else
		return __netdev_pick_tx(dev, skb);
#endif
}
#endif /* CONFIG_USER_QUEUE */
#endif /* HAVE_NETDEV_SELECT_QUEUE */
/////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
 * Polling 'interrupt' - used by things like netconsole to send skbs
 * without having to re-enable interrupts. It's not called while
 * the interrupt routine is executing.
 */
static void grtnic_netpoll(struct net_device *netdev)
{
	struct grtnic_adapter *adapter = netdev_priv(netdev);

	/* if interface is down do nothing */
	if (test_bit(__GRTNIC_DOWN, &adapter->state))
		return;

	if (adapter->flags & GRTNIC_FLAG_MSIX_ENABLED) {
		int i;
		for (i = 0; i < adapter->num_q_vectors; i++) {
			adapter->q_vector[i]->netpoll_rx = true;
			grtnic_msix_ring(0, adapter->q_vector[i]);
			adapter->q_vector[i]->netpoll_rx = false;
		}
	} else {
		grtnic_isr(0, adapter);
	}
}
#endif /* CONFIG_NET_POLL_CONTROLLER */


/////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_NET_DEVICE_OPS
static const struct net_device_ops grtnic_netdev_ops = {
	.ndo_open           = grtnic_open,
	.ndo_stop           = grtnic_close,
	.ndo_start_xmit     = grtnic_xmit_frame,

#ifdef HAVE_NDO_GET_STATS64
	.ndo_get_stats64	= grtnic_get_stats64,
#else
	.ndo_get_stats		= grtnic_get_stats,
#endif /* HAVE_NDO_GET_STATS64 */

#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= grtnic_netpoll,
#endif
#ifndef HAVE_RHEL6_NET_DEVICE_EXTENDED
#ifdef HAVE_NDO_BUSY_POLL
	.ndo_busy_poll		= grtnic_busy_poll_recv,
#endif /* HAVE_NDO_BUSY_POLL */
#endif /* !HAVE_RHEL6_NET_DEVICE_EXTENDED */

#ifdef HAVE_RHEL7_EXTENDED_MIN_MAX_MTU
	.extended.ndo_change_mtu	= grtnic_change_mtu,
#else
	.ndo_change_mtu						= grtnic_change_mtu,
#endif /* HAVE_RHEL7_EXTENDED_MIN_MAX_MTU */

#ifdef HAVE_NDO_ETH_IOCTL
	.ndo_eth_ioctl 		= grtnic_ioctl,
#else
	.ndo_do_ioctl			= grtnic_ioctl,
#endif /* HAVE_NDO_ETH_IOCTL */

#ifdef HAVE_RHEL7_NET_DEVICE_OPS_EXT
/* RHEL7 requires this to be defined to enable extended ops.  RHEL7 uses the
 * function get_ndo_ext to retrieve offsets for extended fields from with the
 * net_device_ops struct and ndo_size is checked to determine whether or not
 * the offset is valid.
 */
	.ndo_size		= sizeof(const struct net_device_ops),
#endif

	.ndo_set_rx_mode    = grtnic_set_rx_mode,
	.ndo_set_mac_address= grtnic_set_mac,
#ifdef HAVE_NETDEV_SELECT_QUEUE
#ifdef CONFIG_USER_QUEUE
	.ndo_select_queue   = grtnic_select_queue,
#else
#ifndef HAVE_MQPRIO
	.ndo_select_queue	= __netdev_pick_tx,
#endif
#endif /* CONFIG_USER_QUEUE */
#endif /* HAVE_NETDEV_SELECT_QUEUE */

//  .ndo_tx_timeout   = xdmanet_tx_timeout,
};
#endif /* HAVE_NET_DEVICE_OPS */

/////////////////////////////////////////////////////////////////////////////////////////////////

void grtnic_assign_netdev_ops(struct net_device *netdev)
{
#ifdef HAVE_NET_DEVICE_OPS
	netdev->netdev_ops = &grtnic_netdev_ops;
#else
	netdev->open = &grtnic_open;
	netdev->stop = &grtnic_close;
	netdev->hard_start_xmit = &grtnic_xmit_frame;
	netdev->get_stats = &grtnic_get_stats;
#ifdef HAVE_SET_RX_MODE
	netdev->set_rx_mode = &grtnic_set_rx_mode;
#endif
	netdev->set_multicast_list = &grtnic_set_rx_mode;

	netdev->set_mac_address = &grtnic_set_mac;
	netdev->change_mtu 		= &grtnic_change_mtu;
//	netdev->tx_timeout = &xdmanet_tx_timeout;
	netdev->do_ioctl = &grtnic_ioctl;

#ifdef CONFIG_NET_POLL_CONTROLLER
	netdev->poll_controller = &grtnic_netpoll;
#endif

#ifdef HAVE_NETDEV_SELECT_QUEUE
#ifdef CONFIG_USER_QUEUE
	netdev->select_queue = &grtnic_select_queue;
#else
	netdev->select_queue = &__netdev_pick_tx;
#endif /*CONFIG_USER_QUEUE*/
#endif /* HAVE_NETDEV_SELECT_QUEUE */
#endif /*HAVE_NET_DEVICE_OPS*/

#ifdef HAVE_RHEL6_NET_DEVICE_EXTENDED
#ifdef HAVE_NDO_BUSY_POLL
	netdev_extended(netdev)->ndo_busy_poll		= grtnic_busy_poll_recv;
#endif /* HAVE_NDO_BUSY_POLL */
#endif /* HAVE_RHEL6_NET_DEVICE_EXTENDED */

	grtnic_set_ethtool_ops(netdev);
	netdev->watchdog_timeo = 5 * HZ;
}