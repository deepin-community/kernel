// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#include <linux/types.h>
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
#include <linux/if_bridge.h>
#include <linux/prefetch.h>
#include <linux/capability.h>
#include <linux/sort.h>
#include <net/xdp_sock_drv.h>
#include "rnpm.h"
#include "rnpm_common.h"
#include "rnpm_sriov.h"
#include "rnpm_ptp.h"
#include "rnpm_tc.h"
#include "rnpm_mbx.h"
#include "rnpm_mbx_fw.h"
#include "version.h"
#include "rnpm_mpe.h"
#include <net/vxlan.h>
#include <net/udp_tunnel.h>

#define TX_IRQ_MISS_REDUCE
char rnpm_driver_name[] = "rnpm";
char rnpm_port_name[] = "enp";

static const char rnpm_driver_string[] =
	"mucse 4/8 ports 1/10 Gigabit PCI Express Network Driver";
static char rnpm_default_device_descr[] __maybe_unused =
	"mucse(R) 4/8 ports 1/10 Gigabit Network Connection";
#define DRV_VERSION "1.1.0"
const char rnpm_driver_version[] = DRV_VERSION;
static const char rnpm_copyright[] =
	"Copyright (c) 2020-2025 mucse Corporation.";

static struct rnpm_info *rnpm_info_tbl[] = {
	[board_n10] = &rnpm_n10_info,
	[board_n400_4x1G] = &rnpm_n400_4x1G_info,
};

#ifdef CONFIG_RNPM_DISABLE_PACKET_SPLIT
static bool rnpm_alloc_mapped_skb(struct rnpm_ring *rx_ring,
				  struct rnpm_rx_buffer *bi);

#else
static void rnpm_pull_tail(struct sk_buff *skb);
#ifdef RNPM_OPTM_WITH_LPAGE
static bool rnpm_alloc_mapped_page(struct rnpm_ring *rx_ring,
				   struct rnpm_rx_buffer *bi,
				   union rnpm_rx_desc *rx_desc, u16 bufsz,
				   u64 fun_id);
static void rnpm_put_rx_buffer(struct rnpm_ring *rx_ring,
			       struct rnpm_rx_buffer *rx_buffer);

#else
static bool rnpm_alloc_mapped_page(struct rnpm_ring *rx_ring,
				   struct rnpm_rx_buffer *bi);
static void rnpm_put_rx_buffer(struct rnpm_ring *rx_ring,
			       struct rnpm_rx_buffer *rx_buffer,
			       struct sk_buff *skb);
#endif
#endif

#define DEFAULT_MIN_SIZE 54
#define DEFAULT_MAX_SIZE 1522
#define MIN_RX_PKT_LEN_LIMT 33

static struct pci_device_id rnpm_pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, 0x1060),
	  .driver_data = board_n10 },
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, 0x1C60),
	  .driver_data = board_n10 },
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, 0x1020),
	  .driver_data = board_n10 },
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, 0x1C20),
	  .driver_data = board_n10 },

	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, 0x1021),
	  .driver_data = board_n400_4x1G },
	{ PCI_DEVICE(PCI_VENDOR_ID_MUCSE, 0x1c21),
	  .driver_data = board_n400_4x1G },
	{
		0,
	},
};

MODULE_DEVICE_TABLE(pci, rnpm_pci_tbl);

static unsigned int mac_loop_en;
module_param(mac_loop_en, uint, 0000);

#ifdef CONFIG_PCI_IOV
static unsigned int max_vfs;
module_param(max_vfs, uint, 0000);
MODULE_PARM_DESC(max_vfs,
		 "Maximum number of virtual functions to allocate per \
		physical function - default is zero and maximum \
		value is 127");
#endif /* CONFIG_PCI_IOV */

#define DEFAULT_MSG_ENABLE \
	(NETIF_MSG_DRV | NETIF_MSG_PROBE | NETIF_MSG_LINK)
static int debug = -1;
module_param(debug, int, 0000);
MODULE_PARM_DESC(debug, "Debug level (0=none,...,16=all)");

static unsigned int pf_msix_counts_set;
module_param(pf_msix_counts_set, uint, 0000);
MODULE_PARM_DESC(pf_msix_counts_set, "set msix count by one pf");

static unsigned int pf_support_vlan0;
module_param(pf_support_vlan0, uint, 0000);
MODULE_PARM_DESC(pf_support_vlan0, "pf support vlan0");

static unsigned int pf1_use_port;
module_param(pf1_use_port, uint, 0000);
MODULE_PARM_DESC(pf1_use_port, "pf1 use port");

static unsigned int fix_eth_name;
module_param(fix_eth_name, uint, 0000);
MODULE_PARM_DESC(fix_eth_name, "set eth adapter name to rnpmXX");

#ifndef NO_PTP
static int module_enable_ptp = 1;
module_param(module_enable_ptp, uint, 0000);
MODULE_PARM_DESC(module_enable_ptp,
		 "enable ptp feature, disabled default");
#endif

unsigned int mpe_src_port;
module_param(mpe_src_port, uint, 0000);
MODULE_PARM_DESC(mpe_src_port, "mpe src port");

unsigned int mpe_pkt_version;
module_param(mpe_pkt_version, uint, 0000);
MODULE_PARM_DESC(mpe_pkt_version, "ipv4 or ipv6 src port");

static int port_valid_pf0 = 0xf;
module_param(port_valid_pf0, uint, 0000);
MODULE_PARM_DESC(port_valid_pf0, "pf0 valid (only in 8 ports mode");

static int port_valid_pf1 = 0xf;
module_param(port_valid_pf1, uint, 0000);
MODULE_PARM_DESC(port_valid_pf1, "pf1 valid (only in 8 ports mode");

static unsigned int port_names_pf0 = 0x03020100;
module_param(port_names_pf0, uint, 0000);
MODULE_PARM_DESC(port_names_pf0, "pf0 names (only in 8 ports mode");

static unsigned int port_names_pf1 = 0x03020100;
module_param(port_names_pf1, uint, 0000);
MODULE_PARM_DESC(port_names_pf1, "pf1 names (only in 8 ports mode");

static int fw_10g_1g_auto_det;
module_param(fw_10g_1g_auto_det, uint, 0000);

static int force_speed_ablity_pf0;
module_param(force_speed_ablity_pf0, uint, 0000);
MODULE_PARM_DESC(force_speed_ablity_pf0,
		 "allow to force speed 1/10G for fiber on pf0");

static int force_speed_ablity_pf1;
module_param(force_speed_ablity_pf1, uint, 0000);
MODULE_PARM_DESC(force_speed_ablity_pf1,
		 "allow to force speed 1/10G for fiber on pf1");

MODULE_PARM_DESC(
	fw_10g_1g_auto_det,
	"enable 4x10G cards partially supported 10G and 1G SFP at the same time ");

MODULE_AUTHOR("Mucse Corporation, <mucse@mucse.com>");
MODULE_DESCRIPTION("Mucse(R) 1/10 Gigabit PCI Express Network Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

unsigned int min_rx_pkt_len = DEFAULT_MIN_SIZE;
module_param(min_rx_pkt_len, uint, 0000);
MODULE_PARM_DESC(min_rx_pkt_len, "Minumum RX packet length");

static int enable_hi_dma;

#ifdef RNPM_DEBUG_KYLIN_SM
static void buf_dump_skb(struct net_device *netdev, void *buf, int len)
{
	int i, offset = 0;
	int msg_len = 1024;
	u8 msg_buf[1024];
	u8 *ptr = (u8 *)buf;

	offset += snprintf(msg_buf + offset, msg_len,
			   "=== data:%p==\n000: ", buf);

	for (i = 0; i < len; ++i) {
		if ((i != 0) && (i % 16) == 0 &&
		    (offset >= (1024 - 10 * 16))) {
			netdev_info(netdev, "%s\n", msg_buf);
			offset = 0;
		}

		if ((i != 0) && (i % 16) == 0)
			offset += snprintf(msg_buf + offset, msg_len,
					   "\n%03x: ", i);
		offset += snprintf(msg_buf + offset, msg_len, "%02x, ",
				   ptr[i]);
	}

	offset += snprintf(msg_buf + offset, msg_len, "\n");
	netdev_info(netdev, "%s\n", msg_buf);
}

static u32 checksum_nofold(void *data, size_t len, u32 sum)
{
	u16 *words = data;
	int i;

	for (i = 0; i < len / 2; i++)
		sum += words[i];

	if (len & 1)
		sum += ((char *)data)[len - 1];
	return sum;
}

static u16 checksum_fold(void *data, size_t len, u32 sum)
{
	sum = checksum_nofold(data, len, sum);
	while (sum > 0xffff)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~sum;
}

static u16 udp_checksum(void *buf, struct iphdr *iph)
{
	struct pseudo_header4 {
		u32 saddr;
		u32 daddr;
		u16 protocol;
		u16 payload_len;
	} ph4;

	u32 sum = 0;
	u16 csum = 0;

	ph4.saddr = iph->saddr;
	ph4.daddr = iph->daddr;
	ph4.protocol = htons(IPPROTO_UDP);
	ph4.payload_len = htons(sizeof(struct udphdr) + 1472);

	sum = checksum_nofold(&ph4, sizeof(ph4), 0);
	csum = checksum_fold(buf, sizeof(struct udphdr) + 1472, sum);
	return csum ? csum : 0xffff;
}

__maybe_unused void check_udp_chksum(struct net_device *netdev, void *data,
				     struct rnpm_rx_buffer *rx_buffer)
{
	struct iphdr *iph = (struct iphdr *)(data + 14);
	struct udphdr *udh = (struct udphdr *)(data + 14 + 20);

	if (iph->protocol == IPPROTO_UDP && ntohs(iph->tot_len) == 1500) {
		__sum16 csum_temp = udh->check;
		__sum16 csum;

		udh->check = 0;
		csum = udp_checksum(udh, iph);
		if (csum != csum_temp)
			buf_dump_skb(netdev, data, 1514);
		udh->check = csum_temp;
	}
}
#endif

#if (PAGE_SIZE < 8192)
#define RNPM_MAX_2K_FRAME_BUILD_SKB (RNPM_RXBUFFER_1536 - NET_IP_ALIGN)
#define RNPM_2K_TOO_SMALL_WITH_PADDING        \
	((NET_SKB_PAD + RNPM_RXBUFFER_1536) > \
	 SKB_WITH_OVERHEAD(RNPM_RXBUFFER_2K))

static inline int rnpm_compute_pad(int rx_buf_len)
{
	int page_size, pad_size;

	page_size = ALIGN(rx_buf_len, PAGE_SIZE / 2);
	pad_size = SKB_WITH_OVERHEAD(page_size) - rx_buf_len;

	return pad_size;
}

static inline int rnpm_skb_pad(void)
{
	int rx_buf_len;

	/* If a 2K buffer cannot handle a standard Ethernet frame then
	 * optimize padding for a 3K buffer instead of a 1.5K buffer.
	 *
	 * For a 3K buffer we need to add enough padding to allow for
	 * tailroom due to NET_IP_ALIGN possibly shifting us out of
	 * cache-line alignment.
	 */
	if (RNPM_2K_TOO_SMALL_WITH_PADDING)
		rx_buf_len =
			RNPM_RXBUFFER_3K + SKB_DATA_ALIGN(NET_IP_ALIGN);
	else
		rx_buf_len = RNPM_RXBUFFER_1536;

	/* if needed make room for NET_IP_ALIGN */
	rx_buf_len -= NET_IP_ALIGN;
	return rnpm_compute_pad(rx_buf_len);
}

#define RNPM_SKB_PAD rnpm_skb_pad()
#else
#define RNPM_SKB_PAD (NET_SKB_PAD + NET_IP_ALIGN)
#endif

#define __RNPM_RX_OFFSET_BY_L3_COHERENCY_LINE_SIZE 128
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
static inline unsigned int rnpm_rx_offset(struct rnpm_ring *rx_ring)
{
	struct rnpm_pf_adapter *pf_adapter =
		rx_ring->q_vector->adapter->pf_adapter;
	int size = pf_adapter->cpu_l3_cache_size;

	if (size == __RNPM_RX_OFFSET_BY_L3_COHERENCY_LINE_SIZE)
		return __RNPM_RX_OFFSET_BY_L3_COHERENCY_LINE_SIZE;
	return ring_uses_build_skb(rx_ring) ? RNPM_SKB_PAD : 0;
}
#endif

void rnpm_service_event_schedule(struct rnpm_adapter *adapter)
{
	if (!test_bit(__RNPM_DOWN, &adapter->state) &&
	    !test_and_set_bit(__RNPM_SERVICE_SCHED, &adapter->state)) {
		schedule_work(&adapter->service_task);
		adapter->service_count++;
	}
}

static void
rnpm_pf_service_event_schedule(struct rnpm_pf_adapter *pf_adapter)
{
	schedule_work(&pf_adapter->service_task);
}

static void rnpm_service_event_complete(struct rnpm_adapter *adapter)
{
	BUG_ON(!test_bit(__RNPM_SERVICE_SCHED, &adapter->state));
	clear_bit(__RNPM_SERVICE_SCHED, &adapter->state);
}

/**
 * rnpm_set_ivar - set the ring_vector registers,
 * mapping interrupt causes to vectors
 * @adapter: pointer to adapter struct
 * @queue: queue to map the corresponding interrupt to
 * @msix_vector: the vector to map to the corresponding queue
 *
 */
static void rnpm_set_ring_vector(struct rnpm_adapter *adapter,
				 u8 rnpm_queue, u8 rnpm_msix_vector)
{
	struct rnpm_hw *hw = &adapter->hw;
	u32 data = 0;

	data = hw->pfvfnum << 24;
	data |= (rnpm_msix_vector << 8);
	data |= (rnpm_msix_vector << 0);

	DPRINTK(IFUP, INFO,
		"Set Ring-Vector queue:%d (reg:0x%x) <-- Rx-MSIX:%d, Tx-MSIX:%d\n",
		rnpm_queue, RING_VECTOR(rnpm_queue), rnpm_msix_vector,
		rnpm_msix_vector);
	rnpm_wr_reg(hw->ring_msix_base + RING_VECTOR(rnpm_queue), data);
}

static inline void rnpm_irq_rearm_queues(struct rnpm_adapter *adapter,
					 u64 qmask)
{
}

void rnpm_unmap_and_free_tx_resource(struct rnpm_ring *ring,
				     struct rnpm_tx_buffer *tx_buffer)
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
			       dma_unmap_len(tx_buffer, len),
			       DMA_TO_DEVICE);
	}
	tx_buffer->next_to_watch = NULL;
	tx_buffer->skb = NULL;
	dma_unmap_len_set(tx_buffer, len, 0);
}

static u64 rnpm_get_tx_completed(struct rnpm_ring *ring)
{
	return ring->stats.packets;
}

static u64 rnpm_get_tx_pending(struct rnpm_ring *ring)
{
	struct rnpm_adapter *adapter = netdev_priv(ring->netdev);
	struct rnpm_hw *hw = &adapter->hw;

	u32 head = rd32(
		hw, RNPM_DMA_REG_TX_DESC_BUF_HEAD(ring->rnpm_queue_idx));
	u32 tail = rd32(
		hw, RNPM_DMA_REG_TX_DESC_BUF_TAIL(ring->rnpm_queue_idx));

	if (head != tail)
		return (head < tail) ? tail - head :
				       (tail + ring->count - head);
	return 0;
}

static inline bool rnpm_check_tx_hang(struct rnpm_ring *tx_ring)
{
	u32 tx_done = rnpm_get_tx_completed(tx_ring);
	u32 tx_done_old = tx_ring->tx_stats.tx_done_old;
	u32 tx_pending = rnpm_get_tx_pending(tx_ring);
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
		ret = test_and_set_bit(__RNPM_HANG_CHECK_ARMED,
				       &tx_ring->state);
	} else {
		/* update completed stats and continue */
		tx_ring->tx_stats.tx_done_old = tx_done;
		/* reset the countdown */
		clear_bit(__RNPM_HANG_CHECK_ARMED, &tx_ring->state);
	}

	return ret;
}

/**
 * rnpm_tx_timeout_reset - initiate reset due to Tx timeout
 * @adapter: driver private struct
 **/
static void rnpm_tx_timeout_reset(struct rnpm_adapter *adapter)
{
	if (!test_bit(__RNPM_DOWN, &adapter->state)) {
		set_bit(RNPM_PF_RESET, &adapter->pf_adapter->flags);
		e_warn(drv, "initiating reset due to tx timeout\n");
	}
}

static void rnpm_check_restart_tx(struct rnpm_q_vector *q_vector,
				  struct rnpm_ring *tx_ring)
{
	struct rnpm_adapter *adapter = q_vector->adapter;

#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)
	if (likely(netif_carrier_ok(tx_ring->netdev) &&
		   (rnpm_desc_unused(tx_ring) >= TX_WAKE_THRESHOLD))) {
		/* Make sure that anybody stopping the queue after this
		 * sees the new next_to_clean.
		 */
		smp_mb();
		if (__netif_subqueue_stopped(tx_ring->netdev,
					     tx_ring->queue_index) &&
		    !test_bit(__RNPM_DOWN, &adapter->state)) {
			netif_wake_subqueue(tx_ring->netdev,
					    tx_ring->queue_index);
			++tx_ring->tx_stats.restart_queue;
		}
	}
}

/**
 * rnpm_clean_tx_irq - Reclaim resources after transmit completes
 * @q_vector: structure containing interrupt and ring information
 * @tx_ring: tx ring to clean
 **/
static bool rnpm_clean_tx_irq(struct rnpm_q_vector *q_vector,
			      struct rnpm_ring *tx_ring, int napi_budget)
{
	struct rnpm_adapter *adapter = q_vector->adapter;
	struct rnpm_tx_buffer *tx_buffer;
	struct rnpm_tx_desc *tx_desc;
	unsigned int total_bytes = 0, total_packets = 0;
	unsigned int budget = q_vector->tx.work_limit;
	unsigned int i = tx_ring->next_to_clean;

	if (test_bit(__RNPM_DOWN, &adapter->state))
		return true;
	tx_ring->tx_stats.poll_count++;
	tx_buffer = &tx_ring->tx_buffer_info[i];
	tx_desc = RNPM_TX_DESC(tx_ring, i);
	i -= tx_ring->count;

	do {
		struct rnpm_tx_desc *eop_desc = tx_buffer->next_to_watch;

		if (!eop_desc)
			break;

		/* prevent any other reads prior to eop_desc */
		smp_rmb();
		/* if eop DD is not set pending work has not been completed */
		if (!(eop_desc->vlan_cmd & cpu_to_le32(RNPM_TXD_STAT_DD)))
			break;

		/* clear next_to_watch to prevent false hangs */
		tx_buffer->next_to_watch = NULL;
		total_bytes += tx_buffer->bytecount;
		total_packets += tx_buffer->gso_segs;

		napi_consume_skb(tx_buffer->skb, napi_budget);
		dma_unmap_single(tx_ring->dev,
				 dma_unmap_addr(tx_buffer, dma),
				 dma_unmap_len(tx_buffer, len),
				 DMA_TO_DEVICE);

		/* clear tx_buffer data */
		tx_buffer->skb = NULL;
		dma_unmap_len_set(tx_buffer, len, 0);

		while (tx_desc != eop_desc) {
			buf_dump_line("desc %d  ", i + tx_ring->count,
				      tx_desc, sizeof(*tx_desc));

			tx_buffer++;
			tx_desc++;
			i++;
			if (unlikely(!i)) {
				i -= tx_ring->count;
				tx_buffer = tx_ring->tx_buffer_info;
				tx_desc = RNPM_TX_DESC(tx_ring, 0);
			}

			/* unmap any remaining paged data */
			if (dma_unmap_len(tx_buffer, len)) {
				dma_unmap_page(
					tx_ring->dev,
					dma_unmap_addr(tx_buffer, dma),
					dma_unmap_len(tx_buffer, len),
					DMA_TO_DEVICE);
				dma_unmap_len_set(tx_buffer, len, 0);
			}
		}
		tx_buffer++;
		tx_desc++;
		i++;
		if (unlikely(!i)) {
			i -= tx_ring->count;
			tx_buffer = tx_ring->tx_buffer_info;
			tx_desc = RNPM_TX_DESC(tx_ring, 0);
		}
		prefetch(tx_desc);
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
	tx_ring->tx_stats.send_done_bytes += total_bytes;
	netdev_tx_completed_queue(txring_txq(tx_ring), total_packets,
				  total_bytes);
#ifndef TX_IRQ_MISS_REDUCE
#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)
	if (likely(netif_carrier_ok(tx_ring->netdev) &&
		   (rnpm_desc_unused(tx_ring) >= TX_WAKE_THRESHOLD))) {
		/* Make sure that anybody stopping the queue after this
		 * sees the new next_to_clean.
		 */
		smp_mb();
		if (__netif_subqueue_stopped(tx_ring->netdev,
					     tx_ring->queue_index) &&
		    !test_bit(__RNPM_DOWN, &adapter->state)) {
			netif_wake_subqueue(tx_ring->netdev,
					    tx_ring->queue_index);
			++tx_ring->tx_stats.restart_queue;
		}
	}
#endif
	return !!budget;
}

static inline void rnpm_rx_hash(struct rnpm_ring *ring,
				union rnpm_rx_desc *rx_desc,
				struct sk_buff *skb)
{
	int rss_type;

	if (!(ring->netdev->features & NETIF_F_RXHASH))
		return;
#define RNPM_RSS_TYPE_MASK 0xc0
	rss_type = rx_desc->wb.cmd & RNPM_RSS_TYPE_MASK;
	skb_set_hash(skb, le32_to_cpu(rx_desc->wb.rss_hash),
		     rss_type ? PKT_HASH_TYPE_L4 : PKT_HASH_TYPE_L3);
}

/**
 * rnpm_rx_checksum - indicate in skb if hw indicated a good cksum
 * @ring: structure containing ring specific data
 * @rx_desc: current Rx descriptor being processed
 * @skb: skb currently being received and modified
 **/
static inline void rnpm_rx_checksum(struct rnpm_ring *ring,
				    union rnpm_rx_desc *rx_desc,
				    struct sk_buff *skb)
{
	bool encap_pkt = false;

	skb_checksum_none_assert(skb);
	/* Rx csum disabled */
	if (!(ring->netdev->features & NETIF_F_RXCSUM))
		return;

	/* vxlan packet handle ? */
	if (rnpm_get_stat(rx_desc, RNPM_RXD_STAT_TUNNEL_MASK) ==
	    RNPM_RXD_STAT_TUNNEL_VXLAN) {
		encap_pkt = true;
		skb->encapsulation = 1;
		skb->ip_summed = CHECKSUM_NONE;
	}

	/* if outer L3/L4  error */
	/* must in promisc mode */
	if (rnpm_test_staterr(rx_desc, RNPM_RXD_STAT_ERR_MASK) &&
	    !ignore_veb_pkg_err(ring->q_vector->adapter, rx_desc))
		return;

	ring->rx_stats.csum_good++;
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	if (encap_pkt) {
		/* If we checked the outer header let the stack know */
		skb->csum_level = 1;
	}
}

static inline void rnpm_update_rx_tail(struct rnpm_ring *rx_ring, u32 val)
{
	rx_ring->next_to_use = val;
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
	rx_ring->next_to_alloc = val;
#endif
	/*
	 * Force memory writes to complete before letting h/w
	 * know there are new descriptors to fetch.  (Only
	 * applicable for weak-ordered memory model archs,
	 * such as IA-64).
	 */
	wmb();
	rnpm_wr_reg(rx_ring->tail, val);
}

#ifdef RNPM_OPTM_WITH_LPAGE
/**
 * rnpm_alloc_rx_buffers - Replace used receive buffers
 * @rx_ring: ring to place buffers on
 * @cleaned_count: number of buffers to replace
 **/
static bool rnpm_alloc_rx_buffers(struct rnpm_ring *rx_ring,
				  u16 cleaned_count)
{
	union rnpm_rx_desc *rx_desc;
	struct rnpm_rx_buffer *bi;
	u16 i = rx_ring->next_to_use;
	u64 fun_id = ((u64)(rx_ring->pfvfnum) << (32 + 24));
	bool err = false;
	u16 bufsz;

	if (!cleaned_count)
		return err;

	rx_desc = RNPM_RX_DESC(rx_ring, i);
	BUG_ON(rx_desc == NULL);
	bi = &rx_ring->rx_buffer_info[i];
	BUG_ON(bi == NULL);

	i -= rx_ring->count;
	bufsz = rnpm_rx_bufsz(rx_ring);
	do {
		int count = 1;
		struct page *page;

		if (!rnpm_alloc_mapped_page(rx_ring, bi, rx_desc, bufsz,
					    fun_id)) {
			err = true;
			break;
		}

		page = bi->page;
		rx_desc->resv_cmd = 0;
		rx_desc++;
		i++;
		bi++;

		if (unlikely(!i)) {
			rx_desc = RNPM_RX_DESC(rx_ring, 0);
			bi = rx_ring->rx_buffer_info;
			i -= rx_ring->count;
		}

		rx_desc->resv_cmd = 0;
		cleaned_count--;

		while (count < rx_ring->rx_page_buf_nums &&
		       cleaned_count) {
			dma_addr_t dma;

			bi->page_offset = rx_ring->rx_per_buf_mem * count +
					  rnpm_rx_offset(rx_ring);
			/* map page for use */
			dma = dma_map_page_attrs(rx_ring->dev, page,
						 bi->page_offset, bufsz,
						 DMA_FROM_DEVICE,
						 RNPM_RX_DMA_ATTR);
			if (dma_mapping_error(rx_ring->dev, dma)) {
				rx_ring->rx_stats.alloc_rx_page_failed++;
				break;
			}

			bi->dma = dma;
			bi->page = page;

			page_ref_add(page, USHRT_MAX);
			bi->pagecnt_bias = USHRT_MAX;

			/* sync the buffer for use by the device */
			dma_sync_single_range_for_device(rx_ring->dev,
							 bi->dma, 0, bufsz,
							 DMA_FROM_DEVICE);
			/*
			 * Refresh the desc even if buffer_addrs didn't change
			 * because each write-back erases this info.
			 */
			rx_desc->pkt_addr = cpu_to_le64(bi->dma + fun_id);
			rx_desc->resv_cmd = 0;

			rx_desc++;
			bi++;
			i++;
			if (unlikely(!i)) {
				rx_desc = RNPM_RX_DESC(rx_ring, 0);
				bi = rx_ring->rx_buffer_info;
				i -= rx_ring->count;
			}
			count++;
			cleaned_count--;
		}
	} while (cleaned_count);

	i += rx_ring->count;

	if (rx_ring->next_to_use != i)
		rnpm_update_rx_tail(rx_ring, i);
	return err;
}
#else
/**
 * rnpm_alloc_rx_buffers - Replace used receive buffers
 * @rx_ring: ring to place buffers on
 * @cleaned_count: number of buffers to replace
 **/
static bool rnpm_alloc_rx_buffers(struct rnpm_ring *rx_ring,
				  u16 cleaned_count)
{
	union rnpm_rx_desc *rx_desc;
	struct rnpm_rx_buffer *bi;
	u16 i = rx_ring->next_to_use;
	u64 fun_id = ((u64)(rx_ring->pfvfnum) << (32 + 24));
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
	u16 bufsz;
#endif
	bool err = false;

	if (!cleaned_count)
		return err;

	rx_desc = RNPM_RX_DESC(rx_ring, i);
	BUG_ON(rx_desc == NULL);
	bi = &rx_ring->rx_buffer_info[i];
	BUG_ON(bi == NULL);
	i -= rx_ring->count;
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
	bufsz = rnpm_rx_bufsz(rx_ring);
#endif

	do {
#ifdef CONFIG_RNPM_DISABLE_PACKET_SPLIT
		if (!rnpm_alloc_mapped_skb(rx_ring, bi)) {
			err = true;
			break;
		}
#else
		if (!rnpm_alloc_mapped_page(rx_ring, bi)) {
			err = true;
			break;
		}
		dma_sync_single_range_for_device(rx_ring->dev, bi->dma,
						 bi->page_offset, bufsz,
						 DMA_FROM_DEVICE);
#endif
		/*
		 * Refresh the desc even if buffer_addrs didn't change
		 * because each write-back erases this info.
		 */
#ifdef CONFIG_RNPM_DISABLE_PACKET_SPLIT
		rx_desc->pkt_addr = cpu_to_le64(bi->dma + fun_id);
#else
		rx_desc->pkt_addr =
			cpu_to_le64(bi->dma + bi->page_offset + fun_id);
#endif
		/* clean dd */
		rx_desc->resv_cmd = 0;
		rx_desc++;
		bi++;
		i++;
		if (unlikely(!i)) {
			rx_desc = RNPM_RX_DESC(rx_ring, 0);
			bi = rx_ring->rx_buffer_info;
			i -= rx_ring->count;
		}
		cleaned_count--;
	} while (cleaned_count);
	i += rx_ring->count;
	if (rx_ring->next_to_use != i)
		rnpm_update_rx_tail(rx_ring, i);
	return err;
}
#endif
/**
 * rnpm_get_headlen - determine size of header for RSC/LRO/GRO/FCOE
 * @data: pointer to the start of the headers
 * @max_len: total length of section to find headers in
 *
 * This function is meant to determine the length of headers that will
 * be recognized by hardware for LRO, GRO, and RSC offloads.  The main
 * motivation of doing this is to only perform one pull for IPv4 TCP
 * packets so that we can do basic things like calculating the gso_size
 * based on the average data per packet.
 **/
__maybe_unused static unsigned int rnpm_get_headlen(unsigned char *data,
						    unsigned int max_len)
{
	union {
		unsigned char *network;
		struct ethhdr *eth;
		struct vlan_hdr *vlan;
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
	if (protocol == __constant_htons(ETH_P_8021Q)) {
		if ((hdr.network - data) > (max_len - VLAN_HLEN))
			return max_len;
		protocol = hdr.vlan->h_vlan_encapsulated_proto;
		hdr.network += VLAN_HLEN;
	}

	if (protocol == __constant_htons(ETH_P_IP)) {
		if ((hdr.network - data) >
		    (max_len - sizeof(struct iphdr)))
			return max_len;
		/* access ihl as a u8 to avoid unaligned access on ia64 */
		hlen = (hdr.network[0] & 0x0F) << 2;

		/* verify hlen meets minimum size requirements */
		if (hlen < sizeof(struct iphdr))
			return hdr.network - data;

		/* record next protocol if header is present */
		if (!(hdr.ipv4->frag_off & htons(IP_OFFSET)))
			nexthdr = hdr.ipv4->protocol;
	} else if (protocol == __constant_htons(ETH_P_IPV6)) {
		if ((hdr.network - data) >
		    (max_len - sizeof(struct ipv6hdr)))
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
		if ((hdr.network - data) >
		    (max_len - sizeof(struct tcphdr)))
			return max_len;
		/* access doff as a u8 to avoid unaligned access on ia64 */
		hlen = (hdr.network[12] & 0xF0) >> 2;
		/* verify hlen meets minimum size requirements */
		if (hlen < sizeof(struct tcphdr))
			return hdr.network - data;

		hdr.network += hlen;
	} else if (nexthdr == IPPROTO_UDP) {
		if ((hdr.network - data) >
		    (max_len - sizeof(struct udphdr)))
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

static void rnpm_set_rsc_gso_size(struct rnpm_ring *ring,
				  struct sk_buff *skb)
{
	u16 hdr_len = skb_headlen(skb);

	/* set gso_size to avoid messing up TCP MSS */
	skb_shinfo(skb)->gso_size = DIV_ROUND_UP((skb->len - hdr_len),
						 RNPM_CB(skb)->append_cnt);
	skb_shinfo(skb)->gso_type = SKB_GSO_TCPV4;
}

__maybe_unused static void rnpm_update_rsc_stats(struct rnpm_ring *rx_ring,
						 struct sk_buff *skb)
{
	/* if append_cnt is 0 then frame is not RSC */
	if (!RNPM_CB(skb)->append_cnt)
		return;

	rx_ring->rx_stats.rsc_count += RNPM_CB(skb)->append_cnt;
	rx_ring->rx_stats.rsc_flush++;

	rnpm_set_rsc_gso_size(rx_ring, skb);

	/* gso_size is computed using append_cnt so always clear it last */
	RNPM_CB(skb)->append_cnt = 0;
}
static void rnpm_rx_vlan(struct rnpm_ring *rx_ring,
			 union rnpm_rx_desc *rx_desc, struct sk_buff *skb)
{
#ifdef NETIF_F_HW_VLAN_CTAG_RX
	if ((netdev_ring(rx_ring)->features & NETIF_F_HW_VLAN_CTAG_RX) &&
	    rnpm_test_staterr(rx_desc, RNPM_RXD_STAT_VLAN_VALID)) {
#else
	if ((netdev_ring(rx_ring)->features & NETIF_F_HW_VLAN_RX) &&
	    rnpm_test_staterr(rx_desc, RNPM_RXD_STAT_VLAN_VALID)) {
#endif
		rx_ring->rx_stats.vlan_remove++;
		__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
				       le16_to_cpu(rx_desc->wb.vlan));
	}
}
/**
 * rnpm_process_skb_fields - Populate skb header fields from Rx descriptor
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @rx_desc: pointer to the EOP Rx descriptor
 * @skb: pointer to current skb being populated
 *
 * This function checks the ring, descriptor, and packet information in
 * order to populate the hash, checksum, VLAN, timestamp, protocol, and
 * other fields within the skb.
 **/
static void rnpm_process_skb_fields(struct rnpm_ring *rx_ring,
				    union rnpm_rx_desc *rx_desc,
				    struct sk_buff *skb)
{
	struct net_device *dev = rx_ring->netdev;

	rnpm_rx_hash(rx_ring, rx_desc, skb);
	rnpm_rx_checksum(rx_ring, rx_desc, skb);
	rnpm_rx_vlan(rx_ring, rx_desc, skb);
	skb_record_rx_queue(skb, rx_ring->queue_index);
	skb->protocol = eth_type_trans(skb, dev);
}

static void rnpm_rx_skb(struct rnpm_q_vector *q_vector,
			struct sk_buff *skb)
{
	napi_gro_receive(&q_vector->napi, skb);
}

#ifdef CONFIG_RNPM_DISABLE_PACKET_SPLIT
/**
 * rnpm_merge_active_tail - merge active tail into lro skb
 * @tail: pointer to active tail in frag_list
 *
 * This function merges the length and data of an active tail into the
 * skb containing the frag_list.  It resets the tail's pointer to the head,
 * but it leaves the heads pointer to tail intact.
 **/
static inline struct sk_buff *rnpm_merge_active_tail(struct sk_buff *tail)
{
	struct sk_buff *head = RNPM_CB(tail)->head;

	if (!head)
		return tail;

	head->len += tail->len;
	head->data_len += tail->len;
	head->truesize += tail->truesize;

	RNPM_CB(tail)->head = NULL;

	return head;
}

/**
 * rnpm_add_active_tail - adds an active tail into the skb frag_list
 * @head: pointer to the start of the skb
 * @tail: pointer to active tail to add to frag_list
 *
 * This function adds an active tail to the end of the frag list.  This tail
 * will still be receiving data so we cannot yet ad it's stats to the main
 * skb.  That is done via rnpm_merge_active_tail.
 **/
static inline void rnpm_add_active_tail(struct sk_buff *head,
					struct sk_buff *tail)
{
	struct sk_buff *old_tail = RNPM_CB(head)->tail;

	if (old_tail) {
		rnpm_merge_active_tail(old_tail);
		old_tail->next = tail;
	} else {
		skb_shinfo(head)->frag_list = tail;
	}

	RNPM_CB(tail)->head = head;
	RNPM_CB(head)->tail = tail;
}

/**
 * rnpm_close_active_frag_list - cleanup pointers on a frag_list skb
 * @head: pointer to head of an active frag list
 *
 * This function will clear the frag_tail_tracker pointer on an active
 * frag_list and returns true if the pointer was actually set
 **/
static inline bool rnpm_close_active_frag_list(struct sk_buff *head)
{
	struct sk_buff *tail = RNPM_CB(head)->tail;

	if (!tail)
		return false;

	rnpm_merge_active_tail(tail);
	RNPM_CB(head)->tail = NULL;
	return true;
}

#endif

#ifdef RNPM_OPTM_WITH_LPAGE
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
static bool rnpm_is_non_eop(struct rnpm_ring *rx_ring,
			    union rnpm_rx_desc *rx_desc)
{
	u32 ntc = rx_ring->next_to_clean + 1;

	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;

	prefetch(RNPM_RX_DESC(rx_ring, ntc));
	/* if we are the last buffer then there is nothing else to do */
	if (likely(rnpm_test_staterr(rx_desc, RNPM_RXD_STAT_EOP)))
		return false;

	rx_ring->rx_stats.non_eop_descs++;
	/* we should clean it since we used all info in it */
	rx_desc->wb.cmd = 0;
	return true;
}
#else
/**
 * rnpm_is_non_eop - process handling of non-EOP buffers
 * @rx_ring: Rx ring being processed
 * @rx_desc: Rx descriptor for current buffer
 * @skb: Current socket buffer containing buffer in progress
 *
 * This function updates next to clean.  If the buffer is an EOP buffer
 * this function exits returning false, otherwise it will place the
 * sk_buff in the next buffer to be chained and return true indicating
 * that this is in fact a non-EOP buffer.
 **/
static bool rnpm_is_non_eop(struct rnpm_ring *rx_ring,
			    union rnpm_rx_desc *rx_desc,
			    struct sk_buff *skb)
{
	u32 ntc = rx_ring->next_to_clean + 1;

	/* fetch, update, and store next to clean */
	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;

	prefetch(RNPM_RX_DESC(rx_ring, ntc));

	/* if we are the last buffer then there is nothing else to do */
	if (likely(rnpm_test_staterr(rx_desc, RNPM_RXD_STAT_EOP)))
		return false;
#ifndef CONFIG_RNPM_RNPM_DISABLE_PACKET_SPLIT
	/* place skb in next buffer to be received */
	rx_ring->rx_buffer_info[ntc].skb = skb;
#endif
	rx_ring->rx_stats.non_eop_descs++;
	/* we should clean it since we used all info in it */
	rx_desc->wb.cmd = 0;
	return true;
}
#endif

/* drop this packets if error */
static bool rnpm_check_csum_error(struct rnpm_ring *rx_ring,
				  union rnpm_rx_desc *rx_desc,
				  unsigned int size,
				  unsigned int *driver_drop_packets)
{
	struct net_device *netdev = rx_ring->netdev;
	bool err = false;
	bool drop_err = true;

	if (netdev->features & NETIF_F_RXCSUM) {
		if (unlikely(rnpm_test_staterr(rx_desc,
					       RNPM_RXD_STAT_ERR_MASK))) {
			rx_debug_printk(
				rx_ring,
				"rx error: VEB:%s mark:0x%x cmd:0x%x\n",
				(rx_ring->q_vector->adapter->flags &
				 RNPM_FLAG_SRIOV_ENABLED) ?
					"On" :
					"Off",
				rx_desc->wb.mark, rx_desc->wb.cmd);
			/* push this packet to stack if in promisc mode */
			rx_ring->rx_stats.csum_err++;

			if (((netdev->flags & IFF_PROMISC) ||
			     ((netdev->features & NETIF_F_RXALL)))) {
				drop_err = false;
			}

			/* if not ipv4 with l4 error, we should ignore l4 csum error */
			if (unlikely(rnpm_test_staterr(
					     rx_desc,
					     RNPM_RXD_STAT_L4_MASK) &&
				     (!(rx_desc->wb.rev1 &
					RNPM_RX_L3_TYPE_MASK)))) {
				rx_ring->rx_stats.csum_err--;
				goto skip_fix;
			}

			if (unlikely(rnpm_test_staterr(
				    rx_desc, RNPM_RXD_STAT_SCTP_MASK))) {
				if ((size > 60) &&
				    (rx_desc->wb.rev1 &
				     RNPM_RX_L3_TYPE_MASK)) {
					if (drop_err)
						err = true;
				} else {
					/* sctp less than 60 hw report err by mistake */
					rx_ring->rx_stats.csum_err--;
				}
			} else {
				if (drop_err)
					err = true;
			}
		}
	}

skip_fix:
	if (err) {
		u32 ntc = rx_ring->next_to_clean + 1;

#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
		struct rnpm_rx_buffer *rx_buffer;

		/* if eop add drop_packets */
		if (likely(rnpm_test_staterr(rx_desc, RNPM_RXD_STAT_EOP)))
			*driver_drop_packets = *driver_drop_packets + 1;

		/* we are reusing so sync this buffer for CPU use */
		rx_buffer =
			&rx_ring->rx_buffer_info[rx_ring->next_to_clean];
		dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma,
					      rx_buffer->page_offset,
					      RNPM_RXBUFFER_1536,
					      DMA_FROM_DEVICE);
#ifdef RNPM_OPTM_WITH_LPAGE
		rnpm_put_rx_buffer(rx_ring, rx_buffer);
#else
		rnpm_put_rx_buffer(rx_ring, rx_buffer, NULL);
#endif
#endif /* CONFIG_RNPM_DISABLE_PACKET_SPLIT */
		ntc = (ntc < rx_ring->count) ? ntc : 0;
		rx_ring->next_to_clean = ntc;
	}

	return err;
}

/**
 * rnpm_rx_ring_reinit - just reinit rx_ring with new count in ->reset_count
 * @rx_ring: rx descriptor ring to transact packets on
 */
static int rnpm_rx_ring_reinit(struct rnpm_adapter *adapter,
			       struct rnpm_ring *rx_ring)
{
	struct rnpm_ring *temp_ring = NULL;
	int err = 0;
	struct rnpm_hw *hw = &adapter->hw;

	temp_ring = vmalloc(array_size(1, sizeof(struct rnpm_ring)));
	if (!temp_ring)
		return -ENOMEM;

	if (rx_ring->count == rx_ring->reset_count)
		return 0;
	/* stop rx queue */
	rnpm_disable_rx_queue(adapter, rx_ring);
	memset(temp_ring, 0x00, sizeof(struct rnpm_ring));
	memcpy(temp_ring, rx_ring, sizeof(struct rnpm_ring));
	temp_ring->count = rx_ring->reset_count;
	err = rnpm_setup_rx_resources(temp_ring, adapter);
	if (err) {
		rnpm_free_rx_resources(temp_ring);
		goto err_setup;
	}
	rnpm_free_rx_resources(rx_ring);
	memcpy(rx_ring, temp_ring, sizeof(struct rnpm_ring));
	rnpm_configure_rx_ring(adapter, rx_ring);
err_setup:
	vfree(temp_ring);
	/* start rx */
	wr32(hw, RNPM_DMA_RX_START(rx_ring->rnpm_queue_idx), 1);
	return 0;
}

#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
#ifdef RNPM_OPTM_WITH_LPAGE
static bool rnpm_alloc_mapped_page(struct rnpm_ring *rx_ring,
				   struct rnpm_rx_buffer *bi,
				   union rnpm_rx_desc *rx_desc, u16 bufsz,
				   u64 fun_id)
{
	struct page *page = bi->page;
	dma_addr_t dma;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page))
		return true;

	page = dev_alloc_pages(RNPM_ALLOC_PAGE_ORDER);
	if (unlikely(!page)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	bi->page_offset = rnpm_rx_offset(rx_ring);

	/* map page for use */
	dma = dma_map_page_attrs(rx_ring->dev, page, bi->page_offset,
				 bufsz, DMA_FROM_DEVICE, RNPM_RX_DMA_ATTR);
	/*
	 * if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		__free_pages(page, RNPM_ALLOC_PAGE_ORDER);
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}
	bi->dma = dma;
	bi->page = page;
	bi->page_offset = rnpm_rx_offset(rx_ring);
	page_ref_add(page, USHRT_MAX - 1);
	bi->pagecnt_bias = USHRT_MAX;
	rx_ring->rx_stats.alloc_rx_page++;
	/* sync the buffer for use by the device */
	dma_sync_single_range_for_device(rx_ring->dev, bi->dma, 0, bufsz,
					 DMA_FROM_DEVICE);
	rx_desc->pkt_addr = cpu_to_le64(bi->dma + fun_id);
	return true;
}

#else
static bool rnpm_alloc_mapped_page(struct rnpm_ring *rx_ring,
				   struct rnpm_rx_buffer *bi)
{
	struct page *page = bi->page;
	dma_addr_t dma;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page))
		return true;

