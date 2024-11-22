// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#include <linux/capability.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/pci.h>
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
#include <linux/if_bridge.h>
#include <linux/bitops.h>
#include <linux/prefetch.h>
#include <linux/netdevice.h>
#include <net/tc_act/tc_gact.h>
#include <net/tc_act/tc_mirred.h>
#include <net/pkt_cls.h>
#include <net/xdp_sock_drv.h>
#include <net/vxlan.h>
#include <net/udp_tunnel.h>

#include "rnp_tc_u32_parse.h"
#include "rnp_common.h"
#include "rnp.h"
#include "rnp_dcb.h"
#include "rnp_sriov.h"
#include "rnp_ptp.h"
#include "rnp_ethtool.h"
#include "rnp_mpe.h"

#ifdef CONFIG_ARM64
#define NO_BQL_TEST
#endif

char rnp_driver_name[] = "rnp";
static const char rnp_driver_string[] =
	"mucse 1/10/25/40 Gigabit PCI Express Network Driver";
#define DRV_VERSION "1.0.1-rc2"
#include "version.h"

const char rnp_driver_version[] = DRV_VERSION;
static const char rnp_copyright[] =
	"Copyright (c) 2020-2024 mucse Corporation.";

extern struct rnp_info rnp_n10_info;
extern struct rnp_info rnp_n400_info;

static struct rnp_info *rnp_info_tbl[] = {
	[board_n10] = &rnp_n10_info,
	[board_n400] = &rnp_n400_info,
};

static int register_mbx_irq(struct rnp_adapter *adapter);
static void remove_mbx_irq(struct rnp_adapter *adapter);

static void rnp_pull_tail(struct sk_buff *skb);
#ifdef OPTM_WITH_LPAGE
static bool rnp_alloc_mapped_page(struct rnp_ring *rx_ring,
				  struct rnp_rx_buffer *bi,
				  union rnp_rx_desc *rx_desc, u16 bufsz,
				  u64 fun_id);

static void rnp_put_rx_buffer(struct rnp_ring *rx_ring,
			      struct rnp_rx_buffer *rx_buffer);
#else
static bool rnp_alloc_mapped_page(struct rnp_ring *rx_ring,
				  struct rnp_rx_buffer *bi);
static void rnp_put_rx_buffer(struct rnp_ring *rx_ring,
			      struct rnp_rx_buffer *rx_buffer,
			      struct sk_buff *skb);
#endif

static struct pci_device_id rnp_pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, PCI_DEVICE_ID_N10),
	  .driver_data = board_n10 }, /* n10 40G 10G */
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, PCI_DEVICE_ID_N10_X1),
	  .driver_data = board_n10 }, /* n10 40G 10G */
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, PCI_DEVICE_ID_N10_TP),
	  .driver_data = board_n10 }, /* n10 10G TP */
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, PCI_DEVICE_ID_N400),
	  .driver_data = board_n400 }, /* n400 4port 1G */
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, PCI_DEVICE_ID_N400C),
	  .driver_data = board_n400 }, /* n400 4port 1G */
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, PCI_DEVICE_ID_N400_X1),
	  .driver_data = board_n10 }, /* n400 1port 10G/1G */
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, PCI_DEVICE_ID_N400C_X1),
	  .driver_data = board_n10 }, /* n400 1port 10G/1G */
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, PCI_DEVICE_ID_N10C),
	  .driver_data = board_n10 }, /* n10c 40G 10G */
	/* required last entry */
	{
		0,
	},
};

MODULE_DEVICE_TABLE(pci, rnp_pci_tbl);

#define DEFAULT_MSG_ENABLE (NETIF_MSG_DRV | NETIF_MSG_PROBE | NETIF_MSG_LINK)
static int debug = -1;
module_param(debug, int, 0000);
MODULE_PARM_DESC(debug, "Debug level (0=none,...,16=all)");

static unsigned int fix_eth_name;
module_param(fix_eth_name, uint, 0000);
MODULE_PARM_DESC(fix_eth_name, "set eth adapter name to rnpXX");

static int module_enable_ptp = 1;
module_param(module_enable_ptp, uint, 0000);
MODULE_PARM_DESC(module_enable_ptp, "enable ptp feature, disabled default");

unsigned int mpe_src_port;
module_param(mpe_src_port, uint, 0000);
MODULE_PARM_DESC(mpe_src_port, "mpe src port");

unsigned int mpe_pkt_version;
module_param(mpe_pkt_version, uint, 0000);
MODULE_PARM_DESC(mpe_pkt_version, "ipv4 or ipv6 src port");

MODULE_AUTHOR("Mucse Corporation, <mucse@mucse.com>");
MODULE_DESCRIPTION("Mucse(R) 1/10/25/40 Gigabit PCI Express Network Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

static struct workqueue_struct *rnp_wq;
static int enable_hi_dma;
extern void rnp_service_timer(struct timer_list *t);

void rnp_service_event_schedule(struct rnp_adapter *adapter)
{
	if (!test_bit(__RNP_DOWN, &adapter->state) &&
	    !test_and_set_bit(__RNP_SERVICE_SCHED, &adapter->state))
		queue_work(rnp_wq, &adapter->service_task);
}

static void rnp_service_event_complete(struct rnp_adapter *adapter)
{
	BUG_ON(!test_bit(__RNP_SERVICE_SCHED, &adapter->state));

	/* flush memory to make sure state is correct before next watchdog */
	smp_mb__before_atomic();
	clear_bit(__RNP_SERVICE_SCHED, &adapter->state);
}


/**
 * rnp_set_ring_vector - set the ring_vector registers, mapping interrupt 
 * 	causes to  vectors
 * 
 * @adapter: pointer to adapter struct
 * @queue: queue to map the corresponding interrupt to
 * @msix_vector: the vector to map to the corresponding queue
 *
 */
static void rnp_set_ring_vector(struct rnp_adapter *adapter, u8 rnp_queue,
				u8 rnp_msix_vector)
{
	struct rnp_hw *hw = &adapter->hw;
	u32 data = 0;

	data = hw->pfvfnum << 24;
	data |= (rnp_msix_vector << 8);
	data |= (rnp_msix_vector << 0);

	DPRINTK(IFUP, INFO,
		"Set Ring-Vector queue:%d (reg:0x%x) <-- Rx-MSIX:%d, Tx-MSIX:%d\n",
		rnp_queue, RING_VECTOR(rnp_queue), rnp_msix_vector,
		rnp_msix_vector);

	rnp_wr_reg(hw->ring_msix_base + RING_VECTOR(rnp_queue), data);
}

static void rnp_unmap_and_free_tx_resource(struct rnp_ring *ring,
					   struct rnp_tx_buffer *tx_buffer)
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

static u64 rnp_get_tx_completed(struct rnp_ring *ring)
{
	return ring->stats.packets;
}

static u64 rnp_get_tx_pending(struct rnp_ring *ring)
{
	u32 head = ring_rd32(ring, RNP_DMA_REG_TX_DESC_BUF_HEAD);
	u32 tail = ring_rd32(ring, RNP_DMA_REG_TX_DESC_BUF_TAIL);

	if (head != tail)
		return (head < tail) ? tail - head :
				       (tail + ring->count - head);

	return 0;
}

static inline bool rnp_check_tx_hang(struct rnp_ring *tx_ring)
{
	u32 tx_done = rnp_get_tx_completed(tx_ring);
	u32 tx_done_old = tx_ring->tx_stats.tx_done_old;
	u32 tx_pending = rnp_get_tx_pending(tx_ring);
	bool ret = false;

	clear_check_for_tx_hang(tx_ring);

	/*
	 * Check for a hung queue, but be thorough. This verifies
	 * that a transmit has been completed since the previous
	 * check AND there is at least one packet pending. The
	 * ARMED bit is set to indicate a potential hang. The
	 * bit is cleared if a pause frame is received to remove
	 * false hang detection due to PFC or 802.3x frames. By
	 * requiring this to fail twice we avoid races with
	 * pfc clearing the ARMED bit and conditions where we
	 * run the check_tx_hang logic with a transmit completion
	 * pending but without time to complete it yet.
	 */
	if ((tx_done_old == tx_done) && tx_pending) {
		/* make sure it is true for two checks in a row */
		ret = test_and_set_bit(__RNP_HANG_CHECK_ARMED, &tx_ring->state);
	} else {
		/* update completed stats and continue */
		tx_ring->tx_stats.tx_done_old = tx_done;
		/* reset the countdown */
		clear_bit(__RNP_HANG_CHECK_ARMED, &tx_ring->state);
	}
	return ret;
}

/**
 * rnp_tx_timeout_reset - initiate reset due to Tx timeout
 * @adapter: driver private struct
 **/
static void rnp_tx_timeout_reset(struct rnp_adapter *adapter)
{
	/* Do the reset outside of interrupt context */
	if (!test_bit(__RNP_DOWN, &adapter->state)) {
		adapter->flags2 |= RNP_FLAG2_RESET_REQUESTED;
		e_warn(drv, "initiating reset due to tx timeout\n");
		rnp_service_event_schedule(adapter);
	}
}

static void rnp_check_restart_tx(struct rnp_q_vector *q_vector,
				 struct rnp_ring *tx_ring)
{
	struct rnp_adapter *adapter = q_vector->adapter;
#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)
	if (likely(netif_carrier_ok(tx_ring->netdev) &&
		   (rnp_desc_unused(tx_ring) >= TX_WAKE_THRESHOLD))) {
		/* Make sure that anybody stopping the queue after this
		 * sees the new next_to_clean.
		 */
		smp_mb();
		if (__netif_subqueue_stopped(tx_ring->netdev,
					     tx_ring->queue_index) &&
		    !test_bit(__RNP_DOWN, &adapter->state)) {
			netif_wake_subqueue(tx_ring->netdev,
					    tx_ring->queue_index);
			++tx_ring->tx_stats.restart_queue;
		}
	}
}

/**
 * rnp_clean_tx_irq - Reclaim resources after transmit completes
 * @q_vector: structure containing interrupt and ring information
 * @tx_ring: tx ring to clean
 **/
static bool rnp_clean_tx_irq(struct rnp_q_vector *q_vector,
			     struct rnp_ring *tx_ring, int napi_budget)
{
	struct rnp_adapter *adapter = q_vector->adapter;
	struct rnp_tx_buffer *tx_buffer;
	struct rnp_tx_desc *tx_desc;
	u64 total_bytes = 0, total_packets = 0;
	int budget = q_vector->tx.work_limit;
	int i = tx_ring->next_to_clean;

	if (test_bit(__RNP_DOWN, &adapter->state))
		return true;
	tx_ring->tx_stats.poll_count++;
	tx_buffer = &tx_ring->tx_buffer_info[i];
	tx_desc = RNP_TX_DESC(tx_ring, i);
	i -= tx_ring->count;

	do {
		struct rnp_tx_desc *eop_desc = tx_buffer->next_to_watch;
		/* if next_to_watch is not set then there is no work pending */
		if (!eop_desc)
			break;

		/* prevent any other reads prior to eop_desc */
		rmb();

		/* if eop DD is not set pending work has not been completed */
		if (!(eop_desc->vlan_cmd & cpu_to_le32(RNP_TXD_STAT_DD)))
			break;
		/* clear next_to_watch to prevent false hangs */
		tx_buffer->next_to_watch = NULL;

		/* update the statistics for this packet */
		total_bytes += tx_buffer->bytecount;
		total_packets += tx_buffer->gso_segs;

		/* free the skb */
		napi_consume_skb(tx_buffer->skb, napi_budget);

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
				tx_desc = RNP_TX_DESC(tx_ring, 0);
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
			tx_desc = RNP_TX_DESC(tx_ring, 0);
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
	tx_ring->tx_stats.tx_clean_count += total_packets;
	tx_ring->tx_stats.tx_clean_times++;
	if (tx_ring->tx_stats.tx_clean_times > 10) {
		tx_ring->tx_stats.tx_clean_times = 0;
		tx_ring->tx_stats.tx_clean_count = 0;
	}

	u64_stats_update_end(&tx_ring->syncp);
	q_vector->tx.total_bytes += total_bytes;
	q_vector->tx.total_packets += total_packets;

	tx_ring->tx_stats.send_done_bytes += total_bytes;
#ifdef NO_BQL_TEST
#else
	netdev_tx_completed_queue(txring_txq(tx_ring), total_packets,
				  total_bytes);
#endif

	if (!(q_vector->vector_flags & RNP_QVECTOR_FLAG_REDUCE_TX_IRQ_MISS)) {
#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)
		if (likely(netif_carrier_ok(tx_ring->netdev) &&
			   (rnp_desc_unused(tx_ring) >= TX_WAKE_THRESHOLD))) {
			/* Make sure that anybody stopping the queue after this
			 * sees the new next_to_clean.
			 */
			smp_mb();
			if (__netif_subqueue_stopped(tx_ring->netdev,
						     tx_ring->queue_index) &&
			    !test_bit(__RNP_DOWN, &adapter->state)) {
				netif_wake_subqueue(tx_ring->netdev,
						    tx_ring->queue_index);
				++tx_ring->tx_stats.restart_queue;
			}
		}
	}

	/* now we start tx queue later */
	return !!budget;
}

static inline void rnp_rx_hash(struct rnp_ring *ring,
			       union rnp_rx_desc *rx_desc, struct sk_buff *skb)
{
	int rss_type;

	if (!(ring->netdev->features & NETIF_F_RXHASH))
		return;
#define RNP_RSS_TYPE_MASK 0xc0
	rss_type = rx_desc->wb.cmd & RNP_RSS_TYPE_MASK;
	skb_set_hash(skb, le32_to_cpu(rx_desc->wb.rss_hash),
		     rss_type ? PKT_HASH_TYPE_L4 : PKT_HASH_TYPE_L3);
}

/**
 * rnp_rx_checksum - indicate in skb if hw indicated a good cksum
 * @ring: structure containing ring specific data
 * @rx_desc: current Rx descriptor being processed
 * @skb: skb currently being received and modified
 **/
static inline void rnp_rx_checksum(struct rnp_ring *ring,
				   union rnp_rx_desc *rx_desc,
				   struct sk_buff *skb)
{
	bool encap_pkt = false;

	skb_checksum_none_assert(skb);
	/* Rx csum disabled */
	if (!(ring->netdev->features & NETIF_F_RXCSUM))
		return;

	if (!(ring->ring_flags & RNP_RING_NO_TUNNEL_SUPPORT)) {
		if (rnp_get_stat(rx_desc, RNP_RXD_STAT_TUNNEL_MASK) ==
		    RNP_RXD_STAT_TUNNEL_VXLAN) {
			encap_pkt = true;
			skb->encapsulation = 1;
			skb->ip_summed = CHECKSUM_NONE;
		}
	}
	/* if outer L3/L4  error */
	/* must in promisc mode or rx-all mode */
	if (rnp_test_staterr(rx_desc, RNP_RXD_STAT_ERR_MASK)) {
		return;
	}
	ring->rx_stats.csum_good++;
	/* at least it is a ip packet which has ip checksum */

	/* It must be a TCP or UDP packet with a valid checksum */
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	if (encap_pkt) {
		/* If we checked the outer header let the stack know */
		skb->csum_level = 1;
	}
}

static inline void rnp_update_rx_tail(struct rnp_ring *rx_ring, u32 val)
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
	rnp_wr_reg(rx_ring->tail, val);
}

#if (PAGE_SIZE < 8192)
#define RNP_MAX_2K_FRAME_BUILD_SKB (RNP_RXBUFFER_1536 - NET_IP_ALIGN)
#define RNP_2K_TOO_SMALL_WITH_PADDING \
	((NET_SKB_PAD + RNP_RXBUFFER_1536) > SKB_WITH_OVERHEAD(RNP_RXBUFFER_2K))

static inline int rnp_compute_pad(int rx_buf_len)
{
	int page_size, pad_size;

	page_size = ALIGN(rx_buf_len, PAGE_SIZE / 2);
	pad_size = SKB_WITH_OVERHEAD(page_size) - rx_buf_len;

	return pad_size;
}

static inline int rnp_skb_pad(void)
{
	int rx_buf_len;

	/* If a 2K buffer cannot handle a standard Ethernet frame then
	 * optimize padding for a 3K buffer instead of a 1.5K buffer.
	 *
	 * For a 3K buffer we need to add enough padding to allow for
	 * tailroom due to NET_IP_ALIGN possibly shifting us out of
	 * cache-line alignment.
	 */
	if (RNP_2K_TOO_SMALL_WITH_PADDING)
		rx_buf_len = RNP_RXBUFFER_3K + SKB_DATA_ALIGN(NET_IP_ALIGN);
	else
		rx_buf_len = RNP_RXBUFFER_1536;

	/* if needed make room for NET_IP_ALIGN */
	rx_buf_len -= NET_IP_ALIGN;
	return rnp_compute_pad(rx_buf_len);
}

#define RNP_SKB_PAD rnp_skb_pad()
#else /* PAGE_SIZE < 8192 */
#define RNP_SKB_PAD (NET_SKB_PAD + NET_IP_ALIGN)
#endif

/**
 * rnp_process_skb_fields - Populate skb header fields from Rx descriptor
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @rx_desc: pointer to the EOP Rx descriptor
 * @skb: pointer to current skb being populated
 *
 * This function checks the ring, descriptor, and packet information in
 * order to populate the hash, checksum, VLAN, timestamp, protocol, and
 * other fields within the skb.
 **/
static void rnp_process_skb_fields(struct rnp_ring *rx_ring,
				   union rnp_rx_desc *rx_desc,
				   struct sk_buff *skb)
{
	struct net_device *dev = rx_ring->netdev;
	struct rnp_adapter *adapter = netdev_priv(dev);

	rnp_rx_hash(rx_ring, rx_desc, skb);
	rnp_rx_checksum(rx_ring, rx_desc, skb);
	if (((dev->features & NETIF_F_HW_VLAN_CTAG_RX)
	     || (dev->features & NETIF_F_HW_VLAN_STAG_RX)) &&
	    rnp_test_staterr(rx_desc, RNP_RXD_STAT_VLAN_VALID) &&
	    !ignore_veb_vlan(rx_ring->q_vector->adapter, rx_desc)) {

		if (rx_ring->ring_flags & RNP_RING_DOUBLE_VLAN_SUPPORT) {
			/* check outer vlan first */
			if (rnp_test_ext_cmd(rx_desc, REV_OUTER_VLAN)) {
				u16 vid_inner = le16_to_cpu(rx_desc->wb.vlan);
				u16 vid_outer;
				u16 vlan_tci = htons(ETH_P_8021Q);
				__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
						       vid_inner);
				/* check outer vlan type */
				if (rx_ring->ring_flags &
				    RNP_RING_STAGS_SUPPORT) {
					if (rnp_test_staterr(
						    rx_desc,
						    RNP_RXD_STAT_STAG)) {
						switch (rx_ring->q_vector
								->adapter
								->outer_vlan_type) {
						case outer_vlan_type_88a8:
							vlan_tci = htons(
								ETH_P_8021AD);
							break;
						case outer_vlan_type_9100:
							vlan_tci = htons(
								ETH_P_QINQ1);
							break;
						case outer_vlan_type_9200:
							vlan_tci = htons(
								ETH_P_QINQ2);
							break;
						default:
							vlan_tci = htons(
								ETH_P_8021AD);
							break;
						}
					} else {
						vlan_tci = htons(ETH_P_8021Q);
					}
				} else {
					vlan_tci = htons(ETH_P_8021Q);
				}
				vid_outer = le16_to_cpu(rx_desc->wb.mark);
				/* if in stags mode should ignore only stags */
				if (adapter->flags2 &
				    RNP_FLAG2_VLAN_STAGS_ENABLED) {
					/* push outer in if not equal stags or cvlan */
					if ((vid_outer != adapter->stags_vid) ||
					    (vlan_tci == htons(ETH_P_8021Q))) {
						/* push outer inner */
						skb = __vlan_hwaccel_push_inside(
							skb);
						__vlan_hwaccel_put_tag(
							skb, vlan_tci,
							vid_outer);
					}
				} else {
					/* push outer */
					skb = __vlan_hwaccel_push_inside(skb);
					__vlan_hwaccel_put_tag(skb, vlan_tci,
							       vid_outer);
				}
			} else {
				/* only inner vlan */
				u16 vid = le16_to_cpu(rx_desc->wb.vlan);
				if (rx_ring->ring_flags & RNP_RING_STAGS_SUPPORT) {
					if (rnp_test_staterr(rx_desc,
						    RNP_RXD_STAT_STAG)) {
						if ((adapter->flags2 &
						     RNP_FLAG2_VLAN_STAGS_ENABLED) &&
						    (vid ==
						     adapter->stags_vid)) {
						} else
							__vlan_hwaccel_put_tag(
								skb,
								htons(ETH_P_8021AD),
								vid);

					} else {
						__vlan_hwaccel_put_tag(skb,
							htons(ETH_P_8021Q),
							vid);
					}
				} else {
					__vlan_hwaccel_put_tag(
						skb, htons(ETH_P_8021Q), vid);
				}
			}
		} else {
			u16 vid = le16_to_cpu(rx_desc->wb.vlan);
			if (rx_ring->ring_flags & RNP_RING_STAGS_SUPPORT) {
				if (rnp_test_staterr(rx_desc,
						     RNP_RXD_STAT_STAG)) {
					__vlan_hwaccel_put_tag(
						skb, htons(ETH_P_8021AD), vid);
				} else {
					__vlan_hwaccel_put_tag(
						skb, htons(ETH_P_8021Q), vid);
				}
			} else {
				__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
						       vid);
			}
		}
		rx_ring->rx_stats.vlan_remove++;
	}

	skb_record_rx_queue(skb, rx_ring->queue_index);

	skb->protocol = eth_type_trans(skb, dev);
}

static void rnp_rx_skb(struct rnp_q_vector *q_vector, struct sk_buff *skb)
{
	struct rnp_adapter *adapter = q_vector->adapter;

	if (!(adapter->flags & RNP_FLAG_IN_NETPOLL))
		napi_gro_receive(&q_vector->napi, skb);
	else
		netif_rx(skb);
}

