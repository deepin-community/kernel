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
#include <linux/fs.h>
#include <linux/file.h>
#include <net/xdp_sock_drv.h>
#include <net/vxlan.h>

#include "rnpgbe_common.h"
#include "rnpgbe.h"
#include "rnpgbe_sriov.h"
#include "rnpgbe_ptp.h"
#include "rnpgbe_ethtool.h"


/* for test only */
#ifdef CONFIG_ARM64
#define NO_BQL_TEST
#endif

char rnpgbe_driver_name[] = "rnpgbe";
static const char rnpgbe_driver_string[] =
	"mucse 1 Gigabit PCI Express Network Driver";
#define DRV_VERSION "0.2.3-rc3"
static u32 driver_version = 0x00020303;
#include "version.h"

const char rnpgbe_driver_version[] = DRV_VERSION;
static const char rnpgbe_copyright[] =
	"Copyright (c) 2020-2024 mucse Corporation.";

static struct rnpgbe_info *rnpgbe_info_tbl[] = {
	[board_n500] = &rnpgbe_n500_info,
	[board_n210] = &rnpgbe_n210_info,
};

static int register_mbx_irq(struct rnpgbe_adapter *adapter);
static void remove_mbx_irq(struct rnpgbe_adapter *adapter);

static void rnpgbe_pull_tail(struct sk_buff *skb);
#ifdef OPTM_WITH_LPAGE
static bool rnpgbe_alloc_mapped_page(struct rnpgbe_ring *rx_ring,
				     struct rnpgbe_rx_buffer *bi,
				     union rnpgbe_rx_desc *rx_desc, u16 bufsz,
				     u64 fun_id);
static void rnpgbe_put_rx_buffer(struct rnpgbe_ring *rx_ring,
				 struct rnpgbe_rx_buffer *rx_buffer);
#else /* OPTM_WITH_LPAGE */
static bool rnpgbe_alloc_mapped_page(struct rnpgbe_ring *rx_ring,
				     struct rnpgbe_rx_buffer *bi);
static void rnpgbe_put_rx_buffer(struct rnpgbe_ring *rx_ring,
				 struct rnpgbe_rx_buffer *rx_buffer,
				 struct sk_buff *skb);
#endif /* OPTM_WITH_LPAGE */

static struct pci_device_id rnpgbe_pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, PCI_DEVICE_ID_N500_QUAD_PORT),
	  .driver_data = board_n500 }, /* n500 */
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, PCI_DEVICE_ID_N500_DUAL_PORT),
	  .driver_data = board_n500 }, /* n500 */
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, PCI_DEVICE_ID_N210),
	  .driver_data = board_n210 }, /* n210 */
	/* required last entry */
	{
		0,
	},
};
MODULE_DEVICE_TABLE(pci, rnpgbe_pci_tbl);

#define DEFAULT_MSG_ENABLE (NETIF_MSG_DRV | NETIF_MSG_PROBE | NETIF_MSG_LINK)
static int debug = -1;
module_param(debug, int, 0000);
MODULE_PARM_DESC(debug, "Debug level (0=none,...,16=all)");

static unsigned int fix_eth_name;
module_param(fix_eth_name, uint, 0000);
MODULE_PARM_DESC(fix_eth_name, "set eth adapter name to rnpgbeXX");

static unsigned int module_enable_ptp = 1;
module_param(module_enable_ptp, uint, 0000);
MODULE_PARM_DESC(module_enable_ptp, "enable ptp, disabled default");

MODULE_AUTHOR("Mucse Corporation, <mucse@mucse.com>");
MODULE_DESCRIPTION("Mucse(R) 1 Gigabit PCI Express Network Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

static struct workqueue_struct *rnpgbe_wq;

static int enable_hi_dma;

#define RNP_LPI_T(x) (jiffies + msecs_to_jiffies(x))

static void rnpgbe_service_timer(struct timer_list *t);
static void rnpgbe_setup_eee_mode(struct rnpgbe_adapter *adapter, bool status);

static void rnpgbe_service_event_schedule(struct rnpgbe_adapter *adapter)
{
	if (!test_bit(__RNP_DOWN, &adapter->state) &&
	    !test_and_set_bit(__RNP_SERVICE_SCHED, &adapter->state))
		queue_work(rnpgbe_wq, &adapter->service_task);
}

static void rnpgbe_service_event_complete(struct rnpgbe_adapter *adapter)
{
	BUG_ON(!test_bit(__RNP_SERVICE_SCHED, &adapter->state));

	/* flush memory to make sure state is correct before next watchdog */
	smp_mb__before_atomic();
	clear_bit(__RNP_SERVICE_SCHED, &adapter->state);
}

/**
 * rnpgbe_set_ring_vector - set the ring_vector registers,
 * mapping interrupt causes to vectors
 * @adapter: pointer to adapter struct
 * @queue: queue to map the corresponding interrupt to
 * @msix_vector: the vector to map to the corresponding queue
 *
 */
static void rnpgbe_set_ring_vector(struct rnpgbe_adapter *adapter,
				   u8 rnpgbe_queue, u8 rnpgbe_msix_vector)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	u32 data = 0;

	data = hw->pfvfnum << 24;
	data |= (rnpgbe_msix_vector << 8);
	data |= (rnpgbe_msix_vector << 0);

	DPRINTK(IFUP, INFO,
		"Set Ring-Vector queue:%d (reg:0x%x) <-- Rx-MSIX:%d, Tx-MSIX:%d\n",
		rnpgbe_queue, RING_VECTOR(rnpgbe_queue), rnpgbe_msix_vector,
		rnpgbe_msix_vector);

	rnpgbe_wr_reg(hw->ring_msix_base + RING_VECTOR(rnpgbe_queue), data);
}

void rnpgbe_unmap_and_free_tx_resource(struct rnpgbe_ring *ring,
				       struct rnpgbe_tx_buffer *tx_buffer)
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

static u64 rnpgbe_get_tx_completed(struct rnpgbe_ring *ring)
{
	return ring->stats.packets;
}

static u64 rnpgbe_get_tx_pending(struct rnpgbe_ring *ring)
{
	u32 head = ring_rd32(ring, RNP_DMA_REG_TX_DESC_BUF_HEAD);
	u32 tail = ring_rd32(ring, RNP_DMA_REG_TX_DESC_BUF_TAIL);

	if (head != tail)
		return (head < tail) ? tail - head :
				       (tail + ring->count - head);

	return 0;
}

static inline bool rnpgbe_check_tx_hang(struct rnpgbe_ring *tx_ring)
{
	u32 tx_done = rnpgbe_get_tx_completed(tx_ring);
	u32 tx_done_old = tx_ring->tx_stats.tx_done_old;
	u32 tx_pending = rnpgbe_get_tx_pending(tx_ring);
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
 * rnpgbe_tx_timeout_reset - initiate reset due to Tx timeout
 * @adapter: driver private struct
 **/
static void rnpgbe_tx_timeout_reset(struct rnpgbe_adapter *adapter)
{
	/* Do the reset outside of interrupt context */
	if (!test_bit(__RNP_DOWN, &adapter->state)) {
		adapter->flags2 |= RNP_FLAG2_RESET_REQUESTED;
		e_warn(drv, "initiating reset due to tx timeout\n");
		rnpgbe_service_event_schedule(adapter);
	}
}

/**
 * rnpgbe_enable_eee_mode - check and enter in LPI mode
 * @priv: driver private structure
 * Description: this function is to verify and enter in LPI mode in case of
 * EEE.
 */
static void rnpgbe_enable_eee_mode(struct rnpgbe_adapter *adapter)
{
	int i = 0;
	struct rnpgbe_ring *tx_ring;
	struct rnpgbe_hw *hw = &adapter->hw;

	for (i = 0; i < (adapter->num_tx_queues); i++) {
		tx_ring = adapter->tx_ring[i];
		if (tx_ring->next_to_use != tx_ring->next_to_clean)
			return;
	}
	/* Check and enter in LPI mode */
	if (!adapter->tx_path_in_lpi_mode) {
		if (hw->ops.set_eee_mode)
			hw->ops.set_eee_mode(hw,
					     adapter->en_tx_lpi_clockgating);
	}
	adapter->tx_path_in_lpi_mode = true;
}

/**
 * rnpgbe_disable_eee_mode - disable and exit from LPI mode
 * @priv: driver private structure
 * Description: this function is to exit and disable EEE in case of
 * LPI state is true. This is called by the xmit.
 */
void rnpgbe_disable_eee_mode(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;

	if (hw->ops.reset_eee_mode)
		hw->ops.reset_eee_mode(hw);

	if (!test_bit(__RNP_EEE_REMOVE, &adapter->state))
		mod_timer(&adapter->eee_ctrl_timer,
			  RNP_LPI_T(adapter->eee_timer));
}

/**
 * rnpgbe_clean_tx_irq - Reclaim resources after transmit completes
 * @q_vector: structure containing interrupt and ring information
 * @tx_ring: tx ring to clean
 **/
static bool rnpgbe_clean_tx_irq(struct rnpgbe_q_vector *q_vector,
				struct rnpgbe_ring *tx_ring, int napi_budget)
{
	struct rnpgbe_adapter *adapter = q_vector->adapter;
	struct rnpgbe_tx_buffer *tx_buffer;
	struct rnpgbe_tx_desc *tx_desc;
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
		struct rnpgbe_tx_desc *eop_desc = tx_buffer->next_to_watch;

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
			budget--;
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
	} while (likely(budget > 0));
#ifdef NO_BQL_TEST
#else
	netdev_tx_completed_queue(txring_txq(tx_ring), total_packets,
				  total_bytes);
#endif
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

	if (!(q_vector->vector_flags & RNP_QVECTOR_FLAG_REDUCE_TX_IRQ_MISS)) {
#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)
		if (likely(netif_carrier_ok(tx_ring->netdev) &&
			   (rnpgbe_desc_unused(tx_ring) >= TX_WAKE_THRESHOLD))) {
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

	if (adapter->eee_active && total_packets) {
		if (!adapter->tx_path_in_lpi_mode) {
			if (!test_bit(__RNP_EEE_REMOVE, &adapter->state))
				mod_timer(&adapter->eee_ctrl_timer,
					  RNP_LPI_T(adapter->eee_timer));
		}
	}

	return !!budget;
}

static inline void rnpgbe_rx_hash(struct rnpgbe_ring *ring,
				  union rnpgbe_rx_desc *rx_desc,
				  struct sk_buff *skb)
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
 * rnpgbe_rx_checksum - indicate in skb if hw indicated a good cksum
 * @ring: structure containing ring specific data
 * @rx_desc: current Rx descriptor being processed
 * @skb: skb currently being received and modified
 **/
static inline void rnpgbe_rx_checksum(struct rnpgbe_ring *ring,
				      union rnpgbe_rx_desc *rx_desc,
				      struct sk_buff *skb)
{
	skb_checksum_none_assert(skb);
	/* Rx csum disabled */
	if (!(ring->netdev->features & NETIF_F_RXCSUM))
		return;

	/* if outer L3/L4  error */
	/* must in promisc mode or rx-all mode */
	if (rnpgbe_test_staterr(rx_desc, RNP_RXD_STAT_ERR_MASK))
		return;
	ring->rx_stats.csum_good++;
	/* at least it is a ip packet which has ip checksum */

	/* It must be a TCP or UDP packet with a valid checksum */
	skb->ip_summed = CHECKSUM_UNNECESSARY;
}

static inline void rnpgbe_update_rx_tail(struct rnpgbe_ring *rx_ring, u32 val)
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
	rnpgbe_wr_reg(rx_ring->tail, val);

	/* if rx_ring in delay setup mode, don't update
	 * next_to_use to hw large than RNP_MIN_RXD
	 */
}

#if (PAGE_SIZE < 8192)
#define RNP_MAX_2K_FRAME_BUILD_SKB (RNP_RXBUFFER_1536 - NET_IP_ALIGN)
#define RNP_2K_TOO_SMALL_WITH_PADDING                                          \
	((NET_SKB_PAD + RNP_RXBUFFER_1536) > SKB_WITH_OVERHEAD(RNP_RXBUFFER_2K))

static inline int rnpgbe_compute_pad(int rx_buf_len)
{
	int page_size, pad_size;

	page_size = ALIGN(rx_buf_len, PAGE_SIZE / 2);
	pad_size = SKB_WITH_OVERHEAD(page_size) - rx_buf_len;

	return pad_size;
}

static inline int rnpgbe_skb_pad(void)
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
	return rnpgbe_compute_pad(rx_buf_len);
}

#define RNP_SKB_PAD rnpgbe_skb_pad()
#else /* PAGE_SIZE < 8192 */
#define RNP_SKB_PAD (NET_SKB_PAD + NET_IP_ALIGN)
#endif
/**
 * rnpgbe_process_skb_fields - Populate skb header fields from Rx descriptor
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @rx_desc: pointer to the EOP Rx descriptor
 * @skb: pointer to current skb being populated
 *
 * This function checks the ring, descriptor, and packet information in
 * order to populate the hash, checksum, VLAN, timestamp, protocol, and
 * other fields within the skb.
 **/
static void rnpgbe_process_skb_fields(struct rnpgbe_ring *rx_ring,
				      union rnpgbe_rx_desc *rx_desc,
				      struct sk_buff *skb)
{
	struct net_device *dev = rx_ring->netdev;
	struct rnpgbe_adapter *adapter = netdev_priv(dev);
	struct rnpgbe_hw *hw = &adapter->hw;
	// rnpgbe_update_rsc_stats(rx_ring, skb);
	rnpgbe_rx_hash(rx_ring, rx_desc, skb);

	rnpgbe_rx_checksum(rx_ring, rx_desc, skb);

	if (hw->ncsi_en) {
		// if ncsi with stags on
		if (adapter->flags2 & RNP_FLAG2_VLAN_STAGS_ENABLED) {
			// check outer stags with set one
			u8 header[ETH_ALEN + ETH_ALEN];
			u8 *data = skb->data;
			struct vlan_ethhdr *veth = (struct vlan_ethhdr *)skb->data;
			u16 vlan_tci;

			switch (adapter->outer_vlan_type) {
			case outer_vlan_type_88a8:
				vlan_tci = htons(ETH_P_8021AD);
				break;
			case outer_vlan_type_9100:
				vlan_tci = htons(ETH_P_QINQ1);
				break;
			case outer_vlan_type_9200:
				vlan_tci = htons(ETH_P_QINQ2);
				break;
			default:
				vlan_tci = htons(ETH_P_8021AD);
				break;
			}

			if (veth->h_vlan_proto != vlan_tci)
				goto skip_vlan;

			if (veth->h_vlan_TCI != htons(adapter->stags_vid))
				goto skip_vlan;

			memcpy(header, data, ETH_ALEN + ETH_ALEN);
			memcpy(skb->data + 4, header, ETH_ALEN + ETH_ALEN);
			skb->len -= 4;
			skb->data += 4;
			goto skip_vlan;

		}
	}

	if (((dev->features & NETIF_F_HW_VLAN_CTAG_RX)
	    || (dev->features & NETIF_F_HW_VLAN_STAG_RX)) &&
	    rnpgbe_test_staterr(rx_desc, RNP_RXD_STAT_VLAN_VALID) &&
	    !ignore_veb_vlan(rx_ring->q_vector->adapter, rx_desc)) {
		if (rnpgbe_test_ext_cmd(rx_desc, REV_OUTER_VLAN)) {
			u16 vid_inner = le16_to_cpu(rx_desc->wb.vlan);
			u16 vid_outer;
			u16 vlan_tci = htons(ETH_P_8021Q);
			__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
					       vid_inner);
			if (rnpgbe_test_staterr(rx_desc, RNP_RXD_STAT_STAG)) {
				switch (rx_ring->q_vector->adapter
						->outer_vlan_type) {
				case outer_vlan_type_88a8:
					vlan_tci = htons(ETH_P_8021AD);
					break;
				case outer_vlan_type_9100:
					vlan_tci = htons(ETH_P_QINQ1);
					break;
				case outer_vlan_type_9200:
					vlan_tci = htons(ETH_P_QINQ2);
					break;
				default:
					vlan_tci = htons(ETH_P_8021AD);
					break;
				}
			} else {
				vlan_tci = htons(ETH_P_8021Q);
			}
			vid_outer = le16_to_cpu(rx_desc->wb.mark);
			/* if in stags mode should ignore only stags */
			if (adapter->flags2 & RNP_FLAG2_VLAN_STAGS_ENABLED) {
				/* push outer in if not equal stags or cvlan */
				if ((vid_outer != adapter->stags_vid) ||
				    (vlan_tci == htons(ETH_P_8021Q))) {
					/* push outer inner */
					skb = __vlan_hwaccel_push_inside(skb);
					__vlan_hwaccel_put_tag(skb, vlan_tci,
							       vid_outer);
					/* if not 88a8, push again to avoid kernel crash
					 * todo
					 * */
				}
				/* if vid_outer is stags_vid do nothing */
			} else {
				/* push outer */
				skb = __vlan_hwaccel_push_inside(skb);
				__vlan_hwaccel_put_tag(skb, vlan_tci,
						       vid_outer);
			}

		} else {
			/* only inner vlan */
			u16 vid = le16_to_cpu(rx_desc->wb.vlan);
			/* check vlan type */
			if (rnpgbe_test_staterr(rx_desc, RNP_RXD_STAT_STAG)) {
				if ((adapter->flags2 &
				     RNP_FLAG2_VLAN_STAGS_ENABLED) &&
				    (vid == adapter->stags_vid)) {
					/* do nothing ignore this stags */
				} else {
					/* should consider other stags */
					switch (rx_ring->q_vector->adapter
							->outer_vlan_type) {
					case outer_vlan_type_88a8:
						__vlan_hwaccel_put_tag(
							skb,
							htons(ETH_P_8021AD),
							vid);
						break;
					case outer_vlan_type_9100:
						__vlan_hwaccel_put_tag(
							skb, htons(ETH_P_QINQ1),
							vid);
						break;
					case outer_vlan_type_9200:
						__vlan_hwaccel_put_tag(
							skb, htons(ETH_P_QINQ2),
							vid);
						break;
					default:
						__vlan_hwaccel_put_tag(
							skb,
							htons(ETH_P_8021AD),
							vid);
						break;
					}
				}
			} else {
				__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
						       vid);
			}
		}
		rx_ring->rx_stats.vlan_remove++;
	}
skip_vlan:
	skb_record_rx_queue(skb, rx_ring->queue_index);

	skb->protocol = eth_type_trans(skb, dev);
}

static void rnpgbe_rx_skb(struct rnpgbe_q_vector *q_vector, struct sk_buff *skb)
{
	struct rnpgbe_adapter *adapter = q_vector->adapter;

	if (!(adapter->flags & RNP_FLAG_IN_NETPOLL))
		napi_gro_receive(&q_vector->napi, skb);
	else
		netif_rx(skb);
}