	/* alloc new page for storage */
	page = dev_alloc_pages(rnpm_rx_pg_order(rx_ring));
	if (unlikely(!page)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	/* map page for use */
	dma = dma_map_page_attrs(rx_ring->dev, page, 0,
				 rnpm_rx_pg_size(rx_ring), DMA_FROM_DEVICE,
				 RNPM_RX_DMA_ATTR);
	/*
	 * if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		__free_pages(page, rnpm_rx_pg_order(rx_ring));

		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	bi->dma = dma;
	bi->page = page;
	bi->page_offset = rnpm_rx_offset(rx_ring);
	page_ref_add(page, USHRT_MAX - 1);
	bi->pagecnt_bias = USHRT_MAX;
	rx_ring->rx_stats.alloc_rx_page++;
	return true;
}
#endif /* RNPM_OPTM_WITH_LPAGE */
/**
 * rnpm_pull_tail - rnpm specific version of skb_pull_tail
 * @skb: pointer to current skb being adjusted
 *
 * This function is an rnpm specific version of __pskb_pull_tail.  The
 * main difference between this version and the original function is that
 * this function can make several assumptions about the state of things
 * that allow for significant optimizations versus the standard function.
 * As a result we can do things like drop a frag and maintain an accurate
 * truesize for the skb.
 */
static void rnpm_pull_tail(struct sk_buff *skb)
{
	skb_frag_t *frag = &skb_shinfo(skb)->frags[0];
	unsigned char *va;
	unsigned int pull_len;

	va = skb_frag_address(frag);
	/*
	 * we need the header to contain the greater of either ETH_HLEN or
	 * 60 bytes if the skb->len is less than 60 for skb_pad.
	 */
	pull_len = rnpm_get_headlen(va, RNPM_RX_HDR_SIZE);
	/* align pull length to size of long to optimize memcpy performance */
	skb_copy_to_linear_data(skb, va, ALIGN(pull_len, sizeof(long)));
	/* update all of the pointers */
	skb_frag_size_sub(frag, pull_len);
	skb_frag_off_add(frag, pull_len);
	skb->data_len -= pull_len;
	skb->tail += pull_len;
}

/**
 * rnpm_dma_sync_frag - perform DMA sync for first frag of SKB
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @skb: pointer to current skb being updated
 *
 * This function provides a basic DMA sync up for the first fragment of an
 * skb.  The reason for doing this is that the first fragment cannot be
 * unmapped until we have reached the end of packet descriptor for a buffer
 * chain.
 */
__maybe_unused static void rnpm_dma_sync_frag(struct rnpm_ring *rx_ring,
					      struct sk_buff *skb)
{
	/* if the page was released unmap it, else just sync our portion */
	if (unlikely(RNPM_CB(skb)->page_released)) {
		dma_unmap_page_attrs(rx_ring->dev, RNPM_CB(skb)->dma,
				     rnpm_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE, RNPM_RX_DMA_ATTR);
	} else if (ring_uses_build_skb(rx_ring)) {
		unsigned long offset = (unsigned long)(skb->data) &
				       ~PAGE_MASK;

		dma_sync_single_range_for_cpu(rx_ring->dev,
					      RNPM_CB(skb)->dma, offset,
					      skb_headlen(skb),
					      DMA_FROM_DEVICE);
	} else {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[0];

		dma_sync_single_range_for_cpu(rx_ring->dev,
					      RNPM_CB(skb)->dma,
					      skb_frag_off(frag),
					      skb_frag_size(frag),
					      DMA_FROM_DEVICE);
	}
}

/**
 * rnpm_cleanup_headers - Correct corrupted or empty headers
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
static bool rnpm_cleanup_headers(struct rnpm_ring __maybe_unused *rx_ring,
				 union rnpm_rx_desc *rx_desc,
				 struct sk_buff *skb)
{
#ifndef RNPM_OPTM_WITH_LPAGE
	if (IS_ERR(skb))
		return true;
#endif

	if (!skb_headlen(skb))
		rnpm_pull_tail(skb);

	if (eth_skb_pad(skb))
		return true;
	return false;
}

/**
 * rnpm_reuse_rx_page - page flip buffer and store it back on the ring
 * @rx_ring: rx descriptor ring to store buffers on
 * @old_buff: donor buffer to have page reused
 *
 * Synchronizes page for reuse by the adapter
 **/
static void rnpm_reuse_rx_page(struct rnpm_ring *rx_ring,
			       struct rnpm_rx_buffer *old_buff)
{
	struct rnpm_rx_buffer *new_buff;
	u16 nta = rx_ring->next_to_alloc;

	new_buff = &rx_ring->rx_buffer_info[nta];
	nta++;
	rx_ring->next_to_alloc = (nta < rx_ring->count) ? nta : 0;

	/* Transfer page from old buffer to new buffer.
	 * Move each member individually to avoid possible store
	 * forwarding stalls and unnecessary copy of skb.
	 */
	new_buff->dma = old_buff->dma;
	new_buff->page = old_buff->page;
	new_buff->page_offset = old_buff->page_offset;
	new_buff->pagecnt_bias = old_buff->pagecnt_bias;
}

static inline bool rnpm_page_is_reserved(struct page *page)
{
	return (page_to_nid(page) != numa_mem_id()) ||
	       page_is_pfmemalloc(page);
}

static bool rnpm_can_reuse_rx_page(struct rnpm_rx_buffer *rx_buffer)
{
	unsigned int pagecnt_bias = rx_buffer->pagecnt_bias;
	struct page *page = rx_buffer->page;

#ifdef RNPM_OPTM_WITH_LPAGE
	return false;
#endif
	if (unlikely(rnpm_page_is_reserved(page)))
		return false;
#if (PAGE_SIZE < 8192)
	if (unlikely((page_ref_count(page) - pagecnt_bias) > 1))
		return false;
#else
	/* The last offset is a bit aggressive in that we assume the
	 * worst case of FCoE being enabled and using a 3K buffer.
	 * However this should have minimal impact as the 1K extra is
	 * still less than one buffer in size.
	 */
#ifdef RNPM_DMA_SLICE
#if (PAGE_SIZE >= 65536)
#define RNPM_LAST_OFFSET (SKB_WITH_OVERHEAD(PAGE_SIZE) - 9100)
#else
#define RNPM_LAST_OFFSET (SKB_WITH_OVERHEAD(PAGE_SIZE) - RNPM_RXBUFFER_2K)
#endif
#else
#define RNPM_LAST_OFFSET (SKB_WITH_OVERHEAD(PAGE_SIZE) - RNPM_RXBUFFER_2K)
#endif
	if (rx_buffer->page_offset > RNPM_LAST_OFFSET)
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
 * rnpm_add_rx_frag - Add contents of Rx buffer to sk_buff
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
static void rnpm_add_rx_frag(struct rnpm_ring *rx_ring,
			     struct rnpm_rx_buffer *rx_buffer,
			     struct sk_buff *skb, unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnpm_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize =
		ring_uses_build_skb(rx_ring) ?
			SKB_DATA_ALIGN(RNPM_SKB_PAD + size) :
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

#ifdef RNPM_OPTM_WITH_LPAGE
static struct rnpm_rx_buffer *
rnpm_get_rx_buffer(struct rnpm_ring *rx_ring, union rnpm_rx_desc *rx_desc,
		   const unsigned int size)
{
	struct rnpm_rx_buffer *rx_buffer;

	rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
	prefetchw(rx_buffer->page);

	rx_buf_dump("rx buf",
		    page_address(rx_buffer->page) + rx_buffer->page_offset,
		    rx_desc->wb.len);
	/* we are reusing so sync this buffer for CPU use */
	dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma, 0,
				      size, DMA_FROM_DEVICE);
	/* skip_sync: */
	rx_buffer->pagecnt_bias--;
	return rx_buffer;
}
#else
static struct rnpm_rx_buffer *
rnpm_get_rx_buffer(struct rnpm_ring *rx_ring, union rnpm_rx_desc *rx_desc,
		   struct sk_buff **skb, const unsigned int size)
{
	struct rnpm_rx_buffer *rx_buffer;

	rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
	prefetchw(rx_buffer->page);
	*skb = rx_buffer->skb;

	rx_buf_dump("rx buf",
		    page_address(rx_buffer->page) + rx_buffer->page_offset,
		    rx_desc->wb.len);
	/* reusing so sync this buffer for CPU use */
	dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma,
				      rx_buffer->page_offset, size,
				      DMA_FROM_DEVICE);
	rx_buffer->pagecnt_bias--;

	return rx_buffer;
}
#endif

#ifdef RNPM_OPTM_WITH_LPAGE
static void rnpm_put_rx_buffer(struct rnpm_ring *rx_ring,
			       struct rnpm_rx_buffer *rx_buffer)
{
	if (rnpm_can_reuse_rx_page(rx_buffer)) {
		/* hand second half of page back to the ring */
		rnpm_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		/* we are not reusing the buffer so unmap it */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnpm_rx_bufsz(rx_ring),
				     DMA_FROM_DEVICE, RNPM_RX_DMA_ATTR);
		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
	}

	/* clear contents of rx_buffer */
	rx_buffer->page = NULL;
}

#else
static void rnpm_put_rx_buffer(struct rnpm_ring *rx_ring,
			       struct rnpm_rx_buffer *rx_buffer,
			       struct sk_buff *skb)
{
	if (!rx_buffer || !rx_buffer->page || !rx_ring) {
		netdev_err(rx_ring->netdev, "rnpm rx buffer is null!\n");
		WARN_ON(1);
		return;
	}

	if (rnpm_can_reuse_rx_page(rx_buffer)) {
		rnpm_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnpm_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE, RNPM_RX_DMA_ATTR);
		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
	}

	rx_buffer->page = NULL;
	rx_buffer->skb = NULL;
}
#endif
#ifdef RNPM_OPTM_WITH_LPAGE
static struct sk_buff *rnpm_construct_skb(struct rnpm_ring *rx_ring,
					  struct rnpm_rx_buffer *rx_buffer,
					  union rnpm_rx_desc *rx_desc,
					  unsigned int size)
{
	void *va = page_address(rx_buffer->page) + rx_buffer->page_offset;
	unsigned int truesize = SKB_DATA_ALIGN(size);
	unsigned int headlen;
	struct sk_buff *skb;

	prefetch(va);
#if L1_CACHE_BYTES < 128
	prefetch(va + L1_CACHE_BYTES);
#endif
	skb = napi_alloc_skb(&rx_ring->q_vector->napi, RNPM_RX_HDR_SIZE);
	if (unlikely(!skb))
		return NULL;
	prefetchw(skb->data);
	/* Determine available headroom for copy */
	headlen = size;
	if (headlen > RNPM_RX_HDR_SIZE)
		headlen = rnpm_get_headlen(va, RNPM_RX_HDR_SIZE);
	memcpy(__skb_put(skb, headlen), va, ALIGN(headlen, sizeof(long)));
	size -= headlen;
	if (size) {
		skb_add_rx_frag(skb, 0, rx_buffer->page,
				(va + headlen) -
					page_address(rx_buffer->page),
				size, truesize);
		rx_buffer->page_offset += truesize;
	} else {
		rx_buffer->pagecnt_bias++;
	}
	return skb;
}

static struct sk_buff *rnpm_build_skb(struct rnpm_ring *rx_ring,
				      struct rnpm_rx_buffer *rx_buffer,
				      union rnpm_rx_desc *rx_desc,
				      unsigned int size)
{
	void *va = page_address(rx_buffer->page) + rx_buffer->page_offset;
	unsigned int truesize =
		SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
		SKB_DATA_ALIGN(size + RNPM_SKB_PAD);
	struct sk_buff *skb;

	/* prefetch first cache line of first page */
	prefetch(va);
#if L1_CACHE_BYTES < 128
	prefetch(va + L1_CACHE_BYTES);
#endif

	/* build an skb around the page buffer */
	skb = build_skb(va - RNPM_SKB_PAD, truesize);
	if (unlikely(!skb))
		return NULL;

	/* update pointers within the skb to store the data */
	skb_reserve(skb, RNPM_SKB_PAD);
	__skb_put(skb, size);
	return skb;
}
#else
static struct sk_buff *rnpm_construct_skb(struct rnpm_ring *rx_ring,
					  struct rnpm_rx_buffer *rx_buffer,
					  struct xdp_buff *xdp,
					  union rnpm_rx_desc *rx_desc)
{
	unsigned int size = xdp->data_end - xdp->data;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnpm_rx_pg_size(rx_ring) / 2;
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
	/* Note, we get here by enabling legacy-rx via:
	 *
	 *    ethtool --set-priv-flags <dev> legacy-rx on
	 *
	 * In this mode, we currently get 0 extra XDP headroom as
	 * opposed to having legacy-rx off, where we process XDP
	 * packets going to stack via rnpm_build_skb(). The latter
	 * provides us currently with 192 bytes of headroom.
	 *
	 * For rnpm_construct_skb() mode it means that the
	 * xdp->data_meta will always point to xdp->data, since
	 * the helper cannot expand the head. Should this ever
	 * change in future for legacy-rx mode on, then lets also
	 * add xdp->data_meta handling here.
	 */

	/* allocate a skb to store the frags */
	skb = napi_alloc_skb(&rx_ring->q_vector->napi, RNPM_RX_HDR_SIZE);
	if (unlikely(!skb))
		return NULL;

	prefetchw(skb->data);

	if (size > RNPM_RX_HDR_SIZE) {
		skb_add_rx_frag(skb, 0, rx_buffer->page,
				xdp->data - page_address(rx_buffer->page),
				size, truesize);
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

static struct sk_buff *rnpm_build_skb(struct rnpm_ring *rx_ring,
				      struct rnpm_rx_buffer *rx_buffer,
				      struct xdp_buff *xdp,
				      union rnpm_rx_desc *rx_desc)
{
	unsigned int metasize = xdp->data - xdp->data_meta;
	void *va = xdp->data_meta;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnpm_rx_pg_size(rx_ring) / 2;
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

#define RNPM_XDP_PASS 0
#define RNPM_XDP_CONSUMED 1
#define RNPM_XDP_TX 2

#ifndef RNPM_OPTM_WITH_LPAGE
static void rnpm_rx_buffer_flip(struct rnpm_ring *rx_ring,
				struct rnpm_rx_buffer *rx_buffer,
				unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = rnpm_rx_pg_size(rx_ring) / 2;

	rx_buffer->page_offset ^= truesize;
#else
	unsigned int truesize =
		ring_uses_build_skb(rx_ring) ?
			SKB_DATA_ALIGN(RNPM_SKB_PAD + size) :
			SKB_DATA_ALIGN(size);

	rx_buffer->page_offset += truesize;
#endif
}
#endif

#ifdef RNPM_OPTM_WITH_LPAGE
static int rnpm_clean_rx_irq(struct rnpm_q_vector *q_vector,
			     struct rnpm_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	unsigned int driver_drop_packets = 0;
	struct sk_buff *skb = rx_ring->skb;
	struct rnpm_adapter *adapter = q_vector->adapter;
	u16 cleaned_count = rnpm_desc_unused_rx(rx_ring);
	bool fail_alloc = false;

	while (likely(total_rx_packets < budget)) {
		union rnpm_rx_desc *rx_desc;
		struct rnpm_rx_buffer *rx_buffer;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= RNPM_RX_BUFFER_WRITE) {
			fail_alloc = rnpm_alloc_rx_buffers(rx_ring,
							   cleaned_count);
			cleaned_count = 0;
			if (fail_alloc)
				return budget;
		}
		rx_desc = RNPM_RX_DESC(rx_ring, rx_ring->next_to_clean);

		rx_buf_dump("rx-desc:", rx_desc, sizeof(*rx_desc));
		rx_debug_printk(rx_ring, "dd set: %s\n",
				(rx_desc->wb.cmd & RNPM_RXD_STAT_DD) ?
					"Yes" :
					"No");

		if (!rnpm_test_staterr(rx_desc, RNPM_RXD_STAT_DD))
			break;

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();

		rx_debug_printk(
			rx_ring,
			"queue:%d  rx-desc:%d has-data len:%d next_to_clean %d\n",
			rx_ring->rnpm_queue_idx, rx_ring->next_to_clean,
			rx_desc->wb.len, rx_ring->next_to_clean);

		/* handle padding */
		if ((adapter->priv_flags &
		     RNPM_PRIV_FLAG_PCIE_CACHE_ALIGN_PATCH) &&
		    (!(adapter->priv_flags &
		       RNPM_PRIV_FLAG_PADDING_DEBUG))) {
			if (likely(rnpm_test_staterr(rx_desc,
						     RNPM_RXD_STAT_EOP)))
				size = le16_to_cpu(rx_desc->wb.len) -
				       le16_to_cpu(
					       rx_desc->wb.padding_len);
			else
				size = le16_to_cpu(rx_desc->wb.len);
		} else {
			/* size should not zero */
			size = le16_to_cpu(rx_desc->wb.len);
		}

		if (!size)
			break;

		if (rnpm_check_csum_error(rx_ring, rx_desc, size,
					  &driver_drop_packets)) {
			cleaned_count++;
			continue;
		}

		rx_buffer = rnpm_get_rx_buffer(rx_ring, rx_desc, size);

		if (skb)
			rnpm_add_rx_frag(rx_ring, rx_buffer, skb, size);
		else if (ring_uses_build_skb(rx_ring))
			skb = rnpm_build_skb(rx_ring, rx_buffer, rx_desc,
					     size);
		else
			skb = rnpm_construct_skb(rx_ring, rx_buffer,
						 rx_desc, size);

		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			rx_buffer->pagecnt_bias++;
			break;
		}

#ifndef NO_PTP
		if (module_enable_ptp && adapter->ptp_rx_en &&
		    adapter->flags2 & RNPM_FLAG2_PTP_ENABLED)
			rnpm_ptp_get_rx_hwstamp(adapter, rx_desc, skb);
#endif
		rnpm_put_rx_buffer(rx_ring, rx_buffer);
		cleaned_count++;

		/* place incomplete frames back on ring for completion */
		if (rnpm_is_non_eop(rx_ring, rx_desc))
			continue;

		/* verify the packet layout is correct */
		if (rnpm_cleanup_headers(rx_ring, rx_desc, skb)) {
			skb = NULL;
			continue;
		}
		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, timestamp, VLAN, and protocol */
		rnpm_process_skb_fields(rx_ring, rx_desc, skb);
		/* we should clean it since we used all info in it */
		rx_desc->wb.cmd = 0;
		rnpm_rx_skb(q_vector, skb);
		skb = NULL;
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
	if (total_rx_packets)
		q_vector->rx.poll_times++;

	if (total_rx_packets >= budget)
		rx_ring->rx_stats.poll_again_count++;
	return total_rx_packets;
}
#else /* RNPM_OPTM_WITH_LPAGE */
/**
 * rnpm_clean_rx_irq - Clean completed descriptors from Rx ring - bounce buf
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

static int rnpm_clean_rx_irq(struct rnpm_q_vector *q_vector,
			     struct rnpm_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	unsigned int driver_drop_packets = 0;
	struct rnpm_adapter *adapter = q_vector->adapter;
	u16 cleaned_count = rnpm_desc_unused_rx(rx_ring);
	bool xdp_xmit = false;
	struct xdp_buff xdp;
	bool fail_alloc = false;

	xdp.data = NULL;
	xdp.data_end = NULL;

	while (likely(total_rx_packets < budget)) {
		union rnpm_rx_desc *rx_desc;
		struct rnpm_rx_buffer *rx_buffer;
		struct sk_buff *skb;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= RNPM_RX_BUFFER_WRITE) {
			fail_alloc = rnpm_alloc_rx_buffers(rx_ring,
							   cleaned_count);
			cleaned_count = 0;
			if (fail_alloc)
				return budget;
		}
		rx_desc = RNPM_RX_DESC(rx_ring, rx_ring->next_to_clean);
		rx_buf_dump("rx-desc:", rx_desc, sizeof(*rx_desc));
		rx_debug_printk(rx_ring, "dd set: %s\n",
				(rx_desc->wb.cmd & RNPM_RXD_STAT_DD) ?
					"Yes" :
					"No");

		if (!rnpm_test_staterr(rx_desc, RNPM_RXD_STAT_DD))
			break;

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();

		rx_debug_printk(
			rx_ring,
			"queue:%d  rx-desc:%d has-data len:%d next_to_clean %d\n",
			rx_ring->rnpm_queue_idx, rx_ring->next_to_clean,
			rx_desc->wb.len, rx_ring->next_to_clean);

		/* handle padding */
		if ((adapter->priv_flags &
		     RNPM_PRIV_FLAG_PCIE_CACHE_ALIGN_PATCH) &&
		    (!(adapter->priv_flags &
		       RNPM_PRIV_FLAG_PADDING_DEBUG))) {
			if (likely(rnpm_test_staterr(rx_desc,
						     RNPM_RXD_STAT_EOP)))
				size = le16_to_cpu(rx_desc->wb.len) -
				       le16_to_cpu(
					       rx_desc->wb.padding_len);
			else
				size = le16_to_cpu(rx_desc->wb.len);
		} else {
			/* size should not zero */
			size = le16_to_cpu(rx_desc->wb.len);
		}

		if (!size)
			break;

		if (rnpm_check_csum_error(rx_ring, rx_desc, size,
					  &driver_drop_packets)) {
			cleaned_count++;
			continue;
		}

		rx_buffer =
			rnpm_get_rx_buffer(rx_ring, rx_desc, &skb, size);

		if (!skb) {
			xdp.data = page_address(rx_buffer->page) +
				   rx_buffer->page_offset;
			xdp.data_meta = xdp.data;
			xdp.data_hard_start =
				xdp.data - rnpm_rx_offset(rx_ring);
			xdp.data_end = xdp.data + size;
		}

		if (IS_ERR(skb)) {
			if (PTR_ERR(skb) == -RNPM_XDP_TX) {
				xdp_xmit = true;
				rnpm_rx_buffer_flip(rx_ring, rx_buffer,
						    size);
			} else {
				rx_buffer->pagecnt_bias++;
			}
			total_rx_packets++;
			total_rx_bytes += size;
		} else if (skb) {
			rnpm_add_rx_frag(rx_ring, rx_buffer, skb, size);
		} else if (ring_uses_build_skb(rx_ring)) {
			skb = rnpm_build_skb(rx_ring, rx_buffer, &xdp,
					     rx_desc);
		} else {
			skb = rnpm_construct_skb(rx_ring, rx_buffer, &xdp,
						 rx_desc);
		}

		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			rx_buffer->pagecnt_bias++;
			break;
		}

#ifndef NO_PTP
		if (module_enable_ptp && adapter->ptp_rx_en &&
		    adapter->flags2 & RNPM_FLAG2_PTP_ENABLED)
			rnpm_ptp_get_rx_hwstamp(adapter, rx_desc, skb);
#endif
		rnpm_put_rx_buffer(rx_ring, rx_buffer, skb);
		cleaned_count++;
		/* place incomplete frames back on ring for completion */
		if (rnpm_is_non_eop(rx_ring, rx_desc, skb))
			continue;

		/* verify the packet layout is correct */
		if (rnpm_cleanup_headers(rx_ring, rx_desc, skb))
			continue;
		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;
		total_rx_packets++;

		/* populate checksum, timestamp, VLAN, and protocol */
		rnpm_process_skb_fields(rx_ring, rx_desc, skb);
		/* we should clean it since we used all info in it */
		rx_desc->wb.cmd = 0;
		rnpm_rx_skb(q_vector, skb);

		/* update budget accounting */
	}

	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	rx_ring->rx_stats.driver_drop_packets += driver_drop_packets;
	u64_stats_update_end(&rx_ring->syncp);
	q_vector->rx.total_packets += total_rx_packets;
	q_vector->rx.total_bytes += total_rx_bytes;
	if (total_rx_packets)
		q_vector->rx.poll_times++;

	if (total_rx_packets >= budget)
		rx_ring->rx_stats.poll_again_count++;
	return total_rx_packets;
}
#endif /* RNPM_OPTM_WITH_LPAGE */
#else /* CONFIG_RNPM_DISABLE_PACKET_SPLIT */

static bool rnpm_alloc_mapped_skb(struct rnpm_ring *rx_ring,
				  struct rnpm_rx_buffer *bi)
{
	struct sk_buff *skb = bi->skb;
	dma_addr_t dma = bi->dma;

	if (unlikely(dma))
		return true;

	if (likely(!skb)) {
		skb = netdev_alloc_skb_ip_align(rx_ring->netdev,
						rx_ring->rx_buf_len);
		if (unlikely(!skb)) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			return false;
		}
		bi->skb = skb;
	}
	dma = dma_map_single(rx_ring->dev, skb->data, rx_ring->rx_buf_len,
			     DMA_FROM_DEVICE);
	/*
	 * if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		dev_kfree_skb_any(skb);
		bi->skb = NULL;

		rx_ring->rx_stats.alloc_rx_buff_failed++;
		return false;
	}

	bi->dma = dma;
	return true;
}

/**
 * rnpm_clean_rx_irq - Clean completed descriptors from Rx ring - legacy
 * @q_vector: structure containing interrupt and ring information
 * @rx_ring: rx descriptor ring to transact packets on
 * @budget: Total limit on number of packets to process
 *
 * This function provides a legacy approach to Rx interrupt
 * handling.  This version will perform better on systems with a low cost
 * dma mapping API.
 *
 * Returns amount of work completed.
 **/
static int rnpm_clean_rx_irq(struct rnpm_q_vector *q_vector,
			     struct rnpm_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	struct rnpm_adapter *adapter = q_vector->adapter;
	unsigned int driver_drop_packets = 0;
	bool fail_alloc = false;
	u16 len = 0;
	u16 cleaned_count = rnpm_desc_unused_rx(rx_ring);

	while (likely(total_rx_packets < budget)) {
		struct rnpm_rx_buffer *rx_buffer;
		union rnpm_rx_desc *rx_desc;
		struct sk_buff *skb;
		u16 ntc;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= RNPM_RX_BUFFER_WRITE) {
			fail_alloc = rnpm_alloc_rx_buffers(rx_ring,
							   cleaned_count);
			cleaned_count = 0;
			if (fail_alloc)
				return budget;
		}

		ntc = rx_ring->next_to_clean;
		rx_desc = RNPM_RX_DESC(rx_ring, ntc);
		rx_buffer = &rx_ring->rx_buffer_info[ntc];

		if (!rnpm_test_staterr(rx_desc, RNPM_RXD_STAT_DD))
			break;
		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();
		skb = rx_buffer->skb;
		prefetch(skb->data);
		/* handle padding */
		if ((adapter->priv_flags &
		     RNPM_PRIV_FLAG_PCIE_CACHE_ALIGN_PATCH) &&
		    (!(adapter->priv_flags &
		       RNPM_PRIV_FLAG_PADDING_DEBUG))) {
			if (likely(rnpm_test_staterr(rx_desc,
						     RNPM_RXD_STAT_EOP)))
				len = le16_to_cpu(rx_desc->wb.len) -
				      le16_to_cpu(rx_desc->wb.padding_len);
			else
				len = le16_to_cpu(rx_desc->wb.len);
		} else {
			/* size should not zero */
			len = le16_to_cpu(rx_desc->wb.len);
		}

		if (!len)
			break;

		if (rnpm_check_csum_error(rx_ring, rx_desc, len,
					  &driver_drop_packets)) {
			dev_kfree_skb_any(skb);
			cleaned_count++;
			continue;
		}
		__skb_put(skb, len);
		dma_unmap_single(rx_ring->dev, rx_buffer->dma,
				 rx_ring->rx_buf_len, DMA_FROM_DEVICE);
		rx_buffer->skb = NULL;
		rx_buffer->dma = 0;
		cleaned_count++;
		if (rnpm_is_non_eop(rx_ring, rx_desc, skb))
			continue;
		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, timestamp, VLAN, and protocol */
		rnpm_process_skb_fields(rx_ring, rx_desc, skb);
		/* we should clean it since we used all info in it */
		rx_desc->wb.cmd = 0;
		rnpm_rx_skb(q_vector, skb);

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
	if (total_rx_packets)
		q_vector->rx.poll_times++;

	if (total_rx_packets >= budget)
		rx_ring->rx_stats.poll_again_count++;
	return total_rx_packets;
}

#endif /* CONFIG_RNPM_DISABLE_PACKET_SPLIT */

/**
 * rnpm_configure_msix - Configure MSI-X hardware
 * @adapter: board private structure
 *
 * rnpm_configure_msix sets up the hardware to properly generate MSI-X
 * interrupts.
 **/
static void rnpm_configure_msix(struct rnpm_adapter *adapter)
{
	struct rnpm_q_vector *q_vector;
	int i;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnpm_ring *ring;

		q_vector = adapter->q_vector[i];
		rnpm_for_each_ring(ring, q_vector->rx)
			rnpm_set_ring_vector(adapter, ring->rnpm_queue_idx,
					     q_vector->v_idx);
	}
}

static inline bool rnpm_container_is_rx(struct rnpm_q_vector *q_vector,
					struct rnpm_ring_container *rc)
{
	return &q_vector->rx == rc;
}