/* drop this packets if error */
static bool rnp_check_csum_error(struct rnp_ring *rx_ring,
				 union rnp_rx_desc *rx_desc, unsigned int size,
				 unsigned int *driver_drop_packets)
{
	bool err = false;

	struct net_device *netdev = rx_ring->netdev;

	if (netdev->features & NETIF_F_RXCSUM) {
		if (unlikely(
			    rnp_test_staterr(rx_desc, RNP_RXD_STAT_ERR_MASK))) {
			rx_debug_printk("rx error: VEB:%s mark:0x%x cmd:0x%x\n",
					(rx_ring->q_vector->adapter->flags &
					 RNP_FLAG_SRIOV_ENABLED) ?
						"On" :
						"Off",
					rx_desc->wb.mark, rx_desc->wb.cmd);
			/* push this packet to stack if in promisc mode */
			rx_ring->rx_stats.csum_err++;

			if ((!(netdev->flags & IFF_PROMISC) &&
			     (!(netdev->features & NETIF_F_RXALL)))) {
				if (rx_ring->ring_flags & RNP_RING_CHKSM_FIX) {
					err = true;
					goto skip_fix;
				}
				if (unlikely(rnp_test_staterr(
						     rx_desc,
						     RNP_RXD_STAT_L4_MASK) &&
					     (!(rx_desc->wb.rev1 &
						RNP_RX_L3_TYPE_MASK)))) {
					rx_ring->rx_stats.csum_err--;
					goto skip_fix;
				}
				/* we ignore sctp csum erro small than 60 */
				if (unlikely(rnp_test_staterr(rx_desc,
						RNP_RXD_STAT_SCTP_MASK))) {
					if ((size > 60) &&
					    (rx_desc->wb.rev1 &
					     RNP_RX_L3_TYPE_MASK)) {
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
		struct rnp_rx_buffer *rx_buffer;
#if (PAGE_SIZE < 8192)
		unsigned int truesize = rnp_rx_pg_size(rx_ring) / 2;
#else
		unsigned int truesize =
			ring_uses_build_skb(rx_ring) ?
				SKB_DATA_ALIGN(RNP_SKB_PAD + size) :
				SKB_DATA_ALIGN(size);
#endif

		/* if eop add drop_packets */
		if (likely(rnp_test_staterr(rx_desc, RNP_RXD_STAT_EOP)))
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
		rnp_put_rx_buffer(rx_ring, rx_buffer);
#else
		rnp_put_rx_buffer(rx_ring, rx_buffer, NULL);
#endif
		ntc = (ntc < rx_ring->count) ? ntc : 0;
		rx_ring->next_to_clean = ntc;
	}
	return err;
}

/**
 * rnp_rx_ring_reinit - just reinit rx_ring with new count in ->reset_count
 * @rx_ring: rx descriptor ring to transact packets on
 */
static int rnp_rx_ring_reinit(struct rnp_adapter *adapter, struct rnp_ring *rx_ring)
{
	struct rnp_ring *temp_ring;
	int err = 0;

	if (rx_ring->count == rx_ring->reset_count)
		return 0;
	/* stop rx queue */

	temp_ring = vzalloc(array_size(1, sizeof(struct rnp_ring)));
	if (!temp_ring)
		goto err_setup;

	rnp_disable_rx_queue(adapter, rx_ring);
	/* reinit for this ring */
	memcpy(temp_ring, rx_ring, sizeof(struct rnp_ring));
	/* setup new count */
	temp_ring->count = rx_ring->reset_count;
	err = rnp_setup_rx_resources(temp_ring, adapter);
	if (err) {
		rnp_free_rx_resources(temp_ring);
		vfree(temp_ring);
		goto err_setup;
	}
	rnp_free_rx_resources(rx_ring);
	memcpy(rx_ring, temp_ring, sizeof(struct rnp_ring));
	rnp_configure_rx_ring(adapter, rx_ring);
	/* start rx */
	vfree(temp_ring);
	ring_wr32(rx_ring, RNP_DMA_RX_START, 1);
	return 0;
err_setup:
	return -1;
}

#ifndef OPTM_WITH_LPAGE
/**
 * rnp_alloc_rx_buffers - Replace used receive buffers
 * @rx_ring: ring to place buffers on
 * @cleaned_count: number of buffers to replace
 **/
void rnp_alloc_rx_buffers(struct rnp_ring *rx_ring, u16 cleaned_count)
{
	union rnp_rx_desc *rx_desc;
	struct rnp_rx_buffer *bi;
	u16 i = rx_ring->next_to_use;
	u64 fun_id = ((u64)(rx_ring->pfvfnum) << (32 + 24));
	u16 bufsz;
	/* nothing to do */
	if (!cleaned_count)
		return;

	rx_desc = RNP_RX_DESC(rx_ring, i);

	BUG_ON(rx_desc == NULL);

	bi = &rx_ring->rx_buffer_info[i];

	BUG_ON(bi == NULL);

	i -= rx_ring->count;
	bufsz = rnp_rx_bufsz(rx_ring);

	do {
		if (!rnp_alloc_mapped_page(rx_ring, bi))
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
		rx_desc->resv_cmd = 0;

		rx_desc++;
		bi++;
		i++;
		if (unlikely(!i)) {
			rx_desc = RNP_RX_DESC(rx_ring, 0);
			bi = rx_ring->rx_buffer_info;
			i -= rx_ring->count;
		}

		/* clear the hdr_addr for the next_to_use descriptor */
		cleaned_count--;
	} while (cleaned_count);

	i += rx_ring->count;

	if (rx_ring->next_to_use != i)
		rnp_update_rx_tail(rx_ring, i);
}
#endif

static inline unsigned int rnp_rx_offset(struct rnp_ring *rx_ring)
{
	return ring_uses_build_skb(rx_ring) ? RNP_SKB_PAD : 0;
}

#ifdef OPTM_WITH_LPAGE
/**
 * rnp_alloc_rx_buffers - Replace used receive buffers
 * @rx_ring: ring to place buffers on
 * @cleaned_count: number of buffers to replace
 **/
void rnp_alloc_rx_buffers(struct rnp_ring *rx_ring, u16 cleaned_count)
{
	union rnp_rx_desc *rx_desc;
	struct rnp_rx_buffer *bi;
	u16 i = rx_ring->next_to_use;
	u64 fun_id = ((u64)(rx_ring->pfvfnum) << (32 + 24));
	u16 bufsz;
	/* nothing to do */
	if (!cleaned_count)
		return;

	rx_desc = RNP_RX_DESC(rx_ring, i);

	BUG_ON(rx_desc == NULL);

	bi = &rx_ring->rx_buffer_info[i];

	BUG_ON(bi == NULL);

	i -= rx_ring->count;
	bufsz = rnp_rx_bufsz(rx_ring);

	do {
		int count = 1;
		struct page *page;

		if (!rnp_alloc_mapped_page(rx_ring, bi, rx_desc, bufsz, fun_id))
			break;
		page = bi->page;

		rx_desc->resv_cmd = 0;

		rx_desc++;
		i++;
		bi++;

		if (unlikely(!i)) {
			rx_desc = RNP_RX_DESC(rx_ring, 0);
			bi = rx_ring->rx_buffer_info;
			i -= rx_ring->count;
		}

		rx_desc->resv_cmd = 0;

		cleaned_count--;

		while (count < rx_ring->rx_page_buf_nums && cleaned_count) {
			dma_addr_t dma;

			bi->page_offset = rx_ring->rx_per_buf_mem * count +
					  rnp_rx_offset(rx_ring);
			/* map page for use */
			dma = dma_map_page_attrs(rx_ring->dev, page,
						 bi->page_offset, bufsz,
						 DMA_FROM_DEVICE,
						 RNP_RX_DMA_ATTR);

			if (dma_mapping_error(rx_ring->dev, dma)) {
				printk("map second error\n");
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
			rx_desc->resv_cmd = 0;

			rx_desc++;
			bi++;
			i++;
			if (unlikely(!i)) {
				rx_desc = RNP_RX_DESC(rx_ring, 0);
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
		rnp_update_rx_tail(rx_ring, i);
}

#endif /* OPTM_WITH_LPAGE */
/**
 * rnp_get_headlen - determine size of header for RSC/LRO/GRO/FCOE
 * @data: pointer to the start of the headers
 * @max_len: total length of section to find headers in
 *
 * This function is meant to determine the length of headers that will
 * be recognized by hardware for LRO, GRO, and RSC offloads.  The main
 * motivation of doing this is to only perform one pull for IPv4 TCP
 * packets so that we can do basic things like calculating the gso_size
 * based on the average data per packet.
 **/
static unsigned int rnp_get_headlen(unsigned char *data, unsigned int max_len)
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

#ifdef OPTM_WITH_LPAGE
/**
 * rnp_is_non_eop - process handling of non-EOP buffers
 * @rx_ring: Rx ring being processed
 * @rx_desc: Rx descriptor for current buffer
 * @skb: Current socket buffer containing buffer in progress
 *
 * This function updates next to clean.  If the buffer is an EOP buffer
 * this function exits returning false, otherwise it will place the
 * sk_buff in the next buffer to be chained and return true indicating
 * that this is in fact a non-EOP buffer.
 **/
static bool rnp_is_non_eop(struct rnp_ring *rx_ring, union rnp_rx_desc *rx_desc)
{
	u32 ntc = rx_ring->next_to_clean + 1;
	/* fetch, update, and store next to clean */
	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;

	prefetch(RNP_RX_DESC(rx_ring, ntc));

	/* if we are the last buffer then there is nothing else to do */
	if (likely(rnp_test_staterr(rx_desc, RNP_RXD_STAT_EOP)))
		return false;
	/* place skb in next buffer to be received */
	/* we should clean it since we used all info in it */
	rx_desc->wb.cmd = 0;

	return true;
}

static bool rnp_alloc_mapped_page(struct rnp_ring *rx_ring,
				  struct rnp_rx_buffer *bi,
				  union rnp_rx_desc *rx_desc, u16 bufsz,
				  u64 fun_id)
{
	struct page *page = bi->page;
	dma_addr_t dma;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page))
		return true;

	page = dev_alloc_pages(RNP_ALLOC_PAGE_ORDER);
	if (unlikely(!page)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	bi->page_offset = rnp_rx_offset(rx_ring);

	/* map page for use */
	dma = dma_map_page_attrs(rx_ring->dev, page, bi->page_offset, bufsz,
				 DMA_FROM_DEVICE,
				 RNP_RX_DMA_ATTR);

	/*
	 * if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		__free_pages(page, RNP_ALLOC_PAGE_ORDER);
		printk("map failed\n");

		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}
	bi->dma = dma;
	bi->page = page;
	bi->page_offset = rnp_rx_offset(rx_ring);
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

#else
static bool rnp_alloc_mapped_page(struct rnp_ring *rx_ring,
				  struct rnp_rx_buffer *bi)
{
	struct page *page = bi->page;
	dma_addr_t dma;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page))
		return true;

	page = dev_alloc_pages(rnp_rx_pg_order(rx_ring));
	if (unlikely(!page)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	/* map page for use */
	dma = dma_map_page_attrs(rx_ring->dev, page, 0, rnp_rx_pg_size(rx_ring),
				 DMA_FROM_DEVICE,
				 RNP_RX_DMA_ATTR);

	/*
	 * if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		__free_pages(page, rnp_rx_pg_order(rx_ring));

		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}
	bi->dma = dma;
	bi->page = page;
	bi->page_offset = rnp_rx_offset(rx_ring);
	page_ref_add(page, USHRT_MAX - 1);
	bi->pagecnt_bias = USHRT_MAX;
	rx_ring->rx_stats.alloc_rx_page++;

	return true;
}

/**
 * rnp_is_non_eop - process handling of non-EOP buffers
 * @rx_ring: Rx ring being processed
 * @rx_desc: Rx descriptor for current buffer
 * @skb: Current socket buffer containing buffer in progress
 *
 * This function updates next to clean.  If the buffer is an EOP buffer
 * this function exits returning false, otherwise it will place the
 * sk_buff in the next buffer to be chained and return true indicating
 * that this is in fact a non-EOP buffer.
 **/
static bool rnp_is_non_eop(struct rnp_ring *rx_ring, union rnp_rx_desc *rx_desc,
			   struct sk_buff *skb)
{
	u32 ntc = rx_ring->next_to_clean + 1;

	/* fetch, update, and store next to clean */
	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;

	prefetch(RNP_RX_DESC(rx_ring, ntc));

	/* if we are the last buffer then there is nothing else to do */
	if (likely(rnp_test_staterr(rx_desc, RNP_RXD_STAT_EOP)))
		return false;
	/* place skb in next buffer to be received */
	rx_ring->rx_buffer_info[ntc].skb = skb;
	rx_ring->rx_stats.non_eop_descs++;
	/* we should clean it since we used all info in it */
	rx_desc->wb.cmd = 0;

	return true;
}

#endif
/**
 * rnp_pull_tail - rnp specific version of skb_pull_tail
 * @skb: pointer to current skb being adjusted
 *
 * This function is an rnp specific version of __pskb_pull_tail.  The
 * main difference between this version and the original function is that
 * this function can make several assumptions about the state of things
 * that allow for significant optimizations versus the standard function.
 * As a result we can do things like drop a frag and maintain an accurate
 * truesize for the skb.
 */
static void rnp_pull_tail(struct sk_buff *skb)
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
	pull_len = rnp_get_headlen(va, RNP_RX_HDR_SIZE);

	/* align pull length to size of long to optimize memcpy performance */
	skb_copy_to_linear_data(skb, va, ALIGN(pull_len, sizeof(long)));

	/* update all of the pointers */
	skb_frag_size_sub(frag, pull_len);
	skb_frag_off_add(frag, pull_len);
	skb->data_len -= pull_len;
	skb->tail += pull_len;
}

static bool rnp_check_src_mac(struct sk_buff *skb, struct net_device *netdev)
{
	char *data = (char *)skb->data;
	bool ret = false;
	struct netdev_hw_addr *ha;

	if (is_multicast_ether_addr(data)) {
		if (0 == memcmp(data + netdev->addr_len, netdev->dev_addr,
				netdev->addr_len)) {
			dev_kfree_skb_any(skb);
			ret = true;
		}
		/* if src mac equal own mac */
		netdev_for_each_uc_addr(ha, netdev) {
			if (0 == memcmp(data + netdev->addr_len, ha->addr,
					netdev->addr_len)) {
				dev_kfree_skb_any(skb);
				ret = true;
			}
		}
	}
	return ret;
}

/**
 * rnp_cleanup_headers - Correct corrupted or empty headers
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @rx_desc: pointer to the EOP Rx descriptor
 * @skb: pointer to current skb being fixed
 *
 * Check if the skb is valid. In the XDP case it will be an error pointer.
 * Return true in this case to abort processing and advance to next
 * descriptor.
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
static bool rnp_cleanup_headers(struct rnp_ring __maybe_unused *rx_ring,
				union rnp_rx_desc *rx_desc, struct sk_buff *skb)
{
	struct net_device *netdev = rx_ring->netdev;
	struct rnp_adapter *adapter = netdev_priv(netdev);
#ifdef OPTM_WITH_LPAGE
#else
	/* XDP packets use error pointer so abort at this point */
	if (IS_ERR(skb))
		return true;
#endif

	/* place header in linear portion of buffer */
	if (!skb_headlen(skb))
		rnp_pull_tail(skb);
	/* if eth_skb_pad returns an error the skb was freed */
	if (eth_skb_pad(skb))
		return true;

	if ((adapter->flags & RNP_FLAG_SRIOV_ENABLED) &&
	    (!(rx_ring->ring_flags & RNP_RING_VEB_MULTI_FIX)))
		return rnp_check_src_mac(skb, rx_ring->netdev);
	else
		return false;
}

/**
 * rnp_reuse_rx_page - page flip buffer and store it back on the ring
 * @rx_ring: rx descriptor ring to store buffers on
 * @old_buff: donor buffer to have page reused
 *
 * Synchronizes page for reuse by the adapter
 **/
static void rnp_reuse_rx_page(struct rnp_ring *rx_ring,
			      struct rnp_rx_buffer *old_buff)
{
	struct rnp_rx_buffer *new_buff;
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

static inline bool rnp_page_is_reserved(struct page *page)
{
	return (page_to_nid(page) != numa_mem_id()) || page_is_pfmemalloc(page);
}

static bool rnp_can_reuse_rx_page(struct rnp_rx_buffer *rx_buffer, int size)
{
	unsigned int pagecnt_bias = rx_buffer->pagecnt_bias;
	struct page *page = rx_buffer->page;

#ifdef OPTM_WITH_LPAGE
	return false;
#endif
	/* avoid re-using remote pages */
	if (unlikely(rnp_page_is_reserved(page)))
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
#define RNP_LAST_OFFSET (SKB_WITH_OVERHEAD(PAGE_SIZE))
	if (rx_buffer->page_offset > (RNP_LAST_OFFSET - size))
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

/**
 * rnp_add_rx_frag - Add contents of Rx buffer to sk_buff
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
static void rnp_add_rx_frag(struct rnp_ring *rx_ring,
			    struct rnp_rx_buffer *rx_buffer,
			    struct sk_buff *skb, unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnp_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize = ring_uses_build_skb(rx_ring) ?
					SKB_DATA_ALIGN(RNP_SKB_PAD + size) :
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
static struct rnp_rx_buffer *rnp_get_rx_buffer(struct rnp_ring *rx_ring,
					       union rnp_rx_desc *rx_desc,
					       const unsigned int size)
{
	struct rnp_rx_buffer *rx_buffer;

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
#else

static struct rnp_rx_buffer *rnp_get_rx_buffer(struct rnp_ring *rx_ring,
					       union rnp_rx_desc *rx_desc,
					       struct sk_buff **skb,
					       const unsigned int size)
{
	struct rnp_rx_buffer *rx_buffer;

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
	rx_buffer->pagecnt_bias--;

	return rx_buffer;
}
#endif

#ifdef OPTM_WITH_LPAGE
static void rnp_put_rx_buffer(struct rnp_ring *rx_ring,
			      struct rnp_rx_buffer *rx_buffer)
{
	struct rnp_q_vector *q_vector = rx_ring->q_vector;
	struct rnp_adapter *adapter = q_vector->adapter;
	struct rnp_hw *hw = &adapter->hw;

	if (rnp_can_reuse_rx_page(rx_buffer, hw->dma_split_size)) {
		/* hand second half of page back to the ring */
		rnp_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		/* we are not reusing the buffer so unmap it */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnp_rx_bufsz(rx_ring), DMA_FROM_DEVICE,
				     RNP_RX_DMA_ATTR);
		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
	}

	/* clear contents of rx_buffer */
	rx_buffer->page = NULL;
}

#else
static void rnp_put_rx_buffer(struct rnp_ring *rx_ring,
			      struct rnp_rx_buffer *rx_buffer,
			      struct sk_buff *skb)
{
	struct rnp_q_vector *q_vector = rx_ring->q_vector;
	struct rnp_adapter *adapter = q_vector->adapter;
	struct rnp_hw *hw = &adapter->hw;

	if (rnp_can_reuse_rx_page(rx_buffer, hw->dma_split_size)) {
		/* hand second half of page back to the ring */
		rnp_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		/* we are not reusing the buffer so unmap it */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnp_rx_pg_size(rx_ring), DMA_FROM_DEVICE,
				     RNP_RX_DMA_ATTR);
		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
	}

	/* clear contents of rx_buffer */
	rx_buffer->page = NULL;
	rx_buffer->skb = NULL;
}
#endif

#ifdef OPTM_WITH_LPAGE
static struct sk_buff *rnp_construct_skb(struct rnp_ring *rx_ring,
					 struct rnp_rx_buffer *rx_buffer,
					 union rnp_rx_desc *rx_desc,
					 unsigned int size)
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
	skb = napi_alloc_skb(&rx_ring->q_vector->napi, RNP_RX_HDR_SIZE);
	if (unlikely(!skb))
		return NULL;

	prefetchw(skb->data);

	/* Determine available headroom for copy */
	headlen = size;
	if (headlen > RNP_RX_HDR_SIZE)
		headlen = rnp_get_headlen(va, RNP_RX_HDR_SIZE);

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

static struct sk_buff *rnp_build_skb(struct rnp_ring *rx_ring,
				     struct rnp_rx_buffer *rx_buffer,
				     union rnp_rx_desc *rx_desc,
				     unsigned int size)
{
	void *va = page_address(rx_buffer->page) + rx_buffer->page_offset;
	unsigned int truesize = SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
				SKB_DATA_ALIGN(size + RNP_SKB_PAD);
	struct sk_buff *skb;

	/* prefetch first cache line of first page */
	prefetch(va);
#if L1_CACHE_BYTES < 128
	prefetch(va + L1_CACHE_BYTES);
#endif

	/* build an skb around the page buffer */
	skb = build_skb(va - RNP_SKB_PAD, truesize);
	if (unlikely(!skb))
		return NULL;

	/* update pointers within the skb to store the data */
	skb_reserve(skb, RNP_SKB_PAD);
	__skb_put(skb, size);
	/* record DMA address if this is the start of a
	 * chain of buffers
	 */

	return skb;
}

#else

static struct sk_buff *rnp_construct_skb(struct rnp_ring *rx_ring,
					 struct rnp_rx_buffer *rx_buffer,
					 struct xdp_buff *xdp,
					 union rnp_rx_desc *rx_desc)
{
	unsigned int size = xdp->data_end - xdp->data;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnp_rx_pg_size(rx_ring) / 2;
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
	skb = napi_alloc_skb(&rx_ring->q_vector->napi, RNP_RX_HDR_SIZE);
	if (unlikely(!skb))
		return NULL;

	prefetchw(skb->data);

	if (size > RNP_RX_HDR_SIZE) {

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

static struct sk_buff *rnp_build_skb(struct rnp_ring *rx_ring,
				     struct rnp_rx_buffer *rx_buffer,
				     struct xdp_buff *xdp,
				     union rnp_rx_desc *rx_desc)
{
	unsigned int metasize = xdp->data - xdp->data_meta;
	void *va = xdp->data_meta;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnp_rx_pg_size(rx_ring) / 2;
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

#endif

#define RNP_XDP_PASS 0
#define RNP_XDP_CONSUMED 1
#define RNP_XDP_TX 2

#ifndef OPTM_WITH_LPAGE
static void rnp_rx_buffer_flip(struct rnp_ring *rx_ring,
			       struct rnp_rx_buffer *rx_buffer,
			       unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnp_rx_pg_size(rx_ring) / 2;

	rx_buffer->page_offset ^= truesize;
#else
	unsigned int truesize = ring_uses_build_skb(rx_ring) ?
					SKB_DATA_ALIGN(RNP_SKB_PAD + size) :
					SKB_DATA_ALIGN(size);

	rx_buffer->page_offset += truesize;
#endif
}
#endif

#ifdef OPTM_WITH_LPAGE
/**
 * rnp_clean_rx_irq - Clean completed descriptors from Rx ring - bounce buf
 * @q_vector: structure containing interrupt and ring information
 * @rx_ring: rx descriptor ring to transact packets on
 * @budget: Total limit on number of packets to process
 *
 * This function provides a "bounce buffer" approach to Rx interrupt
 * processing.  The advantage to this is that on systems that have
 * expensive overhead for IOMMU access this provides a means of avoiding
 * it by maintaining the mapping of the page to the system.
 *
 * Returns amount of work completed.
 **/

static int rnp_clean_rx_irq(struct rnp_q_vector *q_vector,
			    struct rnp_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	unsigned int err_packets = 0;
	unsigned int driver_drop_packets = 0;
	struct sk_buff *skb = rx_ring->skb;
	struct rnp_adapter *adapter = q_vector->adapter;
	u16 cleaned_count = rnp_desc_unused_rx(rx_ring);

	while (likely(total_rx_packets < budget)) {
		union rnp_rx_desc *rx_desc;
		struct rnp_rx_buffer *rx_buffer;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= RNP_RX_BUFFER_WRITE) {
			rnp_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}
		rx_desc = RNP_RX_DESC(rx_ring, rx_ring->next_to_clean);

		rx_buf_dump("rx-desc:", rx_desc, sizeof(*rx_desc));
		rx_debug_printk("  dd set: %s\n",
				(rx_desc->wb.cmd & RNP_RXD_STAT_DD) ? "Yes" :
								      "No");

		if (!rnp_test_staterr(rx_desc, RNP_RXD_STAT_DD))
			break;

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();

		rx_debug_printk(
			"queue:%d  rx-desc:%d has-data len:%d next_to_clean %d\n",
			rx_ring->rnp_queue_idx, rx_ring->next_to_clean,
			rx_desc->wb.len, rx_ring->next_to_clean);

		/* handle padding */
		if ((adapter->priv_flags & RNP_PRIV_FLAG_FT_PADDING) &&
		    (!(adapter->priv_flags & RNP_PRIV_FLAG_PADDING_DEBUG))) {
			if (likely(rnp_test_staterr(rx_desc,
						    RNP_RXD_STAT_EOP))) {
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
		 * maybe one packet use multiple descs
		 * no problems hw set all csum_err in multiple descs
		 * maybe BUG if the last sctp desc less than 60
		 */
		if (rnp_check_csum_error(rx_ring, rx_desc, size,
					 &driver_drop_packets)) {
			cleaned_count++;
			err_packets++;
			if (err_packets + total_rx_packets > budget)
				break;
			continue;
		}

		rx_buffer = rnp_get_rx_buffer(rx_ring, rx_desc, size);

		if (skb) {
			rnp_add_rx_frag(rx_ring, rx_buffer, skb, size);
		} else if (ring_uses_build_skb(rx_ring)) {
			skb = rnp_build_skb(rx_ring, rx_buffer, rx_desc, size);
		} else {
			skb = rnp_construct_skb(rx_ring, rx_buffer, rx_desc,
						size);
		}

		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			rx_buffer->pagecnt_bias++;
			break;
		}
		if (module_enable_ptp && adapter->ptp_rx_en &&
		    adapter->flags2 & RNP_FLAG2_PTP_ENABLED)
			rnp_ptp_get_rx_hwstamp(adapter, rx_desc, skb);

		rnp_put_rx_buffer(rx_ring, rx_buffer);
		cleaned_count++;

		/* place incomplete frames back on ring for completion */
		if (rnp_is_non_eop(rx_ring, rx_desc))
			continue;

		/* verify the packet layout is correct */
		if (rnp_cleanup_headers(rx_ring, rx_desc, skb)) {
			skb = NULL;
			/* we should clean it since we used all info in it */
			rx_desc->wb.cmd = 0;
			continue;
		}

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, timestamp, VLAN, and protocol */
		rnp_process_skb_fields(rx_ring, rx_desc, skb);

		/* we should clean it since we used all info in it */
		rx_desc->wb.cmd = 0;
		rnp_rx_skb(q_vector, skb);
		skb = NULL;

		/* update budget accounting */
		total_rx_packets++;
	}

	rx_ring->skb = skb;

	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	rx_ring->rx_stats.driver_drop_packets += driver_drop_packets;
	rx_ring->rx_stats.rx_clean_count += total_rx_packets;
	rx_ring->rx_stats.rx_clean_times++;
	if (rx_ring->rx_stats.rx_clean_times > 10) {
		rx_ring->rx_stats.rx_clean_times = 0;
		rx_ring->rx_stats.rx_clean_count = 0;
	}
	u64_stats_update_end(&rx_ring->syncp);
	q_vector->rx.total_packets += total_rx_packets;
	q_vector->rx.total_bytes += total_rx_bytes;

	if (total_rx_packets >= budget)
		rx_ring->rx_stats.poll_again_count++;

	return total_rx_packets;
}

#else
/**
 * rnp_clean_rx_irq - Clean completed descriptors from Rx ring - bounce buf
 * @q_vector: structure containing interrupt and ring information
 * @rx_ring: rx descriptor ring to transact packets on
 * @budget: Total limit on number of packets to process
 *
 * This function provides a "bounce buffer" approach to Rx interrupt
 * processing.  The advantage to this is that on systems that have
 * expensive overhead for IOMMU access this provides a means of avoiding
 * it by maintaining the mapping of the page to the system.
 *
 * Returns amount of work completed.
 **/
static int rnp_clean_rx_irq(struct rnp_q_vector *q_vector,
			    struct rnp_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	unsigned int err_packets = 0;
	unsigned int driver_drop_packets = 0;
	struct rnp_adapter *adapter = q_vector->adapter;
	u16 cleaned_count = rnp_desc_unused_rx(rx_ring);
	bool xdp_xmit = false;
	struct xdp_buff xdp;

	xdp.data = NULL;
	xdp.data_end = NULL;

	while (likely(total_rx_packets < budget)) {
		union rnp_rx_desc *rx_desc;
		struct rnp_rx_buffer *rx_buffer;
		struct sk_buff *skb;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= RNP_RX_BUFFER_WRITE) {
			rnp_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}
		rx_desc = RNP_RX_DESC(rx_ring, rx_ring->next_to_clean);

		rx_buf_dump("rx-desc:", rx_desc, sizeof(*rx_desc));
		rx_debug_printk("  dd set: %s\n",
				(rx_desc->wb.cmd & RNP_RXD_STAT_DD) ? "Yes" :
								      "No");

		if (!rnp_test_staterr(rx_desc, RNP_RXD_STAT_DD))
			break;

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();

		rx_debug_printk(
			"queue:%d  rx-desc:%d has-data len:%d next_to_clean %d\n",
			rx_ring->rnp_queue_idx, rx_ring->next_to_clean,
			rx_desc->wb.len, rx_ring->next_to_clean);

		/* handle padding */
		if ((adapter->priv_flags & RNP_PRIV_FLAG_FT_PADDING) &&
		    (!(adapter->priv_flags & RNP_PRIV_FLAG_PADDING_DEBUG))) {
			if (likely(rnp_test_staterr(rx_desc,
						    RNP_RXD_STAT_EOP))) {
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
		 * maybe one packet use multiple descs
		 * no problems hw set all csum_err in multiple descs
		 * maybe BUG if the last sctp desc less than 60
		 */
		if (rnp_check_csum_error(rx_ring, rx_desc, size,
					 &driver_drop_packets)) {
			cleaned_count++;
			err_packets++;
			if (err_packets + total_rx_packets > budget)
				break;
			continue;
		}

		rx_buffer = rnp_get_rx_buffer(rx_ring, rx_desc, &skb, size);

		if (!skb) {
			xdp.data = page_address(rx_buffer->page) +
				   rx_buffer->page_offset;
			xdp.data_meta = xdp.data;
			xdp.data_hard_start = xdp.data - rnp_rx_offset(rx_ring);
			xdp.data_end = xdp.data + size;
		}

		if (IS_ERR(skb)) {
			if (PTR_ERR(skb) == -RNP_XDP_TX) {
				xdp_xmit = true;
				rnp_rx_buffer_flip(rx_ring, rx_buffer, size);
			} else {
				rx_buffer->pagecnt_bias++;
			}
			total_rx_packets++;
			total_rx_bytes += size;
		} else if (skb) {
			rnp_add_rx_frag(rx_ring, rx_buffer, skb, size);
		} else if (ring_uses_build_skb(rx_ring)) {
			skb = rnp_build_skb(rx_ring, rx_buffer, &xdp, rx_desc);
		} else {
			skb = rnp_construct_skb(rx_ring, rx_buffer, &xdp,
						rx_desc);
		}

		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			rx_buffer->pagecnt_bias++;
			break;
		}
		if (module_enable_ptp && adapter->ptp_rx_en &&
		    adapter->flags2 & RNP_FLAG2_PTP_ENABLED)
			rnp_ptp_get_rx_hwstamp(adapter, rx_desc, skb);

		rnp_put_rx_buffer(rx_ring, rx_buffer, skb);
		cleaned_count++;

		/* place incomplete frames back on ring for completion */
		if (rnp_is_non_eop(rx_ring, rx_desc, skb))
			continue;

		/* verify the packet layout is correct */
		if (rnp_cleanup_headers(rx_ring, rx_desc, skb)) {
			/* we should clean it since we used all info in it */
			rx_desc->wb.cmd = 0;
			continue;
		}

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, timestamp, VLAN, and protocol */
		rnp_process_skb_fields(rx_ring, rx_desc, skb);

		/* we should clean it since we used all info in it */
		rx_desc->wb.cmd = 0;

		rnp_rx_skb(q_vector, skb);

		/* update budget accounting */
		total_rx_packets++;
	}
	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	rx_ring->rx_stats.driver_drop_packets += driver_drop_packets;
	rx_ring->rx_stats.rx_clean_count += total_rx_packets;
	rx_ring->rx_stats.rx_clean_times++;
	if (rx_ring->rx_stats.rx_clean_times > 10) {
		rx_ring->rx_stats.rx_clean_times = 0;
		rx_ring->rx_stats.rx_clean_count = 0;
	}
	u64_stats_update_end(&rx_ring->syncp);
	q_vector->rx.total_packets += total_rx_packets;
	q_vector->rx.total_bytes += total_rx_bytes;

	if (total_rx_packets >= budget)
		rx_ring->rx_stats.poll_again_count++;
	return total_rx_packets;
}
#endif

/**
 * rnp_configure_msix - Configure MSI-X hardware
 * @adapter: board private structure
 *
 * rnp_configure_msix sets up the hardware to properly generate MSI-X
 * interrupts.
 **/
static void rnp_configure_msix(struct rnp_adapter *adapter)
{
	struct rnp_q_vector *q_vector;
	int i;

	/*
	 * configure ring-msix Registers table
	 */
	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnp_ring *ring;

		q_vector = adapter->q_vector[i];
		rnp_for_each_ring(ring, q_vector->rx) {
			rnp_set_ring_vector(adapter, ring->rnp_queue_idx,
					    q_vector->v_idx);
		}
	}
}

/**
 * rnp_update_itr - update the dynamic ITR value based on statistics
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
static void rnp_update_itr(struct rnp_q_vector *q_vector,
			   struct rnp_ring_container *ring_container, int type)
{
	unsigned int itr = RNP_ITR_ADAPTIVE_MIN_USECS |
			   RNP_ITR_ADAPTIVE_LATENCY;
	unsigned int avg_wire_size, packets, bytes;
	unsigned int packets_old;
	unsigned long next_update = jiffies;
	u32 old_itr;
	u16 add_itr, add = 0;
	/* 0 is tx ;1 is rx */
	if (type)
		old_itr = q_vector->itr_rx;
	else
		old_itr = q_vector->itr_tx;

	/* If we don't have any rings just leave ourselves set for maximum
	 * possible latency so we take ourselves out of the equation.
	 */
	if (!ring_container->ring)
		return;

	packets_old = ring_container->total_packets_old;
	packets = ring_container->total_packets;
	bytes = ring_container->total_bytes;
	add_itr = ring_container->add_itr;
	/* If Rx and there are 1 to 23 packets and bytes are less than
	 * 12112 assume insufficient data to use bulk rate limiting
	 * approach. Instead we will focus on simply trying to target
	 * receiving 8 times as much data in the next interrupt.
	 */
	if (!packets)
		return;

	if (packets && packets < 24 && bytes < 12112) {
		itr = RNP_ITR_ADAPTIVE_LATENCY;

		avg_wire_size = (bytes + packets * 24);
		avg_wire_size =
			clamp_t(unsigned int, avg_wire_size, 128, 12800);

		goto adjust_for_speed;
	}

	/* Less than 48 packets we can assume that our current interrupt delay
	 * is only slightly too low. As such we should increase it by a small
	 * fixed amount.
	 */
	if (packets < 48) {
		if (add_itr) {
			if (packets_old < packets) {
				itr = (old_itr >> 2) + RNP_ITR_ADAPTIVE_MIN_INC;
				if (itr > RNP_ITR_ADAPTIVE_MAX_USECS)
					itr = RNP_ITR_ADAPTIVE_MAX_USECS;
				add = 1;

				if (packets < 8)
					itr += RNP_ITR_ADAPTIVE_LATENCY;
				else
					itr += ring_container->itr &
					       RNP_ITR_ADAPTIVE_LATENCY;

			} else {
				/* we add itr before ,but not get more packets */
				itr = (old_itr >> 2) - RNP_ITR_ADAPTIVE_MIN_INC;
				if (itr < RNP_ITR_ADAPTIVE_MIN_USECS)
					itr = RNP_ITR_ADAPTIVE_MIN_USECS;
			}

		} else {
			/* we not add before, add itr */
			add = 1;
			itr = (old_itr >> 2) + RNP_ITR_ADAPTIVE_MIN_INC;
			if (itr > RNP_ITR_ADAPTIVE_MAX_USECS)
				itr = RNP_ITR_ADAPTIVE_MAX_USECS;

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
				itr += RNP_ITR_ADAPTIVE_LATENCY;
			else
				itr += ring_container->itr &
				       RNP_ITR_ADAPTIVE_LATENCY;
		}
		goto clear_counts;
	}

	/* Between 48 and 96 is our "goldilocks" zone where we are working
	 * out "just right". Just report that our current ITR is good for us.
	 */
	if (packets < 96) {
		itr = old_itr >> 2;
		goto clear_counts;
	}
	/* If packet count is 96 or greater we are likely looking at a slight
	 * overrun of the delay we want. Try halving our delay to see if that
	 * will cut the number of packets in half per interrupt.
	 */
	if (packets < 256) {
		itr = old_itr >> 3;
		if (itr < RNP_ITR_ADAPTIVE_MIN_USECS)
			itr = RNP_ITR_ADAPTIVE_MIN_USECS;
		goto clear_counts;
	}

	/* The paths below assume we are dealing with a bulk ITR since number
	 * of packets is 256 or greater. We are just going to have to compute
	 * a value and try to bring the count under control, though for smaller
	 * packet sizes there isn't much we can do as NAPI polling will likely
	 * be kicking in sooner rather than later.
	 */
	itr = RNP_ITR_ADAPTIVE_BULK;

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
	switch (q_vector->adapter->link_speed) {
	case RNP_LINK_SPEED_10GB_FULL:
	case RNP_LINK_SPEED_100_FULL:
	default:
		itr += DIV_ROUND_UP(avg_wire_size,
				    RNP_ITR_ADAPTIVE_MIN_INC * 256) *
		       RNP_ITR_ADAPTIVE_MIN_INC;
		break;
	case RNP_LINK_SPEED_1GB_FULL:
	case RNP_LINK_SPEED_10_FULL:
		itr += DIV_ROUND_UP(avg_wire_size,
				    RNP_ITR_ADAPTIVE_MIN_INC * 64) *
		       RNP_ITR_ADAPTIVE_MIN_INC;
		break;
	}

	/* In the case of a latency specific workload only allow us to
	 * reduce the ITR by at most 2us. By doing this we should dial
	 * in so that our number of interrupts is no more than 2x the number
	 * of packets for the least busy workload. So for example in the case
	 * of a TCP worload the ack packets being received would set the
	 * the interrupt rate as they are a latency specific workload.
	 */
	if ((itr & RNP_ITR_ADAPTIVE_LATENCY) && itr < ring_container->itr)
		itr = ring_container->itr - RNP_ITR_ADAPTIVE_MIN_INC;

clear_counts:
	/* write back value */
	ring_container->itr = itr;

	/* next update should occur within next jiffy */
	ring_container->next_update = next_update + 1;

	ring_container->total_bytes = 0;
	ring_container->total_packets_old = packets;
	ring_container->add_itr = add;
	ring_container->total_packets = 0;
}

/**
 * rnp_write_eitr - write EITR register in hardware specific way
 * @q_vector: structure containing interrupt and ring information
 *
 * This function is made to be called by ethtool and by the driver
 * when it needs to update EITR registers at runtime.  Hardware
 * specific quirks/differences are taken care of here.
 */
static void rnp_write_eitr_rx(struct rnp_q_vector *q_vector)
{
	struct rnp_adapter *adapter = q_vector->adapter;
	struct rnp_hw *hw = &adapter->hw;
	u32 itr_reg = q_vector->itr_rx >> 2;
	struct rnp_ring *ring;

	itr_reg = itr_reg * hw->usecstocount;

	rnp_for_each_ring(ring, q_vector->rx) {
		ring_wr32(ring, RNP_DMA_REG_RX_INT_DELAY_TIMER, itr_reg);
	}
}

static void rnp_set_itr(struct rnp_q_vector *q_vector)
{
	u32 new_itr_rx;

	rnp_update_itr(q_vector, &q_vector->rx, 1);
	new_itr_rx = q_vector->rx.itr;
	new_itr_rx &= RNP_ITR_ADAPTIVE_MASK_USECS;
	new_itr_rx <<= 2;
	if (new_itr_rx != q_vector->itr_rx) {
		/* save the algorithm value here */
		q_vector->itr_rx = new_itr_rx;
		rnp_write_eitr_rx(q_vector);
	}
}

enum latency_range {
	lowest_latency = 0,
	low_latency = 1,
	bulk_latency = 2,
	latency_invalid = 255
};

static inline void rnp_irq_enable_queues(struct rnp_adapter *adapter,
					 struct rnp_q_vector *q_vector)
{
	struct rnp_ring *ring;

	rnp_for_each_ring(ring, q_vector->rx)
		rnp_wr_reg(ring->dma_int_mask, ~(RX_INT_MASK | TX_INT_MASK));
}

static inline void rnp_irq_disable_queues(struct rnp_q_vector *q_vector)
{
	struct rnp_ring *ring;

	rnp_for_each_ring(ring, q_vector->tx) {
		if (q_vector->new_rx_count != q_vector->old_rx_count) {
			ring_wr32(ring, RNP_DMA_REG_RX_INT_DELAY_PKTCNT,
				  q_vector->new_rx_count);
			q_vector->old_rx_count = q_vector->new_rx_count;
		}
		rnp_wr_reg(ring->dma_int_mask, (RX_INT_MASK | TX_INT_MASK));
	}
}

/**
 * rnp_irq_enable - Enable default interrupt generation settings
 * @adapter: board private structure
 **/
static inline void rnp_irq_enable(struct rnp_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_q_vectors; i++)
		rnp_irq_enable_queues(adapter, adapter->q_vector[i]);
}

static irqreturn_t rnp_msix_other(int irq, void *data)
{
	struct rnp_adapter *adapter = data;
	set_bit(__RNP_IN_IRQ, &adapter->state);

	rnp_msg_task(adapter);

	clear_bit(__RNP_IN_IRQ, &adapter->state);

	return IRQ_HANDLED;
}

static void rnp_htimer_start(struct rnp_q_vector *q_vector)
{
	unsigned long ns = q_vector->irq_check_usecs * NSEC_PER_USEC / 2;

	hrtimer_start_range_ns(&q_vector->irq_miss_check_timer, ns_to_ktime(ns),
			       ns, HRTIMER_MODE_REL);
}

static void rnp_htimer_stop(struct rnp_q_vector *q_vector)
{
	hrtimer_cancel(&q_vector->irq_miss_check_timer);
}

static irqreturn_t rnp_msix_clean_rings(int irq, void *data)
{
	struct rnp_q_vector *q_vector = data;

	if (q_vector->vector_flags & RNP_QVECTOR_FLAG_IRQ_MISS_CHECK)
		rnp_htimer_stop(q_vector);

	/*  disabled interrupts (on this vector) for us */
	rnp_irq_disable_queues(q_vector);

	if (q_vector->rx.ring || q_vector->tx.ring)
		napi_schedule_irqoff(&q_vector->napi);

	return IRQ_HANDLED;
}

static void update_rx_count(int cleaned, struct rnp_q_vector *q_vector)
{
	struct rnp_adapter *adapter = q_vector->adapter;
	u32 link_speed = adapter->link_speed;
	struct rnp_ring *ring;

	if (link_speed == RNP_LINK_SPEED_10GB_FULL) {
		if ((cleaned) && (cleaned != q_vector->new_rx_count)) {
			if (cleaned < 5) {
				q_vector->small_times = 0;
				q_vector->large_times = 0;
				q_vector->too_small_times++;
				if (q_vector->too_small_times >= 2) {
					q_vector->new_rx_count = 1;
				}
			} else if (cleaned < 30) {
				q_vector->too_small_times = 0;
				q_vector->middle_time++;
				/* count is 5 -30 */
				if (cleaned < q_vector->new_rx_count) {
					/* change small */
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
						/* should no more than q_vector */
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
						/* should no more than q_vector */
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
	} else {
		rnp_for_each_ring(ring, q_vector->rx) {
			if (ring->ring_flags & RNP_RING_LOWER_ITR) {
				q_vector->new_rx_count = 1;
			} else {
				q_vector->new_rx_count = 32;
			}
		}


	}
}

/**
 * rnp_poll - NAPI Rx polling callback
 * @napi: structure for representing this polling device
 * @budget: how many packets driver is allowed to clean
 *
 * This function is used for legacy and MSI, NAPI mode
 **/
int rnp_poll(struct napi_struct *napi, int budget)
{
	struct rnp_q_vector *q_vector =
		container_of(napi, struct rnp_q_vector, napi);
	struct rnp_adapter *adapter = q_vector->adapter;
	struct rnp_ring *ring;
	int per_ring_budget, work_done = 0;
	bool clean_complete = true;
	int cleaned_total = 0;

	rnp_for_each_ring(ring, q_vector->tx) {
		if (!rnp_clean_tx_irq(q_vector, ring, budget))
			clean_complete = false;
	}

	/* attempt to distribute budget to each queue fairly, but don't allow
	 * the budget to go below 1 because we'll exit polling
	 */
	if (q_vector->rx.count > 1)
		per_ring_budget = max(budget / q_vector->rx.count, 1);
	else
		per_ring_budget = budget;

	rnp_for_each_ring(ring, q_vector->rx) {
		int cleaned = 0;
		/* this ring is waitting to reset rx_len*/
		/* avoid to deal this ring until reset done */
		if (likely(!(ring->ring_flags & RNP_RING_FLAG_DO_RESET_RX_LEN)))
			cleaned = rnp_clean_rx_irq(q_vector, ring,
						   per_ring_budget);
		/* check delay rx setup */
		if (unlikely(ring->ring_flags &
			     RNP_RING_FLAG_DELAY_SETUP_RX_LEN)) {
			int head;

			rnp_disable_rx_queue(adapter, ring);
			head = ring_rd32(ring, RNP_DMA_REG_RX_DESC_BUF_HEAD);
			if (head < RNP_MIN_RXD) {
				/* it is time to delay set */
				/* stop rx */
				rnp_disable_rx_queue(adapter, ring);
				ring->ring_flags &=
					(~RNP_RING_FLAG_DELAY_SETUP_RX_LEN);
				ring->ring_flags |=
					RNP_RING_FLAG_DO_RESET_RX_LEN;
			} else {

				ring_wr32(ring, RNP_DMA_RX_START, 1);
			}
		}
		work_done += cleaned;
		cleaned_total += cleaned;
		if (cleaned >= per_ring_budget)
			clean_complete = false;
	}

	/* force close irq */
	if (test_bit(__RNP_DOWN, &adapter->state)) {
		clean_complete = true;
	}
	/* all work done, exit the polling mode */
	if (!(q_vector->vector_flags & RNP_QVECTOR_FLAG_ITR_FEATURE))
		update_rx_count(cleaned_total, q_vector);

	if (!clean_complete) {
		int cpu_id = smp_processor_id();

		/* It is possible that the interrupt affinity has changed but,
		 * if the cpu is pegged at 100%, polling will never exit while
		 * traffic continues and the interrupt will be stuck on this
		 * cpu.  We check to make sure affinity is correct before we
		 * continue to poll, otherwise we must stop polling so the
		 * interrupt can move to the correct cpu.
		 */
		if (!cpumask_test_cpu(cpu_id, &q_vector->affinity_mask)) {
			/* Tell napi that we are done polling */
			napi_complete_done(napi, work_done);
			if (!test_bit(__RNP_DOWN, &adapter->state)) {
				rnp_irq_enable_queues(adapter, q_vector);
				/* we need this to ensure irq start before tx start */
				if (q_vector->vector_flags &
				    RNP_QVECTOR_FLAG_REDUCE_TX_IRQ_MISS) {
					smp_mb();
					rnp_for_each_ring(ring, q_vector->tx) {
						rnp_check_restart_tx(q_vector,
								     ring);
						if (q_vector->new_rx_count !=
						    q_vector->old_rx_count) {
							ring_wr32(
								ring,
								RNP_DMA_REG_RX_INT_DELAY_PKTCNT,
								q_vector->new_rx_count);
							q_vector->old_rx_count =
								q_vector->new_rx_count;
						}
					}
				}
				if (!test_bit(__RNP_DOWN, &adapter->state)) {
					if (q_vector->vector_flags &
					    RNP_QVECTOR_FLAG_IRQ_MISS_CHECK)
						rnp_htimer_start(q_vector);
					/* Return budget-1 so that polling stops */
					return budget - 1;
				}
			}
			return min(work_done, budget - 1);
		}
		if (q_vector->vector_flags &
		    RNP_QVECTOR_FLAG_REDUCE_TX_IRQ_MISS) {
			rnp_for_each_ring(ring, q_vector->tx) {
				rnp_check_restart_tx(q_vector, ring);
				/* update rx count if need */
				if (q_vector->new_rx_count !=
				    q_vector->old_rx_count) {
					ring_wr32(ring,
						RNP_DMA_REG_RX_INT_DELAY_PKTCNT,
						q_vector->new_rx_count);
					q_vector->old_rx_count =
						q_vector->new_rx_count;
				}
			}
		}
		return budget;
	}

	if (likely(napi_complete_done(napi, work_done))) {
		/* try to do itr handle */
		if (q_vector->vector_flags & RNP_QVECTOR_FLAG_ITR_FEATURE)
			rnp_set_itr(q_vector);

		if (!test_bit(__RNP_DOWN, &adapter->state)) {
			rnp_irq_enable_queues(adapter, q_vector);
			smp_mb();
			/* we need this to ensure irq start before tx start */
			if (q_vector->vector_flags &
			    RNP_QVECTOR_FLAG_REDUCE_TX_IRQ_MISS) {
				rnp_for_each_ring(ring, q_vector->tx) {
					rnp_check_restart_tx(q_vector, ring);
					if (q_vector->new_rx_count !=
					    q_vector->old_rx_count) {
						ring_wr32(
							ring,
							RNP_DMA_REG_RX_INT_DELAY_PKTCNT,
							q_vector->new_rx_count);
						q_vector->old_rx_count =
							q_vector->new_rx_count;
					}
				}
			}
			if (q_vector->vector_flags & RNP_QVECTOR_FLAG_IRQ_MISS_CHECK)
				rnp_htimer_start(q_vector);

		}
	}

	return min(work_done, budget - 1);
}

/**
 * rnp_irq_affinity_notify - Callback for affinity changes
 * @notify: context as to what irq was changed
 * @mask: the new affinity mask
 *
 * This is a callback function used by the irq_set_affinity_notifier function
 * so that we may register to receive changes to the irq affinity masks.
 **/
static void rnp_irq_affinity_notify(struct irq_affinity_notify *notify,
				    const cpumask_t *mask)
{
	struct rnp_q_vector *q_vector =
		container_of(notify, struct rnp_q_vector, affinity_notify);

	cpumask_copy(&q_vector->affinity_mask, mask);
}

/**
 * rnp_irq_affinity_release - Callback for affinity notifier release
 * @ref: internal core kernel usage
 *
 * This is a callback function used by the irq_set_affinity_notifier function
 * to inform the current notification subscriber that they will no longer
 * receive notifications.
 **/
static void rnp_irq_affinity_release(struct kref *ref)
{
}

static irqreturn_t rnp_intr(int irq, void *data)
{
	struct rnp_adapter *adapter = data;
	struct rnp_q_vector *q_vector = adapter->q_vector[0];
	if (q_vector->vector_flags & RNP_QVECTOR_FLAG_IRQ_MISS_CHECK)
		rnp_htimer_stop(q_vector);

	/*  disabled interrupts (on this vector) for us */
	rnp_irq_disable_queues(q_vector);

	if (q_vector->rx.ring || q_vector->tx.ring)
		napi_schedule_irqoff(&q_vector->napi);

	/* handle other */
	rnp_msg_task(adapter);

	return IRQ_HANDLED;
}

/**
 * rnp_request_msix_irqs - Initialize MSI-X interrupts
 * @adapter: board private structure
 *
 * rnp_request_msix_irqs allocates MSI-X vectors and requests
 * interrupts from the kernel.
 **/
static int rnp_request_msix_irqs(struct rnp_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int err;
	int i = 0;
	DPRINTK(IFUP, INFO, "[%s] num_q_vectors:%d\n", __func__,
		adapter->num_q_vectors);

	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnp_q_vector *q_vector = adapter->q_vector[i];
		struct msix_entry *entry =
			&adapter->msix_entries[i + adapter->q_vector_off];

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
		err = request_irq(entry->vector, &rnp_msix_clean_rings, 0,
				  q_vector->name, q_vector);
		if (err) {
			e_err(probe,
			      "%s:request_irq failed for MSIX interrupt:%d "
			      "Error: %d\n",
			      netdev->name, entry->vector, err);
			goto free_queue_irqs;
		}
		/* register for affinity change notifications */
		q_vector->affinity_notify.notify = rnp_irq_affinity_notify;
		q_vector->affinity_notify.release = rnp_irq_affinity_release;
		irq_set_affinity_notifier(entry->vector,
					  &q_vector->affinity_notify);
		DPRINTK(IFUP, INFO, "[%s] set %s affinity_mask\n", __func__,
			q_vector->name);

		irq_set_affinity_hint(entry->vector, &q_vector->affinity_mask);
	}

	return 0;

free_queue_irqs:
	while (i) {
		i--;
		irq_set_affinity_hint(
			adapter->msix_entries[i + adapter->q_vector_off].vector,
			NULL);
		free_irq(
			adapter->msix_entries[i + adapter->q_vector_off].vector,
			adapter->q_vector[i]);
		irq_set_affinity_notifier(
			adapter->msix_entries[i + adapter->q_vector_off].vector,
			NULL);
		irq_set_affinity_hint(
			adapter->msix_entries[i + adapter->q_vector_off].vector,
			NULL);
	}
	return err;
}

static int rnp_free_msix_irqs(struct rnp_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnp_q_vector *q_vector = adapter->q_vector[i];
		struct msix_entry *entry =
			&adapter->msix_entries[i + adapter->q_vector_off];

		/* free only the irqs that were actually requested */
		if (!q_vector->rx.ring && !q_vector->tx.ring)
			continue;
		/* clear the affinity notifier in the IRQ descriptor */
		irq_set_affinity_notifier(entry->vector, NULL);
		/* clear the affinity_mask in the IRQ descriptor */
		irq_set_affinity_hint(entry->vector, NULL);
		DPRINTK(IFDOWN, INFO, "free irq %s\n", q_vector->name);
		free_irq(entry->vector, q_vector);
	}

	return 0;
}

/**
 * rnp_request_irq - initialize interrupts
 * @adapter: board private structure
 *
 * Attempts to configure interrupts using the best available
 * capabilities of the hardware and kernel.
 **/
static int rnp_request_irq(struct rnp_adapter *adapter)
{
	int err;

	if (adapter->flags & RNP_FLAG_MSIX_ENABLED) {
		pr_info("msix mode is used\n");
		err = rnp_request_msix_irqs(adapter);

	} else if (adapter->flags & RNP_FLAG_MSI_ENABLED) {
		/* in this case one for all */
		pr_info("msi mode is used\n");
		err = request_irq(adapter->pdev->irq, rnp_intr, 0,
				  adapter->netdev->name, adapter);
		adapter->hw.mbx.other_irq_enabled = true;
	} else {
		pr_info("legacy mode is used\n");
		err = request_irq(adapter->pdev->irq, rnp_intr, IRQF_SHARED,
				  adapter->netdev->name, adapter);
		adapter->hw.mbx.other_irq_enabled = true;
	}

	if (err)
		e_err(probe, "request_irq failed, Error %d\n", err);

	return err;
}

static void rnp_free_irq(struct rnp_adapter *adapter)
{
	if (adapter->flags & RNP_FLAG_MSIX_ENABLED) {
		rnp_free_msix_irqs(adapter);
	} else if (adapter->flags & RNP_FLAG_MSI_ENABLED) {
		/* in this case one for all */
		free_irq(adapter->pdev->irq, adapter);
		adapter->hw.mbx.other_irq_enabled = false;
	} else {
		free_irq(adapter->pdev->irq, adapter);
		adapter->hw.mbx.other_irq_enabled = false;
	}

}

/**
 * rnp_irq_disable - Mask off interrupt generation on the NIC
 * @adapter: board private structure
 **/
static inline void rnp_irq_disable(struct rnp_adapter *adapter)
{
	int i, j;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		rnp_irq_disable_queues(adapter->q_vector[i]);
		j = i + adapter->q_vector_off;
		if (adapter->flags & RNP_FLAG_MSIX_ENABLED)
			synchronize_irq(adapter->msix_entries[j].vector);
		else
			synchronize_irq(adapter->pdev->irq);
	}
}

int rnp_setup_tx_maxrate(struct rnp_ring *tx_ring, u64 max_rate,
			 int samples_1sec)
{
	/* set hardware samping internal 1S */
	ring_wr32(tx_ring, RNP_DMA_REG_TX_FLOW_CTRL_TM, samples_1sec);
	ring_wr32(tx_ring, RNP_DMA_REG_TX_FLOW_CTRL_TH, max_rate);

	return 0;
}

/**
 * rnp_tx_maxrate_own - callback to set the maximum per-queue bitrate
 * @netdev: network interface device structure
 * @queue_index: Tx queue to set
 * @maxrate: desired maximum transmit bitrate Mbps
 **/
static int rnp_tx_maxrate_own(struct rnp_adapter *adapter, int queue_index)
{
	struct rnp_ring *tx_ring = adapter->tx_ring[queue_index];
	u64 real_rate = 0;
	u32 maxrate = adapter->max_rate[queue_index];

	if (!maxrate)
		return rnp_setup_tx_maxrate(tx_ring, 0,
					    adapter->hw.usecstocount * 1000000);
	/* we need turn it to bytes/s */
	real_rate = ((u64)maxrate * 1024 * 1024) / 8;
	rnp_setup_tx_maxrate(tx_ring, real_rate,
			     adapter->hw.usecstocount * 1000000);

	return 0;
}

/**
 * rnp_configure_tx_ring - Configure 8259x Tx ring after Reset
 * @adapter: board private structure
 * @ring: structure containing ring specific data
 *
 * Configure the Tx descriptor ring after a reset.
 **/
void rnp_configure_tx_ring(struct rnp_adapter *adapter, struct rnp_ring *ring)
{
	struct rnp_hw *hw = &adapter->hw;

	/* disable queue to avoid issues while updating state */

	if (!(ring->ring_flags & RNP_RING_SKIP_TX_START))
		ring_wr32(ring, RNP_DMA_TX_START, 0);

	ring_wr32(ring, RNP_DMA_REG_TX_DESC_BUF_BASE_ADDR_LO, (u32)ring->dma);
	ring_wr32(ring, RNP_DMA_REG_TX_DESC_BUF_BASE_ADDR_HI,
		  (u32)(((u64)ring->dma) >> 32) | (hw->pfvfnum << 24));
	ring_wr32(ring, RNP_DMA_REG_TX_DESC_BUF_LEN, ring->count);

	ring->next_to_clean = ring_rd32(ring, RNP_DMA_REG_TX_DESC_BUF_HEAD);
	ring->next_to_use = ring->next_to_clean;
	ring->tail = ring->ring_addr + RNP_DMA_REG_TX_DESC_BUF_TAIL;
	rnp_wr_reg(ring->tail, ring->next_to_use);

	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED) {
		ring_wr32(ring, RNP_DMA_REG_TX_DESC_FETCH_CTRL,
			  (8 << 0) /* max_water_flow */
				  | (8 << 16)
			  /* max-num_descs_peer_read */
		);

	} else {
		ring_wr32(ring, RNP_DMA_REG_TX_DESC_FETCH_CTRL,
			  (64 << 0) /* max_water_flow */
				  | (TSRN10_TX_DEFAULT_BURST << 16)
			  /* max-num_descs_peer_read */
		);
	}
	ring_wr32(ring, RNP_DMA_REG_TX_INT_DELAY_TIMER,
		  adapter->tx_usecs * hw->usecstocount);
	ring_wr32(ring, RNP_DMA_REG_TX_INT_DELAY_PKTCNT, adapter->tx_frames);

	rnp_tx_maxrate_own(adapter, ring->queue_index);
	if (adapter->flags & RNP_FLAG_FDIR_HASH_CAPABLE) {
		ring->atr_sample_rate = adapter->atr_sample_rate;
		ring->atr_count = 0;
		set_bit(__RNP_TX_FDIR_INIT_DONE, &ring->state);
	} else {
		ring->atr_sample_rate = 0;
	}
	/* initialize XPS */
	if (!test_and_set_bit(__RNP_TX_XPS_INIT_DONE, &ring->state)) {
		struct rnp_q_vector *q_vector = ring->q_vector;

		if (q_vector)
			netif_set_xps_queue(adapter->netdev,
					    &q_vector->affinity_mask,
					    ring->queue_index);
	}

	clear_bit(__RNP_HANG_CHECK_ARMED, &ring->state);

	if (!(ring->ring_flags & RNP_RING_SKIP_TX_START)) {
		/* should wait tx_ready before open tx start */
		int timeout = 0;
		u32 status = 0;

		do {
			status = ring_rd32(ring, RNP_DMA_TX_READY);
			usleep_range(100, 200);
			timeout++;
			rnp_dbg("wait %d tx ready to 1\n", ring->rnp_queue_idx);
		} while ((status != 1) && (timeout < 100));

		if (timeout >= 100)
			printk("wait tx ready timeout\n");
		ring_wr32(ring, RNP_DMA_TX_START, 1);
	}
}

/**
 * rnp_configure_tx - Configure Transmit Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Tx unit of the MAC after a reset.
 **/
static void rnp_configure_tx(struct rnp_adapter *adapter)
{
	u32 i, dma_axi_ctl;
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_dma_info *dma = &hw->dma;

	/* dma_axi_en.tx_en must be before Tx queues are enabled */
	dma_axi_ctl = dma_rd32(dma, RNP_DMA_AXI_EN);
	dma_axi_ctl |= TX_AXI_RW_EN;
	dma_wr32(dma, RNP_DMA_AXI_EN, dma_axi_ctl);

	/* Setup the HW Tx Head and Tail descriptor pointers */
	for (i = 0; i < (adapter->num_tx_queues); i++)
		rnp_configure_tx_ring(adapter, adapter->tx_ring[i]);
}

void rnp_disable_rx_queue(struct rnp_adapter *adapter, struct rnp_ring *ring)
{
	ring_wr32(ring, RNP_DMA_RX_START, 0);
}

void rnp_configure_rx_ring(struct rnp_adapter *adapter, struct rnp_ring *ring)
{
	struct rnp_hw *hw = &adapter->hw;
	u64 desc_phy = ring->dma;
	u16 q_idx = ring->queue_index;

	/* disable queue to avoid issues while updating state */
	rnp_disable_rx_queue(adapter, ring);

	/* set descripts registers*/
	ring_wr32(ring, RNP_DMA_REG_RX_DESC_BUF_BASE_ADDR_LO, (u32)desc_phy);
	ring_wr32(ring, RNP_DMA_REG_RX_DESC_BUF_BASE_ADDR_HI,
		  ((u32)(desc_phy >> 32)) | (hw->pfvfnum << 24));
	ring_wr32(ring, RNP_DMA_REG_RX_DESC_BUF_LEN, ring->count);

	ring->tail = ring->ring_addr + RNP_DMA_REG_RX_DESC_BUF_TAIL;
	ring->next_to_clean = ring_rd32(ring, RNP_DMA_REG_RX_DESC_BUF_HEAD);
	ring->next_to_use = ring->next_to_clean;

	if (ring->ring_flags & RNP_RING_SCATER_SETUP)
		ring_wr32(ring, PCI_DMA_REG_RX_SCATTER_LENGTH, 96);

	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED) {
		ring_wr32(ring, RNP_DMA_REG_RX_DESC_FETCH_CTRL,
			  0 | (TSRN10_RX_DEFAULT_LINE << 0) /* rx-desc-flow */
				  | (TSRN10_RX_DEFAULT_BURST << 16)
			  /* max-read-desc-cnt */
		);

	} else {
		ring_wr32(ring, RNP_DMA_REG_RX_DESC_FETCH_CTRL,
			  0 | (TSRN10_RX_DEFAULT_LINE << 0) /* rx-desc-flow */
				  | (TSRN10_RX_DEFAULT_BURST << 16)
			  /* max-read-desc-cnt */
		);
	}
	/* setup rx drop */
	if (adapter->rx_drop_status & BIT(q_idx)) {
		ring_wr32(ring, PCI_DMA_REG_RX_DESC_TIMEOUT_TH,
			  adapter->drop_time);
	} else {
		ring_wr32(ring, PCI_DMA_REG_RX_DESC_TIMEOUT_TH, 0);
	}

	if (ring->ring_flags & RNP_RING_IRQ_MISS_FIX)
		ring_wr32(ring, RNP_DMA_INT_TRIG, TX_INT_MASK | RX_INT_MASK);

	ring_wr32(ring, RNP_DMA_REG_RX_INT_DELAY_TIMER,
		  adapter->rx_usecs * hw->usecstocount);
	ring_wr32(ring, RNP_DMA_REG_RX_INT_DELAY_PKTCNT, adapter->rx_frames);
	rnp_alloc_rx_buffers(ring, rnp_desc_unused_rx(ring));
}

static void rnp_configure_virtualization(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_dma_info *dma = &hw->dma;
	u32 ring, vfnum;
	u64 real_rate = 0;
	int i, vf_ring, j;

	if (!(adapter->flags & RNP_FLAG_SRIOV_ENABLED)) {
		hw->ops.set_sriov_status(hw, false);
		return;
	}

	/* Enable only the PF's pool for Tx/Rx */

	if (adapter->flags2 & RNP_FLAG2_BRIDGE_MODE_VEB) {
		dma_wr32(dma, RNP_DMA_CONFIG,
			 dma_rd32(dma, RNP_DMA_CONFIG) & (~DMA_VEB_BYPASS));
		adapter->flags2 |= RNP_FLAG2_BRIDGE_MODE_VEB;
	}
	ring = adapter->tx_ring[0]->rnp_queue_idx;
	hw->ops.set_sriov_status(hw, true);

	/* store vfnum */
	vfnum = hw->max_vfs - 1;
	hw->veb_ring = ring;
	hw->vfnum = vfnum;
	/* use last-vf's table entry. */
	adapter->vf_num_for_pf = 0x80 | vfnum;

	/* setup vf tx rate setup here */
	for (i = 0; i < adapter->num_vfs; i++) {
		real_rate = (adapter->vfinfo[i].tx_rate * 1024 * 128) /
			    hw->sriov_ring_limit;
		for (j = 0; i < hw->sriov_ring_limit; i++) {
			vf_ring = rnp_get_vf_ringnum(hw, i, j);
			rnp_setup_ring_maxrate(adapter, vf_ring, real_rate);
		}
	}
}

static void rnp_set_rx_buffer_len(struct rnp_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int max_frame = netdev->mtu + ETH_HLEN + ETH_FCS_LEN * 3;
	struct rnp_ring *rx_ring;
	int i;

	if (max_frame < (ETH_FRAME_LEN + ETH_FCS_LEN))
		max_frame = (ETH_FRAME_LEN + ETH_FCS_LEN);

	for (i = 0; i < adapter->num_rx_queues; i++) {
		rx_ring = adapter->rx_ring[i];
		clear_bit(__RNP_RX_3K_BUFFER, &rx_ring->state);
		clear_bit(__RNP_RX_BUILD_SKB_ENABLED, &rx_ring->state);
		set_bit(__RNP_RX_BUILD_SKB_ENABLED, &rx_ring->state);

#ifdef OPTM_WITH_LPAGE
		rx_ring->rx_page_buf_nums = RNP_PAGE_BUFFER_NUMS(rx_ring);
		rx_ring->rx_per_buf_mem =
			ALIGN((rnp_rx_offset(rx_ring) + rnp_rx_bufsz(rx_ring) +
			       SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
			       RNP_RX_HWTS_OFFSET),
			      1024);
#endif

	}
}

/**
 * rnp_configure_rx - Configure 8259x Receive Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Rx unit of the MAC after a reset.
 **/
static void rnp_configure_rx(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_dma_info *dma = &hw->dma;
	int i;
	u32 rxctrl = 0, dma_axi_ctl;

	/* disable receives while setting up the descriptors */
	/* set_rx_buffer_len must be called before ring initialization */
	rnp_set_rx_buffer_len(adapter);

	/*
	 * Setup the HW Rx Head and Tail Descriptor Pointers and
	 * the Base and Length of the Rx Descriptor Ring
	 */
	for (i = 0; i < adapter->num_rx_queues; i++)
		rnp_configure_rx_ring(adapter, adapter->rx_ring[i]);

	if (adapter->num_rx_queues > 0) {
		wr32(hw, RNP_ETH_DEFAULT_RX_RING,
		     adapter->rx_ring[0]->rnp_queue_idx);
	}

	/* enable all receives */
	rxctrl |= 0;

	dma_axi_ctl = dma_rd32(dma, RNP_DMA_AXI_EN);
	dma_axi_ctl |= RX_AXI_RW_EN;
	dma_wr32(dma, RNP_DMA_AXI_EN, dma_axi_ctl);
}

static int rnp_vlan_rx_add_vid(struct net_device *netdev,
			       __always_unused __be16 proto, u16 vid)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	bool veb_setup = true;
	bool sriov_flag = !!(adapter->flags & RNP_FLAG_SRIOV_ENABLED);

	if (sriov_flag) {
		if (hw->feature_flags & RNP_VEB_VLAN_MASK_EN) {
			if (hw->ops.set_veb_vlan_mask) {
				if (hw->ops.set_veb_vlan_mask(
					    hw, vid, hw->vfnum, true) != 0) {
					dev_err(&adapter->pdev->dev,
						"out of vlan entries in sriov mode \n");
					return -EACCES;
				}
			}
		} else {
			/* in sriov mode */
			if ((vid) && (adapter->vf_vlan) &&
			    (vid != adapter->vf_vlan)) {
				dev_err(&adapter->pdev->dev,
					"only 1 vlan in sriov mode \n");
				return -EACCES;
			}

			/* update this */
			if (vid) {
				adapter->vf_vlan = vid;
				if (hw->ops.set_vf_vlan_mode) {
					if (hw->feature_flags &
					    RNP_NET_FEATURE_VF_FIXED)
						hw->ops.set_vf_vlan_mode(
							hw, vid, 0, true);
					else
						hw->ops.set_vf_vlan_mode(
							hw, vid, hw->vfnum,
							true);
				}
			}
		}
	}

	if (vid) {
		if (proto == htons(ETH_P_8021Q)) {
			adapter->vlan_count++;
		}
	}

	if (vid < VLAN_N_VID) {
		if (proto != htons(ETH_P_8021Q)) {
			set_bit(vid, adapter->active_vlans_stags);
			veb_setup = false;
		} else {
			set_bit(vid, adapter->active_vlans);
		}
	}
	
	if (hw->ops.set_vlan_filter) {
		hw->ops.set_vlan_filter(hw, vid, true,
					(sriov_flag && veb_setup));
	}

	return 0;
}

static int rnp_vlan_rx_kill_vid(struct net_device *netdev,
				__always_unused __be16 proto, u16 vid)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_eth_info *eth = &hw->eth;
	int i;
	bool sriov_flag = !!(adapter->flags & RNP_FLAG_SRIOV_ENABLED);
	bool veb_setup = true;

	if (!vid)
		return 0;

	if (sriov_flag) {
		if (vid) {
			int true_remove = 1;
			/* clean this */
			adapter->vf_vlan = 0;
			for (i = 0; i < adapter->num_vfs; i++) {
				if (vid == adapter->vfinfo[i].vf_vlan) {
					true_remove = 0;
				}
				if (vid == adapter->vfinfo[i].pf_vlan) {
					true_remove = 0;
				}
				/* setup pf_vlan */
			}
			/* if no vf use this vid */
			if (true_remove) {
				/* if remove stags */
				if (proto != htons(ETH_P_8021Q)) {
					veb_setup = false;
					if (!test_bit(vid,
						      adapter->active_vlans))
						true_remove = 1;
				} else {
					/* if remove ctags */
					if (!test_bit(vid,
						    adapter->active_vlans_stags))
						true_remove = 1;
				}
				/* if no other tags use this vid */
				if (true_remove) {
					if ((adapter->flags2 &
					     RNP_FLAG2_VLAN_STAGS_ENABLED) &&
					    (vid != adapter->stags_vid))
						/* should also check stags */
						hw->ops.set_vlan_filter(
							hw, vid, false,
							veb_setup);
				}
			}
			/* always clean veb */
			hw->ops.set_vlan_filter(hw, vid, true, false);

			if (hw->ops.set_vf_vlan_mode) {
				if (hw->feature_flags &
				    RNP_NET_FEATURE_VF_FIXED)
					hw->ops.set_vf_vlan_mode(hw, vid, 0,
								 false);
				else
					hw->ops.set_vf_vlan_mode(
						hw, vid, hw->vfnum, false);
			}

			/* remove veb */
			if (hw->feature_flags & RNP_VEB_VLAN_MASK_EN) {
				if (hw->ops.set_veb_vlan_mask) {
					hw->ops.set_veb_vlan_mask(
						hw, vid, hw->vfnum, false);
				}
			}
		}
	} else {
		int true_remove = 0;
		if (proto != htons(ETH_P_8021Q)) {
			veb_setup = false;
			if (!test_bit(vid, adapter->active_vlans))
				true_remove = 1;

		} else {
			/* if remove ctags */
			if (!test_bit(vid, adapter->active_vlans_stags))
				true_remove = 1;
		}
		if (true_remove) {
			if ((adapter->flags2 & RNP_FLAG2_VLAN_STAGS_ENABLED) &&
			    (vid != adapter->stags_vid))
				/* should also check stags */
				hw->ops.set_vlan_filter(hw, vid, false, false);
		}
	}

	/* need set ncsi vfta again */
	if (hw->ncsi_en)
		eth->ops.ncsi_set_vfta(eth);

	if (vid) {
		if (proto == htons(ETH_P_8021Q)) {
			/* should check proto todo */
			adapter->vlan_count--;
		}
	}
	if (proto == htons(ETH_P_8021Q))
		clear_bit(vid, adapter->active_vlans);
		/* clear stags */
	if (proto != htons(ETH_P_8021Q))
		clear_bit(vid, adapter->active_vlans_stags);
	return 0;
}