/* drop this packets if error */
static bool rnpgbe_check_csum_error(struct rnpgbe_ring *rx_ring,
				    union rnpgbe_rx_desc *rx_desc,
				    unsigned int size,
				    unsigned int *driver_drop_packets)
{
	bool err = false;

	struct net_device *netdev = rx_ring->netdev;

	if (netdev->features & NETIF_F_RXCSUM) {
		if (unlikely(rnpgbe_test_staterr(rx_desc,
						 RNP_RXD_STAT_ERR_MASK))) {
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
				/* if we fixed in hw */
				if (rx_ring->ring_flags & RNP_RING_CHKSM_FIX) {
					err = true;
					goto skip_fix;
				}
				/* if not ipv4 with l4 error, we should ignore l4 csum error */
				if (unlikely(rnpgbe_test_staterr(
						     rx_desc,
						     RNP_RXD_STAT_L4_MASK) &&
					     (!(rx_desc->wb.rev1 &
						RNP_RX_L3_TYPE_MASK)))) {
					rx_ring->rx_stats.csum_err--;
					goto skip_fix;
				}
				/* we ignore sctp csum erro small than 60 */
				if (unlikely(rnpgbe_test_staterr(
					    rx_desc, RNP_RXD_STAT_SCTP_MASK))) {
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
		struct rnpgbe_rx_buffer *rx_buffer;
#if (PAGE_SIZE < 8192)
		unsigned int truesize = rnpgbe_rx_pg_size(rx_ring) / 2;
#else
		unsigned int truesize =
			ring_uses_build_skb(rx_ring) ?
				SKB_DATA_ALIGN(RNP_SKB_PAD + size) :
				SKB_DATA_ALIGN(size);
#endif

		if (likely(rnpgbe_test_staterr(rx_desc, RNP_RXD_STAT_EOP)))
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
		rnpgbe_put_rx_buffer(rx_ring, rx_buffer);
#else
		rnpgbe_put_rx_buffer(rx_ring, rx_buffer, NULL);
#endif
		/* update to the next desc */
		ntc = (ntc < rx_ring->count) ? ntc : 0;
		rx_ring->next_to_clean = ntc;
	}
	return err;
}

/**
 * rnpgbe_rx_ring_reinit - just reinit rx_ring with new count in ->reset_count
 * @rx_ring: rx descriptor ring to transact packets on
 */
int rnpgbe_rx_ring_reinit(struct rnpgbe_adapter *adapter,
			  struct rnpgbe_ring *rx_ring)
{
	struct rnpgbe_ring *temp_ring;
	int err = 0;

	if (rx_ring->count == rx_ring->reset_count)
		return 0;
	/* stop rx queue */
	temp_ring = vmalloc(array_size(1, sizeof(struct rnpgbe_ring)));
	if (!temp_ring)
		return -1;

	rnpgbe_disable_rx_queue(adapter, rx_ring);
	memset(temp_ring, 0x00, sizeof(struct rnpgbe_ring));
	/* reinit for this ring */
	memcpy(temp_ring, rx_ring, sizeof(struct rnpgbe_ring));
	/* setup new count */
	temp_ring->count = rx_ring->reset_count;
	err = rnpgbe_setup_rx_resources(temp_ring, adapter);
	if (err) {
		rnpgbe_free_rx_resources(temp_ring);
		goto err_setup;
	}
	rnpgbe_free_rx_resources(rx_ring);
	memcpy(rx_ring, temp_ring, sizeof(struct rnpgbe_ring));
	rnpgbe_configure_rx_ring(adapter, rx_ring);
err_setup:
	vfree(temp_ring);
	/* start rx */
	ring_wr32(rx_ring, RNP_DMA_RX_START, 1);
	return 0;
}

#ifndef OPTM_WITH_LPAGE
/**
 * rnpgbe_alloc_rx_buffers - Replace used receive buffers
 * @rx_ring: ring to place buffers on
 * @cleaned_count: number of buffers to replace
 **/
void rnpgbe_alloc_rx_buffers(struct rnpgbe_ring *rx_ring, u16 cleaned_count)
{
	union rnpgbe_rx_desc *rx_desc;
	struct rnpgbe_rx_buffer *bi;
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
	bufsz = rnpgbe_rx_bufsz(rx_ring);

	do {
		if (!rnpgbe_alloc_mapped_page(rx_ring, bi))
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
		// rx_desc->cmd = 0;
		cleaned_count--;
	} while (cleaned_count);

	i += rx_ring->count;

	if (rx_ring->next_to_use != i)
		rnpgbe_update_rx_tail(rx_ring, i);
}

#endif

static inline unsigned int rnpgbe_rx_offset(struct rnpgbe_ring *rx_ring)
{
	return ring_uses_build_skb(rx_ring) ? RNP_SKB_PAD : 0;
}

#ifdef OPTM_WITH_LPAGE
/**
 * rnpgbe_alloc_rx_buffers - Replace used receive buffers
 * @rx_ring: ring to place buffers on
 * @cleaned_count: number of buffers to replace
 **/
void rnpgbe_alloc_rx_buffers(struct rnpgbe_ring *rx_ring, u16 cleaned_count)
{
	union rnpgbe_rx_desc *rx_desc;
	struct rnpgbe_rx_buffer *bi;
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
	bufsz = rnpgbe_rx_bufsz(rx_ring);

	do {
		int count = 1;
		struct page *page;

		if (!rnpgbe_alloc_mapped_page(rx_ring, bi, rx_desc, bufsz,
					      fun_id))
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
					  rnpgbe_rx_offset(rx_ring);
			/* map page for use */
			dma = dma_map_page_attrs(rx_ring->dev, page,
						 bi->page_offset, bufsz,
						 DMA_FROM_DEVICE,
						 RNP_RX_DMA_ATTR);

			if (dma_mapping_error(rx_ring->dev, dma)) {
				netdev_dbg(rx_ring->netdev,
					   "map second error\n");
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
		rnpgbe_update_rx_tail(rx_ring, i);
}
#endif /* OPTM_WITH_LPAGE */
/**
 * rnpgbe_get_headlen - determine size of header for RSC/LRO/GRO/FCOE
 * @data: pointer to the start of the headers
 * @max_len: total length of section to find headers in
 *
 * This function is meant to determine the length of headers that will
 * be recognized by hardware for LRO, GRO, and RSC offloads.  The main
 * motivation of doing this is to only perform one pull for IPv4 TCP
 * packets so that we can do basic things like calculating the gso_size
 * based on the average data per packet.
 **/
static unsigned int rnpgbe_get_headlen(unsigned char *data,
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

#ifdef OPTM_WITH_LPAGE

/**
 * rnpgbe_is_non_eop - process handling of non-EOP buffers
 * @rx_ring: Rx ring being processed
 * @rx_desc: Rx descriptor for current buffer
 * @skb: Current socket buffer containing buffer in progress
 *
 * This function updates next to clean.  If the buffer is an EOP buffer
 * this function exits returning false, otherwise it will place the
 * sk_buff in the next buffer to be chained and return true indicating
 * that this is in fact a non-EOP buffer.
 **/
static bool rnpgbe_is_non_eop(struct rnpgbe_ring *rx_ring,
			      union rnpgbe_rx_desc *rx_desc)
{
	u32 ntc = rx_ring->next_to_clean + 1;
	/* fetch, update, and store next to clean */
	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;

	prefetch(RNP_RX_DESC(rx_ring, ntc));

	/* if we are the last buffer then there is nothing else to do */
	if (likely(rnpgbe_test_staterr(rx_desc, RNP_RXD_STAT_EOP)))
		return false;
	/* place skb in next buffer to be received */

	/* we should clean it since we used all info in it */
	rx_desc->wb.cmd = 0;

	return true;
}

static bool rnpgbe_alloc_mapped_page(struct rnpgbe_ring *rx_ring,
				     struct rnpgbe_rx_buffer *bi,
				     union rnpgbe_rx_desc *rx_desc, u16 bufsz,
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

	bi->page_offset = rnpgbe_rx_offset(rx_ring);

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
		netdev_dbg(rx_ring->netdev, "map failed\n");

		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}
	bi->dma = dma;
	bi->page = page;
	bi->page_offset = rnpgbe_rx_offset(rx_ring);
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
static bool rnpgbe_alloc_mapped_page(struct rnpgbe_ring *rx_ring,
				     struct rnpgbe_rx_buffer *bi)
{
	struct page *page = bi->page;
	dma_addr_t dma;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page))
		return true;

	page = dev_alloc_pages(rnpgbe_rx_pg_order(rx_ring));
	if (unlikely(!page)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	/* map page for use */
	dma = dma_map_page_attrs(rx_ring->dev, page, 0,
				 rnpgbe_rx_pg_size(rx_ring), DMA_FROM_DEVICE,
				 RNP_RX_DMA_ATTR);

	/*
	 * if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		__free_pages(page, rnpgbe_rx_pg_order(rx_ring));
		netdev_dbg(rx_ring->netdev, "map failed\n");

		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}
	bi->dma = dma;
	bi->page = page;
	bi->page_offset = rnpgbe_rx_offset(rx_ring);
	page_ref_add(page, USHRT_MAX - 1);
	bi->pagecnt_bias = USHRT_MAX;
	rx_ring->rx_stats.alloc_rx_page++;

	return true;
}

/**
 * rnpgbe_is_non_eop - process handling of non-EOP buffers
 * @rx_ring: Rx ring being processed
 * @rx_desc: Rx descriptor for current buffer
 * @skb: Current socket buffer containing buffer in progress
 *
 * This function updates next to clean.  If the buffer is an EOP buffer
 * this function exits returning false, otherwise it will place the
 * sk_buff in the next buffer to be chained and return true indicating
 * that this is in fact a non-EOP buffer.
 **/
static bool rnpgbe_is_non_eop(struct rnpgbe_ring *rx_ring,
			      union rnpgbe_rx_desc *rx_desc,
			      struct sk_buff *skb)
{
	u32 ntc = rx_ring->next_to_clean + 1;
	/* fetch, update, and store next to clean */
	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;

	prefetch(RNP_RX_DESC(rx_ring, ntc));

	/* if we are the last buffer then there is nothing else to do */
	if (likely(rnpgbe_test_staterr(rx_desc, RNP_RXD_STAT_EOP)))
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
 * rnpgbe_pull_tail - rnpgbe specific version of skb_pull_tail
 * @skb: pointer to current skb being adjusted
 *
 * This function is an rnpgbe specific version of __pskb_pull_tail.  The
 * main difference between this version and the original function is that
 * this function can make several assumptions about the state of things
 * that allow for significant optimizations versus the standard function.
 * As a result we can do things like drop a frag and maintain an accurate
 * truesize for the skb.
 */
static void rnpgbe_pull_tail(struct sk_buff *skb)
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
	pull_len = rnpgbe_get_headlen(va, RNP_RX_HDR_SIZE);

	/* align pull length to size of long to optimize memcpy performance */
	skb_copy_to_linear_data(skb, va, ALIGN(pull_len, sizeof(long)));

	/* update all of the pointers */
	skb_frag_size_sub(frag, pull_len);
	skb_frag_off_add(frag, pull_len);
	skb->data_len -= pull_len;
	skb->tail += pull_len;
}

static bool rnpgbe_check_src_mac(struct sk_buff *skb, struct net_device *netdev)
{
	u8 *data = (u8 *)skb->data;
	bool ret = false;
	struct netdev_hw_addr *ha;

	if (is_multicast_ether_addr(data)) {
		if (memcmp(data + netdev->addr_len, netdev->dev_addr,
			   netdev->addr_len) == 0) {
			dev_kfree_skb_any(skb);
			ret = true;
		}
		/* if src mac equal own mac */
		netdev_for_each_uc_addr (ha, netdev) {
			if (memcmp(data + netdev->addr_len, ha->addr,
				   netdev->addr_len) == 0) {
				dev_kfree_skb_any(skb);
				ret = true;
			}
		}
	}
	return ret;
}

/**
 * rnpgbe_cleanup_headers - Correct corrupted or empty headers
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
static bool rnpgbe_cleanup_headers(struct rnpgbe_ring __maybe_unused *rx_ring,
				   union rnpgbe_rx_desc *rx_desc,
				   struct sk_buff *skb)
{
	struct net_device *netdev = rx_ring->netdev;
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
#ifdef OPTM_WITH_LPAGE
#else
	/* XDP packets use error pointer so abort at this point */
	if (IS_ERR(skb))
		return true;
#endif
	/* place header in linear portion of buffer */
	if (!skb_headlen(skb))
		rnpgbe_pull_tail(skb);
	/* if eth_skb_pad returns an error the skb was freed */
	/* will padding skb->len to 60 */
	if (eth_skb_pad(skb))
		return true;

	if ((adapter->flags & RNP_FLAG_SRIOV_ENABLED) &&
	    (!(rx_ring->ring_flags & RNP_RING_VEB_MULTI_FIX)))
		return rnpgbe_check_src_mac(skb, rx_ring->netdev);
	else
		return false;
}

/**
 * rnpgbe_reuse_rx_page - page flip buffer and store it back on the ring
 * @rx_ring: rx descriptor ring to store buffers on
 * @old_buff: donor buffer to have page reused
 *
 * Synchronizes page for reuse by the adapter
 **/
static void rnpgbe_reuse_rx_page(struct rnpgbe_ring *rx_ring,
				 struct rnpgbe_rx_buffer *old_buff)
{
	struct rnpgbe_rx_buffer *new_buff;
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

static inline bool rnpgbe_page_is_reserved(struct page *page)
{
	return (page_to_nid(page) != numa_mem_id()) || page_is_pfmemalloc(page);
}

static bool rnpgbe_can_reuse_rx_page(struct rnpgbe_rx_buffer *rx_buffer)
{
	unsigned int pagecnt_bias = rx_buffer->pagecnt_bias;
	struct page *page = rx_buffer->page;

#ifdef OPTM_WITH_LPAGE
	return false;
#endif
	/* avoid re-using remote pages */
	if (unlikely(rnpgbe_page_is_reserved(page)))
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
#define RNP_LAST_OFFSET (SKB_WITH_OVERHEAD(PAGE_SIZE) - RNP_RXBUFFER_2K)
	if (rx_buffer->page_offset > RNP_LAST_OFFSET)
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
 * rnpgbe_add_rx_frag - Add contents of Rx buffer to sk_buff
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
static void rnpgbe_add_rx_frag(struct rnpgbe_ring *rx_ring,
			       struct rnpgbe_rx_buffer *rx_buffer,
			       struct sk_buff *skb, unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnpgbe_rx_pg_size(rx_ring) / 2;
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
static struct rnpgbe_rx_buffer *
rnpgbe_get_rx_buffer(struct rnpgbe_ring *rx_ring, union rnpgbe_rx_desc *rx_desc,
		     const unsigned int size)
{
	struct rnpgbe_rx_buffer *rx_buffer;

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

static struct rnpgbe_rx_buffer *
rnpgbe_get_rx_buffer(struct rnpgbe_ring *rx_ring, union rnpgbe_rx_desc *rx_desc,
		     struct sk_buff **skb, const unsigned int size)
{
	struct rnpgbe_rx_buffer *rx_buffer;

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
#endif

#ifdef OPTM_WITH_LPAGE
static void rnpgbe_put_rx_buffer(struct rnpgbe_ring *rx_ring,
				 struct rnpgbe_rx_buffer *rx_buffer)
{
	if (rnpgbe_can_reuse_rx_page(rx_buffer)) {
		/* hand second half of page back to the ring */
		rnpgbe_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		/* we are not reusing the buffer so unmap it */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnpgbe_rx_bufsz(rx_ring), DMA_FROM_DEVICE,
				     RNP_RX_DMA_ATTR);
		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
	}

	/* clear contents of rx_buffer */
	rx_buffer->page = NULL;
}

#else
static void rnpgbe_put_rx_buffer(struct rnpgbe_ring *rx_ring,
				 struct rnpgbe_rx_buffer *rx_buffer,
				 struct sk_buff *skb)
{
	if (rnpgbe_can_reuse_rx_page(rx_buffer)) {
		/* hand second half of page back to the ring */
		rnpgbe_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		/* we are not reusing the buffer so unmap it */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnpgbe_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE,
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
static struct sk_buff *rnpgbe_construct_skb(struct rnpgbe_ring *rx_ring,
					    struct rnpgbe_rx_buffer *rx_buffer,
					    union rnpgbe_rx_desc *rx_desc,
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
		headlen = rnpgbe_get_headlen(va, RNP_RX_HDR_SIZE);
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

static struct sk_buff *rnpgbe_build_skb(struct rnpgbe_ring *rx_ring,
					struct rnpgbe_rx_buffer *rx_buffer,
					union rnpgbe_rx_desc *rx_desc,
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

static struct sk_buff *rnpgbe_construct_skb(struct rnpgbe_ring *rx_ring,
					    struct rnpgbe_rx_buffer *rx_buffer,
					    struct xdp_buff *xdp,
					    union rnpgbe_rx_desc *rx_desc)
{
	unsigned int size = xdp->data_end - xdp->data;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnpgbe_rx_pg_size(rx_ring) / 2;
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
		/*
		 * if (!rnpgbe_test_staterr(rx_desc, RNP_RXD_STAT_EOP))
		 * RNP_CB(skb)->dma = rx_buffer->dma;
		 */

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

static struct sk_buff *rnpgbe_build_skb(struct rnpgbe_ring *rx_ring,
					struct rnpgbe_rx_buffer *rx_buffer,
					struct xdp_buff *xdp,
					union rnpgbe_rx_desc *rx_desc)
{
	unsigned int metasize = xdp->data - xdp->data_meta;
	void *va = xdp->data_meta;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnpgbe_rx_pg_size(rx_ring) / 2;
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
static void rnpgbe_rx_buffer_flip(struct rnpgbe_ring *rx_ring,
				  struct rnpgbe_rx_buffer *rx_buffer,
				  unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnpgbe_rx_pg_size(rx_ring) / 2;

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
 * rnpgbe_clean_rx_irq - Clean completed descriptors from Rx ring - bounce buf
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

static int rnpgbe_clean_rx_irq(struct rnpgbe_q_vector *q_vector,
			       struct rnpgbe_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	unsigned int err_packets = 0;
	unsigned int driver_drop_packets = 0;
	struct sk_buff *skb = rx_ring->skb;
	struct rnpgbe_adapter *adapter = q_vector->adapter;
	u16 cleaned_count = rnpgbe_desc_unused_rx(rx_ring);

	while (likely(total_rx_packets < budget)) {
		union rnpgbe_rx_desc *rx_desc;
		struct rnpgbe_rx_buffer *rx_buffer;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= RNP_RX_BUFFER_WRITE) {
			rnpgbe_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}
		rx_desc = RNP_RX_DESC(rx_ring, rx_ring->next_to_clean);

		rx_buf_dump("rx-desc:", rx_desc, sizeof(*rx_desc));
		rx_debug_printk("  dd set: %s\n",
				(rx_desc->wb.cmd & RNP_RXD_STAT_DD) ? "Yes" :
								      "No");

		if (!rnpgbe_test_staterr(rx_desc, RNP_RXD_STAT_DD))
			break;

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();
		rx_debug_printk(
			"queue:%d  rx-desc:%d has-data len:%d next_to_clean %d\n",
			rx_ring->rnpgbe_queue_idx, rx_ring->next_to_clean,
			rx_desc->wb.len, rx_ring->next_to_clean);

		/* handle padding */
		if ((adapter->priv_flags & RNP_PRIV_FLAG_FT_PADDING) &&
		    (!(adapter->priv_flags & RNP_PRIV_FLAG_PADDING_DEBUG))) {
			if (likely(rnpgbe_test_staterr(rx_desc,
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
		if (rnpgbe_check_csum_error(rx_ring, rx_desc, size,
					    &driver_drop_packets)) {
			cleaned_count++;
			err_packets++;
			if (err_packets + total_rx_packets > budget)
				break;
			continue;
		}

		rx_buffer = rnpgbe_get_rx_buffer(rx_ring, rx_desc, size);

		if (skb) {
			rnpgbe_add_rx_frag(rx_ring, rx_buffer, skb, size);
		} else if (ring_uses_build_skb(rx_ring)) {
			skb = rnpgbe_build_skb(rx_ring, rx_buffer, rx_desc,
					       size);
		} else {
			skb = rnpgbe_construct_skb(rx_ring, rx_buffer, rx_desc,
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
			rnpgbe_ptp_get_rx_hwstamp(adapter, rx_desc, skb);

		rnpgbe_put_rx_buffer(rx_ring, rx_buffer);
		cleaned_count++;

		/* place incomplete frames back on ring for completion */
		if (rnpgbe_is_non_eop(rx_ring, rx_desc))
			continue;

		/* verify the packet layout is correct */
		if (rnpgbe_cleanup_headers(rx_ring, rx_desc, skb)) {
			/* we should clean it since we used all info in it */
			rx_desc->wb.cmd = 0;
			skb = NULL;
			continue;
		}

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, timestamp, VLAN, and protocol */
		rnpgbe_process_skb_fields(rx_ring, rx_desc, skb);

		/* we should clean it since we used all info in it */
		rx_desc->wb.cmd = 0;
		rnpgbe_rx_skb(q_vector, skb);
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
 * rnpgbe_clean_rx_irq - Clean completed descriptors from Rx ring - bounce buf
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

static int rnpgbe_clean_rx_irq(struct rnpgbe_q_vector *q_vector,
			       struct rnpgbe_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	unsigned int err_packets = 0;
	unsigned int driver_drop_packets = 0;
	struct rnpgbe_adapter *adapter = q_vector->adapter;
	u16 cleaned_count = rnpgbe_desc_unused_rx(rx_ring);
	bool xdp_xmit = false;
	struct xdp_buff xdp;

	xdp.data = NULL;
	xdp.data_end = NULL;

	while (likely(total_rx_packets < budget)) {
		union rnpgbe_rx_desc *rx_desc;
		struct rnpgbe_rx_buffer *rx_buffer;
		struct sk_buff *skb;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= RNP_RX_BUFFER_WRITE) {
			rnpgbe_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}
		rx_desc = RNP_RX_DESC(rx_ring, rx_ring->next_to_clean);

		rx_buf_dump("rx-desc:", rx_desc, sizeof(*rx_desc));
		rx_debug_printk("  dd set: %s\n",
				(rx_desc->wb.cmd & RNP_RXD_STAT_DD) ? "Yes" :
								      "No");

		if (!rnpgbe_test_staterr(rx_desc, RNP_RXD_STAT_DD))
			break;

		rx_debug_printk(
			"queue:%d  rx-desc:%d has-data len:%d next_to_clean %d\n",
			rx_ring->rnpgbe_queue_idx, rx_ring->next_to_clean,
			rx_desc->wb.len, rx_ring->next_to_clean);

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();
		size = le16_to_cpu(rx_desc->wb.len);
		if (!size)
			break;

		/*
		 * should check csum err
		 * maybe one packet use multiple descs
		 * no problems hw set all csum_err in multiple descs
		 * maybe BUG if the last sctp desc less than 60
		 */
		if (rnpgbe_check_csum_error(rx_ring, rx_desc, size,
					    &driver_drop_packets)) {
			cleaned_count++;
			err_packets++;
			if (err_packets + total_rx_packets > budget)
				break;
			continue;
		}

		rx_buffer = rnpgbe_get_rx_buffer(rx_ring, rx_desc, &skb, size);

		if (!skb) {
			xdp.data = page_address(rx_buffer->page) +
				   rx_buffer->page_offset;
			xdp.data_meta = xdp.data;
			xdp.data_hard_start =
				xdp.data - rnpgbe_rx_offset(rx_ring);
			xdp.data_end = xdp.data + size;
			/* call xdp hook use this to support xdp hook */
		}

		if (IS_ERR(skb)) {
			if (PTR_ERR(skb) == -RNP_XDP_TX) {
				xdp_xmit = true;
				rnpgbe_rx_buffer_flip(rx_ring, rx_buffer, size);
			} else {
				rx_buffer->pagecnt_bias++;
			}
			total_rx_packets++;
			total_rx_bytes += size;
		} else if (skb) {
			rnpgbe_add_rx_frag(rx_ring, rx_buffer, skb, size);
		} else if (ring_uses_build_skb(rx_ring)) {
			skb = rnpgbe_build_skb(rx_ring, rx_buffer, &xdp,
					       rx_desc);
		} else {
			skb = rnpgbe_construct_skb(rx_ring, rx_buffer, &xdp,
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
			rnpgbe_ptp_get_rx_hwstamp(adapter, rx_desc, skb);

		rnpgbe_put_rx_buffer(rx_ring, rx_buffer, skb);
		cleaned_count++;

		/* place incomplete frames back on ring for completion */
		if (rnpgbe_is_non_eop(rx_ring, rx_desc, skb))
			continue;

		/* verify the packet layout is correct */
		if (rnpgbe_cleanup_headers(rx_ring, rx_desc, skb)) {
			/* we should clean it since we used all info in it */
			rx_desc->wb.cmd = 0;
			continue;
		}

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, timestamp, VLAN, and protocol */
		rnpgbe_process_skb_fields(rx_ring, rx_desc, skb);

		/* we should clean it since we used all info in it */
		rx_desc->wb.cmd = 0;

		rnpgbe_rx_skb(q_vector, skb);

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
 * rnpgbe_configure_msix - Configure MSI-X hardware
 * @adapter: board private structure
 *
 * rnpgbe_configure_msix sets up the hardware to properly generate MSI-X
 * interrupts.
 **/
static void rnpgbe_configure_msix(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_q_vector *q_vector;
	int i;
	struct rnpgbe_hw *hw = &adapter->hw;

	/*
	 * configure ring-msix Registers table
	 */
	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnpgbe_ring *ring;

		q_vector = adapter->q_vector[i];
		rnpgbe_for_each_ring(ring, q_vector->rx) {
			rnpgbe_set_ring_vector(adapter, ring->rnpgbe_queue_idx,
					       q_vector->v_idx);
		}
	}
	/* n500 should mask other */
	if ((hw->hw_type == rnpgbe_hw_n500) ||
	    (hw->hw_type == rnpgbe_hw_n210)) {
		/*
	 *  8  lpi | PMT
	 *  9  BMC_RX_IRQ |
	 *  10 PHY_IRQ | LPI_IRQ
	 *  11 BMC_TX_IRQ |
	 *  may DMAR error if set pf to vm
	 */
#define OTHER_VECTOR_START (8)
#define OTHER_VECTOR_STOP (11)
#define MSIX_UNUSED (0x0f0f)
		for (i = OTHER_VECTOR_START; i <= OTHER_VECTOR_STOP; i++) {
			if (hw->feature_flags & RNP_HW_SOFT_MASK_OTHER_IRQ)
				rnpgbe_wr_reg(hw->ring_msix_base +
						      RING_VECTOR(i),
					      MSIX_UNUSED);
			else
				rnpgbe_wr_reg(
					hw->ring_msix_base + RING_VECTOR(i), 0);
		}
		if (hw->feature_flags & RNP_HW_FEATURE_EEE) {
#define LPI_IRQ (8)
			/* only open lpi irq */
			if (hw->feature_flags & RNP_HW_SOFT_MASK_OTHER_IRQ)
				rnpgbe_wr_reg(hw->ring_msix_base +
						      RING_VECTOR(LPI_IRQ),
					      0x000f);
			else
				rnpgbe_wr_reg(hw->ring_msix_base +
						      RING_VECTOR(LPI_IRQ),
					      0x0000);
		}
	}
}


static void rnpgbe_update_ring_itr_rx(struct rnpgbe_q_vector *q_vector)
{
	int new_val = q_vector->itr_rx;
	int avg_wire_size = 0;
	struct rnpgbe_adapter *adapter = q_vector->adapter;
	unsigned int packets;
	/* For non-gigabit speeds, just fix the interrupt rate at 4000
	 * ints/sec - ITR timer value of 120 ticks.
	 */
	switch (adapter->link_speed) {
	case RNP_LINK_SPEED_10_FULL:
	case RNP_LINK_SPEED_100_FULL:
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

static void rnpgbe_write_eitr_rx(struct rnpgbe_q_vector *q_vector)
{
	struct rnpgbe_adapter *adapter = q_vector->adapter;
	struct rnpgbe_hw *hw = &adapter->hw;
	u32 new_itr_rx = q_vector->rx.itr;
	u32 old_itr_rx = q_vector->rx.itr;
	struct rnpgbe_ring *ring;

	new_itr_rx = new_itr_rx * hw->usecstocount;
	/* if we are in auto mode write to hw */
	if (!(adapter->priv_flags & RNP_PRIV_FLAG_RX_COALESCE)) {
		rnpgbe_for_each_ring(ring, q_vector->rx) {
			ring_wr32(ring, RNP_DMA_REG_RX_INT_DELAY_TIMER,
				  new_itr_rx);
			if (ring->ring_flags & RNP_RING_LOWER_ITR) {
				/* if we are alredy in this mode skip */
				if (q_vector->itr_rx == RNP_LOWEREST_ITR)
					continue;
				ring_wr32(ring, RNP_DMA_REG_RX_INT_DELAY_PKTCNT,
					  1);
				ring_wr32(ring, RNP_DMA_REG_RX_INT_DELAY_TIMER,
					  RNP_LOWEREST_ITR);
				q_vector->itr_rx = RNP_LOWEREST_ITR;
			} else {
				if (new_itr_rx == q_vector->itr_rx)
					continue;
				ring_wr32(ring, RNP_DMA_REG_RX_INT_DELAY_TIMER,
					  new_itr_rx);
				ring_wr32(ring, RNP_DMA_REG_RX_INT_DELAY_PKTCNT,
					  adapter->rx_frames);
				q_vector->itr_rx = old_itr_rx;
			}
		}
	}
}

enum latency_range {
	lowest_latency = 0,
	low_latency = 1,
	bulk_latency = 2,
	latency_invalid = 255
};

static inline void rnpgbe_irq_enable_queues(struct rnpgbe_adapter *adapter,
					    struct rnpgbe_q_vector *q_vector)
{
	struct rnpgbe_ring *ring;

	rnpgbe_for_each_ring(ring, q_vector->rx) {
#ifdef CONFIG_RNP_DISABLE_TX_IRQ
		rnpgbe_wr_reg(ring->dma_int_mask, ~(RX_INT_MASK));
#else
		rnpgbe_wr_reg(ring->dma_int_mask, ~(RX_INT_MASK | TX_INT_MASK));
		ring_wr32(ring, RNP_DMA_INT_TRIG,
			  (0x3 << 16) | TX_INT_MASK | RX_INT_MASK);
#endif
	}
}

static inline void rnpgbe_irq_disable_queues(struct rnpgbe_q_vector *q_vector)
{
	struct rnpgbe_ring *ring;

	rnpgbe_for_each_ring(ring, q_vector->tx) {
		ring_wr32(ring, RNP_DMA_INT_TRIG,
			  (0x3 << 16) | (~(TX_INT_MASK | RX_INT_MASK)));
		rnpgbe_wr_reg(ring->dma_int_mask, (RX_INT_MASK | TX_INT_MASK));
	}
}
/**
 * rnpgbe_irq_enable - Enable default interrupt generation settings
 * @adapter: board private structure
 **/
static inline void rnpgbe_irq_enable(struct rnpgbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_q_vectors; i++)
		rnpgbe_irq_enable_queues(adapter, adapter->q_vector[i]);
}

static void rnpgbe_lpi_task(struct rnpgbe_adapter *adapter)
{
	int status;
	struct rnpgbe_hw *hw = &adapter->hw;

	if (hw->feature_flags & RNP_HW_FEATURE_EEE) {
		status = hw->ops.get_lpi_status(hw);

		if (status) {
			if (status & CORE_IRQ_TX_PATH_IN_LPI_MODE)
				adapter->tx_path_in_lpi_mode = true;
			if (status & CORE_IRQ_TX_PATH_EXIT_LPI_MODE)
				adapter->tx_path_in_lpi_mode = false;
		}
	}
}

static irqreturn_t rnpgbe_msix_other(int irq, void *data)
{
	struct rnpgbe_adapter *adapter = data;

	set_bit(__RNP_IN_IRQ, &adapter->state);
	rnpgbe_lpi_task(adapter);
	rnpgbe_msg_task(adapter);
	clear_bit(__RNP_IN_IRQ, &adapter->state);

	return IRQ_HANDLED;
}

static irqreturn_t rnpgbe_msix_clean_rings(int irq, void *data)
{
	struct rnpgbe_q_vector *q_vector = data;

	rnpgbe_irq_disable_queues(q_vector);

	rnpgbe_write_eitr_rx(q_vector);
	/*  disabled interrupts (on this vector) for us */

	if (q_vector->rx.ring || q_vector->tx.ring)
		napi_schedule_irqoff(&q_vector->napi);

	return IRQ_HANDLED;
}

/**
 * rnpgbe_poll - NAPI Rx polling callback
 * @napi: structure for representing this polling device
 * @budget: how many packets driver is allowed to clean
 *
 * This function is used for legacy and MSI, NAPI mode
 **/
int rnpgbe_poll(struct napi_struct *napi, int budget)
{
	struct rnpgbe_q_vector *q_vector =
		container_of(napi, struct rnpgbe_q_vector, napi);
	struct rnpgbe_adapter *adapter = q_vector->adapter;
	struct rnpgbe_ring *ring;
	int per_ring_budget, work_done = 0;
	bool clean_complete = true;
	int cleaned_total = 0;

	rnpgbe_for_each_ring(ring, q_vector->tx) {
		clean_complete = rnpgbe_clean_tx_irq(q_vector, ring, budget);
	}

	/* attempt to distribute budget to each queue fairly, but don't allow
	 * the budget to go below 1 because we'll exit polling
	 */
	if (q_vector->rx.count > 1)
		per_ring_budget = max(budget / q_vector->rx.count, 1);
	else
		per_ring_budget = budget;

	rnpgbe_for_each_ring(ring, q_vector->rx) {
		int cleaned = 0;
		/* this ring is waitting to reset rx_len*/
		/* avoid to deal this ring until reset done */
		if (likely(!(ring->ring_flags & RNP_RING_FLAG_DO_RESET_RX_LEN)))
			cleaned = rnpgbe_clean_rx_irq(q_vector, ring,
						      per_ring_budget);
		work_done += cleaned;
		cleaned_total += cleaned;
		if (cleaned >= per_ring_budget)
			clean_complete = false;
	}

	/* force close irq */
	if (test_bit(__RNP_DOWN, &adapter->state))
		clean_complete = true;

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
			if (likely(napi_complete_done(napi, work_done))) {
				if (!test_bit(__RNP_DOWN, &adapter->state))
					rnpgbe_irq_enable_queues(adapter, q_vector);
			}
			return min(work_done, budget - 1);
		}
		return budget;
	}

	if (likely(napi_complete_done(napi, work_done))) {
		/* try to do itr handle */
		if (!(adapter->priv_flags & RNP_PRIV_FLAG_RX_COALESCE))
			rnpgbe_update_ring_itr_rx(q_vector);

		if (!test_bit(__RNP_DOWN, &adapter->state))
			rnpgbe_irq_enable_queues(adapter, q_vector);
	}

	return min(work_done, budget - 1);
}

/**
 * rnpgbe_irq_affinity_notify - Callback for affinity changes
 * @notify: context as to what irq was changed
 * @mask: the new affinity mask
 *
 * This is a callback function used by the irq_set_affinity_notifier function
 * so that we may register to receive changes to the irq affinity masks.
 **/
static void rnpgbe_irq_affinity_notify(struct irq_affinity_notify *notify,
				       const cpumask_t *mask)
{
	struct rnpgbe_q_vector *q_vector =
		container_of(notify, struct rnpgbe_q_vector, affinity_notify);

	cpumask_copy(&q_vector->affinity_mask, mask);
}

/**
 * rnpgbe_irq_affinity_release - Callback for affinity notifier release
 * @ref: internal core kernel usage
 *
 * This is a callback function used by the irq_set_affinity_notifier function
 * to inform the current notification subscriber that they will no longer
 * receive notifications.
 **/
static void rnpgbe_irq_affinity_release(struct kref *ref)
{
}

static irqreturn_t rnpgbe_intr(int irq, void *data)
{
	struct rnpgbe_adapter *adapter = data;
	struct rnpgbe_q_vector *q_vector = adapter->q_vector[0];

	/* disabled interrupts (on this vector) for us */
	rnpgbe_irq_disable_queues(q_vector);

	rnpgbe_write_eitr_rx(q_vector);

	if (q_vector->rx.ring || q_vector->tx.ring)
		napi_schedule_irqoff(&q_vector->napi);

	rnpgbe_msg_task(adapter);
	rnpgbe_lpi_task(adapter);

	return IRQ_HANDLED;
}

/**
 * rnpgbe_request_msix_irqs - Initialize MSI-X interrupts
 * @adapter: board private structure
 *
 * rnpgbe_request_msix_irqs allocates MSI-X vectors and requests
 * interrupts from the kernel.
 **/
static int rnpgbe_request_msix_irqs(struct rnpgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int err;
	int i = 0;

	DPRINTK(IFUP, INFO, "[%s] num_q_vectors:%d\n", __func__,
		adapter->num_q_vectors);

	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnpgbe_q_vector *q_vector = adapter->q_vector[i];
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
		err = request_irq(entry->vector, &rnpgbe_msix_clean_rings, 0,
				  q_vector->name, q_vector);
		if (err) {
			e_err(probe,
			      "%s:request_irq failed for MSIX interrupt:%d "
			      "Error: %d\n",
			      netdev->name, entry->vector, err);
			goto free_queue_irqs;
		}
		/* register for affinity change notifications */
		q_vector->affinity_notify.notify = rnpgbe_irq_affinity_notify;
		q_vector->affinity_notify.release = rnpgbe_irq_affinity_release;
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

static int rnpgbe_free_msix_irqs(struct rnpgbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnpgbe_q_vector *q_vector = adapter->q_vector[i];
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

#ifdef DISABLE_RX_IRQ
int rx_poll_thread_handler(void *data)
{
	int i;
	struct rnpgbe_adapter *adapter = data;

	dbg("%s  %s running...\n", __func__, adapter->name);

	do {
		for (i = 0; i < adapter->num_q_vectors; i++)
			rnpgbe_msix_clean_rings(0, adapter->q_vector[i]);

		msleep(30);
	} while (!kthread_should_stop() && adapter->quit_poll_thread != true);

	dbg("%s  %s stopped\n", __func__, adapter->name);

	return 0;
}
#endif

/**
 * rnpgbe_request_irq - initialize interrupts
 * @adapter: board private structure
 *
 * Attempts to configure interrupts using the best available
 * capabilities of the hardware and kernel.
 **/
static int rnpgbe_request_irq(struct rnpgbe_adapter *adapter)
{
	int err;
	struct rnpgbe_hw *hw = &adapter->hw;

#ifdef DISABLE_RX_IRQ
	adapter->rx_poll_thread =
		kthread_run(rx_poll_thread_handler, adapter, adapter->name);
	if (!adapter->rx_poll_thread) {
		rnpgbe_err("kthread_run failed!\n");
		return -EIO;
	}
	return 0;
#endif
	if (adapter->flags & RNP_FLAG_MSIX_ENABLED) {
		pr_info("msix mode is used\n");
		err = rnpgbe_request_msix_irqs(adapter);
		if ((hw->hw_type == rnpgbe_hw_n500) ||
		    (hw->hw_type == rnpgbe_hw_n210))
			wr32(hw, RNP500_LEGANCY_ENABLE, 0);
	} else if (adapter->flags & RNP_FLAG_MSI_ENABLED) {
		/* in this case one for all */
		pr_info("msi mode is used\n");
		err = request_irq(adapter->pdev->irq, rnpgbe_intr, 0,
				  adapter->netdev->name, adapter);
		adapter->hw.mbx.other_irq_enabled = true;
		if ((hw->hw_type == rnpgbe_hw_n500) ||
		    (hw->hw_type == rnpgbe_hw_n210))
			wr32(hw, RNP500_LEGANCY_ENABLE, 0);
	} else {
		pr_info("legacy mode is used\n");
		err = request_irq(adapter->pdev->irq, rnpgbe_intr, IRQF_SHARED,
				  adapter->netdev->name, adapter);
		adapter->hw.mbx.other_irq_enabled = true;
		if ((hw->hw_type == rnpgbe_hw_n500) ||
		    (hw->hw_type == rnpgbe_hw_n210)) {
			wr32(hw, RNP500_LEGANCY_ENABLE, 1);
			wr32(hw, RNP500_LEGANCY_TIME, 0x200);
		}
	}

	if (err)
		e_err(probe, "request_irq failed, Error %d\n", err);

	return err;
}

static void rnpgbe_free_irq(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;

#ifdef DISABLE_RX_IRQ
	return;
#endif
	if (adapter->flags & RNP_FLAG_MSIX_ENABLED) {
		rnpgbe_free_msix_irqs(adapter);
	} else if (adapter->flags & RNP_FLAG_MSI_ENABLED) {
		/* in this case one for all */
		free_irq(adapter->pdev->irq, adapter);
		adapter->hw.mbx.other_irq_enabled = false;
	} else {
		free_irq(adapter->pdev->irq, adapter);
		adapter->hw.mbx.other_irq_enabled = false;
		if ((hw->hw_type == rnpgbe_hw_n500) ||
		    (hw->hw_type == rnpgbe_hw_n210))
			wr32(hw, RNP500_LEGANCY_ENABLE, 0);
	}
}

/**
 * rnpgbe_irq_disable - Mask off interrupt generation on the NIC
 * @adapter: board private structure
 **/
static inline void rnpgbe_irq_disable(struct rnpgbe_adapter *adapter)
{
	int i, j;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		rnpgbe_irq_disable_queues(adapter->q_vector[i]);
		j = i + adapter->q_vector_off;

		if (adapter->flags & RNP_FLAG_MSIX_ENABLED)
			synchronize_irq(adapter->msix_entries[j].vector);
		else
			synchronize_irq(adapter->pdev->irq);
	}
}

int rnpgbe_setup_tx_maxrate(struct rnpgbe_ring *tx_ring, u64 max_rate,
			    int samples_1sec)
{
	/* set hardware samping internal 1S */
	ring_wr32(tx_ring, RNP_DMA_REG_TX_FLOW_CTRL_TM, samples_1sec);
	ring_wr32(tx_ring, RNP_DMA_REG_TX_FLOW_CTRL_TH, max_rate);

	return 0;
}

/**
 * rnpgbe_tx_maxrate_own - callback to set the maximum per-queue bitrate
 * @netdev: network interface device structure
 * @queue_index: Tx queue to set
 * @maxrate: desired maximum transmit bitrate Mbps
 **/
static int rnpgbe_tx_maxrate_own(struct rnpgbe_adapter *adapter,
				 int queue_index)
{
	struct rnpgbe_ring *tx_ring = adapter->tx_ring[queue_index];
	u64 real_rate = 0;
	u32 maxrate = adapter->max_rate[queue_index];

	if (!maxrate)
		return rnpgbe_setup_tx_maxrate(
			tx_ring, 0, adapter->hw.usecstocount * 100000);
	/* we need turn it to bytes/s */
	if (real_rate < 50)
		real_rate = ((u64)maxrate * 1000 * 85) >> 3;
	else
		real_rate = ((u64)maxrate * 1000 * 94) >> 3;
	rnpgbe_setup_tx_maxrate(tx_ring, real_rate,
				adapter->hw.usecstocount * 100000);

	return 0;
}

/**
 * rnpgbe_configure_tx_ring - Configure 8259x Tx ring after Reset
 * @adapter: board private structure
 * @ring: structure containing ring specific data
 *
 * Configure the Tx descriptor ring after a reset.
 **/
void rnpgbe_configure_tx_ring(struct rnpgbe_adapter *adapter,
			      struct rnpgbe_ring *ring)
{
	struct rnpgbe_hw *hw = &adapter->hw;

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
	rnpgbe_wr_reg(ring->tail, ring->next_to_use);

	ring_wr32(ring, RNP_DMA_REG_TX_DESC_FETCH_CTRL,
		  (8 << 0) /* max_water_flow */
			  | (TSRN10_TX_DEFAULT_BURST << 16)
		  /* max-num_descs_peer_read */
	);

	ring_wr32(ring, RNP_DMA_REG_TX_INT_DELAY_TIMER,
		  adapter->tx_usecs * hw->usecstocount);
	ring_wr32(ring, RNP_DMA_REG_TX_INT_DELAY_PKTCNT, adapter->tx_frames);

	rnpgbe_tx_maxrate_own(adapter, ring->queue_index);
	/* flow control: bytes-peer-ctrl-tm-clk. 0:no-control */
	if (adapter->flags & RNP_FLAG_FDIR_HASH_CAPABLE) {
		ring->atr_sample_rate = adapter->atr_sample_rate;
		ring->atr_count = 0;
		set_bit(__RNP_TX_FDIR_INIT_DONE, &ring->state);
	} else {
		ring->atr_sample_rate = 0;
	}

	clear_bit(__RNP_HANG_CHECK_ARMED, &ring->state);

	if (!(ring->ring_flags & RNP_RING_SKIP_TX_START)) {
		/* n500 should wait tx_ready before open tx start */
		int timeout = 0;
		u32 status = 0;

		do {
			status = ring_rd32(ring, RNP_DMA_TX_READY);
			usleep_range(100, 200);
			timeout++;
			rnpgbe_dbg("wait %d tx ready to 1\n",
				   ring->rnpgbe_queue_idx);
		} while ((status != 1) && (timeout < 100));

		if (timeout >= 100)
			rnpgbe_dbg("wait tx ready timeout\n");
		ring_wr32(ring, RNP_DMA_TX_START, 1);
	}
}

/**
 * rnpgbe_configure_tx - Configure Transmit Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Tx unit of the MAC after a reset.
 **/
static void rnpgbe_configure_tx(struct rnpgbe_adapter *adapter)
{
	u32 i, dma_axi_ctl;
	struct rnpgbe_hw *hw = &adapter->hw;
	struct rnpgbe_dma_info *dma = &hw->dma;

	/* dma_axi_en.tx_en must be before Tx queues are enabled */
	dma_axi_ctl = dma_rd32(dma, RNP_DMA_AXI_EN);
	dma_axi_ctl |= TX_AXI_RW_EN;
	dma_wr32(dma, RNP_DMA_AXI_EN, dma_axi_ctl);

	/* Setup the HW Tx Head and Tail descriptor pointers */
	for (i = 0; i < (adapter->num_tx_queues); i++)
		rnpgbe_configure_tx_ring(adapter, adapter->tx_ring[i]);
}

void rnpgbe_disable_rx_queue(struct rnpgbe_adapter *adapter,
			     struct rnpgbe_ring *ring)
{
	ring_wr32(ring, RNP_DMA_RX_START, 0);
}

void rnpgbe_configure_rx_ring(struct rnpgbe_adapter *adapter,
			      struct rnpgbe_ring *ring)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	u64 desc_phy = ring->dma;
	u16 q_idx = ring->queue_index;

	/* disable queue to avoid issues while updating state */
	rnpgbe_disable_rx_queue(adapter, ring);

	/* set descripts registers*/
	ring_wr32(ring, RNP_DMA_REG_RX_DESC_BUF_BASE_ADDR_LO, (u32)desc_phy);
	ring_wr32(ring, RNP_DMA_REG_RX_DESC_BUF_BASE_ADDR_HI,
		  ((u32)(desc_phy >> 32)) | (hw->pfvfnum << 24));
	ring_wr32(ring, RNP_DMA_REG_RX_DESC_BUF_LEN, ring->count);

	ring->tail = ring->ring_addr + RNP_DMA_REG_RX_DESC_BUF_TAIL;
	ring->next_to_clean = ring_rd32(ring, RNP_DMA_REG_RX_DESC_BUF_HEAD);
	ring->next_to_use = ring->next_to_clean;

	if (ring->ring_flags & RNP_RING_SCATER_SETUP) {
#if (PAGE_SIZE < 8192)
		{
			int split_size;

			split_size = rnpgbe_rx_pg_size(ring) / 2 -
				     rnpgbe_rx_offset(ring) -
				     sizeof(struct skb_shared_info);
			split_size = split_size >> 4;
			ring_wr32(ring, PCI_DMA_REG_RX_SCATTER_LENGTH,
				  split_size);
		}
#else
		ring_wr32(ring, PCI_DMA_REG_RX_SCATTER_LENGTH, 96);
#endif
	}

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
		/* if ncsi card ,maybe should setup this */
		/* drop packets if no rx-desc in 800ms, maybe os crash */
		if (hw->ncsi_en)
			ring_wr32(ring, PCI_DMA_REG_RX_DESC_TIMEOUT_TH, 100000);

		else
			ring_wr32(ring, PCI_DMA_REG_RX_DESC_TIMEOUT_TH, 0);
	}

	ring_wr32(ring, RNP_DMA_REG_RX_INT_DELAY_TIMER,
		  adapter->rx_usecs * hw->usecstocount);
	ring_wr32(ring, RNP_DMA_REG_RX_INT_DELAY_PKTCNT, adapter->rx_frames);
	rnpgbe_alloc_rx_buffers(ring, rnpgbe_desc_unused_rx(ring));
}

static void rnpgbe_configure_virtualization(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct rnpgbe_dma_info *dma = &hw->dma;
	u32 ring, vfnum;
	int i, vf_ring;
	u64 real_rate = 0;

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
	ring = adapter->tx_ring[0]->rnpgbe_queue_idx;
	hw->ops.set_sriov_status(hw, true);

	/* store vfnum */
	vfnum = hw->max_vfs - 1;
	hw->veb_ring = ring;
	hw->vfnum = vfnum;
	/* use last-vf's table entry. the last */
	adapter->vf_num_for_pf = 0x80 | vfnum;

	/* setup vf tx rate setup here */
	for (i = 0; i < adapter->num_vfs; i++) {
		vf_ring = rnpgbe_get_vf_ringnum(hw, i, 0);
		real_rate = (adapter->vfinfo[i].tx_rate * 1024 * 128);
		rnpgbe_setup_ring_maxrate(adapter, vf_ring, real_rate);
	}
}

static void rnpgbe_set_rx_buffer_len(struct rnpgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int max_frame = netdev->mtu + ETH_HLEN + ETH_FCS_LEN * 3;
	struct rnpgbe_ring *rx_ring;
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
		rx_ring->rx_per_buf_mem = ALIGN(
			(rnpgbe_rx_offset(rx_ring) + rnpgbe_rx_bufsz(rx_ring) +
			 SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
			 RNP_RX_HWTS_OFFSET),
			1024);
#endif
	}
}

/**
 * rnpgbe_configure_rx - Configure 8259x Receive Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Rx unit of the MAC after a reset.
 **/
static void rnpgbe_configure_rx(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct rnpgbe_dma_info *dma = &hw->dma;
	int i;
	u32 rxctrl = 0, dma_axi_ctl;

	/* set_rx_buffer_len must be called before ring initialization */
	rnpgbe_set_rx_buffer_len(adapter);

	/*
	 * Setup the HW Rx Head and Tail Descriptor Pointers and
	 * the Base and Length of the Rx Descriptor Ring
	 */
	for (i = 0; i < adapter->num_rx_queues; i++)
		rnpgbe_configure_rx_ring(adapter, adapter->rx_ring[i]);

	if (adapter->num_rx_queues > 0) {
		wr32(hw, RNP_ETH_DEFAULT_RX_RING,
		     adapter->rx_ring[0]->rnpgbe_queue_idx);
	}

	/* enable all receives */
	rxctrl |= 0;

	dma_axi_ctl = dma_rd32(dma, RNP_DMA_AXI_EN);
	dma_axi_ctl |= RX_AXI_RW_EN;
	dma_wr32(dma, RNP_DMA_AXI_EN, dma_axi_ctl);
}

static int rnpgbe_vlan_rx_add_vid(struct net_device *netdev,
				  __always_unused __be16 proto, u16 vid)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;
	bool veb_setup = true;

	bool sriov_flag = !!(adapter->flags & RNP_FLAG_SRIOV_ENABLED);

	if (sriov_flag) {
		if (hw->feature_flags & RNP_VEB_VLAN_MASK_EN) {
			if (hw->ops.set_veb_vlan_mask) {
				if (hw->ops.set_veb_vlan_mask(
					    hw, vid, hw->vfnum, true) != 0) {
					netdev_dbg(netdev,
						   "out of vlan entries\n");
					return -EACCES;
				}
			}
		} else {
			/* in sriov mode */
			if ((vid) && (adapter->vf_vlan) &&
			    (vid != adapter->vf_vlan)) {
				netdev_dbg(netdev,
					   "only 1 vlan in sriov mode\n");
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
		if (proto == htons(ETH_P_8021Q))
			adapter->vlan_count++;
	}

	if (vid < VLAN_N_VID) {
		if (proto != htons(ETH_P_8021Q)) {
			set_bit(vid, adapter->active_vlans_stags);
			veb_setup = false;
		} else {
			set_bit(vid, adapter->active_vlans);
		}
	}
	/* if vid 0 never setup veb */
	if (vid == 0)
		veb_setup = false;

	/* only ctags setup veb if in sriov and not stags */
	if (hw->ops.set_vlan_filter) {
		hw->ops.set_vlan_filter(hw, vid, true,
					(sriov_flag && veb_setup));
	}
	return 0;
}

static int rnpgbe_vlan_rx_kill_vid(struct net_device *netdev,
				   __always_unused __be16 proto, u16 vid)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;
	int i;
	bool sriov_flag = !!(adapter->flags & RNP_FLAG_SRIOV_ENABLED);
	bool veb_setup = true;

	if (!vid)
		return 0;
	if (sriov_flag) {
		if (vid) {
			int true_remove = 1;
			adapter->vf_vlan = 0;
			for (i = 0; i < adapter->num_vfs; i++) {
				if (vid == adapter->vfinfo[i].vf_vlan)
					true_remove = 0;
			}
			/* if no vf use this vid */
			if (true_remove) {
				if (proto != htons(ETH_P_8021Q)) {
					veb_setup = false;
					if (!test_bit(vid,
						      adapter->active_vlans))
						true_remove = 1;
				} else {
					if (!test_bit(
						    vid,
						    adapter->active_vlans_stags))
						true_remove = 1;
				}
				/* if no other tags use this vid */
				if (true_remove) {
					if ((adapter->flags2 &
					     RNP_FLAG2_VLAN_STAGS_ENABLED) &&
					    (vid != adapter->stags_vid))
						hw->ops.set_vlan_filter(
							hw, vid, false,
							veb_setup);
				}
			}
			/* always clean veb */
			hw->ops.set_vlan_filter(hw, vid, false, true);

			if (hw->ops.set_vf_vlan_mode) {
				if (hw->feature_flags &
				    RNP_NET_FEATURE_VF_FIXED)
					hw->ops.set_vf_vlan_mode(hw, vid, 0,
								 false);
				else
					hw->ops.set_vf_vlan_mode(
						hw, vid, hw->vfnum, false);
			}

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
			if (!test_bit(vid, adapter->active_vlans_stags))
				true_remove = 1;
		}
		if (true_remove) {
			if ((adapter->flags2 & RNP_FLAG2_VLAN_STAGS_ENABLED) &&
			    (vid == adapter->stags_vid))
				goto SKIP_REMOVE;
			hw->ops.set_vlan_filter(hw, vid, false, false);
		}
	}
SKIP_REMOVE:;
	if (vid) {
		if (proto == htons(ETH_P_8021Q)) {
			/* should check proto todo */
			adapter->vlan_count--;
		}
	}
	if (proto == htons(ETH_P_8021Q))
		clear_bit(vid, adapter->active_vlans);
	if (proto != htons(ETH_P_8021Q))
		clear_bit(vid, adapter->active_vlans_stags);

	return 0;
}

/**
 * rnpgbe_vlan_strip_disable - helper to disable hw vlan stripping
 * @adapter: driver data
 */
static void rnpgbe_vlan_strip_disable(struct rnpgbe_adapter *adapter)
{
	int i;
	struct rnpgbe_ring *tx_ring;
	struct rnpgbe_hw *hw = &adapter->hw;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		tx_ring = adapter->rx_ring[i];
		hw->ops.set_vlan_strip(hw, tx_ring->rnpgbe_queue_idx, false);
	}
}

/**
 * rnpgbe_vlan_strip_enable - helper to enable hw vlan stripping
 * @adapter: driver data
 */
static void rnpgbe_vlan_strip_enable(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct rnpgbe_ring *tx_ring;
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		tx_ring = adapter->rx_ring[i];

		hw->ops.set_vlan_strip(hw, tx_ring->rnpgbe_queue_idx, true);
	}
}

static void rnpgbe_remove_vlan(struct rnpgbe_adapter *adapter)
{
	adapter->vlan_count = 0;
}

static void rnpgbe_restore_vlan(struct rnpgbe_adapter *adapter)
{
	u16 vid;
	struct rnpgbe_hw *hw = &adapter->hw;
	struct rnpgbe_eth_info *eth = &hw->eth;

	/* in stags open, set stags_vid to vlan filter */
	if (adapter->flags2 & RNP_FLAG2_VLAN_STAGS_ENABLED)
		eth->ops.set_vfta(eth, adapter->stags_vid, true);

	rnpgbe_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q), 0);

	for_each_set_bit(vid, adapter->active_vlans, VLAN_N_VID) {
		rnpgbe_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q),
				       vid);
	}

	for_each_set_bit(vid, adapter->active_vlans_stags, VLAN_N_VID)
		rnpgbe_vlan_rx_add_vid(adapter->netdev, htons(0x88a8), vid);
}

/**
 * rnpgbe_set_rx_mode - Unicast, Multicast and Promiscuous mode set
 * @netdev: network interface device structure
 *
 * The set_rx_method entry point is called whenever the unicast/multicast
 * address list or the network interface flags are updated.  This routine is
 * responsible for configuring the hardware for proper unicast, multicast and
 * promiscuous mode.
 **/
void rnpgbe_set_rx_mode(struct net_device *netdev)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;
	netdev_features_t features;
	bool sriov_flag = !!(adapter->flags & RNP_FLAG_SRIOV_ENABLED);

	hw->ops.set_rx_mode(hw, netdev, sriov_flag);

	if (sriov_flag) {
		if (!test_and_set_bit(__RNP_USE_VFINFI, &adapter->state)) {
			rnpgbe_restore_vf_macvlans(adapter);

			rnpgbe_restore_vf_macs(adapter);
			clear_bit(__RNP_USE_VFINFI, &adapter->state);
		}
	}

	features = netdev->features;

	if (features & NETIF_F_HW_VLAN_CTAG_RX)
		rnpgbe_vlan_strip_enable(adapter);
	else
		rnpgbe_vlan_strip_disable(adapter);
	/* only do this if hw support stags */
	if (hw->feature_flags & RNP_NET_FEATURE_STAG_OFFLOAD) {
		if (features & NETIF_F_HW_VLAN_STAG_RX)
			rnpgbe_vlan_strip_enable(adapter);
		else
			rnpgbe_vlan_strip_disable(adapter);
	}
}

static void rnpgbe_napi_enable_all(struct rnpgbe_adapter *adapter)
{
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++)
		napi_enable(&adapter->q_vector[q_idx]->napi);
}

static void rnpgbe_napi_disable_all(struct rnpgbe_adapter *adapter)
{
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++)
		napi_disable(&adapter->q_vector[q_idx]->napi);
}

static void rnpgbe_fdir_filter_restore(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct hlist_node *node2;
	struct rnpgbe_fdir_filter *filter;

	spin_lock(&adapter->fdir_perfect_lock);

	/* setup ntuple */
	hlist_for_each_entry_safe(filter, node2, &adapter->fdir_filter_list,
			fdir_node) {
		if ((!filter->vf_num) && (filter->action != ACTION_TO_MPE)) {
			rnpgbe_fdir_write_perfect_filter(
					adapter->fdir_mode, hw, &filter->filter, filter->hw_idx,
					(filter->action == RNP_FDIR_DROP_QUEUE) ?
					RNP_FDIR_DROP_QUEUE :
					adapter->rx_ring[filter->action]->rnpgbe_queue_idx,
					(adapter->priv_flags & RNP_PRIV_FLAG_REMAP_PRIO) ?
					true :
					false);
		} else {
			rnpgbe_fdir_write_perfect_filter(
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

static void rnpgbe_configure_pause(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;

	hw->ops.set_pause_mode(hw);
}

static void rnpgbe_vlan_stags_flag(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;

	/* stags is added */
	if (adapter->flags2 & RNP_FLAG2_VLAN_STAGS_ENABLED)
		hw->ops.set_txvlan_mode(hw, false);
	else
		hw->ops.set_txvlan_mode(hw, true);
}

static void rnpgbe_configure(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	bool sriov_flag = !!(adapter->flags & RNP_FLAG_SRIOV_ENABLED);

	/*
	 * We must restore virtualization before VLANs or else
	 * the VLVF registers will not be populated
	 */
	rnpgbe_configure_virtualization(adapter);

	rnpgbe_set_rx_mode(adapter->netdev);
	/* reconfigure hw */
	hw->ops.set_mac(hw, hw->mac.addr, sriov_flag);

	/* in sriov mode vlan is not reset */
	rnpgbe_restore_vlan(adapter);

	hw->ops.update_hw_info(hw);

	/* init setup pause */
	rnpgbe_configure_pause(adapter);

	rnpgbe_vlan_stags_flag(adapter);

	rnpgbe_init_rss_key(adapter);
	rnpgbe_init_rss_table(adapter);

	if (adapter->flags & RNP_FLAG_FDIR_HASH_CAPABLE) {

	} else if (adapter->flags & RNP_FLAG_FDIR_PERFECT_CAPABLE)
		rnpgbe_fdir_filter_restore(adapter);

	rnpgbe_configure_tx(adapter);
	rnpgbe_configure_rx(adapter);
}

/**
 * rnpgbe_sfp_link_config - set up SFP+ link
 * @adapter: pointer to private adapter struct
 **/
static void rnpgbe_sfp_link_config(struct rnpgbe_adapter *adapter)
{
	/*
	 * We are assuming the worst case scenario here, and that
	 * is that an SFP was inserted/removed after the reset
	 * but before SFP detection was enabled.  As such the best
	 * solution is to just start searching as soon as we start
	 */
	adapter->flags2 |= RNP_FLAG2_SFP_NEEDS_RESET;
}

static void rnpgbe_up_complete(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	int i;

	rnpgbe_configure_msix(adapter);

	/* enable the optics for n10 SFP+ fiber */
	if (hw->ops.enable_tx_laser)
		hw->ops.enable_tx_laser(hw);

	/* we need this */
	smp_mb__before_atomic();
	clear_bit(__RNP_DOWN, &adapter->state);
	rnpgbe_napi_enable_all(adapter);

	rnpgbe_sfp_link_config(adapter);
	/*clear any pending interrupts*/
	rnpgbe_irq_enable(adapter);

	/* enable transmits */
	netif_tx_start_all_queues(adapter->netdev);

	/* enable rx transmit */
	/* setup rx scater */
	for (i = 0; i < adapter->num_rx_queues; i++)
		ring_wr32(adapter->rx_ring[i], RNP_DMA_RX_START, 1);

	/* bring the link up in the watchdog, this could race with our first
	 * link up interrupt but shouldn't be a problems
	 */
	adapter->flags |= RNP_FLAG_NEED_LINK_UPDATE;
	adapter->link_check_timeout = jiffies;
	mod_timer(&adapter->service_timer, jiffies);

	/* Set PF Reset Done bit so PF/VF Mail Ops can work */
	/* maybe differ in n500 */
	hw->link = 0;
	hw->ops.set_mbx_link_event(hw, 1);
	hw->ops.set_mbx_ifup(hw, 1);

}

void rnpgbe_reinit_locked(struct rnpgbe_adapter *adapter)
{
	WARN_ON(in_interrupt());

	while (test_and_set_bit(__RNP_RESETTING, &adapter->state))
		usleep_range(1000, 2000);
	rnpgbe_down(adapter);
	/*
	 * If SR-IOV enabled then wait a bit before bringing the adapter
	 * back up to give the VFs time to respond to the reset.  The
	 * two second wait is based upon the watchdog timer cycle in
	 * the VF driver.
	 */
	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED)
		msleep(2000);
	rnpgbe_up(adapter);

	clear_bit(__RNP_RESETTING, &adapter->state);
}

void rnpgbe_up(struct rnpgbe_adapter *adapter)
{
	/* hardware has been reset, we need to reload some things */
	rnpgbe_configure(adapter);
	rnpgbe_up_complete(adapter);
}

void rnpgbe_reset(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	int err;
	bool sriov_flag = !!(adapter->flags & RNP_FLAG_SRIOV_ENABLED);

	rnpgbe_logd(LOG_ADPT_STAT, "%s\n", __func__);

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
			rnpgbe_ptp_reset(adapter);
	}
}

#ifdef OPTM_WITH_LPAGE
/**
 * rnpgbe_clean_rx_ring - Free Rx Buffers per Queue
 * @rx_ring: ring to free buffers from
 **/
static void rnpgbe_clean_rx_ring(struct rnpgbe_ring *rx_ring)
{
	u16 i = rx_ring->next_to_clean;
	struct rnpgbe_rx_buffer *rx_buffer;

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
					      rnpgbe_rx_bufsz(rx_ring),
					      DMA_FROM_DEVICE);

		/* free resources associated with mapping */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnpgbe_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE,
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
 * rnpgbe_clean_rx_ring - Free Rx Buffers per Queue
 * @rx_ring: ring to free buffers from
 **/
static void rnpgbe_clean_rx_ring(struct rnpgbe_ring *rx_ring)
{
	u16 i = rx_ring->next_to_clean;
	struct rnpgbe_rx_buffer *rx_buffer = &rx_ring->rx_buffer_info[i];

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
					      rnpgbe_rx_bufsz(rx_ring),
					      DMA_FROM_DEVICE);

		/* free resources associated with mapping */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnpgbe_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE,
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
 * rnpgbe_clean_tx_ring - Free Tx Buffers
 * @tx_ring: ring to be cleaned
 **/
static void rnpgbe_clean_tx_ring(struct rnpgbe_ring *tx_ring)
{
	unsigned long size;
	u16 i = tx_ring->next_to_clean;
	struct rnpgbe_tx_buffer *tx_buffer = &tx_ring->tx_buffer_info[i];

	BUG_ON(tx_ring == NULL);

	/* ring already cleared, nothing to do */
	if (!tx_ring->tx_buffer_info)
		return;

	while (i != tx_ring->next_to_use) {
		struct rnpgbe_tx_desc *eop_desc, *tx_desc;

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
	size = sizeof(struct rnpgbe_tx_buffer) * tx_ring->count;
	memset(tx_ring->tx_buffer_info, 0, size);

	/* Zero out the descriptor ring */
	memset(tx_ring->desc, 0, tx_ring->size);
	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;
}

/**
 * rnpgbe_clean_all_rx_rings - Free Rx Buffers for all queues
 * @adapter: board private structure
 **/
static void rnpgbe_clean_all_rx_rings(struct rnpgbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		rnpgbe_clean_rx_ring(adapter->rx_ring[i]);
}

/**
 * rnpgbe_clean_all_tx_rings - Free Tx Buffers for all queues
 * @adapter: board private structure
 **/
static void rnpgbe_clean_all_tx_rings(struct rnpgbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		rnpgbe_clean_tx_ring(adapter->tx_ring[i]);
}

static void rnpgbe_fdir_filter_exit(struct rnpgbe_adapter *adapter)
{
	struct hlist_node *node2;
	struct rnpgbe_fdir_filter *filter;
	struct rnpgbe_hw *hw = &adapter->hw;

	spin_lock(&adapter->fdir_perfect_lock);

	hlist_for_each_entry_safe (filter, node2, &adapter->fdir_filter_list,
				   fdir_node) {
		/* call earase to hw */
		rnpgbe_fdir_erase_perfect_filter(adapter->fdir_mode, hw,
						 &filter->filter,
						 filter->hw_idx);

		hlist_del(&filter->fdir_node);
		kfree(filter);
	}
	adapter->fdir_filter_count = 0;

	adapter->layer2_count = hw->layer2_count;
	adapter->tuple_5_count = hw->tuple5_count;

	spin_unlock(&adapter->fdir_perfect_lock);
}

static int rnpgbe_xmit_nop_frame_ring(struct rnpgbe_adapter *adapter,
			       struct rnpgbe_ring *tx_ring)
{
	u16 i = tx_ring->next_to_use;
	struct rnpgbe_tx_desc *tx_desc;

	tx_desc = RNP_TX_DESC(tx_ring, i);

	/* set length to 0 */
	tx_desc->blen_mac_ip_len = 0;
	tx_desc->vlan_cmd = cpu_to_le32(RNP_TXD_CMD_EOP | RNP_TXD_CMD_RS);
	/* update tail */
	rnpgbe_wr_reg(tx_ring->tail, 0);
	return 0;
}

static void print_status(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct rnpgbe_eth_info *eth = &hw->eth;
	int i;
	struct rnpgbe_dma_info *dma = &hw->dma;
	struct net_device *netdev = adapter->netdev;

	netdev_dbg(netdev, "eth 0x120 %x\n", eth_rd32(eth, 0x120));
	netdev_dbg(netdev, "eth 0x124 %x\n", eth_rd32(eth, 0x124));

	for (i = 0x200; i < 0x220; i = i + 4)
		netdev_dbg(netdev, "eth 0x%x %x\n", i, eth_rd32(eth, i));

	for (i = 0x300; i < 0x318; i = i + 4)
		netdev_dbg(netdev, "eth 0x%x %x\n", i, eth_rd32(eth, i));

	netdev_dbg(netdev, "eth 0x%x %x\n", 0x98, eth_rd32(eth, 0x98));
	netdev_dbg(netdev, "eth 0x%x %x\n", 0x220, eth_rd32(eth, 0x220));

	for (i = 0x138; i < 0x158; i = i + 4)
		netdev_dbg(netdev, "dma 0x%x %x\n", i, dma_rd32(dma, i));
	i = 0x170;
	netdev_dbg(netdev, "dma 0x%x %x\n", i, dma_rd32(dma, i));
	i = 0x174;
	netdev_dbg(netdev, "dma 0x%x %x\n", i, dma_rd32(dma, i));
	for (i = 0x214; i < 0x220; i = i + 4)
		netdev_dbg(netdev, "dma 0x%x %x\n", i, dma_rd32(dma, i));
	for (i = 0x234; i < 0x270; i = i + 4)
		netdev_dbg(netdev, "dma 0x%x %x\n", i, dma_rd32(dma, i));
	i = 0x1018;
	netdev_dbg(netdev, "dma 0x%x %x\n", i, dma_rd32(dma, i));
	i = 0x101c;
	netdev_dbg(netdev, "dma 0x%x %x\n", i, dma_rd32(dma, i));
	i = 0x1084;
	netdev_dbg(netdev, "dma 0x%x %x\n", i, dma_rd32(dma, i));
}

void rnpgbe_down(struct rnpgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct rnpgbe_hw *hw = &adapter->hw;
	int i;
	int free_tx_ealay = 0;
	int err = 0;
	/* signal that we are down to the interrupt handler */
	set_bit(__RNP_DOWN, &adapter->state);

	if (!hw->ncsi_en)
		hw->ops.set_mac_rx(hw, false);

	hw->ops.set_mbx_link_event(hw, 0);
	hw->ops.set_mbx_ifup(hw, 0);

	rnpgbe_setup_eee_mode(adapter, false);

	if (hw->ops.clean_link)
		hw->ops.clean_link(hw);

	if (netif_carrier_ok(netdev))
		e_info(drv, "NIC Link is Down\n");

	rnpgbe_remove_vlan(adapter);

	netif_tx_stop_all_queues(netdev);

	netif_carrier_off(netdev);

	usleep_range(5000, 10000);
	/* if we have tx desc to clean */
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct rnpgbe_ring *tx_ring = adapter->tx_ring[i];

		if (!(tx_ring->ring_flags & RNP_RING_SKIP_TX_START)) {
			int head, tail;
			int timeout = 0;

			free_tx_ealay = 1;
			if (tx_ring->next_to_use == tx_ring->next_to_clean)
				continue;

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
					       "wait ring %d tx done timeout %x %x\n",
					       i, head, tail);
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
		rnpgbe_clean_all_tx_rings(adapter);

	usleep_range(2000, 5000);

	rnpgbe_irq_disable(adapter);

	usleep_range(5000, 10000);

	netif_tx_disable(netdev);

	/* disable all enabled rx queues */
	for (i = 0; i < adapter->num_rx_queues; i++) {
		rnpgbe_disable_rx_queue(adapter, adapter->rx_ring[i]);
		/* only handle when srio enable and change rx length setup */
		if ((adapter->flags & RNP_FLAG_SRIOV_ENABLED) &&
		    (adapter->rx_ring[i]->ring_flags &
		     RNP_RING_FLAG_CHANGE_RX_LEN)) {
			int head;
			struct rnpgbe_ring *ring = adapter->rx_ring[i];

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

	rnpgbe_napi_disable_all(adapter);

	adapter->flags2 &=
		~(RNP_FLAG2_FDIR_REQUIRES_REINIT | RNP_FLAG2_RESET_REQUESTED);
	adapter->flags &= ~RNP_FLAG_NEED_LINK_UPDATE;

	/* ping all the active vfs to let them know we are going down */
	if (adapter->num_vfs)
		rnpgbe_ping_all_vfs(adapter);

	/* disable transmits in the hardware now that interrupts are off */
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct rnpgbe_ring *tx_ring = adapter->tx_ring[i];
		int count = tx_ring->count;
		int head;
		int timeout = 0;

		/* 1. stop queue */
		if (!err) {
			if (!(tx_ring->ring_flags & RNP_RING_SKIP_TX_START))
				ring_wr32(tx_ring, RNP_DMA_TX_START, 0);
		}
		/* 2. try to set tx head to 0 in sriov mode
		 * since we don't reset
		 */
		if ((adapter->flags & RNP_FLAG_SRIOV_ENABLED) &&
		    (!(tx_ring->ring_flags & RNP_RING_SIZE_CHANGE_FIX))) {
			/* only do this if hw not support tx head to zero auto */

			/* n10 should wait tx_ready */
			u32 status = 0;

			timeout = 0;
			do {
				status = ring_rd32(tx_ring, RNP_DMA_TX_READY);
				usleep_range(100, 200);
				timeout++;
				rnpgbe_dbg("wait %d tx ready to 1\n",
					   tx_ring->rnpgbe_queue_idx);
			} while ((status != 1) && (timeout < 100));

			if (timeout >= 100)
				rnpgbe_dbg("wait tx ready timeout\n");

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
				rnpgbe_xmit_nop_frame_ring(adapter, tx_ring);
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
					rnpgbe_dbg(
						"[%s] Wait Tx-ring %d head to zero time out\n",
						netdev->name,
						tx_ring->rnpgbe_queue_idx);
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
			if (!(adapter->flags & RNP_FLAG_SRIOV_ENABLED))
				rnpgbe_reset(adapter);
			else if (!(adapter->flags & RNP_FLAG_SRIOV_INIT_DONE))
				rnpgbe_reset(adapter);
		}
	}
	/* power down the optics for n10 SFP+ fiber */
	if (hw->ops.disable_tx_laser)
		hw->ops.disable_tx_laser(hw);

	if (!free_tx_ealay)
		rnpgbe_clean_all_tx_rings(adapter);

	rnpgbe_clean_all_rx_rings(adapter);
}

/**
 * rnpgbe_tx_timeout - Respond to a Tx Hang
 * @netdev: network interface device structure
 **/
static void rnpgbe_tx_timeout(struct net_device *netdev, unsigned int txqueue)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	/* Do the reset outside of interrupt context */
	int i;
	bool real_tx_hang = false;

#define TX_TIMEO_LIMIT 16000
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct rnpgbe_ring *tx_ring = adapter->tx_ring[i];

		if (check_for_tx_hang(tx_ring) &&
		    rnpgbe_check_tx_hang(tx_ring)) {
			real_tx_hang = true;
		}
	}

	if (real_tx_hang) {
		e_info(drv, "hw real hang!!!!");
		/* Do the reset outside of interrupt context */
		rnpgbe_tx_timeout_reset(adapter);
	} else {
		e_info(drv,
		       "Fake Tx hang detected with timeout of %d "
		       "seconds\n",
		       netdev->watchdog_timeo / HZ);
		for (i = 0; i < adapter->num_tx_queues; i++) {
			struct rnpgbe_ring *temp_ring = adapter->tx_ring[i];
			u32 head, tail;
			struct rnpgbe_hw *hw = &adapter->hw;

			head = ring_rd32(temp_ring,
					 RNP_DMA_REG_TX_DESC_BUF_HEAD);
			tail = ring_rd32(temp_ring,
					 RNP_DMA_REG_TX_DESC_BUF_TAIL);
			e_info(drv, "sw ring %d ---- %d %d\n",
			       temp_ring->rnpgbe_queue_idx,
			       temp_ring->next_to_use,
			       temp_ring->next_to_clean);
			e_info(drv, "hw ring %d ---- %d %d\n",
			       temp_ring->rnpgbe_queue_idx, head, tail);
			e_info(drv, "dma version %d\n",
			       rnpgbe_rd_reg(hw->hw_addr));
		}
		print_status(adapter);
		/* fake Tx hang - increase the kernel timeout */
		if (netdev->watchdog_timeo < TX_TIMEO_LIMIT)
			netdev->watchdog_timeo *= 2;
	}
}

/**
 * rnpgbe_sw_init - Initialize general software structures (struct rnpgbe_adapter)
 * @adapter: board private structure to initialize
 *
 * rnpgbe_sw_init initializes the Adapter private data structure.
 * Fields are initialized based on PCI device information and
 * OS network device settings (MTU size).
 **/
static int rnpgbe_sw_init(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct pci_dev *pdev = adapter->pdev;
	unsigned int rss = 0, fdir;
	int rss_limit = num_online_cpus();

	hw->vendor_id = pdev->vendor;
	hw->device_id = pdev->device;
	hw->subsystem_vendor_id = pdev->subsystem_vendor;
	hw->subsystem_device_id = pdev->subsystem_device;

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

	mutex_init(&adapter->eee_lock);
	adapter->tx_lpi_timer = RNP_DEFAULT_TWT_LS;

	/* itr sw setup here */
	adapter->sample_interval = 1;
	adapter->adaptive_rx_coal = 1;
	adapter->adaptive_tx_coal = 1;
	adapter->auto_rx_coal = 0;
	adapter->napi_budge = 64;
	/* set default work limits */
	adapter->tx_work_limit = RNP_DEFAULT_TX_WORK;
	adapter->rx_usecs = RNP_PKT_TIMEOUT;
	adapter->rx_frames = RNP_RX_PKT_POLL_BUDGET;
	adapter->priv_flags &= ~RNP_PRIV_FLAG_RX_COALESCE;
	adapter->priv_flags &= ~RNP_PRIV_FLAG_TX_COALESCE;
	adapter->tx_usecs = RNP_PKT_TIMEOUT_TX;
	adapter->tx_frames = RNP_TX_PKT_POLL_BUDGET;

	/* n-tuple support exists, always init our spinlock */
	/* init fdir count */
	/* enable itr by default in dynamic mode */
	/* set default ring sizes */
	adapter->tx_ring_item_count = RNP_DEFAULT_TXD;
	adapter->rx_ring_item_count = RNP_DEFAULT_RXD;
	set_bit(__RNP_DOWN, &adapter->state);

	return 0;
}

/**
 * rnpgbe_setup_tx_resources - allocate Tx resources (Descriptors)
 * @tx_ring:    tx descriptor ring (for a specific queue) to setup
 *
 * Return 0 on success, negative on failure
 **/

int rnpgbe_setup_tx_resources(struct rnpgbe_ring *tx_ring,
			      struct rnpgbe_adapter *adapter)
{
	struct device *dev = tx_ring->dev;
	int orig_node = dev_to_node(dev);
	int numa_node = NUMA_NO_NODE;
	int size;

	size = sizeof(struct rnpgbe_tx_buffer) * tx_ring->count;

	if (tx_ring->q_vector)
		numa_node = tx_ring->q_vector->numa_node;
	tx_ring->tx_buffer_info = vzalloc_node(size, numa_node);
	if (!tx_ring->tx_buffer_info)
		tx_ring->tx_buffer_info = vzalloc(size);
	if (!tx_ring->tx_buffer_info)
		goto err;
	/* round up to nearest 4K */
	tx_ring->size = tx_ring->count * sizeof(struct rnpgbe_tx_desc);
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
		tx_ring->rnpgbe_queue_idx, tx_ring->q_vector->v_idx,
		tx_ring->count, tx_ring->desc, tx_ring->dma, numa_node);
	return 0;

err:

	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;
	dev_err(dev, "Unable to allocate memory for the Tx descriptor ring\n");
	return -ENOMEM;
}

/**
 * rnpgbe_setup_all_tx_resources - allocate all queues Tx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int rnpgbe_setup_all_tx_resources(struct rnpgbe_adapter *adapter)
{
	int i, err = 0;

	tx_dbg("adapter->num_tx_queues:%d, adapter->tx_ring[0]:%p\n",
	       adapter->num_tx_queues, adapter->tx_ring[0]);

	for (i = 0; i < (adapter->num_tx_queues); i++) {
		BUG_ON(adapter->tx_ring[i] == NULL);
		err = rnpgbe_setup_tx_resources(adapter->tx_ring[i], adapter);
		if (!err)
			continue;

		e_err(probe, "Allocation for Tx Queue %u failed\n", i);
		goto err_setup_tx;
	}

	return 0;
err_setup_tx:
	/* rewind the index freeing the rings as we go */
	while (i--)
		rnpgbe_free_tx_resources(adapter->tx_ring[i]);
	return err;
}

/**
 * rnpgbe_setup_rx_resources - allocate Rx resources (Descriptors)
 * @rx_ring:    rx descriptor ring (for a specific queue) to setup
 *
 * Returns 0 on success, negative on failure
 **/
int rnpgbe_setup_rx_resources(struct rnpgbe_ring *rx_ring,
			      struct rnpgbe_adapter *adapter)
{
	struct device *dev = rx_ring->dev;
	int orig_node = dev_to_node(dev);
	int numa_node = NUMA_NO_NODE;
	int size;

	BUG_ON(rx_ring == NULL);

	size = sizeof(struct rnpgbe_rx_buffer) * rx_ring->count;

	if (rx_ring->q_vector)
		numa_node = rx_ring->q_vector->numa_node;

	rx_ring->rx_buffer_info = vzalloc_node(size, numa_node);
	if (!rx_ring->rx_buffer_info)
		rx_ring->rx_buffer_info = vzalloc(size);
	if (!rx_ring->rx_buffer_info)
		goto err;
	/* Round up to nearest 4K */
	rx_ring->size = rx_ring->count * sizeof(union rnpgbe_rx_desc);
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
		rx_ring->rnpgbe_queue_idx, rx_ring->q_vector->v_idx,
		rx_ring->count, rx_ring->desc, rx_ring->dma, numa_node);

	return 0;
err:

	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;
	dev_err(dev, "Unable to allocate memory for the Rx descriptor ring\n");
	return -ENOMEM;
}

/**
 * rnpgbe_setup_all_rx_resources - allocate all queues Rx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int rnpgbe_setup_all_rx_resources(struct rnpgbe_adapter *adapter)
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
			    adapter->rx_ring[i]->rnpgbe_queue_idx);
			adapter->rx_ring[i]->ring_flags |=
				RNP_RING_FLAG_DELAY_SETUP_RX_LEN;
			adapter->rx_ring[i]->reset_count =
				adapter->rx_ring[i]->count;
			adapter->rx_ring[i]->count = head + 1;
		}
		err = rnpgbe_setup_rx_resources(adapter->rx_ring[i], adapter);
		if (!err)
			continue;

		e_err(probe, "Allocation for Rx Queue %u failed\n", i);
		goto err_setup_rx;
	}

	return 0;
err_setup_rx:
	/* rewind the index freeing the rings as we go */
	while (i--)
		rnpgbe_free_rx_resources(adapter->rx_ring[i]);
	return err;
}

/**
 * rnpgbe_free_tx_resources - Free Tx Resources per Queue
 * @tx_ring: Tx descriptor ring for a specific queue
 *
 * Free all transmit software resources
 **/
void rnpgbe_free_tx_resources(struct rnpgbe_ring *tx_ring)
{
	BUG_ON(tx_ring == NULL);

	rnpgbe_clean_tx_ring(tx_ring);
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
 * rnpgbe_free_all_tx_resources - Free Tx Resources for All Queues
 * @adapter: board private structure
 *
 * Free all transmit software resources
 **/
static void rnpgbe_free_all_tx_resources(struct rnpgbe_adapter *adapter)
{
	int i;

	for (i = 0; i < (adapter->num_tx_queues); i++)
		rnpgbe_free_tx_resources(adapter->tx_ring[i]);
}

/**
 * rnpgbe_free_rx_resources - Free Rx Resources
 * @rx_ring: ring to clean the resources from
 *
 * Free all receive software resources
 **/
void rnpgbe_free_rx_resources(struct rnpgbe_ring *rx_ring)
{
	BUG_ON(rx_ring == NULL);

	rnpgbe_clean_rx_ring(rx_ring);

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
 * rnpgbe_free_all_rx_resources - Free Rx Resources for All Queues
 * @adapter: board private structure
 *
 * Free all receive software resources
 **/
static void rnpgbe_free_all_rx_resources(struct rnpgbe_adapter *adapter)
{
	int i;

	for (i = 0; i < (adapter->num_rx_queues); i++) {
		if (adapter->rx_ring[i]->desc)
			rnpgbe_free_rx_resources(adapter->rx_ring[i]);
	}
}

/**
 * rnpgbe_change_mtu - Change the Maximum Transfer Unit
 * @netdev: network interface device structure
 * @new_mtu: new value for maximum frame size
 *
 * Returns 0 on success, negative on failure
 **/
static int rnpgbe_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;
	int max_frame = new_mtu + ETH_HLEN + ETH_FCS_LEN * 2;

	/* MTU < 68 is an error and causes problems on some kernels */
	if ((new_mtu < hw->min_length) || (max_frame > hw->max_length))
		return -EINVAL;

	e_info(probe, "changing MTU from %d to %d\n", netdev->mtu, new_mtu);

	if (netdev->mtu == new_mtu)
		return 0;

	/* must set new MTU before calling down or up */
	netdev->mtu = new_mtu;

	rnpgbe_msg_post_status(adapter, PF_SET_MTU);

	if (netif_running(netdev))
		rnpgbe_reinit_locked(adapter);

	return 0;
}

/**
 * rnpgbe_tx_maxrate - callback to set the maximum per-queue bitrate
 * @netdev: network interface device structure
 * @queue_index: Tx queue to set
 * @maxrate: desired maximum transmit bitrate Mbps
 **/
__maybe_unused static int rnpgbe_tx_maxrate(struct net_device *netdev,
					    int queue_index, u32 maxrate)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_ring *tx_ring = adapter->tx_ring[queue_index];
	u64 real_rate = 0;

	adapter->max_rate[queue_index] = maxrate;
	rnpgbe_dbg("%s: queue:%d maxrate:%d\n", __func__, queue_index, maxrate);
	if (!maxrate)
		return rnpgbe_setup_tx_maxrate(
			tx_ring, 0, adapter->hw.usecstocount * 100000);
	/* we need turn it to bytes/s */
	real_rate = ((u64)maxrate * 1000 * 94) >> 3;
	rnpgbe_setup_tx_maxrate(tx_ring, real_rate,
				adapter->hw.usecstocount * 100000);

	return 0;
}

/**
 * rnpgbe_open - Called when a network interface is made active
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
static int rnpgbe_open(struct net_device *netdev)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;
	int err;

	DPRINTK(IFUP, INFO, "ifup\n");

	/* disallow open during test */
	if (test_bit(__RNP_TESTING, &adapter->state))
		return -EBUSY;

	netif_carrier_off(netdev);

	/* allocate transmit descriptors */
	err = rnpgbe_setup_all_tx_resources(adapter);
	if (err)
		goto err_setup_tx;

	/* allocate receive descriptors */
	err = rnpgbe_setup_all_rx_resources(adapter);
	if (err)
		goto err_setup_rx;

	rnpgbe_configure(adapter);

	err = rnpgbe_request_irq(adapter);
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
		rnpgbe_ptp_register(adapter);

	rnpgbe_up_complete(adapter);

	return 0;

err_set_queues:
	rnpgbe_free_irq(adapter);
err_req_irq:
	rnpgbe_free_all_rx_resources(adapter);
err_setup_rx:
	rnpgbe_free_all_tx_resources(adapter);
err_setup_tx:
	hw->ops.set_mbx_ifup(hw, 0);
	rnpgbe_reset(adapter);

	return err;
}

/**
 * rnpgbe_close - Disables a network interface
 * @netdev: network interface device structure
 *
 * Returns 0, this is not allowed to fail
 *
 * The close entry point is called when an interface is de-activated
 * by the OS.  The hardware is still under the drivers control, but
 * needs to be disabled.  A global MAC reset is issued to stop the
 * hardware, and all transmit and receive resources are freed.
 **/
static int rnpgbe_close(struct net_device *netdev)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	DPRINTK(IFDOWN, INFO, "ifdown\n");

#ifdef DISABLE_RX_IRQ
	adapter->quit_poll_thread = true;
#endif

	if (module_enable_ptp)
		rnpgbe_ptp_unregister(adapter);

	rnpgbe_down(adapter);

	rnpgbe_free_irq(adapter);

	rnpgbe_free_all_tx_resources(adapter);
	rnpgbe_free_all_rx_resources(adapter);

	/* if in sriov mode send link down to all vfs */
	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED) {
		adapter->link_up = 0;
		adapter->link_up_old = 0;
		rnpgbe_msg_post_status(adapter, PF_SET_LINK_STATUS);
		/* wait all vf get this status */
		usleep_range(5000, 10000);
	}

	return 0;
}

static int rnpgbe_resume(struct device *dev)
{
	struct rnpgbe_adapter *adapter;
	struct net_device *netdev;
	u32 err;
	struct rnpgbe_hw *hw;
	struct pci_dev *pdev = to_pci_dev(dev);

	adapter = pci_get_drvdata(pdev);
	hw = &adapter->hw;
	netdev = adapter->netdev;
	pr_info("call rnpgbe_resume\n");
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

	rtnl_lock();

	err = rnpgbe_init_interrupt_scheme(adapter);
	if (!err)
		err = register_mbx_irq(adapter);

	if (hw->ops.driver_status)
		hw->ops.driver_status(hw, false, rnpgbe_driver_suspuse);

	if (hw->ops.driver_status)
		hw->ops.driver_status(hw, true, rnpgbe_driver_insmod);

	rnpgbe_reset(adapter);

	/* we should setup link in default */
	hw->ops.setup_link(hw, DEFAULT_ADV, 1, 0, 0);
	hw->advertised_link = DEFAULT_ADV;

	if (!err) {
		if (netif_running(netdev)) {
			err = rnpgbe_open(netdev);
		} else {
			hw->ops.set_mbx_link_event(hw, 0);
			hw->ops.set_mbx_ifup(hw, 0);
		}
	}

	rtnl_unlock();

	if (err)
		return err;

	netif_device_attach(netdev);

	return 0;
}

/**
 * rnpgbe_freeze - quiesce the device (no IRQ's or DMA)
 * @dev: The port's netdev
 */
static int rnpgbe_freeze(struct device *dev)
{
	struct rnpgbe_adapter *adapter = pci_get_drvdata(to_pci_dev(dev));
	struct net_device *netdev = adapter->netdev;

	rtnl_lock();
	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		rnpgbe_down(adapter);
		rnpgbe_free_irq(adapter);

		rnpgbe_free_all_tx_resources(adapter);
		rnpgbe_free_all_rx_resources(adapter);
	}

	remove_mbx_irq(adapter);
	rnpgbe_clear_interrupt_scheme(adapter);
	rtnl_unlock();

	return 0;
}

/**
 * rnpgbe_thaw - un-quiesce the device
 * @dev: The port's netdev
 */
static int rnpgbe_thaw(struct device *dev)
{
	struct rnpgbe_adapter *adapter = pci_get_drvdata(to_pci_dev(dev));
	struct net_device *netdev = adapter->netdev;
	u32 err;

	rtnl_lock();
	err = rnpgbe_init_interrupt_scheme(adapter);

	if (netif_running(netdev))
		err = rnpgbe_open(netdev);

	rtnl_unlock();

	if (err)
		return err;

	netif_device_attach(netdev);

	return 0;
}

static int __rnpgbe_shutdown_suspuse(struct pci_dev *pdev, bool *enable_wake)
{
	struct rnpgbe_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
	struct rnpgbe_hw *hw = &adapter->hw;
	u32 wufc = adapter->wol;
	int retval = 0;

	netif_device_detach(netdev);

	rtnl_lock();
	if (netif_running(netdev)) {
		rnpgbe_down(adapter);
		rnpgbe_free_irq(adapter);
		rnpgbe_free_all_tx_resources(adapter);
		rnpgbe_free_all_rx_resources(adapter);
		/* should consider sriov mode ? */
	}
	rtnl_unlock();

	/* if we open wol or ncsi_en, we must send this to hw */
	if ((hw->ncsi_en || adapter->wol) && hw->ops.driver_status)
		hw->ops.driver_status(hw, true, rnpgbe_driver_suspuse);

	remove_mbx_irq(adapter);
	rnpgbe_clear_interrupt_scheme(adapter);

	retval = pci_save_state(pdev);
	if (retval)
		return retval;

	if (wufc) {
		rnpgbe_set_rx_mode(netdev);

		/* enable the optics for n10 SFP+ fiber as we can WoL */
		if (hw->ops.enable_tx_laser)
			hw->ops.enable_tx_laser(hw);

		/* turn on all-multi mode if wake on multicast is enabled */

	} else {
	}

	if (hw->ops.setup_wol)
		hw->ops.setup_wol(hw, adapter->wol);

	pci_wake_from_d3(pdev, !!wufc);
	*enable_wake = !!wufc;

	pci_disable_device(pdev);

	return 0;
}

static int __rnpgbe_shutdown(struct pci_dev *pdev, bool *enable_wake)
{
	struct rnpgbe_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
	struct rnpgbe_hw *hw = &adapter->hw;
	u32 wufc = adapter->wol;
	int retval = 0;

	netif_device_detach(netdev);

	rtnl_lock();
	if (netif_running(netdev)) {
		rnpgbe_down(adapter);
		rnpgbe_free_irq(adapter);
		rnpgbe_free_all_tx_resources(adapter);
		rnpgbe_free_all_rx_resources(adapter);
		/* should consider sriov mode ? */
	}
	rtnl_unlock();

	/* only send mbx if ncsi or wol on */
	if ((hw->ncsi_en || adapter->wol) && hw->ops.driver_status)
		hw->ops.driver_status(hw, false, rnpgbe_driver_insmod);

	remove_mbx_irq(adapter);
	rnpgbe_clear_interrupt_scheme(adapter);

	retval = pci_save_state(pdev);
	if (retval)
		return retval;

	if (wufc) {
		rnpgbe_set_rx_mode(netdev);

		/* enable the optics for n10 SFP+ fiber as we can WoL */
		if (hw->ops.enable_tx_laser)
			hw->ops.enable_tx_laser(hw);

		/* turn on all-multi mode if wake on multicast is enabled */

	} else {
	}

	if (hw->ops.setup_wol)
		hw->ops.setup_wol(hw, adapter->wol);

	pci_wake_from_d3(pdev, !!wufc);
	*enable_wake = !!wufc;

	pci_disable_device(pdev);

	return 0;
}

static int rnpgbe_suspend(struct device *dev)
{
	int retval;
	bool wake;
	struct pci_dev *pdev = to_pci_dev(dev);

	pr_info("call rnpgbe_suspend\n");

	retval = __rnpgbe_shutdown_suspuse(pdev, &wake);
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

__maybe_unused static void rnpgbe_shutdown(struct pci_dev *pdev)
{
	bool wake = false;

	pr_info("call rnpgbe_shutdown\n");

	__rnpgbe_shutdown(pdev, &wake);

	pr_info("call rnpgbe_shutdown wake %x\n", wake);

	if (system_state == SYSTEM_POWER_OFF) {
		pci_wake_from_d3(pdev, wake);
		pci_set_power_state(pdev, PCI_D3hot);
	}
	pr_info("call rnpgbe_shutdown done\n");
}

/**
 * rnpgbe_update_stats - Update the board statistics counters.
 * @adapter: board private structure
 **/
void rnpgbe_update_stats(struct rnpgbe_adapter *adapter)
{
	struct net_device_stats *net_stats = &adapter->netdev->stats;
	struct rnpgbe_hw *hw = &adapter->hw;
	struct rnpgbe_hw_stats *hw_stats = &adapter->hw_stats;

	int i;
	struct rnpgbe_ring *ring;
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
		rnpgbe_for_each_ring(ring, adapter->q_vector[i]->rx) {
			hw_csum_rx_error += ring->rx_stats.csum_err;
			hw_csum_rx_good += ring->rx_stats.csum_good;
			hw_stats->vlan_strip_cnt += ring->rx_stats.vlan_remove;
			net_stats->rx_packets += ring->stats.packets;
			net_stats->rx_bytes += ring->stats.bytes;
		}

		rnpgbe_for_each_ring(ring, adapter->q_vector[i]->tx) {
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
 * rnpgbe_watchdog_update_link - update the link status
 * @adapter: pointer to the device adapter structure
 * @link_speed: pointer to a u32 to store the link_speed
 **/
static void rnpgbe_watchdog_update_link(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
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
	case rnpgbe_fc_none:
		flow_rx = false;
		flow_tx = false;
		break;
	case rnpgbe_fc_tx_pause:
		flow_rx = false;
		flow_tx = true;

		break;
	case rnpgbe_fc_rx_pause:
		flow_rx = true;
		flow_tx = false;
		break;

	case rnpgbe_fc_full:
		flow_rx = true;
		flow_tx = true;
		break;
	default:
		hw_dbg(hw, "Flow control param set incorrectly\n");
	}

	if (adapter->link_up) {
		if (hw->ops.set_mac_speed)
			hw->ops.set_mac_speed(hw, true, link_speed, duplex);
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
							   "1000 Mbps" :
							   (link_speed == RNP_LINK_SPEED_100_FULL ?
								    "100 Mbps" :
								    (link_speed == RNP_LINK_SPEED_10_FULL ?
									     "10 Mbps" :
									     "unknown speed")))))),
		       ((duplex) ? "Full" : "Half"),
		       ((flow_rx && flow_tx) ?
				"RX/TX" :
				(flow_rx ? "RX" : (flow_tx ? "TX" : "None"))));
	} else {
		if (hw->ops.set_mac_speed)
			hw->ops.set_mac_speed(hw, false, 0, false);
	}
}

/**
 * rnpgbe_eee_ctrl_timer - EEE TX SW timer.
 * @arg : data hook
 * Description:
 *  if there is no data transfer and if we are not in LPI state,
 *  then MAC Transmitter can be moved to LPI state.
 */
static void rnpgbe_eee_ctrl_timer(struct timer_list *t)
{
	struct rnpgbe_adapter *adapter = from_timer(adapter, t, eee_ctrl_timer);

	rnpgbe_enable_eee_mode(adapter);
	if (!test_bit(__RNP_EEE_REMOVE, &adapter->state))
		mod_timer(&adapter->eee_ctrl_timer,
			  RNP_LPI_T(adapter->eee_timer));
}

static bool rnpgbe_eee_init(struct rnpgbe_adapter *adapter)
{
	int tx_lpi_timer = adapter->tx_lpi_timer;
	struct rnpgbe_hw *hw = &adapter->hw;

	mutex_lock(&adapter->eee_lock);

	/* Check if it needs to be deactivated */
	if (!adapter->eee_active) {
		set_bit(__RNP_EEE_REMOVE, &adapter->state);
		netdev_dbg(adapter->netdev, "disable EEE\n");
		del_timer_sync(&adapter->eee_ctrl_timer);
		hw->ops.set_eee_timer(hw, 0, tx_lpi_timer);
	} else {
		clear_bit(__RNP_EEE_REMOVE, &adapter->state);
		timer_setup(&adapter->eee_ctrl_timer, rnpgbe_eee_ctrl_timer, 0);
		mod_timer(&adapter->eee_ctrl_timer,
			  RNP_LPI_T(adapter->eee_timer));
		hw->ops.set_eee_timer(hw, RNP_DEFAULT_LIT_LS, tx_lpi_timer);
	}

	mutex_unlock(&adapter->eee_lock);
	netdev_dbg(adapter->netdev, "Energy-Efficient Ethernet initialized\n");
	return true;
}

static int rnpgbe_phy_init_eee(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;

	/* if hw no eee capability or eee closed by ethtool */
	if ((!hw->eee_capability) || (!adapter->eee_enabled))
		return -EIO;
	/* init eee only in full duplex */
	if (!hw->duplex)
		return -EIO;
	/* init eee not in speed 10 */
	if (hw->speed == 10)
		return -EIO;
	/* init eee only local and lp all support eee */
	if (!(adapter->local_eee & adapter->partner_eee))
		return -EIO;
	if ((hw->hw_type == rnpgbe_hw_n500) ||
	    (hw->hw_type == rnpgbe_hw_n210)) {
		/* n500 only support eee in 100/1000 full */
		if (!hw->duplex)
			return -EIO;

		if ((adapter->speed != RNP_LINK_SPEED_100_FULL) &&
		    (adapter->speed != RNP_LINK_SPEED_1GB_FULL))
			return -EIO;
	}
	/* if in sriov mode cannot open eee */
	if (adapter->flags & RNP_FLAG_SRIOV_ENABLED)
		return -EIO;

	return 0;
}

static void rnpgbe_setup_eee_mode(struct rnpgbe_adapter *adapter, bool status)
{
	struct rnpgbe_hw *hw = &adapter->hw;

	if (status) {
		/* if eee active before, first close it */
		if (adapter->eee_active) {
			adapter->eee_active = 0;
			rnpgbe_eee_init(adapter);
		}
		/* if up, try to active eee */
		adapter->eee_active = rnpgbe_phy_init_eee(adapter) >= 0;
		/* if we can active eee, init it */
		if (adapter->eee_active) {
			rnpgbe_eee_init(adapter);
			if (hw->ops.set_eee_pls)
				hw->ops.set_eee_pls(hw, true);
		}
	} else {
		/* if eee active before, close it */
		if (adapter->eee_active) {
			adapter->eee_active = 0;
			rnpgbe_eee_init(adapter);
		}
		if (hw->ops.set_eee_pls)
			hw->ops.set_eee_pls(hw, false);
	}
}

/**
 * rnpgbe_watchdog_link_is_up - update netif_carrier status and
 *                             print link up message
 * @adapter: pointer to the device adapter structure
 **/
static void rnpgbe_watchdog_link_is_up(struct rnpgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct rnpgbe_hw *hw = &adapter->hw;

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

	rnpgbe_setup_eee_mode(adapter, true);
}

/**
 * rnpgbe_watchdog_link_is_down - update netif_carrier status and
 *                               print link down message
 * @adapter: pointer to the adapter structure
 **/
static void rnpgbe_watchdog_link_is_down(struct rnpgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct rnpgbe_hw *hw = &adapter->hw;

	adapter->link_up = false;
	adapter->link_speed = 0;

	/* only continue if link was up previously */
	if (!netif_carrier_ok(netdev))
		return;

	/* poll for SFP+ cable when link is down */
	adapter->flags2 |= RNP_FLAG2_SEARCH_FOR_SFP;

	e_info(drv, "NIC Link is Down\n");

	netif_carrier_off(netdev);

	netif_tx_stop_all_queues(netdev);

	hw->ops.set_mac_rx(hw, false);

	rnpgbe_setup_eee_mode(adapter, false);
}

static void rnpgbe_update_link_to_vf(struct rnpgbe_adapter *adapter)
{
	if (!(adapter->flags & RNP_FLAG_VF_INIT_DONE))
		return;

	if ((adapter->link_up_old != adapter->link_up) ||
	    (adapter->link_speed_old != adapter->link_speed)) {
		if (!test_bit(__RNP_IN_IRQ, &adapter->state)) {
			if (rnpgbe_msg_post_status(adapter,
						   PF_SET_LINK_STATUS) == 0) {
				adapter->link_up_old = adapter->link_up;
				adapter->link_speed_old = adapter->link_speed;
			}
		}
	}
}

/**
 * rnpgbe_watchdog_subtask - check and bring link up
 * @adapter: pointer to the device adapter structure
 **/
static void rnpgbe_watchdog_subtask(struct rnpgbe_adapter *adapter)
{
	/* if interface is down do nothing */
	/* should do link status if in sriov */
	if (test_bit(__RNP_DOWN, &adapter->state) ||
	    test_bit(__RNP_RESETTING, &adapter->state))
		return;

	rnpgbe_watchdog_update_link(adapter);

	if (adapter->link_up)
		rnpgbe_watchdog_link_is_up(adapter);
	else
		rnpgbe_watchdog_link_is_down(adapter);

	rnpgbe_update_link_to_vf(adapter);

	rnpgbe_update_stats(adapter);
}

/**
 * rnpgbe_service_timer - Timer Call-back
 * @data: pointer to adapter cast into an unsigned long
 **/
void rnpgbe_service_timer(struct timer_list *t)
{
	struct rnpgbe_adapter *adapter = from_timer(adapter, t, service_timer);
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
		rnpgbe_service_event_schedule(adapter);
}

static void rnpgbe_reset_pf_subtask(struct rnpgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	u32 err;

	if (!(adapter->flags2 & RNP_FLAG2_RESET_PF))
		return;

	rtnl_lock();
	netif_device_detach(netdev);
	if (netif_running(netdev)) {
		rnpgbe_down(adapter);
		rnpgbe_free_irq(adapter);
		rnpgbe_free_all_tx_resources(adapter);
		rnpgbe_free_all_rx_resources(adapter);
	}
	rtnl_unlock();
	adapter->link_up = 0;
	adapter->link_up_old = 0;
	rnpgbe_msg_post_status(adapter, PF_SET_LINK_STATUS);
	/* wait all vf get this status */
	usleep_range(500, 1000);
	rnpgbe_reset(adapter);
	remove_mbx_irq(adapter);
	rnpgbe_clear_interrupt_scheme(adapter);
	rtnl_lock();
	err = rnpgbe_init_interrupt_scheme(adapter);
	register_mbx_irq(adapter);

	if (!err && netif_running(netdev))
		err = rnpgbe_open(netdev);

	rtnl_unlock();
	rnpgbe_msg_post_status(adapter, PF_SET_RESET);
	netif_device_attach(netdev);
	adapter->flags2 &= (~RNP_FLAG2_RESET_PF);
}

static void rnpgbe_reset_subtask(struct rnpgbe_adapter *adapter)
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
	rnpgbe_reinit_locked(adapter);
	rtnl_unlock();
}

static void rnpgbe_rx_len_reset_subtask(struct rnpgbe_adapter *adapter)
{
	int i;
	struct rnpgbe_ring *rx_ring;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		rx_ring = adapter->rx_ring[i];
		if (unlikely(rx_ring->ring_flags &
			     RNP_RING_FLAG_DO_RESET_RX_LEN)) {
			dbg("[%s] Rx-ring %d count reset\n",
			    adapter->netdev->name, rx_ring->rnpgbe_queue_idx);
			rnpgbe_rx_ring_reinit(adapter, rx_ring);
			rx_ring->ring_flags &= (~RNP_RING_FLAG_DO_RESET_RX_LEN);
		}
	}
}

/* just modify rx itr */
static void rnpgbe_auto_itr_moderation(struct rnpgbe_adapter *adapter)
{
	int i;
	struct rnpgbe_ring *rx_ring;
	u64 period = (u64)(jiffies - adapter->last_moder_jiffies);


	if (adapter->priv_flags & RNP_PRIV_FLAG_RX_COALESCE)
		return;

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

		if (packets == 0) {
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
 * rnpgbe_service_task - manages and runs subtasks
 * @work: pointer to work_struct containing our data
 **/
static void rnpgbe_service_task(struct work_struct *work)
{
	struct rnpgbe_adapter *adapter =
		container_of(work, struct rnpgbe_adapter, service_task);

	rnpgbe_reset_subtask(adapter);
	rnpgbe_reset_pf_subtask(adapter);
	rnpgbe_watchdog_subtask(adapter);
	rnpgbe_rx_len_reset_subtask(adapter);
	rnpgbe_auto_itr_moderation(adapter);
	rnpgbe_service_event_complete(adapter);
}

static int rnpgbe_tso(struct rnpgbe_ring *tx_ring,
		      struct rnpgbe_tx_buffer *first, u32 *mac_ip_len,
		      u8 *hdr_len, u32 *tx_flags)
{
	struct sk_buff *skb = first->skb;
	struct net_device *netdev = tx_ring->netdev;
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
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
	tx_dbg("before l4 checksum is %x\n", l4.tcp->check);

	if (skb->csum_offset == offsetof(struct tcphdr, check)) {
		tx_dbg("tcp before l4 checksum is %x\n", l4.tcp->check);
		*tx_flags |= RNP_TXD_L4_TYPE_TCP;
		/* compute length of segmentation header */
		*hdr_len = (l4.tcp->doff * 4) + l4_offset;
		csum_replace_by_diff(&l4.tcp->check,
				     (__force __wsum)htonl(paylen));
		l4.tcp->psh = 0;
		tx_dbg("tcp l4 checksum is %x\n", l4.tcp->check);
	} else {
		tx_dbg("paylen is %x\n", paylen);
		*tx_flags |= RNP_TXD_L4_TYPE_UDP;
		/* compute length of segmentation header */
		tx_dbg("udp before l4 checksum is %x\n", l4.udp->check);
		*hdr_len = sizeof(*l4.udp) + l4_offset;
		csum_replace_by_diff(&l4.udp->check,
				     (__force __wsum)htonl(paylen));
		tx_dbg("udp l4 checksum is %x\n", l4.udp->check);
	}

	tx_dbg("l4 checksum is %x\n", l4.tcp->check);
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

static int rnpgbe_tx_csum(struct rnpgbe_ring *tx_ring,
			  struct rnpgbe_tx_buffer *first, u32 *mac_ip_len,
			  u32 *tx_flags)
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

	if ((tx_ring->ring_flags & RNP_RING_NO_TUNNEL_SUPPORT) &&
	    (first->ctx_flag)) {
		/* if not support tunnel */
		if (!(first->priv_tags)) {
			first->ctx_flag = false;
			mac_len += first->tunnel_hdr_len;
			first->tunnel_hdr_len = 0;
		}
	}
	tx_dbg("mac length is %d\n", mac_len);
	tx_dbg("ip length is %d\n", ip_len);
	*mac_ip_len = (mac_len << 9) | ip_len;

	return 0;
}

static int __rnpgbe_maybe_stop_tx(struct rnpgbe_ring *tx_ring, u16 size)
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
	if (likely(rnpgbe_desc_unused(tx_ring) < size))
		return -EBUSY;

	/* A reprieve! - use start_queue because it doesn't call schedule */
	netif_start_subqueue(tx_ring->netdev, tx_ring->queue_index);
	++tx_ring->tx_stats.restart_queue;

	return 0;
}

static inline int rnpgbe_maybe_stop_tx(struct rnpgbe_ring *tx_ring, u16 size)
{
	if (likely(rnpgbe_desc_unused(tx_ring) >= size))
		return 0;
	return __rnpgbe_maybe_stop_tx(tx_ring, size);
}

static int rnpgbe_tx_map(struct rnpgbe_ring *tx_ring,
			 struct rnpgbe_tx_buffer *first, u32 mac_ip_len,
			 u32 tx_flags)
{
	struct sk_buff *skb = first->skb;
	struct rnpgbe_tx_buffer *tx_buffer;
	struct rnpgbe_tx_desc *tx_desc;
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

	skb_tx_timestamp(skb);
#ifdef SIMULATE_TX
	napi_consume_skb(first->skb, 64);
	dma_unmap_single(tx_ring->dev, dma_unmap_addr(first, dma),
			 dma_unmap_len(first, len), DMA_TO_DEVICE);

	tx_ring->stats.bytes += skb->len;
	tx_ring->stats.packets += 1;
	first->skb = NULL;
#else

#ifdef NO_BQL_TEST
#else
	netdev_tx_sent_queue(txring_txq(tx_ring), first->bytecount);
#endif
	/* notify HW of packet */
	rnpgbe_wr_reg(tx_ring->tail, i);
#endif
	return 0;
dma_error:
	dev_err(tx_ring->dev, "TX DMA map failed\n");

	/* clear dma mappings for failed tx_buffer_info map */
	for (;;) {
		tx_buffer = &tx_ring->tx_buffer_info[i];
		rnpgbe_unmap_and_free_tx_resource(tx_ring, tx_buffer);
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

static void rnpgbe_force_src_mac(struct sk_buff *skb, struct net_device *netdev)
{
	u8 *data = skb->data;
	bool ret = false;
	struct netdev_hw_addr *ha;

	if (is_multicast_ether_addr(data)) {
		if (memcmp(data + netdev->addr_len, netdev->dev_addr,
			   netdev->addr_len) == 0) {
			ret = true;
			goto DONE;
		}
		netdev_for_each_uc_addr (ha, netdev) {
			if (memcmp(data + netdev->addr_len, ha->addr,
				   netdev->addr_len) == 0) {
				ret = true;
				/* if it is src mac, nothing todo */
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

static netdev_tx_t rnpgbe_xmit_frame_ring(struct sk_buff *skb,
				   struct rnpgbe_adapter *adapter,
				   struct rnpgbe_ring *tx_ring, bool tx_padding)
{
	struct rnpgbe_tx_buffer *first;
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
	if (adapter->tx_path_in_lpi_mode)
		rnpgbe_disable_eee_mode(adapter);

	for (f = 0; f < skb_shinfo(skb)->nr_frags; f++) {
		skb_frag_t *frag_temp = &skb_shinfo(skb)->frags[f];

		count += TXD_USE_COUNT(skb_frag_size(frag_temp));
		tx_dbg(" rnp #%d frag: size:%d\n", f, skb_frag_size(frag_temp));
	}

	if (rnpgbe_maybe_stop_tx(tx_ring, count + 3)) {
		tx_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}

	if ((adapter->flags & RNP_FLAG_SRIOV_ENABLED) &&
	    (!(tx_ring->ring_flags & RNP_RING_VEB_MULTI_FIX)))
		rnpgbe_force_src_mac(skb, tx_ring->netdev);

	/* record the location of the first descriptor for this packet */
	first = &tx_ring->tx_buffer_info[tx_ring->next_to_use];
	first->skb = skb;
	/* maybe consider len smaller than 60 */
	first->bytecount = (skb->len > 60) ? skb->len : 60;
	first->gso_segs = 1;
	first->priv_tags = 0;
	first->mss_len_vf_num = 0;
	first->inner_vlan_tunnel_len = 0;
	first->ctx_flag =
		(adapter->flags & RNP_FLAG_SRIOV_ENABLED) ? true : false;

	/* if we have a HW VLAN tag being added default to the HW one */
	/* RNP_TXD_VLAN_VALID is used for veb */
	/* setup padding flag */
	if (adapter->priv_flags & RNP_PRIV_FLAG_TX_PADDING) {
		first->ctx_flag = true;
		first->gso_need_padding = tx_padding;
	}

	/* RNP_FLAG2_VLAN_STAGS_ENABLED and
	 * tx-stags-offload not support together
	 */
	if (adapter->flags2 & RNP_FLAG2_VLAN_STAGS_ENABLED) {
		/* always add a stags for any packets out */
		if (tx_ring->ring_flags & RNP_RING_OUTER_VLAN_FIX) {
			/* n500 set outer_vlan to ctx */
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
			netdev_dbg(tx_ring->netdev, "ptp_tx_skb miss\n");
		}
	}
	/* record initial flags and protocol */
	tso = rnpgbe_tso(tx_ring, first, &mac_ip_len, &hdr_len, &tx_flags);
	if (tso < 0)
		goto out_drop;
	else if (!tso)
		rnpgbe_tx_csum(tx_ring, first, &mac_ip_len, &tx_flags);
	/* check sriov mode */
	/* in this mode pf send msg should with vf_num */
	if (unlikely(adapter->flags & RNP_FLAG_SRIOV_ENABLED)) {
		first->ctx_flag = true;
		first->mss_len_vf_num |= (adapter->vf_num_for_pf << 16);
	}

	/* add control desc */
	rnpgbe_maybe_tx_ctxtdesc(tx_ring, first, ignore_vlan);

	if (rnpgbe_tx_map(tx_ring, first, mac_ip_len, tx_flags))
		goto cleanup_tx_tstamp;
	/* need this */
	rnpgbe_maybe_stop_tx(tx_ring, DESC_NEEDED);

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
	switch (l4_proto) {
	case IPPROTO_SCTP:
		no_padding = true;
		break;
	default:

		break;
	}

	return no_padding;
}

static netdev_tx_t rnpgbe_xmit_frame(struct sk_buff *skb,
				     struct net_device *netdev)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_ring *tx_ring;
	bool tx_padding = false;

	if (!netif_carrier_ok(netdev)) {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	/*
	 * The minimum packet size for olinfo paylen is 17 so pad the skb
	 * in order to meet this minimum size requirement.
	 */
	if ((adapter->priv_flags & RNP_PRIV_FLAG_TX_PADDING) &&
	    (!(adapter->priv_flags & RNP_PRIV_FLAG_SOFT_TX_PADDING))) {
		if (skb->len < 60) {
			if (!check_sctp_no_padding(skb)) {
				if (skb_put_padto(skb, 60))
					return NETDEV_TX_OK;

			} else {
				tx_padding = true;
			}
		}
	} else {
		if (skb->len < 33) {
			if (skb_padto(skb, 33))
				return NETDEV_TX_OK;
			skb->len = 33;
		}
	}
	tx_ring = adapter->tx_ring[skb->queue_mapping];

	return rnpgbe_xmit_frame_ring(skb, adapter, tx_ring, tx_padding);
}

/**
 * rnpgbe_set_mac - Change the Ethernet Address of the NIC
 * @netdev: network interface device structure
 * @p: pointer to an address structure
 *
 * Returns 0 on success, negative on failure
 **/
static int rnpgbe_set_mac(struct net_device *netdev, void *p)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;
	struct sockaddr *addr = p;
	const u8 target_addr[ETH_ALEN];
	bool sriov_flag = !!(adapter->flags & RNP_FLAG_SRIOV_ENABLED);

	dbg("[%s] call set mac\n", netdev->name);

	memcpy((void *)target_addr, addr->sa_data, netdev->addr_len);

	if (!is_valid_ether_addr(target_addr))
		return -EADDRNOTAVAIL;

	eth_hw_addr_set(netdev, target_addr);
	memcpy(hw->mac.addr, addr->sa_data, netdev->addr_len);
	hw->ops.set_mac(hw, hw->mac.addr, sriov_flag);
	/* reset veb table */
	rnpgbe_configure_virtualization(adapter);
	return 0;
}

static int rnpgbe_mdio_read(struct net_device *netdev, int prtad, int devad,
			    u32 addr, u32 *phy_value)
{
	int rc = -EIO;
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;
	u16 value;

	rc = hw->ops.phy_read_reg(hw, addr, 0, &value);
	*phy_value = value;

	return rc;
}

static int rnpgbe_mdio_write(struct net_device *netdev, int prtad, int devad,
			     u16 addr, u16 value)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;

	return hw->ops.phy_write_reg(hw, addr, 0, value);
}

static int rnpgbe_mii_ioctl(struct net_device *netdev, struct ifreq *ifr,
			    int cmd)
{
	struct mii_ioctl_data *mii = (struct mii_ioctl_data *)&ifr->ifr_data;
	int prtad, devad, ret;
	u32 phy_value;

	prtad = (mii->phy_id & MDIO_PHY_ID_PRTAD) >> 5;
	devad = (mii->phy_id & MDIO_PHY_ID_DEVAD);

	if (cmd == SIOCGMIIREG) {
		ret = rnpgbe_mdio_read(netdev, prtad, devad, mii->reg_num,
				       &phy_value);
		if (ret < 0)
			return ret;
		mii->val_out = phy_value;
		return 0;
	} else {
		return rnpgbe_mdio_write(netdev, prtad, devad, mii->reg_num,
					 mii->val_in);
	}
}

static int rnpgbe_ioctl(struct net_device *netdev, struct ifreq *req, int cmd)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	/* ptp 1588 used this */
	switch (cmd) {
	case SIOCGHWTSTAMP:
		if (module_enable_ptp)
			return rnpgbe_ptp_get_ts_config(adapter, req);
		break;
	case SIOCSHWTSTAMP:
		if (module_enable_ptp)
			return rnpgbe_ptp_set_ts_config(adapter, req);
		break;
	case SIOCGMIIPHY:
		return 0;
	case SIOCGMIIREG:
		/*fall through */
	case SIOCSMIIREG:
		return rnpgbe_mii_ioctl(netdev, req, cmd);
	}
	return -EINVAL;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
 * Polling 'interrupt' - used by things like netconsole to send skbs
 * without having to re-enable interrupts. It's not called while
 * the interrupt routine is executing.
 */
static void rnpgbe_netpoll(struct net_device *netdev)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	int i;

	/* if interface is down do nothing */
	if (test_bit(__RNP_DOWN, &adapter->state))
		return;

	adapter->flags |= RNP_FLAG_IN_NETPOLL;
	for (i = 0; i < adapter->num_q_vectors; i++)
		rnpgbe_msix_clean_rings(0, adapter->q_vector[i]);
	adapter->flags &= ~RNP_FLAG_IN_NETPOLL;
}

#endif

static void rnpgbe_get_stats64(struct net_device *netdev,
			       struct rtnl_link_stats64 *stats)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	int i;

	rcu_read_lock();
	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct rnpgbe_ring *ring = READ_ONCE(adapter->rx_ring[i]);
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
		struct rnpgbe_ring *ring = READ_ONCE(adapter->tx_ring[i]);
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
	/* following stats updated by rnpgbe_watchdog_task() */
	stats->multicast = netdev->stats.multicast;
	stats->rx_errors = netdev->stats.rx_errors;
	stats->rx_length_errors = netdev->stats.rx_length_errors;
	stats->rx_crc_errors = netdev->stats.rx_crc_errors;
	stats->rx_missed_errors = netdev->stats.rx_missed_errors;
}

/**
 * rnpgbe_setup_tc - configure net_device for multiple traffic classes
 *
 * @netdev: net device to configure
 * @tc: number of traffic classes to enable
 */
int rnpgbe_setup_tc(struct net_device *dev, u8 tc)
{
	struct rnpgbe_adapter *adapter = netdev_priv(dev);
	struct rnpgbe_hw *hw = &adapter->hw;
	int ret = 0;

	if ((hw->hw_type != rnpgbe_hw_n10) && (tc))
		return -EINVAL;

	/* if now we are in force mode, never need force, if not force it */
	if (!(adapter->priv_flags & RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE)) {
		hw->ops.set_mac_rx(hw, false);
		if (hw->ops.driver_status)
			hw->ops.driver_status(hw, true,
					      rnpgbe_driver_force_control_phy);
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
		rnpgbe_close(dev);

	rnpgbe_fdir_filter_exit(adapter);
	adapter->priv_flags &= (~RNP_PRIV_FLAG_TCP_SYNC);
	remove_mbx_irq(adapter);
	rnpgbe_clear_interrupt_scheme(adapter);
	adapter->num_tc = tc;

	if (tc) {
		netdev_set_num_tc(dev, tc);
		adapter->flags |= RNP_FLAG_DCB_ENABLED;

	} else {
		netdev_reset_tc(dev);

		adapter->flags &= ~RNP_FLAG_DCB_ENABLED;
	}

	rnpgbe_init_interrupt_scheme(adapter);

	register_mbx_irq(adapter);
	/* rss table must reset */
	adapter->rss_tbl_setup_flag = 0;

	if (netif_running(dev))
		ret = rnpgbe_open(dev);
	if (!(adapter->priv_flags & RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE)) {
		if (hw->ops.driver_status)
			hw->ops.driver_status(hw, false,
					      rnpgbe_driver_force_control_phy);
	}

	clear_bit(__RNP_RESETTING, &adapter->state);
	return ret;
}

#ifdef CONFIG_PCI_IOV
void rnpgbe_sriov_reinit(struct rnpgbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	rtnl_lock();
	rnpgbe_setup_tc(netdev, netdev_get_num_tc(netdev));
	rtnl_unlock();
	usleep_range(10000, 20000);
}
#endif

static void rnpgbe_do_reset(struct net_device *netdev)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev))
		rnpgbe_reinit_locked(adapter);
	else
		rnpgbe_reset(adapter);
}

static netdev_features_t rnpgbe_fix_features(struct net_device *netdev,
					     netdev_features_t features)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	struct rnpgbe_hw *hw = &adapter->hw;

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
		if (!(features & NETIF_F_HW_VLAN_STAG_RX)) {
			features &= ~NETIF_F_HW_VLAN_CTAG_RX;
		}
	}

	if (!(features & NETIF_F_HW_VLAN_CTAG_TX)) {
		if (hw->feature_flags & RNP_NET_FEATURE_STAG_OFFLOAD)
			features &= ~NETIF_F_HW_VLAN_STAG_TX;
	}

	if (hw->feature_flags & RNP_NET_FEATURE_STAG_OFFLOAD) {
		if (!(features & NETIF_F_HW_VLAN_STAG_TX)) {
			features &= ~NETIF_F_HW_VLAN_CTAG_TX;
		}
	}

	return features;
}