/**
 * ixgbe_write_eitr - write EITR register in hardware specific way
 * @q_vector: structure containing interrupt and ring information
 *
 * This function is made to be called by ethtool and by the driver
 * when it needs to update EITR registers at runtime.  Hardware
 * specific quirks/differences are taken care of here.
 */
static void rnpm_write_eitr(struct rnpm_q_vector *q_vector,
			    bool is_rxframe)
{
	struct rnpm_adapter *adapter = q_vector->adapter;
	struct rnpm_hw *hw = &adapter->hw;
	struct rnpm_ring *ring;
	u32 itr_reg = q_vector->adapter->rx_usecs * hw->usecstocount;

	if (is_rxframe) {
		rnpm_for_each_ring(ring, q_vector->rx) {
			wr32(hw,
			     RNPM_DMA_REG_RX_INT_DELAY_PKTCNT(
				     ring->rnpm_queue_idx),
			     q_vector->itr);
		}
	} else {
		rnpm_for_each_ring(ring, q_vector->rx) {
			wr32(hw,
			     RNPM_DMA_REG_RX_INT_DELAY_TIMER(
				     ring->rnpm_queue_idx),
			     itr_reg);
		}
	}
}

static int rnpm_update_itr_by_packets(int speed, int poll_packets, int itr)
{
	unsigned int t;

	if (speed >= SPEED_10000) {
		/* 10G */
		if (((poll_packets - itr) == 1)) {
			/* Hold this itr */
		} else {
			if (poll_packets == itr) {
			} else if (poll_packets > itr) {
				t = DIV_ROUND_UP(poll_packets - itr, 2);
				if (t > 2)
					t = 2;
				itr += t ? t : 1;
			} else {
				itr >>= 1;
			}
		}
		if (itr < 3)
			itr = 3;
	} else if (speed >= SPEED_1000) {
		/* 1G */
		if (((poll_packets - itr) == 1) ||
		    ((poll_packets - itr) == 2)) {
			/* Hold this itr */
		} else {
			if (poll_packets >= itr) {
				t = DIV_ROUND_UP(poll_packets - itr, 2);
				if (t > 2)
					t = 2;
				itr += t ? t : 1;
			} else {
				if (itr >= (poll_packets + 2)) {
					t = DIV_ROUND_UP(
						itr - poll_packets, 2);
					itr -= t ? 2 : 1;
				} else
					itr--;
			}
		}
		if (itr < 3)
			itr = 3;
	} else {
		/* 100M/10M */
		if (((poll_packets - itr) == 1) && (itr != 1)) {
			/* Hold this itr */
		} else {
			if (poll_packets >= itr) {
				t = DIV_ROUND_UP(poll_packets - itr, 2);
				if (t > 2)
					t = 2;
				itr += t ? t : 1;
			} else {
				itr--;
			}
		}
		if (itr < 3)
			itr = 3;
	}

	return itr;
}

static bool rnpm_update_rxf(struct rnpm_q_vector *q_vector,
			    struct rnpm_ring_container *ring_container)
{
	int itr = 1;
	unsigned int avg_wire_size, packets, bytes, t;
	int poll_packets = 0;
	unsigned long next_update = jiffies;
	int factor, off_1, off_2, speed;
	bool ret = true;

	/* If we don't have any rings just leave ourselves set for maximum
	 * possible latency so we take ourselves out of the equation.
	 */

	if (!ring_container->ring)
		return false;

	factor = q_vector->factor;
	packets = ring_container->total_packets / factor;
	bytes = ring_container->total_bytes / factor;

	/* Rx packets is zero, no need modify itr */
	if (!packets)
		return false;

	switch (q_vector->adapter->link_speed) {
	case RNPM_LINK_SPEED_10GB_FULL:
		off_1 = 24;
		off_2 = 10;
		speed = SPEED_10000;
		break;
	// case RNPM_LINK_SPEED_2_5GB_FULL:
	case RNPM_LINK_SPEED_1GB_FULL:
		off_1 = 0;
		off_2 = 0;
		speed = SPEED_1000;
		break;
	case RNPM_LINK_SPEED_100_FULL:
	case RNPM_LINK_SPEED_10_FULL:
		off_1 = 0;
		off_2 = -12;
		speed = SPEED_100;
		break;
	default:
		off_1 = 24;
		off_2 = 10;
		speed = SPEED_10000;
		break;
	}

	/* If we didn't update within up to 1 - 2 jiffies we can assume
	 * that either packets are coming in so slow there hasn't been
	 * any work, or that there is so much work that NAPI is dealing
	 * with interrupt moderation and we don't need to do anything.
	 */
	if (time_after_eq(next_update, ring_container->next_update)) {
		avg_wire_size = bytes / packets;
		if (rnpm_container_is_rx(q_vector, ring_container) &&
		    (speed > SPEED_100)) {
			/* If Rx and there are 1 to 23 packets and bytes are less than
			 * 12112 assume insufficient data to use bulk rate limiting
			 * approach. Instead we will focus on simply trying to target
			 * receiving 8 times as much data in the next interrupt. Assume
			 * max packert is 1514 bytes(1514*8 = 12112), min len is 66 bytes
			 */
			if (packets && packets < (24 + off_1) &&
			    bytes < 12112 * DIV_ROUND_UP(factor, 2)) {
				if ((packets <= 3) &&
				    (avg_wire_size <= 1120) &&
				    (avg_wire_size >= 768))
					itr = 2;
				else
					itr = 1;
				goto clear_counts;
			}
		} else {
			if (packets && packets <= 3 && bytes < 6056) {
				itr = 1;
				goto clear_counts;
			}
		}

		itr = q_vector->itr;

		if (ring_container->poll_times && factor) {
			t = (ring_container->poll_times > factor) ?
				    ring_container->poll_times / factor :
				    1;
			poll_packets = DIV_ROUND_UP(packets, t);
		} else {
			goto clear_counts;
		}

		if (poll_packets <= (32 + off_2)) {
			if ((poll_packets <= 3) &&
			    (avg_wire_size <= 1120) &&
			    (speed > SPEED_100)) {
				/* 1K - 2K bytes*/
				itr = 2;
			} else {
				itr = rnpm_update_itr_by_packets(
					speed, poll_packets, itr);
			}
		} else {
			/* Mabey too large */
			itr = q_vector->itr << 1;
			if (itr > 64)
				itr = 64;
		}
		ret = true;
	} else {
		ret = false;
		goto out;
	}

clear_counts:
	/* write back value */
	ring_container->itr = itr;
	/* next update should occur within next jiffy */
	ring_container->next_update = next_update + 1;
	ring_container->total_bytes = 0;
	ring_container->total_packets = 0;
	ring_container->poll_times = 0;
	ring_container->ring->rx_stats.rx_poll_packets = packets;
	ring_container->ring->rx_stats.rx_poll_avg_packets = poll_packets;
	ring_container->ring->rx_stats.rx_poll_itr = itr;

out:
	return ret;
}

static void rnpm_set_itr(struct rnpm_q_vector *q_vector)
{
	u32 new_itr;

	if (rnpm_update_rxf(q_vector, &q_vector->rx)) {
		new_itr = q_vector->rx.itr;

		if (new_itr != q_vector->itr) {
			/* save the algorithm value here */
			q_vector->itr = new_itr;
			rnpm_write_eitr(q_vector, 1);
		}
	}
}

enum latency_range {
	lowest_latency = 0,
	low_latency = 1,
	bulk_latency = 2,
	latency_invalid = 255
};

static inline void rnpm_irq_enable_queues(struct rnpm_adapter *adapter,
					  struct rnpm_q_vector *q_vector)
{
	struct rnpm_ring *ring;

	rnpm_for_each_ring(ring, q_vector->rx) {
#ifdef CONFIG_RNPM_DISABLE_TX_IRQ
		rnpm_wr_reg(ring->dma_int_mask, ~(RX_INT_MASK));
#else
		rnpm_wr_reg(ring->dma_int_mask,
			    ~(RX_INT_MASK | TX_INT_MASK));
#endif
	}
}

static inline void rnpm_irq_disable_queues(struct rnpm_q_vector *q_vector)
{
	struct rnpm_ring *ring;

	rnpm_for_each_ring(ring, q_vector->tx) {
		rnpm_wr_reg(ring->dma_int_mask,
			    (RX_INT_MASK | TX_INT_MASK));
	}
}
/**
 * rnpm_irq_enable - Enable default interrupt generation settings
 * @adapter: board private structure
 **/
static inline void rnpm_irq_enable(struct rnpm_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_q_vectors; i++)
		rnpm_irq_enable_queues(adapter, adapter->q_vector[i]);
}

static irqreturn_t rnpm_msix_other(int irq, void *data)
{
	struct rnpm_pf_adapter *pf_adapter = data;

	rnpm_msg_task(pf_adapter);
	return IRQ_HANDLED;
}

static void rnpm_htimer_start(struct rnpm_q_vector *q_vector)
{
	unsigned long ns = q_vector->irq_check_usecs * NSEC_PER_USEC / 2;

	hrtimer_start_range_ns(&q_vector->irq_miss_check_timer,
			       ns_to_ktime(ns), ns,
			       HRTIMER_MODE_REL_PINNED);
}

static void rnpm_htimer_stop(struct rnpm_q_vector *q_vector)
{
	hrtimer_cancel(&q_vector->irq_miss_check_timer);
}

static irqreturn_t rnpm_msix_clean_rings(int irq, void *data)
{
	struct rnpm_q_vector *q_vector = data;

	rnpm_htimer_stop(q_vector);
	rnpm_irq_disable_queues(q_vector);
	if (q_vector->rx.ring || q_vector->tx.ring)
		napi_schedule_irqoff(&q_vector->napi);
	return IRQ_HANDLED;
}

/**
 * rnpm_poll - NAPI Rx polling callback
 * @napi: structure for representing this polling device
 * @budget: how many packets driver is allowed to clean
 *
 * This function is used for legacy and MSI, NAPI mode
 **/
int rnpm_poll(struct napi_struct *napi, int budget)
{
	struct rnpm_q_vector *q_vector =
		container_of(napi, struct rnpm_q_vector, napi);
	struct rnpm_adapter *adapter = q_vector->adapter;
	struct rnpm_hw *hw = &adapter->hw;
	struct rnpm_ring *ring;
	int per_ring_budget, work_done = 0;
	bool clean_complete = true;

	/* Port is down/reset, but napi_schedule_irqoff is exec by watchdog task or
	 * irq_miss_check
	 */
	if (test_bit(__RNPM_RESETTING, &adapter->state) ||
	    test_bit(__RNPM_DOWN, &adapter->state))
		return budget;

	rnpm_for_each_ring(ring, q_vector->tx)
		clean_complete &=
			!!rnpm_clean_tx_irq(q_vector, ring, budget);

	if (budget <= 0)
		return budget;

	/* attempt to distribute budget to each queue fairly, but don't allow
	 * the budget to go below 1 because we'll exit polling
	 */
	if (q_vector->rx.count > 1)
		per_ring_budget = max(budget / q_vector->rx.count, 1);
	else
		per_ring_budget = budget;
	rnpm_for_each_ring(ring, q_vector->rx) {
		int cleaned = 0;
		/* this ring is waitting to reset rx_len*/
		/* avoid to deal this ring until reset done */
		if (likely(!(ring->ring_flags &
			     RNPM_RING_FLAG_DO_RESET_RX_LEN)))
			cleaned = rnpm_clean_rx_irq(q_vector, ring,
						    per_ring_budget);
		if (unlikely(ring->ring_flags &
			     RNPM_RING_FLAG_DELAY_SETUP_RX_LEN)) {
			int head;

			rnpm_disable_rx_queue(adapter, ring);
			head = rd32(hw, RNPM_DMA_REG_RX_DESC_BUF_HEAD(
						ring->rnpm_queue_idx));
			if (head < RNPM_MIN_RXD) {
				ring->ring_flags &=
					(~RNPM_RING_FLAG_DELAY_SETUP_RX_LEN);
				ring->ring_flags |=
					RNPM_RING_FLAG_DO_RESET_RX_LEN;
			} else {
				wr32(hw,
				     RNPM_DMA_RX_START(
					     ring->rnpm_queue_idx),
				     1);
			}
		}
		work_done += cleaned;
		if (cleaned >= per_ring_budget)
			clean_complete = false;
	}

	/* If all work not completed, return budget and keep polling */
	if (!clean_complete) {
		int cpu_id = smp_processor_id();

		if (!cpumask_test_cpu(cpu_id, &q_vector->affinity_mask)) {
			napi_complete_done(napi, work_done);
			if (!test_bit(__RNPM_DOWN, &adapter->state))
				rnpm_irq_enable_queues(adapter, q_vector);
#ifdef TX_IRQ_MISS_REDUCE
			/* memory barrior */
			smp_mb();
			rnpm_for_each_ring(ring, q_vector->tx)
				rnpm_check_restart_tx(q_vector, ring);
#endif

			if (!test_bit(__RNPM_DOWN, &adapter->state)) {
				rnpm_htimer_start(q_vector);
				/* Return budget-1 so that polling stops */
				return budget - 1;
			}
		}
#ifdef TX_IRQ_MISS_REDUCE
		rnpm_for_each_ring(ring, q_vector->tx)
			rnpm_check_restart_tx(q_vector, ring);
#endif
		/* do poll only state not down */
		if (!test_bit(__RNPM_DOWN, &adapter->state))
			return budget;
	}

	if (likely(napi_complete_done(napi, work_done))) {
		rnpm_set_itr(q_vector);
		if (!test_bit(__RNPM_DOWN, &adapter->state)) {
			rnpm_irq_enable_queues(adapter, q_vector);
			/* we need this to ensure irq start before tx start */
#ifdef TX_IRQ_MISS_REDUCE
			/* memory barrior */
			smp_mb();
			rnpm_for_each_ring(ring, q_vector->tx)
				rnpm_check_restart_tx(q_vector, ring);
#endif
			rnpm_htimer_start(q_vector);
		}
	}
	return min(work_done, budget - 1);
}

/**
 * rnpm_irq_affinity_notify - Callback for affinity changes
 * @notify: context as to what irq was changed
 * @mask: the new affinity mask
 *
 * This is a callback function used by the irq_set_affinity_notifier function
 * so that we may register to receive changes to the irq affinity masks.
 **/
static void rnpm_irq_affinity_notify(struct irq_affinity_notify *notify,
				     const cpumask_t *mask)
{
	struct rnpm_q_vector *q_vector = container_of(
		notify, struct rnpm_q_vector, affinity_notify);

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
static void rnpm_irq_affinity_release(struct kref *ref)
{
}

/**
 * rnpm_request_msix_irqs - Initialize MSI-X interrupts
 * @adapter: board private structure
 *
 * rnpm_request_msix_irqs allocates MSI-X vectors and requests
 * interrupts from the kernel.
 **/
static int rnpm_request_msix_irqs(struct rnpm_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct rnpm_ring *ring = NULL;
	int err, i = 0;
	int cpu;

#ifdef RNPM_DISABLE_IRQ
	return 0;
#endif
	DPRINTK(IFUP, INFO, "num_q_vectors:%d\n", adapter->num_q_vectors);
	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnpm_q_vector *q_vector = adapter->q_vector[i];
		struct msix_entry *entry = &adapter->msix_entries[i];

		if (q_vector->tx.ring && q_vector->rx.ring) {
			snprintf(q_vector->name,
				 sizeof(q_vector->name) - 1, "%s-%s-%d-%d",
				 netdev->name, "TxRx", i, q_vector->v_idx);
		} else {
			WARN(!(q_vector->tx.ring && q_vector->rx.ring),
			     "%s vector%d tx rx is null, v_idx:%d\n",
			     netdev->name, i, q_vector->v_idx);
			continue;
		}
		err = request_irq(entry->vector, &rnpm_msix_clean_rings, 0,
				  q_vector->name, q_vector);
		if (err) {
			e_err(probe,
			      "%s:request_irq failed for MSIX interrupt:%d Error: %d\n",
			      netdev->name, entry->vector, err);
			goto free_queue_irqs;
		}
		/* register for affinity change notifications */
		q_vector->affinity_notify.notify =
			rnpm_irq_affinity_notify;
		q_vector->affinity_notify.release =
			rnpm_irq_affinity_release;
		irq_set_affinity_notifier(entry->vector,
					  &q_vector->affinity_notify);
		/* Spread affinity hints out across online CPUs.
		 * get_cpu_mask returns a static constant mask with
		 * a permanent lifetime so it's ok to pass to
		 * irq_set_affinity_hint without making a copy.
		 */
		cpu = cpumask_local_spread(q_vector->v_idx, -1);
#ifdef RNPM_ADAPTER_QUEUE_IRQ_AFFINITY_START_CPU0
		cpu = i;
#endif
		irq_set_affinity_hint(entry->vector, get_cpu_mask(cpu));
		rnpm_for_each_ring(ring, q_vector->tx) {
			if (ring) {
				netif_set_xps_queue(ring->netdev,
						    get_cpu_mask(cpu),
						    ring->queue_index);
			}
		}
	}

	return 0;
free_queue_irqs:
	while (i) {
		i--;
		irq_set_affinity_hint(adapter->msix_entries[i].vector,
				      NULL);
		irq_set_affinity_notifier(adapter->msix_entries[i].vector,
					  NULL);
		irq_set_affinity_hint(adapter->msix_entries[i].vector,
				      NULL);
		free_irq(adapter->msix_entries[i].vector,
			 adapter->q_vector[i]);
	}

	kfree(adapter->msix_entries);
	adapter->msix_entries = NULL;
	return err;
}

#ifdef DISABLE_RX_IRQ
int rx_poll_thread_handler(void *data)
{
	struct rnpm_adapter *adapter = data;
	struct net_device *netdev = adapter->netdev;
	int i;

	dbg_apt("%s  %s running...\n", __func__, netdev->name);
	do {
		for (i = 0; i < adapter->num_q_vectors; i++)
			rnpm_msix_clean_rings(0, adapter->q_vector[i]);

		usleep_range(1, 4);
	} while (!kthread_should_stop() &&
		 adapter->quit_poll_thread != true);

	dbg_apt("%s  %s stoped\n", __func__, netdev->name);
	return 0;
}
#endif

/**
 * rnpm_request_irq - initialize interrupts
 * @adapter: board private structure
 *
 * Attempts to configure interrupts using the best available
 * capabilities of the hardware and kernel.
 **/
static int rnpm_request_irq(struct rnpm_adapter *adapter)
{
	int err;

#ifdef DISABLE_RX_IRQ
	adapter->rx_poll_thread = kthread_run(rx_poll_thread_handler,
					      adapter, adapter->name);
	if (!adapter->rx_poll_thread) {
		netdev_err(adapter->netdev, "kthread_run faild!\n");
		return -EIO;
	}
	return 0;
#endif

	err = rnpm_request_msix_irqs(adapter);
	if (err)
		e_err(probe, "request_irq failed, Error %d\n", err);
	return err;
}

static void rnpm_free_irq(struct rnpm_adapter *adapter)
{
	int i;

#ifdef DISABLE_RX_IRQ
	return;
#endif

#ifdef RNPM_DISABLE_IRQ
	return;
#endif
	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct rnpm_q_vector *q_vector = adapter->q_vector[i];
		struct msix_entry *entry = &adapter->msix_entries[i];

		/* free only the irqs that were actually requested */
		if (!q_vector->rx.ring && !q_vector->tx.ring)
			continue;
		/* clear the affinity notifier in the IRQ descriptor */
		irq_set_affinity_notifier(adapter->msix_entries[i].vector,
					  NULL);
		/* clear the affinity_mask in the IRQ descriptor */
		irq_set_affinity_hint(entry->vector, NULL);
		DPRINTK(IFDOWN, INFO, "free irq %s\n", q_vector->name);
		free_irq(entry->vector, q_vector);
	}
}

/**
 * rnpm_irq_disable - Mask off interrupt generation on the NIC
 * @adapter: board private structure
 **/
static inline void rnpm_irq_disable(struct rnpm_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		rnpm_irq_disable_queues(adapter->q_vector[i]);
		synchronize_irq(adapter->msix_entries[i].vector);
	}
}

static int rnpm_xmit_nop_frame_ring(struct rnpm_adapter *adapter,
				    struct rnpm_ring *tx_ring)
{
	u16 i = tx_ring->next_to_use;
	struct rnpm_tx_desc *tx_desc;

	tx_desc = RNPM_TX_DESC(tx_ring, i);
	/* set length to 0 */
	tx_desc->blen_mac_ip_len = 0;
	tx_desc->vlan_cmd =
		cpu_to_le32(RNPM_TXD_CMD_EOP | RNPM_TXD_CMD_RS);
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
	rnpm_wr_reg(tx_ring->tail, 0);
	return 0;
}

__maybe_unused static int
rnpm_xmit_nop_frame_ring_temp(struct rnpm_adapter *adapter,
			      struct rnpm_ring *tx_ring)
{
	u16 i = tx_ring->next_to_use;
	struct rnpm_tx_desc *tx_desc;

	tx_desc = RNPM_TX_DESC(tx_ring, i);
	/* set length to 0 */
	tx_desc->blen_mac_ip_len = 0;
	tx_desc->vlan_cmd =
		cpu_to_le32(RNPM_TXD_CMD_EOP | RNPM_TXD_CMD_RS);
	/* update tail */
	i++;
	tx_desc++;
	if (i == tx_ring->count)
		i = 0;
	tx_ring->next_to_use = i;
	/* memory barrior */
	wmb();
	rnpm_wr_reg(tx_ring->tail, i);
	/* no need clean */
	tx_ring->next_to_clean = i;
	return 0;
}

/**
 * rnpm_tx_maxrate_own - callback to set the maximum per-queue bitrate
 * @netdev: network interface device structure
 * @queue_index: Tx queue to set
 * @maxrate: desired maximum transmit bitrate Mbps
 **/
static int rnpm_tx_maxrate_own(struct rnpm_adapter *adapter,
			       int queue_index)
{
	struct rnpm_ring *tx_ring = adapter->tx_ring[queue_index];
	u64 real_rate = 0;
	u32 maxrate = adapter->max_rate[queue_index];

	if (!maxrate)
		return rnpm_setup_tx_maxrate(
			adapter->hw.hw_addr, tx_ring, 0,
			adapter->hw.usecstocount * 1000000);
	/* we need turn it to bytes/s */
	real_rate = (maxrate * 1024 * 1024) / 8;
	rnpm_setup_tx_maxrate(adapter->hw.hw_addr, tx_ring, real_rate,
			      adapter->hw.usecstocount * 1000000);

	return 0;
}

/**
 * rnpm_configure_tx_ring - Configure 8259x Tx ring after Reset
 * @adapter: board private structure
 * @ring: structure containing ring specific data
 *
 * Configure the Tx descriptor ring after a reset.
 **/
void rnpm_configure_tx_ring(struct rnpm_adapter *adapter,
			    struct rnpm_ring *ring)
{
	struct rnpm_hw *hw = &adapter->hw;
	u8 queue_idx = ring->rnpm_queue_idx;

	wr32(hw, RNPM_DMA_REG_TX_DESC_BUF_BASE_ADDR_LO(queue_idx),
	     (u32)ring->dma);
	wr32(hw, RNPM_DMA_REG_TX_DESC_BUF_BASE_ADDR_HI(queue_idx),
	     (u32)(((u64)ring->dma) >> 32) | (hw->pfvfnum << 24));
	wr32(hw, RNPM_DMA_REG_TX_DESC_BUF_LEN(queue_idx), ring->count);

	/* tail <= head */
	ring->next_to_clean =
		rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_HEAD(queue_idx));
	ring->next_to_use = ring->next_to_clean;
	ring->tail =
		hw->hw_addr + RNPM_DMA_REG_TX_DESC_BUF_TAIL(queue_idx);
	rnpm_wr_reg(ring->tail, ring->next_to_use);
	wr32(hw, RNPM_DMA_REG_TX_DESC_FETCH_CTRL(queue_idx),
	     (8 << 0) | (TSRN10_TX_DEFAULT_BURST << 16));
	wr32(hw, RNPM_DMA_REG_TX_INT_DELAY_TIMER(queue_idx),
	     adapter->tx_usecs * hw->usecstocount);
	wr32(hw, RNPM_DMA_REG_TX_INT_DELAY_PKTCNT(queue_idx),
	     adapter->tx_frames);

	rnpm_tx_maxrate_own(adapter, ring->queue_index);
	/* reinitialize flowdirector state */
	if (adapter->flags & RNPM_FLAG_FDIR_HASH_CAPABLE) {
		ring->atr_sample_rate = adapter->atr_sample_rate;
		ring->atr_count = 0;
		set_bit(__RNPM_TX_FDIR_INIT_DONE, &ring->state);
	} else {
		ring->atr_sample_rate = 0;
	}
	/* initialize XPS */
	if (!test_and_set_bit(__RNPM_TX_XPS_INIT_DONE, &ring->state)) {
		struct rnpm_q_vector *q_vector = ring->q_vector;

		if (q_vector)
			netif_set_xps_queue(adapter->netdev,
					    &q_vector->affinity_mask,
					    ring->queue_index);
	}
	clear_bit(__RNPM_HANG_CHECK_ARMED, &ring->state);
}

/**
 * rnpm_configure_tx - Configure Transmit Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Tx unit of the MAC after a reset.
 **/
static void rnpm_configure_tx(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	u32 i, dma_axi_ctl;

	dma_axi_ctl = rd32(hw, RNPM_DMA_AXI_EN);
	dma_axi_ctl |= TX_AXI_RW_EN;
	wr32(hw, RNPM_DMA_AXI_EN, dma_axi_ctl);

	/* Setup the HW Tx Head and Tail descriptor pointers */
	for (i = 0; i < (adapter->num_tx_queues); i++)
		rnpm_configure_tx_ring(adapter, adapter->tx_ring[i]);
}

void rnpm_disable_rx_queue(struct rnpm_adapter *adapter,
			   struct rnpm_ring *ring)
{
	struct rnpm_hw *hw = &adapter->hw;

	wr32(hw, RNPM_DMA_RX_START(ring->rnpm_queue_idx), 0);
}

void rnpm_configure_rx_ring(struct rnpm_adapter *adapter,
			    struct rnpm_ring *ring)
{
	struct rnpm_hw *hw = &adapter->hw;
	u64 desc_phy = ring->dma;
	u16 q_idx = ring->rnpm_queue_idx;

	rnpm_disable_rx_queue(adapter, ring);
	/* set descripts registers*/
	wr32(hw, RNPM_DMA_REG_RX_DESC_BUF_BASE_ADDR_LO(q_idx),
	     (u32)desc_phy);
	wr32(hw, RNPM_DMA_REG_RX_DESC_BUF_BASE_ADDR_HI(q_idx),
	     ((u32)(desc_phy >> 32)) | (hw->pfvfnum << 24));
	wr32(hw, RNPM_DMA_REG_RX_DESC_BUF_LEN(q_idx), ring->count);

	ring->tail = hw->hw_addr + RNPM_DMA_REG_RX_DESC_BUF_TAIL(q_idx);
	ring->next_to_clean =
		rd32(hw, RNPM_DMA_REG_RX_DESC_BUF_HEAD(q_idx));
	ring->next_to_use = ring->next_to_clean;

	wr32(hw, RNPM_DMA_REG_RX_DESC_FETCH_CTRL(q_idx),
	     0 | (TSRN10_RX_DEFAULT_LINE << 0) |
		     (TSRN10_RX_DEFAULT_BURST << 16));
	wr32(hw, RNPM_DMA_REG_RX_INT_DELAY_TIMER(q_idx),
	     adapter->rx_usecs * hw->usecstocount);
	wr32(hw, RNPM_DMA_REG_RX_INT_DELAY_PKTCNT(q_idx),
	     adapter->rx_frames);
	rnpm_alloc_rx_buffers(ring, rnpm_desc_unused_rx(ring));
}

static void rnpm_configure_virtualization(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	u32 ring, vfnum = 0;

	if (!(adapter->flags & RNPM_FLAG_SRIOV_ENABLED))
		return;

	if (adapter->flags2 & RNPM_FLAG2_BRIDGE_MODE_VEB) {
		wr32(hw, RNPM_DMA_CONFIG,
		     rd32(hw, RNPM_DMA_CONFIG) & (~DMA_VEB_BYPASS));
		adapter->flags2 |= RNPM_FLAG2_BRIDGE_MODE_VEB;
	}
	ring = adapter->tx_ring[0]->rnpm_queue_idx;
	wr32(hw, RNPM_HOST_FILTER_EN, 1);
	wr32(hw, RNPM_REDIR_EN, 1);
	wr32(hw, RNPM_MRQC_IOV_EN, RNPM_IOV_ENABLED);
	wr32(hw, RNPM_ETH_DMAC_FCTRL,
	     rd32(hw, RNPM_ETH_DMAC_FCTRL) | RNPM_FCTRL_BROADCASE_BYPASS);
	/* Map PF MAC address in RAR Entry 0 to first pool following VFs */
	hw->mac.ops.set_vmdq(hw, 0, ring / 2);
	adapter->vf_num_for_pf = 0x80 | vfnum;
}

static void rnpm_set_rx_buffer_len(struct rnpm_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int max_frame = netdev->mtu + ETH_HLEN + 2 * ETH_FCS_LEN;
	struct rnpm_ring *rx_ring;
	int i;

	if (max_frame < (ETH_FRAME_LEN + ETH_FCS_LEN))
		max_frame = (ETH_FRAME_LEN + ETH_FCS_LEN);

	for (i = 0; i < adapter->num_rx_queues; i++) {
		rx_ring = adapter->rx_ring[i];
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
		clear_bit(__RNPM_RX_3K_BUFFER, &rx_ring->state);
		clear_bit(__RNPM_RX_BUILD_SKB_ENABLED, &rx_ring->state);
		set_bit(__RNPM_RX_BUILD_SKB_ENABLED, &rx_ring->state);
		hw_dbg(&adapter->hw, "set build skb\n");
#ifdef RNPM_OPTM_WITH_LPAGE
		rx_ring->rx_page_buf_nums = RNPM_PAGE_BUFFER_NUMS(rx_ring);
		rx_ring->rx_per_buf_mem = ALIGN(
			(rnpm_rx_offset(rx_ring) + rnpm_rx_bufsz(rx_ring) +
			 SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
			 RNPM_RX_HWTS_OFFSET),
			1024);
#endif
#else
		rx_ring->rx_buf_len = max_frame;
#endif /* CONFIG_RNPM_DISABLE_PACKET_SPLIT */
	}
}

/**
 * rnpm_configure_rx - Configure 8259x Receive Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Rx unit of the MAC after a reset.
 **/
static void rnpm_configure_rx(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	int i;
	u32 rxctrl = 0, dma_axi_ctl;

#if (PAGE_SIZE < 8192)
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
	struct rnpm_ring *rx_ring = adapter->rx_ring[0];
#endif
#endif

	/* set_rx_buffer_len must be called before ring initialization */
	rnpm_set_rx_buffer_len(adapter);
	for (i = 0; i < adapter->num_rx_queues; i++)
		rnpm_configure_rx_ring(adapter, adapter->rx_ring[i]);

	rnpm_ipsec_rss_update_args(adapter);

	if (adapter->pf_adapter->default_rx_ring > 0)
		wr32(hw, RNPM_ETH_DEFAULT_RX_RING,
		     adapter->pf_adapter->default_rx_ring);

#if (PAGE_SIZE < 8192)
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
	hw->dma_split_size = rnpm_rx_pg_size(rx_ring) / 2 -
			     rnpm_rx_offset(rx_ring) -
			     sizeof(struct skb_shared_info);
#endif
#endif
	if (!hw->dma_split_size)
		hw->dma_split_size = RNPM_RXBUFFER_1536;
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
		/* dma split size need cal by skb headroom and tailroom */
#define RNPM_DMA_RESPLIT_SIZE (hw->dma_split_size >> 4)
	dbg_apt("%s: dma_split_size=%d page_size=%d rx_page_size=%d rx_offset=%d skb_shared_info=%d\n",
		__func__, hw->dma_split_size, PAGE_SIZE,
		rnpm_rx_pg_size(rx_ring), rnpm_rx_offset(rx_ring),
		sizeof(struct skb_shared_info));
#else
#define RNPM_DMA_RESPLIT_SIZE ((adapter->rx_ring[0]->rx_buf_len + 15) >> 4)
	dbg_apt("%s: rx_buf_len + 15=%d page_size=%d\n", __func__,
		adapter->rx_ring[0]->rx_buf_len + 15, PAGE_SIZE);
#endif

#ifdef RNPM_DMA_SLICE
#if (PAGE_SIZE >= 65536)
	rnpm_setup_dma_rx(adapter, 569);
#else
	rnpm_setup_dma_rx(adapter, RNPM_DMA_RESPLIT_SIZE);
#endif
#else
	rnpm_setup_dma_rx(adapter, RNPM_DMA_RESPLIT_SIZE);
#endif
	/* enable all receives */
	rxctrl |= 0;

	dma_axi_ctl = rd32(hw, RNPM_DMA_AXI_EN);
	dma_axi_ctl |= RX_AXI_RW_EN;
	wr32(hw, RNPM_DMA_AXI_EN, dma_axi_ctl);
}

static int rnpm_vlan_rx_add_vid(struct net_device *netdev,
				__always_unused __be16 proto, u16 vid)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_pf_adapter *pf_adapter = adapter->pf_adapter;
	struct rnpm_hw *hw = &adapter->hw;
	int port = 0;
	unsigned long flags;

	if (!pf_support_vlan0) {
		if (!vid)
			return 0;
	}

	if (hw->mac.vlan_location == rnpm_vlan_location_nic) {
		if (hw->mac.ops.set_vfta) {
			if (vid < VLAN_N_VID) {
				set_bit(vid, adapter->active_vlans);
				spin_lock_irqsave(
					&pf_adapter->vlan_setup_lock,
					flags);
				set_bit(vid, pf_adapter->active_vlans);
				spin_unlock_irqrestore(
					&pf_adapter->vlan_setup_lock,
					flags);
			}
			/* add VID to filter table */
			spin_lock_irqsave(&pf_adapter->vlan_setup_lock,
					  flags);
			hw->mac.ops.set_vfta(&adapter->hw, vid, VMDQ_P(0),
					     true);
			spin_unlock_irqrestore(
				&pf_adapter->vlan_setup_lock, flags);
		}
	} else {
		if (hw->mac.ops.set_vfta_mac) {
			if (vid < VLAN_N_VID)
				set_bit(vid, adapter->active_vlans);
			hw->mac.ops.set_vfta_mac(&adapter->hw, vid,
						 VMDQ_P(0), true);
		}
	}

	if (adapter->flags & RNPM_FLAG_SRIOV_ENABLED) {
		u8 vfnum = RNPM_MAX_VF_CNT - 1;

		if (rd32(hw, RNPM_DMA_VERSION) >= 0x20201231) {
			for (port = 0; port < 4; port++)
				wr32(hw,
				     RNPM_DMA_PORT_VEB_VID_TBL(port,
							       vfnum),
				     vid);
		} else {
			wr32(hw,
			     RNPM_DMA_PORT_VEB_VID_TBL(adapter->port,
						       vfnum),
			     vid);
		}
	}
	return 0;
}

#ifdef NETIF_F_HW_VLAN_CTAG_RX
static int rnpm_vlan_rx_kill_vid(struct net_device *netdev,
				 __always_unused __be16 proto, u16 vid)
#else /* !NETIF_F_HW_VLAN_CTAG_RX */
static int rnpm_vlan_rx_kill_vid(struct net_device *netdev, u16 vid)
#endif /* NETIF_F_HW_VLAN_CTAG_RX */
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_pf_adapter __maybe_unused *pf_adapter =
		adapter->pf_adapter;
	struct rnpm_hw *hw = &adapter->hw;
	unsigned long flags;

	if (!pf_support_vlan0) {
		if (!vid)
			return 0;
	}

	if (hw->mac.ops.set_vfta) {
		/* remove VID from filter table only in no mutiport mode */
		if (!(adapter->flags & RNPM_FLAG_MUTIPORT_ENABLED))
			hw->mac.ops.set_vfta(&adapter->hw, vid, VMDQ_P(0),
					     false);
	}

	clear_bit(vid, adapter->active_vlans);
	if (adapter->flags & RNPM_FLAG_MUTIPORT_ENABLED) {
		if (hw->mac.vlan_location == rnpm_vlan_location_nic) {
			/* mutiport mode , only set update*/
			adapter->flags_feature |=
				RNPM_FLAG_DELAY_UPDATE_VLAN_TABLE;
		} else {
			int i;

			wr32(&adapter->hw,
			     RNPM_MAC_VLAN_HASH_TB(adapter->port), 0);
			/* update vlan hash table in mac */
			for_each_set_bit(i, adapter->active_vlans,
					 VLAN_N_VID) {
				if (hw->mac.ops.set_vfta_mac)
					hw->mac.ops.set_vfta_mac(
						&adapter->hw, i, VMDQ_P(0),
						true);
			}
			rnpm_ncsi_set_vfta_mac_generic(hw);
		}

	} else {
		spin_lock_irqsave(&pf_adapter->vlan_setup_lock, flags);
		clear_bit(vid, pf_adapter->active_vlans);
		spin_unlock_irqrestore(&pf_adapter->vlan_setup_lock,
				       flags);
	}
	return 0;
}