/**
 * rnp_vlan_strip_disable - helper to disable hw vlan stripping
 * @adapter: driver data
 */
static void rnp_vlan_strip_disable(struct rnp_adapter *adapter)
{
	int i;
	struct rnp_ring *tx_ring;
	struct rnp_hw *hw = &adapter->hw;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		tx_ring = adapter->rx_ring[i];
		hw->ops.set_vlan_strip(hw, tx_ring->rnp_queue_idx, false);
	}
}

/**
 * rnp_vlan_strip_enable - helper to enable hw vlan stripping
 * @adapter: driver data
 */
static void rnp_vlan_strip_enable(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_ring *tx_ring;
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		tx_ring = adapter->rx_ring[i];

		hw->ops.set_vlan_strip(hw, tx_ring->rnp_queue_idx, true);
	}
}

static void rnp_remove_vlan(struct rnp_adapter *adapter)
{
	adapter->vlan_count = 0;
}

static void rnp_restore_vlan(struct rnp_adapter *adapter)
{
	u16 vid;
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_eth_info *eth = &hw->eth;
	int i;

	/* in stags open, set stags_vid to vlan filter */
	if (adapter->flags2 & RNP_FLAG2_VLAN_STAGS_ENABLED)
		eth->ops.set_vfta(eth, adapter->stags_vid, true);

	rnp_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q), 0);

	for_each_set_bit(vid, adapter->active_vlans, VLAN_N_VID)
		rnp_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q), vid);
	/* config vlan mode for mac */
	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED) {
		for (i = 0; i < adapter->num_vfs; i++) {
			vid = adapter->vfinfo[i].vf_vlan;
			if (vid) {
				rnp_vlan_rx_add_vid(adapter->netdev,
						    htons(ETH_P_8021Q), vid);
			}
			vid = adapter->vfinfo[i].pf_vlan;
			if (vid) {
				rnp_vlan_rx_add_vid(adapter->netdev,
						    htons(ETH_P_8021Q), vid);
			}
		}
	}
}