static int rnpgbe_set_features(struct net_device *netdev,
			       netdev_features_t features)
{
	struct rnpgbe_adapter *adapter = netdev_priv(netdev);
	netdev_features_t changed = netdev->features ^ features;
	bool need_reset = false;
	struct rnpgbe_hw *hw = &adapter->hw;

	netdev->features = features;

	/* if changed ntuple should close all */
	if (changed & NETIF_F_NTUPLE) {
		if (!(features & NETIF_F_NTUPLE))
			rnpgbe_fdir_filter_exit(adapter);
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
		if (features & (NETIF_F_HW_VLAN_CTAG_FILTER)) {
			/* not open if in promise mode */
			if (!(netdev->flags & IFF_PROMISC))
				hw->ops.set_vlan_filter_en(hw, true);
		} else {
			hw->ops.set_vlan_filter_en(hw, false);
		}
		rnpgbe_msg_post_status(adapter, PF_VLAN_FILTER_STATUS);
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
		} else {
			adapter->priv_flags &= (~RNP_PRIV_FLAG_RX_FCS);
			hw->ops.set_fcs_mode(hw, false);
		}
		rnpgbe_msg_post_status(adapter, PF_FCS_STATUS);
	}

	if (changed & NETIF_F_RXALL)
		need_reset = true;

	if (features & NETIF_F_RXALL)
		adapter->priv_flags |= RNP_PRIV_FLAG_RX_ALL;
	else
		adapter->priv_flags &= (~RNP_PRIV_FLAG_RX_ALL);

	if (features & NETIF_F_HW_VLAN_CTAG_RX)
		rnpgbe_vlan_strip_enable(adapter);
	else
		rnpgbe_vlan_strip_disable(adapter);

	if (need_reset)
		rnpgbe_do_reset(netdev);

	return 0;
}