static u32
rnpm_vlan_filter_status_update(struct rnpm_pf_adapter *pf_adapter)
{
	u32 status = 1;
	unsigned long flags;
	int i;

	for (i = 0; i < pf_adapter->adapter_cnt; i++) {
		if (rnpm_port_is_valid(pf_adapter, i))
			status &= pf_adapter->vlan_filter_status[i];
	}
	spin_lock_irqsave(&pf_adapter->vlan_filter_lock, flags);
	pf_adapter->vlan_status_true = status;
	spin_unlock_irqrestore(&pf_adapter->vlan_filter_lock, flags);
	return status;
}

/**
 * rnpm_vlan_filter_disable - helper to disable hw vlan filtering
 * @adapter: driver data
 */
static void __maybe_unused
rnpm_vlan_filter_disable(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	struct rnpm_pf_adapter *pf_adapter = adapter->pf_adapter;
	u8 port = adapter->port;
	u32 value;

	pf_adapter->vlan_filter_status[port] = 0;
	if (hw->mac.vlan_location == rnpm_vlan_location_nic) {
		adapter->flags_feature |=
			RNPM_FLAG_DELAY_UPDATE_VLAN_FILTER;
		if (!rnpm_vlan_filter_status_update(pf_adapter))
			rnpm_vlan_filter_off(hw);
	} else {
		value = rd32(hw, RNPM_MAC_PKT_FLT(port));
		value &= (~RNPM_VLAN_HASH_EN);
		wr32(hw, RNPM_MAC_PKT_FLT(port), value);
		rnpm_vlan_filter_off(hw);
	}
}

/**
 * rnpm_vlan_filter_enable - helper to enable hw vlan filtering
 * @adapter: driver data
 */
static void __maybe_unused
rnpm_vlan_filter_enable(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	struct rnpm_pf_adapter *pf_adapter = adapter->pf_adapter;
	u8 port = adapter->port;

	pf_adapter->vlan_filter_status[port] = 1;
	/* open vlan filter if all port vlan filter on*/
	if (hw->mac.vlan_location == rnpm_vlan_location_nic) {
		adapter->flags_feature |=
			RNPM_FLAG_DELAY_UPDATE_VLAN_FILTER;
		if (rnpm_vlan_filter_status_update(pf_adapter))
			rnpm_vlan_filter_on(hw);
	} else {
		u32 value;

		value = rd32(hw, RNPM_MAC_PKT_FLT(port));
		value |= RNPM_VLAN_HASH_EN;
		wr32(hw, RNPM_MAC_PKT_FLT(port), value);
		rnpm_vlan_filter_off(hw);
	}
}

/**
 * rnpm_vlan_strip_disable - helper to disable hw vlan stripping
 * @adapter: driver data
 */
static void rnpm_vlan_strip_disable(struct rnpm_adapter *adapter)
{
	struct rnpm_ring *tx_ring;
	struct rnpm_hw *hw = &adapter->hw;
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		tx_ring = adapter->rx_ring[i];
		hw_queue_strip_rx_vlan(hw, tx_ring->rnpm_queue_idx, false);
	}
}

/**
 * rnpm_vlan_strip_enable - helper to enable hw vlan stripping
 * @adapter: driver data
 */
static void rnpm_vlan_strip_enable(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	struct rnpm_ring *tx_ring;
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		tx_ring = adapter->rx_ring[i];
		hw_queue_strip_rx_vlan(hw, tx_ring->rnpm_queue_idx, true);
	}
}

static void rnpm_restore_vlan(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	u16 vid;

	rnpm_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q), 0);
	for_each_set_bit(vid, adapter->active_vlans, VLAN_N_VID)
		rnpm_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q),
				     vid);
	/* config vlan mode for mac */
	wr32(hw, RNPM_MAC_TX_VLAN_MODE(adapter->port), 0x00100000);
}

/**
 * rnpm_write_uc_addr_list - write unicast addresses to RAR table
 * @netdev: network interface device structure
 *
 * Writes unicast address list to the RAR table.
 * Returns: -ENOMEM on failure/insufficient address space
 *                0 on no addresses written
 *                X on writing X addresses to the RAR table
 **/
static int rnpm_write_uc_addr_list(struct net_device *netdev)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	unsigned int rar_entries = adapter->uc_num - 1;
	int count = 0;

	/* In SR-IOV mode significantly less RAR entries are available */
	if (adapter->flags & RNPM_FLAG_SRIOV_ENABLED)
		rar_entries = RNPM_MAX_PF_MACVLANS - 1;

	if (netdev_uc_count(netdev) > rar_entries)
		return -ENOMEM;

	rar_entries += adapter->uc_off;
	if (!netdev_uc_empty(netdev)) {
		struct netdev_hw_addr *ha;

		hw_dbg(hw, "%s: rar_entries:%d, uc_count:%d offset %d\n",
		       __func__, rar_entries, adapter->uc_off,
		       netdev_uc_count(netdev));
		if (!hw->mac.ops.set_rar)
			return -ENOMEM;
		/* setup mac unicast filters */
		if (hw->mac.mc_location == rnpm_mc_location_mac)
			if (!hw->mac.ops.set_rar_mac)
				return -ENOMEM;

		netdev_for_each_uc_addr(ha, netdev) {
			if (!rar_entries)
				break;
			hw->mac.ops.set_rar(hw, rar_entries, ha->addr,
					    VMDQ_P(0), RNPM_RAH_AV);

			/* if use mac filter we should also set Unicast to mac */
			if (hw->mac.mc_location == rnpm_mc_location_mac)
				hw->mac.ops.set_rar_mac(
					hw, rar_entries - adapter->uc_off,
					ha->addr, VMDQ_P(0),
					adapter->port);
			rar_entries--;
			count++;
		}
	}

	hw_dbg(hw, "%s: Clearing RAR[%d - %d]\n", __func__,
	       adapter->uc_off + 1, rar_entries);
	for (; rar_entries > adapter->uc_off; rar_entries--) {
		hw->mac.ops.clear_rar(hw, rar_entries);
		if (hw->mac.mc_location == rnpm_mc_location_mac)
			hw->mac.ops.clear_rar_mac(
				hw, rar_entries - adapter->uc_off,
				adapter->port);
	}
	rnpm_ncsi_set_uc_addr_generic(hw);
	return count;
}

static void rnpm_setup_fctrl(struct rnpm_hw *hw)
{
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	struct rnpm_pf_adapter *pf_adapter = adapter->pf_adapter;
	u32 fctrl = 0;
	int i;

	for (i = 0; i < pf_adapter->adapter_cnt; i++)
		if (rnpm_port_is_valid(pf_adapter, i))
			fctrl |= pf_adapter->fctrl[i];
	wr32(hw, RNPM_ETH_DMAC_FCTRL, fctrl);
}

/**
 * rnpm_set_rx_mode - Unicast, Multicast and Promiscuous mode set
 * @netdev: network interface device structure
 *
 * The set_rx_method entry point is called whenever the unicast/multicast
 * address list or the network interface flags are updated.  This routine is
 * responsible for configuring the hardware for proper unicast, multicast and
 * promiscuous mode.
 **/
void rnpm_set_rx_mode(struct net_device *netdev)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_pf_adapter *pf_adapter = adapter->pf_adapter;
	struct rnpm_hw *hw = &adapter->hw;
	u32 fctrl;
	u32 fctrl_mac = 0;
	netdev_features_t __maybe_unused features = netdev->features;
	int count;
	u8 port = adapter->port;

	fctrl = pf_adapter->fctrl[port];
	fctrl &= ~(RNPM_FCTRL_UPE | RNPM_FCTRL_MPE);

	/* promisc mode */
	if (netdev->flags & IFF_PROMISC) {
		hw->addr_ctrl.user_set_promisc = true;
		fctrl |= (RNPM_FCTRL_UNICASE_BYPASS |
			  RNPM_FCTRL_MULTICASE_BYPASS |
			  RNPM_FCTRL_BROADCASE_BYPASS);
		fctrl_mac |= RNPM_RX_ALL;
		/* disable hardware filter vlans in promisc mode */
#ifdef NETIF_F_HW_VLAN_CTAG_FILTER
		features &= ~NETIF_F_HW_VLAN_CTAG_FILTER;
#endif
		/* close vlan offload */
#ifdef NETIF_F_HW_VLAN_CTAG_RX
		features &= ~NETIF_F_HW_VLAN_CTAG_RX;
#endif
	} else {
		if (netdev->flags & IFF_ALLMULTI) {
			fctrl |= RNPM_FCTRL_MULTICASE_BYPASS;
			fctrl_mac |= RNPM_RX_ALL_MUL;
		} else {
			/* Write addresses to the MTA, if the attempt fails
			 * then we should just turn on promiscuous mode so
			 * that we can at least receive multicast traffic
			 */
			count = hw->mac.ops.update_mc_addr_list(hw,
								netdev);
			if (count < 0) {
				fctrl |= RNPM_FCTRL_MPE;
				fctrl_mac |= RNPM_RX_ALL_MUL;
			}
		}
		hw->addr_ctrl.user_set_promisc = false;
	}

	/*
	 * Write addresses to available RAR registers, if there is not
	 * sufficient space to store all the addresses then enable
	 * unicast promiscuous mode
	 */
	if (rnpm_write_uc_addr_list(netdev) < 0)
		fctrl |= RNPM_FCTRL_UPE;

	if (adapter->num_vfs)
		rnpm_restore_vf_multicasts(adapter);

	if (hw->mac.mc_location == rnpm_mc_location_mac) {
		u32 value;

		value = rd32(hw, RNPM_MAC_PKT_FLT(port));
		if (!(adapter->flags & RNPM_FLAG_SWITCH_LOOPBACK_EN))
			value &= ~(RNPM_RX_ALL | RNPM_RX_ALL_MUL);
		value |= fctrl_mac;
		wr32(hw, RNPM_MAC_PKT_FLT(port), value);
		/* in this mode always close nic mc uc */
		fctrl |= RNPM_FCTRL_MULTICASE_BYPASS;
		fctrl |= RNPM_FCTRL_UNICASE_BYPASS;
		wr32(hw, RNPM_ETH_DMAC_FCTRL, fctrl);
	} else {
		pf_adapter->fctrl[port] = fctrl;

		rnpm_setup_fctrl(hw);
	}

#ifdef NETIF_F_HW_VLAN_CTAG_FILTER
	if (features & NETIF_F_HW_VLAN_CTAG_FILTER)
		rnpm_vlan_filter_enable(adapter);
	else
		rnpm_vlan_filter_disable(adapter);
#endif

#ifdef NETIF_F_HW_VLAN_CTAG_RX
	if (features & NETIF_F_HW_VLAN_CTAG_RX)
		rnpm_vlan_strip_enable(adapter);
	else
		rnpm_vlan_strip_disable(adapter);
#endif
}

static void rnpm_napi_enable_all(struct rnpm_adapter *adapter)
{
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++)
		napi_enable(&adapter->q_vector[q_idx]->napi);
}

static bool rnpm_wait_irq_miss_check_done(struct rnpm_adapter *adapter)
{
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++) {
		if (test_bit(RNPM_IRQ_MISS_HANDLE_DONE,
			     &adapter->q_vector[q_idx]->flags))
			return false;
	}
	return true;
}

static void rnpm_napi_disable_all(struct rnpm_adapter *adapter)
{
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++) {
		/* stop timer avoid error */
		rnpm_htimer_stop(adapter->q_vector[q_idx]);
		napi_disable(&adapter->q_vector[q_idx]->napi);
	}
}

/* Additional bittime to account for RNPM framing */
#define RNPM_ETH_FRAMING 20

static void rnpm_fdir_filter_restore(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	struct hlist_node *node2;
	struct rnpm_fdir_filter *filter;
	unsigned long flags;

	spin_lock_irqsave(&adapter->fdir_perfect_lock, flags);
	/* enable tcam if set tcam mode */
	if (adapter->fdir_mode == fdir_mode_tcam) {
		wr32(hw, RNPM_ETH_TCAM_EN, 1);
		wr32(hw, RNPM_TOP_ETH_TCAM_CONFIG_ENABLE, 1);
		wr32(hw, RNPM_TCAM_CACHE_ENABLE, 1);
	}

	hlist_for_each_entry_safe(filter, node2,
				  &adapter->fdir_filter_list, fdir_node) {
		rnpm_fdir_write_perfect_filter(
			adapter->fdir_mode, hw, &filter->filter,
			filter->hw_idx,
			(filter->action == RNPM_FDIR_DROP_QUEUE) ?
				RNPM_FDIR_DROP_QUEUE :
				adapter->rx_ring[filter->action]
					->rnpm_queue_idx);
	}
	spin_unlock_irqrestore(&adapter->fdir_perfect_lock, flags);
}

__maybe_unused static void
rnpm_configure_pause(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;

	hw->mac.ops.fc_enable(hw);
}

static void rnpm_vlan_stags_flag(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	u8 port = adapter->port;

	if (adapter->flags2 & RNPM_FLAG2_VLAN_STAGS_ENABLED) {
		wr32(hw, RNPM_MAC_TX_VLAN_TAG(port),
		     RNPM_ERIVLT | RNPM_EDVLP | RNPM_ETV |
			     (RNPM_EVLS_ALWAYS_STRIP << RNPM_EVLS_OFFSET) |
			     RNPM_VL_MODE_OFF);
		wr32(hw, RNPM_MAC_TX_VLAN_MODE(port), 0x180000);
		wr32(hw, RNPM_MAC_INNER_VLAN_INCL(port), 0x100000);
	} else {
		wr32(hw, RNPM_MAC_TX_VLAN_TAG(port),
		     RNPM_VTHM | RNPM_VL_MODE_ON | RNPM_ETV);
		wr32(hw, RNPM_MAC_TX_VLAN_MODE(port), 0x100000);
		wr32(hw, RNPM_MAC_INNER_VLAN_INCL(port), 0x100000);
	}
}

static void rnpm_configure(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;

	rnpm_configure_virtualization(adapter);
	/* init setup pause */
	hw->mac.ops.setup_fc(hw);
	/* Unicast, Multicast and Promiscuous mode set */
	rnpm_set_rx_mode(adapter->netdev);
	/* reset unicast address */
	hw->mac.ops.set_rar(hw, adapter->uc_off, hw->mac.addr, VMDQ_P(0),
			    RNPM_RAH_AV);

	/* setup mac unicast filters */
	if (hw->mac.mc_location == rnpm_mc_location_mac)
		hw->mac.ops.set_rar_mac(hw, 0, hw->mac.addr, VMDQ_P(0),
					adapter->port);
	/* what conditions should restore vlan ? */
	rnpm_restore_vlan(adapter);
	wr32(hw, RNPM_HOST_FILTER_EN, 1);
	wr32(hw, RNPM_REDIR_EN, 1);
	rnpm_init_rss_table(adapter);

	/* open sctp check en */
	if (hw->feature_flags & RNPM_NET_FEATURE_RX_CHECKSUM)
		wr32(hw, RNPM_ETH_SCTP_CHECKSUM_EN, 1);

	rnpm_vlan_stags_flag(adapter);
	if (adapter->flags & RNPM_FLAG_FDIR_PERFECT_CAPABLE)
		rnpm_fdir_filter_restore(adapter);

	if (hw->dma_version >= 0x20210108) {
		/*  mark Multicast as broadcast */
		wr32(hw, RNPM_VEB_MAC_MASK_LO, 0xffffffff);
		wr32(hw, RNPM_VEB_MAC_MASK_HI, 0xfeff);
	}

	rnpm_configure_tx(adapter);
	rnpm_configure_rx(adapter);
}

static inline bool rnpm_is_sfp(struct rnpm_hw *hw)
{
	return true;
}

/**
 * rnpm_sfp_link_config - set up SFP+ link
 * @adapter: pointer to private adapter struct
 **/
static void rnpm_sfp_link_config(struct rnpm_adapter *adapter)
{
	/*
	 * We are assuming the worst case scenario here, and that
	 * is that an SFP was inserted/removed after the reset
	 * but before SFP detection was enabled.  As such the best
	 * solution is to just start searching as soon as we start
	 */
	adapter->flags2 |= RNPM_FLAG2_SFP_NEEDS_RESET;
}

/**
 * rnpm_non_sfp_link_config - set up non-SFP+ link
 * @hw: pointer to private hardware struct
 *
 * Returns 0 on success, negative on failure
 **/
static int rnpm_non_sfp_link_config(struct rnpm_hw *hw)
{
	u32 ret = RNPM_ERR_LINK_SETUP;

	return ret;
}

__maybe_unused void rnpm_control_mac_rx(struct rnpm_adapter *adapter,
					bool on)
{
	struct rnpm_hw *hw = &adapter->hw;
	u8 port = adapter->port;
	u32 value = 0;
	u32 count = 0;

	if (pci_channel_offline(hw->pdev))
		return;

	if (on) {
		wr32(hw, RNPM_ETH_RX_PROGFULL_THRESH_PORT(adapter->port),
		     RECEIVE_ALL_THRESH);
		do {
			value = rd32(hw, RNPM_MAC_RX_CFG(port)) | 0x01;
			value = adapter->mac_ipc_off ?
					value & ~RNPM_MAX_RX_CFG_IPC :
					value | RNPM_MAX_RX_CFG_IPC;
			wr32(hw, RNPM_MAC_RX_CFG(port), value);
			usleep_range(100, 200);
			value = rd32(hw, RNPM_MAC_RX_CFG(port));
			count++;
			if (count > 1000) {
				netdev_info(adapter->netdev,
					    "setup rx on timeout\n");
				break;
			}
		} while (!(value & 0x01));

		do {
			value = rd32(hw, RNPM_MAC_RX_CFG(port)) & (~0x400);
			value = adapter->mac_ipc_off ?
					value & ~RNPM_MAX_RX_CFG_IPC :
					value | RNPM_MAX_RX_CFG_IPC;
			if (adapter->is_mac_loopback)
				value |= RNPM_LM;
			else
				value &= ~RNPM_LM;
			wr32(hw, RNPM_MAC_RX_CFG(port), value);
			usleep_range(100, 200);
			value = rd32(hw, RNPM_MAC_RX_CFG(port));
			count++;
			if (count > 1000) {
				netdev_info(adapter->netdev,
					    "setup rx off timeout\n");
				break;
			}
		} while (adapter->is_mac_loopback ? !(value & RNPM_LM) :
						    value & RNPM_LM);
		/* in this mode close mc filter in mac */
		if (hw->mac.mc_location == rnpm_mc_location_nic)
			wr32(hw, RNPM_MAC_PKT_FLT(port),
			     rd32(hw, RNPM_MAC_PKT_FLT(port)) | RNPM_RA);
		else
			wr32(hw, RNPM_MAC_PKT_FLT(port),
			     rd32(hw, RNPM_MAC_PKT_FLT(port)) | RNPM_HPF);
	} else {
		wr32(hw, RNPM_ETH_RX_PROGFULL_THRESH_PORT(adapter->port),
		     DROP_ALL_THRESH);
		do {
			value = rd32(hw, RNPM_MAC_RX_CFG(port)) | 0x400;
			value = adapter->mac_ipc_off ?
					value & ~RNPM_MAX_RX_CFG_IPC :
					value | RNPM_MAX_RX_CFG_IPC;
			wr32(hw, RNPM_MAC_RX_CFG(port), value);
			usleep_range(100, 200);
			value = rd32(hw, RNPM_MAC_RX_CFG(port));
			count++;
			if (count > 1000) {
				netdev_info(adapter->netdev,
					    "setup rx on timeout\n");
				break;
			}
		} while (!(value & RNPM_LM));
	}
}

static void rnpm_up_complete(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	int err, i;

	rnpm_control_mac_rx(adapter, false);
	rnpm_configure_msix(adapter);
	/* memory barrier */
	smp_mb__before_atomic();
	clear_bit(__RNPM_DOWN, &adapter->state);
	rnpm_napi_enable_all(adapter);

	if (rnpm_is_sfp(hw)) {
		rnpm_sfp_link_config(adapter);
	} else {
		err = rnpm_non_sfp_link_config(hw);
		if (err)
			e_err(probe, "link_config FAILED %d\n", err);
	}
	/*clear any pending interrupts*/
	rnpm_irq_enable(adapter);
	/* enable transmits */
	netif_tx_start_all_queues(adapter->netdev);

	/* enable rx transmit */
	for (i = 0; i < adapter->num_rx_queues; i++)
		wr32(hw,
		     RNPM_DMA_RX_START(
			     adapter->rx_ring[i]->rnpm_queue_idx),
		     1);
	adapter->link_check_timeout = jiffies;
	mod_timer(&adapter->service_timer, HZ + jiffies);
	rnpm_mbx_ifup_down(&adapter->hw, MBX_IFUP);
#ifdef RNPM_DISABLE_IRQ
	rnpm_mbx_lane_link_changed_event_enable(&adapter->hw, false);
#else
	rnpm_mbx_lane_link_changed_event_enable(&adapter->hw, true);
#endif
}

void rnpm_reinit_locked(struct rnpm_adapter *adapter)
{
	WARN_ON(in_interrupt());
	while (test_and_set_bit(__RNPM_RESETTING, &adapter->state))
		usleep_range(1000, 2000);
	rnpm_down(adapter);
	/*
	 * If SR-IOV enabled then wait a bit before bringing the adapter
	 * back up to give the VFs time to respond to the reset.  The
	 * two second wait is based upon the watchdog timer cycle in
	 * the VF driver.
	 */
	if (adapter->flags & RNPM_FLAG_SRIOV_ENABLED)
		msleep(2000);
	rnpm_up(adapter);
	clear_bit(__RNPM_RESETTING, &adapter->state);
}

void rnpm_up(struct rnpm_adapter *adapter)
{
	/* hardware has been reset, we need to reload some things */
	rnpm_configure(adapter);
	rnpm_up_complete(adapter);
}

void rnpm_reset(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	int err;

	/* lock SFP init bit to prevent race conditions with the watchdog */
	while (test_and_set_bit(__RNPM_IN_SFP_INIT, &adapter->state))
		usleep_range(1000, 2000);

	/* clear all SFP and link config related flags while holding SFP_INIT */
	adapter->flags2 &=
		~(RNPM_FLAG2_SEARCH_FOR_SFP | RNPM_FLAG2_SFP_NEEDS_RESET);
	adapter->flags &= ~RNPM_FLAG_NEED_LINK_CONFIG;
	err = hw->mac.ops.init_hw(hw);
	if (err)
		e_dev_err("init_hw: Hardware Error: err:%d. line:%d\n",
			  err, __LINE__);
	clear_bit(__RNPM_IN_SFP_INIT, &adapter->state);

	/* reprogram the RAR[0] in case user changed it. */
	hw->mac.ops.set_rar(hw, adapter->uc_off, hw->mac.addr, VMDQ_P(0),
			    RNPM_RAH_AV);
	/* setup mac unicast filters */
	if (hw->mac.mc_location == rnpm_mc_location_mac)
		hw->mac.ops.set_rar_mac(hw, 0, hw->mac.addr, VMDQ_P(0),
					adapter->port);

#ifndef NO_PTP
	if (module_enable_ptp) {
		if (adapter->flags2 & RNPM_FLAG2_PTP_ENABLED &&
		    (adapter->ptp_rx_en || adapter->ptp_tx_en))
			rnpm_ptp_reset(adapter);
	}
#endif
}

#ifdef RNPM_OPTM_WITH_LPAGE
/**
 * rnpm_clean_rx_ring - Free Rx Buffers per Queue
 * @rx_ring: ring to free buffers from
 **/
static void rnpm_clean_rx_ring(struct rnpm_ring *rx_ring)
{
	u16 i = rx_ring->next_to_clean;
	struct rnpm_rx_buffer *rx_buffer;

	if (!rx_ring->rx_buffer_info)
		return;
	if (rx_ring->skb)
		dev_kfree_skb(rx_ring->skb);

	rx_ring->skb = NULL;
	rx_buffer = &rx_ring->rx_buffer_info[i];
	/* Free all the Rx ring sk_buffs */
#ifdef CONFIG_RNPM_DISABLE_PACKET_SPLIT
	while (i != rx_ring->next_to_use) {
#else
	while (i != rx_ring->next_to_alloc) {
#endif
		if (!rx_buffer->page)
			goto next_buffer;
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
		/* Invalidate cache lines that may have been written to by
		 * device so that we avoid corrupting memory.
		 */
		dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma,
					      rx_buffer->page_offset,
					      rnpm_rx_bufsz(rx_ring),
					      DMA_FROM_DEVICE);
		/* free resources associated with mapping */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnpm_rx_bufsz(rx_ring),
				     DMA_FROM_DEVICE, RNPM_RX_DMA_ATTR);
		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
#else /* CONFIG_RNPM_DISABLE_PACKET_SPLIT */
		if (rx_buffer->dma) {
			dma_unmap_single(rx_ring->dev, rx_buffer->dma,
					 rx_ring->rx_buf_len,
					 DMA_FROM_DEVICE);
			rx_buffer->dma = 0;
		}
#endif /* CONFIG_RNPM_DISABLE_PACKET_SPLIT */
		rx_buffer->page = NULL;
next_buffer:
		i++;
		rx_buffer++;
		if (i == rx_ring->count) {
			i = 0;
			rx_buffer = rx_ring->rx_buffer_info;
		}
	}
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
	rx_ring->next_to_alloc = 0;
	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
#endif
}
#else
/**
 * rnpm_clean_rx_ring - Free Rx Buffers per Queue
 * @rx_ring: ring to free buffers from
 **/
static void rnpm_clean_rx_ring(struct rnpm_ring *rx_ring)
{
	u16 i = rx_ring->next_to_clean;
	struct rnpm_rx_buffer *rx_buffer = &rx_ring->rx_buffer_info[i];

	/* Free all the Rx ring sk_buffs */
#ifdef CONFIG_RNPM_DISABLE_PACKET_SPLIT
	while (i != rx_ring->next_to_use) {
#else
	while (i != rx_ring->next_to_alloc) {
#endif
		if (rx_buffer->skb) {
			struct sk_buff *skb = rx_buffer->skb;
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
			/* no need this */
			if (RNPM_CB(skb)->page_released)
				dma_unmap_page_attrs(
					rx_ring->dev, RNPM_CB(skb)->dma,
					rnpm_rx_pg_size(rx_ring),
					DMA_FROM_DEVICE, RNPM_RX_DMA_ATTR);
#endif /* CONFIG_RNPM_DISABLE_PACKET_SPLIT */
			dev_kfree_skb(skb);
			rx_buffer->skb = NULL;
		}

#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
		/* Invalidate cache lines that may have been written to by
		 * device so that we avoid corrupting memory.
		 */
		dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->dma,
					      rx_buffer->page_offset,
					      rnpm_rx_bufsz(rx_ring),
					      DMA_FROM_DEVICE);

		/* free resources associated with mapping */
		dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma,
				     rnpm_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE, RNPM_RX_DMA_ATTR);
		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
		/* now this page is not used */
		rx_buffer->page = NULL;
#else /* CONFIG_RNPM_DISABLE_PACKET_SPLIT */
		if (rx_buffer->dma) {
			dma_unmap_single(rx_ring->dev, rx_buffer->dma,
					 rx_ring->rx_buf_len,
					 DMA_FROM_DEVICE);
			rx_buffer->dma = 0;
		}
#endif /* CONFIG_RNPM_DISABLE_PACKET_SPLIT */
		i++;
		rx_buffer++;
		if (i == rx_ring->count) {
			i = 0;
			rx_buffer = rx_ring->rx_buffer_info;
		}
	}

#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
	rx_ring->next_to_alloc = 0;
	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
#endif
}
#endif
/**
 * rnpm_clean_tx_ring - Free Tx Buffers
 * @tx_ring: ring to be cleaned
 **/
static void rnpm_clean_tx_ring(struct rnpm_ring *tx_ring)
{
	unsigned long size;
	u16 i = tx_ring->next_to_clean;
	struct rnpm_tx_buffer *tx_buffer = &tx_ring->tx_buffer_info[i];

	BUG_ON(tx_ring == NULL);
	if (!tx_ring->tx_buffer_info)
		return;
	while (i != tx_ring->next_to_use) {
		struct rnpm_tx_desc *eop_desc, *tx_desc;

		dev_kfree_skb_any(tx_buffer->skb);
		/* unmap skb header data */
		dma_unmap_single(tx_ring->dev,
				 dma_unmap_addr(tx_buffer, dma),
				 dma_unmap_len(tx_buffer, len),
				 DMA_TO_DEVICE);

		eop_desc = tx_buffer->next_to_watch;
		tx_desc = RNPM_TX_DESC(tx_ring, i);
		/* unmap remaining buffers */
		while (tx_desc != eop_desc) {
			tx_buffer++;
			tx_desc++;
			i++;
			if (unlikely(i == tx_ring->count)) {
				i = 0;
				tx_buffer = tx_ring->tx_buffer_info;
				tx_desc = RNPM_TX_DESC(tx_ring, 0);
			}
			/* unmap any remaining paged data */
			if (dma_unmap_len(tx_buffer, len)) {
				dma_unmap_page(
					tx_ring->dev,
					dma_unmap_addr(tx_buffer, dma),
					dma_unmap_len(tx_buffer, len),
					DMA_TO_DEVICE);
				dma_unmap_len_set(tx_buffer, len, 0);
			}
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
	size = sizeof(struct rnpm_tx_buffer) * tx_ring->count;
	memset(tx_ring->tx_buffer_info, 0, size);
	/* Zero out the descriptor ring */
	memset(tx_ring->desc, 0, tx_ring->size);

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;
}

/**
 * rnpm_clean_all_rx_rings - Free Rx Buffers for all queues
 * @adapter: board private structure
 **/
static void rnpm_clean_all_rx_rings(struct rnpm_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		rnpm_clean_rx_ring(adapter->rx_ring[i]);
}

/**
 * rnpm_clean_all_tx_rings - Free Tx Buffers for all queues
 * @adapter: board private structure
 **/
static void rnpm_clean_all_tx_rings(struct rnpm_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		rnpm_clean_tx_ring(adapter->tx_ring[i]);
}

static void rnpm_fdir_filter_exit(struct rnpm_adapter *adapter)
{
	struct hlist_node *node2;
	struct rnpm_fdir_filter *filter;
	struct rnpm_hw *hw = &adapter->hw;
	unsigned long flags;

	spin_lock_irqsave(&adapter->fdir_perfect_lock, flags);
	hlist_for_each_entry_safe(filter, node2,
				  &adapter->fdir_filter_list, fdir_node) {
		/* call earase to hw */
		rnpm_fdir_erase_perfect_filter(adapter->fdir_mode, hw,
					       &filter->filter,
					       filter->hw_idx);
		hlist_del(&filter->fdir_node);
		kfree(filter);
	}
	adapter->fdir_filter_count = 0;
	spin_unlock_irqrestore(&adapter->fdir_perfect_lock, flags);
}

void rnpm_down(struct rnpm_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct rnpm_hw *hw = &adapter->hw;
	int i, retry = 200;
	bool is_pci_dead = pci_channel_offline(adapter->pdev);
	bool is_pci_online = !is_pci_dead;

	rnpm_dbg("%s %s port:%d!!!\n", netdev->name, __func__,
		 adapter->port);
	rnpm_logd(LOG_FUNC_ENTER, "enter %s %s\n", __func__,
		  adapter->netdev->name);
	/* signal that we are down to the interrupt handler */
	set_bit(__RNPM_DOWN, &adapter->state);
	netif_tx_stop_all_queues(netdev);
	netif_carrier_off(netdev);
	netif_tx_disable(netdev);
	rnpm_control_mac_rx(adapter, false);
	rnpm_mbx_ifup_down(&adapter->hw, MBX_IFDOWN);
	rnpm_link_stat_mark(hw, hw->nr_lane, 0);
	usleep_range(10000, 20000);

	/* disable all enabled rx queues */
	for (i = 0; i < adapter->num_rx_queues && is_pci_online; i++) {
		rnpm_disable_rx_queue(adapter, adapter->rx_ring[i]);
		/* only handle when srio enable or mutiport mode and change
		 * rx length setup
		 */
		if (((adapter->flags & RNPM_FLAG_SRIOV_ENABLED) ||
		     (adapter->flags & RNPM_FLAG_MUTIPORT_ENABLED)) &&
		    (adapter->rx_ring[i]->ring_flags &
		     RNPM_RING_FLAG_CHANGE_RX_LEN)) {
			int head;

			head = rd32(hw, RNPM_DMA_REG_RX_DESC_BUF_HEAD(
						adapter->rx_ring[i]
							->rnpm_queue_idx));
			adapter->rx_ring[i]->ring_flags &=
				(~RNPM_RING_FLAG_CHANGE_RX_LEN);
			/* delay setup rx length to wait rx head to 0 */
			if (head >= adapter->rx_ring[i]->reset_count) {
				adapter->rx_ring[i]->ring_flags |=
					RNPM_RING_FLAG_DELAY_SETUP_RX_LEN;
				/* set sw count to head + 1*/
				adapter->rx_ring[i]->temp_count = head + 1;
			}
		}
	}
	rnpm_irq_disable(adapter);
	rnpm_napi_disable_all(adapter);
	adapter->flags2 &= ~(RNPM_FLAG2_FDIR_REQUIRES_REINIT |
			     RNPM_FLAG2_RESET_REQUESTED);
	adapter->flags &= ~RNPM_FLAG_NEED_LINK_UPDATE;
	if (adapter->num_vfs) {
		/* ping all the active vfs to let them know we are going down */
		rnpm_ping_all_vfs(adapter);
		rnpm_disable_tx_rx(adapter);
	}
	del_timer_sync(&adapter->service_timer);
	cancel_work_sync(&adapter->service_task);
	clear_bit(__RNPM_SERVICE_SCHED, &adapter->state);

	while (retry) {
		if (rnpm_wait_irq_miss_check_done(adapter))
			break;
		retry--;
		usleep_range(20000, 40000);
	}
	if (!retry)
		netdev_err(adapter->netdev,
			   "error: %s wait ire miss check done timeout\n",
			   netdev->name);

	/* disable transmits in the hardware now that interrupts are off */
	for (i = 0; i < adapter->num_tx_queues && is_pci_online; i++) {
		struct rnpm_hw *hw = &adapter->hw;
		struct rnpm_ring *tx_ring = adapter->tx_ring[i];
		int count = tx_ring->count;
		int head, tail;
		int timeout = 0;
		u32 status = 0;

		do {
			status =
				rd32(hw, RNPM_DMA_TX_READY(
						 tx_ring->rnpm_queue_idx));
			usleep_range(1000, 2000);
			timeout++;
			rnpm_dbg("wait %d tx ready to 1\n",
				 tx_ring->rnpm_queue_idx);
		} while ((status != 1) && (timeout < 100));

		if (timeout >= 100) {
			head = rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_HEAD(
						tx_ring->rnpm_queue_idx));

			tail = rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_TAIL(
						tx_ring->rnpm_queue_idx));
			netdev_err(
				netdev,
				"wait tx ready timeout, %s i:%d q_idx:%d head:%d tail:%d\n",
				netdev->name, i, tx_ring->rnpm_queue_idx,
				head, tail);
		}

		/* try to set tx head to 0 in sriov mode since we don't reset
		 * in sriov on or mutiport mode
		 */
		if ((adapter->flags & RNPM_FLAG_SRIOV_ENABLED) ||
		    (adapter->flags & RNPM_FLAG_MUTIPORT_ENABLED)) {
			head = rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_HEAD(
						tx_ring->rnpm_queue_idx));
			if (head != 0) {
				u16 next_to_use = tx_ring->next_to_use;

				if (head != (count - 1))
					wr32(hw,
					     RNPM_DMA_REG_TX_DESC_BUF_LEN(
						     tx_ring->rnpm_queue_idx),
					     head + 1);
				tx_ring->next_to_use = head;
				rnpm_xmit_nop_frame_ring(adapter, tx_ring);
				while ((head != 0) && (timeout < 1000)) {
					head = rd32(
						hw,
						RNPM_DMA_REG_TX_DESC_BUF_HEAD(
							tx_ring->rnpm_queue_idx));
					usleep_range(10000, 20000);
					timeout++;
				}
				if (timeout >= 1000) {
					rnpm_dbg(
						"[%s] Wait Rx-ring %d head to zero time out\n",
						netdev->name,
						tx_ring->rnpm_queue_idx);
				}
				tx_ring->next_to_use = next_to_use;
			}
		}
	}

	if (!pci_channel_offline(adapter->pdev))
		if (!(adapter->flags & RNPM_FLAG_SRIOV_ENABLED) &&
		    (!(adapter->flags & RNPM_FLAG_MUTIPORT_ENABLED)))
			rnpm_reset(adapter);

	rnpm_clean_all_tx_rings(adapter);
	rnpm_clean_all_rx_rings(adapter);

	if (hw->ncsi_en)
		rnpm_control_mac_rx(adapter, true);
	rnpm_logd(LOG_FUNC_ENTER, "exit %s %s\n", __func__,
		  adapter->netdev->name);
}