/**
 * rnp_set_rx_mode - Unicast, Multicast and Promiscuous mode set
 * @netdev: network interface device structure
 *
 * The set_rx_method entry point is called whenever the unicast/multicast
 * address list or the network interface flags are updated.  This routine is
 * responsible for configuring the hardware for proper unicast, multicast and
 * promiscuous mode.
 **/
void rnp_set_rx_mode(struct net_device *netdev)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	netdev_features_t features;
	bool sriov_flag = !!(adapter->flags & RNP_FLAG_SRIOV_ENABLED);

	hw->ops.set_rx_mode(hw, netdev, sriov_flag);

	if (sriov_flag) {
		if (!test_and_set_bit(__RNP_USE_VFINFI, &adapter->state)) {
			rnp_restore_vf_macvlans(adapter);
			rnp_restore_vf_macs(adapter);
			clear_bit(__RNP_USE_VFINFI, &adapter->state);
		}
	}

	features = netdev->features;

	if (features & NETIF_F_HW_VLAN_CTAG_RX)
		rnp_vlan_strip_enable(adapter);
	else
		rnp_vlan_strip_disable(adapter);
	/* stags */
	/* only do this if hw support stags */
	if (hw->feature_flags & RNP_NET_FEATURE_STAG_OFFLOAD) {
		if (features & NETIF_F_HW_VLAN_STAG_RX)
			rnp_vlan_strip_enable(adapter);
		else
			rnp_vlan_strip_disable(adapter);
	}
}

static void rnp_napi_enable_all(struct rnp_adapter *adapter)
{
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++)
		napi_enable(&adapter->q_vector[q_idx]->napi);
}

static void rnp_napi_disable_all(struct rnp_adapter *adapter)
{
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++)
		napi_disable(&adapter->q_vector[q_idx]->napi);
}

static void rnp_fdir_filter_restore(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;
	struct hlist_node *node2;
	struct rnp_fdir_filter *filter;

	spin_lock(&adapter->fdir_perfect_lock);

	/* enable tcam if set tcam mode */
	if (adapter->fdir_mode == fdir_mode_tcam) {
		wr32(hw, RNP_ETH_TCAM_EN, 1);
		wr32(hw, RNP_TOP_ETH_TCAM_CONFIG_ENABLE, 1);
		wr32(hw, RNP_TCAM_CACHE_ENABLE, 0);
	}

	/* setup ntuple */
	hlist_for_each_entry_safe(filter, node2, &adapter->fdir_filter_list,
				  fdir_node) {
		if ((!filter->vf_num) &&
			(filter->action != ACTION_TO_MPE)) {
				rnp_fdir_write_perfect_filter(
					adapter->fdir_mode, hw, &filter->filter, filter->hw_idx,
					(filter->action == RNP_FDIR_DROP_QUEUE) ?
					RNP_FDIR_DROP_QUEUE :
					adapter->rx_ring[filter->action]
					->rnp_queue_idx,
					(adapter->priv_flags & RNP_PRIV_FLAG_REMAP_PRIO) ?
					true :
					false);
		} else {
				rnp_fdir_write_perfect_filter(
					adapter->fdir_mode, hw, &filter->filter,
					filter->hw_idx,
					(filter->action == RNP_FDIR_DROP_QUEUE) ?
					RNP_FDIR_DROP_QUEUE :
					filter->action,
					(adapter->priv_flags &
					 RNP_PRIV_FLAG_REMAP_PRIO) ?
					true :
					false);
		}
	}

	spin_unlock(&adapter->fdir_perfect_lock);
}

static void rnp_configure_pause(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;

	hw->ops.set_pause_mode(hw);
}

static void rnp_vlan_stags_flag(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;

	/* stags is added */
	if (adapter->flags2 & RNP_FLAG2_VLAN_STAGS_ENABLED)
		hw->ops.set_txvlan_mode(hw, false);
	else
		hw->ops.set_txvlan_mode(hw, true);
}

static void rnp_configure(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;
	bool sriov_flag = !!(adapter->flags & RNP_FLAG_SRIOV_ENABLED);
#if (PAGE_SIZE < 8192)
	struct rnp_ring *rx_ring = adapter->rx_ring[0];
#endif
	/*
	 * We must restore virtualization before VLANs or else
	 * the VLVF registers will not be populated
	 */
	rnp_configure_virtualization(adapter);

	/* Unicast, Multicast and Promiscuous mode set */
	rnp_set_rx_mode(adapter->netdev);
	/* reconfigure hw */
	hw->ops.set_mac(hw, hw->mac.addr, sriov_flag);

	/* in sriov mode vlan is not reset */
	rnp_restore_vlan(adapter);

	/* we first update rx_offset */
#if (PAGE_SIZE < 8192)
	/* setup before calculate dma_split_size */
	rnp_set_rx_buffer_len(adapter);
	hw->dma_split_size = rnp_rx_pg_size(rx_ring) / 2 -
			     rnp_rx_offset(rx_ring) -
			     sizeof(struct skb_shared_info);
#else
	/* if mtu more than this */
	hw->dma_split_size = SKB_WITH_OVERHEAD(PAGE_SIZE) - RNP_SKB_PAD;

	if (hw->max_length_current >= 1536)
		hw->dma_split_size = min_t(int, hw->dma_split_size, hw->max_length_current);
	/* up to 16-asign */
	hw->dma_split_size = (hw->dma_split_size + 15) & (~0xf);
#endif
	hw->ops.update_hw_info(hw);

	/* init setup pause */
	rnp_configure_pause(adapter);
	rnp_vlan_stags_flag(adapter);
	rnp_init_rss_key(adapter);
	rnp_init_rss_table(adapter);

	if (adapter->flags & RNP_FLAG_FDIR_HASH_CAPABLE) {

	} else if (adapter->flags & RNP_FLAG_FDIR_PERFECT_CAPABLE) {

		rnp_fdir_filter_restore(adapter);

	}

	/* setup vxlan match mode */
	if (adapter->priv_flags & RNP_PRIV_FLAG_VXLAN_INNER_MATCH)
		hw->ops.set_vxlan_mode(hw, true);
	else
		hw->ops.set_vxlan_mode(hw, false);
	rnp_configure_tx(adapter);
	rnp_configure_rx(adapter);
}

static inline bool rnp_is_sfp(struct rnp_hw *hw)
{
	return true;
}

/**
 * rnp_sfp_link_config - set up SFP+ link
 * @adapter: pointer to private adapter struct
 **/
static void rnp_sfp_link_config(struct rnp_adapter *adapter)
{
	/*
	 * We are assuming the worst case scenario here, and that
	 * is that an SFP was inserted/removed after the reset
	 * but before SFP detection was enabled.  As such the best
	 * solution is to just start searching as soon as we start
	 */
	adapter->flags2 |= RNP_FLAG2_SFP_NEEDS_RESET;
}

static void rnp_up_complete(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;
	int i;

	rnp_configure_msix(adapter);

	/* enable the optics for n10 SFP+ fiber */
	if (hw->ops.enable_tx_laser)
		hw->ops.enable_tx_laser(hw);

	smp_mb__before_atomic();
	clear_bit(__RNP_DOWN, &adapter->state);
	rnp_napi_enable_all(adapter);

	if (rnp_is_sfp(hw)) {
		rnp_sfp_link_config(adapter);
	}
	/*clear any pending interrupts*/
	rnp_irq_enable(adapter);

	/* enable transmits */
	netif_tx_start_all_queues(adapter->netdev);

	/* enable rx transmit */
	for (i = 0; i < adapter->num_rx_queues; i++)
		ring_wr32(adapter->rx_ring[i], RNP_DMA_RX_START, 1);

	/* bring the link up in the watchdog, this could race with our first
	 * link up interrupt but shouldn't be a problems
	 */
	adapter->flags |= RNP_FLAG_NEED_LINK_UPDATE;
	adapter->link_check_timeout = jiffies;
	mod_timer(&adapter->service_timer, jiffies);

	/* Set PF Reset Done bit so PF/VF Mail Ops can work */
	hw->link = 0;
	rnp_mbx_force_speed(hw, hw->saved_force_link_speed);
	hw->ops.set_mbx_link_event(hw, 1);
	hw->ops.set_mbx_ifup(hw, 1);
}

void rnp_reinit_locked(struct rnp_adapter *adapter)
{
	WARN_ON(in_interrupt());
	/* put off any impending NetWatchDogTimeout */
	while (test_and_set_bit(__RNP_RESETTING, &adapter->state))
		usleep_range(1000, 2000);
	rnp_down(adapter);
	/*
	 * If SR-IOV enabled then wait a bit before bringing the adapter
	 * back up to give the VFs time to respond to the reset.  The
	 * two second wait is based upon the watchdog timer cycle in
	 * the VF driver.
	 */
	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED)
		msleep(2000);
	rnp_up(adapter);

	clear_bit(__RNP_RESETTING, &adapter->state);
}

void rnp_up(struct rnp_adapter *adapter)
{
	/* hardware has been reset, we need to reload some things */
	rnp_configure(adapter);
	rnp_up_complete(adapter);
}

void rnp_reset(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;
	int err;
	bool sriov_flag = !!(adapter->flags & RNP_FLAG_SRIOV_ENABLED);

	rnp_logd(LOG_ADPT_STAT, "%s\n", __func__);

	/* lock SFP init bit to prevent race conditions with the watchdog */
	while (test_and_set_bit(__RNP_IN_SFP_INIT, &adapter->state))
		usleep_range(1000, 2000);

	/* clear all SFP and link config related flags while holding SFP_INIT */
	adapter->flags2 &=
		~(RNP_FLAG2_SEARCH_FOR_SFP | RNP_FLAG2_SFP_NEEDS_RESET);
	adapter->flags &= ~RNP_FLAG_NEED_LINK_CONFIG;

	err = hw->ops.init_hw(hw);

	if (err) {
		e_dev_err("init_hw: Hardware Error: err:%d. line:%d\n", err,
			  __LINE__);
	}

	clear_bit(__RNP_IN_SFP_INIT, &adapter->state);

	/* reprogram the RAR[0] in case user changed it. */
	hw->ops.set_mac(hw, hw->mac.addr, sriov_flag);

	if (module_enable_ptp) {
		if (adapter->flags2 & RNP_FLAG2_PTP_ENABLED &&
		    (adapter->ptp_rx_en || adapter->ptp_tx_en))
			rnp_ptp_reset(adapter);
	}
}

#ifdef OPTM_WITH_LPAGE
/**
 * rnp_clean_rx_ring - Free Rx Buffers per Queue
 * @rx_ring: ring to free buffers from
 **/
static void rnp_clean_rx_ring(struct rnp_ring *rx_ring)
{
	u16 i = rx_ring->next_to_clean;
	struct rnp_rx_buffer *rx_buffer;

	if (!rx_ring->rx_buffer_info)
		return;

	if (rx_ring->skb)
		dev_kfree_skb(rx_ring->skb);
	rx_ring->skb = NULL;
	rx_buffer = &rx_ring->rx_buffer_info[i];

	/* Free all the Rx ring sk_buffs */
	while (i != rx_ring->next_to_alloc) {
		if (!rx_buffer->page)
			goto next_buffer;
		/* Invalidate cache lines that may have been written to by
		 * device so that we avoid corrupting memory.
		 */
		dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma,
					      rx_buffer->page_offset,
					      rnp_rx_bufsz(rx_ring),
					      DMA_FROM_DEVICE);

		/* free resources associated with mapping */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnp_rx_pg_size(rx_ring), DMA_FROM_DEVICE,
				     RNP_RX_DMA_ATTR);

		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
		/* now this page is not used */
		rx_buffer->page = NULL;
next_buffer:
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

#else
/**
 * rnp_clean_rx_ring - Free Rx Buffers per Queue
 * @rx_ring: ring to free buffers from
 **/
static void rnp_clean_rx_ring(struct rnp_ring *rx_ring)
{
	u16 i = rx_ring->next_to_clean;
	struct rnp_rx_buffer *rx_buffer = &rx_ring->rx_buffer_info[i];

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
					      rnp_rx_bufsz(rx_ring),
					      DMA_FROM_DEVICE);

		/* free resources associated with mapping */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnp_rx_pg_size(rx_ring), DMA_FROM_DEVICE,
				     RNP_RX_DMA_ATTR);

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
#endif

/**
 * rnp_clean_tx_ring - Free Tx Buffers
 * @tx_ring: ring to be cleaned
 **/
static void rnp_clean_tx_ring(struct rnp_ring *tx_ring)
{
	unsigned long size;
	u16 i = tx_ring->next_to_clean;
	struct rnp_tx_buffer *tx_buffer = &tx_ring->tx_buffer_info[i];

	BUG_ON(tx_ring == NULL);

	/* ring already cleared, nothing to do */
	if (!tx_ring->tx_buffer_info)
		return;

	while (i != tx_ring->next_to_use) {
		struct rnp_tx_desc *eop_desc, *tx_desc;

		dev_kfree_skb_any(tx_buffer->skb);
		/* unmap skb header data */
		dma_unmap_single(tx_ring->dev, dma_unmap_addr(tx_buffer, dma),
				 dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);

		eop_desc = tx_buffer->next_to_watch;
		tx_desc = RNP_TX_DESC(tx_ring, i);
		/* unmap remaining buffers */
		while (tx_desc != eop_desc) {
			tx_buffer++;
			tx_desc++;
			i++;
			if (unlikely(i == tx_ring->count)) {
				i = 0;
				tx_buffer = tx_ring->tx_buffer_info;
				tx_desc = RNP_TX_DESC(tx_ring, 0);
			}

			/* unmap any remaining paged data */
			if (dma_unmap_len(tx_buffer, len))
				dma_unmap_page(tx_ring->dev,
					       dma_unmap_addr(tx_buffer, dma),
					       dma_unmap_len(tx_buffer, len),
					       DMA_TO_DEVICE);
		}
		/* move us one more past the eop_desc for start of next pkt */
		tx_buffer++;
		i++;
		if (unlikely(i == tx_ring->count)) {
			i = 0;
			tx_buffer = tx_ring->tx_buffer_info;
		}
	}

	netdev_tx_reset_queue(txring_txq(tx_ring));
	size = sizeof(struct rnp_tx_buffer) * tx_ring->count;
	memset(tx_ring->tx_buffer_info, 0, size);

	/* Zero out the descriptor ring */
	memset(tx_ring->desc, 0, tx_ring->size);

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;
}

/**
 * rnp_clean_all_rx_rings - Free Rx Buffers for all queues
 * @adapter: board private structure
 **/
static void rnp_clean_all_rx_rings(struct rnp_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		rnp_clean_rx_ring(adapter->rx_ring[i]);
}

/**
 * rnp_clean_all_tx_rings - Free Tx Buffers for all queues
 * @adapter: board private structure
 **/
static void rnp_clean_all_tx_rings(struct rnp_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		rnp_clean_tx_ring(adapter->tx_ring[i]);
}

static void rnp_fdir_filter_exit(struct rnp_adapter *adapter)
{
	struct hlist_node *node2;
	struct rnp_fdir_filter *filter;
	struct rnp_hw *hw = &adapter->hw;

	spin_lock(&adapter->fdir_perfect_lock);

	hlist_for_each_entry_safe(filter, node2, &adapter->fdir_filter_list,
				  fdir_node) {
		/* call earase to hw */
		rnp_fdir_erase_perfect_filter(adapter->fdir_mode, hw,
					      &filter->filter, filter->hw_idx);

		hlist_del(&filter->fdir_node);
		kfree(filter);
	}
	adapter->fdir_filter_count = 0;
	adapter->layer2_count = hw->layer2_count;
	adapter->tuple_5_count = hw->tuple5_count;

	spin_unlock(&adapter->fdir_perfect_lock);
}

static int rnp_xmit_nop_frame_ring(struct rnp_adapter *adapter,
			    struct rnp_ring *tx_ring)
{
	u16 i = tx_ring->next_to_use;
	struct rnp_tx_desc *tx_desc;

	tx_desc = RNP_TX_DESC(tx_ring, i);

	/* set length to 0 */
	tx_desc->blen_mac_ip_len = 0;
	tx_desc->vlan_cmd = cpu_to_le32(RNP_TXD_CMD_EOP | RNP_TXD_CMD_RS);
	/*
	 * Force memory writes to complete before letting h/w know there
	 * are new descriptors to fetch.  (Only applicable for weak-ordered
	 * memory model archs, such as IA-64).
	 *
	 * We also need this memory barrier to make certain all of the
	 * status bits have been updated before next_to_watch is written.
	 */
	wmb();
	/* update tail */
	rnp_wr_reg(tx_ring->tail, 0);
	return 0;
}

static void print_status(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_eth_info *eth = &hw->eth;
	int i;
	struct rnp_dma_info *dma = &hw->dma;

	printk("eth 0x120 %x\n", eth_rd32(eth, 0x120));
	printk("eth 0x124 %x\n", eth_rd32(eth, 0x124));

	for (i = 0x300; i < 0x318; i = i + 4) {
		printk("eth 0x%x %x\n", i, eth_rd32(eth, i));
	}

	printk("eth 0x%x %x\n", 0x98, eth_rd32(eth, 0x98));
	printk("eth 0x%x %x\n", 0x220, eth_rd32(eth, 0x220));

	for (i = 0x138; i < 0x158; i = i + 4) {
		printk("dma 0x%x %x\n", i, dma_rd32(dma, i));
	}
	i = 0x170;
	printk("dma 0x%x %x\n", i, dma_rd32(dma, i));
	i = 0x174;
	printk("dma 0x%x %x\n", i, dma_rd32(dma, i));
	for (i = 0x214; i < 0x220; i = i + 4) {
		printk("dma 0x%x %x\n", i, dma_rd32(dma, i));
	}
	for (i = 0x234; i < 0x270; i = i + 4) {
		printk("dma 0x%x %x\n", i, dma_rd32(dma, i));
	}
}