static int
rnpgbe_ndo_bridge_setlink(struct net_device *dev, struct nlmsghdr *nlh,
			  __always_unused u16 flags,
			  struct netlink_ext_ack __always_unused *ext)
{
	struct rnpgbe_adapter *adapter = netdev_priv(dev);
	struct rnpgbe_hw *hw = &adapter->hw;
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

static int rnpgbe_ndo_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq,
				     struct net_device *dev,
				     u32 __maybe_unused filter_mask,
				     int nlflags)
{
	struct rnpgbe_adapter *adapter = netdev_priv(dev);
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

static netdev_features_t rnpgbe_features_check(struct sk_buff *skb,
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

const struct net_device_ops rnpgbe_netdev_ops = {
	.ndo_open = rnpgbe_open,
	.ndo_stop = rnpgbe_close,
	.ndo_start_xmit = rnpgbe_xmit_frame,
	.ndo_set_rx_mode = rnpgbe_set_rx_mode,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_eth_ioctl = rnpgbe_ioctl,
	.ndo_change_mtu = rnpgbe_change_mtu,
	.ndo_get_stats64 = rnpgbe_get_stats64,
	.ndo_tx_timeout = rnpgbe_tx_timeout,
	.ndo_set_tx_maxrate = rnpgbe_tx_maxrate,
	.ndo_set_mac_address = rnpgbe_set_mac,
	.ndo_vlan_rx_add_vid = rnpgbe_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = rnpgbe_vlan_rx_kill_vid,
	.ndo_set_vf_mac = rnpgbe_ndo_set_vf_mac,
	.ndo_set_vf_vlan = rnpgbe_ndo_set_vf_vlan,
	.ndo_set_vf_rate = rnpgbe_ndo_set_vf_bw,
	.ndo_set_vf_spoofchk = rnpgbe_ndo_set_vf_spoofchk,
	.ndo_set_vf_link_state = rnpgbe_ndo_set_vf_link_state,
	.ndo_set_vf_trust = rnpgbe_ndo_set_vf_trust,
	.ndo_get_vf_config = rnpgbe_ndo_get_vf_config,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller = rnpgbe_netpoll,
#endif
	.ndo_bridge_setlink = rnpgbe_ndo_bridge_setlink,
	.ndo_bridge_getlink = rnpgbe_ndo_bridge_getlink,
	.ndo_features_check = rnpgbe_features_check,
	.ndo_set_features = rnpgbe_set_features,
	.ndo_fix_features = rnpgbe_fix_features,
};

void rnpgbe_assign_netdev_ops(struct net_device *dev)
{
	/* different hw can assign difference fun */
	dev->netdev_ops = &rnpgbe_netdev_ops;
	dev->watchdog_timeo = 5 * HZ;
}

/**
 * rnpgbe_wol_supported - Check whether device supports WoL
 * @hw: hw specific details
 * @device_id: the device ID
 *
 * This function is used by probe and ethtool to determine
 * which devices have WoL support
 *
 **/
int rnpgbe_wol_supported(struct rnpgbe_adapter *adapter, u16 device_id)
{
	int is_wol_supported = 0;

	switch (device_id) {
	case PCI_DEVICE_ID_N210:
	case PCI_DEVICE_ID_N500_QUAD_PORT:
	case PCI_DEVICE_ID_N500_DUAL_PORT:
		is_wol_supported = 1;
		break;
	default:
		is_wol_supported = 0;
		break;
	}

	return is_wol_supported;
}

static inline unsigned long rnpgbe_tso_features(struct rnpgbe_hw *hw)
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

static void remove_mbx_irq(struct rnpgbe_adapter *adapter)
{
	/* mbx */
	if (adapter->num_other_vectors) {
		/* only msix use indepented intr */
		if (adapter->flags & RNP_FLAG_MSIX_ENABLED) {
			adapter->hw.mbx.ops.configure(
				&adapter->hw, adapter->msix_entries[0].entry,
				false);
			if (adapter->hw.mbx.other_irq_enabled) {
				free_irq(adapter->msix_entries[0].vector, adapter);
				adapter->hw.mbx.other_irq_enabled = false;
			}
		}
	}
}

static int register_mbx_irq(struct rnpgbe_adapter *adapter)
{
	struct rnpgbe_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	int err = 0;

	/* for mbx:vector0 */
	if (adapter->num_other_vectors) {
		/* only do this in msix mode */
		if (adapter->flags & RNP_FLAG_MSIX_ENABLED) {
			err = request_irq(adapter->msix_entries[0].vector,
					  rnpgbe_msix_other, 0, netdev->name,
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

static int rnpgbe_rm_adpater(struct rnpgbe_adapter *adapter)
{
	struct net_device *netdev;
	struct rnpgbe_hw *hw = &adapter->hw;

	netdev = adapter->netdev;
	pr_info("= remove adapter:%s =\n", netdev->name);

	rnpgbe_dbg_adapter_exit(adapter);

	netif_carrier_off(netdev);

	set_bit(__RNP_DOWN, &adapter->state);
	set_bit(__RNP_REMOVE, &adapter->state);
	if (module_enable_ptp) {
		while (test_bit(__RNP_PTP_TX_IN_PROGRESS, &adapter->state))
			usleep_range(10000, 20000);
		cancel_work_sync(&adapter->tx_hwtstamp_work);
	}
	if (adapter->eee_active) {
		adapter->eee_active = 0;
		rnpgbe_eee_init(adapter);
	}
	cancel_work_sync(&adapter->service_task);
	del_timer_sync(&adapter->service_timer);
	rnpgbe_sysfs_exit(adapter);

	rnpgbe_fdir_filter_exit(adapter);
	adapter->priv_flags &= (~RNP_PRIV_FLAG_TCP_SYNC);

	if (netdev->reg_state == NETREG_REGISTERED)
		unregister_netdev(netdev);

	adapter->netdev = NULL;

	if (hw->ops.driver_status)
		hw->ops.driver_status(hw, false, rnpgbe_driver_insmod);

	remove_mbx_irq(adapter);

	rnpgbe_clear_interrupt_scheme(adapter);

	if (adapter->io_addr)
		iounmap(adapter->io_addr);

	if (adapter->io_addr_bar0)
		iounmap(adapter->io_addr_bar0);

	free_netdev(netdev);

	pr_info("remove complete\n");

	return 0;
}

static int rnpgbe_init_firmware(struct rnpgbe_hw *hw, struct file *file,
				int file_size)
{
	struct device *dev = &(hw->pdev->dev);
	loff_t old_pos = 0;
	loff_t pos = 0;
	loff_t end_pos = file_size;
	u32 rd_len = 0x1000;
	int get_len = 0;
	u32 iter = 0;
	int err = 0;
	u32 fw_off = 0;
	u32 old_data = 0;
	u32 new_data = 0;
	char *buf = kzalloc(0x1000, GFP_KERNEL);

	dev_info(dev, "initializing firmware, which will take some time.");
	/* capy bin to bar */
	while (pos < end_pos) {
		if ((pos >= 0x1f000) && (pos < 0x20000)) {
			pos += rd_len;
			continue;
		}

		old_pos = pos;
		get_len = kernel_read(file, buf, rd_len, &pos);
		if (((get_len < rd_len) && ((old_pos + get_len) != end_pos)) ||
		    (get_len < 0)) {
			dev_err(dev, "read err, pos 0x%x, get len %d",
				(u32)old_pos, get_len);
			err = -EIO;
			return err;
		}

		for (iter = 0; iter < get_len; iter += 4) {
			old_data = *((u32 *)(buf + iter));
			fw_off = (u32)old_pos + iter + 0x1000;
			iowrite32(old_data, (hw->hw_addr + fw_off));
		}

		if (pos == old_pos) {
			pos += get_len;
		}
	}

	dev_info(dev, "Checking for firmware. Wait a moment, please.");
	/* check */
	pos = 0x0;
	while (pos < end_pos) {
		if ((pos >= 0x1f000) && (pos < 0x20000)) {
			pos += rd_len;
			continue;
		}

		old_pos = pos;
		get_len = kernel_read(file, buf, rd_len, &pos);
		if (((get_len < rd_len) && ((old_pos + get_len) != end_pos)) ||
		    (get_len < 0)) {
			dev_err(dev, "read err, pos 0x%x, get len %d",
				(u32)old_pos, get_len);
			kfree(buf);
			err = -EIO;
			return err;
		}

		for (iter = 0; iter < get_len; iter += 4) {
			old_data = *((u32 *)(buf + iter));
			fw_off = (u32)old_pos + iter + 0x1000;
			new_data = ioread32(hw->hw_addr + fw_off);
			if (old_data != new_data) {
				dev_err(dev,
					"Err at 0x%08x write:0x%08x read:0x%08x",
					fw_off, old_data, new_data);
				err = -EIO;
			}
		}

		if (pos == old_pos) {
			pos += get_len;
		}
	}

	kfree(buf);
	return err;
}

static int rnpgbe_add_adpater(struct pci_dev *pdev, struct rnpgbe_info *ii,
			      struct rnpgbe_adapter **padapter)
{
	int i, err = 0;
	struct rnpgbe_adapter *adapter = NULL;
	struct net_device *netdev;
	struct rnpgbe_hw *hw;
	u8 __iomem *hw_addr = NULL;
	u8 __iomem *hw_addr_bar0 = NULL;

	u32 dma_version = 0;
	u32 nic_version = 0;
	u32 queues = ii->total_queue_pair_cnts;
	static int bd_number;

	pr_info("====  add adapter queues:%d ====", queues);
	netdev = alloc_etherdev_mq(sizeof(struct rnpgbe_adapter), queues);
	if (!netdev)
		return -ENOMEM;

	if (!fix_eth_name)
		SET_NETDEV_DEV(netdev, &pdev->dev);

	adapter = netdev_priv(netdev);

	memset((char *)adapter, 0x00, sizeof(struct rnpgbe_adapter));
	adapter->netdev = netdev;
	adapter->pdev = pdev;
	adapter->max_ring_pair_counts = queues;
	if (padapter)
		*padapter = adapter;

	adapter->bd_number = bd_number++;
	adapter->port = 0;
	snprintf(adapter->name, sizeof(netdev->name), "%s%d",
		 rnpgbe_driver_name, adapter->bd_number);
	pci_set_drvdata(pdev, adapter);

	hw = &adapter->hw;
	hw->back = adapter;
	/* first setup hw type */
	hw->pdev = pdev;
	hw->rss_type = ii->rss_type;
	hw->hw_type = ii->hw_type;
	switch (hw->hw_type) {
	case rnpgbe_hw_n500:
		/* n500 use bar2 */
#define RNP_NIC_BAR_N500 2
		hw_addr = ioremap(pci_resource_start(pdev, RNP_NIC_BAR_N500),
				  pci_resource_len(pdev, RNP_NIC_BAR_N500));
		if (!hw_addr) {
			dev_err(&pdev->dev, "pcim_iomap bar%d failed!\n",
				RNP_NIC_BAR_N500);
			return -EIO;
		}
		pr_info("[bar%d]: %p %llx len=%d kB\n", RNP_NIC_BAR_N500,
			hw_addr,
			(unsigned long long)pci_resource_start(
				pdev, RNP_NIC_BAR_N500),
			(int)pci_resource_len(pdev, RNP_NIC_BAR_N500) / 1024);
		/* get dma version */
		dma_version = rnpgbe_rd_reg(hw_addr);

		hw->hw_addr = hw_addr;
		/* setup msix base */
		hw->ring_msix_base = hw->hw_addr + 0x28700;
		hw->pfvfnum_system = PF_NUM_N500(rnpgbe_get_fuc(pdev));
		nic_version = rd32(hw, RNP500_TOP_NIC_VERSION);
		adapter->irq_mode = irq_mode_msix;
		adapter->flags |= RNP_FLAG_MSI_CAPABLE | RNP_FLAG_MSIX_CAPABLE |
				  RNP_FLAG_LEGACY_CAPABLE;
		/* ifdef uart show tod */
#ifdef FW_UART_SHOW_TSTAMPS
		hw_addr_bar0 = ioremap(pci_resource_start(pdev, 0),
				       pci_resource_len(pdev, 0));
#endif
		break;

	case rnpgbe_hw_n210:
#define RNP_NIC_BAR_N210 2
		if (pci_resource_len(pdev, 0) == 0x100000) {
			char *path = "/lib/firmware/n210_driver_update.bin";
			struct file *file = NULL;
			int file_size = 0;

			hw->hw_addr = ioremap(pci_resource_start(pdev, 0),
					      pci_resource_len(pdev, 0));
			if (!(hw->hw_addr)) {
				dev_err(&pdev->dev, "pci_iomap bar%d failed",
					RNP_NIC_BAR_N210);
				return -EIO;
			}

			file = filp_open(path, O_RDONLY, 0);
			if (IS_ERR(file)) {
				dev_err(&pdev->dev,
					"filp_open(%s) failed with err %ld",
					path, PTR_ERR(file));
				err = PTR_ERR(file);
				return err;
			}
			file_size = file->f_inode->i_size;
			dev_info(&pdev->dev, "%s size %u", path, file_size);

			err = rsp_hal_sfc_flash_erase(hw, file_size);
			if (err) {
				dev_err(&pdev->dev, "erase flash failed!");
				fput(file);
				return err;
			}

			err = rnpgbe_init_firmware(hw, file, file_size);
			if (err) {
				dev_err(&pdev->dev, "init firmware failed!");
				fput(file);
				return err;
			}
			dev_info(&pdev->dev, "init firmware successfully.");
			dev_info(&pdev->dev,
				 "Please reboot. Then you can use the device.");
			fput(file);
			return 0;
		}
		hw_addr = ioremap(pci_resource_start(pdev, RNP_NIC_BAR_N210),
				  pci_resource_len(pdev, RNP_NIC_BAR_N210));
		if (!hw_addr) {
			dev_err(&pdev->dev, "pcim_iomap bar%d failed!\n",
				RNP_NIC_BAR_N210);
			return -EIO;
		}
		pr_info("[bar%d]:%p %llx len=%d MB\n", RNP_NIC_BAR_N210,
			hw_addr,
			(unsigned long long)pci_resource_start(
				pdev, RNP_NIC_BAR_N210),
			(int)pci_resource_len(pdev, RNP_NIC_BAR_N210) / 1024 /
				1024);
		/* get dma version */
		dma_version = rnpgbe_rd_reg(hw_addr);

		hw->hw_addr = hw_addr;
		/* setup msix base */
		hw->ring_msix_base = hw->hw_addr + 0x29000;

		hw->pfvfnum_system = PF_NUM_N500(rnpgbe_get_fuc(pdev));
		nic_version = rd32(hw, RNP500_TOP_NIC_VERSION);
		adapter->irq_mode = irq_mode_msix;
		adapter->flags |= RNP_FLAG_MSI_CAPABLE | RNP_FLAG_MSIX_CAPABLE |
				  RNP_FLAG_LEGACY_CAPABLE;
		break;
	default:
		hw_addr = ioremap(pci_resource_start(pdev, 0),
				  pci_resource_len(pdev, 0));
		goto err_free_net;
	}

	/* assign to adapter */
	adapter->io_addr = hw_addr;
	adapter->io_addr_bar0 = hw_addr_bar0;

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
		rnpgbe_init_mbx_params_pf(hw);
		memcpy(&hw->mbx.ops, ii->mbx_ops, sizeof(hw->mbx.ops));
		if (dma_version >= 0x20210111) {
			rnpgbe_mbx_link_event_enable(hw, 0);
			if ((hw->hw_type == rnpgbe_hw_n10) ||
			    (hw->hw_type == rnpgbe_hw_n400))
				rnpgbe_mbx_force_speed(hw, 0);
			/* call driver status */
			if (hw->ops.driver_status)
				hw->ops.driver_status(hw, true, rnpgbe_driver_insmod);
			if (rnpgbe_mbx_get_capability(hw, ii)) {
				dev_err(&pdev->dev,
					"rnpgbe_mbx_get_capability failed!\n");
				err = -EIO;
				goto err_free_net;
			}

			/* get lldp status if version large than 0.1.1.40 */
			if ((hw->fw_version >= 0x00010128) &&
			    ((hw->fw_version & 0xff000000) == 0))
				rnpgbe_mbx_lldp_get(hw);

			if (hw->lldp_status.enable)
				adapter->priv_flags |= RNP_PRIV_FLAG_LLDP;

			hw->usecstocount = hw->axi_mhz;

			{
				struct rnpgbe_eee_cap eee_cap;

				memset(&eee_cap, 0x00,
				       sizeof(struct rnpgbe_eee_cap));

				if (hw->feature_flags & RNP_HW_FEATURE_EEE) {
					rnpgbe_mbx_get_eee_capability(hw,
								      &eee_cap);
					if (eee_cap.local_capability) {
						hw->eee_capability =
							eee_cap.local_capability;
						adapter->eee_enabled = 1;
						adapter->local_eee =
							eee_cap.local_eee;
						adapter->partner_eee =
							eee_cap.partner_eee;
						rnpgbe_mbx_phy_eee_set(
							hw,
							adapter->tx_lpi_timer,
							hw->eee_capability);
					}
				}
			}
			{
				/* pf0 we cannot detect in vm, close mask in default */
				if (!hw->pfvfnum) {
					hw->feature_flags &=
						(~RNP_HW_SOFT_MASK_OTHER_IRQ);

#ifdef FW_UART_SHOW_TSTAMPS
					rnpgbe_wr_reg(hw_addr_bar0 + 0xc04,
						      0xffff0101);
#endif
				} else {
					if (hw->pfvfnum == hw->pfvfnum_system)
						hw->feature_flags |=
							RNP_HW_SOFT_MASK_OTHER_IRQ;
				}
			}

			adapter->portid_of_card = hw->port_id[0];
			adapter->portid_of_card = hw->pfvfnum >> 5;
			adapter->wol = hw->wol;
		}
	}

	if (hw->force_en)
		adapter->priv_flags |= RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE;
	hw->driver_version = driver_version;

	hw->default_rx_queue = 0;
	pr_info("%s %s: dma version:0x%x, nic version:0x%x, pfvfnum:0x%x\n",
		adapter->name, pci_name(pdev), hw->dma_version, nic_version,
		hw->pfvfnum);

	/* Setup hw api */
	hw->mac.type = ii->mac;
	/* EEPROM */
	if (ii->eeprom_ops)
		memcpy(&hw->eeprom.ops, ii->eeprom_ops, sizeof(hw->eeprom.ops));

	hw->phy.sfp_type = rnpgbe_sfp_type_unknown;
	hw->ops.setup_ethtool(netdev);
	rnpgbe_assign_netdev_ops(netdev);

	rnpgbe_check_options(adapter);
	/* setup the private structure */
	/* this private is used only once
	 */
	err = rnpgbe_sw_init(adapter);
	if (err)
		goto err_sw_init;

	err = hw->ops.reset_hw(hw);
	hw->phy.reset_if_overtemp = false;
	if (err) {
		e_dev_err("HW Init failed: %d\n", err);
		goto err_sw_init;
	}
	hw->ops.setup_link(hw, DEFAULT_ADV, 1, 0, 0);
	hw->advertised_link = DEFAULT_ADV;
	/* should force phy down first */
	hw->ops.set_mbx_link_event(hw, 0);
	hw->ops.set_mbx_ifup(hw, 0);

#if defined(CONFIG_PCI_IOV)
	if (adapter->num_other_vectors) {
		rnpgbe_enable_sriov(adapter);
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

	if (enable_hi_dma)
		netdev->features |= NETIF_F_HIGHDMA;

	if (hw->feature_flags & RNP_NET_FEATURE_TX_UDP_TUNNEL) {
		netdev->gso_partial_features = RNP_GSO_PARTIAL_FEATURES;
		netdev->features |=
			NETIF_F_GSO_PARTIAL | RNP_GSO_PARTIAL_FEATURES;
	}

	netdev->hw_features |= netdev->features;

	if (hw->feature_flags & RNP_NET_FEATURE_VLAN_FILTER)
		netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_FILTER;
	if (hw->feature_flags & RNP_NET_FEATURE_STAG_FILTER)
		netdev->hw_features |= NETIF_F_HW_VLAN_STAG_FILTER;
	if (hw->feature_flags & RNP_NET_FEATURE_VLAN_OFFLOAD) {
		if (!hw->ncsi_en) {
			netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_RX;
			netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_TX;
		}
	}
	if (hw->feature_flags & RNP_NET_FEATURE_STAG_OFFLOAD) {
		if (!hw->ncsi_en) {
			netdev->hw_features |= NETIF_F_HW_VLAN_STAG_RX;
			netdev->hw_features |= NETIF_F_HW_VLAN_STAG_TX;
		}
	}
	netdev->hw_features |= NETIF_F_RXALL;
	if (hw->feature_flags & RNP_NET_FEATURE_RX_NTUPLE_FILTER)
		netdev->hw_features |= NETIF_F_NTUPLE;
	/* only open rx-fcs in no ocp mode */
	if ((hw->feature_flags & RNP_NET_FEATURE_RX_FCS) && (!hw->ncsi_en))
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
		if (!hw->ncsi_en) {
			netdev->features |= NETIF_F_HW_VLAN_CTAG_RX;
			netdev->features |= NETIF_F_HW_VLAN_CTAG_TX;
		}
	}
	if (hw->feature_flags & RNP_NET_FEATURE_STAG_OFFLOAD) {
		if (!hw->ncsi_en) {
			netdev->features |= NETIF_F_HW_VLAN_STAG_RX;
			netdev->features |= NETIF_F_HW_VLAN_STAG_TX;
		}
	}

	netdev->priv_flags |= IFF_UNICAST_FLT;
	netdev->priv_flags |= IFF_SUPP_NOFCS;

	if (adapter->flags2 & RNP_FLAG2_RSC_CAPABLE)
		netdev->hw_features |= NETIF_F_LRO;

	netdev->priv_flags |= IFF_UNICAST_FLT;
	netdev->priv_flags |= IFF_SUPP_NOFCS;

	if (adapter->flags2 & RNP_FLAG2_RSC_ENABLED)
		netdev->features |= NETIF_F_LRO;

	eth_hw_addr_set(netdev, hw->mac.perm_addr);
	memcpy(netdev->perm_addr, hw->mac.perm_addr, netdev->addr_len);
	pr_info("dev mac:%pM\n", netdev->dev_addr);

	if (!is_valid_ether_addr(netdev->dev_addr)) {
		e_dev_err("invalid MAC address\n");
		err = -EIO;
		goto err_sw_init;
	}
	ether_addr_copy(hw->mac.addr, hw->mac.perm_addr);

	timer_setup(&adapter->service_timer, rnpgbe_service_timer, 0);

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
				  rnpgbe_tx_hwtstamp_work);
		}
	}

	INIT_WORK(&adapter->service_task, rnpgbe_service_task);
	clear_bit(__RNP_SERVICE_SCHED, &adapter->state);

	if (fix_eth_name)
		strncpy(netdev->name, adapter->name, sizeof(netdev->name) - 1);
	else
		strscpy(netdev->name, pci_name(pdev), sizeof(netdev->name));

	err = rnpgbe_init_interrupt_scheme(adapter);
	if (err)
		goto err_sw_init;

	err = register_mbx_irq(adapter);
	if (err)
		goto err_register;

#ifdef CONFIG_PCI_IOV
	rnpgbe_enable_sriov_true(adapter);
#endif

	/* WOL not supported for all devices */
	{
		struct ethtool_wolinfo wol;

		if (rnpgbe_wol_exclusion(adapter, &wol) ||
		    !device_can_wakeup(&adapter->pdev->dev))
			adapter->wol = 0;

		device_set_wakeup_enable(&adapter->pdev->dev, !!adapter->wol);
	}
	/* reset the hardware with the new settings */
	err = hw->ops.start_hw(hw);

	/* we should start down? test it */
	set_bit(__RNP_DOWN, &adapter->state);

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
			rnpgbe_vf_configuration(pdev, (i | 0x10000000));
	}

	if (rnpgbe_sysfs_init(adapter))
		e_err(probe, "failed to allocate sysfs resources\n");

	rnpgbe_dbg_adapter_init(adapter);

	return 0;
err_register:
	remove_mbx_irq(adapter);
	rnpgbe_clear_interrupt_scheme(adapter);
err_sw_init:
	rnpgbe_disable_sriov(adapter);
	adapter->flags2 &= ~RNP_FLAG2_SEARCH_FOR_SFP;
err_free_net:
	free_netdev(netdev);
	return err;
}

/**
 * rnpgbe_probe - Device Initialization Routine
 * @pdev: PCI device information struct
 * @ent: entry in rnpgbe_pci_tbl
 *
 * Returns 0 on success, negative on failure
 *
 * rnpgbe_probe initializes an adapter identified by a pci_dev structure.
 * The OS initialization, configuring of the adapter private structure,
 * and a hardware reset occur.
 **/
static int rnpgbe_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct rnpgbe_adapter *adapter;
	struct rnpgbe_info *ii = rnpgbe_info_tbl[id->driver_data];
	int err;

	/* Catch broken hardware that put the wrong VF device ID in
	 * the PCIe SR-IOV capability.
	 */
	if (pdev->is_virtfn) {
		WARN(1, "%s (%hx:%hx) should not be a VF!\n", pci_name(pdev),
		     pdev->vendor, pdev->device);
		return -EINVAL;
	}

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

	err = pci_request_mem_regions(pdev, rnpgbe_driver_name);
	if (err) {
		dev_err(&pdev->dev,
			"pci_request_selected_regions failed 0x%x\n", err);
		goto err_pci_reg;
	}

	pci_set_master(pdev);
	pci_save_state(pdev);

	err = rnpgbe_add_adpater(pdev, ii, &adapter);
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
 * rnpgbe_remove - Device Removal Routine
 * @pdev: PCI device information struct
 *
 * rnpgbe_remove is called by the PCI subsystem to alert the driver
 * that it should release a PCI device.  The could be caused by a
 * Hot-Plug event, or because the driver is going to be removed from
 * memory.
 **/
static void rnpgbe_remove(struct pci_dev *pdev)
{
	struct rnpgbe_adapter *adapter = pci_get_drvdata(pdev);

#ifdef CONFIG_PCI_IOV
	/*
	 * Only disable SR-IOV on unload if the user specified the now
	 * deprecated max_vfs module parameter.
	 */
	rnpgbe_disable_sriov(adapter);
#endif
	rnpgbe_rm_adpater(adapter);
	pci_release_mem_regions(pdev);
	pci_disable_device(pdev);
}

/**
 * rnpgbe_io_error_detected - called when PCI error is detected
 * @pdev: Pointer to PCI device
 * @state: The current pci connection state
 *
 * This function is called after a PCI bus error affecting
 * this device has been detected.
 */
static pci_ers_result_t rnpgbe_io_error_detected(struct pci_dev *pdev,
						 pci_channel_state_t state)
{
	struct rnpgbe_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;

#ifdef CONFIG_PCI_IOV
	struct pci_dev *bdev, *vfdev;
	u32 dw0, dw1, dw2, dw3;
	int vf, pos;
	u16 req_id, pf_func;

	if (adapter->num_vfs == 0)
		goto skip_bad_vf_detection;

	bdev = pdev->bus->self;
	while (bdev && (pci_pcie_type(bdev) != PCI_EXP_TYPE_ROOT_PORT))
		bdev = bdev->bus->self;

	if (!bdev)
		goto skip_bad_vf_detection;

	pos = pci_find_ext_capability(bdev, PCI_EXT_CAP_ID_ERR);
	if (!pos)
		goto skip_bad_vf_detection;

	pci_read_config_dword(bdev, pos + PCI_ERR_HEADER_LOG, &dw0);
	pci_read_config_dword(bdev, pos + PCI_ERR_HEADER_LOG + 4, &dw1);
	pci_read_config_dword(bdev, pos + PCI_ERR_HEADER_LOG + 8, &dw2);
	pci_read_config_dword(bdev, pos + PCI_ERR_HEADER_LOG + 12, &dw3);

	req_id = dw1 >> 16;
	/* On the n500 if bit 7 of the requestor ID is set then it's a VF ? */
	if (!(req_id & 0x0080))
		goto skip_bad_vf_detection;

	pf_func = req_id & 0x01;
	if ((pf_func & 1) == (pdev->devfn & 1)) {
		unsigned int device_id;

		vf = (req_id & 0x7F) >> 1;
		e_dev_err("VF %d has caused a PCIe error\n", vf);
		e_dev_err("TLP: dw0: %8.8x\tdw1: %8.8x\tdw2: "
			  "%8.8x\tdw3: %8.8x\n",
			  dw0, dw1, dw2, dw3);

		device_id = PCI_DEVICE_ID_N500_VF;

		/* Find the pci device of the offending VF */
		vfdev = pci_get_device(PCI_VENDOR_ID_MUCSE, device_id, NULL);
		while (vfdev) {
			if (vfdev->devfn == (req_id & 0xFF))
				break;
			vfdev = pci_get_device(PCI_VENDOR_ID_MUCSE, device_id,
					       vfdev);
		}
		/*
		 * There's a slim chance the VF could have been hot plugged,
		 * so if it is no longer present we don't need to issue the
		 * VFLR.  Just clean up the AER in that case.
		 */
		if (vfdev) {
			e_dev_err("Issuing VFLR to VF %d\n", vf);
			pci_write_config_dword(vfdev, 0xA8, 0x00008000);
			/* Free device reference count */
			pci_dev_put(vfdev);
		}

		pci_aer_clear_nonfatal_status(pdev);
	}

	/*
	 * Even though the error may have occurred on the other port
	 * we still need to increment the vf error reference count for
	 * both ports because the I/O resume function will be called
	 * for both of them.
	 */
	adapter->vferr_refcount++;

	return PCI_ERS_RESULT_RECOVERED;

skip_bad_vf_detection:
#endif /* CONFIG_PCI_IOV */
	netif_device_detach(netdev);

	if (state == pci_channel_io_perm_failure)
		return PCI_ERS_RESULT_DISCONNECT;

	if (netif_running(netdev))
		rnpgbe_down(adapter);
	pci_disable_device(pdev);
	/* Request a slot reset. */
	return PCI_ERS_RESULT_NEED_RESET;
}

/**
 * rnpgbe_io_slot_reset - called after the pci bus has been reset.
 * @pdev: Pointer to PCI device
 *
 * Restart the card from scratch, as if from a cold-boot.
 */
static pci_ers_result_t rnpgbe_io_slot_reset(struct pci_dev *pdev)
{
	pci_ers_result_t result = PCI_ERS_RESULT_NONE;

	struct rnpgbe_adapter *adapter = pci_get_drvdata(pdev);

	if (pci_enable_device_mem(pdev)) {
		e_err(probe, "Cannot re-enable PCI device after reset.\n");
		result = PCI_ERS_RESULT_DISCONNECT;
	} else {
		/* we need this */
		smp_mb__before_atomic();

		pci_set_master(pdev);
		pci_restore_state(pdev);
		pci_save_state(pdev);
		pci_wake_from_d3(pdev, false);
		rnpgbe_reset(adapter);
		result = PCI_ERS_RESULT_RECOVERED;
	}

	pci_aer_clear_nonfatal_status(pdev);
	return result;
}

/**
 * rnpgbe_io_resume - called when traffic can start flowing again.
 * @pdev: Pointer to PCI device
 *
 * This callback is called when the error recovery driver tells us that
 * its OK to resume normal operation.
 */
static void rnpgbe_io_resume(struct pci_dev *pdev)
{
	struct rnpgbe_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;

#ifdef CONFIG_PCI_IOV
	if (adapter->vferr_refcount) {
		e_info(drv, "Resuming after VF err\n");
		adapter->vferr_refcount--;
		return;
	}

#endif
	if (netif_running(netdev))
		rnpgbe_up(adapter);

	netif_device_attach(netdev);
}

static void pci_io_reset_prepare(struct pci_dev *pdev)
{
	struct device *dev = pci_dev_to_dev(pdev);

	rnpgbe_suspend(dev);
}

static void pci_io_reset_done(struct pci_dev *pdev)
{
	struct device *dev = pci_dev_to_dev(pdev);

	rnpgbe_resume(dev);
}

static const struct pci_error_handlers rnpgbe_err_handler = {
	.error_detected = rnpgbe_io_error_detected,
	.slot_reset = rnpgbe_io_slot_reset,
	.reset_prepare = pci_io_reset_prepare,
	.reset_done = pci_io_reset_done,
	.resume = rnpgbe_io_resume,
};

static const struct dev_pm_ops rnpgbe_pm_ops = {
	.suspend = rnpgbe_suspend,
	.resume = rnpgbe_resume,
	.freeze = rnpgbe_freeze,
	.thaw = rnpgbe_thaw,
	.poweroff = rnpgbe_suspend,
	.restore = rnpgbe_resume,
};

static struct pci_driver rnpgbe_driver = {
	.name = rnpgbe_driver_name,
	.id_table = rnpgbe_pci_tbl,
	.probe = rnpgbe_probe,
	.remove = rnpgbe_remove,
	.driver = {
		.pm = &rnpgbe_pm_ops,
	},
	.shutdown = rnpgbe_shutdown,
	.sriov_configure = rnpgbe_pci_sriov_configure,
	.err_handler = &rnpgbe_err_handler
};

static int __init rnpgbe_init_module(void)
{
	int ret;

	pr_info("%s - version %s\n", rnpgbe_driver_string,
		rnpgbe_driver_version);
	pr_info("%s\n", rnpgbe_copyright);
	rnpgbe_wq = create_singlethread_workqueue(rnpgbe_driver_name);

	if (!rnpgbe_wq) {
		pr_err("%s: Failed to create workqueue\n", rnpgbe_driver_name);
		return -ENOMEM;
	}

	rnpgbe_dbg_init();

	ret = pci_register_driver(&rnpgbe_driver);
	if (ret) {
		destroy_workqueue(rnpgbe_wq);
		rnpgbe_dbg_exit();
		return ret;
	}

	return 0;
}

module_init(rnpgbe_init_module);

static void __exit rnpgbe_exit_module(void)
{
	pci_unregister_driver(&rnpgbe_driver);

	destroy_workqueue(rnpgbe_wq);

	rnpgbe_dbg_exit();

	rcu_barrier(); /* Wait for completion of call_rcu()'s */
}

module_exit(rnpgbe_exit_module);