/**
 * rnpm_tx_timeout - Respond to a Tx Hang
 * @netdev: network interface device structure
 **/
static void rnpm_tx_timeout(struct net_device *netdev,
			    unsigned int txqueue)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	bool real_tx_hang = false;
	int i;

#define TX_TIMEO_LIMIT 16000
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct rnpm_ring *tx_ring = adapter->tx_ring[i];

		if (check_for_tx_hang(tx_ring) &&
		    rnpm_check_tx_hang(tx_ring))
			real_tx_hang = true;
	}

	if (real_tx_hang) {
		/* Do the reset outside of interrupt context */
		e_info(drv, "tx real hang\n");
		rnpm_tx_timeout_reset(adapter);
	} else {
		e_info(drv,
		       "Fake Tx hang detected with timeout of %d seconds\n",
		       netdev->watchdog_timeo / HZ);
		/* fake Tx hang - increase the kernel timeout */
		if (netdev->watchdog_timeo < TX_TIMEO_LIMIT)
			netdev->watchdog_timeo *= 2;
	}
}

/**
 * rnpm_sw_init - Initialize general software structures (struct rnpm_adapter)
 * @adapter: board private structure to initialize
 *
 * rnpm_sw_init initializes the Adapter private data structure.
 * Fields are initialized based on PCI device information and
 * OS network device settings (MTU size).
 **/
static int rnpm_sw_init(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	struct pci_dev *pdev = adapter->pdev;
	unsigned int rss = 0, fdir;
	int i;

	hw->vendor_id = pdev->vendor;
	hw->device_id = pdev->device;
	hw->subsystem_vendor_id = pdev->subsystem_vendor;
	hw->subsystem_device_id = pdev->subsystem_device;

	/* Set common capability flags and settings */
	rss = min_t(int, adapter->max_ring_pair_counts, num_online_cpus());
#ifdef RNPM_DEFAULT_RINGS_CNT
	rss = min_t(int, rss, RNPM_DEFAULT_RINGS_CNT);
#endif
	/* limit queue since cpu maybe large than vectors number */
	rss = min_t(int, rss, adapter->max_msix_counts);
	adapter->ring_feature[RING_F_RSS].limit =
		min_t(int, rss, adapter->max_ring_pair_counts);
	adapter->atr_sample_rate = 20;

#ifdef RNPM_MAX_RINGS
	fdir = min_t(int, 32, RNPM_MAX_RINGS);
#else
	fdir = min_t(int, 32, num_online_cpus());
#endif
	adapter->ring_feature[RING_F_FDIR].limit = fdir;
	if (hw->feature_flags & RNPM_NET_FEATURE_RX_NTUPLE_FILTER) {
		spin_lock_init(&adapter->fdir_perfect_lock);
		/* init count record */
		adapter->fdir_filter_count = 0;
		adapter->layer2_count = 0;
		adapter->tuple_5_count = 0;
		if (hw->feature_flags & RNPM_NET_FEATURE_TCAM)
			adapter->fdir_mode = fdir_mode_tcam;
		else
			adapter->fdir_mode = fdir_mode_tuple5;

		adapter->fdir_pballoc = adapter->layer2_count_max +
					adapter->tuple_5_count_max;
	}
	/* itr sw setup here */
	adapter->sample_interval = RNPM_DEFAULT_SAMPLE_INTERVAL;
	adapter->adaptive_rx_coal = RNPM_DEFAULT_ENABLE;
	adapter->adaptive_tx_coal = RNPM_DEFAULT_ENABLE;
	adapter->auto_rx_coal = RNPM_DEFAULT_DISABLE;
	adapter->napi_budge = RNPM_DEFAULT_NAPI_BUDGE;

	/* set default work limits */
	adapter->tx_work_limit =
		rnpm_info_tbl[adapter->pf_adapter->board_type]
			->coalesce.tx_work_limit;
	adapter->rx_usecs = rnpm_info_tbl[adapter->pf_adapter->board_type]
				    ->coalesce.rx_usecs;
	adapter->rx_frames = rnpm_info_tbl[adapter->pf_adapter->board_type]
				     ->coalesce.rx_frames;
	adapter->tx_usecs = rnpm_info_tbl[adapter->pf_adapter->board_type]
				    ->coalesce.tx_usecs;
	adapter->tx_frames = rnpm_info_tbl[adapter->pf_adapter->board_type]
				     ->coalesce.tx_frames;
	if (rnpm_info_tbl[adapter->pf_adapter->board_type]->mac_padding)
		adapter->priv_flags |= RNPM_PRIV_FLAG_TX_PADDING;

	/* Set MAC specific capability flags and exceptions */
	/* port capability is set here */
	switch (hw->mode) {
	case MODE_NIC_MODE_1PORT_40G:
	case MODE_NIC_MODE_1PORT:
		adapter->uc_num = hw->mac.num_rar_entries;
		adapter->uc_off = 0;
		break;
		/* multiple ports use mac */
	case MODE_NIC_MODE_2PORT:
	case MODE_NIC_MODE_4PORT:
		adapter->uc_num = hw->mac.num_rar_entries / 4;
		adapter->uc_off = adapter->uc_num * adapter->port;
		break;
	default:
		break;
	}

#ifdef CONFIG_PCI_IOV
	adapter->num_vfs =
		(max_vfs > (RNPM_MAX_VF_FUNCTIONS - 1)) ? 0 : max_vfs;
#endif
	adapter->tx_ring_item_count =
		rnpm_info_tbl[adapter->pf_adapter->board_type]->queue_depth;
	adapter->rx_ring_item_count =
		rnpm_info_tbl[adapter->pf_adapter->board_type]->queue_depth;
	/*initialization default pause flow */
	hw->fc.requested_mode = rnpm_fc_full;
	hw->fc.pause_time = RNPM_DEFAULT_FCPAUSE;
	hw->fc.current_mode = rnpm_fc_full;
	for (i = 0; i < RNPM_MAX_TRAFFIC_CLASS; i++) {
		hw->fc.high_water[i] = RNPM_DEFAULT_HIGH_WATER;
		hw->fc.low_water[i] = RNPM_DEFAULT_LOW_WATER;
	}
	set_bit(__RNPM_DOWN, &adapter->state);
	return 0;
}

/**
 * rnpm_setup_tx_resources - allocate Tx resources (Descriptors)
 * @tx_ring:    tx descriptor ring (for a specific queue) to setup
 *
 * Return 0 on success, negative on failure
 **/

int rnpm_setup_tx_resources(struct rnpm_ring *tx_ring,
			    struct rnpm_adapter *adapter)
{
	struct device *dev = tx_ring->dev;
	int orig_node = dev_to_node(dev);
	int numa_node = NUMA_NO_NODE;
	int size;

	size = sizeof(struct rnpm_tx_buffer) * tx_ring->count;
	if (tx_ring->q_vector)
		numa_node = tx_ring->q_vector->numa_node;
	tx_ring->tx_buffer_info = vzalloc_node(size, numa_node);
	if (!tx_ring->tx_buffer_info)
		tx_ring->tx_buffer_info = vzalloc(size);
	if (!tx_ring->tx_buffer_info)
		goto err;
	/* round up to nearest 4K */
	tx_ring->size = tx_ring->count * sizeof(struct rnpm_tx_desc);
	tx_ring->size = ALIGN(tx_ring->size, 4096);

	set_dev_node(dev, numa_node);
	tx_ring->desc = dma_alloc_coherent(dev, tx_ring->size,
					   &tx_ring->dma, GFP_KERNEL);
	set_dev_node(dev, orig_node);
	if (!tx_ring->desc)
		tx_ring->desc = dma_alloc_coherent(
			dev, tx_ring->size, &tx_ring->dma, GFP_KERNEL);
	if (!tx_ring->desc)
		goto err;
	memset(tx_ring->desc, 0, tx_ring->size);

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;

	DPRINTK(IFUP, INFO,
		"TxRing:%d vector:%d ItemCounts:%d desc:%p(0x%llx) node:%d\n",
		tx_ring->rnpm_queue_idx, tx_ring->q_vector->v_idx,
		tx_ring->count, tx_ring->desc, tx_ring->dma, numa_node);
	return 0;
err:
	netdev_err(
		adapter->netdev,
		"%s [SetupTxResources] #%d TxRing:%d vector:%d ItemCounts:%d\n",
		tx_ring->netdev->name, tx_ring->queue_index,
		tx_ring->rnpm_queue_idx, tx_ring->q_vector->v_idx,
		tx_ring->count);
	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;
	dev_err(dev,
		"Unable to allocate memory for the Tx descriptor ring\n");
	return -ENOMEM;
}

/**
 * rnpm_setup_all_tx_resources - allocate all queues Tx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int rnpm_setup_all_tx_resources(struct rnpm_adapter *adapter)
{
	int i, err = 0;

	tx_dbg("adapter->num_tx_queues:%d, adapter->tx_ring[0]:%p\n",
	       adapter->num_tx_queues, adapter->tx_ring[0]);
	for (i = 0; i < (adapter->num_tx_queues); i++) {
		BUG_ON(adapter->tx_ring[i] == NULL);
		err = rnpm_setup_tx_resources(adapter->tx_ring[i],
					      adapter);
		if (!err)
			continue;

		e_err(probe, "Allocation for Tx Queue %u failed\n", i);
		goto err_setup_tx;
	}
	return 0;
err_setup_tx:
	/* rewind the index freeing the rings as we go */
	while (i--)
		rnpm_free_tx_resources(adapter->tx_ring[i]);
	return err;
}

/**
 * rnpm_setup_rx_resources - allocate Rx resources (Descriptors)
 * @rx_ring:    rx descriptor ring (for a specific queue) to setup
 *
 * Returns 0 on success, negative on failure
 **/
int rnpm_setup_rx_resources(struct rnpm_ring *rx_ring,
			    struct rnpm_adapter *adapter)
{
	struct device *dev = rx_ring->dev;
	int orig_node = dev_to_node(dev);
	int numa_node = -1;
	int size;

	BUG_ON(rx_ring == NULL);
	size = sizeof(struct rnpm_rx_buffer) * rx_ring->count;
	if (rx_ring->q_vector)
		numa_node = rx_ring->q_vector->numa_node;

	rx_ring->rx_buffer_info = vzalloc_node(size, numa_node);
	if (!rx_ring->rx_buffer_info)
		rx_ring->rx_buffer_info = vzalloc(size);
	if (!rx_ring->rx_buffer_info)
		goto err;
	/* Round up to nearest 4K */
	rx_ring->size = rx_ring->count * sizeof(union rnpm_rx_desc);
	rx_ring->size = ALIGN(rx_ring->size, 4096);

	set_dev_node(dev, numa_node);
	rx_ring->desc = dma_alloc_coherent(dev, rx_ring->size,
					   &rx_ring->dma, GFP_KERNEL);
	set_dev_node(dev, orig_node);
	if (!rx_ring->desc)
		rx_ring->desc = dma_alloc_coherent(
			dev, rx_ring->size, &rx_ring->dma, GFP_KERNEL);
	if (!rx_ring->desc)
		goto err;
	memset(rx_ring->desc, 0, rx_ring->size);
	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;

	DPRINTK(IFUP, INFO,
		"RxRing:%d, vector:%d ItemCounts:%d desc:%p(0x%llx) node:%d\n",
		rx_ring->rnpm_queue_idx, rx_ring->q_vector->v_idx,
		rx_ring->count, rx_ring->desc, rx_ring->dma, numa_node);

	return 0;
err:
	netdev_err(
		adapter->netdev,
		"%s [SetupRxResources] %d RxRing:%d, vector:%d ItemCounts:%d!\n",
		rx_ring->netdev->name, rx_ring->queue_index,
		rx_ring->rnpm_queue_idx, rx_ring->q_vector->v_idx,
		rx_ring->count);

	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;
	dev_err(dev,
		"Unable to allocate memory for the Rx descriptor ring\n");
	return -ENOMEM;
}

/**
 * rnpm_setup_all_rx_resources - allocate all queues Rx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int rnpm_setup_all_rx_resources(struct rnpm_adapter *adapter)
{
	int i, err = 0;
	struct rnpm_hw *hw = &adapter->hw;
	u32 head;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		BUG_ON(adapter->rx_ring[i] == NULL);
		head = rd32(hw,
			    RNPM_DMA_REG_RX_DESC_BUF_HEAD(
				    adapter->rx_ring[i]->rnpm_queue_idx));
		if (unlikely(head >= adapter->rx_ring[i]->count)) {
			dbg_apt("[%s] Ring %d head large than count",
				adapter->netdev->name,
				adapter->rx_ring[i]->rnpm_queue_idx);
			adapter->rx_ring[i]->ring_flags |=
				RNPM_RING_FLAG_DELAY_SETUP_RX_LEN;
			adapter->rx_ring[i]->reset_count =
				adapter->rx_ring[i]->count;
			adapter->rx_ring[i]->count = head + 1;
		}
		err = rnpm_setup_rx_resources(adapter->rx_ring[i],
					      adapter);
		if (!err)
			continue;

		e_err(probe, "Allocation for Rx Queue %u failed\n", i);
		goto err_setup_rx;
	}
	return 0;
err_setup_rx:
	while (i--)
		rnpm_free_rx_resources(adapter->rx_ring[i]);
	return err;
}

/**
 * rnpm_free_tx_resources - Free Tx Resources per Queue
 * @tx_ring: Tx descriptor ring for a specific queue
 *
 * Free all transmit software resources
 **/
void rnpm_free_tx_resources(struct rnpm_ring *tx_ring)
{
	WARN_ON(tx_ring == NULL);
	rnpm_clean_tx_ring(tx_ring);
	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;

	if (!tx_ring->desc)
		return;
	dma_free_coherent(tx_ring->dev, tx_ring->size, tx_ring->desc,
			  tx_ring->dma);
	tx_ring->desc = NULL;
}

/**
 * rnpm_free_all_tx_resources - Free Tx Resources for All Queues
 * @adapter: board private structure
 *
 * Free all transmit software resources
 **/
static void rnpm_free_all_tx_resources(struct rnpm_adapter *adapter)
{
	int i;

	for (i = 0; i < (adapter->num_tx_queues); i++)
		rnpm_free_tx_resources(adapter->tx_ring[i]);
}

/**
 * rnpm_free_rx_resources - Free Rx Resources
 * @rx_ring: ring to clean the resources from
 *
 * Free all receive software resources
 **/
void rnpm_free_rx_resources(struct rnpm_ring *rx_ring)
{
	WARN_ON(rx_ring == NULL);
	rnpm_clean_rx_ring(rx_ring);
	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;
	if (!rx_ring->desc)
		return;
	dma_free_coherent(rx_ring->dev, rx_ring->size, rx_ring->desc,
			  rx_ring->dma);
	rx_ring->desc = NULL;
}

/**
 * rnpm_free_all_rx_resources - Free Rx Resources for All Queues
 * @adapter: board private structure
 *
 * Free all receive software resources
 **/
static void rnpm_free_all_rx_resources(struct rnpm_adapter *adapter)
{
	int i;

	for (i = 0; i < (adapter->num_rx_queues); i++)
		rnpm_free_rx_resources(adapter->rx_ring[i]);
}

/**
 * rnpm_change_mtu - Change the Maximum Transfer Unit
 * @netdev: network interface device structure
 * @new_mtu: new value for maximum frame size
 *
 * Returns 0 on success, negative on failure
 **/
static int rnpm_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_pf_adapter *pf_adapter = adapter->pf_adapter;
	int max_frame = new_mtu + ETH_HLEN + 2 * ETH_FCS_LEN;

	/* MTU < 68 is an error and causes problems on some kernels */
	if ((new_mtu < RNPM_MIN_MTU) ||
	    (max_frame > RNPM_MAX_JUMBO_FRAME_SIZE))
		return -EINVAL;
	e_info(probe, "changing MTU from %d to %d\n", netdev->mtu,
	       new_mtu);
	/* must set new MTU before calling down or up */
	netdev->mtu = new_mtu;
	set_bit(RNPM_PF_SET_MTU, &pf_adapter->flags);
	if (netif_running(netdev))
		rnpm_reinit_locked(adapter);

	return 0;
}

/**
 * rnpm_tx_maxrate - callback to set the maximum per-queue bitrate
 * @netdev: network interface device structure
 * @queue_index: Tx queue to set
 * @maxrate: desired maximum transmit bitrate Mbps
 **/
static int rnpm_tx_maxrate(struct net_device *netdev, int queue_index,
			   u32 maxrate)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_ring *tx_ring = adapter->tx_ring[queue_index];
	u64 real_rate = 0;

	adapter->max_rate[queue_index] = maxrate;
	if (!maxrate)
		return rnpm_setup_tx_maxrate(
			adapter->hw.hw_addr, tx_ring, 0,
			adapter->hw.usecstocount * 1000000);
	/* we need turn it to bytes/s */
	real_rate = ((u64)maxrate * 1024 * 128) * 90 / 100;
	rnpm_setup_tx_maxrate(adapter->hw.hw_addr, tx_ring, real_rate,
			      adapter->hw.usecstocount * 1000000);

	return 0;
}

/**
 * rnpm_open - Called when a network interface is made active
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
int rnpm_open(struct net_device *netdev)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int err;
	unsigned long flags;

	DPRINTK(IFUP, INFO, "ifup\n");
	rnpm_logd(LOG_FUNC_ENTER, "enter %s %s\n", __func__, netdev->name);

	/* disallow open during test */
	if (test_bit(__RNPM_TESTING, &adapter->state))
		return -EBUSY;
	netif_carrier_off(netdev);
	/* allocate transmit descriptors */
	err = rnpm_setup_all_tx_resources(adapter);
	if (err)
		goto err_setup_tx;
	/* allocate receive descriptors */
	err = rnpm_setup_all_rx_resources(adapter);
	if (err)
		goto err_setup_rx;
	rnpm_configure(adapter);
	err = rnpm_request_irq(adapter);
	if (err)
		goto err_req_irq;
	/* Notify the stack of the actual queue counts. */
	err = netif_set_real_num_tx_queues(netdev, adapter->num_tx_queues);
	if (err)
		goto err_set_queues;
	err = netif_set_real_num_rx_queues(netdev, adapter->num_rx_queues);
	if (err)
		goto err_set_queues;
#ifndef NO_PTP
	if (module_enable_ptp)
		rnpm_ptp_register(adapter);
#endif
	rnpm_up_complete(adapter);
	/* set sw dummy 0,  wait fw link to force one interrupt */
	rnpm_link_stat_mark(hw, hw->nr_lane, 0);
	spin_lock_irqsave(&adapter->pf_adapter->pf_setup_lock, flags);
	set_bit(RNPM_PF_LINK_CHANGE, &adapter->pf_adapter->flags);
	spin_unlock_irqrestore(&adapter->pf_adapter->pf_setup_lock, flags);
	rnpm_logd(LOG_FUNC_ENTER, "exit %s %s\n", __func__,
		  adapter->netdev->name);

	return 0;

err_set_queues:
	rnpm_free_irq(adapter);
err_req_irq:
	rnpm_free_all_rx_resources(adapter);
err_setup_rx:
	rnpm_free_all_tx_resources(adapter);
err_setup_tx:
	rnpm_mbx_ifup_down(&adapter->hw, MBX_IFDOWN);
	rnpm_reset(adapter);
	e_err(drv, "open faild!\n");

	return err;
}

/**
 * rnpm_close - Disables a network interface
 * @netdev: network interface device structure
 *
 * Returns 0, this is not allowed to fail
 *
 * The close entry point is called when an interface is de-activated
 * by the OS.  The hardware is still under the drivers control, but
 * needs to be disabled.  A global MAC reset is issued to stop the
 * hardware, and all transmit and receive resources are freed.
 **/
int rnpm_close(struct net_device *netdev)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	unsigned long flags;

	DPRINTK(IFDOWN, INFO, "ifdown\n");
#ifdef DISABLE_RX_IRQ
	adapter->quit_poll_thread = true;
#endif
	/* should clean adapter->ptp_tx_skb */
	if (adapter->ptp_tx_skb) {
		dev_kfree_skb_any(adapter->ptp_tx_skb);
		adapter->ptp_tx_skb = NULL;
		adapter->tx_hwtstamp_timeouts++;
		netdev_warn(adapter->netdev,
			    "clearing Tx timestamp hang\n");
	}

#ifndef NO_PTP
	if (module_enable_ptp)
		rnpm_ptp_unregister(adapter);
#endif

	rnpm_down(adapter);
	rnpm_free_irq(adapter);
	rnpm_fdir_filter_exit(adapter);
	rnpm_free_all_tx_resources(adapter);
	rnpm_free_all_rx_resources(adapter);
	rnpm_mbx_ifup_down(&adapter->hw, MBX_IFDOWN);
	/* when down, disable fw link event interrupt */
	rnpm_link_stat_mark(&adapter->hw, adapter->hw.nr_lane, 0);
	spin_lock_irqsave(&adapter->pf_adapter->pf_setup_lock, flags);
	set_bit(RNPM_PF_SERVICE_SKIP_HANDLE, &adapter->pf_adapter->flags);
	set_bit(RNPM_PF_LINK_CHANGE, &adapter->pf_adapter->flags);
	spin_unlock_irqrestore(&adapter->pf_adapter->pf_setup_lock, flags);
	return 0;
}

/**
 * rnpm_update_stats - Update the board statistics counters.
 * @adapter: board private structure
 **/
void rnpm_update_stats(struct rnpm_adapter *adapter)
{
	struct net_device_stats *net_stats = &adapter->netdev->stats;
	struct rnpm_hw *hw = &adapter->hw;
	struct rnpm_hw_stats *hw_stats = &adapter->hw_stats;
	int i, port = adapter->port;
	struct rnpm_ring *ring;
	u64 hw_csum_rx_error = 0;
	u64 hw_csum_rx_good = 0;
	u64 rx_crc_error = 0;

	net_stats->tx_packets = 0;
	net_stats->tx_bytes = 0;
	net_stats->rx_packets = 0;
	net_stats->rx_bytes = 0;

	hw_stats->vlan_strip_cnt = 0;
	hw_stats->vlan_add_cnt = 0;

	if (test_bit(__RNPM_DOWN, &adapter->state) ||
	    test_bit(__RNPM_RESETTING, &adapter->state))
		return;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		rnpm_for_each_ring(ring, adapter->q_vector[i]->rx) {
			net_stats->rx_packets += ring->stats.packets;
			net_stats->rx_bytes += ring->stats.bytes;
			hw_csum_rx_error += ring->rx_stats.csum_err;
			hw_csum_rx_good += ring->rx_stats.csum_good;
			hw_stats->vlan_strip_cnt +=
				ring->rx_stats.vlan_remove;
		}
		rnpm_for_each_ring(ring, adapter->q_vector[i]->tx) {
			net_stats->tx_packets += ring->stats.packets;
			net_stats->tx_bytes += ring->stats.bytes;
			hw_stats->vlan_add_cnt += ring->tx_stats.vlan_add;
		}
	}

	switch (hw->mode) {
	case MODE_NIC_MODE_1PORT_40G:
	case MODE_NIC_MODE_1PORT:
		hw_stats->dma_to_eth =
			rd32(hw, RNPM_DMA_STATS_DMA_TO_DMA_CHANNEL_0) +
			rd32(hw, RNPM_DMA_STATS_DMA_TO_DMA_CHANNEL_1) +
			rd32(hw, RNPM_DMA_STATS_DMA_TO_DMA_CHANNEL_2) +
			rd32(hw, RNPM_DMA_STATS_DMA_TO_DMA_CHANNEL_3);
		break;
	case MODE_NIC_MODE_2PORT:
		hw_stats->dma_to_eth = 0;
		for (i = port * 2; i < (port + 1) * 2; i++) {
			hw_stats->dma_to_eth += rd32(
				hw, RNPM_DMA_STATS_DMA_TO_DMA_CHANNEL(i));
		}
		break;
	case MODE_NIC_MODE_4PORT:
		hw_stats->dma_to_eth =
			rd32(hw, RNPM_DMA_STATS_DMA_TO_DMA_CHANNEL(port));
		break;
	}

	hw_stats->dma_to_switch = rd32(hw, RNPM_DMA_STATS_DMA_TO_SWITCH);
	hw_stats->mac_to_dma = rd32(hw, RNPM_DMA_STATS_MAC_TO_DMA);

	rx_crc_error = rnpm_recalculate_err_pkts(
		rd32(hw, RNPM_RXTRANS_CRC_ERR_PKTS(port)),
		&(hw->err_pkts_init.crc[port]), false);
	net_stats->rx_crc_errors += rx_crc_error;
	net_stats->rx_errors +=
		rnpm_recalculate_err_pkts(
			rd32(hw, RNPM_RXTRANS_WDT_ERR_PKTS(port)),
			&hw->err_pkts_init.wdt[port], false) +
		rnpm_recalculate_err_pkts(
			rd32(hw, RNPM_RXTRANS_CODE_ERR_PKTS(port)),
			&hw->err_pkts_init.code[port], false) +
		rnpm_recalculate_err_pkts(
			rd32(hw, RNPM_RXTRANS_SLEN_ERR_PKTS(port)),
			&hw->err_pkts_init.slen[port], false) +
		rnpm_recalculate_err_pkts(
			rd32(hw, RNPM_RXTRANS_GLEN_ERR_PKTS(port)),
			&hw->err_pkts_init.glen[port], false) +
		rnpm_recalculate_err_pkts(
			rd32(hw, RNPM_RXTRANS_IPH_ERR_PKTS(port)),
			&hw->err_pkts_init.iph[port], false) +
		rnpm_recalculate_err_pkts(
			rd32(hw, RNPM_RXTRANS_LEN_ERR_PKTS(port)),
			&hw->err_pkts_init.len[port], false) +
		rnpm_recalculate_err_pkts(
			rd32(hw, RNPM_RXTRANS_CSUM_ERR_PKTS(port)),
			&hw->err_pkts_init.csum[port], false) +
		rnpm_recalculate_err_pkts(hw_csum_rx_error,
					  &hw->err_pkts_init.scsum[port],
					  true) +
		rx_crc_error;
	net_stats->rx_dropped +=
		rnpm_recalculate_err_pkts(
			rd32(hw, RNPM_RXTRANS_CUT_ERR_PKTS(port)),
			&hw->err_pkts_init.cut[port], false) +
		rnpm_recalculate_err_pkts(
			rd32(hw, RNPM_RXTRANS_DROP_PKTS(port)),
			&hw->err_pkts_init.drop[port], false);
	adapter->hw_csum_rx_error = hw_csum_rx_error;
	adapter->hw_csum_rx_good = hw_csum_rx_good;

	hw_stats->mac_rx_broadcast =
		rd32(hw, RNPM_MAC_STATS_BROADCAST_LOW(port));
	hw_stats->mac_rx_broadcast +=
		((u64)rd32(hw, RNPM_MAC_STATS_BROADCAST_HIGH(port)) << 32);

	hw_stats->mac_rx_multicast =
		rd32(hw, RNPM_MAC_STATS_MULTICAST_LOW(port));
	hw_stats->mac_rx_multicast +=
		((u64)rd32(hw, RNPM_MAC_STATS_MULTICAST_HIGH(port)) << 32);
	net_stats->multicast = hw_stats->mac_rx_multicast;
	hw_stats->mac_tx_pause_cnt =
		rd32(hw, RNPM_MAC_STATS_TX_PAUSE_LOW(port));
	hw_stats->mac_tx_pause_cnt +=
		((u64)rd32(hw, RNPM_MAC_STATS_TX_PAUSE_HIGH(port)) << 32);

	hw_stats->mac_rx_pause_cnt =
		rd32(hw, RNPM_MAC_STATS_RX_PAUSE_LOW(port));
	hw_stats->mac_rx_pause_cnt +=
		((u64)rd32(hw, RNPM_MAC_STATS_RX_PAUSE_HIGH(port)) << 32);
}

/**
 * rnpm_check_hang_subtask - check for hung queues and dropped interrupts
 * @adapter: pointer to the device adapter structure
 *
 * This function serves two purposes.  First it strobes the interrupt lines
 * in order to make certain interrupts are occurring.  Secondly it sets the
 * bits needed to check for TX hangs.  As a result we should immediately
 * determine if a hang has occurred.
 */
static void rnpm_check_hang_subtask(struct rnpm_adapter *adapter)
{
	struct rnpm_ring *tx_ring;
	u64 tx_next_to_clean_old;
	u64 tx_next_to_clean;
	u64 tx_next_to_use;
	struct rnpm_ring *rx_ring;
	u64 rx_next_to_clean_old;
	u64 rx_next_to_clean;
	union rnpm_rx_desc *rx_desc;
	struct rnpm_q_vector *q_vector;
	int i, size;

	if (test_bit(__RNPM_DOWN, &adapter->state) ||
	    test_bit(__RNPM_RESETTING, &adapter->state))
		return;

	/* Force detection of hung controller */
	if (netif_carrier_ok(adapter->netdev))
		for (i = 0; i < adapter->num_tx_queues; i++)
			set_check_for_tx_hang(adapter->tx_ring[i]);

	for (i = 0; i < adapter->num_tx_queues; i++) {
		tx_ring = adapter->tx_ring[i];
		/* get the last next_to_clean */
		tx_next_to_clean_old = tx_ring->tx_stats.tx_next_to_clean;
		tx_next_to_clean = tx_ring->next_to_clean;
		tx_next_to_use = tx_ring->next_to_use;

		/* if have tx desc to clean */
		if (tx_next_to_use != tx_next_to_clean) {
			if (tx_next_to_clean == tx_next_to_clean_old) {
				tx_ring->tx_stats.tx_equal_count++;
				if (tx_ring->tx_stats.tx_equal_count > 2) {
					struct rnpm_q_vector *q_vector =
						tx_ring->q_vector;

					if (q_vector->rx.ring ||
					    q_vector->tx.ring) {
						rnpm_irq_disable_queues(
							q_vector);
						napi_schedule_irqoff(
							&q_vector->napi);
					}

					tx_ring->tx_stats.tx_irq_miss++;
					tx_ring->tx_stats.tx_equal_count =
						0;
				}
			} else {
				tx_ring->tx_stats.tx_equal_count = 0;
			}
			tx_ring->tx_stats.tx_next_to_clean =
				tx_next_to_clean;
		} else {
			tx_ring->tx_stats.tx_next_to_clean = -1;
		}
	}

	/* check if we lost rx irq */
	for (i = 0; i < adapter->num_rx_queues; i++) {
		rx_ring = adapter->rx_ring[i];
		rx_next_to_clean_old = rx_ring->rx_stats.rx_next_to_clean;
		rx_next_to_clean = rx_ring->next_to_clean;

		if (rx_next_to_clean == rx_next_to_clean_old) {
			rx_ring->rx_stats.rx_equal_count++;

			if ((rx_ring->rx_stats.rx_equal_count > 2) &&
			    (rx_ring->rx_stats.rx_equal_count < 5)) {
				/* check if dd in the clean rx desc */
				rx_desc = RNPM_RX_DESC(
					rx_ring, rx_ring->next_to_clean);
				if (!rnpm_test_staterr(rx_desc,
						       RNPM_RXD_STAT_DD))
					goto skip;
				q_vector = rx_ring->q_vector;
				size = le16_to_cpu(rx_desc->wb.len);
				if (!size)
					goto skip;
				rx_ring->rx_stats.rx_irq_miss++;
				if (q_vector->rx.ring ||
				    q_vector->tx.ring) {
					rnpm_irq_disable_queues(q_vector);
					napi_schedule_irqoff(
						&q_vector->napi);
				}
			}
skip:
			if (rx_ring->rx_stats.rx_equal_count > 1000)
				rx_ring->rx_stats.rx_equal_count = 0;
		} else {
			rx_ring->rx_stats.rx_equal_count = 0;
		}
		rx_ring->rx_stats.rx_next_to_clean = rx_next_to_clean;
	}
}

static int rnpm_pf_get_port_link_stat(struct rnpm_pf_adapter *pf_adapter)
{
	struct rnpm_hw *hw;
	int err = 0, i;

	for (i = 0; i < pf_adapter->adapter_cnt; i++) {
		if (rnpm_port_is_valid(pf_adapter, i)) {
			if (pf_adapter->adapter[i]) {
				hw = &pf_adapter->adapter[i]->hw;
				if (rnpm_mbx_get_lane_stat(hw) < 0)
					goto error;
				if (hw->phy_type == PHY_TYPE_SGMII) {
					err = rnpm_mbx_phy_read(
						hw, 0, &hw->phy.vb_r[0]);
					if (err)
						goto error;
					hw->phy.an =
						(hw->phy.vb_r[0] &
						 BIT(12)) ?
							AUTONEG_ENABLE :
							AUTONEG_DISABLE;
					err = rnpm_mbx_phy_read(
						hw, 4, &hw->phy.vb_r[4]);
					if (err)
						goto error;
					err = rnpm_mbx_phy_read(
						hw, 9, &hw->phy.vb_r[9]);
					if (err)
						goto error;
					err = rnpm_mbx_phy_read(
						hw, 17, &hw->phy.vb_r[17]);
					if (err)
						goto error;
					hw->phy.is_mdix = !!(
						hw->phy.vb_r[17] & 0x0040);
				}
			}
		}
	}
error:
	return err;
}

/**
 * rnpm_watchdog_update_link - update the link status
 * @adapter: pointer to the device adapter structure
 * @link_speed: pointer to a u32 to store the link_speed
 **/
static int rnpm_watchdog_update_link(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;
	u32 link_speed = RNPM_LINK_SPEED_UNKNOWN;
	bool link_up = false;

	if (!(adapter->flags & RNPM_FLAG_NEED_LINK_UPDATE))
		return 1;

	/* Need update port link state */
	if (rnpm_pf_get_port_link_stat(adapter->pf_adapter) < 0) {
		set_bit(RNPM_PF_LINK_CHANGE, &adapter->pf_adapter->flags);
		return 1;
	}

	if (hw->mac.ops.check_link) {
		hw->mac.ops.check_link(hw, &link_speed, &link_up, false);
	} else {
		/* always assume link is up, if no check link function */
		link_speed = RNPM_LINK_SPEED_10GB_FULL;
		link_up = true;
		rnpm_logd(
			LOG_LINK_EVENT,
			"WARN: %s:%s: check_link is null, force speed/link\n",
			__func__, adapter->netdev->name);
	}

	if (link_up || time_after(jiffies, (adapter->link_check_timeout +
					    RNPM_TRY_LINK_TIMEOUT)))
		adapter->flags &= ~RNPM_FLAG_NEED_LINK_UPDATE;
	adapter->link_up = link_up;
	adapter->link_speed = link_speed;
	if (adapter->is_mac_loopback) {
		adapter->link_up = 1;
		rnpm_mbx_ifup_down(&adapter->hw, MBX_IFUP);
		if (hw->is_sgmii)
			hw->mac.ops.power_down(hw, true);
	}

	return 0;
}

/**
 * rnpm_watchdog_link_is_up - update netif_carrier status and
 *                             print link up message
 * @adapter: pointer to the device adapter structure
 **/
static void rnpm_watchdog_link_is_up(struct rnpm_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct rnpm_hw *hw = &adapter->hw;
	u32 link_speed = adapter->link_speed;
	bool flow_rx = false, flow_tx = false;

	if (adapter->is_mac_loopback)
		rnpm_link_stat_mark(hw, hw->nr_lane, 0);
	else
		rnpm_link_stat_mark(hw, hw->nr_lane, 1);

	/* only continue if link was previously down */
	if (netif_carrier_ok(netdev))
		return;

	hw->mac.ops.fc_enable(hw);
	adapter->flags2 &= ~RNPM_FLAG2_SEARCH_FOR_SFP;
	if (hw->fc.current_mode == rnpm_fc_rx_pause) {
		flow_rx = true;
	} else if (hw->fc.current_mode == rnpm_fc_tx_pause) {
		flow_tx = true;
	} else if (hw->fc.current_mode == rnpm_fc_full) {
		flow_rx = true;
		flow_tx = true;
	}

	e_info(drv, "NIC Link is Up %s, Flow Control: %s\n",
	       (link_speed == RNPM_LINK_SPEED_10GB_FULL ?
			"10 Gbps" :
			(link_speed == RNPM_LINK_SPEED_1GB_FULL ?
				 "1 Gbps" :
				 (link_speed == RNPM_LINK_SPEED_100_FULL ?
					  "100 Mbps" :
					  (link_speed == RNPM_LINK_SPEED_10_FULL ?
						   "10 Mbps" :
						   "unknown speed")))),
	       ((flow_rx && flow_tx) ?
			"RX/TX" :
			(flow_rx ? "RX" : (flow_tx ? "TX" : "None"))));

	netif_carrier_on(netdev);
	netif_tx_wake_all_queues(netdev);
	rnpm_control_mac_rx(adapter, true);
}