void rnp_down(struct rnp_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct rnp_hw *hw = &adapter->hw;
	int i;
	int free_tx_ealay = 0;
	int err = 0;
	/* signal that we are down to the interrupt handler */
	set_bit(__RNP_DOWN, &adapter->state);
	if ((!hw->ncsi_en) && (!(adapter->flags & RNP_FLAG_SRIOV_ENABLED)))
		hw->ops.set_mac_rx(hw, false);

	if (hw->ncsi_en) {
		/* if we false down, we should set mac loopback */
		hw->ops.set_mac_rx(hw, false);
	}

	hw->ops.set_mbx_link_event(hw, 0);
	hw->ops.set_mbx_ifup(hw, 0);

	if (hw->ops.clean_link)
		hw->ops.clean_link(hw);

	/* if carrier on before */
	if (netif_carrier_ok(netdev))
		e_info(drv, "NIC Link is Down\n");

	rnp_remove_vlan(adapter);
	netif_tx_stop_all_queues(netdev);
	netif_carrier_off(netdev);
	usleep_range(5000, 10000);
	/* if we have tx desc to clean */
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct rnp_ring *tx_ring = adapter->tx_ring[i];

		if (!(tx_ring->ring_flags & RNP_RING_SKIP_TX_START)) {
			int head, tail;
			int timeout = 0;

			free_tx_ealay = 1;

			head = ring_rd32(tx_ring, RNP_DMA_REG_TX_DESC_BUF_HEAD);
			tail = ring_rd32(tx_ring, RNP_DMA_REG_TX_DESC_BUF_TAIL);

			while (head != tail) {
				usleep_range(30000, 50000);
				head = ring_rd32(tx_ring,
						 RNP_DMA_REG_TX_DESC_BUF_HEAD);
				tail = ring_rd32(tx_ring,
						 RNP_DMA_REG_TX_DESC_BUF_TAIL);
				timeout++;
				if ((timeout >= 100) && (timeout < 101)) {
					e_info(drv,
					       "wait tx done timeout %x %x\n",
					       head, tail);
					/* set this to hold hardware status */
					adapter->priv_flags |=
						RNP_PRIV_FLGA_TEST_TX_HANG;
					print_status(adapter);
					err = 1;
				}
				if (timeout >= 200) {
					e_info(drv,
					       "200 wait tx done timeout %x %x\n",
					       head, tail);
					print_status(adapter);
					break;
				}
			}
		}
	}

	{
		int time = 0;

		while (test_bit(__RNP_SERVICE_CHECK, &adapter->state)) {
			usleep_range(100, 200);
			time++;
			if (time > 100)
				break;
		}
	}

	if (free_tx_ealay)
		rnp_clean_all_tx_rings(adapter);

	usleep_range(2000, 5000);

	rnp_irq_disable(adapter);

	usleep_range(5000, 10000);

	netif_tx_disable(netdev);

	/* disable all enabled rx queues */
	for (i = 0; i < adapter->num_rx_queues; i++) {
		rnp_disable_rx_queue(adapter, adapter->rx_ring[i]);
		/* only handle when srio enable and change rx length setup */
		if ((((adapter->flags & RNP_FLAG_SRIOV_ENABLED) ||
		      hw->ncsi_en)) &&
		    (adapter->rx_ring[i]->ring_flags &
		     RNP_RING_FLAG_CHANGE_RX_LEN)) {
			int head;
			struct rnp_ring *ring = adapter->rx_ring[i];

			head = ring_rd32(ring, RNP_DMA_REG_RX_DESC_BUF_HEAD);
			adapter->rx_ring[i]->ring_flags &=
				(~RNP_RING_FLAG_CHANGE_RX_LEN);
			/* we should delay setup rx length to
			 * wait rx head to 0
			 */
			if (head >= adapter->rx_ring[i]->reset_count) {
				adapter->rx_ring[i]->ring_flags |=
					RNP_RING_FLAG_DELAY_SETUP_RX_LEN;
				/* set sw count to head + 1*/
				adapter->rx_ring[i]->temp_count = head + 1;
			}
		}
		/* only down without rx_len change no need handle */
	}
	/* call carrier off first to avoid false dev_watchdog timeouts */

	rnp_napi_disable_all(adapter);

	adapter->flags2 &=
		~(RNP_FLAG2_FDIR_REQUIRES_REINIT | RNP_FLAG2_RESET_REQUESTED);
	adapter->flags &= ~RNP_FLAG_NEED_LINK_UPDATE;

	if (adapter->num_vfs) {
		/* ping all the active vfs to let them know we are going down */
		rnp_ping_all_vfs(adapter);
		/* Disable all VFTE/VFRE TX/RX */
		rnp_disable_tx_rx(adapter);
	}
	{

		u32 status = 0;
		int timeout = 0;

		do {
			status = rd32(hw, RNP_DMA_AXI_READY);
			usleep_range(100, 200);
			timeout++;
		} while ((status != 0xffff) && (timeout < 100));

		if (timeout > 100)
			printk("wait axi ready timeout\n");
	}


	/* disable transmits in the hardware now that interrupts are off */
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct rnp_ring *tx_ring = adapter->tx_ring[i];
		int count = tx_ring->count;
		int head;
		int timeout = 0;

		/* 1. stop queue */
		if (!err) {
			if (!(tx_ring->ring_flags & RNP_RING_SKIP_TX_START)) {
				ring_wr32(tx_ring, RNP_DMA_TX_START, 0);
			}
		}
		/* 2. try to set tx head to 0 in sriov mode
		 * since we don't reset
		 */
		if ((((adapter->flags & RNP_FLAG_SRIOV_ENABLED) ||
		      hw->ncsi_en)) &&
		    (!(tx_ring->ring_flags & RNP_RING_SIZE_CHANGE_FIX))) {
			/* only do this if hw not support tx head to zero auto */
			/* n10 should wait tx_ready */
			u32 status = 0;

			timeout = 0;
			do {
				status = ring_rd32(tx_ring, RNP_DMA_TX_READY);
				usleep_range(100, 200);
				timeout++;
				rnp_dbg("wait %d tx ready to 1\n",
					tx_ring->rnp_queue_idx);
			} while ((status != 1) && (timeout < 100));

			if (timeout >= 100)
				printk("wait tx ready timeout\n");

			head = ring_rd32(tx_ring, RNP_DMA_REG_TX_DESC_BUF_HEAD);
			if (head != 0) {
				u16 next_to_use = tx_ring->next_to_use;

				if (head != (count - 1)) {
					/* 3 set len head + 1 */
					ring_wr32(tx_ring,
						  RNP_DMA_REG_TX_DESC_BUF_LEN,
						  head + 1);
				}
				/* set to use head */
				tx_ring->next_to_use = head;
				/* 4 send a len zero packet */
				rnp_xmit_nop_frame_ring(adapter, tx_ring);
				if (!(tx_ring->ring_flags &
				      RNP_RING_SKIP_TX_START))
					ring_wr32(tx_ring, RNP_DMA_TX_START, 1);
				/* 5 wait head to zero */
				while ((head != 0) && (timeout < 1000)) {
					head = ring_rd32(
						tx_ring,
						RNP_DMA_REG_TX_DESC_BUF_HEAD);
					usleep_range(10000, 20000);
					timeout++;
				}
				if (timeout >= 1000) {
					printk("[%s] Wait Tx-ring %d head to zero time out\n",
					       netdev->name,
					       tx_ring->rnp_queue_idx);
				}
				/* 6 stop queue again*/
				if (!(tx_ring->ring_flags &
				      RNP_RING_SKIP_TX_START))
					ring_wr32(tx_ring, RNP_DMA_TX_START, 0);
				/* 7 write back next_to_use maybe hw hang */
				tx_ring->next_to_use = next_to_use;
			}
		}
	}
	if (!err) {
		if (!pci_channel_offline(adapter->pdev)) {
			if (hw->ncsi_en == 0 &&
			    !(adapter->flags & RNP_FLAG_SRIOV_ENABLED)) {
				rnp_reset(adapter);
			}
		}
	}
	/* power down the optics for n10 SFP+ fiber */
	if (hw->ops.disable_tx_laser)
		hw->ops.disable_tx_laser(hw);

	if (!free_tx_ealay)
		rnp_clean_all_tx_rings(adapter);

	rnp_clean_all_rx_rings(adapter);

	if (hw->ncsi_en)
		hw->ops.set_mac_rx(hw, true);
}

/**
 * rnp_tx_timeout - Respond to a Tx Hang
 * @netdev: network interface device structure
 **/
static void rnp_tx_timeout(struct net_device *netdev, unsigned int txqueue)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	/* Do the reset outside of interrupt context */
	int i;
	bool real_tx_hang = false;

#define TX_TIMEO_LIMIT 16000
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct rnp_ring *tx_ring = adapter->tx_ring[i];

		if (check_for_tx_hang(tx_ring) && rnp_check_tx_hang(tx_ring))
			real_tx_hang = true;
	}

	if (real_tx_hang) {
		printk("hw real hang!!!!");
		/* Do the reset outside of interrupt context */
		rnp_tx_timeout_reset(adapter);
	} else {
		printk("Fake Tx hang detected with timeout of %d "
		       "seconds\n",
		       netdev->watchdog_timeo / HZ);

		/* fake Tx hang - increase the kernel timeout */
		if (netdev->watchdog_timeo < TX_TIMEO_LIMIT)
			netdev->watchdog_timeo *= 2;
	}
}

/**
 * rnp_sw_init - Initialize general software structures (struct rnp_adapter)
 * @adapter: board private structure to initialize
 *
 * rnp_sw_init initializes the Adapter private data structure.
 * Fields are initialized based on PCI device information and
 * OS network device settings (MTU size).
 **/
static int rnp_sw_init(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;
	struct pci_dev *pdev = adapter->pdev;
	unsigned int rss = 0, fdir;
	int rss_limit = num_online_cpus();

	hw->vendor_id = pdev->vendor;
	hw->device_id = pdev->device;
	hw->subsystem_vendor_id = pdev->subsystem_vendor;
	hw->subsystem_device_id = pdev->subsystem_device;

	/* if this hw can setup msix count */
	rss = min_t(int, adapter->max_ring_pair_counts, rss_limit);
	rss = min_t(int, rss,
		    hw->mac.max_msix_vectors - adapter->num_other_vectors);
	adapter->ring_feature[RING_F_RSS].limit =
		min_t(int, rss, adapter->max_ring_pair_counts);

	adapter->flags |= RNP_FLAG_VXLAN_OFFLOAD_CAPABLE;
	adapter->flags |= RNP_FLAG_VXLAN_OFFLOAD_ENABLE;

	adapter->max_q_vectors = hw->max_msix_vectors - 1;
	adapter->atr_sample_rate = 20;

	fdir = min_t(int, adapter->max_q_vectors, rss_limit);
	adapter->ring_feature[RING_F_FDIR].limit = fdir;

	if (hw->feature_flags & RNP_NET_FEATURE_RX_NTUPLE_FILTER) {
		spin_lock_init(&adapter->fdir_perfect_lock);
		adapter->fdir_filter_count = 0;
		adapter->fdir_mode = hw->fdir_mode;
		/* fdir_pballoc not from zero, so add 2 */
		adapter->fdir_pballoc = 2 + hw->layer2_count + hw->tuple5_count;
		adapter->layer2_count = hw->layer2_count;
		adapter->tuple_5_count = hw->tuple5_count;
	}

	/* itr sw setup here */
	adapter->sample_interval = 10;
	adapter->adaptive_rx_coal = 1;
	adapter->adaptive_tx_coal = 1;
	adapter->auto_rx_coal = 0;
	adapter->napi_budge = 64;
	/* set default work limits */
	adapter->tx_work_limit = RNP_DEFAULT_TX_WORK;
	adapter->rx_usecs = RNP_PKT_TIMEOUT;
	adapter->rx_usecs_usr_set = RNP_PKT_TIMEOUT;
	adapter->rx_frames = RNP_RX_PKT_POLL_BUDGET;
	adapter->tx_usecs = RNP_PKT_TIMEOUT_TX;
	adapter->tx_usecs_usr_set = RNP_PKT_TIMEOUT_TX;
	adapter->tx_frames = RNP_TX_PKT_POLL_BUDGET;

	/* set default ring sizes */
	adapter->tx_ring_item_count = RNP_DEFAULT_TXD;
	adapter->rx_ring_item_count = RNP_DEFAULT_RXD;

	set_bit(__RNP_DOWN, &adapter->state);

	return 0;
}

/**
 * rnp_setup_tx_resources - allocate Tx resources (Descriptors)
 * @tx_ring:    tx descriptor ring (for a specific queue) to setup
 *
 * Return 0 on success, negative on failure
 **/
int rnp_setup_tx_resources(struct rnp_ring *tx_ring,
			   struct rnp_adapter *adapter)
{
	struct device *dev = tx_ring->dev;
	int orig_node = dev_to_node(dev);
	int numa_node = NUMA_NO_NODE;
	int size;

	size = sizeof(struct rnp_tx_buffer) * tx_ring->count;

	if (tx_ring->q_vector)
		numa_node = tx_ring->q_vector->numa_node;
	tx_ring->tx_buffer_info = vzalloc_node(size, numa_node);
	if (!tx_ring->tx_buffer_info)
		tx_ring->tx_buffer_info = vzalloc(size);
	if (!tx_ring->tx_buffer_info)
		goto err;
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
		"TxRing:%d, vector:%d ItemCounts:%d "
		"desc:%p(0x%llx) node:%d\n",
		tx_ring->rnp_queue_idx, tx_ring->q_vector->v_idx,
		tx_ring->count, tx_ring->desc, (u64)tx_ring->dma, numa_node);
	return 0;

err:
	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;
	dev_err(dev, "Unable to allocate memory for the Tx descriptor ring\n");
	return -ENOMEM;
}

/**
 * rnp_setup_all_tx_resources - allocate all queues Tx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int rnp_setup_all_tx_resources(struct rnp_adapter *adapter)
{
	int i, err = 0;

	tx_dbg("adapter->num_tx_queues:%d, adapter->tx_ring[0]:%p\n",
	       adapter->num_tx_queues, adapter->tx_ring[0]);

	for (i = 0; i < (adapter->num_tx_queues); i++) {
		BUG_ON(adapter->tx_ring[i] == NULL);
		err = rnp_setup_tx_resources(adapter->tx_ring[i], adapter);
		if (!err)
			continue;

		e_err(probe, "Allocation for Tx Queue %u failed\n", i);
		goto err_setup_tx;
	}

	return 0;
err_setup_tx:
	/* rewind the index freeing the rings as we go */
	while (i--)
		rnp_free_tx_resources(adapter->tx_ring[i]);
	return err;
}

/**
 * rnp_setup_rx_resources - allocate Rx resources (Descriptors)
 * @rx_ring:    rx descriptor ring (for a specific queue) to setup
 *
 * Returns 0 on success, negative on failure
 **/
int rnp_setup_rx_resources(struct rnp_ring *rx_ring,
			   struct rnp_adapter *adapter)
{
	struct device *dev = rx_ring->dev;
	int orig_node = dev_to_node(dev);
	int numa_node = NUMA_NO_NODE;
	int size;

	BUG_ON(rx_ring == NULL);

	size = sizeof(struct rnp_rx_buffer) * rx_ring->count;

	if (rx_ring->q_vector)
		numa_node = rx_ring->q_vector->numa_node;

	rx_ring->rx_buffer_info = vzalloc_node(size, numa_node);
	if (!rx_ring->rx_buffer_info)
		rx_ring->rx_buffer_info = vzalloc(size);
	if (!rx_ring->rx_buffer_info)
		goto err;
	/* Round up to nearest 4K */
	rx_ring->size = rx_ring->count * sizeof(union rnp_rx_desc);
	rx_ring->size = ALIGN(rx_ring->size, 4096);

	set_dev_node(dev, numa_node);
	rx_ring->desc = dma_alloc_coherent(dev, rx_ring->size, &rx_ring->dma,
					   GFP_KERNEL);
	set_dev_node(dev, orig_node);
	if (!rx_ring->desc)
		rx_ring->desc = dma_alloc_coherent(dev, rx_ring->size,
						   &rx_ring->dma, GFP_KERNEL);
	if (!rx_ring->desc)
		goto err;
	memset(rx_ring->desc, 0, rx_ring->size);

	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;

	DPRINTK(IFUP, INFO,
		"RxRing:%d, vector:%d ItemCounts:%d "
		"desc:%p(0x%llx) node:%d\n",
		rx_ring->rnp_queue_idx, rx_ring->q_vector->v_idx,
		rx_ring->count, rx_ring->desc, (u64)rx_ring->dma, numa_node);

	return 0;
err:

	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;
	dev_err(dev, "Unable to allocate memory for the Rx descriptor ring\n");
	return -ENOMEM;
}

/**
 * rnp_setup_all_rx_resources - allocate all queues Rx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int rnp_setup_all_rx_resources(struct rnp_adapter *adapter)
{
	int i, err = 0;
	u32 head;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		BUG_ON(adapter->rx_ring[i] == NULL);

		/* should check count and head */
		/* in sriov condition may head large than count */
		head = ring_rd32(adapter->rx_ring[i],
				 RNP_DMA_REG_RX_DESC_BUF_HEAD);
		if (unlikely(head >= adapter->rx_ring[i]->count)) {
			dbg("[%s] Ring %d head large than count",
			    adapter->netdev->name,
			    adapter->rx_ring[i]->rnp_queue_idx);
			adapter->rx_ring[i]->ring_flags |=
				RNP_RING_FLAG_DELAY_SETUP_RX_LEN;
			adapter->rx_ring[i]->reset_count =
				adapter->rx_ring[i]->count;
			adapter->rx_ring[i]->count = head + 1;
		}
		err = rnp_setup_rx_resources(adapter->rx_ring[i], adapter);
		if (!err)
			continue;

		e_err(probe, "Allocation for Rx Queue %u failed\n", i);
		goto err_setup_rx;
	}

	return 0;
err_setup_rx:
	/* rewind the index freeing the rings as we go */
	while (i--)
		rnp_free_rx_resources(adapter->rx_ring[i]);
	return err;
}

/**
 * rnp_free_tx_resources - Free Tx Resources per Queue
 * @tx_ring: Tx descriptor ring for a specific queue
 *
 * Free all transmit software resources
 **/
void rnp_free_tx_resources(struct rnp_ring *tx_ring)
{
	BUG_ON(tx_ring == NULL);

	rnp_clean_tx_ring(tx_ring);
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
 * rnp_free_all_tx_resources - Free Tx Resources for All Queues
 * @adapter: board private structure
 *
 * Free all transmit software resources
 **/
static void rnp_free_all_tx_resources(struct rnp_adapter *adapter)
{
	int i;

	for (i = 0; i < (adapter->num_tx_queues); i++)
		rnp_free_tx_resources(adapter->tx_ring[i]);
}

/**
 * rnp_free_rx_resources - Free Rx Resources
 * @rx_ring: ring to clean the resources from
 *
 * Free all receive software resources
 **/
void rnp_free_rx_resources(struct rnp_ring *rx_ring)
{
	BUG_ON(rx_ring == NULL);

	rnp_clean_rx_ring(rx_ring);

	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;

	/* if not set, then don't free */
	if (!rx_ring->desc)
		return;

	dma_free_coherent(rx_ring->dev, rx_ring->size, rx_ring->desc,
			  rx_ring->dma);

	rx_ring->desc = NULL;
}

/**
 * rnp_free_all_rx_resources - Free Rx Resources for All Queues
 * @adapter: board private structure
 *
 * Free all receive software resources
 **/
static void rnp_free_all_rx_resources(struct rnp_adapter *adapter)
{
	int i;

	for (i = 0; i < (adapter->num_rx_queues); i++)
		if (adapter->rx_ring[i]->desc)
			rnp_free_rx_resources(adapter->rx_ring[i]);
}

/**
 * rnp_change_mtu - Change the Maximum Transfer Unit
 * @netdev: network interface device structure
 * @new_mtu: new value for maximum frame size
 *
 * Returns 0 on success, negative on failure
 **/
static int rnp_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int max_frame = new_mtu + ETH_HLEN + ETH_FCS_LEN * 2;

	/* MTU < 68 is an error and causes problems on some kernels */
	if ((new_mtu < hw->min_length) || (max_frame > hw->max_length))
		return -EINVAL;

	e_info(probe, "changing MTU from %d to %d\n", netdev->mtu, new_mtu);

	if (netdev->mtu == new_mtu)
		return 0;

	/* must set new MTU before calling down or up */
	netdev->mtu = new_mtu;

	if (netif_running(netdev))
		rnp_reinit_locked(adapter);

	rnp_msg_post_status(adapter, PF_SET_MTU);

	return 0;
}

/**
 * rnp_tx_maxrate - callback to set the maximum per-queue bitrate
 * @netdev: network interface device structure
 * @queue_index: Tx queue to set
 * @maxrate: desired maximum transmit bitrate Mbps
 **/
__maybe_unused static int rnp_tx_maxrate(struct net_device *netdev,
					 int queue_index, u32 maxrate)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_ring *tx_ring = adapter->tx_ring[queue_index];
	u64 real_rate = 0;

	adapter->max_rate[queue_index] = maxrate;
	rnp_dbg("%s: queue:%d maxrate:%d\n", __func__, queue_index, maxrate);
	if (!maxrate)
		return rnp_setup_tx_maxrate(tx_ring, 0,
					    adapter->hw.usecstocount * 1000000);
	/* we need turn it to bytes/s */
	real_rate = ((u64)maxrate * 1024 * 1024) / 8;
	rnp_setup_tx_maxrate(tx_ring, real_rate,
			     adapter->hw.usecstocount * 1000000);

	return 0;
}

/**
 * rnp_open - Called when a network interface is made active
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
int rnp_open(struct net_device *netdev)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	int err;

	DPRINTK(IFUP, INFO, "ifup\n");

	/* disallow open during test */
	if (test_bit(__RNP_TESTING, &adapter->state))
		return -EBUSY;

	netif_carrier_off(netdev);

	/* allocate transmit descriptors */
	err = rnp_setup_all_tx_resources(adapter);
	if (err)
		goto err_setup_tx;

	/* allocate receive descriptors */
	err = rnp_setup_all_rx_resources(adapter);
	if (err)
		goto err_setup_rx;

	rnp_configure(adapter);

	err = rnp_request_irq(adapter);
	if (err)
		goto err_req_irq;

	/* Notify the stack of the actual queue counts. */
	err = netif_set_real_num_tx_queues(netdev, adapter->num_tx_queues);
	if (err)
		goto err_set_queues;

	err = netif_set_real_num_rx_queues(netdev, adapter->num_rx_queues);
	if (err)
		goto err_set_queues;

	if (module_enable_ptp)
		rnp_ptp_register(adapter);

	rnp_up_complete(adapter);

	return 0;

err_set_queues:
	rnp_free_irq(adapter);
err_req_irq:
	rnp_free_all_rx_resources(adapter);
err_setup_rx:
	rnp_free_all_tx_resources(adapter);
err_setup_tx:
	hw->ops.set_mbx_ifup(hw, 0);
	rnp_reset(adapter);

	return err;
}

/**
 * rnp_close - Disables a network interface
 * @netdev: network interface device structure
 *
 * Returns 0, this is not allowed to fail
 *
 * The close entry point is called when an interface is de-activated
 * by the OS.  The hardware is still under the drivers control, but
 * needs to be disabled.  A global MAC reset is issued to stop the
 * hardware, and all transmit and receive resources are freed.
 **/
int rnp_close(struct net_device *netdev)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	DPRINTK(IFDOWN, INFO, "ifdown\n");

	if (module_enable_ptp)
		rnp_ptp_unregister(adapter);

	rnp_down(adapter);
	rnp_free_irq(adapter);
	rnp_free_all_tx_resources(adapter);
	rnp_free_all_rx_resources(adapter);

	/* if in sriov mode send link down to all vfs */
	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED) {
		adapter->link_up = 0;
		adapter->link_up_old = 0;
		rnp_msg_post_status(adapter, PF_SET_LINK_STATUS);
		/* wait all vf get this status */
		usleep_range(5000, 10000);
	}

	return 0;
}

#ifdef CONFIG_PM
static int rnp_resume(struct pci_dev *pdev)
{
	struct rnp_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
	u32 err;
	struct rnp_hw *hw = &adapter->hw;

	printk("call rnp_resume\n");
	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
	/*
	 * pci_restore_state clears dev->state_saved so call
	 * pci_save_state to restore it.
	 */
	pci_save_state(pdev);

	err = pcim_enable_device(pdev);
	if (err) {
		e_dev_err("Cannot enable PCI device from suspend\n");
		return err;
	}
	pci_set_master(pdev);

	pci_wake_from_d3(pdev, false);

	switch (hw->hw_type) {
	case rnp_hw_n10:
	case rnp_hw_n400:
	case rnp_hw_n20:
	case rnp_hw_uv440:
		wait_mbx_init_done(hw);
#ifdef FIX_VF_BUG
		rnp_wr_reg(adapter->io_addr_bar0 +
				   (0x7982fc & (pci_resource_len(pdev, 0) - 1)),
				   0);
#endif
		break;
	default:

		break;
	}

	rtnl_lock();

	err = rnp_init_interrupt_scheme(adapter);
	if (!err)
		err = register_mbx_irq(adapter);

	if (hw->ops.driver_status)
		hw->ops.driver_status(hw, false, rnp_driver_suspuse);

	rnp_reset(adapter);

	if (!err && netif_running(netdev))
		err = rnp_open(netdev);

	rtnl_unlock();

	if (err)
		return err;

	netif_device_attach(netdev);

	return 0;
}
#endif

static int __rnp_shutdown(struct pci_dev *pdev, bool *enable_wake)
{
	struct rnp_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
	struct rnp_hw *hw = &adapter->hw;
	u32 wufc = adapter->wol;
#ifdef CONFIG_PM
	int retval = 0;
#endif

	netif_device_detach(netdev);

	rtnl_lock();
	if (netif_running(netdev)) {
		rnp_down(adapter);
		rnp_free_irq(adapter);
		rnp_free_all_tx_resources(adapter);
		rnp_free_all_rx_resources(adapter);
		/* should consider sriov mode ? */
	}
	rtnl_unlock();

	if (hw->ops.driver_status)
		hw->ops.driver_status(hw, true, rnp_driver_suspuse);

	remove_mbx_irq(adapter);
	rnp_clear_interrupt_scheme(adapter);

#ifdef CONFIG_PM
	retval = pci_save_state(pdev);
	if (retval)
		return retval;

#endif
	if (wufc) {
		rnp_set_rx_mode(netdev);

		/* enable the optics for n10 SFP+ fiber as we can WoL */
		if (hw->ops.enable_tx_laser)
			hw->ops.enable_tx_laser(hw);

		/* turn on all-multi mode if wake on multicast is enabled */
	}

	if (hw->ops.setup_wol)
		hw->ops.setup_wol(hw, adapter->wol);

	pci_wake_from_d3(pdev, !!wufc);
	*enable_wake = !!wufc;

	pci_disable_device(pdev);

	return 0;
}

#ifdef CONFIG_PM
static int rnp_suspend(struct pci_dev *pdev, pm_message_t state)
{
	int retval;
	bool wake;

	printk("call rnp_suspend\n");

	retval = __rnp_shutdown(pdev, &wake);
	if (retval)
		return retval;

	if (wake) {
		pci_prepare_to_sleep(pdev);
	} else {
		pci_wake_from_d3(pdev, false);
		pci_set_power_state(pdev, PCI_D3hot);
	}

	return 0;
}
#endif /* CONFIG_PM */

__maybe_unused static void rnp_shutdown(struct pci_dev *pdev)
{
	bool wake;

	__rnp_shutdown(pdev, &wake);

	if (system_state == SYSTEM_POWER_OFF) {
		pci_wake_from_d3(pdev, wake);
		pci_set_power_state(pdev, PCI_D3hot);
	}
}

/**
 * rnp_update_stats - Update the board statistics counters.
 * @adapter: board private structure
 **/
void rnp_update_stats(struct rnp_adapter *adapter)
{
	struct net_device_stats *net_stats = &adapter->netdev->stats;
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_hw_stats *hw_stats = &adapter->hw_stats;
	int i;
	struct rnp_ring *ring;
	u64 hw_csum_rx_error = 0;
	u64 hw_csum_rx_good = 0;

	net_stats->tx_packets = 0;
	net_stats->tx_bytes = 0;
	net_stats->rx_packets = 0;
	net_stats->rx_bytes = 0;
	net_stats->rx_dropped = 0;
	net_stats->rx_errors = 0;
	hw_stats->vlan_strip_cnt = 0;
	hw_stats->vlan_add_cnt = 0;

	if (test_bit(__RNP_DOWN, &adapter->state) ||
	    test_bit(__RNP_RESETTING, &adapter->state))
		return;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		rnp_for_each_ring(ring, adapter->q_vector[i]->rx) {
			hw_csum_rx_error += ring->rx_stats.csum_err;
			hw_csum_rx_good += ring->rx_stats.csum_good;
			hw_stats->vlan_strip_cnt += ring->rx_stats.vlan_remove;
			net_stats->rx_packets += ring->stats.packets;
			net_stats->rx_bytes += ring->stats.bytes;
		}
		rnp_for_each_ring(ring, adapter->q_vector[i]->tx) {
			hw_stats->vlan_add_cnt += ring->tx_stats.vlan_add;
			net_stats->tx_packets += ring->stats.packets;
			net_stats->tx_bytes += ring->stats.bytes;
		}
	}
	net_stats->rx_errors += hw_csum_rx_error;

	hw->ops.update_hw_status(hw, hw_stats, net_stats);

	adapter->hw_csum_rx_error = hw_csum_rx_error;
	adapter->hw_csum_rx_good = hw_csum_rx_good;
	net_stats->rx_errors = hw_csum_rx_error;
}

/**
 * rnp_check_hang_subtask - check for hung queues and dropped interrupts
 * @adapter: pointer to the device adapter structure
 *
 * This function serves two purposes.  First it strobes the interrupt lines
 * in order to make certain interrupts are occurring.  Secondly it sets the
 * bits needed to check for TX hangs.  As a result we should immediately
 * determine if a hang has occurred.
 */
static void rnp_check_hang_subtask(struct rnp_adapter *adapter)
{
	int i;
	struct rnp_ring *tx_ring;
	u64 tx_next_to_clean_old;
	u64 tx_next_to_clean;
	u64 tx_next_to_use;
	struct rnp_ring *rx_ring;
	u64 rx_next_to_clean_old;
	u64 rx_next_to_clean;
	union rnp_rx_desc *rx_desc;

	/* If we're down or resetting, just bail */
	if (test_bit(__RNP_DOWN, &adapter->state) ||
	    test_bit(__RNP_RESETTING, &adapter->state))
		return;

	set_bit(__RNP_SERVICE_CHECK, &adapter->state);

	/* Force detection of hung controller */
	if (netif_carrier_ok(adapter->netdev)) {
		for (i = 0; i < adapter->num_tx_queues; i++)
			set_check_for_tx_hang(adapter->tx_ring[i]);
	}

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
					struct rnp_q_vector *q_vector =
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

		if (rx_next_to_clean == rx_next_to_clean_old) {
			rx_ring->rx_stats.rx_equal_count++;

			if ((rx_ring->rx_stats.rx_equal_count > 2) &&
			    (rx_ring->rx_stats.rx_equal_count < 5)) {
				rx_desc = RNP_RX_DESC(rx_ring,
						      rx_ring->next_to_clean);
				if (rnp_test_staterr(rx_desc,
						     RNP_RXD_STAT_DD)) {
					int size;
					struct rnp_q_vector *q_vector =
						rx_ring->q_vector;

					size = le16_to_cpu(rx_desc->wb.len);
					if (size) {
						rx_ring->rx_stats.rx_irq_miss++;
						if (q_vector->rx.ring ||
						    q_vector->tx.ring)
							napi_schedule_irqoff(
								&q_vector->napi);
					} else {
						printk("set RNP_FLAG2_RESET_REQUESTED since size is 0\n");
						adapter->flags2 |=
							RNP_FLAG2_RESET_REQUESTED;
					}
				}
			}
			if (rx_ring->rx_stats.rx_equal_count > 1000)
				rx_ring->rx_stats.rx_equal_count = 0;
		} else {
			rx_ring->rx_stats.rx_equal_count = 0;
		}
		rx_ring->rx_stats.rx_next_to_clean = rx_next_to_clean;
	}

	clear_bit(__RNP_SERVICE_CHECK, &adapter->state);
}

static void update_ring_delay(struct rnp_adapter *adapter)
{
	int i;
	struct rnp_ring *ring;
	struct rnp_hw *hw = &adapter->hw;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		ring = adapter->rx_ring[i];
		ring_wr32(ring, RNP_DMA_REG_RX_INT_DELAY_TIMER,
				adapter->rx_usecs * hw->usecstocount);
		ring = adapter->tx_ring[i];
		ring_wr32(ring, RNP_DMA_REG_TX_INT_DELAY_TIMER,
				adapter->tx_usecs * hw->usecstocount);
	}
}

/**
 * rnp_watchdog_update_link - update the link status
 * @adapter: pointer to the device adapter structure
 * @link_speed: pointer to a u32 to store the link_speed
 **/
static void rnp_watchdog_update_link(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;
	u32 link_speed = adapter->link_speed;
	bool link_up = adapter->link_up;
	bool duplex = adapter->duplex_old;
	bool flow_rx = true, flow_tx = true;

	if (!(adapter->flags & RNP_FLAG_NEED_LINK_UPDATE))
		return;

	if (hw->ops.check_link) {
		hw->ops.check_link(hw, &link_speed, &link_up, &duplex, false);
	} else {
		/* always assume link is up, if no check link function */
		link_speed = RNP_LINK_SPEED_10GB_FULL;
		link_up = true;
	}

	if (link_up || time_after(jiffies, (adapter->link_check_timeout +
					    RNP_TRY_LINK_TIMEOUT))) {
		adapter->flags &= ~RNP_FLAG_NEED_LINK_UPDATE;
	}
	adapter->link_up = link_up;
	adapter->link_speed = link_speed;
	adapter->duplex_old = duplex;

	if (hw->ops.get_pause_mode)
		hw->ops.get_pause_mode(hw);
	switch (hw->fc.current_mode) {
	case rnp_fc_none:
		flow_rx = false;
		flow_tx = false;
		break;
	case rnp_fc_tx_pause:
		flow_rx = false;
		flow_tx = true;

		break;
	case rnp_fc_rx_pause:
		flow_rx = true;
		flow_tx = false;
		break;

	case rnp_fc_full:
		flow_rx = true;
		flow_tx = true;
		break;
	default:
		hw_dbg(hw, "Flow control param set incorrectly\n");
	}
	/* if we detect changed link setup new */
	if (adapter->link_up) {
		if (hw->ops.set_mac_speed)
			hw->ops.set_mac_speed(hw, true, link_speed, duplex);
		/* we should also update pause mode */
		if (hw->ops.set_pause_mode)
			hw->ops.set_pause_mode(hw);

		e_info(drv, "NIC Link is Up %s, %s Duplex, Flow Control: %s\n",
		       (link_speed == RNP_LINK_SPEED_40GB_FULL ?
				"40 Gbps" :
				(link_speed == RNP_LINK_SPEED_25GB_FULL ?
					 "25 Gbps" :
					 (link_speed == RNP_LINK_SPEED_10GB_FULL ?
						  "10 Gbps" :
						  (link_speed == RNP_LINK_SPEED_1GB_FULL ?
							   "1 Gbps" :
							   (link_speed == RNP_LINK_SPEED_100_FULL ?
								    "100 Mbps" :
								    (link_speed == RNP_LINK_SPEED_10_FULL ?
									     "10 Mbps" :
									     "unknown speed")))))),
		       ((duplex) ? "Full" : "Half"),
		       ((flow_rx && flow_tx) ?
				"RX/TX" :
				(flow_rx ? "RX" : (flow_tx ? "TX" : "None"))));
		/* we should update rx irq delay and tx irq delay */
		if (link_speed == RNP_LINK_SPEED_10GB_FULL) {
			adapter->rx_usecs = adapter->rx_usecs_usr_set;
			adapter->tx_usecs = adapter->tx_usecs_usr_set;
		} else {
			adapter->rx_usecs = adapter->rx_usecs_usr_set * 6;
			adapter->tx_usecs = adapter->tx_usecs_usr_set * 2;
		}
		update_ring_delay(adapter);
	} else {
		if (hw->ops.set_mac_speed)
			hw->ops.set_mac_speed(hw, false, 0, false);
	}
}

/**
 * rnp_watchdog_link_is_up - update netif_carrier status and
 *                             print link up message
 * @adapter: pointer to the device adapter structure
 **/
static void rnp_watchdog_link_is_up(struct rnp_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct rnp_hw *hw = &adapter->hw;

	/* only continue if link was previously down */
	if (netif_carrier_ok(netdev))
		return;

	adapter->flags2 &= ~RNP_FLAG2_SEARCH_FOR_SFP;
	switch (hw->mac.type) {
	default:
		break;
	}

	netif_carrier_on(netdev);

	netif_tx_wake_all_queues(netdev);

	hw->ops.set_mac_rx(hw, true);
}

/**
 * rnp_watchdog_link_is_down - update netif_carrier status and
 *                               print link down message
 * @adapter: pointer to the adapter structure
 **/
static void rnp_watchdog_link_is_down(struct rnp_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct rnp_hw *hw = &adapter->hw;

	adapter->link_up = false;
	adapter->link_speed = 0;

	/* only continue if link was up previously */
	if (!netif_carrier_ok(netdev))
		return;

	/* poll for SFP+ cable when link is down */
	if (rnp_is_sfp(hw))
		adapter->flags2 |= RNP_FLAG2_SEARCH_FOR_SFP;

	e_info(drv, "NIC Link is Down\n");

	netif_carrier_off(netdev);

	netif_tx_stop_all_queues(netdev);

	hw->ops.set_mac_rx(hw, false);
}

static void rnp_update_link_to_vf(struct rnp_adapter *adapter)
{
	/* maybe confict with vf */
	if (!(adapter->flags & RNP_FLAG_VF_INIT_DONE))
		return;

	if ((adapter->link_up_old != adapter->link_up) ||
	    (adapter->link_speed_old != adapter->link_speed)) {
		/* if change send mbx to all vf */
		if (!test_bit(__RNP_IN_IRQ, &adapter->state)) {
			if (0 ==
			    rnp_msg_post_status(adapter, PF_SET_LINK_STATUS)) {
				/* maybe delay if we are in other irq? */
				adapter->link_up_old = adapter->link_up;
				adapter->link_speed_old = adapter->link_speed;
			}
		}
	}
}
/**
 * rnp_watchdog_subtask - check and bring link up
 * @adapter: pointer to the device adapter structure
 **/
static void rnp_watchdog_subtask(struct rnp_adapter *adapter)
{
	/* if interface is down do nothing */
	/* should do link status if in sriov */
	if (test_bit(__RNP_DOWN, &adapter->state) ||
	    test_bit(__RNP_RESETTING, &adapter->state))
		return;

	rnp_watchdog_update_link(adapter);

	if (adapter->link_up)
		rnp_watchdog_link_is_up(adapter);
	else
		rnp_watchdog_link_is_down(adapter);

	rnp_update_link_to_vf(adapter);

	rnp_update_stats(adapter);
}

/**
 * rnp_service_timer - Timer Call-back
 * @data: pointer to adapter cast into an unsigned long
 **/
void rnp_service_timer(struct timer_list *t)
{
	struct rnp_adapter *adapter = from_timer(adapter, t, service_timer);
	unsigned long next_event_offset;
	bool ready = true;

	/* poll faster when waiting for link */
	if (adapter->flags & RNP_FLAG_NEED_LINK_UPDATE)
		next_event_offset = HZ / 10;
	else
		next_event_offset = HZ;
	/* Reset the timer */
	if (!test_bit(__RNP_REMOVE, &adapter->state))
		mod_timer(&adapter->service_timer, next_event_offset + jiffies);

	if (ready)
		rnp_service_event_schedule(adapter);
}

static void rnp_reset_pf_subtask(struct rnp_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	u32 err;

	if (!(adapter->flags2 & RNP_FLAG2_RESET_PF))
		return;

	rtnl_lock();
	netif_device_detach(netdev);
	if (netif_running(netdev)) {
		rnp_down(adapter);
		rnp_free_irq(adapter);
		rnp_free_all_tx_resources(adapter);
		rnp_free_all_rx_resources(adapter);
	}
	rtnl_unlock();

	adapter->link_up = 0;
	adapter->link_up_old = 0;
	rnp_msg_post_status(adapter, PF_SET_LINK_STATUS);
	/* wait all vf get this status */
	usleep_range(500, 1000);

	rnp_reset(adapter);
	remove_mbx_irq(adapter);
	rnp_clear_interrupt_scheme(adapter);

	rtnl_lock();
	err = rnp_init_interrupt_scheme(adapter);

	register_mbx_irq(adapter);

	if (!err && netif_running(netdev))
		err = rnp_open(netdev);

	rtnl_unlock();
	rnp_msg_post_status(adapter, PF_SET_RESET);
	netif_device_attach(netdev);
	adapter->flags2 &= (~RNP_FLAG2_RESET_PF);
}

static void rnp_reset_subtask(struct rnp_adapter *adapter)
{
	if (!(adapter->flags2 & RNP_FLAG2_RESET_REQUESTED))
		return;

	adapter->flags2 &= ~RNP_FLAG2_RESET_REQUESTED;

	/* If we're already down or resetting, just bail */
	if (test_bit(__RNP_DOWN, &adapter->state) ||
	    test_bit(__RNP_RESETTING, &adapter->state))
		return;

	netdev_err(adapter->netdev, "Reset adapter\n");
	adapter->tx_timeout_count++;
	rtnl_lock();
	rnp_reinit_locked(adapter);
	rtnl_unlock();
}

static void rnp_rx_len_reset_subtask(struct rnp_adapter *adapter)
{
	int i;
	struct rnp_ring *rx_ring;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		rx_ring = adapter->rx_ring[i];
		if (unlikely(rx_ring->ring_flags &
			     RNP_RING_FLAG_DO_RESET_RX_LEN)) {
			dbg("[%s] Rx-ring %d count reset\n",
			    adapter->netdev->name, rx_ring->rnp_queue_idx);
			if (!rnp_rx_ring_reinit(adapter, rx_ring)) {
				rx_ring->ring_flags &=
					(~RNP_RING_FLAG_DO_RESET_RX_LEN);
			}
		}
	}
}

static void rnp_auto_itr_moderation(struct rnp_adapter *adapter)
{
        int i;
        struct rnp_ring *rx_ring;
        u64 period = (u64)(jiffies - adapter->last_moder_jiffies);

        if (!adapter->adaptive_rx_coal ||
            period < adapter->sample_interval * HZ) {
                return;
        }

        adapter->last_moder_jiffies = jiffies;

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

                        rx_ring->ring_flags |= RNP_RING_LOWER_ITR;
                } else {

                        rx_ring->ring_flags &= (~RNP_RING_LOWER_ITR);
                }

                /* write back new count */
                adapter->last_moder_packets[rx_ring->queue_index] = rx_packets;
        }
}

/**
 * rnp_service_task - manages and runs subtasks
 * @work: pointer to work_struct containing our data
 **/
void rnp_service_task(struct work_struct *work)
{
	struct rnp_adapter *adapter =
		container_of(work, struct rnp_adapter, service_task);

	rnp_reset_subtask(adapter);
	rnp_reset_pf_subtask(adapter);
	rnp_watchdog_subtask(adapter);
	rnp_rx_len_reset_subtask(adapter);
	rnp_auto_itr_moderation(adapter);
	rnp_check_hang_subtask(adapter);
	rnp_service_event_complete(adapter);
}

static int rnp_tso(struct rnp_ring *tx_ring, struct rnp_tx_buffer *first,
		   u32 *mac_ip_len, u8 *hdr_len, u32 *tx_flags)
{
	struct sk_buff *skb = first->skb;
	struct net_device *netdev = tx_ring->netdev;
	struct rnp_adapter *adapter = netdev_priv(netdev);
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
		ip.v4->tot_len = 0;
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
		/* we should alayws do this */
		inner_mac = skb_inner_mac_header(skb);
		first->tunnel_hdr_len = (inner_mac - skb->data);

		if (skb_shinfo(skb)->gso_type &
		    (SKB_GSO_UDP_TUNNEL | SKB_GSO_UDP_TUNNEL_CSUM)) {
			*tx_flags |= RNP_TXD_TUNNEL_VXLAN;
			l4.udp->check = 0;
			tx_dbg("set outer l4.udp to 0\n");
		} else {
			*tx_flags |= RNP_TXD_TUNNEL_NVGRE;
		}

		/* reset pointers to inner headers */
		ip.hdr = skb_inner_network_header(skb);
		l4.hdr = skb_inner_transport_header(skb);
	}

	if (ip.v4->version == 4) {
		/* IP header will have to cancel out any data that
		 * is not a part of the outer IP header
		 */
		ip.v4->tot_len = 0;
		ip.v4->check = 0x0000;

	} else {
		ip.v6->payload_len = 0;
		/* set ipv6 type */
		*tx_flags |= RNP_TXD_FLAG_IPv6;
	}

	/* determine offset of inner transport header */
	l4_offset = l4.hdr - skb->data;

	paylen = skb->len - l4_offset;

	if (skb->csum_offset == offsetof(struct tcphdr, check)) {
		*tx_flags |= RNP_TXD_L4_TYPE_TCP;
		/* compute length of segmentation header */
		*hdr_len = (l4.tcp->doff * 4) + l4_offset;
		csum_replace_by_diff(&l4.tcp->check,
				     (__force __wsum)htonl(paylen));
		l4.tcp->psh = 0;
	} else {
		*tx_flags |= RNP_TXD_L4_TYPE_UDP;
		/* compute length of segmentation header */
		*hdr_len = sizeof(*l4.udp) + l4_offset;
		csum_replace_by_diff(&l4.udp->check,
				     (__force __wsum)htonl(paylen));
	}

	*mac_ip_len = (l4.hdr - ip.hdr) | ((ip.hdr - inner_mac) << 9);

	/* compute header lengths */
	/* pull values out of skb_shinfo */
	gso_size = skb_shinfo(skb)->gso_size;
	gso_segs = skb_shinfo(skb)->gso_segs;

	/* if we close padding check gso confition */
	if (adapter->priv_flags & RNP_PRIV_FLAG_TX_PADDING) {
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
	if (skb->csum_offset == offsetof(struct tcphdr, check)) {
		first->mss_len_vf_num |=
			(gso_size | ((l4.tcp->doff * 4) << 24));
	} else {
		first->mss_len_vf_num |= (gso_size | ((8) << 24));
	}

	*tx_flags |= RNP_TXD_FLAG_TSO | RNP_TXD_IP_CSUM | RNP_TXD_L4_CSUM;

	first->ctx_flag = true;
	return 1;
}

static int rnp_tx_csum(struct rnp_ring *tx_ring, struct rnp_tx_buffer *first,
		       u32 *mac_ip_len, u32 *tx_flags)
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
			*tx_flags |= RNP_TXD_TUNNEL_VXLAN;
			break;
		case IPPROTO_GRE:
			*tx_flags |= RNP_TXD_TUNNEL_NVGRE;
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
		tx_dbg("tunnel length is %d\n", first->tunnel_hdr_len);
	}

	mac_len = (ip.hdr - inner_mac); // mac length
	*mac_ip_len = (ip.hdr - inner_mac) << 9;
	tx_dbg("inner checksum needed %d", skb_checksum_start_offset(skb));
	tx_dbg("skb->encapsulation %d\n", skb->encapsulation);
	ip_len = (l4.hdr - ip.hdr);
	if (ip.v4->version == 4) {
		l4_proto = ip.v4->protocol;
	} else {
		exthdr = ip.hdr + sizeof(*ip.v6);
		l4_proto = ip.v6->nexthdr;
		if (l4.hdr != exthdr)
			ipv6_skip_exthdr(skb, exthdr - skb->data, &l4_proto,
					 &frag_off);
		*tx_flags |= RNP_TXD_FLAG_IPv6;
	}
	/* Enable L4 checksum offloads */
	switch (l4_proto) {
	case IPPROTO_TCP:
		*tx_flags |= RNP_TXD_L4_TYPE_TCP | RNP_TXD_L4_CSUM;
		break;
	case IPPROTO_SCTP:
		tx_dbg("sctp checksum packet\n");
		*tx_flags |= RNP_TXD_L4_TYPE_SCTP | RNP_TXD_L4_CSUM;
		break;
	case IPPROTO_UDP:
		*tx_flags |= RNP_TXD_L4_TYPE_UDP | RNP_TXD_L4_CSUM;
		break;
	default:
		skb_checksum_help(skb);
		return 0;
	}

	/* should consider stags mode */
	if ((tx_ring->ring_flags & RNP_RING_NO_TUNNEL_SUPPORT) &&
	    (first->ctx_flag)) {
		/* if not support tunnel */
		*tx_flags &= (~RNP_TXD_TUNNEL_MASK);
		if (!(first->priv_tags)) {
			first->ctx_flag = false;
			mac_len += first->tunnel_hdr_len;
			first->tunnel_hdr_len = 0;
		}
	}
	*mac_ip_len = (mac_len << 9) | ip_len;

	return 0;
}

static int __rnp_maybe_stop_tx(struct rnp_ring *tx_ring, u16 size)
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
	if (likely(rnp_desc_unused(tx_ring) < size))
		return -EBUSY;

	/* A reprieve! - use start_queue because it doesn't call schedule */
	netif_start_subqueue(tx_ring->netdev, tx_ring->queue_index);
	++tx_ring->tx_stats.restart_queue;
	return 0;
}

static inline int rnp_maybe_stop_tx(struct rnp_ring *tx_ring, u16 size)
{
	if (likely(rnp_desc_unused(tx_ring) >= size))
		return 0;
	return __rnp_maybe_stop_tx(tx_ring, size);
}

static int rnp_tx_map(struct rnp_ring *tx_ring, struct rnp_tx_buffer *first,
		      u32 mac_ip_len, u32 tx_flags)
{
	struct sk_buff *skb = first->skb;
	struct rnp_tx_buffer *tx_buffer;
	struct rnp_tx_desc *tx_desc;
	skb_frag_t *frag;
	dma_addr_t dma;
	unsigned int data_len, size;
	u16 i = tx_ring->next_to_use;
	u64 fun_id = ((u64)(tx_ring->pfvfnum) << (56));

	tx_desc = RNP_TX_DESC(tx_ring, i);
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

		while (unlikely(size > RNP_MAX_DATA_PER_TXD)) {
			tx_desc->vlan_cmd_bsz = build_ctob(
				tx_flags, mac_ip_len, RNP_MAX_DATA_PER_TXD);
			/* ==== desc== */
			buf_dump_line("tx0  ", __LINE__, tx_desc,
				      sizeof(*tx_desc));
			i++;
			tx_desc++;
			if (i == tx_ring->count) {
				tx_desc = RNP_TX_DESC(tx_ring, 0);
				i = 0;
			}
			dma += RNP_MAX_DATA_PER_TXD;
			size -= RNP_MAX_DATA_PER_TXD;

			tx_desc->pkt_addr = cpu_to_le64(dma | fun_id);
		}

		buf_dump_line("tx1  ", __LINE__, tx_desc, sizeof(*tx_desc));
		if (likely(!data_len))
			break;
		tx_desc->vlan_cmd_bsz = build_ctob(tx_flags, mac_ip_len, size);
		buf_dump_line("tx2  ", __LINE__, tx_desc, sizeof(*tx_desc));

		/* ==== frag== */
		i++;
		tx_desc++;
		if (i == tx_ring->count) {
			tx_desc = RNP_TX_DESC(tx_ring, 0);
			i = 0;
		}

		size = skb_frag_size(frag);
		data_len -= size;
		dma = skb_frag_dma_map(tx_ring->dev, frag, 0, size,
				       DMA_TO_DEVICE);
		tx_buffer = &tx_ring->tx_buffer_info[i];
	}

	/* write last descriptor with RS and EOP bits */
	tx_desc->vlan_cmd_bsz = build_ctob(
		tx_flags | RNP_TXD_CMD_EOP | RNP_TXD_CMD_RS, mac_ip_len, size);
	buf_dump_line("tx3  ", __LINE__, tx_desc, sizeof(*tx_desc));

	/* set the timestamp */
	first->time_stamp = jiffies;

	tx_ring->tx_stats.send_bytes += first->bytecount;
#ifdef NO_BQL_TEST
#else
	netdev_tx_sent_queue(txring_txq(tx_ring), first->bytecount);
#endif

	/*
	 * Force memory writes to complete before letting h/w know there
	 * are new descriptors to fetch.  (Only applicable for weak-ordered
	 * memory model archs, such as IA-64).
	 *
	 * We also need this memory barrier to make certain all of the
	 * status bits have been updated before next_to_watch is written.
	 */
	/* timestamp the skb as late as possible, just prior to notifying
	 * the MAC that it should transmit this packet
	 */
	wmb();
	/* set next_to_watch value indicating a packet is present */
	first->next_to_watch = tx_desc;

	buf_dump_line("tx4  ", __LINE__, tx_desc, sizeof(*tx_desc));
	i++;
	if (i == tx_ring->count)
		i = 0;
	tx_ring->next_to_use = i;

	/* need this */
	rnp_maybe_stop_tx(tx_ring, DESC_NEEDED);
	skb_tx_timestamp(skb);

	if (netif_xmit_stopped(txring_txq(tx_ring)) || !netdev_xmit_more()) {
		tx_ring->tx_stats.send_bytes_to_hw += first->bytecount;
		tx_ring->tx_stats.send_bytes_to_hw +=
			tx_ring->tx_stats.todo_update;
		tx_ring->tx_stats.todo_update = 0;
		rnp_wr_reg(tx_ring->tail, i);
	} else {
		tx_ring->tx_stats.todo_update += first->bytecount;
	}
	return 0;
dma_error:
	dev_err(tx_ring->dev, "TX DMA map failed\n");

	/* clear dma mappings for failed tx_buffer_info map */
	for (;;) {
		tx_buffer = &tx_ring->tx_buffer_info[i];
		rnp_unmap_and_free_tx_resource(tx_ring, tx_buffer);
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

static void rnp_force_src_mac(struct sk_buff *skb, struct net_device *netdev)
{
	u8 *data = skb->data;
	bool ret = false;
	struct netdev_hw_addr *ha;
	/* force all src mac to myself */
	if (is_multicast_ether_addr(data)) {
		if (0 == memcmp(data + netdev->addr_len, netdev->dev_addr,
				netdev->addr_len)) {
			ret = true;
			goto DONE;
		}
		netdev_for_each_uc_addr(ha, netdev) {
			if (0 == memcmp(data + netdev->addr_len, ha->addr,
					netdev->addr_len)) {
				ret = true;
				goto DONE;
			}
		}
		/* if not src mac, force to src mac */
		if (!ret)
			memcpy(data + netdev->addr_len, netdev->dev_addr,
			       netdev->addr_len);
	}
DONE:
	return;
}

netdev_tx_t rnp_xmit_frame_ring(struct sk_buff *skb,
				struct rnp_adapter *adapter,
				struct rnp_ring *tx_ring, bool tx_padding)
{
	struct rnp_tx_buffer *first;
	int tso;
	u32 tx_flags = 0;
	unsigned short f;
	u16 count = TXD_USE_COUNT(skb_headlen(skb));
	__be16 protocol = skb->protocol;
	u8 hdr_len = 0;
	int ignore_vlan = 0;
	/* default len should not 0 (hw request) */
	u32 mac_ip_len = 20;

	tx_dbg("=== begin ====\n");
	tx_dbg("rnp skb:%p, skb->len:%d  headlen:%d, data_len:%d\n", skb,
	       skb->len, skb_headlen(skb), skb->data_len);
	tx_dbg("next_to_clean %d, next_to_use %d\n", tx_ring->next_to_clean,
	       tx_ring->next_to_use);
	/*
	 * need: 1 descriptor per page * PAGE_SIZE/RNP_MAX_DATA_PER_TXD,
	 *       + 1 desc for skb_headlen/RNP_MAX_DATA_PER_TXD,
	 *       + 2 desc gap to keep tail from touching head,
	 *       + 1 desc for context descriptor,
	 * otherwise try next time
	 */
	for (f = 0; f < skb_shinfo(skb)->nr_frags; f++) {
		skb_frag_t *frag_temp = &skb_shinfo(skb)->frags[f];

		count += TXD_USE_COUNT(skb_frag_size(frag_temp));
		tx_dbg(" rnp #%d frag: size:%d\n", f, skb_frag_size(frag_temp));
	}

	if (rnp_maybe_stop_tx(tx_ring, count + 3)) {
		tx_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}

	if ((adapter->flags & RNP_FLAG_SRIOV_ENABLED) &&
	    (!(tx_ring->ring_flags & RNP_RING_VEB_MULTI_FIX)))
		rnp_force_src_mac(skb, tx_ring->netdev);

	/* record the location of the first descriptor for this packet */
	first = &tx_ring->tx_buffer_info[tx_ring->next_to_use];
	first->skb = skb;
	first->bytecount = skb->len;
	first->gso_segs = 1;
	first->priv_tags = 0;

	first->mss_len_vf_num = 0;
	first->inner_vlan_tunnel_len = 0;

	first->ctx_flag = (adapter->flags & RNP_FLAG_SRIOV_ENABLED) ? true :
								      false;

	/* if we have a HW VLAN tag being added default to the HW one */
	/* RNP_TXD_VLAN_VALID is used for veb */
	/* setup padding flag */

	if (adapter->priv_flags & RNP_PRIV_FLAG_TX_PADDING) {
		first->ctx_flag = true;
		/* should consider sctp */
		first->gso_need_padding = tx_padding;
	}

	/* RNP_FLAG2_VLAN_STAGS_ENABLED and
	 * tx-stags-offload not support together
	 */
	if (adapter->flags2 & RNP_FLAG2_VLAN_STAGS_ENABLED) {
		/* always add a stags for any packets out */
		if (tx_ring->ring_flags & RNP_RING_OUTER_VLAN_FIX) {
			/* set outer_vlan to ctx */
			first->inner_vlan_tunnel_len |= (adapter->stags_vid);
			first->priv_tags = 1;
			first->ctx_flag = true;

			if (skb_vlan_tag_present(skb)) {
				tx_flags |= RNP_TXD_VLAN_VALID |
					    RNP_TXD_VLAN_CTRL_INSERT_VLAN;
				tx_flags |= skb_vlan_tag_get(skb);
				/* else if it is a SW VLAN check the next
				 * protocol and store the tag
				 */
			} else if (protocol == htons(ETH_P_8021Q)) {
				struct vlan_hdr *vhdr, _vhdr;

				vhdr = skb_header_pointer(
					skb, ETH_HLEN, sizeof(_vhdr), &_vhdr);
				if (!vhdr)
					goto out_drop;

				protocol = vhdr->h_vlan_encapsulated_proto;
				tx_flags |= ntohs(vhdr->h_vlan_TCI);
				tx_flags |= RNP_TXD_VLAN_VALID;
			}

		} else {
			/* sriov mode not support this */
			tx_flags |= adapter->stags_vid;
			tx_flags |= RNP_TXD_VLAN_CTRL_INSERT_VLAN;
			if (skb_vlan_tag_present(skb)) {
				tx_flags |= RNP_TXD_VLAN_VALID;
				first->inner_vlan_tunnel_len |=
					(skb_vlan_tag_get(skb) << 8);
				first->ctx_flag = true;
				/* else if it is a SW VLAN check the next
				 * protocol and store the tag
				 */
			} else if (protocol == htons(ETH_P_8021Q)) {
				struct vlan_hdr *vhdr, _vhdr;

				vhdr = skb_header_pointer(
					skb, ETH_HLEN, sizeof(_vhdr), &_vhdr);
				if (!vhdr)
					goto out_drop;

				protocol = vhdr->h_vlan_encapsulated_proto;
				tx_flags |= RNP_TXD_VLAN_VALID;
			}
		}
	} else {
		/* normal mode*/
		if (skb_vlan_tag_present(skb)) {
			if (skb->vlan_proto != htons(ETH_P_8021Q)) {
				/* veb only use ctags */
				tx_flags |= skb_vlan_tag_get(skb);
				tx_flags |= RNP_TXD_SVLAN_TYPE |
					    RNP_TXD_VLAN_CTRL_INSERT_VLAN;
			} else {
				tx_flags |= skb_vlan_tag_get(skb);
				tx_flags |= RNP_TXD_VLAN_VALID |
					    RNP_TXD_VLAN_CTRL_INSERT_VLAN;
			}
			tx_ring->tx_stats.vlan_add++;
			/* else if it is a SW VLAN check the next
			 * protocol and store the tag
			 */
			/* veb only use ctags */
		} else if (protocol == htons(ETH_P_8021Q)) {
			struct vlan_hdr *vhdr, _vhdr;

			vhdr = skb_header_pointer(skb, ETH_HLEN, sizeof(_vhdr),
						  &_vhdr);
			if (!vhdr)
				goto out_drop;

			protocol = vhdr->h_vlan_encapsulated_proto;
			tx_flags |= ntohs(vhdr->h_vlan_TCI);
			tx_flags |= RNP_TXD_VLAN_VALID;
			ignore_vlan = 1;
		}
	}
	protocol = vlan_get_protocol(skb);
	if (unlikely(skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) &&
	    adapter->flags2 & RNP_FLAG2_PTP_ENABLED && adapter->ptp_tx_en) {
		if (!test_and_set_bit_lock(__RNP_PTP_TX_IN_PROGRESS,
					   &adapter->state)) {
			skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
			tx_flags |= RNP_TXD_FLAG_PTP;
			adapter->ptp_tx_skb = skb_get(skb);
			adapter->tx_hwtstamp_start = jiffies;
			schedule_work(&adapter->tx_hwtstamp_work);
		} else {
			printk("ptp_tx_skb miss\n");
		}
	}
	/* record initial flags and protocol */
	tso = rnp_tso(tx_ring, first, &mac_ip_len, &hdr_len, &tx_flags);
	if (tso < 0)
		goto out_drop;
	else if (!tso)
		rnp_tx_csum(tx_ring, first, &mac_ip_len, &tx_flags);
	/* check sriov mode */
	/* in this mode pf send msg should with vf_num */
	if (unlikely(adapter->flags & RNP_FLAG_SRIOV_ENABLED)) {
		first->ctx_flag = true;
		first->mss_len_vf_num |= (adapter->vf_num_for_pf << 16);
	}

	/* add control desc */
	rnp_maybe_tx_ctxtdesc(tx_ring, first, ignore_vlan);
	/* add the ATR filter if ATR is on */
	if (rnp_tx_map(tx_ring, first, mac_ip_len, tx_flags)) {
		goto cleanup_tx_tstamp;
	}
	tx_dbg("=== end ====\n\n\n\n");
	return NETDEV_TX_OK;

out_drop:
	dev_kfree_skb_any(first->skb);
	first->skb = NULL;
cleanup_tx_tstamp:
	if (unlikely(tx_flags & RNP_TXD_FLAG_PTP)) {
		dev_kfree_skb_any(adapter->ptp_tx_skb);
		adapter->ptp_tx_skb = NULL;
		cancel_work_sync(&adapter->tx_hwtstamp_work);
		clear_bit_unlock(__RNP_PTP_TX_IN_PROGRESS, &adapter->state);
	}

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
	/* sctp set no_padding to true */
	switch (l4_proto) {
	case IPPROTO_SCTP:
		no_padding = true;
		break;
	default:

		break;
	}
	return no_padding;
}

static netdev_tx_t rnp_xmit_frame(struct sk_buff *skb,
				  struct net_device *netdev)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_ring *tx_ring;
	bool tx_padding = false;

	if (!netif_carrier_ok(netdev)) {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	/*
	 * The minimum packet size for olinfo paylen is 17 so pad the skb
	 * in order to meet this minimum size requirement.
	 */
	if (adapter->priv_flags & RNP_PRIV_FLAG_TX_PADDING) {
		if (skb->len < 60) {
			if (!check_sctp_no_padding(skb)) {
				if (skb_put_padto(skb, 60))
					return NETDEV_TX_OK;
			} else {
				/* if sctp smaller than 60, never padding */
				tx_padding = true;
			}
		}
	} else {
		if (skb_put_padto(skb, 33))
			return NETDEV_TX_OK;
	}
	tx_ring = adapter->tx_ring[skb->queue_mapping];

	return rnp_xmit_frame_ring(skb, adapter, tx_ring, tx_padding);
}

/**
 * rnp_set_mac - Change the Ethernet Address of the NIC
 * @netdev: network interface device structure
 * @p: pointer to an address structure
 *
 * Returns 0 on success, negative on failure
 **/
static int rnp_set_mac(struct net_device *netdev, void *p)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	struct sockaddr *addr = p;
	bool sriov_flag = !!(adapter->flags & RNP_FLAG_SRIOV_ENABLED);

	dbg("[%s] call set mac\n", netdev->name);

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	eth_hw_addr_set(netdev, addr->sa_data);
	memcpy(hw->mac.addr, addr->sa_data, netdev->addr_len);
	hw->ops.set_mac(hw, hw->mac.addr, sriov_flag);

	/* reset veb table */
	rnp_configure_virtualization(adapter);
	return 0;
}