/**
 * rnpm_watchdog_link_is_down - update netif_carrier status and
 *                               print link down message
 * @adapter: pointer to the adapter structure
 **/
static void rnpm_watchdog_link_is_down(struct rnpm_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct rnpm_hw *hw = &adapter->hw;

	adapter->link_up = false;
	adapter->link_speed = 0;

	rnpm_link_stat_mark(hw, hw->nr_lane, 0);

	/* only continue if link was up previously */
	if (!netif_carrier_ok(netdev))
		return;
	rnpm_control_mac_rx(adapter, false);
	/* poll for SFP+ cable when link is down */
	if (rnpm_is_sfp(hw))
		adapter->flags2 |= RNPM_FLAG2_SEARCH_FOR_SFP;

	e_info(drv, "NIC Link is Down\n");
	netif_carrier_off(netdev);
	netif_tx_stop_all_queues(netdev);
}

/**
 * rnpm_watchdog_flush_tx - flush queues on link down
 * @adapter: pointer to the device adapter structure
 **/
__maybe_unused static void
rnpm_watchdog_flush_tx(struct rnpm_adapter *adapter)
{
	int i;
	int some_tx_pending = 0;

	if (!netif_carrier_ok(adapter->netdev)) {
		for (i = 0; i < adapter->num_tx_queues; i++) {
			struct rnpm_ring *tx_ring = adapter->tx_ring[i];

			if (tx_ring->next_to_use !=
			    tx_ring->next_to_clean) {
				some_tx_pending = 1;
				break;
			}
		}

		if (some_tx_pending) {
			/* We've lost link, so the controller stops DMA,
			 * but we've got queued Tx work that's never going
			 * to get done, so reset controller to flush Tx.
			 * (Do the reset outside of interrupt context).
			 */
			e_warn(drv,
			       "initiating reset to clear Tx work after link loss\n");
			set_bit(RNPM_PF_RESET,
				&adapter->pf_adapter->flags);
		}
	}
}

/**
 * rnpm_watchdog_subtask - check and bring link up
 * @adapter: pointer to the device adapter structure
 **/
static void rnpm_watchdog_subtask(struct rnpm_adapter *adapter)
{
	if (test_bit(__RNPM_DOWN, &adapter->state) ||
	    test_bit(__RNPM_RESETTING, &adapter->state))
		return;
	rnpm_update_stats(adapter);
	if (rnpm_watchdog_update_link(adapter))
		return;
	if ((adapter->link_up))
		rnpm_watchdog_link_is_up(adapter);
	else
		rnpm_watchdog_link_is_down(adapter);
}

/**
 * rnpm_sfp_link_config_subtask - set up link SFP after module install
 * @adapter: the rnpm adapter structure
 **/
static void rnpm_sfp_link_config_subtask(struct rnpm_adapter *adapter)
{
	if (!(adapter->flags & RNPM_FLAG_NEED_LINK_CONFIG))
		return;
}

/**
 * rnpm_pf_service_timer - Timer Call-back
 * @data: pointer to adapter cast into an unsigned long
 **/
static void rnpm_pf_service_timer(struct timer_list *t)
{
	struct rnpm_pf_adapter *pf_adapter =
		from_timer(pf_adapter, t, service_timer);
	unsigned long next_event_offset;

	next_event_offset = HZ * 2;
	pf_adapter->timer_count++;
	mod_timer(&pf_adapter->service_timer, next_event_offset + jiffies);
	rnpm_pf_service_event_schedule(pf_adapter);
}

/**
 * rnpm_service_timer - Timer Call-back
 * @data: pointer to adapter cast into an unsigned long
 **/
static void rnpm_service_timer(struct timer_list *t)
{
	struct rnpm_adapter *adapter =
		from_timer(adapter, t, service_timer);
	unsigned long next_event_offset;
	bool ready = true;

#ifdef RNPM_DISABLE_IRQ
	rnpm_mbx_get_link(&adapter->hw);
#endif
	if (adapter->flags & RNPM_FLAG_NEED_LINK_UPDATE)
		next_event_offset = HZ / 10;
	else
		next_event_offset = HZ * 2;
	adapter->timer_count++;
	mod_timer(&adapter->service_timer, next_event_offset + jiffies);
	if (ready)
		rnpm_service_event_schedule(adapter);
}

static void rnpm_fix_dma_tx_status(struct rnpm_pf_adapter *pf_adapter)
{
	int i;

	for (i = 0; i < 128; i++)
		wr32(pf_adapter, RNPM_DMA_TX_START(i), 1);
}

static int rnpm_reset_pf(struct rnpm_pf_adapter *pf_adapter)
{
	int times = 0;
	int i = 0;
	u32 status = 0;

	if (pci_channel_offline(pf_adapter->pdev))
		return -EIO;

	wr32(pf_adapter, RNPM_DMA_AXI_EN, 0);
#define TIMEOUT_COUNT (1000)
	/* wait for axi ready */
	while ((status != 0xf) && (times < TIMEOUT_COUNT)) {
		status = rd32(pf_adapter, RNPM_DMA_AXI_STAT);
		usleep_range(4000, 8000);
		times++;
	}
	if (!pf_adapter->hw.ncsi_en) {
		if (times >= TIMEOUT_COUNT) {
			dev_err(&pf_adapter->pdev->dev,
				"wait axi ready timeout\n");
			return -1;
		}
	}

	if (!(pf_adapter->hw.fw_version >= 0x0005070f &&
	      pf_adapter->hw.ncsi_en)) {
		wr32(pf_adapter, RNPM_TOP_NIC_REST_N, NIC_RESET);
		/* memory barrior */
		wmb();
		wr32(pf_adapter, RNPM_TOP_NIC_REST_N, ~NIC_RESET);
	}

	rnpm_mbx_fw_reset_phy(&pf_adapter->hw);
	/* global setup here */
	wr32(pf_adapter, RNPM_TOP_ETH_BUG_40G_PATCH, 1);
	wr32(pf_adapter, RNPM_ETH_TUNNEL_MOD, 1);

	/* set all rx drop */
	for (i = 0; i < 4; i++)
		wr32(pf_adapter, RNPM_ETH_RX_PROGFULL_THRESH_PORT(i),
		     DROP_ALL_THRESH);

	/* setup rss key */
	rnpm_init_rss_key(pf_adapter);
	/* tcam setup */
	if (pf_adapter->adapter_cnt == 1) {
		wr32(pf_adapter, RNPM_ETH_TCAM_EN, 1);
		wr32(pf_adapter, RNPM_TOP_ETH_TCAM_CONFIG_ENABLE, 1);
		wr32(pf_adapter, RNPM_TCAM_MODE, 2);
#define TCAM_NUM (4096)
		for (i = 0; i < TCAM_NUM; i++) {
			wr32(pf_adapter, RNPM_TCAM_SDPQF(i), 0);
			wr32(pf_adapter, RNPM_TCAM_DAQF(i), 0);
			wr32(pf_adapter, RNPM_TCAM_SAQF(i), 0);
			wr32(pf_adapter, RNPM_TCAM_APQF(i), 0);

			wr32(pf_adapter, RNPM_TCAM_SDPQF_MASK(i), 0);
			wr32(pf_adapter, RNPM_TCAM_DAQF_MASK(i), 0);
			wr32(pf_adapter, RNPM_TCAM_SAQF_MASK(i), 0);
			wr32(pf_adapter, RNPM_TCAM_APQF_MASK(i), 0);
		}
		wr32(pf_adapter, RNPM_TCAM_MODE, 1);
	}
	rnpm_fix_dma_tx_status(pf_adapter);
	wr32(pf_adapter, RNPM_ETH_DEFAULT_RX_MIN_LEN,
	     (min_rx_pkt_len < MIN_RX_PKT_LEN_LIMT) ? MIN_RX_PKT_LEN_LIMT :
						      min_rx_pkt_len);
	wr32(pf_adapter, RNPM_ETH_DEFAULT_RX_MAX_LEN, DEFAULT_MAX_SIZE);
	switch (pf_adapter->hw.mode) {
	case MODE_NIC_MODE_1PORT:
	case MODE_NIC_MODE_4PORT:
		wr32(pf_adapter, RNPM_ETH_TC_PORT_OFFSET_TABLE(0), 0);
		wr32(pf_adapter, RNPM_ETH_TC_PORT_OFFSET_TABLE(1), 1);
		wr32(pf_adapter, RNPM_ETH_TC_PORT_OFFSET_TABLE(2), 2);
		wr32(pf_adapter, RNPM_ETH_TC_PORT_OFFSET_TABLE(3), 3);

		break;
	case MODE_NIC_MODE_2PORT:
		wr32(pf_adapter, RNPM_ETH_TC_PORT_OFFSET_TABLE(0), 0);
		wr32(pf_adapter, RNPM_ETH_TC_PORT_OFFSET_TABLE(1), 2);
		break;
	}

	return 0;
}

__maybe_unused static void
wait_all_port_resetting(struct rnpm_pf_adapter *pf_adapter)
{
	struct rnpm_adapter *adapter;
	int i;

	for (i = 0; i < pf_adapter->adapter_cnt - 1; i++) {
		adapter = pf_adapter->adapter[i];
		while (test_and_set_bit(__RNPM_RESETTING, &adapter->state))
			usleep_range(1000, 2000);
	}
}

__maybe_unused static void
clean_all_port_resetting(struct rnpm_pf_adapter *pf_adapter)
{
	struct rnpm_adapter *adapter;
	int i;

	for (i = 0; i < pf_adapter->adapter_cnt - 1; i++) {
		adapter = pf_adapter->adapter[i];
		clear_bit(__RNPM_RESETTING, &adapter->state);
	}
}

static void rnpm_pf_mtu_subtask(struct rnpm_pf_adapter *pf_adapter)
{
	struct rnpm_adapter *adapter;
	struct net_device *netdev;
	int mtu = 0, i;

	for (i = pf_adapter->adapter_cnt - 1; i >= 0; i--) {
		adapter = pf_adapter->adapter[i];
		if (adapter) {
			netdev = adapter->netdev;

			if (mtu < netdev->mtu)
				mtu = netdev->mtu;
		}
	}
	mtu = mtu + ETH_HLEN + 2 * ETH_FCS_LEN;
	wr32(pf_adapter, RNPM_ETH_DEFAULT_RX_MAX_LEN, mtu);
}

static void rnpm_pf_reset_subtask(struct rnpm_pf_adapter *pf_adapter)
{
	struct rnpm_adapter *adapter;
	struct net_device *netdev;
	int err = 0;
	int i;

	while (test_and_set_bit(__RNPM_RESETTING, &pf_adapter->state)) {
		if (test_bit(__RNPM_REMOVING, &pf_adapter->state)) {
			clear_bit(__RNPM_RESETTING, &pf_adapter->state);
			return;
		}
		usleep_range(1000, 2000);
	}
	dev_err(&pf_adapter->pdev->dev, "rx/tx hang detected, reset pf\n");
	err = rnpm_reset_pf(pf_adapter);
	for (i = pf_adapter->adapter_cnt - 1; i >= 0; i--) {
		adapter = pf_adapter->adapter[i];
		if (!adapter)
			continue;

		netdev = adapter->netdev;
		rtnl_lock();
		netif_device_detach(netdev);
		if (netif_running(netdev)) {
			rnpm_down(adapter);
			rnpm_free_irq(adapter);
			rnpm_free_all_tx_resources(adapter);
			rnpm_free_all_rx_resources(adapter);
			rnpm_mbx_ifup_down(&adapter->hw, MBX_IFDOWN);
		}
		rnpm_clear_interrupt_scheme(adapter);
		rtnl_unlock();
	}

	for (i = 0; i < pf_adapter->adapter_cnt; i++) {
		adapter = pf_adapter->adapter[i];
		if (!adapter)
			continue;

		netdev = adapter->netdev;
		rtnl_lock();
		err = rnpm_init_interrupt_scheme(adapter);
		if (!err && netif_running(netdev))
			err = rnpm_open(netdev);

		netif_device_attach(netdev);
		rtnl_unlock();
	}
	clear_bit(__RNPM_RESETTING, &pf_adapter->state);
}

static void rnpm_reset_subtask(struct rnpm_adapter *adapter)
{
	if (!(adapter->flags2 & RNPM_FLAG2_RESET_REQUESTED))
		return;

	adapter->flags2 &= ~RNPM_FLAG2_RESET_REQUESTED;
	if (test_bit(__RNPM_DOWN, &adapter->state) ||
	    test_bit(__RNPM_RESETTING, &adapter->state))
		return;

	netdev_err(adapter->netdev, "Reset adapter\n");
	adapter->tx_timeout_count++;
	rnpm_reinit_locked(adapter);
}

static void rnpm_rx_len_reset_subtask(struct rnpm_adapter *adapter)
{
	struct rnpm_ring *rx_ring;
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		rx_ring = adapter->rx_ring[i];
		if (unlikely(rx_ring->ring_flags &
			     RNPM_RING_FLAG_DO_RESET_RX_LEN)) {
			dbg_apt("[%s] Rx-ring %d count reset\n",
				adapter->netdev->name,
				rx_ring->rnpm_queue_idx);
			rnpm_rx_ring_reinit(adapter, rx_ring);
			rx_ring->ring_flags &=
				(~RNPM_RING_FLAG_DO_RESET_RX_LEN);
		}
	}
}

__maybe_unused static int rnpm_check_mc_addr(struct rnpm_adapter *adapter)
{
	struct rnpm_pf_adapter *pf_adapter = adapter->pf_adapter;
	u32 mta_shadow[RNPM_MAX_MTA];
	int i;
	int j;
	int ret = 0;
	struct rnpm_hw *hw;

	memcpy(mta_shadow, pf_adapter->mta_shadow,
	       sizeof(u32) * RNPM_MAX_MTA);
	for (i = 0; i < RNPM_MAX_MTA; i++) {
		pf_adapter->mta_shadow[i] = 0;
		for (j = 0; j < pf_adapter->adapter_cnt; j++) {
			if (rnpm_port_is_valid(pf_adapter, j)) {
				hw = &pf_adapter->adapter[j]->hw;
				pf_adapter->mta_shadow[i] |=
					hw->mac.mta_shadow[j];
			}
		}
		if (pf_adapter->mta_shadow[i] != mta_shadow[i])
			ret = 1;
	}
	return ret;
}

__maybe_unused static void
rnpm_update_feature_subtask(struct rnpm_adapter *adapter)
{
	struct rnpm_pf_adapter __maybe_unused *pf_adapter =
		adapter->pf_adapter;
	netdev_features_t features = adapter->netdev->features;
	u32 changed = 0;

	if (test_bit(__RNPM_DOWN, &adapter->state) ||
	    test_bit(__RNPM_RESETTING, &adapter->state))
		return;

	if (adapter->flags_feature & RNPM_FLAG_DELAY_UPDATE_VLAN_FILTER) {
#ifdef NETIF_F_HW_VLAN_CTAG_FILTER
		if (pf_adapter->vlan_status_true) {
			if (!(features & NETIF_F_HW_VLAN_CTAG_FILTER)) {
				features |= NETIF_F_HW_VLAN_CTAG_FILTER;
				changed = 1;
			}
		} else {
			if (features & NETIF_F_HW_VLAN_CTAG_FILTER) {
				features &= (~NETIF_F_HW_VLAN_CTAG_FILTER);
				changed = 1;
			}
		}
#endif
	}

	if (changed)
		adapter->netdev->features = features;

	if (adapter->flags_feature & RNPM_FLAG_DELAY_UPDATE_VLAN_TABLE)
		adapter->flags_feature &=
			(~RNPM_FLAG_DELAY_UPDATE_VLAN_TABLE);

	if (adapter->flags_feature & RNPM_FLAG_DELAY_UPDATE_MUTICAST_TABLE)
		adapter->flags_feature &=
			(~RNPM_FLAG_DELAY_UPDATE_MUTICAST_TABLE);
}

/**
 * rnpm_pf_service_task - manages and runs subtasks
 * @work: pointer to work_struct containing our data
 **/
static void rnpm_pf_service_task(struct work_struct *work)
{
	struct rnpm_pf_adapter *pf_adapter =
		container_of(work, struct rnpm_pf_adapter, service_task);

	if (test_bit(__RNPM_REMOVING, &pf_adapter->state))
		return;

	/* reset pf */
	if (test_and_clear_bit(RNPM_PF_RESET, &pf_adapter->flags))
		rnpm_pf_reset_subtask(pf_adapter);

	/* set mtu */
	if (test_and_clear_bit(RNPM_PF_SET_MTU, &pf_adapter->flags))
		rnpm_pf_mtu_subtask(pf_adapter);

	/* when up/down need delay get link stat on next time */
	if (test_and_clear_bit(RNPM_PF_SERVICE_SKIP_HANDLE,
			       &pf_adapter->flags))
		return;

	if (test_bit(RNPM_PF_LINK_CHANGE, &pf_adapter->flags)) {
		if (rnpm_pf_get_port_link_stat(pf_adapter) < 0)
			set_bit(RNPM_PF_LINK_CHANGE, &pf_adapter->flags);
		else
			clear_bit(RNPM_PF_LINK_CHANGE, &pf_adapter->flags);
	}
}

/**
 * rnpm_service_task - manages and runs subtasks
 * @work: pointer to work_struct containing our data
 **/
void rnpm_service_task(struct work_struct *work)
{
	struct rnpm_adapter *adapter =
		container_of(work, struct rnpm_adapter, service_task);

	set_bit(__RNPM_SERVICE_IN_PROGRESS, &adapter->state);
	rnpm_reset_subtask(adapter);
	rnpm_sfp_link_config_subtask(adapter);
	rnpm_watchdog_subtask(adapter);
	rnpm_rx_len_reset_subtask(adapter);
	rnpm_check_hang_subtask(adapter);
	rnpm_service_event_complete(adapter);
	clear_bit(__RNPM_SERVICE_IN_PROGRESS, &adapter->state);
}

static int rnpm_tso(struct rnpm_ring *tx_ring,
		    struct rnpm_tx_buffer *first, u8 *hdr_len)
{
	struct rnpm_adapter *adapter = netdev_priv(tx_ring->netdev);
	struct sk_buff *skb = first->skb;
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

	first->tx_flags |= RNPM_TXD_FLAG_TSO | RNPM_TXD_IP_CSUM |
			   RNPM_TXD_L4_CSUM;

	if (ip.v4->version == 4)
		ip.v4->check = 0x0000;
	else
		ip.v6->payload_len = 0;
	if (skb_shinfo(skb)->gso_type &
	    (SKB_GSO_GRE | SKB_GSO_GRE_CSUM | SKB_GSO_UDP_TUNNEL |
	     SKB_GSO_UDP_TUNNEL_CSUM)) {
		if (!(skb_shinfo(skb)->gso_type & SKB_GSO_PARTIAL) &&
		    (skb_shinfo(skb)->gso_type &
		     SKB_GSO_UDP_TUNNEL_CSUM)) {
		}
		inner_mac = skb_inner_mac_header(skb);
		first->tunnel_hdr_len = (inner_mac - skb->data);
		if (skb_shinfo(skb)->gso_type &
		    (SKB_GSO_UDP_TUNNEL | SKB_GSO_UDP_TUNNEL_CSUM)) {
			first->tx_flags |= RNPM_TXD_TUNNEL_VXLAN;
			l4.udp->check = 0;
			tx_dbg("set outer l4.udp to 0\n");
		} else {
			first->tx_flags |= RNPM_TXD_TUNNEL_NVGRE;
		}
		/* reset pointers to inner headers */
		ip.hdr = skb_inner_network_header(skb);
		l4.hdr = skb_inner_transport_header(skb);
	}
	if (ip.v4->version == 4) {
		ip.v4->check = 0x0000;
	} else {
		ip.v6->payload_len = 0;
		first->tx_flags |= (RNPM_TXD_FLAG_IPv6);
	}

	/* determine offset of inner transport header */
	l4_offset = l4.hdr - skb->data;
	paylen = skb->len - l4_offset;
	if (skb->csum_offset == offsetof(struct tcphdr, check)) {
		tx_dbg("tcp before l4 checksum is %x\n", l4.tcp->check);
		first->tx_flags |= RNPM_TXD_L4_TYPE_TCP;
		/* compute length of segmentation header */
		*hdr_len = (l4.tcp->doff * 4) + l4_offset;
		csum_replace_by_diff(&l4.tcp->check,
				     (__force __wsum)htonl(paylen));
		tx_dbg("tcp l4 checksum is %x\n", l4.tcp->check);
		/* should clear tcp push flags */
		l4.tcp->psh = 0;
	} else {
		tx_dbg("paylen is %x\n", paylen);
		first->tx_flags |= RNPM_TXD_L4_TYPE_UDP;
		/* compute length of segmentation header */
		tx_dbg("udp before l4 checksum is %x\n", l4.udp->check);
		*hdr_len = sizeof(*l4.udp) + l4_offset;
		csum_replace_by_diff(&l4.udp->check,
				     (__force __wsum)htonl(paylen));
		tx_dbg("udp l4 checksum is %x\n", l4.udp->check);
	}

	first->mac_ip_len = l4.hdr - ip.hdr;
	first->mac_ip_len |= (ip.hdr - inner_mac) << 9;
	gso_size = skb_shinfo(skb)->gso_size;
	gso_segs = skb_shinfo(skb)->gso_segs;
	if (adapter->priv_flags & RNPM_PRIV_FLAG_TX_PADDING) {
		gso_need_pad = (first->skb->len - *hdr_len) % gso_size;
		if (gso_need_pad) {
			if ((gso_need_pad + *hdr_len) <=
			    tx_ring->gso_padto_bytes) {
				gso_need_pad = tx_ring->gso_padto_bytes -
					       (gso_need_pad + *hdr_len);
				first->gso_need_padding = !!gso_need_pad;
			}
		}
	}
	first->gso_segs = gso_segs;
	first->bytecount += (first->gso_segs - 1) * *hdr_len;
	first->mss_len_vf_num |= (gso_size | ((l4.tcp->doff * 4) << 24));
	first->ctx_flag = true;
	return 1;
}

__maybe_unused static void set_resevd(struct rnpm_tx_buffer *first)
{
	struct sk_buff *skb = first->skb;
	union {
		struct iphdr *v4;
		struct ipv6hdr *v6;
		unsigned char *hdr;
	} ip;
	union {
		struct tcphdr *tcp;
		struct udphdr *udp;
		unsigned char *hdr;
	} l4 __maybe_unused;

	ip.hdr = skb_network_header(skb);
	if (ip.v4->version == 4) {
		u16 old = ip.v4->frag_off;

		ip.v4->frag_off |= 0x0080;
		csum_replace_by_diff(&ip.v4->check, ip.v4->frag_off - old);
	}
}

static int rnpm_tx_csum(struct rnpm_ring *tx_ring,
			struct rnpm_tx_buffer *first)
{
	struct sk_buff *skb = first->skb;
	struct rnpm_adapter *adapter = netdev_priv(tx_ring->netdev);
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

	if (adapter->priv_flags & RNPM_PRIV_FLAG_TX_PADDING) {
		/* Skb is sctp and len < 60 bytes, need to open mac padding */
		if (tx_ring->gso_padto_bytes != 60)
			first->gso_need_padding = true;
	}
	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return 0;

	ip.hdr = skb_network_header(skb);
	l4.hdr = skb_transport_header(skb);
	inner_mac = skb->data;

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
		switch (l4_proto) {
		case IPPROTO_UDP:
			l4.udp->check = 0;

			first->tx_flags |= RNPM_TXD_TUNNEL_VXLAN;
			break;
		case IPPROTO_GRE:
			first->tx_flags |= RNPM_TXD_TUNNEL_NVGRE;
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

	mac_len = (ip.hdr - inner_mac);
	tx_dbg("inner checksum needed %d", skb_checksum_start_offset(skb));
	tx_dbg("skb->encapsulation %d\n", skb->encapsulation);
	ip_len = (l4.hdr - ip.hdr);
	if (ip.v4->version == 4) {
		l4_proto = ip.v4->protocol;
	} else {
		exthdr = ip.hdr + sizeof(*ip.v6);
		l4_proto = ip.v6->nexthdr;
		if (l4.hdr != exthdr)
			ipv6_skip_exthdr(skb, exthdr - skb->data,
					 &l4_proto, &frag_off);
		first->tx_flags |= RNPM_TXD_FLAG_IPv6;
	}
	/* Enable L4 checksum offloads */
	switch (l4_proto) {
	case IPPROTO_TCP:
		first->tx_flags |= RNPM_TXD_L4_TYPE_TCP | RNPM_TXD_L4_CSUM;
		break;
	case IPPROTO_SCTP:
		tx_dbg("sctp checksum packet\n");
		first->tx_flags |= RNPM_TXD_L4_TYPE_SCTP |
				   RNPM_TXD_L4_CSUM;
		break;
	case IPPROTO_UDP:
		first->tx_flags |= RNPM_TXD_L4_TYPE_UDP | RNPM_TXD_L4_CSUM;
		break;
	default:
		skb_checksum_help(skb);
		return 0;
	}

	tx_dbg("mac length is %d\n", mac_len);
	tx_dbg("ip length is %d\n", ip_len);
	first->mac_ip_len = (mac_len << 9) | ip_len;
	return 0;
}
static int __rnpm_maybe_stop_tx(struct rnpm_ring *tx_ring, u16 size)
{
	netif_stop_subqueue(tx_ring->netdev, tx_ring->queue_index);
	/* memory barrior */
	smp_mb();
	/* We need to check again in a case another CPU has just
	 * made room available.
	 */
	if (likely(rnpm_desc_unused(tx_ring) < size))
		return -EBUSY;

	/* A reprieve! - use start_queue because it doesn't call schedule */
	netif_start_subqueue(tx_ring->netdev, tx_ring->queue_index);
	++tx_ring->tx_stats.restart_queue;
	return 0;
}

static inline int rnpm_maybe_stop_tx(struct rnpm_ring *tx_ring, u16 size)
{
	if (likely(rnpm_desc_unused(tx_ring) >= size))
		return 0;
	return __rnpm_maybe_stop_tx(tx_ring, size);
}

static int rnpm_tx_map(struct rnpm_ring *tx_ring,
		       struct rnpm_tx_buffer *first, const u8 hdr_len)
{
	struct sk_buff *skb = first->skb;
	struct rnpm_tx_buffer *tx_buffer;
	struct rnpm_tx_desc *tx_desc;
	skb_frag_t *frag;
	dma_addr_t dma;
	unsigned int data_len, size;
	u32 tx_flags = first->tx_flags;
	u32 mac_ip_len = (first->mac_ip_len) << 16;
	u16 i = tx_ring->next_to_use;
	u64 fun_id = ((u64)(tx_ring->pfvfnum) << (56));

	tx_desc = RNPM_TX_DESC(tx_ring, i);
	size = skb_headlen(skb);
	data_len = skb->data_len;
	dma = dma_map_single(tx_ring->dev, skb->data, size, DMA_TO_DEVICE);
	tx_buffer = first;
	for (frag = &skb_shinfo(skb)->frags[0];; frag++) {
		if (dma_mapping_error(tx_ring->dev, dma))
			goto dma_error;
		dma_unmap_len_set(tx_buffer, len, size);
		dma_unmap_addr_set(tx_buffer, dma, dma);
		tx_desc->pkt_addr = cpu_to_le64(dma | fun_id);
		while (unlikely(size > RNPM_MAX_DATA_PER_TXD)) {
			tx_desc->vlan_cmd = cpu_to_le32(tx_flags);
			tx_desc->blen_mac_ip_len = cpu_to_le32(
				mac_ip_len ^ RNPM_MAX_DATA_PER_TXD);
			buf_dump_line("tx0  ", __LINE__, tx_desc,
				      sizeof(*tx_desc));
			i++;
			tx_desc++;
			if (i == tx_ring->count) {
				tx_desc = RNPM_TX_DESC(tx_ring, 0);
				i = 0;
			}
			dma += RNPM_MAX_DATA_PER_TXD;
			size -= RNPM_MAX_DATA_PER_TXD;
			tx_desc->pkt_addr = cpu_to_le64(dma | fun_id);
		}
		buf_dump_line("tx1  ", __LINE__, tx_desc,
			      sizeof(*tx_desc));
		if (likely(!data_len))
			break;
		tx_desc->vlan_cmd = cpu_to_le32(tx_flags);
		tx_desc->blen_mac_ip_len = cpu_to_le32(mac_ip_len ^ size);
		buf_dump_line("tx2  ", __LINE__, tx_desc,
			      sizeof(*tx_desc));
		i++;
		tx_desc++;
		if (i == tx_ring->count) {
			tx_desc = RNPM_TX_DESC(tx_ring, 0);
			i = 0;
		}
		size = skb_frag_size(frag);
		data_len -= size;
		dma = skb_frag_dma_map(tx_ring->dev, frag, 0, size,
				       DMA_TO_DEVICE);
		tx_buffer = &tx_ring->tx_buffer_info[i];
	}

	/* write last descriptor with RS and EOP bits */
	tx_desc->vlan_cmd =
		cpu_to_le32(tx_flags | RNPM_TXD_CMD_EOP | RNPM_TXD_CMD_RS);
	tx_desc->blen_mac_ip_len = cpu_to_le32(mac_ip_len ^ size);
	buf_dump_line("tx3  ", __LINE__, tx_desc, sizeof(*tx_desc));
	/* set the timestamp */
	first->time_stamp = jiffies;
	netdev_tx_sent_queue(txring_txq(tx_ring), first->bytecount);
	/*
	 * Force memory writes to complete before letting h/w know there
	 * are new descriptors to fetch.  (Only applicable for weak-ordered
	 * memory model archs, such as IA-64).
	 *
	 * We also need this memory barrier to make certain all of the
	 * status bits have been updated before next_to_watch is written.
	 */
	wmb();
	first->next_to_watch = tx_desc;
	i++;
	if (i == tx_ring->count)
		i = 0;
	tx_ring->next_to_use = i;
	rnpm_maybe_stop_tx(tx_ring, DESC_NEEDED);
	if (netif_xmit_stopped(txring_txq(tx_ring)) ||
	    !netdev_xmit_more()) {
		tx_ring->tx_stats.send_bytes_to_hw += first->bytecount;
		tx_ring->tx_stats.send_bytes_to_hw +=
			tx_ring->tx_stats.todo_update;
		tx_ring->tx_stats.todo_update = 0;
		rnpm_wr_reg(tx_ring->tail, i);
	} else {
		tx_ring->tx_stats.todo_update = first->bytecount;
	}
	return 0;
dma_error:
	dev_err(tx_ring->dev, "TX DMA map failed\n");

	/* clear dma mappings for failed tx_buffer_info map */
	for (;;) {
		tx_buffer = &tx_ring->tx_buffer_info[i];
		rnpm_unmap_and_free_tx_resource(tx_ring, tx_buffer);
		if (tx_buffer == first)
			break;
		if (i == 0)
			i = tx_ring->count;
		i--;
	}
	tx_ring->next_to_use = i;
	return -1;
}

netdev_tx_t rnpm_xmit_frame_ring(struct sk_buff *skb,
				 struct rnpm_adapter *adapter,
				 struct rnpm_ring *tx_ring)
{
	struct rnpm_tx_buffer *first;
	u16 count = TXD_USE_COUNT(skb_headlen(skb));
	__be16 protocol = skb->protocol;
	u8 hdr_len = 0;
	u32 tx_flags = 0;
	unsigned short f;
	int tso;

	/*
	 * need: 1 descriptor per page * PAGE_SIZE/RNPM_MAX_DATA_PER_TXD,
	 *       + 1 desc for skb_headlen/RNPM_MAX_DATA_PER_TXD,
	 *       + 2 desc gap to keep tail from touching head,
	 *       + 1 desc for context descriptor,
	 * otherwise try next time
	 */
	for (f = 0; f < skb_shinfo(skb)->nr_frags; f++) {
		skb_frag_t *frag_temp = &skb_shinfo(skb)->frags[f];

		count += TXD_USE_COUNT(skb_frag_size(frag_temp));
		tx_dbg(" #%d frag: size:%d\n", f,
		       skb_frag_size(frag_temp));
		if (count > 60) {
			e_dev_err("desc too large, %d\n", count);
			return NETDEV_TX_BUSY;
		}
	}

	if (rnpm_maybe_stop_tx(tx_ring, count + 3)) {
		tx_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}

	/* record the location of the first descriptor for this packet */
	first = &tx_ring->tx_buffer_info[tx_ring->next_to_use];
	first->skb = skb;
	first->bytecount = skb->len;
	first->gso_segs = 1;
	first->type_tucmd = 0;

	/* default len should not 0 (hw request) */
	first->mac_ip_len = 20;
	first->mss_len_vf_num = 0;
	first->inner_vlan_tunnel_len = 0;

#ifdef RNPM_IOV_VEB_BUG_NOT_FIXED
	first->ctx_flag =
		(adapter->flags & RNPM_FLAG_SRIOV_ENABLED) ? true : false;
#else
	first->ctx_flag = false;
#endif
	if (adapter->priv_flags & RNPM_PRIV_FLAG_TX_PADDING)
		first->ctx_flag = true;

	if (adapter->flags2 & RNPM_FLAG2_VLAN_STAGS_ENABLED) {
		/* always add a stags for any packets out */
		tx_flags |= adapter->stags_vid;
		tx_flags |= RNPM_TXD_VLAN_CTRL_INSERT_VLAN;
		if (skb_vlan_tag_present(skb)) {
			tx_flags |= RNPM_TXD_VLAN_VALID;
			first->inner_vlan_tunnel_len |=
				(skb_vlan_tag_get(skb) << 8);
			first->ctx_flag = true;
		} else if (protocol == __constant_htons(ETH_P_8021Q)) {
			struct vlan_hdr *vhdr, _vhdr;

			vhdr = skb_header_pointer(skb, ETH_HLEN,
						  sizeof(_vhdr), &_vhdr);
			if (!vhdr)
				goto out_drop;

			protocol = vhdr->h_vlan_encapsulated_proto;
			tx_flags |= RNPM_TXD_VLAN_VALID;
		}
	} else {
		if (skb_vlan_tag_present(skb)) {
			tx_flags |= skb_vlan_tag_get(skb);
			tx_flags |= RNPM_TXD_VLAN_VALID |
				    RNPM_TXD_VLAN_CTRL_INSERT_VLAN;
			tx_ring->tx_stats.vlan_add++;
		} else if (protocol == __constant_htons(ETH_P_8021Q)) {
			struct vlan_hdr *vhdr, _vhdr;

			vhdr = skb_header_pointer(skb, ETH_HLEN,
						  sizeof(_vhdr), &_vhdr);
			if (!vhdr)
				goto out_drop;

			protocol = vhdr->h_vlan_encapsulated_proto;
			tx_flags |= ntohs(vhdr->h_vlan_TCI);
			tx_flags |= RNPM_TXD_VLAN_VALID;
		}
	}
	protocol = vlan_get_protocol(skb);