static int rnp_mdio_read(struct net_device *netdev, int prtad, int devad,
			 u32 addr, u32 *phy_value)
{
	int rc = -EIO;
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;
	u16 value;

	rc = hw->ops.phy_read_reg(hw, addr, 0, &value);
	*phy_value = value;

	return rc;
}

static int rnp_mdio_write(struct net_device *netdev, int prtad, int devad,
			  u16 addr, u16 value)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	return hw->ops.phy_write_reg(hw, addr, 0, value);
}

static int rnp_mii_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	struct mii_ioctl_data *mii = (struct mii_ioctl_data *)&ifr->ifr_data;
	int prtad, devad, ret;
	u32 phy_value;

	prtad = (mii->phy_id & MDIO_PHY_ID_PRTAD) >> 5;
	devad = (mii->phy_id & MDIO_PHY_ID_DEVAD);

	if (cmd == SIOCGMIIREG) {
		ret = rnp_mdio_read(netdev, prtad, devad, mii->reg_num,
				    &phy_value);
		if (ret < 0)
			return ret;
		mii->val_out = phy_value;
		return 0;
	} else {
		return rnp_mdio_write(netdev, prtad, devad, mii->reg_num,
				      mii->val_in);
	}
}

static int rnp_ioctl(struct net_device *netdev, struct ifreq *req, int cmd)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);

	/* ptp 1588 used this */
	switch (cmd) {
	case SIOCGHWTSTAMP:
		if (module_enable_ptp)
			return rnp_ptp_get_ts_config(adapter, req);
		break;
	case SIOCSHWTSTAMP:
		if (module_enable_ptp)
			return rnp_ptp_set_ts_config(adapter, req);
		break;
	case SIOCGMIIPHY:
		return 0;
		break;
	case SIOCGMIIREG:
		/* n400 use this */
		/* fall through */
	case SIOCSMIIREG:
		return rnp_mii_ioctl(netdev, req, cmd);
		break;
	}
	return -EINVAL;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
 * Polling 'interrupt' - used by things like netconsole to send skbs
 * without having to re-enable interrupts. It's not called while
 * the interrupt routine is executing.
 */
static void rnp_netpoll(struct net_device *netdev)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int i;

	/* if interface is down do nothing */
	if (test_bit(__RNP_DOWN, &adapter->state))
		return;

	adapter->flags |= RNP_FLAG_IN_NETPOLL;
	for (i = 0; i < adapter->num_q_vectors; i++)
		rnp_msix_clean_rings(0, adapter->q_vector[i]);
	adapter->flags &= ~RNP_FLAG_IN_NETPOLL;
}

#endif

static void rnp_get_stats64(struct net_device *netdev,
			    struct rtnl_link_stats64 *stats)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	int i;

	rcu_read_lock();
	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct rnp_ring *ring = READ_ONCE(adapter->rx_ring[i]);
		u64 bytes, packets;
		unsigned int start;

		if (ring) {
			do {
				start = u64_stats_fetch_begin(&ring->syncp);
				packets = ring->stats.packets;
				bytes = ring->stats.bytes;
			} while (u64_stats_fetch_retry(&ring->syncp, start));
			stats->rx_packets += packets;
			stats->rx_bytes += bytes;
		}
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct rnp_ring *ring = READ_ONCE(adapter->tx_ring[i]);
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

/**
 * rnp_setup_tc - configure net_device for multiple traffic classes
 *
 * @netdev: net device to configure
 * @tc: number of traffic classes to enable
 */
int rnp_setup_tc(struct net_device *dev, u8 tc)
{
	struct rnp_adapter *adapter = netdev_priv(dev);
	struct rnp_hw *hw = &adapter->hw;
	int ret = 0;

	if (hw->hw_type != rnp_hw_n10)
		return -EINVAL;
	/* if now we are in force mode, never need force, if not force it */
	if (!(adapter->priv_flags & RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE)) {
		hw->ops.set_mac_rx(hw, false);
		if (hw->ops.driver_status)
			hw->ops.driver_status(hw, true,
					      rnp_driver_force_control_mac);
	}

	/* Hardware supports up to 8 traffic classes */
	if ((tc > RNP_MAX_TCS_NUM) || (tc == 1))
		return -EINVAL;
	/* we canot support tc with sriov mode */
	if ((tc) && (adapter->flags & RNP_FLAG_SRIOV_ENABLED))
		return -EINVAL;

	/* Hardware has to reinitialize queues and interrupts to
	 * match packet buffer alignment. Unfortunately, the
	 * hardware is not flexible enough to do this dynamically.
	 */
	while (test_and_set_bit(__RNP_RESETTING, &adapter->state))
		usleep_range(1000, 2000);

	if (netif_running(dev))
		rnp_close(dev);

	rnp_fdir_filter_exit(adapter);
	adapter->priv_flags &= (~RNP_PRIV_FLAG_TCP_SYNC);
	remove_mbx_irq(adapter);
	rnp_clear_interrupt_scheme(adapter);
	adapter->num_tc = tc;

	if (tc) {
		netdev_set_num_tc(dev, tc);
		adapter->flags |= RNP_FLAG_DCB_ENABLED;
	} else {
		netdev_reset_tc(dev);
		adapter->flags &= ~RNP_FLAG_DCB_ENABLED;
	}

	rnp_init_interrupt_scheme(adapter);

	register_mbx_irq(adapter);
	/* rss table must reset */
	adapter->rss_tbl_setup_flag = 0;

	if (netif_running(dev))
		ret = rnp_open(dev);

	/* if we not set force now */
	if (!(adapter->priv_flags & RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE)) {
		hw->ops.set_mac_rx(hw, false);
		if (hw->ops.driver_status)
			hw->ops.driver_status(hw, false,
					      rnp_driver_force_control_mac);
	}

	clear_bit(__RNP_RESETTING, &adapter->state);
	return ret;
}

#ifdef CONFIG_PCI_IOV
void rnp_sriov_reinit(struct rnp_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	rtnl_lock();
	rnp_setup_tc(netdev, netdev_get_num_tc(netdev));
	rtnl_unlock();
	usleep_range(10000, 20000);
}
#endif

static int rnp_delete_knode(struct net_device *dev, struct tc_cls_u32_offload *cls)
{
	/* 1. check weather filter rule is ingress root */
	struct rnp_adapter *adapter = netdev_priv(dev);
	u32 loc = cls->knode.handle & 0xfffff;
	u32 uhtid = TC_U32_USERHTID(cls->knode.handle);
	int ret;

	if ((uhtid != 0x800))
		return -EINVAL;

	spin_lock(&adapter->fdir_perfect_lock);
	ret = rnp_update_ethtool_fdir_entry(adapter, NULL, loc);
	spin_unlock(&adapter->fdir_perfect_lock);

	return ret;
}

#ifdef CONFIG_NET_CLS_ACT
static int rnp_action_parse(struct tcf_exts *exts, u64 *action, u8 *queue)
{
	const struct tc_action *a;
	int j;

	if (!tcf_exts_has_actions(exts))
		return -EINVAL;

	tcf_exts_for_each_action(j, a, exts) {
		/* Drop action */
		if (is_tcf_gact_shot(a)) {
			*action = RNP_FDIR_DROP_QUEUE;
			*queue = RNP_FDIR_DROP_QUEUE;
			return 0;
		}
		/* Redirect to a VF or a offloaded macvlan */
		if (is_tcf_mirred_egress_redirect(a)) {

			struct net_device *dev = tcf_mirred_dev(a);

			if (!dev)
				return -EINVAL;
		}

		return -EINVAL;
	}

	return 0;
}

#else
static int rnp_action_parse(struct tcf_exts *exts, u64 *action, u8 *queue)
{
	return -EINVAL;
}
#endif

static int rnp_clsu32_build_input(struct tc_cls_u32_offload *cls,
				  struct rnp_fdir_filter *input,
				  const struct rnp_match_parser *parsers)
{
	int i = 0, j = 0, err = -1;
	__be32 val, mask, off;
	bool found;

	for (i = 0; i < cls->knode.sel->nkeys; i++) {
		off = cls->knode.sel->keys[i].off;
		val = cls->knode.sel->keys[i].val;
		mask = cls->knode.sel->keys[i].mask;
		dbg("cls-key[%d] off %d val %d mask %d\n ", i, off, val, mask);
		found = false;
		for (j = 0; parsers[j].val; j++) {
			/* according the off select parser */
			if (off == parsers[j].off) {
				found = true;
				err = parsers[j].val(input, val, mask);
				if (err)
					return err;

				break;
			}
		}
		/* if the rule can't parse that we don't support the rule */
		if (!found)
			return -EINVAL;
	}

	return 0;
}

static int rnp_config_knode(struct net_device *dev, __be16 protocol,
		     struct tc_cls_u32_offload *cls)
{
	/*1. check ethernet hw-feature U32 can offload */
	/*2. check U32 protocol We just support IPV4 offloading For now*/
	/*3. check if this cls is a cls of root u32 or cls of class u32*/
	/*4. check if this cls has been added.
	 * the filter extry create but the match val and mask don't fill
	 * so we can use it.
	 * find a exist extry and the match val and mask is added before
	 * so we don't need add it again
	 */
	u32 uhtid, link_uhtid;
	int ret;
	struct rnp_adapter *adapter = netdev_priv(dev);
	u8 queue;
	struct rnp_fdir_filter *input;
	// struct rnp_hw *hw = &adapter->hw;
	u32 loc = cls->knode.handle & 0xfffff;

	if (protocol != htons(ETH_P_IP))
		return -EOPNOTSUPP;

	uhtid = TC_U32_USERHTID(cls->knode.handle);
	link_uhtid = TC_U32_USERHTID(cls->knode.link_handle);

	netdev_info(dev, "uhtid %d link_uhtid %d protocol 0x%2x\n", uhtid,
		    link_uhtid, ntohs(protocol));
	/* For now just support handle root ingress
	 * TODO more feature
	 */
	if (uhtid != 0x800)
		return -EINVAL;

	input = kzalloc(sizeof(*input), GFP_KERNEL);
	/*be carefull this input mem need to free */
	ret = rnp_clsu32_build_input(cls, input, rnp_ipv4_parser);
	if (ret) {
		netdev_warn(dev, "This Rules We Can't Support It\n");
		goto out;
	}
	ret = rnp_action_parse(cls->knode.exts, &input->action, &queue);
	if (ret)
		goto out;

	dbg("tc filter rule sw_location %d\n", loc);

	/* maybe bug here */
	input->hw_idx = adapter->tuple_5_count++;
	input->sw_idx = loc;
	spin_lock(&adapter->fdir_perfect_lock);
	rnp_update_ethtool_fdir_entry(adapter, input, input->sw_idx);
	spin_unlock(&adapter->fdir_perfect_lock);

	return 0;
out:
	kfree(input);
	return -EOPNOTSUPP;
}

static int rnp_setup_tc_cls_u32(struct net_device *dev,
			 struct tc_cls_u32_offload *cls_u32)
{
	__be16 proto = cls_u32->common.protocol;
	dbg("cls_u32->command is %d\n", cls_u32->command);
	switch (cls_u32->command) {
	case TC_CLSU32_NEW_KNODE:
	case TC_CLSU32_REPLACE_KNODE:
		return rnp_config_knode(dev, proto, cls_u32);
	case TC_CLSU32_DELETE_KNODE:
		return rnp_delete_knode(dev, cls_u32);
	default:
		return -EOPNOTSUPP;
	}
}

static int rnp_setup_tc_block_ingress_cb(enum tc_setup_type type,
					 void *type_data, void *cb_priv)
{
	struct net_device *dev = cb_priv;
	struct rnp_adapter *adapter = netdev_priv(dev);

	if (test_bit(__RNP_DOWN, &adapter->state)) {
		netdev_err(
			adapter->netdev,
			"Failed to setup tc on port %d. Link Down? 0x%.2lx\n",
			adapter->port, adapter->state);
		return -EINVAL;
	}
	if (!tc_cls_can_offload_and_chain0(dev, type_data))
		return -EOPNOTSUPP;

	switch (type) {
	case TC_SETUP_CLSU32:
		return rnp_setup_tc_cls_u32(dev, type_data);
	default:
		return -EOPNOTSUPP;
	}
}

static LIST_HEAD(rnp_block_cb_list);

static int rnp_setup_mqprio(struct net_device *dev,
			    struct tc_mqprio_qopt *mqprio)
{
	mqprio->hw = TC_MQPRIO_HW_OFFLOAD_TCS;
	return rnp_setup_tc(dev, mqprio->num_tc);
}

static int __rnp_setup_tc(struct net_device *netdev, enum tc_setup_type type,
			  void *type_data)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	switch (type) {
	case TC_SETUP_BLOCK: {
		struct flow_block_offload *f =
			(struct flow_block_offload *)type_data;
		if (f->binder_type == FLOW_BLOCK_BINDER_TYPE_CLSACT_INGRESS)
			return flow_block_cb_setup_simple(
				type_data, &rnp_block_cb_list,
				rnp_setup_tc_block_ingress_cb, adapter, adapter,
				true);
		else
			return -EOPNOTSUPP;
	}
	case TC_SETUP_CLSU32:
		return rnp_setup_tc_cls_u32(netdev, type_data);
	case TC_SETUP_QDISC_MQPRIO:
		return rnp_setup_mqprio(netdev, type_data);
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

void rnp_do_reset(struct net_device *netdev)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev))
		rnp_reinit_locked(adapter);
	else
		rnp_reset(adapter);
}

static netdev_features_t rnp_fix_features(struct net_device *netdev,
					  netdev_features_t features)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	struct rnp_hw *hw = &adapter->hw;

	/* If Rx checksum is disabled, then RSC/LRO should also be disabled */
	if (!(features & NETIF_F_RXCSUM))
		features &= ~NETIF_F_LRO;

	/* close rx csum when rx fcs on */
	if (!(adapter->flags2 & RNP_FLAG2_CHKSM_FIX)) {
		if (features & NETIF_F_RXFCS)
			features &= (~NETIF_F_RXCSUM);
	}
	/* Turn off LRO if not RSC capable */
	if (!(adapter->flags2 & RNP_FLAG2_RSC_CAPABLE))
		features &= ~NETIF_F_LRO;
	if (!(features & NETIF_F_HW_VLAN_CTAG_FILTER)) {
		if (hw->feature_flags & RNP_NET_FEATURE_STAG_FILTER)
			features &= ~NETIF_F_HW_VLAN_STAG_FILTER;
	}

	if (hw->feature_flags & RNP_NET_FEATURE_STAG_FILTER) {
		if (!(features & NETIF_F_HW_VLAN_STAG_FILTER))
			features &= ~NETIF_F_HW_VLAN_CTAG_FILTER;
	}

	if (!(features & NETIF_F_HW_VLAN_CTAG_RX)) {
		if (hw->feature_flags & RNP_NET_FEATURE_STAG_OFFLOAD)
			features &= ~NETIF_F_HW_VLAN_STAG_RX;
	}

	if (hw->feature_flags & RNP_NET_FEATURE_STAG_OFFLOAD) {
		if (!(features & NETIF_F_HW_VLAN_STAG_RX))
			features &= ~NETIF_F_HW_VLAN_CTAG_RX;
	}

	if (!(features & NETIF_F_HW_VLAN_CTAG_TX)) {
		if (hw->feature_flags & RNP_NET_FEATURE_STAG_OFFLOAD)
			features &= ~NETIF_F_HW_VLAN_STAG_TX;
	}

	if (hw->feature_flags & RNP_NET_FEATURE_STAG_OFFLOAD) {
		if (!(features & NETIF_F_HW_VLAN_STAG_TX))
			features &= ~NETIF_F_HW_VLAN_CTAG_TX;
	}

	return features;
}

static int rnp_set_features(struct net_device *netdev,
			    netdev_features_t features)
{
	struct rnp_adapter *adapter = netdev_priv(netdev);
	netdev_features_t changed = netdev->features ^ features;
	bool need_reset = false;
	struct rnp_hw *hw = &adapter->hw;

	netdev->features = features;

	/* if changed ntuple should close all */
	if (changed & NETIF_F_NTUPLE) {
		if (!(features & NETIF_F_NTUPLE)) {
			rnp_fdir_filter_exit(adapter);
		}
	}

	switch (features & NETIF_F_NTUPLE) {
	case NETIF_F_NTUPLE:
		/* turn off ATR, enable perfect filters and reset */
		if (!(adapter->flags & RNP_FLAG_FDIR_PERFECT_CAPABLE))
			need_reset = true;

		adapter->flags &= ~RNP_FLAG_FDIR_HASH_CAPABLE;
		adapter->flags |= RNP_FLAG_FDIR_PERFECT_CAPABLE;
		break;
	default:
		/* turn off perfect filters, enable ATR and reset */
		if (adapter->flags & RNP_FLAG_FDIR_PERFECT_CAPABLE)
			need_reset = true;

		adapter->flags &= ~RNP_FLAG_FDIR_PERFECT_CAPABLE;

		/* We cannot enable ATR if SR-IOV is enabled */
		if (adapter->flags & RNP_FLAG_SRIOV_ENABLED)
			break;

		/* We cannot enable ATR if we have 2 or more traffic classes */
		if (netdev_get_num_tc(netdev) > 1)
			break;

		/* A sample rate of 0 indicates ATR disabled */
		if (!adapter->atr_sample_rate)
			break;

		adapter->flags |= RNP_FLAG_FDIR_HASH_CAPABLE;
		break;
	}

	/* vlan filter changed */
	if (changed & NETIF_F_HW_VLAN_CTAG_FILTER) {
		if (features & (NETIF_F_HW_VLAN_CTAG_FILTER))
			hw->ops.set_vlan_filter_en(hw, true);
		else
			hw->ops.set_vlan_filter_en(hw, false);
		rnp_msg_post_status(adapter, PF_VLAN_FILTER_STATUS);
	}

	/* rss hash changed */
	if (changed & (NETIF_F_RXHASH)) {
		bool iov_en = (adapter->flags & RNP_FLAG_SRIOV_ENABLED) ? true :
									  false;

		if (netdev->features & (NETIF_F_RXHASH))
			hw->ops.set_rx_hash(hw, true, iov_en);
		else
			hw->ops.set_rx_hash(hw, false, iov_en);
	}

	/* rx fcs changed */
	/* in this mode rx l4/sctp checksum will get error */
	if (changed & NETIF_F_RXFCS) {

		if (features & NETIF_F_RXFCS) {
			adapter->priv_flags |= RNP_PRIV_FLAG_RX_FCS;
			hw->ops.set_fcs_mode(hw, true);
			/* if in rx fcs mode ,hw rxcsum may error,
			 * close rxcusm
			 */
		} else {
			adapter->priv_flags &= (~RNP_PRIV_FLAG_RX_FCS);
			hw->ops.set_fcs_mode(hw, false);
		}
		rnp_msg_post_status(adapter, PF_FCS_STATUS);
	}

	if (changed & NETIF_F_RXALL)
		need_reset = true;

	if (features & NETIF_F_RXALL)
		adapter->priv_flags |= RNP_PRIV_FLAG_RX_ALL;
	else
		adapter->priv_flags &= (~RNP_PRIV_FLAG_RX_ALL);

	if (features & NETIF_F_HW_VLAN_CTAG_RX)
		rnp_vlan_strip_enable(adapter);
	else
		rnp_vlan_strip_disable(adapter);

	if (need_reset)
		rnp_do_reset(netdev);

	return 0;
}

static int rnp_ndo_bridge_setlink(struct net_device *dev, struct nlmsghdr *nlh,
				  __always_unused u16 flags,
				  struct netlink_ext_ack __always_unused *ext)
{
	struct rnp_adapter *adapter = netdev_priv(dev);
	struct rnp_hw *hw = &adapter->hw;
	struct nlattr *attr, *br_spec;
	int rem;

	if (!(adapter->flags & RNP_FLAG_SRIOV_ENABLED))
		return -EOPNOTSUPP;

	br_spec = nlmsg_find_attr(nlh, sizeof(struct ifinfomsg), IFLA_AF_SPEC);

	nla_for_each_nested(attr, br_spec, rem) {
		__u16 mode;

		if (nla_type(attr) != IFLA_BRIDGE_MODE)
			continue;

		mode = nla_get_u16(attr);
		if (mode == BRIDGE_MODE_VEPA) {
			adapter->flags2 &= ~RNP_FLAG2_BRIDGE_MODE_VEB;
			wr32(hw, RNP_DMA_CONFIG,
			     rd32(hw, RNP_DMA_CONFIG) | DMA_VEB_BYPASS);
		} else if (mode == BRIDGE_MODE_VEB) {
			adapter->flags2 |= RNP_FLAG2_BRIDGE_MODE_VEB;
			wr32(hw, RNP_DMA_CONFIG,
			     rd32(hw, RNP_DMA_CONFIG) & (~DMA_VEB_BYPASS));

		} else
			return -EINVAL;

		e_info(drv, "enabling bridge mode: %s\n",
		       mode == BRIDGE_MODE_VEPA ? "VEPA" : "VEB");
	}

	return 0;
}

static int rnp_ndo_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq,
				  struct net_device *dev,
				  u32 __maybe_unused filter_mask, int nlflags)
{
	struct rnp_adapter *adapter = netdev_priv(dev);
	u16 mode;

	if (!(adapter->flags & RNP_FLAG_SRIOV_ENABLED))
		return 0;

	if (adapter->flags2 & RNP_FLAG2_BRIDGE_MODE_VEB)
		mode = BRIDGE_MODE_VEB;
	else
		mode = BRIDGE_MODE_VEPA;

	return ndo_dflt_bridge_getlink(skb, pid, seq, dev, mode, 0, 0, nlflags,
				       filter_mask, NULL);
}

#define RNP_MAX_TUNNEL_HDR_LEN 80
#define RNP_MAX_MAC_HDR_LEN 127
#define RNP_MAX_NETWORK_HDR_LEN 511

static netdev_features_t rnp_features_check(struct sk_buff *skb,
					    struct net_device *dev,
					    netdev_features_t features)
{
	unsigned int network_hdr_len, mac_hdr_len;

	/* Make certain the headers can be described by a context descriptor */
	mac_hdr_len = skb_network_header(skb) - skb->data;
	if (unlikely(mac_hdr_len > RNP_MAX_MAC_HDR_LEN))
		return features &
		       ~(NETIF_F_HW_CSUM | NETIF_F_SCTP_CRC |
			 NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_TSO | NETIF_F_TSO6);

	network_hdr_len = skb_checksum_start(skb) - skb_network_header(skb);
	if (unlikely(network_hdr_len > RNP_MAX_NETWORK_HDR_LEN))
		return features & ~(NETIF_F_HW_CSUM | NETIF_F_SCTP_CRC |
				    NETIF_F_TSO | NETIF_F_TSO6);

	/* We can only support IPV4 TSO in tunnels if we can mangle the
	 * inner IP ID field, so strip TSO if MANGLEID is not supported.
	 */
	if (skb->encapsulation && !(features & NETIF_F_TSO_MANGLEID))
		features &= ~NETIF_F_TSO;

	return features;
}

static void rnp_clear_udp_tunnel_port(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;

	if (!(adapter->flags & (RNP_FLAG_VXLAN_OFFLOAD_CAPABLE)))
		return;

	adapter->vxlan_port = 0;
	hw->ops.set_vxlan_port(hw, adapter->vxlan_port);
}

/**
 * rnp_add_udp_tunnel_port - Get notifications about adding UDP tunnel ports
 * @dev: The port's netdev
 * @ti: Tunnel endpoint information
 **/
__maybe_unused static void rnp_add_udp_tunnel_port(struct net_device *dev,
						   struct udp_tunnel_info *ti)
{
	struct rnp_adapter *adapter = netdev_priv(dev);
	struct rnp_hw *hw = &adapter->hw;
	__be16 port = ti->port;

	if (ti->sa_family != AF_INET)
		return;

	switch (ti->type) {
	case UDP_TUNNEL_TYPE_VXLAN:
		if (!(adapter->flags & RNP_FLAG_VXLAN_OFFLOAD_CAPABLE))
			return;

		if (adapter->vxlan_port == port)
			return;

		if (adapter->vxlan_port) {
			netdev_info(dev,
				    "VXLAN port %d set, not adding port %d\n",
				    ntohs(adapter->vxlan_port), ntohs(port));
			return;
		}

		adapter->vxlan_port = port;
		break;
	default:
		return;
	}
	hw->ops.set_vxlan_port(hw, ntohs(adapter->vxlan_port));
}

/**
 * rnp_del_udp_tunnel_port - Get notifications about removing UDP tunnel ports
 * @dev: The port's netdev
 * @ti: Tunnel endpoint information
 **/
__maybe_unused static void rnp_del_udp_tunnel_port(struct net_device *dev,
						   struct udp_tunnel_info *ti)
{
	struct rnp_adapter *adapter = netdev_priv(dev);

	if (ti->type != UDP_TUNNEL_TYPE_VXLAN)
		return;

	if (ti->sa_family != AF_INET)
		return;

	switch (ti->type) {
	case UDP_TUNNEL_TYPE_VXLAN:
		if (!(adapter->flags & RNP_FLAG_VXLAN_OFFLOAD_CAPABLE))
			return;

		if (adapter->vxlan_port != ti->port) {
			netdev_info(dev, "VXLAN port %d not found\n",
				    ntohs(ti->port));
			return;
		}

		break;
	default:
		return;
	}

	rnp_clear_udp_tunnel_port(adapter);
	adapter->flags2 |= RNP_FLAG2_UDP_TUN_REREG_NEEDED;
}

const struct net_device_ops rnp10_netdev_ops = {
	.ndo_open = rnp_open,
	.ndo_stop = rnp_close,
	.ndo_start_xmit = rnp_xmit_frame,
	.ndo_set_rx_mode = rnp_set_rx_mode,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_eth_ioctl = rnp_ioctl,
	.ndo_change_mtu = rnp_change_mtu,
	.ndo_get_stats64 = rnp_get_stats64,
	.ndo_tx_timeout = rnp_tx_timeout,
	.ndo_set_tx_maxrate = rnp_tx_maxrate,
	.ndo_set_mac_address = rnp_set_mac,
	.ndo_vlan_rx_add_vid = rnp_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = rnp_vlan_rx_kill_vid,
	.ndo_set_vf_mac = rnp_ndo_set_vf_mac,
	.ndo_set_vf_vlan = rnp_ndo_set_vf_vlan,
	.ndo_set_vf_rate = rnp_ndo_set_vf_bw,
	.ndo_set_vf_spoofchk = rnp_ndo_set_vf_spoofchk,
	.ndo_set_vf_link_state = rnp_ndo_set_vf_link_state,
	.ndo_set_vf_trust = rnp_ndo_set_vf_trust,
	.ndo_get_vf_config = rnp_ndo_get_vf_config,
	.ndo_setup_tc = __rnp_setup_tc,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller = rnp_netpoll,
#endif
	.ndo_bridge_setlink = rnp_ndo_bridge_setlink,
	.ndo_bridge_getlink = rnp_ndo_bridge_getlink,
	.ndo_features_check = rnp_features_check,
	.ndo_set_features = rnp_set_features,
	.ndo_fix_features = rnp_fix_features,
};

static void rnp_assign_netdev_ops(struct net_device *dev)
{
	/* different hw can assign difference fun */
	dev->netdev_ops = &rnp10_netdev_ops;
	rnp_set_ethtool_ops(dev);
	dev->watchdog_timeo = 5 * HZ;
}

/**
 * rnp_wol_supported - Check whether device supports WoL
 * @hw: hw specific details
 * @device_id: the device ID
 * @subdev_id: the subsystem device ID
 *
 * This function is used by probe and ethtool to determine
 * which devices have WoL support
 *
 **/
int rnp_wol_supported(struct rnp_adapter *adapter, u16 device_id,
		      u16 subdevice_id)
{
	int is_wol_supported = 0;

	struct rnp_hw *hw = &adapter->hw;

	if (hw->wol_supported)
		is_wol_supported = 1;
	return is_wol_supported;
}

static inline unsigned long rnp_tso_features(struct rnp_hw *hw)
{
	unsigned long features = 0;

	if (hw->feature_flags & RNP_NET_FEATURE_TSO)
		features |= NETIF_F_TSO;
	if (hw->feature_flags & RNP_NET_FEATURE_TSO)
		features |= NETIF_F_TSO6;
	features |= NETIF_F_GSO_PARTIAL;
	if (hw->feature_flags & RNP_NET_FEATURE_TX_UDP_TUNNEL)
		features |= RNP_GSO_PARTIAL_FEATURES;

	return features;
}

static void remove_mbx_irq(struct rnp_adapter *adapter)
{
	/* mbx */
	if (adapter->num_other_vectors) {
		/* only msix use indepented intr */
		if (adapter->flags & RNP_FLAG_MSIX_ENABLED) {
			adapter->hw.mbx.ops.configure(
				&adapter->hw, adapter->msix_entries[0].entry,
				false);
			free_irq(adapter->msix_entries[0].vector, adapter);

			adapter->hw.mbx.other_irq_enabled = false;
		}
	}
}

static int register_mbx_irq(struct rnp_adapter *adapter)
{
	struct rnp_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	int err = 0;

	/* for mbx:vector0 */
	if (adapter->num_other_vectors) {
		/* only do this in msix mode */
		if (adapter->flags & RNP_FLAG_MSIX_ENABLED) {
			err = request_irq(adapter->msix_entries[0].vector,
					  rnp_msix_other, 0, netdev->name,
					  adapter);
			if (err) {
				e_err(probe,
				      "request_irq for msix_other failed: %d\n",
				      err);
				goto err_mbx;
			}
			hw->mbx.ops.configure(
				hw, adapter->msix_entries[0].entry, true);
			adapter->hw.mbx.other_irq_enabled = true;
		}
	}

err_mbx:
	return err;
}

static int rnp_rm_adpater(struct rnp_adapter *adapter)
{
	struct net_device *netdev;
	struct rnp_hw *hw = &adapter->hw;

	netdev = adapter->netdev;
	pr_info("= remove adapter:%s =\n", netdev->name);

	rnp_dbg_adapter_exit(adapter);

	netif_carrier_off(netdev);

	set_bit(__RNP_DOWN, &adapter->state);
	set_bit(__RNP_REMOVE, &adapter->state);
	if (module_enable_ptp) {
		while (test_bit(__RNP_PTP_TX_IN_PROGRESS, &adapter->state)) {
			usleep_range(10000, 20000);
		}
		cancel_work_sync(&adapter->tx_hwtstamp_work);
	}
	cancel_work_sync(&adapter->service_task);

	del_timer_sync(&adapter->service_timer);
	rnp_sysfs_exit(adapter);
	rnp_fdir_filter_exit(adapter);
	adapter->priv_flags &= (~RNP_PRIV_FLAG_TCP_SYNC);

	if (adapter->rpu_inited) {
		rnp_rpu_mpe_stop(adapter);
		adapter->rpu_inited = 0;
	}

	if (netdev->reg_state == NETREG_REGISTERED)
		unregister_netdev(netdev);

	adapter->netdev = NULL;

	if (hw->ops.driver_status)
		hw->ops.driver_status(hw, false, rnp_driver_insmod);

	remove_mbx_irq(adapter);

	rnp_clear_interrupt_scheme(adapter);

	if (hw->ncsi_en) {
		rnp_mbx_probe_stat_set(hw, MBX_REMOVE);
	}

	if (adapter->io_addr)
		iounmap(adapter->io_addr);

	if (adapter->io_addr_bar0)
		iounmap(adapter->io_addr_bar0);

	free_netdev(netdev);

	pr_info("remove complete\n");

	return 0;
}

static void rnp_fix_dma_tx_status(struct rnp_adapter *adapter)
{
	int i;
	struct rnp_hw *hw = &adapter->hw;
	struct rnp_dma_info *dma = &hw->dma;

	if ((hw->hw_type == rnp_hw_n10) || (hw->hw_type == rnp_hw_n400)) {
		for (i = 0; i < dma->max_tx_queues; i++)
			dma_ring_wr32(dma, RING_OFFSET(i) + RNP_DMA_TX_START,
				      1);
	}
}

static u8 rnp10_pfnum(u8 __iomem *hw_addr_bar0, struct pci_dev *pdev)
{
	/* n10 read this from bar0 */
	u16 vf_num = -1;
	u32 pfvfnum_reg;
#define PF_NUM_REG_N10 (0x75f000)
	pfvfnum_reg = (PF_NUM_REG_N10 & (pci_resource_len(pdev, 0) - 1));
	vf_num = readl(hw_addr_bar0 + pfvfnum_reg);
#define VF_NUM_MASK_TEMP (0x400)
#define VF_NUM_OFF (4)
	return ((vf_num & VF_NUM_MASK_TEMP) >> VF_NUM_OFF);
}

static int rnp_can_rpu_start(struct rnp_adapter *adapter)
{
	if (adapter->hw.rpu_addr == NULL)
		return 0;
	if ((adapter->pdev->device & 0xff00) == 0x1c00) {
		return 1;
	}
	if (adapter->hw.rpu_availble) {
		return 1;
	}
	return 0;
}

static int rnp_add_adpater(struct pci_dev *pdev, struct rnp_info *ii,
			   struct rnp_adapter **padapter)
{
	int i, err = 0;
	struct rnp_adapter *adapter = NULL;
	struct net_device *netdev;
	struct rnp_hw *hw;
	u8 __iomem *hw_addr = NULL;
	u8 __iomem *hw_addr_bar0 = NULL;

	u32 dma_version = 0;
	u32 nic_version = 0;
	u32 queues = ii->total_queue_pair_cnts;
	static int bd_number;

	pr_info("====  add adapter queues:%d ====", queues);
	netdev = alloc_etherdev_mq(sizeof(struct rnp_adapter), queues);
	if (!netdev)
		return -ENOMEM;

	if (!fix_eth_name)
		SET_NETDEV_DEV(netdev, &pdev->dev);

	adapter = netdev_priv(netdev);

	memset((char *)adapter, 0x00, sizeof(struct rnp_adapter));
	adapter->netdev = netdev;
	adapter->pdev = pdev;

	adapter->max_ring_pair_counts = queues;
	if (padapter)
		*padapter = adapter;

	adapter->bd_number = bd_number++;
	adapter->port = 0;
	snprintf(adapter->name, sizeof(netdev->name), "%s%d%d", rnp_driver_name,
		 1, adapter->bd_number);
	pci_set_drvdata(pdev, adapter);

	hw = &adapter->hw;
	hw->back = adapter;
	/* first setup hw type */
	hw->rss_type = ii->rss_type;
	hw->hw_type = ii->hw_type;
	switch (hw->hw_type) {
	case rnp_hw_n10:
	case rnp_hw_n20:
	case rnp_hw_n400:
	case rnp_hw_uv440:
		hw_addr_bar0 = ioremap(pci_resource_start(pdev, 0),
				       pci_resource_len(pdev, 0));
		if (!hw_addr_bar0) {
			dev_err(&pdev->dev, "pcim_iomap bar%d failed!\n", 0);
			return -EIO;
		}
#ifdef FIX_VF_BUG
		rnp_wr_reg(hw_addr_bar0 +
				   (0x7982fc & (pci_resource_len(pdev, 0) - 1)),
			   0);
#endif

		/* n10 use bar4 */
#define RNP_NIC_BAR_N10 4
		hw_addr = ioremap(pci_resource_start(pdev, RNP_NIC_BAR_N10),
				  pci_resource_len(pdev, RNP_NIC_BAR_N10));
		if (!hw_addr) {
			dev_err(&pdev->dev, "pcim_iomap bar%d failed!\n",
				RNP_NIC_BAR_N10);
			return -EIO;
		}
		pr_info("[bar%d]:%p %llx len=%d MB\n", RNP_NIC_BAR_N10, hw_addr,
			(unsigned long long)pci_resource_start(pdev,
							       RNP_NIC_BAR_N10),
			(int)pci_resource_len(pdev, RNP_NIC_BAR_N10) / 1024 /
				1024);
		/* get dma version */
		dma_version = rnp_rd_reg(hw_addr);

		if (rnp10_pfnum(hw_addr_bar0, pdev))
			hw->pfvfnum = PF_NUM(1);
		else
			hw->pfvfnum = PF_NUM(0);

#ifdef FIX_VF_BUG
		if (hw->pfvfnum)
			hw->hw_addr = hw_addr + 0x100000;
		else
			hw->hw_addr = hw_addr;
#else
		hw->hw_addr = hw_addr;
#endif
		/* setup msix base */
#ifdef FIX_VF_BUG
		if (hw->pfvfnum)
			hw->ring_msix_base = hw->hw_addr + 0xa4000 + 0x200;
		else
			hw->ring_msix_base = hw->hw_addr + 0xa4000;
#else
		hw->ring_msix_base = hw->hw_addr + 0xa4000;
#endif
		nic_version = rd32(hw, RNP_TOP_NIC_VERSION);
		adapter->irq_mode = irq_mode_msix;
		adapter->flags |= RNP_FLAG_MSIX_CAPABLE;

		break;
	default:
#ifdef FIX_VF_BUG
		hw_addr_bar0 = ioremap(pci_resource_start(pdev, 0),
				       pci_resource_len(pdev, 0));
#endif
		hw_addr = ioremap(pci_resource_start(pdev, 0),
				  pci_resource_len(pdev, 0));
		goto err_free_net;
		break;
	}

	/* setup FT_PADDING */
	{
#ifdef FT_PADDING
		u32 data;

		data = rnp_rd_reg(hw->hw_addr + RNP_DMA_CONFIG);
		SET_BIT(8, data);
		rnp_wr_reg(hw->hw_addr + RNP_DMA_CONFIG, data);
		adapter->priv_flags |= RNP_PRIV_FLAG_FT_PADDING;
#endif
	}

	/* assign to adapter */
	adapter->io_addr = hw_addr;
	adapter->io_addr_bar0 = hw_addr_bar0;
	if (pci_resource_len(pdev, 0) == (8 * 1024 * 1024)) {
		hw->rpu_addr = hw_addr_bar0;
	}

	hw->pdev = pdev;
	hw->dma_version = dma_version;
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

	/* we have other irq */
	adapter->num_other_vectors = 1;
	/* get software info */
	ii->get_invariants(hw);

	spin_lock_init(&adapter->link_stat_lock);

	if (adapter->num_other_vectors) {
		/* Mailbox */
		rnp_init_mbx_params_pf(hw);
		memcpy(&hw->mbx.ops, ii->mbx_ops, sizeof(hw->mbx.ops));
		if (dma_version >= 0x20210111) {
			rnp_mbx_link_event_enable(hw, 0);
			if ((hw->hw_type == rnp_hw_n10) ||
			    (hw->hw_type == rnp_hw_n400))
				rnp_mbx_force_speed(hw, 0);
			if (rnp_mbx_get_capability(hw, ii)) {
				dev_err(&pdev->dev,
					"rnp_mbx_get_capability failed!\n");
				err = -EIO;
				goto err_free_net;
			}

			/* should check eco */
#ifdef VF_PROMISC_SUPPORT
			if (!hw->eco) {
				dev_err(&pdev->dev,
					"only v2 chips support vf promisc!\n");
				err = -EIO;
				goto err_free_net;

			}
#endif
			adapter->portid_of_card = hw->port_id[0];
			if (hw->eco) {
				hw->eth.num_rar_entries -= 1;
				hw->mac.num_rar_entries -= 1;
				hw->num_rar_entries -= 1;
			}

			adapter->portid_of_card = hw->pfvfnum ? 1 : 0;
			adapter->wol = hw->wol;
		}
	}
	if (hw->ncsi_en) {
		hw->eth.num_rar_entries -= hw->ncsi_rar_entries;
		hw->mac.num_rar_entries -= hw->ncsi_rar_entries;
		hw->num_rar_entries -= hw->ncsi_rar_entries;
	} 

	if (hw->force_status)
		adapter->priv_flags |= RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE;
	else
		adapter->priv_flags &= (~RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE);
	hw->default_rx_queue = 0;
	pr_info("%s %s: dma version:0x%x, nic version:0x%x, pfvfnum:0x%x\n",
		adapter->name, pci_name(pdev), hw->dma_version, nic_version,
		hw->pfvfnum);

	/* Setup hw api */
	hw->mac.type = ii->mac;
	/* EEPROM */
	if (ii->eeprom_ops)
		memcpy(&hw->eeprom.ops, ii->eeprom_ops, sizeof(hw->eeprom.ops));

	hw->phy.sfp_type = rnp_sfp_type_unknown;

	hw->ops.setup_ethtool(netdev);
	rnp_assign_netdev_ops(netdev);
	rnp_check_options(adapter);
	/* setup the private structure */
	/* this private is used only once
	 */
	err = rnp_sw_init(adapter);
	if (err)
		goto err_sw_init;

	err = hw->ops.reset_hw(hw);
	hw->phy.reset_if_overtemp = false;
	if (err) {
		e_dev_err("HW Init failed: %d\n", err);
		goto err_sw_init;
	}
	if (hw->ops.driver_status)
		hw->ops.driver_status(hw, true, rnp_driver_insmod);
	if (hw->ops.driver_status) {
		hw->ops.driver_status(hw, !!(adapter->priv_flags & RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE),
				rnp_driver_force_control_mac);
	}

#ifdef CONFIG_PCI_IOV
	if (adapter->num_other_vectors) {
		rnp_enable_sriov(adapter);
		pci_sriov_set_totalvfs(pdev, hw->max_vfs - 1);
	}
#endif

	/* MTU range: 68 - 9710 */
	netdev->min_mtu = hw->min_length;
	netdev->max_mtu = hw->max_length - (ETH_HLEN + 2 * ETH_FCS_LEN);

	if (hw->feature_flags & RNP_NET_FEATURE_SG)
		netdev->features |= NETIF_F_SG;
	if (hw->feature_flags & RNP_NET_FEATURE_TSO)
		netdev->features |= NETIF_F_TSO | NETIF_F_TSO6;
	if (hw->feature_flags & RNP_NET_FEATURE_RX_HASH)
		netdev->features |= NETIF_F_RXHASH;
	if (hw->feature_flags & RNP_NET_FEATURE_RX_CHECKSUM)
		netdev->features |= NETIF_F_RXCSUM;
	if (hw->feature_flags & RNP_NET_FEATURE_TX_CHECKSUM)
		netdev->features |= NETIF_F_HW_CSUM | NETIF_F_SCTP_CRC;

	if (hw->feature_flags & RNP_NET_FEATURE_USO)
		netdev->features |= NETIF_F_GSO_UDP_L4;

	netdev->features |= NETIF_F_HIGHDMA;

	if (hw->feature_flags & RNP_NET_FEATURE_TX_UDP_TUNNEL) {
		netdev->gso_partial_features = RNP_GSO_PARTIAL_FEATURES;
		netdev->features |= NETIF_F_GSO_PARTIAL |
				    RNP_GSO_PARTIAL_FEATURES;
	}

	netdev->hw_features |= netdev->features;

	if (hw->ncsi_en) {
		hw->feature_flags &= ~RNP_NET_FEATURE_VLAN_OFFLOAD;
	}
	if (hw->feature_flags & RNP_NET_FEATURE_VLAN_FILTER)
		netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_FILTER;
	if (hw->feature_flags & RNP_NET_FEATURE_STAG_FILTER)
		netdev->hw_features |= NETIF_F_HW_VLAN_STAG_FILTER;
	if (hw->feature_flags & RNP_NET_FEATURE_VLAN_OFFLOAD) {
		netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_RX |
				       NETIF_F_HW_VLAN_CTAG_TX;
	}
	if (hw->feature_flags & RNP_NET_FEATURE_STAG_OFFLOAD) {
		netdev->hw_features |= NETIF_F_HW_VLAN_STAG_RX |
				       NETIF_F_HW_VLAN_STAG_TX;
	}
	netdev->hw_features |= NETIF_F_RXALL;
	if (hw->feature_flags & RNP_NET_FEATURE_RX_NTUPLE_FILTER)
		netdev->hw_features |= NETIF_F_NTUPLE;
	if (hw->feature_flags & RNP_NET_FEATURE_RX_FCS)
		netdev->hw_features |= NETIF_F_RXFCS;
	if (hw->feature_flags & RNP_NET_FEATURE_HW_TC)
		netdev->hw_features |= NETIF_F_HW_TC;

	netdev->vlan_features |= netdev->features | NETIF_F_TSO_MANGLEID;
	netdev->hw_enc_features |= netdev->vlan_features;
	netdev->mpls_features |= NETIF_F_HW_CSUM;

	if (hw->feature_flags & RNP_NET_FEATURE_VLAN_FILTER)
		netdev->features |= NETIF_F_HW_VLAN_CTAG_FILTER;
	if (hw->feature_flags & RNP_NET_FEATURE_STAG_FILTER)
		netdev->features |= NETIF_F_HW_VLAN_STAG_FILTER;
	if (hw->feature_flags & RNP_NET_FEATURE_VLAN_OFFLOAD) {
		netdev->features |= NETIF_F_HW_VLAN_CTAG_RX |
				    NETIF_F_HW_VLAN_CTAG_TX;
	}
	if (hw->feature_flags & RNP_NET_FEATURE_STAG_OFFLOAD) {
		netdev->features |= NETIF_F_HW_VLAN_STAG_RX |
				    NETIF_F_HW_VLAN_STAG_TX;
	}

	netdev->priv_flags |= IFF_UNICAST_FLT;
	netdev->priv_flags |= IFF_SUPP_NOFCS;

	if (adapter->flags2 & RNP_FLAG2_RSC_CAPABLE)
		netdev->hw_features |= NETIF_F_LRO;

	netdev->priv_flags |= IFF_UNICAST_FLT;
	netdev->priv_flags |= IFF_SUPP_NOFCS;

#if IS_ENABLED(CONFIG_DCB)
	rnp_dcb_init(netdev, adapter);
#endif

	if (adapter->flags2 & RNP_FLAG2_RSC_ENABLED)
		netdev->features |= NETIF_F_LRO;

	eth_hw_addr_set(netdev, hw->mac.perm_addr);
	memcpy(netdev->perm_addr, hw->mac.perm_addr, netdev->addr_len);
	pr_info("dev mac:%pM \n", netdev->dev_addr);

	if (!is_valid_ether_addr(netdev->dev_addr)) {
		e_dev_err("invalid MAC address\n");
		err = -EIO;
		goto err_sw_init;
	}
	ether_addr_copy(hw->mac.addr, hw->mac.perm_addr);

	timer_setup(&adapter->service_timer, rnp_service_timer, 0);

	if (module_enable_ptp) {
		/* setup ptp_addr according to mac type */
		switch (adapter->hw.mac.mac_type) {
		case mac_dwc_xlg:
			adapter->ptp_addr = adapter->hw.mac.mac_addr + 0xd00;
			adapter->gmac4 = 1;
			break;
		case mac_dwc_g:
			adapter->ptp_addr = adapter->hw.mac.mac_addr + 0x700;
			adapter->gmac4 = 0;
			break;
		}
		adapter->flags2 |= RNP_FLAG2_PTP_ENABLED;
		if (adapter->flags2 & RNP_FLAG2_PTP_ENABLED) {
			adapter->tx_timeout_factor = 10;
			INIT_WORK(&adapter->tx_hwtstamp_work,
				  rnp_tx_hwtstamp_work);
		}
	}

	INIT_WORK(&adapter->service_task, rnp_service_task);
	clear_bit(__RNP_SERVICE_SCHED, &adapter->state);

	if (fix_eth_name)
		strncpy(netdev->name, adapter->name, sizeof(netdev->name) - 1);
	else {
		strscpy(netdev->name, pci_name(pdev), sizeof(netdev->name));
	}

	err = rnp_init_interrupt_scheme(adapter);
	if (err)
		goto err_sw_init;

	err = register_mbx_irq(adapter);
	if (err)
		goto err_register;

#ifdef CONFIG_PCI_IOV
	rnp_enable_sriov_true(adapter);
#endif

	/* WOL not supported for all devices */
	{
		struct ethtool_wolinfo wol;

		if (rnp_wol_exclusion(adapter, &wol) ||
		    !device_can_wakeup(&adapter->pdev->dev))
			adapter->wol = 0;
	}
	/* reset the hardware with the new settings */
	err = hw->ops.start_hw(hw);
	rnp_fix_dma_tx_status(adapter);

	if (!fix_eth_name)
		strscpy(netdev->name, "eth%d", sizeof(netdev->name));
	err = register_netdev(netdev);
	if (err) {
		e_dev_err("register_netdev failed!\n");
		goto err_register;
	}

	/* power down the optics for n10 SFP+ fiber */
	if (hw->ops.disable_tx_laser)
		hw->ops.disable_tx_laser(hw);

	/* carrier off reporting is important to ethtool even BEFORE open */
	netif_carrier_off(netdev);

	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED) {
		DPRINTK(PROBE, INFO, "IOV is enabled with %d VFs\n",
			adapter->num_vfs);
		for (i = 0; i < adapter->num_vfs; i++)
			rnp_vf_configuration(pdev, (i | 0x10000000));
	}

	if (rnp_mbx_lldp_status_get(hw) == 1) {
		adapter->priv_flags |= RNP_PRIV_FLAG_LLDP_EN_STAT;
	}

	if (rnp_sysfs_init(adapter))
		e_err(probe, "failed to allocate sysfs resources\n");

	rnp_dbg_adapter_init(adapter);
	/* only pf0 download mpe */
	if (rnp_is_pf0(&adapter->hw) && rnp_can_rpu_start(adapter)) {
		rnp_rpu_mpe_start(adapter);
	}

	if (hw->ncsi_en) {
		hw->ops.set_mac_rx(hw, true);
		rnp_mbx_probe_stat_set(hw, MBX_PROBE);
	}

	return 0;
err_register:
	remove_mbx_irq(adapter);
	rnp_clear_interrupt_scheme(adapter);
err_sw_init:
	rnp_disable_sriov(adapter);
	adapter->flags2 &= ~RNP_FLAG2_SEARCH_FOR_SFP;
err_free_net:
	free_netdev(netdev);
	return err;
}

/**
 * rnp_probe - Device Initialization Routine
 * @pdev: PCI device information struct
 * @ent: entry in rnp_pci_tbl
 *
 * Returns 0 on success, negative on failure
 *
 * rnp_probe initializes an adapter identified by a pci_dev structure.
 * The OS initialization, configuring of the adapter private structure,
 * and a hardware reset occur.
 **/
static int rnp_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct rnp_adapter *adapter;
	struct rnp_info *ii = rnp_info_tbl[id->driver_data];
	int err;

	/* Catch broken hardware that put the wrong VF device ID in
	 * the PCIe SR-IOV capability.
	 */
	if (pdev->is_virtfn) {
		WARN(1, "%s (%hx:%hx) should not be a VF!\n", pci_name(pdev),
		     pdev->vendor, pdev->device);
		return -EINVAL;
	}
	/* not support bus reset*/
	pdev->dev_flags |= PCI_DEV_FLAGS_NO_BUS_RESET;
	err = pci_enable_device_mem(pdev);
	if (err)
		return err;

	if (!dma_set_mask(&pdev->dev, DMA_BIT_MASK(56)) &&
	    !dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(56))) {
		enable_hi_dma = 1;
	} else {
		err = dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));
		if (err) {
			err = dma_set_coherent_mask(&pdev->dev,
						    DMA_BIT_MASK(32));
			if (err) {
				dev_err(&pdev->dev,
					"No usable DMA configuration, aborting\n");
				goto err_dma;
			}
		}
		enable_hi_dma = 0;
	}

	err = pci_request_mem_regions(pdev, rnp_driver_name);
	if (err) {
		dev_err(&pdev->dev,
			"pci_request_selected_regions failed 0x%x\n", err);
		goto err_pci_reg;
	}
	pci_set_master(pdev);
	pci_save_state(pdev);

	err = rnp_add_adpater(pdev, ii, &adapter);
	if (err)
		goto err_regions;

	return 0;
err_regions:
	pci_release_mem_regions(pdev);
err_dma:
err_pci_reg:
	return err;
}

/**
 * rnp_remove - Device Removal Routine
 * @pdev: PCI device information struct
 *
 * rnp_remove is called by the PCI subsystem to alert the driver
 * that it should release a PCI device.  The could be caused by a
 * Hot-Plug event, or because the driver is going to be removed from
 * memory.
 **/
static void rnp_remove(struct pci_dev *pdev)
{
	struct rnp_adapter *adapter = pci_get_drvdata(pdev);

#ifdef CONFIG_PCI_IOV
	/*
	 * Only disable SR-IOV on unload if the user specified the now
	 * deprecated max_vfs module parameter.
	 */
	rnp_disable_sriov(adapter);
#endif
	rnp_rm_adpater(adapter);

	pci_release_mem_regions(pdev);
	pci_disable_device(pdev);
}

static struct pci_driver rnp_driver = {
	.name = rnp_driver_name,
	.id_table = rnp_pci_tbl,
	.probe = rnp_probe,
	.remove = rnp_remove,
#ifdef CONFIG_PM
	.suspend = rnp_suspend,
	.resume = rnp_resume,
#endif
	.shutdown = rnp_shutdown,
	.sriov_configure = rnp_pci_sriov_configure,
};

static int __init rnp_init_module(void)
{
	int ret;

	pr_info("%s - version %s\n", rnp_driver_string, rnp_driver_version);
	pr_info("%s \n", rnp_copyright);
	rnp_wq = create_singlethread_workqueue(rnp_driver_name);

	if (!rnp_wq) {
		pr_err("%s: Failed to create workqueue\n", rnp_driver_name);
		return -ENOMEM;
	}

	rnp_dbg_init();

	ret = pci_register_driver(&rnp_driver);
	if (ret) {
		destroy_workqueue(rnp_wq);
		rnp_dbg_exit();
		return ret;
	}

	return 0;
}
module_init(rnp_init_module);

static void __exit rnp_exit_module(void)
{
	pci_unregister_driver(&rnp_driver);

	destroy_workqueue(rnp_wq);

	rnp_dbg_exit();

	rcu_barrier(); /* Wait for completion of call_rcu()'s */
}

module_exit(rnp_exit_module);