	skb_tx_timestamp(skb);
#ifdef SKB_SHARED_TX_IS_UNION
	if (unlikely(skb_tx(skb)->hardware) &&
	    adapter->flags2 & RNPM_FLAG2_PTP_ENABLED &&
	    adapter->ptp_tx_en) {
		if (!test_and_set_bit_lock(__RNPM_PTP_TX_IN_PROGRESS,
					   &adapter->state)) {
			skb_tx(skb)->in_progress = 1;

#else
	if (unlikely(skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) &&
	    adapter->flags2 & RNPM_FLAG2_PTP_ENABLED &&
	    adapter->ptp_tx_en) {
		if (!test_and_set_bit_lock(__RNPM_PTP_TX_IN_PROGRESS,
					   &adapter->state)) {
			skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
#endif
			tx_flags |= RNPM_TXD_FLAG_PTP;
			adapter->ptp_tx_skb = skb_get(skb);
			adapter->tx_hwtstamp_start = jiffies;
			schedule_work(&adapter->tx_hwtstamp_work);

		} else {
			e_dev_info("ptp_tx_skb miss\n");
		}
	}
	/* record initial flags and protocol */
	first->tx_flags = tx_flags;
	first->protocol = protocol;

	tso = rnpm_tso(tx_ring, first, &hdr_len);
	if (tso < 0)
		goto out_drop;
	else if (!tso)
		rnpm_tx_csum(tx_ring, first);
	/* in this mode pf send msg should with vf_num */
	if (adapter->flags & RNPM_FLAG_SRIOV_ENABLED) {
		first->ctx_flag = true;
		first->mss_len_vf_num |= (adapter->vf_num_for_pf << 16);
	}
	/* send this packet to rpu */
	if (adapter->priv_flags & RNPM_PRIV_FLAG_TO_RPU) {
		first->ctx_flag = true;
		first->type_tucmd |= RNPM_TXD_FLAG_TO_RPU;
	}
	/* add control desc */
	rnpm_maybe_tx_ctxtdesc(tx_ring, first, first->type_tucmd);
	if (rnpm_tx_map(tx_ring, first, hdr_len))
		goto cleanup_tx_tstamp;
	return NETDEV_TX_OK;

out_drop:
	dev_kfree_skb_any(first->skb);
	first->skb = NULL;
cleanup_tx_tstamp:

	if (unlikely(tx_flags & RNPM_TXD_FLAG_PTP)) {
		dev_kfree_skb_any(adapter->ptp_tx_skb);
		adapter->ptp_tx_skb = NULL;
		cancel_work_sync(&adapter->tx_hwtstamp_work);
		clear_bit_unlock(__RNPM_PTP_TX_IN_PROGRESS,
				 &adapter->state);
	}

	return NETDEV_TX_OK;
}

static u8 skb_need_padto_bytes(struct sk_buff *skb, bool mac_padding)
{
	u8 l4_proto = 0;
	union {
		struct iphdr *v4;
		struct ipv6hdr *v6;
		unsigned char *hdr;
	} ip;

	if (mac_padding) {
		ip.hdr = skb_network_header(skb);
		l4_proto = ip.v4->version == 4 ? ip.v4->protocol :
						 ip.v6->nexthdr;
		/* Skb is sctp and len < 60 bytes, need to open mac padding */
		if ((l4_proto == IPPROTO_SCTP) && (skb->len < 60))
			return 33;
		return 60;
	}

	return 33;
}

static netdev_tx_t rnpm_xmit_frame(struct sk_buff *skb,
				   struct net_device *netdev)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_ring *tx_ring;
	u8 padto_bytes;

	if (!netif_carrier_ok(netdev)) {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	/*
	 * The minimum packet size for olinfo paylen is 17 so pad the skb
	 * in order to meet this minimum size requirement.
	 */
	padto_bytes = skb_need_padto_bytes(
		skb, !!(adapter->priv_flags & RNPM_PRIV_FLAG_TX_PADDING));
	if (skb_put_padto(skb, padto_bytes))
		return NETDEV_TX_OK;

	/* for sctp packet , padding 0 change the crc32c */
	/* mac can padding (17-63) length to 64 */
	tx_ring = adapter->tx_ring[skb->queue_mapping];
	tx_ring->gso_padto_bytes = padto_bytes;

	return rnpm_xmit_frame_ring(skb, adapter, tx_ring);
}

/**
 * rnpm_set_mac - Change the Ethernet Address of the NIC
 * @netdev: network interface device structure
 * @p: pointer to an address structure
 *
 * Returns 0 on success, negative on failure
 **/
static int rnpm_set_mac(struct net_device *netdev, void *p)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	struct sockaddr *addr = p;

	dbg_apt("[%s] call set mac\n", netdev->name);

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	eth_hw_addr_set(netdev, addr->sa_data);
	memcpy(hw->mac.addr, addr->sa_data, netdev->addr_len);

	hw->mac.ops.set_rar(hw, adapter->uc_off, hw->mac.addr, VMDQ_P(0),
			    RNPM_RAH_AV);

	/* setup mac unicast filters */
	if (hw->mac.mc_location == rnpm_mc_location_mac)
		hw->mac.ops.set_rar_mac(hw, 0, hw->mac.addr, VMDQ_P(0),
					adapter->port);
	rnpm_configure_virtualization(adapter);
	return 0;
}

static int rnpm_mdio_read(struct net_device *netdev, int prtad, int devad,
			  u16 addr)
{
	int rc = -EIO;
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	u16 value;

	rc = hw->phy.ops.read_reg(hw, addr, 0, &value);
	if (!rc)
		rc = value;

	return rc;
}

__maybe_unused static int rnpm_mdio_write(struct net_device *netdev,
					  int prtad, int devad, u16 addr,
					  u16 value)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;

	return hw->phy.ops.write_reg(hw, addr, 0, value);
}

static int rnpm_mii_ioctl(struct net_device *netdev, struct ifreq *ifr,
			  int cmd)
{
	struct mii_ioctl_data *mii =
		(struct mii_ioctl_data *)&ifr->ifr_data;
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	int prtad, devad, ret = -EIO;

	prtad = (mii->phy_id & MDIO_PHY_ID_PRTAD) >> 5;
	devad = (mii->phy_id & MDIO_PHY_ID_DEVAD);

	switch (cmd) {
	case SIOCGMIIPHY:
		mii->phy_id = hw->phy.phy_addr;
		break;
	case SIOCGMIIREG:
		ret = rnpm_mdio_read(netdev, prtad, devad, mii->reg_num);
		if (ret < 0)
			return ret;
		mii->val_out = ret;
		break;
	case SIOCSMIIREG:
	default:
		return -EOPNOTSUPP;
	}
	return 0;
}

static int rnpm_ioctl(struct net_device *netdev, struct ifreq *req,
		      int cmd)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);

	switch (cmd) {
#ifdef SIOCGHWTSTAMP
	case SIOCGHWTSTAMP:
#ifndef NO_PTP
		if (module_enable_ptp)
			return rnpm_ptp_get_ts_config(adapter, req);
#endif
#endif
		break;
	case SIOCSHWTSTAMP:
#ifndef NO_PTP
		if (module_enable_ptp)
			return rnpm_ptp_set_ts_config(adapter, req);
		break;
#endif
	case SIOCGMIIPHY:
	case SIOCGMIIREG:
	case SIOCSMIIREG:
		return rnpm_mii_ioctl(netdev, req, cmd);
	}
	return -EINVAL;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
 * Polling 'interrupt' - used by things like netconsole to send skbs
 * without having to re-enable interrupts. It's not called while
 * the interrupt routine is executing.
 */
static void rnpm_netpoll(struct net_device *netdev)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	int i;

	/* do nothing, if interface is down */
	if (test_bit(__RNPM_DOWN, &adapter->state))
		return;

	adapter->flags |= RNPM_FLAG_IN_NETPOLL;
	for (i = 0; i < adapter->num_q_vectors; i++)
		rnpm_msix_clean_rings(0, adapter->q_vector[i]);
	adapter->flags &= ~RNPM_FLAG_IN_NETPOLL;
}

#endif

static void rnpm_get_stats64(struct net_device *netdev,
			     struct rtnl_link_stats64 *stats)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	int i;

	rcu_read_lock();

	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct rnpm_ring *ring = READ_ONCE(adapter->rx_ring[i]);
		u64 bytes, packets;
		unsigned int start;

		if (ring) {
			do {
				start = u64_stats_fetch_begin(
					&ring->syncp);
				packets = ring->stats.packets;
				bytes = ring->stats.bytes;
			} while (u64_stats_fetch_retry(&ring->syncp,
						       start));
			stats->rx_packets += packets;
			stats->rx_bytes += bytes;
		}
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct rnpm_ring *ring = READ_ONCE(adapter->tx_ring[i]);
		u64 bytes, packets;
		unsigned int start;

		if (ring) {
			do {
				start = u64_stats_fetch_begin(
					&ring->syncp);
				packets = ring->stats.packets;
				bytes = ring->stats.bytes;
			} while (u64_stats_fetch_retry(&ring->syncp,
						       start));
			stats->tx_packets += packets;
			stats->tx_bytes += bytes;
		}
	}
	rcu_read_unlock();
	stats->multicast = netdev->stats.multicast;
	stats->rx_errors = netdev->stats.rx_errors;
	stats->rx_dropped = netdev->stats.rx_dropped;
	stats->rx_crc_errors = netdev->stats.rx_crc_errors;
}

/**
 * rnpm_setup_tc - configure net_device for multiple traffic classes
 *
 * @netdev: net device to configure
 * @tc: number of traffic classes to enable
 */
int rnpm_setup_tc(struct net_device *dev, u8 tc)
{
	struct rnpm_adapter *adapter = netdev_priv(dev);
	struct rnpm_hw *hw = &adapter->hw;
	int err = 0;

	/* Hardware supports up to 8 traffic classes */
	if (tc > RNPM_MAX_TCS_NUM)
		return -EINVAL;

	/* Hardware has to reinitialize queues and interrupts to
	 * match packet buffer alignment. Unfortunately, the
	 * hardware is not flexible enough to do this dynamically.
	 */
	while (test_and_set_bit(__RNPM_RESETTING,
				&adapter->pf_adapter->state))
		usleep_range(1000, 2000);

	if (netif_running(dev))
		rnpm_close(dev);

	rnpm_clear_interrupt_scheme(adapter);
	hw->mac.ops.clear_hw_cntrs(hw);
	rnpm_update_stats(adapter);
	rnpm_init_interrupt_scheme(adapter);
	/* rss table must reset */
	adapter->rss_tbl_setup_flag = 0;
	if (netif_running(dev))
		err = rnpm_open(dev);
	clear_bit(__RNPM_RESETTING, &adapter->pf_adapter->state);
	return err;
}

#ifdef CONFIG_PCI_IOV
void rnpm_sriov_reinit(struct rnpm_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	rtnl_lock();
	rnpm_setup_tc(netdev, netdev_get_num_tc(netdev));
	rtnl_unlock();
}
#endif

void rnpm_do_reset(struct net_device *netdev)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev))
		rnpm_reinit_locked(adapter);
	else
		rnpm_reset(adapter);
}

static netdev_features_t rnpm_fix_features(struct net_device *netdev,
					   netdev_features_t features)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);

	/* If Rx checksum is disabled, then RSC/LRO should also be disabled */
	if (!(features & NETIF_F_RXCSUM))
		features &= ~NETIF_F_LRO;
	/* close rx csum when rx fcs on */
	if (features & NETIF_F_RXFCS)
		features &= (~NETIF_F_RXCSUM);
	/* Turn off LRO if not RSC capable */
	if (!(adapter->flags2 & RNPM_FLAG2_RSC_CAPABLE))
		features &= ~NETIF_F_LRO;
	return features;
}

static int rnpm_set_features(struct net_device *netdev,
			     netdev_features_t features)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	netdev_features_t changed = netdev->features ^ features;
	bool need_reset = false;
	struct rnpm_hw *hw = &adapter->hw;

	switch (features & NETIF_F_NTUPLE) {
	case NETIF_F_NTUPLE:
		/* turn off ATR, enable perfect filters and reset */
		if (!(adapter->flags & RNPM_FLAG_FDIR_PERFECT_CAPABLE))
			need_reset = true;

		adapter->flags &= ~RNPM_FLAG_FDIR_HASH_CAPABLE;
		adapter->flags |= RNPM_FLAG_FDIR_PERFECT_CAPABLE;
		break;
	default:
		/* turn off perfect filters, enable ATR and reset */
		if (adapter->flags & RNPM_FLAG_FDIR_PERFECT_CAPABLE)
			need_reset = true;

		adapter->flags &= ~RNPM_FLAG_FDIR_PERFECT_CAPABLE;

		/* cannot enable ATR if SR-IOV is enabled */
		if (adapter->flags & RNPM_FLAG_SRIOV_ENABLED)
			break;

		/* cannot enable ATR if we have 2 or more traffic classes */
		if (netdev_get_num_tc(netdev) > 1)
			break;
		if (!adapter->atr_sample_rate)
			break;
		adapter->flags |= RNPM_FLAG_FDIR_HASH_CAPABLE;
		break;
	}

	/* vlan filter changed */
#ifdef NETIF_F_HW_VLAN_CTAG_FILTER
	if (changed & (NETIF_F_HW_VLAN_CTAG_FILTER)) {
		if (features & NETIF_F_HW_VLAN_CTAG_FILTER)
			rnpm_vlan_filter_enable(adapter);
		else
			rnpm_vlan_filter_disable(adapter);
	}
#endif /* NETIF_F_HW_VLAN_CTAG_FILTER */
	/* should set rss table to all 0 */
	if (changed & (NETIF_F_RXHASH)) {
		if (adapter->flags & RNPM_FLAG_MUTIPORT_ENABLED) {
			/* in mutiport mode ,use rss table to zero instead close hw flags */
			if (features & (NETIF_F_RXHASH)) {
				adapter->flags &=
					(~RNPM_FLAG_RXHASH_DISABLE);
				rnpm_store_reta(adapter);
			} else {
				adapter->flags |= RNPM_FLAG_RXHASH_DISABLE;
				rnpm_store_reta(adapter);
			}

		} else {
			u32 iov_en = (adapter->flags &
				      RNPM_FLAG_SRIOV_ENABLED) ?
					     RNPM_IOV_ENABLED :
					     0;
			/* close rxhash will lead all rx packets to ring 0 */
			if (features & (NETIF_F_RXHASH))
				wr32(hw, RNPM_ETH_RSS_CONTROL,
				     RNPM_ETH_ENABLE_RSS_ONLY | iov_en);
			else
				wr32(hw, RNPM_ETH_RSS_CONTROL,
				     RNPM_ETH_DISABLE_RSS | iov_en);
		}
	}

	/* in this mode rx l4/sctp checksum will get error */
	if (changed & NETIF_F_RXFCS) {
		u32 old_value;

		rnpm_msg_post_status(adapter, PF_FCS_STATUS);

		old_value = rd32(hw, RNPM_MAC_RX_CFG(adapter->port));
#define FCS_MASK (0x6)
		if (features & NETIF_F_RXFCS) {
			/* if in rx fcs mode , hw rxcsum may error, close rxcusm */
			old_value &= (~FCS_MASK);
		} else {
			old_value |= FCS_MASK;
		}
		old_value = adapter->mac_ipc_off ?
				    old_value & ~RNPM_MAX_RX_CFG_IPC :
				    old_value | RNPM_MAX_RX_CFG_IPC;
		wr32(hw, RNPM_MAC_RX_CFG(adapter->port), old_value);
	}

	if (changed & NETIF_F_RXALL)
		need_reset = true;

#ifdef NETIF_F_HW_VLAN_CTAG_RX
	if (changed & NETIF_F_HW_VLAN_CTAG_RX) {
		if (features & NETIF_F_HW_VLAN_CTAG_RX)
			rnpm_vlan_strip_enable(adapter);
		else
			rnpm_vlan_strip_disable(adapter);
	}
#endif

	/* set up active feature */
	netdev->features = features;

	if (need_reset)
		rnpm_do_reset(netdev);

	return 0;
}

static int
rnpm_ndo_bridge_setlink(struct net_device *dev, struct nlmsghdr *nlh,
			__always_unused u16 flags,
			struct netlink_ext_ack __always_unused *ext)
{
	struct rnpm_adapter *adapter = netdev_priv(dev);
	struct rnpm_hw *hw = &adapter->hw;
	struct nlattr *attr, *br_spec;
	int rem;

	if (!(adapter->flags & RNPM_FLAG_SRIOV_ENABLED))
		return -EOPNOTSUPP;

	br_spec = nlmsg_find_attr(nlh, sizeof(struct ifinfomsg),
				  IFLA_AF_SPEC);

	nla_for_each_nested(attr, br_spec, rem) {
		__u16 mode;

		if (nla_type(attr) != IFLA_BRIDGE_MODE)
			continue;

		mode = nla_get_u16(attr);
		if (mode == BRIDGE_MODE_VEPA) {
			adapter->flags2 &= ~RNPM_FLAG2_BRIDGE_MODE_VEB;
			wr32(hw, RNPM_DMA_CONFIG,
			     rd32(hw, RNPM_DMA_CONFIG) | DMA_VEB_BYPASS);
		} else if (mode == BRIDGE_MODE_VEB) {
			adapter->flags2 |= RNPM_FLAG2_BRIDGE_MODE_VEB;
			wr32(hw, RNPM_DMA_CONFIG,
			     rd32(hw, RNPM_DMA_CONFIG) &
				     (~DMA_VEB_BYPASS));

		} else
			return -EINVAL;

		e_info(drv, "enabling bridge mode: %s\n",
		       mode == BRIDGE_MODE_VEPA ? "VEPA" : "VEB");
	}

	return 0;
}

static int rnpm_ndo_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq,
				   struct net_device *dev,
				   u32 __maybe_unused filter_mask,
				   int nlflags)
{
	struct rnpm_adapter *adapter = netdev_priv(dev);
	u16 mode;

	if (!(adapter->flags & RNPM_FLAG_SRIOV_ENABLED))
		return 0;

	if (adapter->flags2 & RNPM_FLAG2_BRIDGE_MODE_VEB)
		mode = BRIDGE_MODE_VEB;
	else
		mode = BRIDGE_MODE_VEPA;
	return ndo_dflt_bridge_getlink(skb, pid, seq, dev, mode, 0, 0,
				       nlflags, filter_mask, NULL);
}

static void rnpm_clear_udp_tunnel_port(struct rnpm_adapter *adapter)
{
	struct rnpm_hw *hw = &adapter->hw;

	if (!(adapter->flags & (RNPM_FLAG_VXLAN_OFFLOAD_CAPABLE)))
		return;

	wr32(hw, RNPM_ETH_VXLAN_PORT, 0);
	adapter->vxlan_port = 0;
}

/**
 * rnpm_add_udp_tunnel_port - Get notifications about adding UDP tunnel ports
 * @dev: The port's netdev
 * @ti: Tunnel endpoint information
 **/
__maybe_unused static void
rnpm_add_udp_tunnel_port(struct net_device *dev,
			 struct udp_tunnel_info *ti)
{
	struct rnpm_adapter *adapter = netdev_priv(dev);
	struct rnpm_hw *hw = &adapter->hw;
	__be16 port = ti->port;

	if (ti->sa_family != AF_INET)
		return;
	switch (ti->type) {
	case UDP_TUNNEL_TYPE_VXLAN:
		if (!(adapter->flags & RNPM_FLAG_VXLAN_OFFLOAD_CAPABLE))
			return;
		if (adapter->vxlan_port == port)
			return;
		if (adapter->vxlan_port) {
			netdev_info(
				dev,
				"VXLAN port %d set, not adding port %d\n",
				ntohs(adapter->vxlan_port), ntohs(port));
			return;
		}
		adapter->vxlan_port = port;
		break;
	default:
		return;
	}
	wr32(hw, RNPM_ETH_VXLAN_PORT, adapter->vxlan_port);
}

/**
 * rnpm_del_udp_tunnel_port - Get notifications about removing UDP tunnel ports
 * @dev: The port's netdev
 * @ti: Tunnel endpoint information
 **/
__maybe_unused static void
rnpm_del_udp_tunnel_port(struct net_device *dev,
			 struct udp_tunnel_info *ti)
{
	struct rnpm_adapter *adapter = netdev_priv(dev);

	if (ti->type != UDP_TUNNEL_TYPE_VXLAN)
		return;

	if (ti->sa_family != AF_INET)
		return;

	switch (ti->type) {
	case UDP_TUNNEL_TYPE_VXLAN:
		if (!(adapter->flags & RNPM_FLAG_VXLAN_OFFLOAD_CAPABLE))
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

	rnpm_clear_udp_tunnel_port(adapter);
	adapter->flags2 |= RNPM_FLAG2_UDP_TUN_REREG_NEEDED;
}

#define RNPM_MAX_TUNNEL_HDR_LEN 80
#define RNPM_MAX_MAC_HDR_LEN 127
#define RNPM_MAX_NETWORK_HDR_LEN 511

static netdev_features_t rnpm_features_check(struct sk_buff *skb,
					     struct net_device *dev,
					     netdev_features_t features)
{
	unsigned int network_hdr_len, mac_hdr_len;

	/* Make certain the headers can be described by a context descriptor */
	mac_hdr_len = skb_network_header(skb) - skb->data;
	if (unlikely(mac_hdr_len > RNPM_MAX_MAC_HDR_LEN))
		return features & ~(NETIF_F_HW_CSUM | NETIF_F_SCTP_CRC |
				    NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_TSO |
				    NETIF_F_TSO6);

	network_hdr_len =
		skb_checksum_start(skb) - skb_network_header(skb);
	if (unlikely(network_hdr_len > RNPM_MAX_NETWORK_HDR_LEN))
		return features & ~(NETIF_F_HW_CSUM | NETIF_F_SCTP_CRC |
				    NETIF_F_TSO | NETIF_F_TSO6);

	/* We can only support IPV4 TSO in tunnels if we can mangle the
	 * inner IP ID field, so strip TSO if MANGLEID is not supported.
	 */
	if (skb->encapsulation && !(features & NETIF_F_TSO_MANGLEID))
		features &= ~NETIF_F_TSO;

	return features;
}

const struct net_device_ops rnpm_netdev_ops = {
	.ndo_open = rnpm_open,
	.ndo_stop = rnpm_close,
	.ndo_start_xmit = rnpm_xmit_frame,
	.ndo_set_rx_mode = rnpm_set_rx_mode,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_eth_ioctl = rnpm_ioctl,
	.ndo_change_mtu = rnpm_change_mtu,
	.ndo_get_stats64 = rnpm_get_stats64,
	.ndo_tx_timeout = rnpm_tx_timeout,
	.ndo_set_tx_maxrate = rnpm_tx_maxrate,
	.ndo_set_mac_address = rnpm_set_mac,
	.ndo_vlan_rx_add_vid = rnpm_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = rnpm_vlan_rx_kill_vid,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller = rnpm_netpoll,
#endif
	.ndo_bridge_setlink = rnpm_ndo_bridge_setlink,
	.ndo_bridge_getlink = rnpm_ndo_bridge_getlink,
	.ndo_features_check = rnpm_features_check,
	.ndo_set_features = rnpm_set_features,
	.ndo_fix_features = rnpm_fix_features,
};

static void rnpm_assign_netdev_ops(struct net_device *dev)
{
	dev->netdev_ops = &rnpm_netdev_ops;
	rnpm_set_ethtool_ops(dev);
	dev->watchdog_timeo = 5 * HZ;
}

/**
 * rnpm_wol_supported - Check whether device supports WoL
 * @hw: hw specific details
 * @device_id: the device ID
 * @subdev_id: the subsystem device ID
 *
 * This function is used by probe and ethtool to determine
 * which devices have WoL support
 *
 **/
int rnpm_wol_supported(struct rnpm_adapter *adapter, u16 device_id,
		       u16 subdevice_id)
{
	struct rnpm_hw *hw = &adapter->hw;
	int is_wol_supported = 0;

	if (hw->wol_supported)
		is_wol_supported = 1;
	return is_wol_supported;
}

static inline unsigned long rnpm_tso_features(void)
{
	unsigned long features = 0;

#ifdef NETIF_F_TSO
	features |= NETIF_F_TSO;
#endif /* NETIF_F_TSO */
#ifdef NETIF_F_TSO6
	features |= NETIF_F_TSO6;
#endif /* NETIF_F_TSO6 */
	features |= NETIF_F_GSO_PARTIAL | RNPM_GSO_PARTIAL_FEATURES;
	return features;
}

static int rnpm_rm_adpater(struct rnpm_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	netdev_info(netdev, "remove adapter\n");
	rnpm_dbg_adapter_exit(adapter);
	netif_carrier_off(netdev);
	set_bit(__RNPM_DOWN, &adapter->state);
#ifndef NO_PTP
	if (module_enable_ptp) {
		/* should wait ptp timeout */
		while (test_bit(__RNPM_PTP_TX_IN_PROGRESS,
				&adapter->state))
			usleep_range(10000, 20000);
		cancel_work_sync(&adapter->tx_hwtstamp_work);
	}
#endif
	cancel_work_sync(&adapter->service_task);
	/* muset wait service task over, we can free irq */
	while (test_bit(__RNPM_SERVICE_IN_PROGRESS, &adapter->state))
		usleep_range(10000, 20000);
#ifdef RNPM_PF1_ONE_PORT
	if ((adapter->port == 0 && rnpm_is_pf1(adapter->pf_adapter->pdev)))
		rnpm_sysfs_exit(adapter);
#else
	if (pf1_use_port) {
		if ((rnpm_is_pf1(adapter->pf_adapter->pdev) &&
		     adapter->port < pf1_use_port) ||
		    rnpm_is_pf1(adapter->pf_adapter->pdev) == 0)
			rnpm_sysfs_exit(adapter);
	} else {
		rnpm_sysfs_exit(adapter);
	}
#endif

	if (adapter->netdev_registered) {
#ifdef RNPM_PF1_ONE_PORT
		if ((adapter->port == 0 &&
		     rnpm_is_pf1(adapter->pf_adapter->pdev)))
			unregister_netdev(netdev);
#else
		if (pf1_use_port) {
			if ((rnpm_is_pf1(adapter->pf_adapter->pdev) &&
			     adapter->port < pf1_use_port) ||
			    rnpm_is_pf1(adapter->pf_adapter->pdev) == 0)
				unregister_netdev(netdev);
		} else {
			unregister_netdev(netdev);
		}
#endif
		adapter->netdev_registered = false;
	}
	rnpm_clear_interrupt_scheme(adapter);
	free_netdev(netdev);
	return 0;
}

static void rnpm_fix_queue_number(struct rnpm_hw *hw)
{
	struct rnpm_adapter *adapter = hw->back;
	u32 count;

	/* total_queue_pair_cnts equal to 64 on n400 tp & n10 4x10 board , when
	 * nic-mode 3 and adapter cnt 2
	 */
	if ((rnpm_info_tbl[adapter->pf_adapter->board_type]->adapter_cnt ==
	     2) &&
	    (hw->mode == MODE_NIC_MODE_4PORT)) {
		if ((adapter->pf_adapter->board_type == board_n10) ||
		    (adapter->pf_adapter->board_type == board_n400_4x1G))
			rnpm_info_tbl[adapter->pf_adapter->board_type]
				->total_queue_pair_cnts = 64;
	}

	count = rnpm_info_tbl[adapter->pf_adapter->board_type]
			->total_queue_pair_cnts /
		rnpm_info_tbl[adapter->pf_adapter->board_type]->adapter_cnt;

	if (count != adapter->max_ring_pair_counts) {
		e_h_info("reset max_ring_pair_counts from %d to %d\n",
			 adapter->max_ring_pair_counts, count);
		adapter->max_ring_pair_counts = count;
	}

#ifdef RNPM_MAX_RINGS
	adapter->max_ring_pair_counts = RNPM_MAX_RINGS;
#endif
}

static int check_valid_mode(struct rnpm_pf_adapter *pf_adapter)
{
	int err = 0;

	switch (pf_adapter->board_type) {
	case board_n10:
	case board_n400_4x1G:
		return 0;
	default:
		rnpm_dbg("board mode error\n");
		err = -1;
		break;
	}
	return err;
}

static int rnpm_init_msix_pf_adapter(struct rnpm_pf_adapter *pf_adapter)
{
	int total_msix_counts;
	int valid_port = Hamming_weight_1(pf_adapter->port_valid);
	int vector, vectors = 0, err, max_msix_counts_per_port;
	int min_vectors = valid_port + 1;
	int remain, i;
#ifdef NO_PCI_MSIX_COUNT
	total_msix_counts = 64;
#else
	total_msix_counts = pci_msix_vec_count(pf_adapter->pdev);
#endif
#ifdef CONFIG_MXGBEM_MSIX_COUNT
	total_msix_counts = CONFIG_MXGBEM_MSIX_COUNT;
#endif
	if (pf_msix_counts_set)
		total_msix_counts =
			pf_msix_counts_set < 5 ? 5 : pf_msix_counts_set;
	/* reserve one for mailbox */
	total_msix_counts -= 1;
	total_msix_counts = min_t(int,
				  rnpm_info_tbl[pf_adapter->board_type]
					  ->total_queue_pair_cnts,
				  total_msix_counts);
	max_msix_counts_per_port = total_msix_counts / valid_port;
	remain = total_msix_counts - max_msix_counts_per_port * valid_port;

	/* calc max msix for each port */
	for (i = 0; i < MAX_PORT_NUM; i++) {
		if (pf_adapter->port_valid & (1 << i)) {
			if (remain) {
				pf_adapter->max_msix_counts[i] =
					max_msix_counts_per_port + 1;
				remain--;
			} else {
				pf_adapter->max_msix_counts[i] =
					max_msix_counts_per_port;
			}
		}
		pf_adapter->max_msix_counts[i] =
			min_t(int, pf_adapter->max_msix_counts[i],
			      num_online_cpus());
		rnpm_dbg("port %d, max_msix_counts %d\n", i,
			 pf_adapter->max_msix_counts[i]);
		vectors += pf_adapter->max_msix_counts[i];
	}
	/* mbx use vector 0 */
	pf_adapter->other_irq = 0;
	vectors += 1;

	pf_adapter->msix_entries =
		kcalloc(vectors, sizeof(struct msix_entry), GFP_KERNEL);
	if (!pf_adapter->msix_entries) {
		dev_err(&pf_adapter->pdev->dev,
			"alloc msix_entries faild!\n");
		return -ENOMEM;
	}

	for (vector = 0; vector < vectors; vector++)
		pf_adapter->msix_entries[vector].entry = vector;

	err = pci_enable_msix_range(pf_adapter->pdev,
				    pf_adapter->msix_entries, min_vectors,
				    vectors);

	if (err < 0) {
		dev_err(&pf_adapter->pdev->dev,
			"pci_enable_msix faild: req:%d err:%d\n", vectors,
			err);
		kfree(pf_adapter->msix_entries);
		pf_adapter->msix_entries = NULL;
		return -EINVAL;
	} else if ((err > 0) && (err != vectors)) {
		total_msix_counts = err;
		total_msix_counts -= 1; /* reserve one for mailbox */

		max_msix_counts_per_port = total_msix_counts / valid_port;
		remain = total_msix_counts -
			 max_msix_counts_per_port * valid_port;
		/* decide max msix for each port */
		for (i = 0; i < MAX_PORT_NUM; i++) {
			if (pf_adapter->port_valid & (1 << i)) {
				if (remain) {
					pf_adapter->max_msix_counts[i] =
						max_msix_counts_per_port +
						1;
					remain--;
				} else {
					pf_adapter->max_msix_counts[i] =
						max_msix_counts_per_port;
				}
			}
			pf_adapter->max_msix_counts[i] =
				min_t(int, pf_adapter->max_msix_counts[i],
				      num_online_cpus());
			dev_info(&pf_adapter->pdev->dev,
				 "port %d, max_msix_counts %d\n", i,
				 pf_adapter->max_msix_counts[i]);
		}
	}
	return 0;
}

static int rnpm_rm_msix_pf_adapter(struct rnpm_pf_adapter *pf_adapter)
{
	pci_disable_msix(pf_adapter->pdev);
	if (pf_adapter->msix_entries)
		kfree(pf_adapter->msix_entries);
	pf_adapter->msix_entries = 0;
	return 0;
}

int rnpm_set_clause73_autoneg_enable(struct net_device *netdev, int enable)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);

	if (!adapter)
		return -EINVAL;

	if (test_bit(__RNPM_DOWN, &adapter->state) ||
	    test_bit(__RNPM_RESETTING, &adapter->state))
		return -EBUSY;

	return rnpm_hw_set_clause73_autoneg_enable(&adapter->hw, enable);
}
EXPORT_SYMBOL(rnpm_set_clause73_autoneg_enable);

static void rnpm_rm_mbx_irq(struct rnpm_pf_adapter *pf_adapter)
{
	pf_adapter->hw.mbx.ops.configure(
		&pf_adapter->hw,
		pf_adapter->msix_entries[pf_adapter->other_irq].entry,
		false);

#ifdef RNPM_DISABLE_IRQ
	return;
#endif
	free_irq(pf_adapter->msix_entries[pf_adapter->other_irq].vector,
		 pf_adapter);
	pf_adapter->hw.mbx.irq_enabled = false;
}

static int rnpm_request_mbx_irq(struct rnpm_pf_adapter *pf_adapter)
{
	int err = 0;

#ifdef RNPM_DISABLE_IRQ
	return 0;
#endif

	snprintf(pf_adapter->name, 20, "rnpm%d%d-other%d",
		 rnpm_is_pf1(pf_adapter->pdev), pf_adapter->bd_number,
		 pf_adapter->other_irq);
	err = request_irq(
		pf_adapter->msix_entries[pf_adapter->other_irq].vector,
		rnpm_msix_other, 0, pf_adapter->name, pf_adapter);

	if (err) {
		err = -1;
		goto err_mbx_irq;
	}

	pf_adapter->hw.mbx.ops.configure(
		&pf_adapter->hw,
		pf_adapter->msix_entries[pf_adapter->other_irq].entry,
		true);
	pf_adapter->hw.mbx.irq_enabled = true;
	dbg_dev("enable mbx-other irq. %s vec:%d", pf_adapter->name,
		pf_adapter->msix_entries[pf_adapter->other_irq].vector);
err_mbx_irq:
	return err;
}

static int rnpm_add_pf_adapter(struct pci_dev *pdev,
			       struct rnpm_pf_adapter **ppf_adapter,
			       const struct pci_device_id *id)
{
	struct rnpm_pf_adapter *pf_adapter;
#ifdef FT_PADDING
	u32 data;
#endif
	u8 __iomem *hw_addr_bar0 = 0;
	static int pf0_cards_found;
	static int pf1_cards_found;
	struct rnpm_hw *hw;
	struct rnpm_info *ii = rnpm_info_tbl[(int)id->driver_data];
	int i, err = 0;

	pf_adapter =
		devm_kzalloc(&pdev->dev, sizeof(*pf_adapter), GFP_KERNEL);
	if (pf_adapter) {
		*ppf_adapter = pf_adapter;
	} else {
		err = -ENOMEM;
		goto err_pf_alloc;
	}

	pf_adapter->board_type = (int)id->driver_data;
	pf_adapter->pdev = pdev;
	pci_set_drvdata(pdev, pf_adapter);
#define RNPM_NIC_BAR0 (0)
	hw_addr_bar0 = pcim_iomap(pdev, RNPM_NIC_BAR0, 0);
	if (!hw_addr_bar0) {
		dev_err(&pdev->dev, "pcim_iomap bar%d faild!\n", 0);
		goto err_ioremap0;
	}
#ifdef FIX_VF_BUG
	rnpm_wr_reg(hw_addr_bar0 +
			    (0x7982fc &
			     (pci_resource_len(pdev, RNPM_NIC_BAR0) - 1)),
		    0);
#else
	rnpm_wr_reg(hw_addr_bar0 +
			    (0x7982fc &
			     (pci_resource_len(pdev, RNPM_NIC_BAR0) - 1)),
		    1);
#endif
	pf_adapter->hw_bar0 = hw_addr_bar0;
	hw = &pf_adapter->hw;

	if (pci_resource_len(pdev, 0) == 8 * 1024 * 1024)
		hw->rpu_addr = pf_adapter->hw_bar0;
	else
		hw->rpu_addr = NULL;
	dbg_dev("[bar0]:%p %llx len=%d MB rpu:%p\n", pf_adapter->hw_bar0,
		(unsigned long long)pci_resource_start(pdev, 0),
		(int)pci_resource_len(pdev, 0) / 1024 / 1024,
		hw->rpu_addr);

#define RNPM_NIC_BAR4 (4)
	pf_adapter->hw_addr4 = pf_adapter->hw_addr =
		pcim_iomap(pdev, RNPM_NIC_BAR4, 0);
	if (!pf_adapter->hw_addr) {
		err = -EIO;
		goto err_ioremap4;
	}

	if (rnpm_is_pf1(pdev))
		pf_adapter->bd_number = pf0_cards_found++;
	else
		pf_adapter->bd_number = pf1_cards_found++;
	mutex_init(&pf_adapter->mbx_lock);
	hw = &pf_adapter->hw;
#ifdef FIX_VF_BUG
	if (rnpm_is_pf1(pdev)) {
		hw->hw_addr = pf_adapter->hw_addr4 + 0x100000;
		pf_adapter->hw_addr = hw->hw_addr;
		hw->ring_msix_base = hw->hw_addr + 0xa4000 + 0x200;
	} else {
		hw->hw_addr = pf_adapter->hw_addr;
		hw->ring_msix_base = hw->hw_addr + 0xa4000;
	}
#else
	hw->hw_addr = pf_adapter->hw_addr;
	hw->ring_msix_base = hw->hw_addr + 0xa4000;
#endif

	hw->hw_kram_addr = pf_adapter->rpu_addr;
	hw->hw_mpe_addr = pf_adapter->rpu_addr;

	hw->pdev = pf_adapter->pdev;
	hw->mbx.lock = &pf_adapter->mbx_lock;
	rnpm_init_mbx_params_pf(hw);
	memcpy(&hw->mbx.ops, ii->mbx_ops, sizeof(hw->mbx.ops));
	spin_lock_init(&pf_adapter->vlan_setup_lock);
	spin_lock_init(&pf_adapter->drop_setup_lock);
	spin_lock_init(&pf_adapter->dummy_setup_lock);
	spin_lock_init(&pf_adapter->pf_setup_lock);
	spin_lock_init(&pf_adapter->priv_flags_lock);
	rnpm_mbx_pf_link_event_enable_nolock(hw, 0);
	if (rnpm_mbx_get_capability(hw, ii)) {
		dev_err(&pdev->dev, "rnp_mbx_get_capablity faild!\n");
		err = -EIO;
		goto err_mbx_capability;
	}
	pf_adapter->port_valid = hw->lane_mask;
	if (hw->port_ids != 0xffffffff)
		pf_adapter->port_names = hw->port_ids;
	else
		pf_adapter->port_names = port_names_pf0;

	pf_adapter->adapter_cnt = ii->adapter_cnt;
	pf_adapter->hw.axi_mhz = hw->axi_mhz;
	pf_adapter->hw.ncsi_en = hw->ncsi_en;
	pf_adapter->hw.wol = hw->wol;
	pf_adapter->hw.fw_version = hw->fw_version;
	/* some global var init here */
	spin_lock_init(&pf_adapter->key_setup_lock);
	pf_adapter->default_rx_ring = 0;
	spin_lock_init(&pf_adapter->mc_setup_lock);
	pf_adapter->mc_location = rnpm_mc_location_nic;
	switch (pf_adapter->hw.mode) {
	case MODE_NIC_MODE_1PORT:
		pf_adapter->mcft_size = 128;
		break;
	case MODE_NIC_MODE_2PORT:
	case MODE_NIC_MODE_4PORT:
		pf_adapter->mcft_size = 8;
		break;
	default:
		pf_adapter->mcft_size = 128;
		break;
	}
	pf_adapter->cpu_l3_cache_size = rnpm_get_cpu_l3_cache_size();
	pf_adapter->mc_filter_type = rnpm_mc_filter_type0;
	spin_lock_init(&pf_adapter->vlan_filter_lock);

	for (i = 0; i < MAX_PORT_NUM; i++) {
		pf_adapter->vlan_filter_status[i] = 1;
		/* broadcast bypass should always set */
		pf_adapter->fctrl[i] = RNPM_FCTRL_BROADCASE_BYPASS;
	}
	pf_adapter->vlan_status_true = 1;

	pf_adapter->priv_flags = 0;
#ifdef FT_PADDING
	rnpm_dbg("ft padding status on\n");
	pf_adapter->priv_flags |= RNPM_PRIV_FLAG_PCIE_CACHE_ALIGN_PATCH;
	data = rd32(pf_adapter, RNPM_DMA_CONFIG);
	SET_BIT(padding_enable, data);
	wr32(pf_adapter, RNPM_DMA_CONFIG, data);
#endif
#ifdef RNPM_CMCC_PTP_EXTCLOCK
	/* CMCC custom, used ext clock, select lan0 by ref clock */
	if (!rnpm_is_pf1(pdev))
		wr32(pf_adapter, RNPM_ETH_PTP_TIMESTAMP_SEL, 0);
#endif

	err = check_valid_mode(pf_adapter);
	if (err)
		goto err_msix;

	err = rnpm_init_msix_pf_adapter(pf_adapter);

	if (err)
		goto err_msix;

	/* reset card */
	err = rnpm_reset_pf(pf_adapter);
	if (err)
		goto err_reset;

	err = rnpm_request_mbx_irq(pf_adapter);
	if (err)
		goto err_mbx_irq;
	timer_setup(&pf_adapter->service_timer, rnpm_pf_service_timer, 0);
	INIT_WORK(&pf_adapter->service_task, rnpm_pf_service_task);
	return 0;
err_mbx_irq:
	dev_err(&pdev->dev, "error: err_mbx_irq!\n");
	rnpm_rm_mbx_irq(pf_adapter);
err_reset:
	dev_err(&pdev->dev, "error: err_reset!\n");
	rnpm_rm_mbx_irq(pf_adapter);
	rnpm_rm_msix_pf_adapter(pf_adapter);

err_msix:
	dev_err(&pdev->dev, "error: err_msix!\n");
err_mbx_capability:
	pcim_iounmap(pdev, pf_adapter->hw_addr4);
err_ioremap0:
err_ioremap4:
	devm_kfree(&pdev->dev, pf_adapter);
	dev_err(&pdev->dev, "error: err_ioremap4!\n");
err_pf_alloc:
	dev_err(&pdev->dev, "error: err_pf_alloc!\n");
	return err;
}

static int rnpm_rm_pf_adapter(struct pci_dev *pdev,
			      struct rnpm_pf_adapter *pf_adapter)
{
	if (pf_adapter->service_timer.function)
		del_timer_sync(&pf_adapter->service_timer);
	cancel_work_sync(&pf_adapter->service_task);

	rnpm_rm_mbx_irq(pf_adapter);
	rnpm_rm_msix_pf_adapter(pf_adapter);

	if (pf_adapter->rpu_inited) {
		rnpm_rpu_mpe_stop(pf_adapter);
		pf_adapter->rpu_inited = 0;
	}

	if (pf_adapter->hw.ncsi_en)
		rnpm_mbx_probe_stat_set(pf_adapter, MBX_REMOVE);

	rnpm_wr_reg(pf_adapter->hw_bar0 +
			    (0x7982fc &
			     (pci_resource_len(pdev, RNPM_NIC_BAR0) - 1)),
		    1);
	if (pf_adapter->hw_bar0)
		pcim_iounmap(pdev, pf_adapter->hw_bar0);
	if (pf_adapter->hw_addr4)
		pcim_iounmap(pdev, pf_adapter->hw_addr4);
	if (pf_adapter->hw.mbx.reply_dma)
		dma_free_coherent(&pdev->dev,
				  pf_adapter->hw.mbx.reply_dma_size,
				  pf_adapter->hw.mbx.reply_dma,
				  pf_adapter->hw.mbx.reply_dma_phy);
	if (pf_adapter)
		devm_kfree(&pdev->dev, pf_adapter);
	return 0;
}

static int rnpm_add_adpater(struct pci_dev *pdev,
			    const struct rnpm_info *ii,
			    struct rnpm_adapter **padapter,
			    struct rnpm_pf_adapter *pf_adapter, int port,
			    int msix_offset, int port_name)
{
	struct rnpm_adapter *adapter = NULL;
	struct net_device *netdev;
	struct rnpm_hw *hw;
	unsigned int queues;
	unsigned int indices;
	int adapter_cnt = pf_adapter->adapter_cnt;
	int i, err = 0;

	queues = ii->total_queue_pair_cnts / adapter_cnt;
	indices = queues;
	pr_info("add adapter queues:%d table %d\n", queues,
		pf_adapter->max_msix_counts[port]);

	netdev = alloc_etherdev_mq(sizeof(struct rnpm_adapter), indices);
	if (!netdev) {
		dev_err(&pf_adapter->pdev->dev, "alloc etherdev errors\n");
		return -ENOMEM;
	}

	adapter = netdev_priv(netdev);
	adapter->netdev = netdev;
	adapter->pdev = pdev;

	adapter->bd_number = pf_adapter->bd_number;
	adapter->port = port;
	adapter->lane = port;
#ifdef RNPM_WITH_IPC_OFF
	adapter->mac_ipc_off = true;
#endif
	adapter->max_ring_pair_counts = queues;
	adapter->vector_off = msix_offset;

	adapter->max_msix_counts = pf_adapter->max_msix_counts[port];
	adapter->max_q_vectors = adapter->max_msix_counts;

	adapter->layer2_count_max = ii->total_layer2_count / adapter_cnt;
	adapter->layer2_offset = adapter->layer2_count_max * adapter->port;
	adapter->tuple_5_count_max = ii->total_tuple5_count / adapter_cnt;
	adapter->tuple_5_offset =
		adapter->tuple_5_count_max * adapter->port;
	adapter->priv_flags = pf_adapter->priv_flags;

#ifdef RNPM_NAME_BY_LANES
	snprintf(adapter->name, sizeof(netdev->name), "%s%ds%df%d",
		 rnpm_port_name, pdev->bus->number, rnpm_is_pf1(pdev),
		 adapter->port);
#else
	snprintf(adapter->name, sizeof(netdev->name), "%s%ds%df%d",
		 rnpm_port_name, pdev->bus->number, rnpm_is_pf1(pdev),
		 port_name);
#endif
	if (padapter) {
		*padapter = adapter;
		(*padapter)->pf_adapter = pf_adapter;
	}
	hw = &adapter->hw;
	hw->back = adapter;
	hw->nr_lane = hw->num = adapter->port;
	hw->port_idx = pf_adapter->hw.port_id[hw->nr_lane];
	hw->pdev = pdev;
	hw->mode = pf_adapter->hw.mode;
	hw->lane_mask = pf_adapter->hw.lane_mask;
	hw->fw_version = pf_adapter->hw.fw_version;
	hw->fw_uid = pf_adapter->hw.fw_uid;
	hw->phy.media_type = hw->phy_type = pf_adapter->hw.phy_type;
	hw->axi_mhz = pf_adapter->hw.axi_mhz;
	hw->phy.id = pf_adapter->hw.phy.id;
	hw->single_lane_link_evt_ctrl_ablity =
		pf_adapter->hw.single_lane_link_evt_ctrl_ablity;
	hw->ncsi_rar_entries = pf_adapter->hw.ncsi_rar_entries;
	hw->ncsi_en = pf_adapter->hw.ncsi_en;
	hw->fw_lldp_ablity = pf_adapter->hw.fw_lldp_ablity;
	hw->max_speed_1g = pf_adapter->hw.max_speed_1g;
	if (pf_adapter->hw.is_sgmii_bitmaps_valid)
		hw->is_sgmii = !!(pf_adapter->hw.is_sgmii_bitmaps &
				  BIT(hw->nr_lane));
	else
		hw->is_sgmii = pf_adapter->hw.is_sgmii;
	adapter->wol = pf_adapter->hw.wol;
	memcpy(&hw->mbx, &pf_adapter->hw.mbx, sizeof(pf_adapter->hw.mbx));
	memcpy(&hw->mac.ops, &pf_adapter->hw.mac.ops,
	       sizeof(pf_adapter->hw.mac.ops));

	adapter->msg_enable =
		netif_msg_init(debug, NETIF_MSG_DRV
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

	if (rnpm_is_pf1(pdev))
		hw->pfvfnum = PF_NUM(1);
	else
		hw->pfvfnum = PF_NUM(0);

	/* adapter hw->mode to decide flags */
	switch (hw->mode) {
	case MODE_NIC_MODE_1PORT_40G:
	case MODE_NIC_MODE_1PORT:
		adapter->flags &= (~RNPM_FLAG_MUTIPORT_ENABLED);
		break;
	case MODE_NIC_MODE_2PORT:
		adapter->flags |= RNPM_FLAG_MUTIPORT_ENABLED;
		break;
	case MODE_NIC_MODE_4PORT:
		adapter->flags |= RNPM_FLAG_MUTIPORT_ENABLED;
		break;
	default:
		adapter->flags |= RNPM_FLAG_MUTIPORT_ENABLED;
		break;
	}

	if (adapter->flags & RNPM_FLAG_MUTIPORT_ENABLED)
		netdev->dev_port = port_name;
	SET_NETDEV_DEV(netdev, &pdev->dev);
	adapter->portid_of_card = port_name;
	hw->default_rx_queue = 0;
	hw->rss_type = ii->rss_type;
#ifdef FIX_VF_BUG
	if (hw->pfvfnum) {
		hw->hw_addr = pf_adapter->hw_addr4 + 0x100000;
		pf_adapter->hw_addr = hw->hw_addr;
		hw->ring_msix_base = hw->hw_addr + 0xa4000 + 0x200;
	} else {
		hw->hw_addr = pf_adapter->hw_addr;
		hw->ring_msix_base = hw->hw_addr + 0xa4000;
	}
#else
	hw->hw_addr = pf_adapter->hw_addr;
	hw->ring_msix_base = hw->hw_addr + 0xa4000;
#endif
	hw->rpu_addr = pf_adapter->hw.rpu_addr;
	hw->hw_kram_addr = hw->rpu_addr;
	hw->hw_mpe_addr = hw->rpu_addr;

	rnpm_fix_queue_number(hw);
	/* get version */
	hw->dma_version = rd32(hw, RNPM_DMA_VERSION);
	pr_info("%s %s: dma version:0x%x, nic version:0x%x, pfvfnum:0x%x lane%d %p sgmii:%d\n",
		adapter->name, pci_name(pdev), hw->dma_version,
		rd32(hw, RNPM_TOP_NIC_VERSION), hw->pfvfnum, hw->nr_lane,
		hw, hw->is_sgmii);
	rnpm_assign_netdev_ops(netdev);
	strncpy(netdev->name, adapter->name, sizeof(netdev->name) - 1);

	/* Setup hw api */
	memcpy(&hw->mac.ops, ii->mac_ops, sizeof(hw->mac.ops));
	/* PHY */
	memcpy(&hw->phy.ops, ii->phy_ops, sizeof(hw->phy.ops));
	hw->phy.sfp_type = rnpm_sfp_type_unknown;
	/* PCS */
	memcpy(&hw->pcs.ops, ii->pcs_ops, sizeof(hw->pcs.ops));
	ii->get_invariants(hw);
	err = rnpm_sw_init(adapter);
	if (err) {
		err = -EIO;
		goto err_sw_init;
	}

	/* reset_hw fills in the perm_addr as well */
	err = hw->mac.ops.reset_hw(hw);
	if (err) {
		e_dev_err("HW Init failed: %d\n", err);
		err = -EIO;
		goto err_sw_init;
	}
	/* MTU range: 68 - 9710 */
	netdev->min_mtu = RNPM_MIN_MTU;
	netdev->max_mtu =
		RNPM_MAX_JUMBO_FRAME_SIZE - (ETH_HLEN + 2 * ETH_FCS_LEN);
	if (hw->feature_flags & RNPM_NET_FEATURE_SG)
		netdev->features |= NETIF_F_SG;
	if (hw->feature_flags & RNPM_NET_FEATURE_TSO)
		netdev->features |= NETIF_F_TSO | NETIF_F_TSO6;
	if (hw->feature_flags & RNPM_NET_FEATURE_RX_HASH)
		netdev->features |= NETIF_F_RXHASH;
	if (hw->feature_flags & RNPM_NET_FEATURE_RX_CHECKSUM)
		netdev->features |= NETIF_F_RXCSUM;
	if (hw->feature_flags & RNPM_NET_FEATURE_TX_CHECKSUM)
		netdev->features |= NETIF_F_HW_CSUM | NETIF_F_SCTP_CRC;

	netdev->features |= NETIF_F_HIGHDMA;
	netdev->gso_partial_features = RNPM_GSO_PARTIAL_FEATURES;
	netdev->features |= NETIF_F_GSO_PARTIAL |
			    RNPM_GSO_PARTIAL_FEATURES;

	netdev->hw_features |= netdev->features;

	if (hw->feature_flags & RNPM_NET_FEATURE_VLAN_FILTER)
		netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_FILTER;
	if (hw->feature_flags & RNPM_NET_FEATURE_VLAN_OFFLOAD)
		netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_RX |
				       NETIF_F_HW_VLAN_CTAG_TX;
	netdev->hw_features |= NETIF_F_RXALL;
	if (hw->feature_flags & RNPM_NET_FEATURE_RX_NTUPLE_FILTER)
		netdev->hw_features |= NETIF_F_NTUPLE;
	if (hw->feature_flags & RNPM_NET_FEATURE_RX_FCS)
		netdev->hw_features |= NETIF_F_RXFCS;

	netdev->vlan_features |= netdev->features | NETIF_F_TSO_MANGLEID;
	netdev->hw_enc_features |= netdev->vlan_features;
	netdev->mpls_features |= NETIF_F_HW_CSUM;

	if (hw->feature_flags & RNPM_NET_FEATURE_VLAN_FILTER)
		netdev->features |= NETIF_F_HW_VLAN_CTAG_FILTER;
	if (hw->feature_flags & RNPM_NET_FEATURE_VLAN_OFFLOAD)
		netdev->features |= NETIF_F_HW_VLAN_CTAG_RX |
				    NETIF_F_HW_VLAN_CTAG_TX;
	netdev->priv_flags |= IFF_UNICAST_FLT;
	netdev->priv_flags |= IFF_SUPP_NOFCS;
	if (adapter->flags2 & RNPM_FLAG2_RSC_CAPABLE)
		netdev->hw_features |= NETIF_F_LRO;
	netdev->priv_flags |= IFF_UNICAST_FLT;
	netdev->priv_flags |= IFF_SUPP_NOFCS;
	if (adapter->flags2 & RNPM_FLAG2_RSC_ENABLED)
		netdev->features |= NETIF_F_LRO;
	eth_hw_addr_set(netdev, hw->mac.perm_addr);
	memcpy(netdev->perm_addr, hw->mac.perm_addr, netdev->addr_len);
	pr_info("set dev_addr:%pM\n", netdev->dev_addr);

	if (!is_valid_ether_addr(netdev->dev_addr)) {
		e_dev_err("invalid MAC address\n");
		err = -EIO;
		goto err_sw_init;
	}
	ether_addr_copy(hw->mac.addr, hw->mac.perm_addr);

	timer_setup(&adapter->service_timer, rnpm_service_timer, 0);

#ifndef NO_PTP
	if (module_enable_ptp) {
		adapter->flags2 |= RNPM_FLAG2_PTP_ENABLED;
		if (adapter->flags2 & RNPM_FLAG2_PTP_ENABLED) {
			adapter->tx_timeout_factor = 10;
			INIT_WORK(&adapter->tx_hwtstamp_work,
				  rnpm_tx_hwtstamp_work);
		}
	}
#endif

	INIT_WORK(&adapter->service_task, rnpm_service_task);
	clear_bit(__RNPM_SERVICE_SCHED, &adapter->state);

	err = rnpm_init_interrupt_scheme(adapter);
	if (err) {
		err = -EIO;
		goto err_interrupt_scheme;
	}

	/* reset the hardware with the new settings */
	err = hw->mac.ops.start_hw(hw);

	adapter->pf_adapter->force_10g_1g_speed_ablity =
		rnpm_is_pf1(pdev) ? !!force_speed_ablity_pf1 :
				    !!force_speed_ablity_pf0;
	if (hw->max_speed_1g)
		adapter->pf_adapter->force_10g_1g_speed_ablity = 0;

	if (adapter->pf_adapter->force_10g_1g_speed_ablity)
		pf_adapter->priv_flags |= RNPM_PRIV_FLAG_FORCE_SPEED_ABLIY;

	/* Disable fiber force speed */
	if (hw->max_speed_1g == 0)
		rnpm_mbx_force_speed(hw, 0);

	strncpy(netdev->name, adapter->name, sizeof(netdev->name) - 1);
	if (fix_eth_name) {
		if (!(adapter->flags & RNPM_FLAG_MUTIPORT_ENABLED))
			snprintf(adapter->name, sizeof(netdev->name),
				 "rnp%d%d", rnpm_is_pf1(pdev),
				 adapter->bd_number);
		else
			snprintf(adapter->name, sizeof(netdev->name),
				 "rnpm%d%d%d", rnpm_is_pf1(pdev),
				 adapter->bd_number, adapter->port);
		strncpy(netdev->name, adapter->name,
			sizeof(netdev->name) - 1);
	} else {
		strcpy(netdev->name, "eth%d");
	}
#ifdef RNPM_PF1_ONE_PORT
	if ((adapter->port == 0 &&
	     rnpm_is_pf1(adapter->pf_adapter->pdev))) {
		err = register_netdev(netdev);
		if (err) {
			err = -EIO;
			rnpm_err("register_netdev faild! err code %x\n",
				 err);
			goto err_register;
		}
	}
#else
	if (pf1_use_port) {
		if ((rnpm_is_pf1(adapter->pf_adapter->pdev) &&
		     adapter->port < pf1_use_port) ||
		    rnpm_is_pf1(adapter->pf_adapter->pdev) == 0) {
			err = register_netdev(netdev);
			if (err) {
				err = -EIO;
				rnpm_err(
					"register_netdev faild! err code %x\n",
					err);
				goto err_register;
			}
		}
	} else {
		err = register_netdev(netdev);
		if (err) {
			err = -EIO;
			rnpm_err("register_netdev faild! err code %x\n",
				 err);
			goto err_register;
		}
	}
#endif
	adapter->netdev_registered = true;
	if (hw->ncsi_en)
		rnpm_control_mac_rx(adapter, true);

	/* carrier off reporting is important to ethtool even BEFORE open */
	netif_carrier_off(netdev);
	if (adapter->flags & RNPM_FLAG_SRIOV_ENABLED) {
		e_info(probe, "IOV is enabled with %d VFs\n",
		       adapter->num_vfs);
		for (i = 0; i < adapter->num_vfs; i++)
			rnpm_vf_configuration(pdev, (i | 0x10000000));
	}

	if (rnpm_mbx_lldp_status_get(hw) == 1)
		adapter->priv_flags |= RNPM_PRIV_FLAG_LLDP_EN_STAT;
#ifdef RNPM_PF1_ONE_PORT
	if ((adapter->port == 0 &&
	     rnpm_is_pf1(adapter->pf_adapter->pdev))) {
		if (rnpm_sysfs_init(adapter, port))
			e_err(probe,
			      "failed to allocate sysfs resources\n");
		rnpm_dbg_adapter_init(adapter);
	}
#else
	if (pf1_use_port) {
		if ((rnpm_is_pf1(adapter->pf_adapter->pdev) &&
		     adapter->port < pf1_use_port) ||
		    rnpm_is_pf1(adapter->pf_adapter->pdev) == 0) {
			if (rnpm_sysfs_init(adapter, port))
				e_err(probe,
				      "failed to allocate sysfs resources\n");
			rnpm_dbg_adapter_init(adapter);
		}
	} else {
		if (rnpm_sysfs_init(adapter, port))
			e_err(probe,
			      "failed to allocate sysfs resources\n");
		rnpm_dbg_adapter_init(adapter);
	}
#endif
	return 0;
err_register:
	e_dev_err("error: err_register err=%d\n", err);
	rnpm_clear_interrupt_scheme(adapter);
err_interrupt_scheme:
	e_dev_err("error: err_interrupt_scheme err=%d\n", err);
	if (adapter->service_timer.function)
		del_timer_sync(&adapter->service_timer);
err_sw_init:
	e_dev_err("error: err_sw_init err=%d\n", err);
	rnpm_disable_sriov(adapter);
	adapter->flags2 &= ~RNPM_FLAG2_SEARCH_FOR_SFP;
	free_netdev(netdev);
	adapter->netdev_registered = false;

	return err;
}

static int rnpm_check_rpu_status(struct rnpm_pf_adapter *pf_adapter)
{
	if (pf_adapter->hw.rpu_addr == NULL)
		return 0;
	if ((pf_adapter->pdev->device & 0xff00) == 0x1c00)
		return 1;
	if (pf_adapter->hw.rpu_availble)
		return 1;
	return 0;
}

/**
 * rnpm_probe - Device Initialization Routine
 * @pdev: PCI device information struct
 * @ent: entry in rnpm_pci_tbl
 *
 * Returns 0 on success, negative on failure
 *
 * rnpm_probe initializes an adapter identified by a pci_dev structure.
 * The OS initialization, configuring of the adapter private structure,
 * and a hardware reset occur.
 **/
static int rnpm_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct rnpm_pf_adapter *pf_adapter;
	const struct rnpm_info *ii;
	int i = 0, vector_idx = 0, err;
	int vector_idx_new, port_name, port_name_new, lane_num;
	int valid_port;
	u32 port_valid;

	/* Catch broken hardware that put the wrong VF device ID in
	 * the PCIe SR-IOV capability.
	 */
	if (pdev->is_virtfn) {
		WARN(1, "%s (%x:%x) should not be a VF!\n", pci_name(pdev),
		     pdev->vendor, pdev->device);
		return -EINVAL;
	}
	/*not support bus reset*/
	pdev->dev_flags |= PCI_DEV_FLAGS_NO_BUS_RESET;
	err = pci_enable_device_mem(pdev);
	if (err) {
		dev_err(&pdev->dev, "pci_enable_device_mem failed 0x%x\n",
			err);
		return err;
	}
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
	err = pci_request_mem_regions(pdev, rnpm_driver_name);
	if (err) {
		dev_err(&pdev->dev,
			"pci_request_selected_regions failed 0x%x\n", err);
		goto err_pci_reg;
	}
	pci_set_master(pdev);
	pci_save_state(pdev);

	err = rnpm_add_pf_adapter(pdev, &pf_adapter, id);
	if (err) {
		dev_err(&pdev->dev, "rnpm_add_pf_adapter failed 0x%x\n",
			err);
		goto err_pf_adpater;
	}

	if (rnpm_check_rpu_status(pf_adapter))
		rnpm_rpu_mpe_start(pf_adapter);
	ii = rnpm_info_tbl[pf_adapter->board_type];
	memset(pf_adapter->adapter, 0, sizeof(pf_adapter->adapter));
	if (pf_adapter->adapter_cnt > MAX_PORT_NUM) {
		dev_err(&pdev->dev, "invalid adapt cnt:%d\n",
			pf_adapter->adapter_cnt);
		return -EIO;
	}
	valid_port = Hamming_weight_1(pf_adapter->port_valid);
	port_valid = pf_adapter->port_valid;
	do {
		port_name = -1;
		vector_idx = 1;
		lane_num = 0;
		vector_idx_new = 1;
		for (i = 0, vector_idx = 1; i < pf_adapter->adapter_cnt;
		     i++) {
			if (port_valid & (1 << i)) {
				port_name_new = (pf_adapter->port_names >>
						 (i * 8)) &
						0xff;
				if ((port_name == -1) ||
				    (port_name > port_name_new)) {
					port_name = port_name_new;
					lane_num = i;
					vector_idx_new = vector_idx;
				}
			}
			vector_idx += pf_adapter->max_msix_counts[i];
		}
		err = rnpm_add_adpater(pdev, ii,
				       &pf_adapter->adapter[lane_num],
				       pf_adapter, lane_num,
				       vector_idx_new, port_name);
		if (err) {
			dev_err(&pdev->dev,
				"add adpater %d failed, err=%d\n", i, err);
			goto err_adpater;
		}
		port_valid &= (~(1 << lane_num));
		valid_port--;
	} while (valid_port > 0);

	if (rnpm_card_partially_supported_10g_1g_sfp(pf_adapter)) {
		if (fw_10g_1g_auto_det)
			pf_adapter->priv_flags |=
				RNPM_PRIV_FLAG_FW_10G_1G_AUTO_DETCH_EN;
		rnpm_hw_set_fw_10g_1g_auto_detch(&pf_adapter->hw,
						 fw_10g_1g_auto_det);
	}

	if (pf_adapter->hw.single_lane_link_evt_ctrl_ablity == 0)
		rnpm_mbx_pf_link_event_enable_nolock(&pf_adapter->hw, 1);
	mod_timer(&pf_adapter->service_timer, HZ + jiffies);
	if (pf_adapter->hw.ncsi_en)
		rnpm_mbx_probe_stat_set(pf_adapter, MBX_PROBE);

	if (pf_adapter->rpu_inited &&
	    ((pf_adapter->adapter[0]->hw.feature_flags) &
	     RNPM_NET_FEATURE_TCAM)) {
#ifdef IPSec
		rnpm_esp_tcam_enable(pf_adapter);
		rnpm_set_esp_to_mpe_tcam_rule(pf_adapter);
#endif
	}

	if (!test_bit(RNPM_PF_LINK_CHANGE, &pf_adapter->flags))
		set_bit(RNPM_PF_LINK_CHANGE, &pf_adapter->flags);
	return 0;

err_adpater:
	dev_err(&pdev->dev, "error: err_adpater!\n");
	rnpm_rm_pf_adapter(pdev, pf_adapter);
err_pf_adpater:
	pci_release_mem_regions(pdev);
err_dma:
err_pci_reg:
	dev_err(&pdev->dev, "probe err = %d!\n", err);
	return err;
}

/**
 * rnpm_remove - Device Removal Routine
 * @pdev: PCI device information struct
 *
 * rnpm_remove is called by the PCI subsystem to alert the driver
 * that it should release a PCI device.  The could be caused by a
 * Hot-Plug event, or because the driver is going to be removed from
 * memory.
 **/
static void rnpm_remove(struct pci_dev *pdev)
{
	struct rnpm_pf_adapter *pf_adapter = pci_get_drvdata(pdev);
	int i;

	if (pci_channel_offline(pdev))
		dev_info(&pdev->dev,
			 "%s:%s  card pluged out ,pci-err-stat:%d\n",
			 __func__, pci_name(pdev), pdev->error_state);

	set_bit(__RNPM_DOWN, &pf_adapter->state);

	/* Disable fw send link event to rc */
	rnpm_mbx_pf_link_event_enable_nolock(&pf_adapter->hw, 0);

	while (test_and_set_bit(__RNPM_RESETTING, &pf_adapter->state))
		usleep_range(1000, 2000);
	while (mutex_lock_interruptible(pf_adapter->hw.mbx.lock))
		usleep_range(1000, 2000);
	set_bit(__RNPM_REMOVING, &pf_adapter->state);
	mutex_unlock(pf_adapter->hw.mbx.lock);

	/* must rm in this order */
	for (i = pf_adapter->adapter_cnt - 1; i >= 0; i--) {
		if (rnpm_port_is_valid(pf_adapter, i))
			if (pf_adapter->adapter[i])
				rnpm_rm_adpater(pf_adapter->adapter[i]);
	}

	if (pf_adapter->hw.mbx.ops.configure)
		pf_adapter->hw.mbx.ops.configure(&pf_adapter->hw, 0,
						 false);
	rnpm_rm_pf_adapter(pdev, pf_adapter);
	pci_wait_for_pending_transaction(pdev);
	pci_release_mem_regions(pdev);
	pci_disable_device(pdev);
}

/**
 * rnpm_io_error_detected - called when PCI error is detected
 * @pdev: Pointer to PCI device
 * @state: The current pci connection state
 *
 * This function is called after a PCI bus error affecting
 * this device has been detected.
 */
static pci_ers_result_t rnpm_io_error_detected(struct pci_dev *pdev,
					       pci_channel_state_t state)
{
	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t rnpm_io_slot_reset(struct pci_dev *pdev)
{
	return PCI_ERS_RESULT_NONE;
}

#ifdef CONFIG_PM
static int rnpm_resume(struct pci_dev *pdev)
{
	struct rnpm_pf_adapter *pf_adapter = pci_get_drvdata(pdev);
	struct rnpm_adapter *adapter;
	struct net_device *netdev;
	int i;
	u32 err;

	dev_info(&pdev->dev, "call rnp_resume\n");
	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
	pci_save_state(pdev);
	err = pci_enable_device_mem(pdev);
	if (err) {
		dev_err(&pdev->dev,
			"Cannot enable PCI device from suspend\n");
		return err;
	}
	pci_set_master(pdev);
	pci_wake_from_d3(pdev, false);
	err = rnpm_init_msix_pf_adapter(pf_adapter);
	rnpm_request_mbx_irq(pf_adapter);

	if (pf_adapter->adapter_cnt == 1) {
		wr32(pf_adapter, RNPM_ETH_TCAM_EN, 1);
		wr32(pf_adapter, RNPM_TOP_ETH_TCAM_CONFIG_ENABLE, 1);
		wr32(pf_adapter, RNPM_TCAM_MODE, 2);
#define TCAM_NUM (4096)
		for (i = 0; i < TCAM_NUM; i++) {
			wr32(pf_adapter, RNPM_TCAM_SDPQF(i), 0);
			wr32(pf_adapter, RNPM_TCAM_DAQF(i), 0);
			wr32(pf_adapter, RNPM_TCAM_SAQF(i), 0);
			wr32(pf_adapter, RNPM_TCAM_APQF(i), 0);

			wr32(pf_adapter, RNPM_TCAM_SDPQF_MASK(i), 0);
			wr32(pf_adapter, RNPM_TCAM_DAQF_MASK(i), 0);
			wr32(pf_adapter, RNPM_TCAM_SAQF_MASK(i), 0);
			wr32(pf_adapter, RNPM_TCAM_APQF_MASK(i), 0);
		}
		wr32(pf_adapter, RNPM_TCAM_MODE, 1);
	}

	rnpm_fix_dma_tx_status(pf_adapter);
	for (i = 0; i < pf_adapter->adapter_cnt; i++) {
		if (!rnpm_port_is_valid(pf_adapter, i))
			continue;

		adapter = pf_adapter->adapter[i];
		netdev = adapter->netdev;
		rnpm_reset(adapter);
		rtnl_lock();
		err = rnpm_init_interrupt_scheme(adapter);
		if (!err && netif_running(netdev))
			err = rnpm_open(netdev);

		rtnl_unlock();
		netif_device_attach(netdev);
	}
	if (err)
		return err;
	return 0;
}
#endif /* CONFIG_PM */

__maybe_unused static int __rnpm_shutdown(struct pci_dev *pdev,
					  bool *enable_wake)
{
	struct rnpm_pf_adapter *pf_adapter = pci_get_drvdata(pdev);
	struct rnpm_adapter *adapter;
	int i;
	struct net_device *netdev;
	struct rnpm_hw *hw;
	u32 wufc = 0;
#ifdef CONFIG_PM
	int retval = 0;
#endif

	for (i = pf_adapter->adapter_cnt - 1; i >= 0; i--) {
		if (!rnpm_port_is_valid(pf_adapter, i))
			continue;

		adapter = pf_adapter->adapter[i];
		netdev = adapter->netdev;
		hw = &adapter->hw;
		rtnl_lock();
		netif_device_detach(netdev);
		if (netif_running(netdev)) {
			rnpm_down(adapter);
			rnpm_free_irq(adapter);
			rnpm_free_all_tx_resources(adapter);
			rnpm_free_all_rx_resources(adapter);
		}
		rtnl_unlock();
		rnpm_clear_interrupt_scheme(adapter);
		wufc = adapter->wol;
		if (wufc)
			rnpm_set_rx_mode(netdev);
	}

#ifdef CONFIG_PM
	retval = pci_save_state(pdev);
	if (retval)
		return retval;
#endif
	pci_wake_from_d3(pdev, false);
	*enable_wake = false;
	rnpm_rm_mbx_irq(pf_adapter);
	rnpm_rm_msix_pf_adapter(pf_adapter);

	pci_disable_device(pdev);

	return 0;
}

#ifdef CONFIG_PM
static int rnpm_suspend(struct pci_dev *pdev, pm_message_t state)
{
	int retval;
	bool wake;

	dev_info(&pdev->dev, "call rnp_suspend\n");
	retval = __rnpm_shutdown(pdev, &wake);
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

__maybe_unused static void rnpm_shutdown(struct pci_dev *pdev)
{
	rnpm_remove(pdev);
	if (system_state == SYSTEM_POWER_OFF)
		pci_set_power_state(pdev, PCI_D3hot);
}

/**
 * rnpm_io_resume - called when traffic can start flowing again.
 * @pdev: Pointer to PCI device
 *
 * This callback is called when the error recovery driver tells us that
 * its OK to resume normal operation.
 */
static void rnpm_io_resume(struct pci_dev *pdev)
{
	struct rnpm_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;

	if (adapter->vferr_refcount) {
		e_info(drv, "Resuming after VF err\n");
		adapter->vferr_refcount--;
		return;
	}
	if (netif_running(netdev))
		rnpm_up(adapter);
	netif_device_attach(netdev);
}

static const struct pci_error_handlers rnpm_err_handler = {
	.error_detected = rnpm_io_error_detected,
	.slot_reset = rnpm_io_slot_reset,
	.resume = rnpm_io_resume,
};

static struct pci_driver rnpm_driver = {
	.name = rnpm_driver_name,
	.id_table = rnpm_pci_tbl,
	.probe = rnpm_probe,
	.remove = rnpm_remove,
#ifdef CONFIG_PM
	.suspend = rnpm_suspend,
	.resume = rnpm_resume,
#endif
	.shutdown = rnpm_shutdown,
	.err_handler = &rnpm_err_handler,
};

static int __init rnpm_init_module(void)
{
	int ret;

	pr_info("%s - version %s\n", rnpm_driver_string,
		rnpm_driver_version);
	pr_info("%s\n", rnpm_copyright);
	rnpm_dbg_init();
	ret = pci_register_driver(&rnpm_driver);
	if (ret) {
		rnpm_dbg_exit();
		return ret;
	}
	return 0;
}
module_init(rnpm_init_module);

static void __exit rnpm_exit_module(void)
{
	pci_unregister_driver(&rnpm_driver);
	rnpm_dbg_exit();
	rcu_barrier();
}

module_exit(rnpm_exit_module);
