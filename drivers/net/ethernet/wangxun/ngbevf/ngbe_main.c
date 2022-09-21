/*******************************************************************************

  WangXun(R) 1GbE PCI Express Virtual Function Linux Network Driver
  Copyright(c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Software Team <linux.nic@trustnetic.com>
  WangXun Technology, HuaXing Times Square A507, Hangzhou, China.

*******************************************************************************/

/******************************************************************************
 Copyright (c)2006 - 2007 Myricom, Inc. for some LRO specific code
******************************************************************************/
#include <linux/types.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/sctp.h>
#include <linux/ipv6.h>
#include <linux/sctp.h>
#ifdef NETIF_F_TSO
#include <net/checksum.h>
#ifdef NETIF_F_TSO6
#include <net/ipv6.h>
#include <net/ip6_checksum.h>
#endif
#endif /* NETIF_F_TSO */
#ifdef SIOCETHTOOL
#include <linux/ethtool.h>
#endif
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
#include <linux/if_vlan.h>
#endif

#include "ngbe.h"
#include "ngbe_txrx.h"
#ifdef HAVE_XDP_SUPPORT
#include <linux/bpf.h>
#include <linux/bpf_trace.h>
#include <linux/atomic.h>

#endif /* HAVE_XDP_SUPPORT */
/*====================*/
#if defined(NGBE_SUPPORT_KYLIN_FT)
#define DRV_VERSION     __stringify(1.1.1kylinft)
#elif defined(NGBE_SUPPORT_DEEPIN_SW)
#define DRV_VERSION     __stringify(1.1.1deepinsw)
#elif defined(NGBE_SUPPORT_KYLIN_SW)
#define DRV_VERSION     __stringify(1.1.1kylinsw)
#else
#define DRV_VERSION     __stringify(1.1.1)
#endif
/*====================*/

const char ngbe_driver_name[] = KBUILD_MODNAME;
static const char ngbe_driver_string[] =
	"WangXun(R) GbE PCI Express Virtual Function Linux Network Driver";

#define NGBE_CONF_VERSION DRV_VERSION
const char ngbe_driver_version[] = NGBE_CONF_VERSION;

char ngbe_firmware_version[32] = "N/A";

static char ngbe_copyright[] =
	"Copyright(c) 2019 - 2020 Beijing WangXun Technology Co., Ltd.";

static struct ngbe_info ngbe_em_vf_info = {
	.mac    = ngbe_mac_em_vf,
	.flags  = 0,
};

enum ngbe_boards {
	board_em_vf,
};

static const struct ngbe_info *ngbe_info_tbl[] = {
	[board_em_vf] = &ngbe_em_vf_info,
};

/* ngbe_pci_tbl - PCI Device ID Table */
static struct pci_device_id ngbe_pci_tbl[] = {
	{ PCI_VDEVICE(WANGXUN, NGBE_DEV_ID_EM_WX1860AL_W), board_em_vf },
	{ PCI_VDEVICE(WANGXUN, NGBE_DEV_ID_EM_WX1860A2), board_em_vf },
	{ PCI_VDEVICE(WANGXUN, NGBE_DEV_ID_EM_WX1860A2S), board_em_vf },
	{ PCI_VDEVICE(WANGXUN, NGBE_DEV_ID_EM_WX1860A4), board_em_vf },
	{ PCI_VDEVICE(WANGXUN, NGBE_DEV_ID_EM_WX1860A4S), board_em_vf },
	{ PCI_VDEVICE(WANGXUN, NGBE_DEV_ID_EM_WX1860AL2), board_em_vf },
	{ PCI_VDEVICE(WANGXUN, NGBE_DEV_ID_EM_WX1860AL2S), board_em_vf },
	{ PCI_VDEVICE(WANGXUN, NGBE_DEV_ID_EM_WX1860AL4), board_em_vf },
	{ PCI_VDEVICE(WANGXUN, NGBE_DEV_ID_EM_WX1860AL4S), board_em_vf },
	{ PCI_VDEVICE(WANGXUN, NGBE_DEV_ID_EM_WX1860NCSI), board_em_vf },
	{ PCI_VDEVICE(WANGXUN, NGBE_DEV_ID_EM_WX1860A1), board_em_vf },
	{ PCI_VDEVICE(WANGXUN, NGBE_DEV_ID_EM_WX1860AL1), board_em_vf },
	{ .device = 0 } /* required last entry */
};

MODULE_DEVICE_TABLE(pci, ngbe_pci_tbl);

/* easy access pointer for debug */
struct ngbevf_adapter *debug_adapter;

void ngbe_service_event_schedule(struct ngbevf_adapter *adapter)
{
	if (!test_bit(__NGBE_DOWN, &adapter->state) &&
	    !test_bit(__NGBE_REMOVING, &adapter->state) &&
	    !test_and_set_bit(__NGBE_SERVICE_SCHED, &adapter->state))
		schedule_work(&adapter->service_task);
}

void ngbe_service_event_complete(struct ngbevf_adapter *adapter)
{
	BUG_ON(!test_bit(__NGBE_SERVICE_SCHED, &adapter->state));

	/* flush memory to make sure state is correct before next watchdog */
	smp_mb__before_atomic();
	clear_bit(__NGBE_SERVICE_SCHED, &adapter->state);
}

/* forward decls */
void ngbe_queue_reset_subtask(struct ngbevf_adapter *adapter);
void ngbe_set_itr(struct ngbe_q_vector *q_vector);
void ngbe_free_all_rx_resources(struct ngbevf_adapter *adapter);
static bool ngbe_can_reuse_rx_page(struct ngbe_rx_buffer *rx_buffer);
static void ngbe_reuse_rx_page(struct ngbe_ring *rx_ring,
				  struct ngbe_rx_buffer *old_buff);

void ngbe_remove_adapter(struct ngbevf_hw *hw)
{
	struct ngbevf_adapter *adapter = hw->back;

	if (NGBE_REMOVED(hw->hw_addr))
		return;
	hw->hw_addr = NULL;
	DPRINTK(DRV, ERR, "Adapter removed\n");
	if (test_bit(__NGBE_SERVICE_INITED, &adapter->state))
		ngbe_service_event_schedule(adapter);
}

void ngbe_check_remove(struct ngbevf_hw *hw, u32 reg)
{
	u32 value;

	/* The following check not only optimizes a bit by not
	 * performing a read on the status register when the
	 * register just read was a status register read that
	 * returned NGBE_FAILED_READ_REG. It also blocks any
	 * potential recursion.
	 */
	if (reg == NGBE_VXSTATUS) {
		ngbe_remove_adapter(hw);
		return;
	}
	value = rd32(hw, NGBE_VXSTATUS);
	if (value == NGBE_FAILED_READ_REG)
		ngbe_remove_adapter(hw);
}

u32 ngbe_validate_register_read(struct ngbevf_hw *hw, u32 reg)
{
	int i;
	u32 value;
	u8 __iomem *reg_addr;

	reg_addr = READ_ONCE(hw->hw_addr);
	if (NGBE_REMOVED(reg_addr))
		return NGBE_FAILED_READ_REG;
	for (i = 0; i < NGBE_DEAD_READ_RETRIES; ++i) {
		value = ngbe_rd32(reg_addr, reg);
		if (value != NGBE_DEAD_READ_REG)
			break;
	}
	if (value == NGBE_DEAD_READ_REG)
		pr_info("%s: register %x read unchanged\n", __func__, reg);
	else
		pr_info("%s: register %x read recovered after %d retries\n",
			__func__, reg, i + 1);
	return value;
}

u32 ngbe_read_reg(struct ngbevf_hw *hw, u32 reg)
{
	u32 value;
	u8 __iomem *reg_addr;

	reg_addr = READ_ONCE(hw->hw_addr);
	if (NGBE_REMOVED(reg_addr))
		return NGBE_FAILED_READ_REG;
	value = ngbe_rd32(reg_addr, reg);
	if (unlikely(value == NGBE_FAILED_READ_REG))
		ngbe_check_remove(hw, reg);
	if (unlikely(value == NGBE_DEAD_READ_REG))
		value = ngbe_validate_register_read(hw, reg);
	return value;
}

/**
 * ngbe_set_ivar - set IVAR registers - maps interrupt causes to vectors
 * @adapter: pointer to adapter struct
 * @direction: 0 for Rx, 1 for Tx, -1 for other causes
 * @queue: queue to map the corresponding interrupt to
 * @msix_vector: the vector to map to the corresponding queue
 *
 **/
void ngbe_set_ivar(struct ngbevf_adapter *adapter, s8 direction,
			   u8 queue, u8 msix_vector)
{
	u32 ivar, index;
	struct ngbevf_hw *hw = &adapter->hw;

	if (direction == -1) {
		/* other causes */
		msix_vector |= NGBE_VXIVAR_MISC_VALID;
		ivar = rd32(hw, NGBE_VXIVAR_MISC);
		ivar &= ~0xFF;
		ivar |= msix_vector;
		wr32(hw, NGBE_VXIVAR_MISC, ivar);
	} else if (0 <= direction &&  direction <= 1) {
		/* tx or rx causes */
		msix_vector |= NGBE_VXIVAR_MISC_VALID;
		index = ((16 * (queue & 1)) + (8 * direction));
		ivar = rd32(hw, NGBE_VXIVAR(queue >> 1));
		ivar &= ~(0xFF << index);
		ivar |= (msix_vector << index);
		wr32(hw, NGBE_VXIVAR(queue >> 1), ivar);
	}
}

void ngbe_unmap_and_free_tx_resource(struct ngbe_ring *tx_ring,
					     struct ngbe_tx_buffer *tx_buffer)
{
	if (tx_buffer->skb) {
		dev_kfree_skb_any(tx_buffer->skb);
		if (dma_unmap_len(tx_buffer, len))
			dma_unmap_single(tx_ring->dev,
					 dma_unmap_addr(tx_buffer, dma),
					 dma_unmap_len(tx_buffer, len),
					 DMA_TO_DEVICE);
	} else if (dma_unmap_len(tx_buffer, len)) {
		dma_unmap_page(tx_ring->dev,
			       dma_unmap_addr(tx_buffer, dma),
			       dma_unmap_len(tx_buffer, len),
			       DMA_TO_DEVICE);
	}
	tx_buffer->next_to_watch = NULL;
	tx_buffer->skb = NULL;
	dma_unmap_len_set(tx_buffer, len, 0);
	/* tx_buffer must be completely set up in the transmit path */
}

#if 0
static u64 ngbe_get_tx_completed(struct ngbe_ring *ring)
{
	return ring->stats.packets;
}

#if IS_ENABLED(CONFIG_PCI_HYPERV)
/**
  *  ngbe_hv_set_rar_vf - set device MAC address Hyper-V variant
  *  @hw: pointer to hardware structure
  *  @index: Receive address register to write
  *  @addr: Address to put into receive address register
  *  @vmdq: Unused in this implementation
  *
  * We don't really allow setting the device MAC address. However,
  * if the address being set is the permanent MAC address we will
  * permit that.
  **/
s32 ngbe_hv_set_rar_vf(struct ngbe_hw *hw, u32 index, u8 *addr,
			   u32 vmdq, u32 enable_addr)
{
	if (ether_addr_equal(addr, hw->mac.perm_addr))
		return 0;

	return -EOPNOTSUPP;
}

/**
  * Hyper-V variant; the VF/PF communication is through the PCI
  * config space.
  */
s32 ngbe_hv_reset_hw_vf(struct ngbe_hw *hw)
{
	struct ngbevf_adapter *adapter = hw->back;
#if IS_ENABLED(CONFIG_PCI_MMCONFIG)
	int i;

	for (i = 0; i < 6; i++)
		pci_read_config_byte(adapter->pdev,
							//(i + TXGBE_HV_RESET_OFFSET),
							(i + 0x201).
							&hw->mac.perm_addr[i]);
	return 0;
#else

	dev_err(&adapter->pdev->dev,
			"PCI_MMCONFIG needs to be enabled for Hyper-V\n");
	return -EOPNOTSUPP;
#endif
}
#endif
#endif

inline u32 ngbe_get_tx_pending(struct ngbe_ring *ring)
{
	struct ngbevf_adapter *adapter = netdev_priv(ring->netdev);
	struct ngbevf_hw *hw = &adapter->hw;

	u32 head = rd32(hw, NGBE_VXTDH);
	u32 tail = rd32(hw, NGBE_VXTDT);

	if (head != tail)
		return (head < tail) ?
			tail - head : (tail + ring->count - head);

	return 0;
}

inline bool ngbe_check_tx_hang(struct ngbe_ring *tx_ring)
{
	u32 tx_done = tx_ring->stats.packets;
	u32 tx_done_old = tx_ring->tx_stats.tx_done_old;
	u32 tx_pending = ngbe_get_tx_pending(tx_ring);

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
	if (tx_done_old == tx_done && tx_pending)
		/* make sure it is true for two checks in a row */
		return test_and_set_bit(__NGBE_HANG_CHECK_ARMED,
					&tx_ring->state);
	/* reset the countdown */
	clear_bit(__NGBE_HANG_CHECK_ARMED, &tx_ring->state);
	/* update completed stats and continue */
	tx_ring->tx_stats.tx_done_old = tx_done;


	return false;
}

void ngbe_tx_timeout_reset(struct ngbevf_adapter *adapter)
{
	/* Do the reset outside of interrupt context */
	if (!test_bit(__NGBE_DOWN, &adapter->state)) {
		adapter->flagsd |= NGBE_F_REQ_RESET;
		ngbe_service_event_schedule(adapter);
	}
}

/**
 * ngbe_tx_timeout - Respond to a Tx Hang
 * @netdev: network interface device structure
 **/
#ifdef HAVE_TX_TIMEOUT_TXQUEUE
	static void ngbe_tx_timeout(struct net_device *netdev, unsigned int txqueue)
#else
void ngbe_tx_timeout(struct net_device *netdev)
#endif
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);

	ngbe_tx_timeout_reset(adapter);
}


/**
 * ngbe_clean_tx_irq - Reclaim resources after transmit completes
 * @q_vector: board private structure
 * @tx_ring: tx ring to clean
 **/
bool ngbe_clean_tx_irq(struct ngbe_q_vector *q_vector,
				 struct ngbe_ring *tx_ring, int napi_budget)
{
	struct ngbevf_adapter *adapter = q_vector->adapter;
	struct ngbe_tx_buffer *tx_buffer;
	struct ngbe_tx_desc *tx_desc;
	unsigned int total_bytes = 0, total_packets = 0;
	unsigned int budget = tx_ring->count / 2;
	unsigned int i = tx_ring->next_to_clean;

	if (test_bit(__NGBE_DOWN, &adapter->state))
		return true;

	tx_buffer = &tx_ring->tx_buffer_info[i];
	tx_desc = NGBE_TX_DESC(tx_ring, i);
	i -= tx_ring->count;

	do {
		struct ngbe_tx_desc *eop_desc = tx_buffer->next_to_watch;

		/* if next_to_watch is not set then there is no work pending */
		if (!eop_desc)
			break;

		/* prevent any other reads prior to eop_desc */
		smp_rmb();

		/* if DD is not set pending work has not been completed */
		if (!(eop_desc->status & NGBE_TXD_STAT_DD))
			break;

		/* clear next_to_watch to prevent false hangs */
		tx_buffer->next_to_watch = NULL;

		/* update the statistics for this packet */
		total_bytes += tx_buffer->bytecount;
		total_packets += tx_buffer->gso_segs;

		/* free the skb */
#ifdef HAVE_XDP_SUPPORT
		if (ring_is_xdp(tx_ring))
			page_frag_free(tx_buffer->data);
		else
			napi_consume_skb(tx_buffer->skb, napi_budget);
#else
		napi_consume_skb(tx_buffer->skb, napi_budget);
#endif

		/* unmap skb header data */
		dma_unmap_single(tx_ring->dev,
				 dma_unmap_addr(tx_buffer, dma),
				 dma_unmap_len(tx_buffer, len),
				 DMA_TO_DEVICE);

		/* clear tx_buffer data */
		//tx_buffer->skb = NULL;
		dma_unmap_len_set(tx_buffer, len, 0);

		/* unmap remaining buffers */
		while (tx_desc != eop_desc) {
			tx_buffer++;
			tx_desc++;
			i++;
			if (unlikely(!i)) {
				i -= tx_ring->count;
				tx_buffer = tx_ring->tx_buffer_info;
				tx_desc = NGBE_TX_DESC(tx_ring, 0);
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
			tx_desc = NGBE_TX_DESC(tx_ring, 0);
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

	if (check_for_tx_hang(tx_ring) && ngbe_check_tx_hang(tx_ring)) {
		struct ngbevf_hw *hw = &adapter->hw;
		struct ngbe_tx_desc *eop_desc;

		eop_desc = tx_ring->tx_buffer_info[i].next_to_watch;

		pr_err("Detected Tx Unit Hang%s\n"
		       "  Tx Queue             <%d>\n"
		       "  TDH, TDT             <%x>, <%x>\n"
		       "  next_to_use          <%x>\n"
		       "  next_to_clean        <%x>\n"
		       "tx_buffer_info[next_to_clean]\n"
		       "  next_to_watch        <%p>\n"
		       "  eop_desc.status      <%x>\n"
		       "  time_stamp           <%lx>\n"
		       "  jiffies              <%lx>\n",
		       ring_is_xdp(tx_ring) ? " XDP" : "",
		       tx_ring->que_idx,
		       rd32(hw, NGBE_VXTDH),
		       rd32(hw, NGBE_VXTDT),
		       tx_ring->next_to_use, i,
		       eop_desc, (eop_desc ? eop_desc->status : 0),
		       tx_ring->tx_buffer_info[i].time_stamp, jiffies);

		if (!ring_is_xdp(tx_ring))
			netif_stop_subqueue(tx_ring->netdev,
					    tx_ring->que_idx);

		/* schedule immediate reset if we believe we hung */
		ngbe_tx_timeout_reset(adapter);

		return true;
	}

	if (ring_is_xdp(tx_ring))
		return !!budget;

	netdev_tx_completed_queue(txring_txq(tx_ring), total_packets, total_bytes);

#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)
	if (unlikely(total_packets && netif_carrier_ok(tx_ring->netdev) &&
		     (ngbe_desc_unused(tx_ring) >= TX_WAKE_THRESHOLD))) {
		/* Make sure that anybody stopping the queue after this
		 * sees the new next_to_clean.
		 */
		smp_mb();
#ifdef HAVE_TX_MQ
		if (__netif_subqueue_stopped(tx_ring->netdev,
					     tx_ring->que_idx)
		    && !test_bit(__NGBE_DOWN, &adapter->state)) {
			netif_wake_subqueue(tx_ring->netdev,
					    tx_ring->que_idx);
			++tx_ring->tx_stats.tx_restart_queue;
		}
#else
		if (netif_queue_stopped(tx_ring->netdev) &&
		    !test_bit(__NGBE_DOWN, &adapter->state)) {
			netif_wake_queue(tx_ring->netdev);
			++tx_ring->tx_stats.tx_restart_queue;
		}
#endif
	}

	return !!budget;
}

#ifdef HAVE_VLAN_RX_REGISTER
void ngbe_rx_vlan(struct ngbe_ring *rx_ring,
			    union ngbe_rx_desc *rx_desc,
			    struct sk_buff *skb)
{
	struct net_device *dev = rx_ring->netdev;

#ifdef NETIF_F_HW_VLAN_CTAG_RX
	if ((dev->features & NETIF_F_HW_VLAN_CTAG_RX) &&
#else
	if ((dev->features & NETIF_F_HW_VLAN_RX) &&
#endif
	    (NGBE_RXD_STATUS(rx_desc) & NGBE_RXD_STAT_VP))
		NGBE_CB(skb)->vid = NGBE_RXD_VLAN(rx_desc);
	else
		NGBE_CB(skb)->vid = 0;
}

/**
 * ngbe_receive_skb - Send a completed packet up the stack
 * @q_vector: structure containing interrupt and ring information
 * @skb: packet to send up
 **/
void ngbe_receive_skb(struct ngbe_q_vector *q_vector,
			      struct sk_buff *skb)
{
	struct ngbe_adapter *adapter = q_vector->adapter;
	u16 vlan_tag = NGBE_CB(skb)->vid;

#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	struct vlan_group **vlgrp = &adapter->vlgrp;

	if (vlan_tag & VLAN_VID_MASK && *vlgrp != NULL) {

		if (q_vector->netpoll_rx)
			vlan_hwaccel_rx(skb, *vlgrp, vlan_tag);
		else
			vlan_gro_receive(&q_vector->napi,
					 *vlgrp, vlan_tag, skb);
	} else {
#endif
		if (q_vector->netpoll_rx)
			netif_rx(skb);
		else
			napi_gro_receive(&q_vector->napi, skb);
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	}
#endif
}
#endif /* HAVE_VLAN_RX_REGISTER */

/**
 * ngbe_rx_skb - Helper function to determine proper Rx method
 * @q_vector: structure containing interrupt and ring information
 * @skb: packet to send up
 **/
void ngbe_rx_skb(struct ngbe_q_vector *q_vector,
			 struct sk_buff *skb)
{
#ifdef CONFIG_NET_RX_BUSY_POLL
	skb_mark_napi_id(skb, &q_vector->napi);

	if (ngbe_qv_busy_polling(q_vector) || q_vector->netpoll_rx) {
		netif_receive_skb(skb);
		/* exit early if we busy polled */
		return;
	}
#endif
#ifdef HAVE_VLAN_RX_REGISTER
	ngbe_receive_skb(q_vector, skb);
#else /* !HAVE_VLAN_RX_REGISTER */
#ifndef CONFIG_NET_RX_BUSY_POLL
	if (q_vector->netpoll_rx)
		netif_rx(skb);
	else
#endif
		napi_gro_receive(&q_vector->napi, skb);
#endif /* HAVE_VLAN_RX_REGISTER */
#ifndef NETIF_F_GRO
	q_vector->adapter->netdev->last_rx = jiffies;
#endif
}

#ifdef NETIF_F_RXHASH
#define NGBE_RSS_L4_TYPES_MASK \
	((1ul << NGBE_RSSTYPE_IPV4_TCP) | \
	 (1ul << NGBE_RSSTYPE_IPV4_UDP) | \
	 (1ul << NGBE_RSSTYPE_IPV6_TCP) | \
	 (1ul << NGBE_RSSTYPE_IPV6_UDP))

inline void ngbe_rx_hash(struct ngbe_ring *ring,
				   union ngbe_rx_desc *rx_desc,
				   struct sk_buff *skb)
{
	u16 rss_type;

	if (!(ring->netdev->features & NETIF_F_RXHASH))
		return;

	rss_type = NGBE_RXD_RSSTYPE(rx_desc);

	if (!rss_type)
		return;

	skb_set_hash(skb, NGBE_RXD_RSS_HASH(rx_desc),
		     (NGBE_RSS_L4_TYPES_MASK & (1ul << rss_type)) ?
		     PKT_HASH_TYPE_L4 : PKT_HASH_TYPE_L3);
}
#endif /* NETIF_F_RXHASH */

/* ngbe_rx_checksum - indicate in skb if hw indicated a good cksum
 * @ring: structure containig ring specific data
 * @rx_desc: current Rx descriptor being processed
 * @skb: skb currently being received and modified
 */
inline void ngbe_rx_checksum(struct ngbe_ring *ring,
				       union ngbe_rx_desc *rx_desc,
				       struct sk_buff *skb)
{
	ngbe_dptype dptype;
	skb->ip_summed = CHECKSUM_NONE;

	/* Rx csum disabled */
	if (!(ring->netdev->features & NETIF_F_RXCSUM))
		return;

	dptype = ngbe_rx_decode_ptype(rx_desc);
	if (!dptype.known)
		return;
	if (dptype.etype)
		skb->encapsulation = 1;

	/* if IP and error */
	if ((NGBE_RXD_STATUS(rx_desc) & NGBE_RXD_STAT_IPCS &&
	     NGBE_RXD_STATUS(rx_desc) & NGBE_RXD_ERR_IPE) ||
	    (NGBE_RXD_STATUS(rx_desc) & NGBE_RXD_STAT_EIPCS &&
	     NGBE_RXD_STATUS(rx_desc) & NGBE_RXD_ERR_EIPERR)) {
		ring->rx_stats.csum_err++;
		return;
	}

	if (!(NGBE_RXD_STATUS(rx_desc) & NGBE_RXD_STAT_TPCS))
		return;

	if (NGBE_RXD_STATUS(rx_desc) & NGBE_RXD_ERR_TPE) {
		ring->rx_stats.csum_err++;
		return;
	}

	/* It must be a TCP or UDP packet with a valid checksum */
	skb->ip_summed = CHECKSUM_UNNECESSARY;
}

/* ngbe_process_skb_fields - Populate skb header fields from Rx descriptor
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @rx_desc: pointer to the EOP Rx descriptor
 * @skb: pointer to current skb being populated
 *
 * This function checks the ring, descriptor, and packet information in
 * order to populate the checksum, VLAN, protocol, and other fields within
 * the skb.
 */
void ngbe_process_skb_fields(struct ngbe_ring *rx_ring,
				       union ngbe_rx_desc *rx_desc,
				       struct sk_buff *skb)
{
#ifdef NETIF_F_RXHASH
	ngbe_rx_hash(rx_ring, rx_desc, skb);
#endif
	ngbe_rx_checksum(rx_ring, rx_desc, skb);
#ifdef HAVE_VLAN_RX_REGISTER
	ngbe_rx_vlan(rx_ring, rx_desc, skb);
#else
	if (NGBE_RXD_STATUS(rx_desc) & NGBE_RXD_STAT_VP) {
		u16 vid = NGBE_RXD_VLAN(rx_desc);
		unsigned long *active_vlans = netdev_priv(rx_ring->netdev);

		if (test_bit(vid & VLAN_VID_MASK, active_vlans))
			__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), vid);
	}
#endif

	skb->protocol = eth_type_trans(skb, rx_ring->netdev);
}

static
struct ngbe_rx_buffer *ngbe_get_rx_buffer(struct ngbe_ring *rx_ring,
						const unsigned int size)
{
	struct ngbe_rx_buffer *rx_buffer;

	rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
	prefetchw(rx_buffer->page);

	/* we are reusing so sync this buffer for CPU use */
	dma_sync_single_range_for_cpu(rx_ring->dev,
				      rx_buffer->dma_addr,
				      rx_buffer->page_offset,
				      size,
				      DMA_FROM_DEVICE);

	rx_buffer->pagecnt_bias--;

	return rx_buffer;
}

static void ngbe_put_rx_buffer(struct ngbe_ring *rx_ring,
				  struct ngbe_rx_buffer *rx_buffer,
				  struct sk_buff *skb)
{
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
	DEFINE_DMA_ATTRS(attrs);

	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);

#endif
	if (ngbe_can_reuse_rx_page(rx_buffer)) {
		/* hand second half of page back to the ring */
		ngbe_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		if (IS_ERR(skb))
			/* We are not reusing the buffer so unmap it and free
			 * any references we are holding to it
			 */
			dma_unmap_page_attrs(rx_ring->dev, rx_buffer->dma_addr,
					     ngbe_rx_pg_size(rx_ring),
					     DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
					     &attrs);
#else
					     NGBE_RX_DMA_ATTR);
#endif
		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);
	}

	/* clear contents of rx_buffer */
	rx_buffer->page = NULL;
}

/**
 * ngbe_is_non_eop - process handling of non-EOP buffers
 * @rx_ring: Rx ring being processed
 * @rx_desc: Rx descriptor for current buffer
 * @skb: current socket buffer containing buffer in progress
 *
 * This function updates next to clean.  If the buffer is an EOP buffer
 * this function exits returning false, otherwise it will place the
 * sk_buff in the next buffer to be chained and return true indicating
 * that this is in fact a non-EOP buffer.
 **/
bool ngbe_is_non_eop(struct ngbe_ring *rx_ring,
			     union ngbe_rx_desc *rx_desc)
{
	u32 ntc = rx_ring->next_to_clean + 1;

	/* fetch, update, and store next to clean */
	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;

	prefetch(NGBE_RX_DESC(rx_ring, ntc));

	if (likely(NGBE_RXD_STATUS(rx_desc) & NGBE_RXD_STAT_EOP))
		return false;

	return true;
}

static inline unsigned int ngbe_rx_offset(struct ngbe_ring *rx_ring)
{
	return ring_uses_build_skb(rx_ring) ? NGBE_SKB_PAD : 0;
}

bool ngbe_alloc_mapped_page(struct ngbe_ring *rx_ring,
				      struct ngbe_rx_buffer *bi)
{
	struct page *page = bi->page;
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
	page = dev_alloc_pages(ngbe_rx_pg_order(rx_ring));
	if (unlikely(!page)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	/* map page for use */
	dma = dma_map_page_attrs(rx_ring->dev, page, 0,
				 ngbe_rx_pg_size(rx_ring),
				 DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
				 &attrs);
#else
				 NGBE_RX_DMA_ATTR);
#endif

	/* if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		__free_pages(page, ngbe_rx_pg_order(rx_ring));

		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	bi->dma_addr = dma;
	bi->page = page;
	bi->page_offset = ngbe_rx_offset(rx_ring);
	bi->pagecnt_bias = 1;
	rx_ring->rx_stats.alloc_rx_page++;

	return true;
}

/**
 * ngbe_alloc_rx_buffers - Replace used receive buffers; packet split
 * @rx_ring: rx descriptor ring (for a specific queue) to setup buffers on
 * @cleaned_count: number of buffers to replace
 **/
void ngbe_alloc_rx_buffers(struct ngbe_ring *rx_ring,
				     u16 cleaned_count)
{
	union ngbe_rx_desc *rx_desc;
	struct ngbe_rx_buffer *bi;
	unsigned int i = rx_ring->next_to_use;

	/* nothing to do or no valid netdev defined */
	if (!cleaned_count || !rx_ring->netdev)
		return;

	rx_desc = NGBE_RX_DESC(rx_ring, i);
	bi = &rx_ring->rx_buffer_info[i];
	i -= rx_ring->count;

	do {
		if (!ngbe_alloc_mapped_page(rx_ring, bi))
			break;

		/* sync the buffer for use by the device */
		dma_sync_single_range_for_device(rx_ring->dev, bi->dma_addr,
						 bi->page_offset,
						 ngbe_rx_bufsz(rx_ring),
						 DMA_FROM_DEVICE);

		/* Refresh the desc even if pkt_addr didn't change
		 * because each write-back erases this info.
		 */
		rx_desc->rd.pkt_addr = NGBE_RXD_PKTADDR(bi->dma_addr + bi->page_offset);

		rx_desc++;
		bi++;
		i++;
		if (unlikely(!i)) {
			rx_desc = NGBE_RX_DESC(rx_ring, 0);
			bi = rx_ring->rx_buffer_info;
			i -= rx_ring->count;
		}

		/* clear the hdr_addr for the next_to_use descriptor */
		rx_desc->rd.hdr_addr = 0;

		cleaned_count--;
	} while (cleaned_count);

	i += rx_ring->count;

	if (rx_ring->next_to_use != i) {
		/* record the next descriptor to use */
		rx_ring->next_to_use = i;

		/* update next to alloc since we have filled the ring */
		rx_ring->next_to_alloc = i;

		/* Force memory writes to complete before letting h/w
		 * know there are new descriptors to fetch.  (Only
		 * applicable for weak-ordered memory model archs,
		 * such as IA-64).
		 */
		wmb();
		writel(i, rx_ring->tail);
	}
}

/* ngbe_get_headlen - determine size of header for RSC/LRO/GRO/FCOE
 * @data: pointer to the start of the headers
 * @max_len: total length of section to find headers in
 *
 * This function is meant to determine the length of headers that will
 * be recognized by hardware for LRO, GRO, and RSC offloads.  The main
 * motivation of doing this is to only perform one pull for IPv4 TCP
 * packets so that we can do basic things like calculating the gso_size
 * based on the average data per packet.
 */
unsigned int ngbe_get_headlen(unsigned char *data,
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
	if (protocol == __constant_htons(ETH_P_8021Q)) {
		if ((hdr.network - data) > (max_len - VLAN_HLEN))
			return max_len;

		protocol = hdr.vlan->h_vlan_encapsulated_proto;
		hdr.network += VLAN_HLEN;
	}

	/* handle L3 protocols */
	if (protocol == __constant_htons(ETH_P_IP)) {
		if ((hdr.network - data) > (max_len - sizeof(struct iphdr)))
			return max_len;

		/* access ihl as a u8 to avoid unaligned access on ia64 */
		hlen = (hdr.network[0] & 0x0F) << 2;

		/* verify hlen meets minimum size requirements */
		if (hlen < sizeof(struct iphdr))
			return hdr.network - data;

		/* record next protocol */
		nexthdr = hdr.ipv4->protocol;
		hdr.network += hlen;
	} else if (protocol == __constant_htons(ETH_P_IPV6)) {
		if ((hdr.network - data) > (max_len - sizeof(struct ipv6hdr)))
			return max_len;

		/* record next protocol */
		nexthdr = hdr.ipv6->nexthdr;
		hdr.network += sizeof(struct ipv6hdr);
	} else {
		return hdr.network - data;
	}

	/* finally sort out TCP */
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

#if 0
/* ngbe_pull_tail - ngbe specific version of skb_pull_tail
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @skb: pointer to current skb being adjusted
 *
 * This function is an ngbe specific version of __pskb_pull_tail.  The
 * main difference between this version and the original function is that
 * this function can make several assumptions about the state of things
 * that allow for significant optimizations versus the standard function.
 * As a result we can do things like drop a frag and maintain an accurate
 * truesize for the skb.
 */
void ngbe_pull_tail(struct ngbe_ring __always_unused *rx_ring,
			      struct sk_buff *skb)
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
	pull_len = ngbe_get_headlen(va, NGBE_RX_HDR_SIZE);

	/* align pull length to size of long to optimize memcpy performance */
	skb_copy_to_linear_data(skb, va, ALIGN(pull_len, sizeof(long)));

	/* update all of the pointers */
	skb_frag_size_sub(frag, pull_len);
	skb_frag_off_add(frag, pull_len);
	skb->data_len -= pull_len;
	skb->tail += pull_len;
}
#endif

/**
 * ngbe_cleanup_headers - Correct corrupted or empty headers
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
bool ngbe_cleanup_headers(struct ngbe_ring *rx_ring,
				  union ngbe_rx_desc *rx_desc,
				  struct sk_buff *skb)
{
	/* XDP packets use error pointer so abort at this point */
	if (IS_ERR(skb))
		return true;

	/* verify that the packet does not have any known errors */
	if (unlikely(NGBE_RXD_STATUS(rx_desc) & NGBE_RXD_ERR_RXE)) {
		struct net_device *netdev = rx_ring->netdev;
		if (!(netdev->features & NETIF_F_RXALL)) {
			dev_kfree_skb_any(skb);
			return true;
		}
	}

	/* if eth_skb_pad returns an error the skb was freed */
	if (eth_skb_pad(skb))
		return true;

	return false;
}

/**
 * ngbe_reuse_rx_page - page flip buffer and store it back on the ring
 * @rx_ring: rx descriptor ring to store buffers on
 * @old_buff: donor buffer to have page reused
 *
 * Synchronizes page for reuse by the adapter
 **/
void ngbe_reuse_rx_page(struct ngbe_ring *rx_ring,
				struct ngbe_rx_buffer *old_buff)
{
	struct ngbe_rx_buffer *new_buff;
	u16 nta = rx_ring->next_to_alloc;

	new_buff = &rx_ring->rx_buffer_info[nta];

	/* update, and store next to alloc */
	nta++;
	rx_ring->next_to_alloc = (nta < rx_ring->count) ? nta : 0;

	/* transfer page from old buffer to new buffer */
	new_buff->page = old_buff->page;
	new_buff->dma_addr = old_buff->dma_addr;
	new_buff->page_offset = old_buff->page_offset;
	new_buff->pagecnt_bias = old_buff->pagecnt_bias;
}

static inline bool ngbe_page_is_reserved(struct page *page)
{
	return (page_to_nid(page) != numa_mem_id()) || page_is_pfmemalloc(page);
}

static bool ngbe_can_reuse_rx_page(struct ngbe_rx_buffer *rx_buffer)
{
	unsigned int pagecnt_bias = rx_buffer->pagecnt_bias;
	struct page *page = rx_buffer->page;

	/* avoid re-using remote pages */
	if (unlikely(ngbe_page_is_reserved(page)))
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
#define NGBE_LAST_OFFSET \
	(SKB_WITH_OVERHEAD(PAGE_SIZE) - NGBE_RXBUFFER_2048)

	if (rx_buffer->page_offset > NGBE_LAST_OFFSET)
		return false;

#endif
#ifdef HAVE_PAGE_COUNT_BULK_UPDATE
	/* If we have drained the page fragment pool we need to update
	 * the pagecnt_bias and page count so that we fully restock the
	 * number of references the driver holds.
	 */
	if (unlikely(!pagecnt_bias)) {
		page_ref_add(page, USHRT_MAX);
		rx_buffer->pagecnt_bias = USHRT_MAX;
	}

#else /* !HAVE_PAGE_COUNT_BULK_UPDATE */
	/* Even if we own the page, we are not allowed to use atomic_set()
	 * This would break get_page_unless_zero() users.
	 */
	if (likely(!pagecnt_bias)) {
		page_ref_inc(page);
		rx_buffer->pagecnt_bias = 1;
	}

#endif /* !HAVE_PAGE_COUNT_BULK_UPDATE */
	return true;
}

/**
 * ngbe_add_rx_frag - Add contents of Rx buffer to sk_buff
 * @rx_ring: rx descriptor ring to transact packets on
 * @rx_buffer: buffer containing page to add
 * @rx_desc: descriptor containing length of buffer written by hardware
 * @skb: sk_buff to place the data into
 *
 * This function will add the data contained in rx_buffer->page to the skb.
 * This is done either through a direct copy if the data in the buffer is
 * less than the skb header size, otherwise it will just attach the page as
 * a frag to the skb.
 *
 * The function will then update the page offset if necessary and return
 * true if the buffer can be reused by the adapter.
 **/
static void ngbe_add_rx_frag(struct ngbe_ring __always_unused *rx_ring,
			      struct ngbe_rx_buffer *rx_buffer,
				struct sk_buff *skb,
				unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = ngbe_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize = ring_uses_build_skb(rx_ring) ?
				SKB_DATA_ALIGN(NGBE_SKB_PAD + size) :
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

static
struct sk_buff *ngbe_construct_skb(struct ngbe_ring *rx_ring,
				      struct ngbe_rx_buffer *rx_buffer,
				      struct xdp_buff *xdp,
				      union ngbe_rx_desc *rx_desc)
{
	unsigned int size = xdp->data_end - xdp->data;
#if (PAGE_SIZE < 8192)
	unsigned int truesize = ngbe_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize = SKB_DATA_ALIGN(xdp->data_end -
					       xdp->data_hard_start);
#endif
	unsigned int headlen;
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
	 * packets going to stack via ixgbevf_build_skb().
	 *
	 * For ixgbevf_construct_skb() mode it means that the
	 * xdp->data_meta will always point to xdp->data, since
	 * the helper cannot expand the head. Should this ever
	 * changed in future for legacy-rx mode on, then lets also
	 * add xdp->data_meta handling here.
	 */

	/* allocate a skb to store the frags */
	skb = napi_alloc_skb(&rx_ring->q_vector->napi, NGBE_RX_HDR_SIZE);
	if (unlikely(!skb))
		return NULL;

	/* Determine available headroom for copy */
	headlen = size;
	if (headlen > NGBE_RX_HDR_SIZE)
		headlen = eth_get_headlen(skb->dev,xdp->data, NGBE_RX_HDR_SIZE);

	/* align pull length to size of long to optimize memcpy performance */
	memcpy(__skb_put(skb, headlen), xdp->data,
	       ALIGN(headlen, sizeof(long)));

	/* update all of the pointers */
	size -= headlen;
	if (size) {
		skb_add_rx_frag(skb, 0, rx_buffer->page,
				(xdp->data + headlen) -
					page_address(rx_buffer->page),
				size, truesize);
#if (PAGE_SIZE < 8192)
		rx_buffer->page_offset ^= truesize;
#else
		rx_buffer->page_offset += truesize;
#endif
	} else {
		rx_buffer->pagecnt_bias++;
	}

	return skb;
}
inline void ngbe_irq_enable_queues(struct ngbevf_adapter *adapter,
					     u32 qmask)
{
	struct ngbevf_hw *hw = &adapter->hw;

	wr32(hw, NGBE_VXIMC, qmask);
}

#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC
static struct sk_buff *ngbe_build_skb(struct ngbe_ring *rx_ring,
					 struct ngbe_rx_buffer *rx_buffer,
					 struct xdp_buff *xdp,
					 union ngbe_rx_desc *rx_desc)
{
#ifdef HAVE_XDP_BUFF_DATA_META
	unsigned int metasize = xdp->data - xdp->data_meta;
	void *va = xdp->data_meta;
#else
	void *va = xdp->data;
#endif /* HAVE_XDP_BUFF_DATA_META */
#if (PAGE_SIZE < 8192)
	unsigned int truesize = ngbe_rx_pg_size(rx_ring) / 2;
#else
	unsigned int truesize = SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
				SKB_DATA_ALIGN(xdp->data_end -
					       xdp->data_hard_start);
#endif
	struct sk_buff *skb;

	/* Prefetch first cache line of first page. If xdp->data_meta
	 * is unused, this points to xdp->data, otherwise, we likely
	 * have a consumer accessing first few bytes of meta data,
	 * and then actual data.
	 */
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
#ifdef HAVE_XDP_BUFF_DATA_META
	if (metasize)
		skb_metadata_set(skb, metasize);
#endif /* HAVE_XDP_BUFF_DATA_META */

	/* update buffer offset */
#if (PAGE_SIZE < 8192)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif

	return skb;
}

#endif /* HAVE_SWIOTLB_SKIP_CPU_SYNC */
#define NGBE_TXD_CMD (NGBE_TXD_EOP | \
		       NGBE_TXD_RS)

#define NGBE_XDP_PASS 0
#define NGBE_XDP_CONSUMED 1
#define NGBE_XDP_TX 2

#ifdef HAVE_XDP_SUPPORT
static int ngbe_xmit_xdp_ring(struct ngbe_ring *ring,
				 struct xdp_buff *xdp)
{
	struct ngbe_tx_buffer *tx_buffer;
	struct ngbe_tx_desc *tx_desc;
	u32 len, cmd_type;
	dma_addr_t dma;
	u16 i;

	len = xdp->data_end - xdp->data;

	if (unlikely(!ngbe_desc_unused(ring)))
		return NGBE_XDP_CONSUMED;

	dma = dma_map_single(ring->dev, xdp->data, len, DMA_TO_DEVICE);
	if (dma_mapping_error(ring->dev, dma))
		return NGBE_XDP_CONSUMED;

	/* record the location of the first descriptor for this packet */
	i = ring->next_to_use;
	tx_buffer = &ring->tx_buffer_info[i];

	dma_unmap_len_set(tx_buffer, len, len);
	dma_unmap_addr_set(tx_buffer, dma, dma);
	tx_buffer->data = xdp->data;
	tx_buffer->bytecount = len;
	tx_buffer->gso_segs = 1;
	tx_buffer->protocol = 0;

	/* Populate minimal context descriptor that will provide for the
	 * fact that we are expected to process Ethernet frames.
	 */
	if (!test_bit(__NGBE_TX_XDP_RING_PRIMED, &ring->state)) {
		struct ngbe_adv_tx_context_desc *context_desc;

		set_bit(__NGBE_TX_XDP_RING_PRIMED, &ring->state);

		context_desc = NGBE_TX_CTXTDESC(ring, 0);
		context_desc->vlan_macip_lens	=
			cpu_to_le32(ETH_HLEN << NGBE_ADVTXD_MACLEN_SHIFT);
		context_desc->seqnum_seed	= 0;
		context_desc->type_tucmd_mlhl	=
			cpu_to_le32(NGBE_TXD_CMD_DEXT |
				    NGBE_ADVTXD_DTYP_CTXT);
		context_desc->mss_l4len_idx	= 0;

		i = 1;
	}

	/* put descriptor type bits */
	cmd_type = NGBE_ADVTXD_DTYP_DATA |
		   NGBE_ADVTXD_DCMD_DEXT ;
	//	   TXGBE_ADVTXD_DCMD_IFCS;
	cmd_type |= len | NGBE_TXD_CMD;

	tx_desc = NGBE_TX_DESC(ring, i);
	//tx_desc->read.buffer_addr = cpu_to_le64(dma);

	//tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type);
	//tx_desc->read.olinfo_status =
	//		cpu_to_le32((len << TXGBE_ADVTXD_PAYLEN_SHIFT) |
	//			    TXGBE_ADVTXD_CC);
	tx_desc->pkt_addr = cpu_to_le64(dma);

	tx_desc->cmd_type_len = cpu_to_le32(cmd_type);
	tx_desc->status =
			cpu_to_le32((len << NGBE_ADVTXD_PAYLEN_SHIFT) |
				    NGBE_ADVTXD_CC);


	/* Avoid any potential race with cleanup */
	smp_wmb();

	/* set next_to_watch value indicating a packet is present */
	i++;
	if (i == ring->count)
		i = 0;

	tx_buffer->next_to_watch = tx_desc;
	ring->next_to_use = i;

	return NGBE_XDP_TX;
}

#endif /* HAVE_XDP_SUPPORT */
static struct sk_buff *
ngbe_run_xdp(struct ngbevf_adapter __maybe_unused *adapter,
		struct ngbe_ring __maybe_unused *rx_ring,
		struct xdp_buff __maybe_unused *xdp)
{
	int result = NGBE_XDP_PASS;
#ifdef HAVE_XDP_SUPPORT
	struct ngbe_ring *xdp_ring;
	struct bpf_prog *xdp_prog;
	u32 act;

	rcu_read_lock();
	xdp_prog = READ_ONCE(rx_ring->xdp_prog);

	if (!xdp_prog)
		goto xdp_out;

	act = bpf_prog_run_xdp(xdp_prog, xdp);
	switch (act) {
	case XDP_PASS:
		break;
	case XDP_TX:
		xdp_ring = adapter->xdp_ring[rx_ring->que_idx];
		result = ngbe_xmit_xdp_ring(xdp_ring, xdp);
		break;
	default:
		bpf_warn_invalid_xdp_action(act);
		/* fallthrough */
	case XDP_ABORTED:
		trace_xdp_exception(rx_ring->netdev, xdp_prog, act);
		/* fallthrough -- handle aborts by dropping packet */
	case XDP_DROP:
		result = NGBE_XDP_CONSUMED;
		break;
	}
xdp_out:
	rcu_read_unlock();
#endif /* HAVE_XDP_SUPPORT */

	return ERR_PTR(-result);
}

static void ngbe_rx_buffer_flip(struct ngbe_ring *rx_ring,
				   struct ngbe_rx_buffer *rx_buffer,
				   unsigned int size)
{
#if (PAGE_SIZE < 8192)
	unsigned int truesize = ngbe_rx_pg_size(rx_ring) / 2;

	rx_buffer->page_offset ^= truesize;
#else
	unsigned int truesize = ring_uses_build_skb(rx_ring) ?
				SKB_DATA_ALIGN(NGBE_SKB_PAD + size) :
				SKB_DATA_ALIGN(size);

	rx_buffer->page_offset += truesize;
#endif
}



int ngbe_clean_rx_irq(struct ngbe_q_vector *q_vector,
				 struct ngbe_ring *rx_ring,
				 int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	struct ngbevf_adapter *adapter = q_vector->adapter;
	u16 cleaned_count = ngbe_desc_unused(rx_ring);
	struct sk_buff *skb = rx_ring->skb;
	bool xdp_xmit = false;
	struct xdp_buff xdp;

	xdp.data = NULL;
	xdp.data_end = NULL;
#ifdef HAVE_XDP_BUFF_RXQ
	xdp.rxq = &rx_ring->xdp_rxq;
#endif /* HAVE_XDP_BUFF_RXQ */

	while (likely(total_rx_packets < budget)) {
		struct ngbe_rx_buffer *rx_buffer;
		union ngbe_rx_desc *rx_desc;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= NGBE_RX_BUFFER_WRITE) {
			ngbe_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}

		rx_desc = NGBE_RX_DESC(rx_ring, rx_ring->next_to_clean);

		size = le16_to_cpu(rx_desc->wb.upper.length);
		if (!size)
			break;
		if (!(NGBE_RXD_STATUS(rx_desc) & NGBE_RXD_STAT_DD))
			break;

		/*
		 * This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * RXD_STAT_DD bit is set
		 */
		rmb();

		/* retrieve a buffer from the ring */
		rx_buffer = ngbe_get_rx_buffer(rx_ring, size);

		if (!skb) {
			xdp.data = page_address(rx_buffer->page) +
				   rx_buffer->page_offset;
#ifdef HAVE_XDP_BUFF_DATA_META
			xdp.data_meta = xdp.data;
#endif /* HAVE_XDP_BUFF_DATA_META */
			xdp.data_hard_start = xdp.data -
					      ngbe_rx_offset(rx_ring);
			xdp.data_end = xdp.data + size;

			skb = ngbe_run_xdp(adapter, rx_ring, &xdp);
		}

		if (IS_ERR(skb)) {
			if (PTR_ERR(skb) == -NGBE_XDP_TX) {
				xdp_xmit = true;
				ngbe_rx_buffer_flip(rx_ring, rx_buffer,
						       size);
			} else {
				rx_buffer->pagecnt_bias++;
			}
			total_rx_packets++;
			total_rx_bytes += size;
		} else if (skb) {
			ngbe_add_rx_frag(rx_ring, rx_buffer, skb, size);
#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC
		} else if (ring_uses_build_skb(rx_ring)) {
			skb = ngbe_build_skb(rx_ring, rx_buffer,
						&xdp, rx_desc);
#endif /* HAVE_SWIOTLB_SKIP_CPU_SYNC */
		} else {
			skb = ngbe_construct_skb(rx_ring, rx_buffer,
						    &xdp, rx_desc);
		}

		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			rx_buffer->pagecnt_bias++;
			break;
		}

		ngbe_put_rx_buffer(rx_ring, rx_buffer, skb);
		cleaned_count++;

		/* fetch next buffer in frame if non-eop */
		if (ngbe_is_non_eop(rx_ring, rx_desc))
			continue;

		/* verify the packet layout is correct */
		if (ngbe_cleanup_headers(rx_ring, rx_desc, skb)) {
			skb = NULL;
			continue;
		}

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* Workarouid hardware that can't do proper VEPA multicast
		 * source pruning.
		 */
		if ((skb->pkt_type == PACKET_BROADCAST ||
		     skb->pkt_type == PACKET_MULTICAST) &&
		    ether_addr_equal(rx_ring->netdev->dev_addr,
				     eth_hdr(skb)->h_source)) {
			dev_kfree_skb_irq(skb);
			continue;
		}

		/* populate checksum, VLAN, and protocol */
		ngbe_process_skb_fields(rx_ring, rx_desc, skb);

		ngbe_rx_skb(q_vector, skb);

		/* reset skb pointer */
		skb = NULL;

		/* update budget accounting */
		total_rx_packets++;
	}

	/* place incomplete frames back on ring for completion */
	rx_ring->skb = skb;

	if (xdp_xmit) {
		struct ngbe_ring *xdp_ring =
			adapter->xdp_ring[rx_ring->que_idx];

		/* Force memory writes to complete before letting h/w
		 * know there are new descriptors to fetch.
		 */
		wmb();
		writel(xdp_ring->next_to_use, xdp_ring->tail);
	}

	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	u64_stats_update_end(&rx_ring->syncp);
	q_vector->rx.total_packets += total_rx_packets;
	q_vector->rx.total_bytes += total_rx_bytes;
	q_vector->netpoll_rx = false;


	return total_rx_packets;
}

/**
 * ngbe_poll - NAPI polling calback
 * @napi: napi struct with our devices info in it
 * @budget: amount of work driver is allowed to do this pass, in packets
 *
 * This function will clean more than one or more rings associated with a
 * q_vector.
 **/
int ngbe_poll(struct napi_struct *napi, int budget)
{
	struct ngbe_q_vector *q_vector =
		container_of(napi, struct ngbe_q_vector, napi);
	struct ngbevf_adapter *adapter = q_vector->adapter;
	struct ngbe_ring *ring;
	int per_ring_budget, work_done = 0;
	bool clean_complete = true;

	ngbe_for_each_ring(ring, q_vector->tx) {
		if (!ngbe_clean_tx_irq(q_vector, ring, budget))
			clean_complete = false;
	}

	if (budget <= 0)
		return budget;

#ifdef CONFIG_NET_RX_BUSY_POLL
	if (test_bit(NAPI_STATE_NPSVC, &napi->state))
		return budget;

	if (!ngbe_qv_lock_napi(q_vector))
		return budget;
#endif

	/* attempt to distribute budget to each queue fairly, but don't allow
	 * the budget to go below 1 because we'll exit polling */
	if (q_vector->rx.count > 1)
		per_ring_budget = max(budget/q_vector->rx.count, 1);
	else
		per_ring_budget = budget;

	ngbe_for_each_ring(ring, q_vector->rx) {
		int cleaned = ngbe_clean_rx_irq(q_vector, ring,
						   per_ring_budget);
		work_done += cleaned;
		if (cleaned >= per_ring_budget)
			clean_complete = false;
	}
#ifdef CONFIG_NET_RX_BUSY_POLL
	ngbe_qv_unlock_napi(q_vector);
#endif

#ifndef HAVE_NETDEV_NAPI_LIST
	if (!netif_running(adapter->netdev))
		clean_complete = true;
#endif

	/* If all work not completed, return budget and keep polling */
	if (!clean_complete)
		return budget;
	/* all work done, exit the polling mode */
	napi_complete_done(napi, work_done);
	if (adapter->rx_itr_setting == 1)
		ngbe_set_itr(q_vector);
	if (!test_bit(__NGBE_DOWN, &adapter->state))
		ngbe_irq_enable_queues(adapter,
					  1 << q_vector->v_idx);

	return 0;
}

/**
 * ngbe_write_eitr - write VTEITR register in hardware specific way
 * @q_vector: structure containing interrupt and ring information
 */
void ngbe_write_eitr(struct ngbe_q_vector *q_vector)
{
	struct ngbevf_adapter *adapter = q_vector->adapter;
	struct ngbevf_hw *hw = &adapter->hw;
	int v_idx = q_vector->v_idx;
	u32 itr_reg = NGBE_VXITR_INTERVAL(q_vector->itr);

	/*
	 * set the WDIS bit to not clear the timer bits and cause an
	 * immediate assertion of the interrupt
	 */
	itr_reg |= NGBE_VXITR_CNT_WDIS;

	wr32(hw, NGBE_VXITR(v_idx), itr_reg);
}

#ifdef HAVE_NDO_BUSY_POLL
/* must be called with local_bh_disable()d */
int ngbe_busy_poll_recv(struct napi_struct *napi)
{
	struct ngbe_q_vector *q_vector =
			container_of(napi, struct ngbe_q_vector, napi);
	struct ngbevf_adapter *adapter = q_vector->adapter;
	struct ngbe_ring  *ring;
	int found = 0;

	if (test_bit(__NGBE_DOWN, &adapter->state))
		return LL_FLUSH_FAILED;

	if (!ngbe_qv_lock_poll(q_vector))
		return LL_FLUSH_BUSY;

	ngbe_for_each_ring(ring, q_vector->rx) {
		found = ngbe_clean_rx_irq(q_vector, ring, 4);
#ifdef BP_EXTENDED_STATS
		if (found)
			ring->stats.cleaned += found;
		else
			ring->stats.misses++;
#endif
		if (found)
			break;
	}

	ngbe_qv_unlock_poll(q_vector);

	return found;
}
#endif /* CONFIG_NET_RX_BUSY_POLL */

/**
 * ngbe_configure_msix - Configure MSI-X hardware
 * @adapter: board private structure
 *
 * ngbe_configure_msix sets up the hardware to properly generate MSI-X
 * interrupts.
 **/
void ngbe_configure_msix(struct ngbevf_adapter *adapter)
{
	struct ngbe_q_vector *q_vector;
	int v_idx;

	adapter->eims_enable_mask = 0;

	/*
	 * Populate the IVAR table and set the ITR values to the
	 * corresponding register.
	 */
	for (v_idx = 0; v_idx < adapter->num_q_vectors; v_idx++) {
		struct ngbe_ring *ring;
		q_vector = adapter->q_vector[v_idx];

		ngbe_for_each_ring(ring, q_vector->rx)
			ngbe_set_ivar(adapter, 0, ring->reg_idx, v_idx);

		ngbe_for_each_ring(ring, q_vector->tx)
			ngbe_set_ivar(adapter, 1, ring->reg_idx, v_idx);

		if (q_vector->tx.ring && !q_vector->rx.ring) {
			/* tx only vector */
			if (adapter->tx_itr_setting == 1)
				q_vector->itr = NGBE_12K_ITR;
			else
				q_vector->itr = adapter->tx_itr_setting;
		} else {
			/* rx or rx/tx vector */
			if (adapter->rx_itr_setting == 1)
				q_vector->itr = NGBE_20K_ITR;
			else
				q_vector->itr = adapter->rx_itr_setting;
		}

		/* add q_vector eims value to global eims_enable_mask */
		adapter->eims_enable_mask |= BIT(v_idx);

		ngbe_write_eitr(q_vector);
	}

	ngbe_set_ivar(adapter, -1, 1, v_idx);
	/* setup eims_other and add value to global eims_enable_mask */
	adapter->eims_other = BIT(v_idx);
	adapter->eims_enable_mask |= adapter->eims_other;
}

enum latency_range {
	lowest_latency = 0,
	low_latency = 1,
	bulk_latency = 2,
	latency_invalid = 255
};

/**
 * ngbe_update_itr - update the dynamic ITR value based on statistics
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
 *      this functionality is controlled by the InterruptThrottleRate module
 *      parameter (see ngbe_param.c)
 **/
void ngbe_update_itr(struct ngbe_q_vector *q_vector,
			       struct ngbe_ring_container *ring_container)
{
	int bytes = ring_container->total_bytes;
	int packets = ring_container->total_packets;
	u32 timepassed_us;
	u64 bytes_perint;
	u8 itr_setting = ring_container->itr;

	if (packets == 0)
		return;


	/* simple throttlerate management
	 *    0-20MB/s lowest (100000 ints/s)
	 *   20-100MB/s low   (20000 ints/s)
	 *  100-1249MB/s bulk (12000 ints/s)
	 */
	/* what was last interrupt timeslice? */
	timepassed_us = q_vector->itr >> 2;
	bytes_perint = bytes / timepassed_us; /* bytes/usec */

	switch (itr_setting) {
	case lowest_latency:
		if (bytes_perint > 10) {
			itr_setting = low_latency;
		}
		break;
	case low_latency:
		if (bytes_perint > 20) {
			itr_setting = bulk_latency;
		}
		else if (bytes_perint <= 10) {
			itr_setting = lowest_latency;
		}
		break;
	case bulk_latency:
		if (bytes_perint <= 20) {
			itr_setting = low_latency;
		}
		break;
	}

	/* clear work counters since we have the values we need */
	ring_container->total_bytes = 0;
	ring_container->total_packets = 0;

	/* write updated itr to ring container */
	ring_container->itr = itr_setting;
}

void ngbe_set_itr(struct ngbe_q_vector *q_vector)
{
	u32 new_itr = q_vector->itr;
	u8 current_itr;

	ngbe_update_itr(q_vector, &q_vector->tx);
	ngbe_update_itr(q_vector, &q_vector->rx);

	current_itr = max(q_vector->rx.itr, q_vector->tx.itr);

	switch (current_itr) {
	/* counts and packets in update_itr are dependent on these numbers */
	case lowest_latency:
		new_itr = NGBE_100K_ITR;
		break;
	case low_latency:
		new_itr = NGBE_20K_ITR;
		break;
	case bulk_latency:
		new_itr = NGBE_12K_ITR;
		break;
	default:
		break;
	}

	if (new_itr != q_vector->itr) {
		/* do an exponential smoothing */
		new_itr = (10 * new_itr * q_vector->itr) /
			  ((9 * new_itr) + q_vector->itr);

		/* save the algorithm value here */
		q_vector->itr = new_itr;

		ngbe_write_eitr(q_vector);
	}
}

irqreturn_t ngbe_msix_other(int __always_unused irq, void *data)
{
	struct ngbevf_adapter *adapter = data;
	struct ngbevf_hw *hw = &adapter->hw;

	hw->mac.get_link_status = 1;

	ngbe_service_event_schedule(adapter);

	wr32(hw, NGBE_VXIMC, adapter->eims_other);

	return IRQ_HANDLED;
}

/**
 * ngbe_msix_rings - single unshared vector rx clean (all queues)
 * @irq: unused
 * @data: pointer to our q_vector struct for this interrupt vector
 **/
irqreturn_t ngbe_msix_rings(int __always_unused irq, void *data)
{
	struct ngbe_q_vector *q_vector = data;

	/* EIAM disabled interrupts (on this vector) for us */
	if (q_vector->rx.ring || q_vector->tx.ring)
		napi_schedule_irqoff(&q_vector->napi);

	return IRQ_HANDLED;
}

/**
 * ngbe_request_msix_irqs - Initialize MSI-X interrupts
 * @adapter: board private structure
 *
 * ngbe_request_msix_irqs allocates MSI-X vectors and requests
 * interrupts from the kernel.
 **/
int ngbe_request_msix_irqs(struct ngbevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int vector, err;
	int ri = 0, ti = 0;

	for (vector = 0; vector < adapter->num_q_vectors; vector++) {
		struct ngbe_q_vector *q_vector = adapter->q_vector[vector];
		struct msix_entry *entry = &adapter->msix_entries[vector];

		if (q_vector->tx.ring && q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name) - 1,
				 "%s-%s-%d", netdev->name, "TxRx", ri++);
			ti++;
		} else if (q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name) - 1,
				 "%s-%s-%d", netdev->name, "rx", ri++);
		} else if (q_vector->tx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name) - 1,
				 "%s-%s-%d", netdev->name, "tx", ti++);
		} else {
			/* skip this unused q_vector */
			continue;
		}
		err = request_irq(entry->vector, &ngbe_msix_rings, 0,
				  q_vector->name, q_vector);
		if (err) {
			DPRINTK(PROBE, ERR,
				"request_irq failed for MSIX interrupt "
				"Error: %d\n", err);
			goto free_queue_irqs;
		}
	}

	err = request_irq(adapter->msix_entries[vector].vector,
			  &ngbe_msix_other, 0, netdev->name, adapter);
	if (err) {
		DPRINTK(PROBE, ERR,
			"request_irq for msix_other failed: %d\n", err);
		goto free_queue_irqs;
	}

	return 0;

free_queue_irqs:
	while (vector) {
		vector--;
		free_irq(adapter->msix_entries[vector].vector,
			 adapter->q_vector[vector]);
	}
	pci_disable_msix(adapter->pdev);
	kfree(adapter->msix_entries);
	adapter->msix_entries = NULL;
	return err;
}

/**
 * ngbe_request_irq - initialize interrupts
 * @adapter: board private structure
 *
 * Attempts to configure interrupts using the best available
 * capabilities of the hardware and kernel.
 **/
int ngbe_request_irq(struct ngbevf_adapter *adapter)
{
	int err;

	err = ngbe_request_msix_irqs(adapter);

	if (err)
		DPRINTK(PROBE, ERR, "request_irq failed, Error %d\n", err);

	return err;
}

void ngbe_free_irq(struct ngbevf_adapter *adapter)
{
	int vector;

	for (vector = 0; vector < adapter->num_q_vectors; vector++) {
		struct ngbe_q_vector *q_vector = adapter->q_vector[vector];
		struct msix_entry *entry = &adapter->msix_entries[vector];

		/* free only the irqs that were actually requested */
		if (!q_vector->rx.ring && !q_vector->tx.ring)
			continue;

		free_irq(entry->vector, q_vector);
	}

	free_irq(adapter->msix_entries[vector++].vector, adapter);
}

/**
 * ngbe_irq_disable - Mask off interrupt generation on the NIC
 * @adapter: board private structure
 **/
inline void ngbe_irq_disable(struct ngbevf_adapter *adapter)
{
	struct ngbevf_hw *hw = &adapter->hw;
	int vector;

	wr32(hw, NGBE_VXIMS, ~0);

	ngbe_flush(hw);

	for (vector = 0; vector < adapter->num_q_vectors; vector++)
		synchronize_irq(adapter->msix_entries[vector].vector);

	synchronize_irq(adapter->msix_entries[vector++].vector);
}

/**
 * ngbe_irq_enable - Enable default interrupt generation settings
 * @adapter: board private structure
 **/
inline void ngbe_irq_enable(struct ngbevf_adapter *adapter)
{
	struct ngbevf_hw *hw = &adapter->hw;

	wr32(hw, NGBE_VXIMC, adapter->eims_enable_mask);
}

/**
 * ngbe_configure_tx_ring - Configure Tx ring after Reset
 * @adapter: board private structure
 * @ring: structure containing ring specific data
 *
 * Configure the Tx descriptor ring after a reset.
 **/
void ngbe_configure_tx_ring(struct ngbevf_adapter *adapter,
			     struct ngbe_ring *ring)
{
	struct ngbevf_hw *hw = &adapter->hw;
	u64 tdba = ring->dma_addr;
	u32 txdctl = 0;
	u8 reg_idx = ring->reg_idx;

	/* disable queue to avoid issues while updating state */
	wr32(hw, NGBE_VXTXDCTL, NGBE_VXTXDCTL_FLUSH);
	ngbe_flush(hw);

	wr32(hw, NGBE_VXTDBAL, tdba & DMA_BIT_MASK(32));
	wr32(hw, NGBE_VXTDBAH, tdba >> 32);

	/* enable relaxed ordering */
	pcie_capability_clear_and_set_word(adapter->pdev, PCI_EXP_DEVCTL,
		0, PCI_EXP_DEVCTL_RELAX_EN);

	/* reset head and tail pointers */
	wr32(hw, NGBE_VXTDH, 0);
	wr32(hw, NGBE_VXTDT, 0);
	ring->tail = adapter->io_addr + NGBE_VXTDT;

	/* reset ntu and ntc to place SW in sync with hardwdare */
	ring->next_to_clean = 0;
	ring->next_to_use = 0;

	txdctl |= NGBE_VXTXDCTL_BUFLEN(ngbe_buf_len(ring->count));
	txdctl |= NGBE_VXTXDCTL_ENABLE;

	/* set WTHRESH to encourage burst writeback, it should not be set
	 * higher than 1 when ITR is 0 as it could cause false TX hangs
	 *
	 * In order to avoid issues WTHRESH + PTHRESH should always be equal
	 * to or less than the number of on chip descriptors, which is
	 * currently 40.
	 */
#if 0 /* fixme */
	if (!ring->q_vector || !ring->q_vector->itr)
		txdctl |= NGBE_VXTXDCTL_WTHRESH(0);
	else
		txdctl |= NGBE_VXTXDCTL_WTHRESH((ring->q_vector->itr + 63) / 64);
#endif
	/* reinitialize tx_buffer_info */
	memset(ring->tx_buffer_info, 0,
	       sizeof(struct ngbe_tx_buffer) * ring->count);

	clear_bit(__NGBE_HANG_CHECK_ARMED, &ring->state);
	clear_bit(__NGBE_TX_XDP_RING_PRIMED, &ring->state);

	wr32(hw, NGBE_VXTXDCTL, txdctl);
	/* poll to verify queue is enabled */
	if (po32m(hw, NGBE_VXTXDCTL,
		NGBE_VXTXDCTL_ENABLE, NGBE_VXTXDCTL_ENABLE, 1000, 10))
		DPRINTK(PROBE, ERR, "Could not enable Tx Queue %d\n", reg_idx);
}

/**
 * ngbe_configure_tx - Configure Transmit Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Tx unit of the MAC after a reset.
 **/
void ngbe_configure_tx(struct ngbevf_adapter *adapter)
{
	u32 i;

	/* Setup the HW Tx Head and Tail descriptor pointers */
	for (i = 0; i < adapter->num_tx_queues; i++)
		ngbe_configure_tx_ring(adapter, adapter->tx_ring[i]);
	for (i = 0; i < adapter->num_xdp_queues; i++)
		ngbe_configure_tx_ring(adapter, adapter->xdp_ring[i]);
}

void ngbe_configure_srrctl(struct ngbevf_adapter *adapter, 
					struct ngbe_ring *ring, int index)
{
	struct ngbevf_hw *hw = &adapter->hw;
	u32 srrctl;

	srrctl = rd32m(hw, NGBE_VXRXDCTL,
			~(NGBE_VXRXDCTL_HDRSZ(~0) | NGBE_VXRXDCTL_BUFSZ(~0)));
	srrctl |= NGBE_VXRXDCTL_DROP;
	srrctl |= NGBE_VXRXDCTL_HDRSZ(ngbe_hdr_sz(NGBE_RX_HDR_SIZE));
	if (ring_uses_large_buffer(ring))
		srrctl |= NGBE_VXRXDCTL_BUFSZ(ngbe_buf_sz(3072));
	else
	srrctl |= NGBE_VXRXDCTL_BUFSZ(ngbe_buf_sz(NGBE_RX_BUF_SIZE));

	wr32(hw, NGBE_VXRXDCTL, srrctl);
}

void ngbe_setup_psrtype(struct ngbevf_adapter *adapter)
{
	struct ngbevf_hw *hw = &adapter->hw;

	/* PSRTYPE must be initialized */
	u32 psrtype = NGBE_VXMRQC_PSR_L2HDR |
		      NGBE_VXMRQC_PSR_L3HDR |
		      NGBE_VXMRQC_PSR_L4HDR |
		      NGBE_VXMRQC_PSR_TUNHDR |
		      NGBE_VXMRQC_PSR_TUNMAC;

	wr32m(hw, NGBE_VXMRQC, NGBE_VXMRQC_PSR(~0), NGBE_VXMRQC_PSR(psrtype));
}

void ngbe_disable_rx_queue(struct ngbevf_adapter *adapter,
				     struct ngbe_ring *ring)
{
	struct ngbevf_hw *hw = &adapter->hw;
	u32 rxdctl;
	u8 reg_idx = ring->reg_idx;

	if (NGBE_REMOVED(hw->hw_addr))
		return;
	rxdctl = rd32(hw, NGBE_VXRXDCTL);
	rxdctl &= ~NGBE_VXRXDCTL_ENABLE;

	/* write value back with RXDCTL.ENABLE bit cleared */
	wr32(hw, NGBE_VXRXDCTL, rxdctl);

	/* the hardware may take up to 100us to really disable the rx queue */
	if (po32m(hw, NGBE_VXRXDCTL,
		NGBE_VXRXDCTL_ENABLE, 0, 10, 10))
		DPRINTK(PROBE, ERR,
			"RXDCTL.ENABLE queue %d not cleared while polling\n",
			reg_idx);
}

void ngbe_rx_desc_queue_enable(struct ngbevf_adapter *adapter,
					 struct ngbe_ring *ring)
{
	struct ngbevf_hw *hw = &adapter->hw;
	u8 reg_idx = ring->reg_idx;

	if (NGBE_REMOVED(hw->hw_addr))
		return;
	if (po32m(hw, NGBE_VXRXDCTL,
		NGBE_VXRXDCTL_ENABLE, NGBE_VXRXDCTL_ENABLE, 1000, 10))
		DPRINTK(PROBE, ERR,
			"RXDCTL.ENABLE queue %d not set while polling\n",
			reg_idx);
}

void ngbe_configure_rx_ring(struct ngbevf_adapter *adapter,
			     struct ngbe_ring *ring)
{
	struct ngbevf_hw *hw = &adapter->hw;
	u64 rdba = ring->dma_addr;
	u32 rxdctl;
	union ngbe_rx_desc *rx_desc;
	u8 reg_idx = ring->reg_idx;

	/* disable queue to avoid issues while updating state */
	rxdctl = rd32(hw, NGBE_VXRXDCTL);
	ngbe_disable_rx_queue(adapter, ring);

	wr32(hw, NGBE_VXRDBAL, rdba & DMA_BIT_MASK(32));
	wr32(hw, NGBE_VXRDBAH, rdba >> 32);

	/* enable relaxed ordering */
	pcie_capability_clear_and_set_word(adapter->pdev, PCI_EXP_DEVCTL,
		0, PCI_EXP_DEVCTL_RELAX_EN);

	/* reset head and tail pointers */
	wr32(hw, NGBE_VXRDH, 0);
	wr32(hw, NGBE_VXRDT, 0);
	ring->tail = adapter->io_addr + NGBE_VXRDT;

	/* initialize rx_buffer_info */
	memset(ring->rx_buffer_info, 0,
	       sizeof(struct ngbe_rx_buffer) * ring->count);

	/* initialize Rx descriptor 0 */
	rx_desc = NGBE_RX_DESC(ring, 0);
	rx_desc->wb.upper.length = 0;

	/* reset ntu and ntc to place SW in sync with hardwdare */
	ring->next_to_clean = 0;
	ring->next_to_use = 0;
	ring->next_to_alloc = 0;

	ngbe_configure_srrctl(adapter, ring,reg_idx);

	/* allow any size packet since we can handle overflow */
	rxdctl &= ~NGBE_VXRXDCTL_BUFLEN(~0);
#if 0
#if (PAGE_SIZE < 8192)
		/* Limit the maximum frame size so we don't overrun the skb */
		if (ring_uses_build_skb(ring) &&
		    !ring_uses_large_buffer(ring))
			rxdctl |= TXGBE_VXRXDCTL_BUFLEN(TXGBE_MAX_FRAME_BUILD_SKB);
#endif
#endif
	rxdctl |= NGBE_VXRXDCTL_BUFLEN(ngbe_buf_len(ring->count));
	rxdctl |= NGBE_VXRXDCTL_ENABLE | NGBE_VXRXDCTL_VLAN;

	wr32(hw, NGBE_VXRXDCTL, rxdctl);

	ngbe_rx_desc_queue_enable(adapter, ring);
	ngbe_alloc_rx_buffers(ring, ngbe_desc_unused(ring));
}

#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC
static void ngbe_set_rx_buffer_len(struct ngbevf_adapter *adapter,
				      struct ngbe_ring *rx_ring)
{
	struct net_device *netdev = adapter->netdev;
	unsigned int max_frame = netdev->mtu + ETH_HLEN + ETH_FCS_LEN;

	/* set build_skb and buffer size flags */
	clear_ring_build_skb_enabled(rx_ring);
	clear_ring_uses_large_buffer(rx_ring);

	if (adapter->flags & NGBE_FLAGS_LEGACY_RX)
		return;

	set_ring_build_skb_enabled(rx_ring);

	if (PAGE_SIZE < 8192) {
		if (max_frame <= NGBE_MAX_FRAME_BUILD_SKB)
			return;

		set_ring_uses_large_buffer(rx_ring);
	}
}

#endif /* HAVE_SWIOTLB_SKIP_CPU_SYNC */
/**
 * ngbe_configure_rx - Configure Receive Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Rx unit of the MAC after a reset.
 **/
void ngbe_configure_rx(struct ngbevf_adapter *adapter)
{
	struct ngbevf_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	int i, ret;

	ngbe_setup_psrtype(adapter);

	spin_lock_bh(&adapter->mbx_lock);
	ret = ngbe_rlpml_set_vf(hw, netdev->mtu + ETH_HLEN + ETH_FCS_LEN);
	spin_unlock_bh(&adapter->mbx_lock);
	if (ret)
		DPRINTK(HW, DEBUG, "Failed to set MTU at %d\n", netdev->mtu);

	/* Setup the HW Rx Head and Tail Descriptor Pointers and
	 * the Base and Length of the Rx Descriptor Ring
	 */
	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct ngbe_ring *rx_ring = adapter->rx_ring[i];
#ifdef HAVE_SWIOTLB_SKIP_CPU_SYNC
		ngbe_set_rx_buffer_len(adapter, rx_ring);
#endif
		ngbe_configure_rx_ring(adapter, rx_ring);
	}
}

#ifdef HAVE_VLAN_RX_REGISTER
void ngbe_vlan_rx_register(struct net_device *netdev,
				     struct vlan_group *grp)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbevf_hw *hw = &adapter->hw;
	int i, j;
	u32 ctrl;

	adapter->vlgrp = grp;

//	for (i = 0; i < adapter->num_rx_queues; i++) {
//		j = adapter->rx_ring[i]->reg_idx;
		ctrl = rd32(hw, NGBE_VXRXDCTL);
		ctrl |= NGBE_VXRXDCTL_VLAN;
		wr32(hw, NGBE_VXRXDCTL, ctrl);
//	}
}
#endif /* HAVE_VLAN_RX_REGISTER */

#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
#ifdef HAVE_INT_NDO_VLAN_RX_ADD_VID
#ifdef NETIF_F_HW_VLAN_CTAG_TX
int ngbe_vlan_rx_add_vid(struct net_device *netdev,
				   __always_unused __be16 proto, u16 vid)
#else
int ngbe_vlan_rx_add_vid(struct net_device *netdev, u16 vid)
#endif
#else  /* HAVE_INT_NDO_VLAN_RX_ADD_VID */
void ngbe_vlan_rx_add_vid(struct net_device *netdev, u16 vid)
#endif /* HAVE_INT_NDO_VLAN_RX_ADD_VID */
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbevf_hw *hw = &adapter->hw;
#ifndef HAVE_NETDEV_VLAN_FEATURES
	struct net_device *v_netdev;
#endif /* HAVE_NETDEV_VLAN_FEATURES */
	int err;

	spin_lock_bh(&adapter->mbx_lock);

	/* add VID to filter table */
	err = TCALL(hw, mac.ops.set_vfta, vid, 0, true, false);

	spin_unlock_bh(&adapter->mbx_lock);

	if (err) {
		printk(KERN_ERR "VF could not set VLAN %d\n", vid);
	} else {
#ifndef HAVE_NETDEV_VLAN_FEATURES

		/*
		 * Copy feature flags from netdev to the vlan netdev for this
		 * vid.  This allows things like TSO to bubble down to our
		 * vlan device.
		 */
		if (adapter->vlgrp) {
			v_netdev = vlan_group_get_device(adapter->vlgrp, vid);
			if (v_netdev) {
				v_netdev->features |= adapter->netdev->features;
				vlan_group_set_device(adapter->vlgrp,
						      vid, v_netdev);
			}
		}
#endif /* HAVE_NETDEV_VLAN_FEATURES */
#ifndef HAVE_VLAN_RX_REGISTER
		if (vid < VLAN_N_VID)
			set_bit(vid, adapter->active_vlans);
#endif
	}

#ifdef HAVE_INT_NDO_VLAN_RX_ADD_VID
	return err;
#endif
}

#ifdef HAVE_INT_NDO_VLAN_RX_ADD_VID
#ifdef NETIF_F_HW_VLAN_CTAG_RX
int ngbe_vlan_rx_kill_vid(struct net_device *netdev,
				  __always_unused __be16 proto, u16 vid)
#else /* !NETIF_F_HW_VLAN_CTAG_RX */
int ngbe_vlan_rx_kill_vid(struct net_device *netdev, u16 vid)
#endif /* NETIF_F_HW_VLAN_CTAG_RX */
#else
void ngbe_vlan_rx_kill_vid(struct net_device *netdev, u16 vid)
#endif
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbevf_hw *hw = &adapter->hw;

#ifndef HAVE_NETDEV_VLAN_FEATURES
	if (!test_bit(__NGBE_DOWN, &adapter->state))
		ngbe_irq_disable(adapter);

	vlan_group_set_device(adapter->vlgrp, vid, NULL);

	if (!test_bit(__NGBE_DOWN, &adapter->state))
		ngbe_irq_enable(adapter);
#endif

	spin_lock_bh(&adapter->mbx_lock);

	/* remove VID from filter table */
	TCALL(hw, mac.ops.set_vfta, vid, 0, false, false);

	spin_unlock_bh(&adapter->mbx_lock);

#ifndef HAVE_VLAN_RX_REGISTER
	clear_bit(vid, adapter->active_vlans);
#endif
#ifdef HAVE_INT_NDO_VLAN_RX_ADD_VID
	return 0;
#endif
}

void ngbe_restore_vlan(struct ngbevf_adapter *adapter)
{
	u16 vid;

#ifdef HAVE_VLAN_RX_REGISTER
	ngbe_vlan_rx_register(adapter->netdev, adapter->vlgrp);

	if (adapter->vlgrp) {
		for (vid = 0; vid < VLAN_N_VID; vid++) {
			if (!vlan_group_get_device(adapter->vlgrp, vid))
				continue;
#ifdef NETIF_F_HW_VLAN_CTAG_RX
			ngbe_vlan_rx_add_vid(adapter->netdev,
						htons(ETH_P_8021Q),
						vid);
#else
			ngbe_vlan_rx_add_vid(adapter->netdev, vid);
#endif
		}
	}
#else /* !HAVE_VLAN_RX_REGISTER */
	for_each_set_bit(vid, adapter->active_vlans, VLAN_N_VID)
#ifdef NETIF_F_HW_VLAN_CTAG_RX
		ngbe_vlan_rx_add_vid(adapter->netdev, htons(ETH_P_8021Q),
					vid);
#else
		ngbe_vlan_rx_add_vid(adapter->netdev, vid);
#endif
#endif /* HAVE_VLAN_RX_REGISTER */
}
#endif /* NETIF_F_HW_VLAN_TX || NETIF_F_HW_VLAN_CTAG_TX */

u8 *ngbe_addr_list_itr(struct ngbevf_hw __maybe_unused *hw, u8 **mc_addr_ptr,
				 u32 *vmdq)
{
#ifdef NETDEV_HW_ADDR_T_MULTICAST
	struct netdev_hw_addr *mc_ptr;
#else
	struct dev_mc_list *mc_ptr;
#endif
	u8 *addr = *mc_addr_ptr;
	*vmdq = 0;

#ifdef NETDEV_HW_ADDR_T_MULTICAST
	mc_ptr = container_of(addr, struct netdev_hw_addr, addr[0]);
	if (mc_ptr->list.next) {
		struct netdev_hw_addr *ha;

		ha = list_entry(mc_ptr->list.next, struct netdev_hw_addr, list);
		*mc_addr_ptr = ha->addr;
	}
#else
	mc_ptr = container_of(addr, struct dev_mc_list, dmi_addr[0]);
	if (mc_ptr->next)
		*mc_addr_ptr = mc_ptr->next->dmi_addr;
#endif
	else
		*mc_addr_ptr = NULL;

	return addr;
}

#ifdef NETDEV_HW_ADDR_T_UNICAST
int ngbe_write_uc_addr_list(struct net_device *netdev)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbevf_hw *hw = &adapter->hw;
	int count = 0;

	if (!netdev_uc_empty(netdev)) {
		struct netdev_hw_addr *ha;
		netdev_for_each_uc_addr(ha, netdev) {
			TCALL(hw, mac.ops.set_uc_addr, ++count, ha->addr);
			udelay(200);
		}
	} else {
		/*
		 * If the list is empty then send message to PF driver to
		 * clear all macvlans on this VF.
		 */
		TCALL(hw, mac.ops.set_uc_addr, 0, NULL);
	}

	return count;
}

#endif /* NETDEV_HW_ADDR_T_UNICAST */
/**
 * ngbe_set_rx_mode - Multicast and unicast set
 * @netdev: network interface device structure
 *
 * The set_rx_method entry point is called whenever the multicast address
 * list, unicast address list or the network interface flags are updated.
 * This routine is responsible for configuring the hardware for proper
 * multicast mode and configuring requested unicast filters.
 **/
void ngbe_set_rx_mode(struct net_device *netdev)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbevf_hw *hw = &adapter->hw;
	unsigned int flags = netdev->flags;
	int xcast_mode;
	u8 *addr_list = NULL;
	int addr_count = 0;

	xcast_mode = (flags & IFF_ALLMULTI) ? NGBE_XCAST_MODE_ALLMULTI :
		     (flags & (IFF_BROADCAST | IFF_MULTICAST)) ?
		     NGBE_XCAST_MODE_MULTI : NGBE_XCAST_MODE_NONE;

	/* reprogram multicast list */
	addr_count = netdev_mc_count(netdev);
	if (addr_count) {
#ifdef NETDEV_HW_ADDR_T_MULTICAST
		struct netdev_hw_addr *ha;
		ha = list_first_entry(&netdev->mc.list,
			struct netdev_hw_addr, list);
		addr_list = ha->addr;
#else
		addr_list = netdev->mc_list->dmi_addr;
#endif
	}

	spin_lock_bh(&adapter->mbx_lock);

	TCALL(hw, mac.ops.update_xcast_mode, xcast_mode);

	TCALL(hw, mac.ops.update_mc_addr_list, addr_list, addr_count,
					ngbe_addr_list_itr, false);

#ifdef NETDEV_HW_ADDR_T_UNICAST
	ngbe_write_uc_addr_list(netdev);
#endif

	spin_unlock_bh(&adapter->mbx_lock);
}

void ngbe_napi_enable_all(struct ngbevf_adapter *adapter)
{
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++) {
#ifdef CONFIG_NET_RX_BUSY_POLL
		ngbe_qv_init_lock(adapter->q_vector[q_idx]);
#endif
		napi_enable(&adapter->q_vector[q_idx]->napi);
	}
}

void ngbe_napi_disable_all(struct ngbevf_adapter *adapter)
{
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++) {
		napi_disable(&adapter->q_vector[q_idx]->napi);
#ifdef CONFIG_NET_RX_BUSY_POLL
		while(!ngbe_qv_disable(adapter->q_vector[q_idx])) {
			pr_info("QV %d locked\n", q_idx);
			usleep_range(1000, 20000);
		}
#endif /* CONFIG_NET_RX_BUSY_POLL */
	}
}

void ngbe_configure(struct ngbevf_adapter *adapter)
{

	ngbe_set_rx_mode(adapter->netdev);

#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	ngbe_restore_vlan(adapter);

#endif
	ngbe_configure_tx(adapter);
	ngbe_configure_rx(adapter);
}

void ngbe_save_reset_stats(struct ngbevf_adapter *adapter)
{
	/* Only save pre-reset stats if there are some */
	if (adapter->stats.gprc || adapter->stats.gptc) {
		adapter->reset_stats.gprc += adapter->stats.gprc -
			adapter->base_stats.gprc;
		adapter->reset_stats.gptc += adapter->stats.gptc -
			adapter->base_stats.gptc;
		adapter->reset_stats.gorc += adapter->stats.gorc -
			adapter->base_stats.gorc;
		adapter->reset_stats.gotc += adapter->stats.gotc -
			adapter->base_stats.gotc;
		adapter->reset_stats.mprc += adapter->stats.mprc -
			adapter->base_stats.mprc;
		adapter->reset_stats.bprc += adapter->stats.bprc -
			adapter->base_stats.bprc;
		adapter->reset_stats.mptc += adapter->stats.mptc -
			adapter->base_stats.mptc;
		adapter->reset_stats.bptc += adapter->stats.bptc -
			adapter->base_stats.bptc;
	}
}

void ngbe_init_last_counter_stats(struct ngbevf_adapter *adapter)
{
	struct ngbevf_hw *hw = &adapter->hw;

	adapter->last_stats.gprc = rd32(hw, NGBE_VXGPRC);
	adapter->last_stats.gorc = rd32(hw, NGBE_VXGORC_LSB);
	adapter->last_stats.gorc |=
		(((u64)(rd32(hw, NGBE_VXGORC_MSB))) << 32);
	adapter->last_stats.gptc = rd32(hw, NGBE_VXGPTC);
	adapter->last_stats.gotc = rd32(hw, NGBE_VXGOTC_LSB);
	adapter->last_stats.gotc |=
		(((u64)(rd32(hw, NGBE_VXGOTC_MSB))) << 32);
	adapter->last_stats.mprc = rd32(hw, NGBE_VXMPRC);
	adapter->last_stats.bprc = rd32(hw, NGBE_VXBPRC);
	adapter->last_stats.mptc = rd32(hw, NGBE_VXMPTC);
	adapter->last_stats.bptc = rd32(hw, NGBE_VXBPTC);

	adapter->base_stats.gprc = adapter->last_stats.gprc;
	adapter->base_stats.gorc = adapter->last_stats.gorc;
	adapter->base_stats.gptc = adapter->last_stats.gptc;
	adapter->base_stats.gotc = adapter->last_stats.gotc;
	adapter->base_stats.mprc = adapter->last_stats.mprc;
	adapter->base_stats.bprc = adapter->last_stats.bprc;
	adapter->base_stats.mptc = adapter->last_stats.mptc;
	adapter->base_stats.bptc = adapter->last_stats.bptc;
}

void ngbe_negotiate_api(struct ngbevf_adapter *adapter)
{
	struct ngbevf_hw *hw = &adapter->hw;
	int api[] = {
			ngbe_mbox_api_13,
			ngbe_mbox_api_12,
			ngbe_mbox_api_11,
			ngbe_mbox_api_10,
			ngbe_mbox_api_max };
	int err = 0, idx = 0;

	spin_lock_bh(&adapter->mbx_lock);
	
	while (api[idx] != ngbe_mbox_api_max) {
			err = ngbe_negotiate_api_version(hw, api[idx]);
			if (!err)
				break;
			idx++;
	}
	spin_unlock_bh(&adapter->mbx_lock);

}

void ngbe_up_complete(struct ngbevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct ngbevf_hw *hw = &adapter->hw;

#ifdef CONFIG_NETDEVICES_MULTIQUEUE
	if (adapter->num_tx_queues > 1)
		netdev->features |= NETIF_F_MULTI_QUEUE;

#endif
	ngbe_configure_msix(adapter);

	spin_lock_bh(&adapter->mbx_lock);

	if (is_valid_ether_addr(hw->mac.addr))
		TCALL(hw, mac.ops.set_rar, 0, hw->mac.addr, 0, 0);
	else
		TCALL(hw, mac.ops.set_rar, 0, hw->mac.perm_addr, 0, 0);

	spin_unlock_bh(&adapter->mbx_lock);

	smp_mb__before_atomic();
	clear_bit(__NGBE_DOWN, &adapter->state);
	ngbe_napi_enable_all(adapter);

	/* clear any pending interrupts, may auto mask */
	wr32(hw, NGBE_VXICR, ~0);
	ngbe_irq_enable(adapter);

	/* enable transmits */
	netif_tx_start_all_queues(netdev);

	ngbe_save_reset_stats(adapter);
	ngbe_init_last_counter_stats(adapter);

	hw->mac.get_link_status = 1;
	mod_timer(&adapter->service_timer, jiffies);
}

void ngbe_up(struct ngbevf_adapter *adapter)
{
	ngbe_configure(adapter);

	ngbe_up_complete(adapter);
}

/**
 * ngbe_clean_rx_ring - Free Rx Buffers per Queue
 * @rx_ring: ring to free buffers from
 **/
void ngbe_clean_rx_ring(struct ngbe_ring *rx_ring)
{
	u16 i = rx_ring->next_to_clean;
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
	DEFINE_DMA_ATTRS(attrs);

	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_set_attr(DMA_ATTR_WEAK_ORDERING, &attrs);
#endif

	/* Free Rx ring sk_buff */
	if (rx_ring->skb) {
		dev_kfree_skb(rx_ring->skb);
		rx_ring->skb = NULL;
	}

	/* Free all the Rx ring pages */
	while (i != rx_ring->next_to_alloc) {
		struct ngbe_rx_buffer *rx_buffer;

		rx_buffer = &rx_ring->rx_buffer_info[i];

		/* Invalidate cache lines that may have been written to by
		 * device so that we avoid corrupting memory.
		 */
		dma_sync_single_range_for_cpu(rx_ring->dev,
					      rx_buffer->dma_addr,
					      rx_buffer->page_offset,
					      ngbe_rx_bufsz(rx_ring),
					      DMA_FROM_DEVICE);

		/* free resources associated with mapping */
		dma_unmap_page_attrs(rx_ring->dev,
				     rx_buffer->dma_addr,
				     ngbe_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE,
#if defined(HAVE_STRUCT_DMA_ATTRS) && defined(HAVE_SWIOTLB_SKIP_CPU_SYNC)
				     &attrs);
#else
				     NGBE_RX_DMA_ATTR);
#endif

		__page_frag_cache_drain(rx_buffer->page,
					rx_buffer->pagecnt_bias);

		i++;
		if (i == rx_ring->count)
			i = 0;
	}

	rx_ring->next_to_alloc = 0;
	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
}

/**
 * ngbe_clean_tx_ring - Free Tx Buffers
 * @tx_ring: ring to be cleaned
 **/
void ngbe_clean_tx_ring(struct ngbe_ring *tx_ring)
{
	u16 i = tx_ring->next_to_clean;
	struct ngbe_tx_buffer *tx_buffer = &tx_ring->tx_buffer_info[i];

	while (i != tx_ring->next_to_use) {
		struct ngbe_tx_desc *eop_desc, *tx_desc;

		/* Free all the Tx ring sk_buffs */
#ifdef HAVE_XDP_SUPPORT
		if (ring_is_xdp(tx_ring))
			page_frag_free(tx_buffer->data);
		else
			dev_kfree_skb_any(tx_buffer->skb);
#else
		dev_kfree_skb_any(tx_buffer->skb);
#endif

		/* unmap skb header data */
		dma_unmap_single(tx_ring->dev,
				 dma_unmap_addr(tx_buffer, dma),
				 dma_unmap_len(tx_buffer, len),
				 DMA_TO_DEVICE);

		/* check for eop_desc to determine the end of the packet */
		eop_desc = tx_buffer->next_to_watch;
		tx_desc = NGBE_TX_DESC(tx_ring, i);

		/* unmap remaining buffers */
		while (tx_desc != eop_desc) {
			tx_buffer++;
			tx_desc++;
			i++;
			if (unlikely(i == tx_ring->count)) {
				i = 0;
				tx_buffer = tx_ring->tx_buffer_info;
				tx_desc = NGBE_TX_DESC(tx_ring, 0);
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

	/* reset BQL for queue */
	if (!ring_is_xdp(tx_ring))
		netdev_tx_reset_queue(txring_txq(tx_ring));

	/* reset next_to_use and next_to_clean */
	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;
}

/**
 * ngbe_clean_all_rx_rings - Free Rx Buffers for all queues
 * @adapter: board private structure
 **/
void ngbe_clean_all_rx_rings(struct ngbevf_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		ngbe_clean_rx_ring(adapter->rx_ring[i]);
}

/**
 * ngbe_clean_all_tx_rings - Free Tx Buffers for all queues
 * @adapter: board private structure
 **/
void ngbe_clean_all_tx_rings(struct ngbevf_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		ngbe_clean_tx_ring(adapter->tx_ring[i]);
	for (i = 0; i < adapter->num_xdp_queues; i++)
		ngbe_clean_tx_ring(adapter->xdp_ring[i]);
}

void ngbe_down(struct ngbevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct ngbevf_hw *hw = &adapter->hw;
	int i;

	/* signal that we are down to the interrupt handler */
	if (test_and_set_bit(__NGBE_DOWN, &adapter->state))
		return; /* do nothing if already down */

	/* disable all enabled rx queues */
	for (i = 0; i < adapter->num_rx_queues; i++)
		ngbe_disable_rx_queue(adapter, adapter->rx_ring[i]);

	usleep_range(10000, 20000);

	netif_tx_stop_all_queues(netdev);

	/* call carrier off first to avoid false dev_watchdog timeouts */
	netif_carrier_off(netdev);
	netif_tx_disable(netdev);

	ngbe_irq_disable(adapter);

	ngbe_napi_disable_all(adapter);

	del_timer_sync(&adapter->service_timer);

	/* disable transmits in the hardware now that interrupts are off */
	wr32(hw, NGBE_VXTXDCTL, NGBE_VXTXDCTL_FLUSH);

#ifdef HAVE_PCI_ERS
	if (!pci_channel_offline(adapter->pdev))
#endif
		ngbe_reset(adapter);

	ngbe_clean_all_tx_rings(adapter);
	ngbe_clean_all_rx_rings(adapter);
}

void ngbevf_reinit_locked(struct ngbevf_adapter *adapter)
{
	WARN_ON(in_interrupt());

	while (test_and_set_bit(__NGBE_RESETTING, &adapter->state))
		msleep(1);
	
	/*ngbe_down +  free_irq*/
	ngbe_down(adapter);
	ngbe_free_irq(adapter);
	
	pci_set_master(adapter->pdev);

	/*ngbe_up +  request_irq*/
	ngbe_configure(adapter);
	ngbe_request_irq(adapter);
	ngbe_up_complete(adapter);

	clear_bit(__NGBE_RESETTING, &adapter->state);
}

void ngbe_reset(struct ngbevf_adapter *adapter)
{
	struct ngbevf_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	s32 err;

	if (NGBE_REMOVED(hw->hw_addr))
		return;

	err = TCALL(hw, mac.ops.reset_hw);
	if (!err)
		err = TCALL(hw, mac.ops.init_hw);

	if (err)
		DPRINTK(PROBE, ERR, "reset function failed, err=%d\n", err);
	else
		ngbe_negotiate_api(adapter);

	if (is_valid_ether_addr(adapter->hw.mac.addr)) {
		ether_addr_copy(netdev->dev_addr, adapter->hw.mac.addr);
		ether_addr_copy(netdev->perm_addr, adapter->hw.mac.addr);
	}

	adapter->last_reset = jiffies;
}

int ngbe_acquire_msix_vectors(struct ngbevf_adapter *adapter,
					int vectors)
{
	int vector_threshold;

	/* We'll want at least 2 (vector_threshold):
	*	* 1) TxQ[0] + RxQ[0] handler
	*		* 2) Other (Link Status Change, etc.)
	*			*/
	vector_threshold = MIN_MSIX_COUNT;
	
	/* The more we get, the more we will assign to Tx/Rx Cleanup
	*	* for the separate queues...where Rx Cleanup >= Tx Cleanup.
	*		* Right now, we simply care about how many we'll get; we'll
	*			* set them up later while requesting irq's.
	*				*/
	vectors = pci_enable_msix_range(adapter->pdev, adapter->msix_entries,
					vector_threshold, vectors);
	
	if (vectors < 0) {
		DPRINTK(HW, DEBUG, "Unable to allocate MSI-X interrupts\n");
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;
		return vectors;
	}
	
	/* Adjust for only the vectors we'll use, which is the number of
	*	* vectors we were allocated.
	*		*/
	adapter->num_q_vectors = vectors - NON_Q_VECTORS;
	
	return 0;

}

/*
 * ngbe_set_num_queues - Allocate queues for device, feature dependent
 * @adapter: board private structure to initialize
 *
 * This is the top level queue allocation routine.  The order here is very
 * important, starting with the "most" number of features turned on at once,
 * and ending with the smallest set of features.  This way large combinations
 * can be allocated if they're turned on, and smaller combinations are the
 * fallthrough conditions.
 *
 **/
void ngbe_set_num_queues(struct ngbevf_adapter *adapter)
{
	struct ngbevf_hw *hw = &adapter->hw;
	unsigned int def_q = 0;
	unsigned int num_tcs = 0;
	int err;

	/* Start with base case */
	adapter->num_rx_queues = 1;
	adapter->num_tx_queues = 1;
	adapter->num_xdp_queues = 0;

	spin_lock_bh(&adapter->mbx_lock);
	/* fetch queue configuration from the PF */
	err = ngbe_get_queues(hw, &num_tcs, &def_q);
	spin_unlock_bh(&adapter->mbx_lock);

	if (err)
		return;

	/* we need as many queues as traffic classes */
	if (num_tcs > 1) {
		adapter->num_rx_queues = num_tcs;
	} else {
		/*emerald 1 queue per vf*/
		u16 rss = min_t(u16, num_online_cpus(), 1);

		switch (hw->api_version) {
		case ngbe_mbox_api_11:
		case ngbe_mbox_api_12:
		case ngbe_mbox_api_13:
			if (adapter->xdp_prog &&
			    hw->mac.max_tx_queues == rss)
				rss = rss > 3 ? 2 : 1;
			adapter->num_rx_queues = rss;
#ifdef HAVE_TX_MQ
			adapter->num_tx_queues = rss;
#endif
			adapter->num_xdp_queues = adapter->xdp_prog ? rss : 0;
		default:
			break;
		}
	}
}

/**
 * ngbe_set_interrupt_capability - set MSI-X or FAIL if not supported
 * @adapter: board private structure to initialize
 *
 * Attempt to configure the interrupts using the best available
 * capabilities of the hardware and the kernel.
 */
int ngbe_set_interrupt_capability(struct ngbevf_adapter *adapter)
{
	int vector, v_budget;

	/* It's easy to be greedy for MSI-X vectors, but it really
	 * doesn't do us much good if we have a lot more vectors
	 * than CPU's.  So let's be conservative and only ask for
	 * (roughly) the same number of vectors as there are CPU's.
	 * The default is to use pairs of vectors.
	 */
	v_budget = max(adapter->num_rx_queues, adapter->num_tx_queues);
	v_budget = min_t(int, v_budget, num_online_cpus());
	v_budget += NON_Q_VECTORS;

	adapter->msix_entries = kcalloc(v_budget,
					sizeof(struct msix_entry), GFP_KERNEL);
	if (!adapter->msix_entries) {
		return -ENOMEM;
	}

	for (vector = 0; vector < v_budget; vector++)
		adapter->msix_entries[vector].entry = vector;

	/* A failure in MSI-X entry allocation isn't fatal, but the VF driver
	 * does not support any other modes, so we will simply fail here. Note
	 * that we clean up the msix_entries pointer else-where.
	 */
	return ngbe_acquire_msix_vectors(adapter, v_budget);
}

void ngbe_add_ring(struct ngbe_ring *ring,
			     struct ngbe_ring_container *head)
{
	ring->next = head->ring;
	head->ring = ring;
	head->count++;
}

/**
 * ngbe_alloc_q_vector - Allocate memory for a single interrupt vector
 * @adapter: board private structure to initialize
 * @v_idx: index of vector in adapter struct
 *
 * We allocate one q_vector.  If allocation fails we return -ENOMEM.
 **/
int ngbe_alloc_q_vector(struct ngbevf_adapter *adapter, int v_idx,
				  int txr_count, int txr_idx,
				  int xdp_count, int xdp_idx,
				  int rxr_count, int rxr_idx)
{
	struct ngbe_q_vector *q_vector;
	int reg_idx = txr_idx + xdp_idx;
	struct ngbe_ring *ring;
	int ring_count, size;

	ring_count = txr_count + xdp_count + rxr_count;
	size = sizeof(*q_vector) + (sizeof(*ring) * ring_count);

	/* allocate q_vector and rings */
	q_vector = kzalloc(size, GFP_KERNEL);
	if (!q_vector)
		return -ENOMEM;

	/* initialize NAPI */
	netif_napi_add(adapter->netdev, &q_vector->napi, ngbe_poll, 64);

	/* tie q_vector and adapter together */
	adapter->q_vector[v_idx] = q_vector;
	q_vector->adapter = adapter;
	q_vector->v_idx = v_idx;

	/* initialize pointer to rings */
	ring = q_vector->ring;

	while (txr_count) {
		/* assign generic ring traits */
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;

		/* configure backlink on ring */
		ring->q_vector = q_vector;

		/* update q_vector Tx values */
		ngbe_add_ring(ring, &q_vector->tx);

		/* apply Tx specific ring traits */
		ring->count = adapter->tx_ring_count;
		ring->que_idx = txr_idx;
		ring->reg_idx = reg_idx;

		/* assign ring to adapter */
		 adapter->tx_ring[txr_idx] = ring;

		/* update count and index */
		txr_count--;
		txr_idx++;
		reg_idx++;

		/* push pointer to next ring */
		ring++;
	}

	while (xdp_count) {
		/* assign generic ring traits */
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;

		/* configure backlink on ring */
		ring->q_vector = q_vector;

		/* update q_vector Tx values */
		ngbe_add_ring(ring, &q_vector->tx);

		/* apply Tx specific ring traits */
		ring->count = adapter->tx_ring_count;
		ring->que_idx = xdp_idx;
		ring->reg_idx = reg_idx;
		set_ring_xdp(ring);

		/* assign ring to adapter */
		adapter->xdp_ring[xdp_idx] = ring;

		/* update count and index */
		xdp_count--;
		xdp_idx++;
		reg_idx++;

		/* push pointer to next ring */
		ring++;
	}

	while (rxr_count) {
		/* assign generic ring traits */
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;

		/* configure backlink on ring */
		ring->q_vector = q_vector;

		/* update q_vector Rx values */
		ngbe_add_ring(ring, &q_vector->rx);

		/* errata: UDP frames with a 0 checksum
		 * can be marked as checksum errors.
		 */
		if (adapter->hw.mac.type == ngbe_mac_em_vf)
			set_bit(__NGBE_RX_CSUM_UDP_ZERO_ERR, &ring->state);

		/* apply Rx specific ring traits */
		ring->count = adapter->rx_ring_count;
		ring->que_idx = rxr_idx;
		ring->reg_idx = rxr_idx;

		/* assign ring to adapter */
		adapter->rx_ring[rxr_idx] = ring;

		/* update count and index */
		rxr_count--;
		rxr_idx++;

		/* push pointer to next ring */
		ring++;
	}
#ifndef HAVE_NETIF_NAPI_ADD_CALLS_NAPI_HASH_ADD
#ifdef HAVE_NDO_BUSY_POLL
	napi_hash_add(&q_vector->napi);
#endif
#endif

	return 0;
}

/**
 * ngbe_free_q_vector - Free memory allocated for specific interrupt vector
 * @adapter: board private structure to initialize
 *
 * This function frees the memory allocated to the q_vector.  In addition if
 * NAPI is enabled it will delete any references to the NAPI struct prior
 * to freeing the q_vector.
 **/
void ngbe_free_q_vector(struct ngbevf_adapter *adapter, int v_idx)
{
	struct ngbe_q_vector *q_vector = adapter->q_vector[v_idx];
	struct ngbe_ring *ring;

	ngbe_for_each_ring(ring, q_vector->tx)
		if (ring_is_xdp(ring))
			adapter->xdp_ring[ring->que_idx] = NULL;
		else
		adapter->tx_ring[ring->que_idx] = NULL;

	ngbe_for_each_ring(ring, q_vector->rx)
		adapter->rx_ring[ring->que_idx] = NULL;

	adapter->q_vector[v_idx] = NULL;
#ifdef HAVE_NDO_BUSY_POLL
	napi_hash_del(&q_vector->napi);
#endif
	netif_napi_del(&q_vector->napi);

	/* ngbe_get_stats64() might access the rings on this vector,
	 * we must wait a grace period before freeing it.
	*/
	kfree_rcu(q_vector, rcu);
}

/**
 * ngbe_alloc_q_vectors - Allocate memory for interrupt vectors
 * @adapter: board private structure to initialize
 *
 * We allocate one q_vector per queue interrupt.  If allocation fails we
 * return -ENOMEM.
 **/
int ngbe_alloc_q_vectors(struct ngbevf_adapter *adapter)
{
	int q_vectors = adapter->num_q_vectors;
	int rxr_remaining = adapter->num_rx_queues;
	int txr_remaining = adapter->num_tx_queues;
	int xdp_remaining = adapter->num_xdp_queues;
	int rxr_idx = 0, txr_idx = 0, xdp_idx = 0, v_idx = 0;
	int err;

	if (q_vectors >= (rxr_remaining + txr_remaining + xdp_remaining)) {
		for (; rxr_remaining; v_idx++, q_vectors--) {
			int rqpv = DIV_ROUND_UP(rxr_remaining, q_vectors);
			err = ngbe_alloc_q_vector(adapter, v_idx,
						     0, 0, 0, 0, rqpv, rxr_idx);
			if (err)
				goto err_out;

			/* update counts and index */
			rxr_remaining -= rqpv;
			rxr_idx += rqpv;
		}
	}

	for (; q_vectors; v_idx++, q_vectors--) {
		int rqpv = DIV_ROUND_UP(rxr_remaining, q_vectors);
		int tqpv = DIV_ROUND_UP(txr_remaining, q_vectors);
		int xqpv = DIV_ROUND_UP(xdp_remaining, q_vectors);

		err = ngbe_alloc_q_vector(adapter, v_idx,
					     tqpv, txr_idx,
					     xqpv, xdp_idx,
					     rqpv, rxr_idx);

		if (err)
			goto err_out;

		/* update counts and index */
		rxr_remaining -= rqpv;
		rxr_idx += rqpv;
		txr_remaining -= tqpv;
		txr_idx += tqpv;
		xdp_remaining -= xqpv;
		xdp_idx += xqpv;
	}

	return 0;

err_out:
	adapter->num_tx_queues = 0;
	adapter->num_xdp_queues = 0;
	adapter->num_rx_queues = 0;
	adapter->num_q_vectors = 0;

	while (v_idx--)
		ngbe_free_q_vector(adapter, v_idx);

	return -ENOMEM;
}

/**
 * ngbe_free_q_vectors - Free memory allocated for interrupt vectors
 * @adapter: board private structure to initialize
 *
 * This function frees the memory allocated to the q_vectors.  In addition if
 * NAPI is enabled it will delete any references to the NAPI struct prior
 * to freeing the q_vector.
 **/
void ngbe_free_q_vectors(struct ngbevf_adapter *adapter)
{
	int v_idx = adapter->num_q_vectors;

	adapter->num_tx_queues = 0;
	adapter->num_rx_queues = 0;
	adapter->num_q_vectors = 0;

	while (v_idx--)
		ngbe_free_q_vector(adapter, v_idx);
}

/**
 * ngbe_reset_interrupt_capability - Reset MSIX setup
 * @adapter: board private structure
 *
 **/
void ngbe_reset_interrupt_capability(struct ngbevf_adapter *adapter)
{
	pci_disable_msix(adapter->pdev);
	kfree(adapter->msix_entries);
	adapter->msix_entries = NULL;
}

/**
 * ngbe_init_interrupt_scheme - Determine if MSIX is supported and init
 * @adapter: board private structure to initialize
 *
 **/
int ngbe_init_interrupt_scheme(struct ngbevf_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pdev;
	int err;

	/* Number of supported queues */
	ngbe_set_num_queues(adapter);

	err = ngbe_set_interrupt_capability(adapter);
	if (err) {
		dev_err(&pdev->dev, "Unable to setup interrupt capabilities\n");
		goto err_set_interrupt;
	}

	err = ngbe_alloc_q_vectors(adapter);
	if (err) {
		dev_err(&pdev->dev, "Unable to allocate memory for queue vectors\n");
		goto err_alloc_q_vectors;
	}

#ifdef HAVE_XDP_SUPPORT
	dev_info(&pdev->dev, "Multiqueue %s: Rx Queue count = %u, Tx Queue count = %u XDP Queue count %u\n",
		 (adapter->num_rx_queues > 1) ? "Enabled" : "Disabled",
		 adapter->num_rx_queues, adapter->num_tx_queues,
		 adapter->num_xdp_queues);
#else
	dev_info(&pdev->dev, "Multiqueue %s: Rx Queue count = %u, Tx Queue count = %u\n",
		 (adapter->num_rx_queues > 1) ? "Enabled" : "Disabled",
		 adapter->num_rx_queues, adapter->num_tx_queues);
#endif /* HAVE_XDP_SUPPORT */

	set_bit(__NGBE_DOWN, &adapter->state);

	return 0;
err_alloc_q_vectors:
	ngbe_reset_interrupt_capability(adapter);
err_set_interrupt:
	return err;
}

/**
 * ngbe_clear_interrupt_scheme - Clear the current interrupt scheme settings
 * @adapter: board private structure to clear interrupt scheme on
 *
 * We go through and clear interrupt specific resources and reset the structure
 * to pre-load conditions
 **/
void ngbe_clear_interrupt_scheme(struct ngbevf_adapter *adapter)
{
	adapter->num_tx_queues = 0;
	adapter->num_xdp_queues = 0;
	adapter->num_rx_queues = 0;

	ngbe_free_q_vectors(adapter);
	ngbe_reset_interrupt_capability(adapter);
}

/**
 * ngbe_sw_init - Initialize general software structures
 * (struct ngbe_adapter)
 * @adapter: board private structure to initialize
 *
 * ngbe_sw_init initializes the Adapter private data structure.
 * Fields are initialized based on PCI device information and
 * OS network device settings (MTU size).
 **/
int __devinit ngbe_sw_init(struct ngbevf_adapter *adapter)
{
	struct ngbevf_hw *hw = &adapter->hw;
	struct pci_dev *pdev = adapter->pdev;
	struct net_device *netdev = adapter->netdev;
	int err;

	/* PCI config space info */

	hw->vendor_id = pdev->vendor;
	hw->device_id = pdev->device;
	pci_read_config_byte(pdev, PCI_REVISION_ID, &hw->revision_id);
	hw->subsystem_vendor_id = pdev->subsystem_vendor;
	hw->subsystem_device_id = pdev->subsystem_device;

	ngbe_init_ops_vf(hw);
	TCALL(hw, mbx.ops.init_params);

	/* assume legacy case in which PF would only give VF 1 queues */
	/*in gigabit mode pf only give vf 1 queues*/
	hw->mac.max_tx_queues = 1;
	hw->mac.max_rx_queues = 1;

	/* lock to protect mailbox accesses */
	spin_lock_init(&adapter->mbx_lock);

	err = TCALL(hw, mac.ops.reset_hw);
	if (err) {
		dev_info(pci_dev_to_dev(pdev),
			 "PF still in reset state.  Is the PF interface up?\n");
	} else {
		err = TCALL(hw, mac.ops.init_hw);
		if (err) {
			dev_err(pci_dev_to_dev(pdev),
				"init_shared_code failed: %d\n", err);
			return err;
		}
		ngbe_negotiate_api(adapter);
		err = TCALL(hw, mac.ops.get_mac_addr, hw->mac.addr);
		if (err)
			dev_info(&pdev->dev, "Error reading MAC address\n");
		else if (is_zero_ether_addr(adapter->hw.mac.addr))
			dev_info(&pdev->dev,
				 "MAC address not assigned by administrator.\n");
		ether_addr_copy(netdev->dev_addr, hw->mac.addr);
	}

	if (!is_valid_ether_addr(netdev->dev_addr)) {
		dev_info(&pdev->dev, "Assigning random MAC address\n");
		eth_hw_addr_random(netdev);
		ether_addr_copy(hw->mac.addr, netdev->dev_addr);
		ether_addr_copy(hw->mac.perm_addr, netdev->dev_addr);
	}

	/* Enable dynamic interrupt throttling rates */
	adapter->rx_itr_setting = 1;
	adapter->tx_itr_setting = 1;

	/* set default ring sizes */
	adapter->tx_ring_count = NGBE_DEFAULT_TXD;
	adapter->rx_ring_count = NGBE_DEFAULT_RXD;

	/* enable rx csum by default */
	adapter->flagsd |= NGBE_F_CAP_RX_CSUM;

	set_bit(__NGBE_DOWN, &adapter->state);

	return 0;
}

#define UPDATE_VF_COUNTER_32bit(reg, last_counter, counter)	\
	{							\
		u32 current_counter = NGBE_READ_REG(hw, reg);	\
		if (current_counter < last_counter)		\
			counter += 0x100000000LL;		\
		last_counter = current_counter;			\
		counter &= 0xFFFFFFFF00000000LL;		\
		counter |= current_counter;			\
	}

#define UPDATE_VF_COUNTER_36bit(reg_lsb, reg_msb, last_counter, counter) \
	{								 \
		u64 current_counter_lsb = NGBE_READ_REG(hw, reg_lsb);	 \
		u64 current_counter_msb = NGBE_READ_REG(hw, reg_msb);	 \
		u64 current_counter = (current_counter_msb << 32) |      \
			current_counter_lsb;                             \
		if (current_counter < last_counter)			 \
			counter += 0x1000000000LL;			 \
		last_counter = current_counter;				 \
		counter &= 0xFFFFFFF000000000LL;			 \
		counter |= current_counter;				 \
	}
/**
 * ngbe_update_stats - Update the board statistics counters.
 * @adapter: board private structure
 **/
void ngbe_update_stats(struct ngbevf_adapter *adapter)
{
	struct ngbevf_hw *hw = &adapter->hw;
	struct ngbe_ring *ring;
	ngbe_net_stats_t *net_stats = &adapter->net_stats;
	u64 tx_restart_queue = 0, tx_busy = 0;
	u64 rx_csum_bad = 0;
	u32 page_failed = 0, buff_failed = 0;
	u32 i;

	if (test_bit(__NGBE_DOWN, &adapter->state) ||
	    test_bit(__NGBE_RESETTING, &adapter->state))
		return;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		ring = adapter->rx_ring[i];
		rx_csum_bad += ring->rx_stats.csum_err;
		page_failed += ring->rx_stats.alloc_rx_page_failed;
		buff_failed += ring->rx_stats.alloc_rx_buff_failed;
	}
	adapter->sw_stats.rx_csum_bad = rx_csum_bad;
	adapter->sw_stats.rx_alloc_page_failed = page_failed;
	adapter->sw_stats.rx_alloc_buff_failed = buff_failed;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		ring = adapter->tx_ring[i];
		tx_restart_queue += ring->tx_stats.tx_restart_queue;
		tx_busy += ring->tx_stats.tx_busy;
	}
	adapter->sw_stats.tx_restart_queue = tx_restart_queue;
	adapter->sw_stats.tx_busy = tx_busy;

	/* update hardware counters */
	ngbe_update32(hw, NGBE_VXGPRC,
			       &adapter->last_stats.gprc, &adapter->stats.gprc);
	ngbe_update32(hw, NGBE_VXGPTC,
			       &adapter->last_stats.gptc, &adapter->stats.gptc);
	ngbe_update36(hw, NGBE_VXGORC_LSB, NGBE_VXGORC_MSB,
			       &adapter->last_stats.gorc, &adapter->stats.gorc);
	ngbe_update36(hw, NGBE_VXGOTC_LSB, NGBE_VXGOTC_MSB,
			       &adapter->last_stats.gotc, &adapter->stats.gotc);
	ngbe_update32(hw, NGBE_VXMPRC,
			       &adapter->last_stats.mprc, &adapter->stats.mprc);
	ngbe_update32(hw, NGBE_VXBPRC,
			       &adapter->last_stats.bprc, &adapter->stats.bprc);
	ngbe_update32(hw, NGBE_VXMPTC,
			       &adapter->last_stats.mptc, &adapter->stats.mptc);
	ngbe_update32(hw, NGBE_VXBPTC,
			       &adapter->last_stats.bptc, &adapter->stats.bptc);

	/* update global counters */
	net_stats->multicast = adapter->stats.mprc - adapter->base_stats.mprc;
	net_stats->rx_crc_errors = rx_csum_bad;
}

/**
 * ngbe_service_timer - Timer Call-back
 * @data: pointer to adapter cast into an unsigned long
 **/
void ngbe_service_timer(struct timer_list *t)
{
	struct ngbevf_adapter *adapter = from_timer(adapter, t, service_timer);

	/* Reset the timer */
	mod_timer(&adapter->service_timer, (HZ * 2) + jiffies);

	ngbe_service_event_schedule(adapter);
}

void ngbe_reset_subtask(struct ngbevf_adapter *adapter)
{
	if (!(adapter->flagsd & NGBE_F_REQ_RESET))
		return;

	adapter->flagsd &= ~NGBE_F_REQ_RESET;

	/* If we're already down or resetting, just bail */
	if (test_bit(__NGBE_DOWN, &adapter->state) ||
	    test_bit(__NGBE_RESETTING, &adapter->state))
		return;

	adapter->sw_stats.tx_timeout_count++;

	ngbevf_reinit_locked(adapter);
}

/* ngbe_check_hang_subtask - check for hung queues and dropped interrupts
 * @adapter - pointer to the device adapter structure
 *
 * This function serves two purposes.  First it strobes the interrupt lines
 * in order to make certain interrupts are occurring.  Secondly it sets the
 * bits needed to check for TX hangs.  As a result we should immediately
 * determine if a hang has occurred.
 */
void ngbe_check_hang_subtask(struct ngbevf_adapter *adapter)
{
	int i;

	/* If we're down or resetting, just bail */
	if (test_bit(__NGBE_DOWN, &adapter->state) ||
	    test_bit(__NGBE_RESETTING, &adapter->state))
		return;

	/* Force detection of hung controller */
	if (netif_carrier_ok(adapter->netdev)) {
		for (i = 0; i < adapter->num_tx_queues; i++)
			set_check_for_tx_hang(adapter->tx_ring[i]);
		for (i = 0; i < adapter->num_xdp_queues; i++)
			set_check_for_tx_hang(adapter->xdp_ring[i]);
	}
}

/**
 * ngbe_watchdog_update_link - update the link status
 * @adapter - pointer to the device adapter structure
 **/
void ngbe_watchdog_update_link(struct ngbevf_adapter *adapter)
{
	struct ngbevf_hw *hw = &adapter->hw;
	u32 link_speed = adapter->link_speed;
	bool link_up = adapter->link_up;
	s32 err;

	spin_lock_bh(&adapter->mbx_lock);

	err = TCALL(hw, mac.ops.check_link, &link_speed, &link_up, false);

	spin_unlock_bh(&adapter->mbx_lock);

	/* if check for link returns error we will need to reset */
	if (err && time_after(jiffies, adapter->last_reset + (10 * HZ))) {
		adapter->flagsd |= NGBE_F_REQ_RESET;
		link_up = false;
	}

	adapter->link_up = link_up;
	adapter->link_speed = link_speed;
}

/**
 * ngbe_watchdog_link_is_up - update netif_carrier status and
 *                               print link up message
 * @adapter - pointer to the device adapter structure
 **/
void ngbe_watchdog_link_is_up(struct ngbevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	/* only continue if link was previously down */
	if (netif_carrier_ok(netdev))
		return;

	dev_info(&adapter->pdev->dev, "NIC Link is Up %s\n",
		 (adapter->link_speed == NGBE_LINK_SPEED_1GB_FULL) ?
		 "1 Gbps" :
		 (adapter->link_speed == NGBE_LINK_SPEED_100_FULL) ?
		 "100 Mbps" :
		 (adapter->link_speed == NGBE_LINK_SPEED_10_FULL) ?
		 "10 Mbps" :
		 "unknown speed");

	netif_carrier_on(netdev);
}

/**
 * ngbe_watchdog_link_is_down - update netif_carrier status and
 *                                 print link down message
 * @adapter - pointer to the adapter structure
**/
void ngbe_watchdog_link_is_down(struct ngbevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	adapter->link_speed = 0;

	/* only continue if link was up previously */
	if (!netif_carrier_ok(netdev))
		return;

	dev_info(&adapter->pdev->dev, "NIC Link is Down\n");

	netif_carrier_off(netdev);
}

/*
 * ngbe_watchdog_subtask - worker thread to bring link up
 * @work: pointer to work_struct containing our data
 */
void ngbe_watchdog_subtask(struct ngbevf_adapter *adapter)
{

	/* if interface is down do nothing */
	if (test_bit(__NGBE_DOWN, &adapter->state) ||
	    test_bit(__NGBE_RESETTING, &adapter->state))
		return;

	ngbe_watchdog_update_link(adapter);
	if (adapter->link_up)
		ngbe_watchdog_link_is_up(adapter);
	else
		ngbe_watchdog_link_is_down(adapter);

	ngbe_update_stats(adapter);
}

/**
 * ngbe_service_task - manages and runs subtasks
 * @work: pointer to work_struct containing our data
 **/
void ngbe_service_task(struct work_struct *work)
{
	struct ngbevf_adapter *adapter = container_of(work,
						       struct ngbevf_adapter,
						       service_task);
	struct ngbevf_hw *hw = &adapter->hw;

	if (NGBE_REMOVED(hw->hw_addr)) {
		if (!test_bit(__NGBE_DOWN, &adapter->state)) {
			rtnl_lock();
			ngbe_down(adapter);
			rtnl_unlock();
		}
		return;
	}

	ngbe_queue_reset_subtask(adapter);
	ngbe_reset_subtask(adapter);
	ngbe_watchdog_subtask(adapter);
	ngbe_check_hang_subtask(adapter);

	ngbe_service_event_complete(adapter);
}

/**
 * ngbe_free_tx_resources - Free Tx Resources per Queue
 * @tx_ring: Tx descriptor ring for a specific queue
 *
 * Free all transmit software resources
 **/
void ngbe_free_tx_resources(struct ngbe_ring *tx_ring)
{
	ngbe_clean_tx_ring(tx_ring);

	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;

	/* if not set, then don't free */
	if (!tx_ring->desc)
		return;

	dma_free_coherent(tx_ring->dev, tx_ring->size,
			  tx_ring->desc, tx_ring->dma_addr);

	tx_ring->desc = NULL;
}

/**
 * ngbe_free_all_tx_resources - Free Tx Resources for All Queues
 * @adapter: board private structure
 *
 * Free all transmit software resources
 **/
void ngbe_free_all_tx_resources(struct ngbevf_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		if (adapter->tx_ring[i]->desc)
			ngbe_free_tx_resources(adapter->tx_ring[i]);
	for (i = 0; i < adapter->num_xdp_queues; i++)
		if (adapter->xdp_ring[i]->desc)
			ngbe_free_tx_resources(adapter->xdp_ring[i]);
}

/**
 * ngbe_setup_tx_resources - allocate Tx resources (Descriptors)
 * @tx_ring:    tx descriptor ring (for a specific queue) to setup
 *
 * Return 0 on success, negative on failure
 **/
int ngbe_setup_tx_resources(struct ngbe_ring *tx_ring)
{
	int size;

	size = sizeof(struct ngbe_tx_buffer) * tx_ring->count;
	tx_ring->tx_buffer_info = vmalloc(size);
	if (!tx_ring->tx_buffer_info)
		goto err;

	u64_stats_init(&tx_ring->syncp);

	/* round up to nearest 4K */
	tx_ring->size = tx_ring->count * sizeof(struct ngbe_tx_desc);
	tx_ring->size = ALIGN(tx_ring->size, 4096);

	tx_ring->desc = dma_alloc_coherent(tx_ring->dev, tx_ring->size,
					   &tx_ring->dma_addr, GFP_KERNEL);
	if (!tx_ring->desc)
		goto err;

	return 0;

err:
	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;
	dev_err(tx_ring->dev, "Unable to allocate memory for the Tx descriptor ring\n");
	return -ENOMEM;
}

/**
 * ngbe_setup_all_tx_resources - allocate all queues Tx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
int ngbe_setup_all_tx_resources(struct ngbevf_adapter *adapter)
{
	int i, j = 0, err = 0;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		err = ngbe_setup_tx_resources(adapter->tx_ring[i]);
		if (!err)
			continue;
		DPRINTK(PROBE, ERR, "Allocation for Tx Queue %u failed\n", i);
		goto err_setup_tx;
	}

	for (j = 0; j < adapter->num_xdp_queues; j++) {
		err = ngbe_setup_tx_resources(adapter->xdp_ring[j]);
		if (!err)
			continue;
		hw_dbg(&adapter->hw, "Allocation for XDP Queue %u failed\n", j);
		goto err_setup_tx;
	}

	return 0;
err_setup_tx:
	/* rewind the index freeing the rings as we go */
	while (j--)
		ngbe_free_tx_resources(adapter->xdp_ring[j]);
	while (i--)
		ngbe_free_tx_resources(adapter->tx_ring[i]);
	return err;
}

/**
 * ngbe_setup_rx_resources - allocate Rx resources (Descriptors)
 * @rx_ring:    rx descriptor ring (for a specific queue) to setup
 *
 * Returns 0 on success, negative on failure
 **/
int ngbe_setup_rx_resources(struct ngbevf_adapter *adapter,
					struct ngbe_ring *rx_ring)
{
	int size;

	size = sizeof(struct ngbe_rx_buffer) * rx_ring->count;
	rx_ring->rx_buffer_info = vmalloc(size);
	if (!rx_ring->rx_buffer_info) {
		goto err;
	}

	/* Round up to nearest 4K */
	rx_ring->size = rx_ring->count * sizeof(union ngbe_rx_desc);
	rx_ring->size = ALIGN(rx_ring->size, 4096);

	rx_ring->desc = dma_alloc_coherent(rx_ring->dev, rx_ring->size,
					   &rx_ring->dma_addr, GFP_KERNEL);

	if (!rx_ring->desc)
		goto err;

#ifdef HAVE_XDP_BUFF_RXQ
	/* XDP RX-queue info */
	if (xdp_rxq_info_reg(&rx_ring->xdp_rxq, adapter->netdev,
			     rx_ring->que_idx, 0) < 0)
		goto err;

#endif /* HAVE_XDP_BUFF_RXQ */
	rx_ring->xdp_prog = adapter->xdp_prog;

	return 0;
err:
	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;
	dev_err(rx_ring->dev, "Unable to allocate memory for the Rx descriptor ring\n");
	return -ENOMEM;
}

/**
 * ngbe_setup_all_rx_resources - allocate all queues Rx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
int ngbe_setup_all_rx_resources(struct ngbevf_adapter *adapter)
{
	int i, err = 0;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		err = ngbe_setup_rx_resources(adapter, adapter->rx_ring[i]);
		if (!err)
			continue;
		DPRINTK(PROBE, ERR, "Allocation for Rx Queue %u failed\n", i);
		goto err_setup_rx;
	}

	return 0;
err_setup_rx:
	/* rewind the index freeing the rings as we go */
	while (i--)
		ngbe_free_rx_resources(adapter->rx_ring[i]);
	return err;
}

/**
 * ngbe_free_rx_resources - Free Rx Resources
 * @rx_ring: ring to clean the resources from
 *
 * Free all receive software resources
 **/
void ngbe_free_rx_resources(struct ngbe_ring *rx_ring)
{
	ngbe_clean_rx_ring(rx_ring);

	rx_ring->xdp_prog = NULL;
#ifdef HAVE_XDP_BUFF_RXQ
	xdp_rxq_info_unreg(&rx_ring->xdp_rxq);
#endif /* HAVE_XDP_BUFF_RXQ */
	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;

	dma_free_coherent(rx_ring->dev, rx_ring->size,
			  rx_ring->desc, rx_ring->dma_addr);

	rx_ring->desc = NULL;
}

/**
 * ngbe_free_all_rx_resources - Free Rx Resources for All Queues
 * @adapter: board private structure
 *
 * Free all receive software resources
 **/
void ngbe_free_all_rx_resources(struct ngbevf_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		if (adapter->rx_ring[i]->desc)
			ngbe_free_rx_resources(adapter->rx_ring[i]);
}

/**
 * ngbe_open - Called when a network interface is made active
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
int ngbe_open(struct net_device *netdev)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbevf_hw *hw = &adapter->hw;
	int err;

	/* A previous failure to open the device because of a lack of
	 * available MSIX vector resources may have reset the number
	 * of q vectors variable to zero.  The only way to recover
	 * is to unload/reload the driver and hope that the system has
	 * been able to recover some MSIX vector resources.
	 */
	if (!adapter->num_q_vectors)
		return -ENOMEM;

	if (hw->adapter_stopped) {
		ngbe_reset(adapter);
		/* if adapter is still stopped then PF isn't up and
		 * the vf can't start. */
		if (hw->adapter_stopped) {
			err = NGBE_ERR_MBX;
			DPRINTK(DRV, ERR, "Unable to start - perhaps the PF"
				"Driver isn't up yet\n");
			goto err_setup_reset;
		}
	}

	/* disallow open during test */
	if (test_bit(__NGBE_TESTING, &adapter->state))
		return -EBUSY;

	netif_carrier_off(netdev);

	/* allocate transmit descriptors */
	err = ngbe_setup_all_tx_resources(adapter);
	if (err)
		goto err_setup_tx;

	/* allocate receive descriptors */
	err = ngbe_setup_all_rx_resources(adapter);
	if (err)
		goto err_setup_rx;
	ngbe_configure(adapter);
	err = ngbe_request_irq(adapter);
	if (err)
		goto err_req_irq;
	/* Notify the stack of the actual queue counts. */
	err = netif_set_real_num_tx_queues(netdev, adapter->num_tx_queues);
	if (err)
		goto err_set_queues;
	err = netif_set_real_num_rx_queues(netdev, adapter->num_rx_queues);
	if (err)
		goto err_set_queues;

	ngbe_up_complete(adapter);

	return 0;

err_set_queues:
	ngbe_free_irq(adapter);
err_req_irq:
	ngbe_free_all_rx_resources(adapter);
err_setup_rx:
	ngbe_free_all_tx_resources(adapter);
err_setup_tx:
	ngbe_reset(adapter);
err_setup_reset:

	return err;
}

/**
 * ixgbevf_close_suspend - actions necessary to both suspend and close flows
 * @adapter: the private adapter struct
 *
 * This function should contain the necessary work common to both suspending
 * and closing of the device.
 */
static void ngbe_close_suspend(struct ngbevf_adapter *adapter)
{
		ngbe_down(adapter);
		ngbe_free_irq(adapter);
		ngbe_free_all_tx_resources(adapter);
		ngbe_free_all_rx_resources(adapter);
}

/*
 * ngbe_close - Disables a network interface
 * @netdev: network interface device structure
 *
 * Returns 0, this is not allowed to fail
 *
 * The close entry point is called when an interface is de-activated
 * by the OS.  The hardware is still under the drivers control, but
 * needs to be disabled.  A global MAC reset is issued to stop the
 * hardware, and all transmit and receive resources are freed.
 **/
int ngbe_close(struct net_device *netdev)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);

	ngbe_down(adapter);
	ngbe_free_irq(adapter);

	ngbe_free_all_tx_resources(adapter);
	ngbe_free_all_rx_resources(adapter);

	return 0;
}

void ngbe_queue_reset_subtask(struct ngbevf_adapter *adapter)
{
	struct net_device *dev = adapter->netdev;

	if (!(adapter->flagsd & NGBE_F_REQ_QUEUE_RESET))
		return;

	adapter->flagsd &= ~NGBE_F_REQ_QUEUE_RESET;

	/* if interface is down do nothing */
	if (test_bit(__NGBE_DOWN, &adapter->state) ||
	    test_bit(__NGBE_RESETTING, &adapter->state))
		return;

	/*
	 * Hardware has to reinitialize queues and interrupts to
	 * match packet buffer alignment. Unfortunately, the
	 * hardware is not flexible enough to do this dynamically.
	 */
	rtnl_lock();

	/* disable running interface */
	if (netif_running(dev))
		ngbe_close(dev);

	/* free and reallocate queues */
	ngbe_clear_interrupt_scheme(adapter);
	ngbe_init_interrupt_scheme(adapter);

	/* reenable running interface */
	if (netif_running(dev))
		ngbe_open(dev);

	rtnl_unlock();
}

void ngbe_tx_ctxtdesc(struct ngbe_ring *tx_ring,
				u32 vlan_macip_lens, u32 type_tucmd,
				u32 mss_l4len_idx)
{
	struct ngbe_adv_tx_context_desc *context_desc;
	u16 i = tx_ring->next_to_use;

	context_desc = NGBE_TX_CTXTDESC(tx_ring, i);

	i++;
	tx_ring->next_to_use = (i < tx_ring->count) ? i : 0;

	/* set bits to identify this as an advanced context descriptor */
	type_tucmd |= NGBE_TXD_DTYP_CTXT;

	context_desc->vlan_macip_lens   = cpu_to_le32(vlan_macip_lens);
	context_desc->seqnum_seed       = 0;
	context_desc->type_tucmd_mlhl   = cpu_to_le32(type_tucmd);
	context_desc->mss_l4len_idx     = cpu_to_le32(mss_l4len_idx);

	WJPRINTK("context_desc: %08x %08x %08x %08x\n",
		context_desc->vlan_macip_lens,
		context_desc->seqnum_seed,
		context_desc->type_tucmd_mlhl,
		context_desc->mss_l4len_idx);
}

int ngbe_tso(struct ngbe_ring *tx_ring,
	      struct ngbe_tx_buffer *first, u8 *hdr_len, ngbe_dptype dptype)
{
	struct sk_buff *skb = first->skb;
#ifdef NETIF_F_TSO
	u32 vlan_macip_lens, type_tucmd;
	u32 mss_l4len_idx, l4len;

	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return 0;

	if (!skb_is_gso(skb))
		return 0;

	if (skb_header_cloned(skb)) {
		int err = pskb_expand_head(skb, 0, 0, GFP_ATOMIC);
		if (err)
			return err;
	}

	type_tucmd = NGBE_TXD_PKTTYPE(dptype.ptype);

	if (first->protocol == htons(ETH_P_IP)) {
		struct iphdr *iph = ip_hdr(skb);
		iph->tot_len = 0;
		iph->check = 0;
		tcp_hdr(skb)->check = ~csum_tcpudp_magic(
			iph->saddr, iph->daddr, 0, IPPROTO_TCP, 0);
		first->tx_flags |= NGBE_TX_FLAGS_TSO |
				   NGBE_TX_FLAGS_CSUM |
				   NGBE_TX_FLAGS_IPV4;
#ifdef NETIF_F_TSO6
	} else if (skb_is_gso_v6(skb)) {
		ipv6_hdr(skb)->payload_len = 0;
		tcp_hdr(skb)->check = ~csum_ipv6_magic(
			&ipv6_hdr(skb)->saddr, &ipv6_hdr(skb)->daddr,
			0, IPPROTO_TCP, 0);
		first->tx_flags |= NGBE_TX_FLAGS_TSO |
				   NGBE_TX_FLAGS_CSUM;
#endif
	}

	/* compute header lengths */
	l4len = tcp_hdrlen(skb);
	*hdr_len += l4len;
	*hdr_len = skb_transport_offset(skb) + l4len;

	/* update gso size and bytecount with header size */
	first->gso_segs = skb_shinfo(skb)->gso_segs;
	first->bytecount += (first->gso_segs - 1) * *hdr_len;

	/* mss_l4len_id: use 1 as index for TSO */
	mss_l4len_idx = NGBE_TXD_TPLEN(l4len);
	mss_l4len_idx |= NGBE_TXD_MSS(skb_shinfo(skb)->gso_size);
	mss_l4len_idx |= NGBE_TXD_BAK_DESC;

	/* vlan_macip_lens: HEADLEN, MACLEN, VLAN tag */
	vlan_macip_lens = NGBE_TXD_HDRLEN(skb_network_header_len(skb));
	vlan_macip_lens |= NGBE_TXD_MACLEN(skb_network_offset(skb));
	vlan_macip_lens |= first->tx_flags & NGBE_TX_FLAGS_VLAN_MASK;

	ngbe_tx_ctxtdesc(tx_ring, vlan_macip_lens,
			    type_tucmd, mss_l4len_idx);

	return 1;
#else
	return 0;
#endif /* NETIF_F_TSO */
}

static inline bool ngbe_ipv6_csum_is_sctp(struct sk_buff *skb)
{
	unsigned int offset = 0;

	ipv6_find_hdr(skb, &offset, IPPROTO_SCTP, NULL, NULL);

	return offset == skb_checksum_start_offset(skb);
}

void ngbe_tx_csum(struct ngbe_ring *tx_ring,
		   struct ngbe_tx_buffer *first, ngbe_dptype dptype)
{
	struct sk_buff *skb = first->skb;
	u32 vlan_macip_lens = 0;
	u32 mss_l4len_idx = 0;
	u32 type_tucmd;

	type_tucmd = NGBE_TXD_PKTTYPE(dptype.ptype);

	if (skb->ip_summed != CHECKSUM_PARTIAL) {
		if (!(first->tx_flags & NGBE_TX_FLAGS_VLAN))
			return;
		vlan_macip_lens |= NGBE_TXD_MACLEN(skb_network_offset(skb));
	} else {
		u8 l4_hdr = 0;
#ifdef HAVE_ENCAP_TSO_OFFLOAD
		__be16 frag_off;
#endif

		switch (first->protocol) {
		case __constant_htons(ETH_P_IP):
			vlan_macip_lens |= NGBE_TXD_HDRLEN(skb_network_header_len(skb));
			l4_hdr = ip_hdr(skb)->protocol;
			break;
#ifdef NETIF_F_IPV6_CSUM
		case __constant_htons(ETH_P_IPV6):
			vlan_macip_lens |= NGBE_TXD_HDRLEN(skb_network_header_len(skb));
			l4_hdr = ipv6_hdr(skb)->nexthdr;
#ifdef HAVE_ENCAP_TSO_OFFLOAD
			if (likely(skb_network_header_len(skb) ==
				   sizeof(struct ipv6hdr)))
				break;
			ipv6_skip_exthdr(skb, skb_network_offset(skb) +
					      sizeof(struct ipv6hdr),
					 &l4_hdr, &frag_off);
			if (unlikely(frag_off))
				l4_hdr = NEXTHDR_FRAGMENT;
#endif /* HAVE_ENCAP_TSO_OFFLOAD */
			break;
#endif /* NETIF_F_IPV6_CSUM */
		default:
			if (unlikely(net_ratelimit())) {
				dev_warn(tx_ring->dev,
				 "partial checksum but proto=%x!\n",
				 first->protocol);
			}
			break;
		}

		switch (l4_hdr) {
		case IPPROTO_TCP:
			mss_l4len_idx = NGBE_TXD_TPLEN(tcp_hdrlen(skb));
			break;
		case IPPROTO_SCTP:
			mss_l4len_idx = NGBE_TXD_TPLEN(sizeof(struct sctphdr));
			break;
		case IPPROTO_UDP:
			mss_l4len_idx = NGBE_TXD_TPLEN(sizeof(struct udphdr));
			break;
		default:
#ifdef HAVE_ENCAP_TSO_OFFLOAD
			if (unlikely(net_ratelimit())) {
				dev_warn(tx_ring->dev,
					 "partial checksum, l3 proto=%x, l4 proto=%x\n",
					 first->protocol, l4_hdr);
			}
			skb_checksum_help(skb);
			goto no_csum;
#else /* !HAVE_ENCAP_TSO_OFFLOAD */
			if (unlikely(net_ratelimit())) {
				dev_warn(tx_ring->dev,
				 "partial checksum but l4 proto=%x!\n",
				 l4_hdr);
			}
			break;
#endif /* HAVE_ENCAP_TSO_OFFLOAD */
		}

		/* update TX checksum flag */
		first->tx_flags |= NGBE_TX_FLAGS_CSUM;
	}

#ifdef HAVE_ENCAP_TSO_OFFLOAD
no_csum:
#endif
	/* vlan_macip_lens: MACLEN, VLAN tag */
	vlan_macip_lens |= NGBE_TXD_MACLEN(skb_network_offset(skb));
	vlan_macip_lens |= first->tx_flags & NGBE_TX_FLAGS_VLAN_MASK;

	ngbe_tx_ctxtdesc(tx_ring, vlan_macip_lens, type_tucmd, mss_l4len_idx);
}

__le32 ngbe_tx_cmd_type(u32 tx_flags)
{
	/* set type for advanced descriptor with frame checksum insertion */
	__le32 cmd_type = cpu_to_le32(NGBE_TXD_FCS);

	/* set HW vlan bit if vlan is present */
	if (tx_flags & NGBE_TX_FLAGS_VLAN)
		cmd_type |= cpu_to_le32(NGBE_TXD_VLE);

	/* set segmentation enable bits for TSO/FSO */
	if (tx_flags & NGBE_TX_FLAGS_TSO)
		cmd_type |= cpu_to_le32(NGBE_TXD_TSE);

	return cmd_type;
}

__le32 ngbe_tx_olinfo_status(struct ngbe_tx_desc *tx_desc,
				     u32 tx_flags, unsigned int paylen)
{
	__le32 status = NGBE_TXD_PAYLEN(paylen);

	/* enable L4 checksum for TSO and TX checksum offload */
	if (tx_flags & NGBE_TX_FLAGS_CSUM)
		status |= NGBE_TXD_TPCS;

	/* enble IPv4 checksum for TSO */
	if (tx_flags & NGBE_TX_FLAGS_IPV4)
		status |= NGBE_TXD_IPCS;

	/* use index 1 context for TSO/FSO/FCOE */
	if (tx_flags & NGBE_TX_FLAGS_TSO)
		status |= NGBE_TXD_BAK_DESC;

	/*
	 * Check Context must be set if Tx switch is enabled, which it
	 * always is for case where virtual functions are running
	 */
	status |= NGBE_TXD_CC;

	return status;
}

static int __ngbe_maybe_stop_tx(struct ngbe_ring *tx_ring, int size)
{
	netif_stop_subqueue(tx_ring->netdev, tx_ring->que_idx);
	/* Herbert's original patch had:
	*  smp_mb__after_netif_stop_queue();
	* but since that doesn't exist yet, just open code it.
	*/
	smp_mb();

	/* We need to check again in a case another CPU has just
	* made room available.
	*/
	if (likely(ngbe_desc_unused(tx_ring) < size))
		 return -EBUSY;

	/* A reprieve! - use start_queue because it doesn't call schedule */
	netif_start_subqueue(tx_ring->netdev, tx_ring->que_idx);
	++tx_ring->tx_stats.tx_restart_queue;
	
	return 0;
}

static inline int ngbe_maybe_stop_tx(struct ngbe_ring *tx_ring, int size)
{
	if (likely(ngbe_desc_unused(tx_ring) >= size))
		return 0;
	return __ngbe_maybe_stop_tx(tx_ring, size);
}

void ngbe_tx_map(struct ngbe_ring *tx_ring,
			   struct ngbe_tx_buffer *first,
			   const u8 hdr_len)
{
	dma_addr_t dma_addr;
	struct sk_buff *skb = first->skb;
	struct ngbe_tx_buffer *tx_buffer;
	struct ngbe_tx_desc *tx_desc;
	skb_frag_t *frag = &skb_shinfo(skb)->frags[0];
	unsigned int data_len = skb->data_len;
	unsigned int size = skb_headlen(skb);
	unsigned int paylen = skb->len - hdr_len;
	u32 tx_flags = first->tx_flags;
	__le32 cmd_type, status;
	u16 i = tx_ring->next_to_use;

	tx_desc = NGBE_TX_DESC(tx_ring, i);

	status = ngbe_tx_olinfo_status(tx_desc, tx_flags, paylen);
	cmd_type = ngbe_tx_cmd_type(tx_flags);

	dma_addr = dma_map_single(tx_ring->dev, skb->data, size, DMA_TO_DEVICE);
	if (dma_mapping_error(tx_ring->dev, dma_addr))
		goto dma_error;

	/* record length, and DMA address */
	dma_unmap_len_set(first, len, size);
	dma_unmap_addr_set(first, dma, dma_addr);

	tx_desc->pkt_addr = cpu_to_le64(dma_addr);
	tx_desc->status = status;
	for (;;) {
		if (size == 0)
			goto next_frag;

		while (unlikely(size > NGBE_MAX_DATA_PER_TXD)) {
			tx_desc->cmd_type_len =
				cmd_type | cpu_to_le32(NGBE_MAX_DATA_PER_TXD);

			i++;
			tx_desc++;
			if (i == tx_ring->count) {
				tx_desc = NGBE_TX_DESC(tx_ring, 0);
				i = 0;
			}

			dma_addr += NGBE_MAX_DATA_PER_TXD;
			size -= NGBE_MAX_DATA_PER_TXD;

			tx_desc->pkt_addr = cpu_to_le64(dma_addr);
			tx_desc->status = status;
		}

		if (likely(!data_len))
			break;

		tx_desc->cmd_type_len = cmd_type | cpu_to_le32(size);
		WJPRINTK("tx_desc: %016llx %08x %08x\n",
			tx_desc->pkt_addr,
			tx_desc->cmd_type_len,
			tx_desc->status);
		i++;
		tx_desc++;
		if (i == tx_ring->count) {
			tx_desc = NGBE_TX_DESC(tx_ring, 0);
			i = 0;
		}
next_frag:
		size = skb_frag_size(frag);
		data_len -= size;

		dma_addr = skb_frag_dma_map(tx_ring->dev, frag, 0, size,
				       DMA_TO_DEVICE);
		if (dma_mapping_error(tx_ring->dev, dma_addr))
			goto dma_error;

		tx_buffer = &tx_ring->tx_buffer_info[i];
		dma_unmap_len_set(tx_buffer, len, size);
		dma_unmap_addr_set(tx_buffer, dma, dma_addr);

		tx_desc->pkt_addr = cpu_to_le64(dma_addr);
		tx_desc->status = status;

		frag++;
	}

	netdev_tx_sent_queue(txring_txq(tx_ring), first->bytecount);
	/* write last descriptor with RS and EOP bits */
	cmd_type |= cpu_to_le32(size) | cpu_to_le32(NGBE_TXD_EOP | NGBE_TXD_RS);
	tx_desc->cmd_type_len = cmd_type;

	/* set the timestamp */
	first->time_stamp = jiffies;
#ifndef HAVE_TRANS_START_IN_QUEUE
		tx_ring->netdev->trans_start = first->time_stamp;
#endif
	WJPRINTK("tx_desc: %016llx %08x %08x\n",
			tx_desc->pkt_addr,
			tx_desc->cmd_type_len,
			tx_desc->status);
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

	ngbe_maybe_stop_tx(tx_ring, DESC_NEEDED);

	if (netif_xmit_stopped(txring_txq(tx_ring)) || !netdev_xmit_more()) {
		writel(i, tx_ring->tail);
#ifndef SPIN_UNLOCK_IMPLIES_MMIOWB
		/* we need this if more than one processor can write to our tail
		 * at a time, it synchronizes IO on IA64/Altix systems
		 */
		mmiowb();
#endif /* SPIN_UNLOCK_IMPLIES_MMIOWB */
	}

	return;
dma_error:
	dev_err(tx_ring->dev, "TX DMA map failed\n");

	/* clear dma mappings for failed tx_buffer_info map */
	for (;;) {
		tx_buffer = &tx_ring->tx_buffer_info[i];
		ngbe_unmap_and_free_tx_resource(tx_ring, tx_buffer);
		if (tx_buffer == first)
			break;
		if (i == 0)
			i = tx_ring->count;
		i--;
	}

	tx_ring->next_to_use = i;
}

#if 0
int __ngbe_maybe_stop_tx(struct ngbe_ring *tx_ring, int size)
{
	netif_stop_subqueue(tx_ring->netdev, tx_ring->que_idx);
	/* Herbert's original patch had:
	 *  smp_mb__after_netif_stop_queue();
	 * but since that doesn't exist yet, just open code it. */
	smp_mb();

	/* We need to check again in a case another CPU has just
	 * made room available. */
	if (likely(ngbe_desc_unused(tx_ring) < size))
		return -EBUSY;

	/* A reprieve! - use start_queue because it doesn't call schedule */
	netif_start_subqueue(tx_ring->netdev, tx_ring->que_idx);
	++tx_ring->tx_stats.tx_restart_queue;

	return 0;
}

inline int ngbe_maybe_stop_tx(struct ngbe_ring *tx_ring, int size)
{
	if (likely(ngbe_desc_unused(tx_ring) >= size))
		return 0;
	return __ngbe_maybe_stop_tx(tx_ring, size);
}
#endif

int ngbe_xmit_frame_ring(struct sk_buff *skb,
				   struct ngbe_ring *tx_ring)
{
	struct ngbe_tx_buffer *first;
	ngbe_dptype dptype;
	int tso;
	u32 tx_flags = 0;
	u16 count = TXD_USE_COUNT(skb_headlen(skb));
	unsigned short f;
	__be16 protocol = skb->protocol;
	u8 hdr_len = 0;

	/*
	 * if this is an LLDP ether frame then drop it - VFs do not
	 * forward LLDP frames.
	 */
	if (ntohs(skb->protocol) == ETH_P_LLDP) {
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}

	/*
	 * need: 1 descriptor per page * PAGE_SIZE/NGBE_MAX_DATA_PER_TXD,
	 *     + 1 desc for skb_headlen/NGBE_MAX_DATA_PER_TXD,
	 *     + 2 desc gap to keep tail from touching head,
	 *     + 1 desc for context descriptor,
	 * otherwise try next time
	 */
	for (f = 0; f < skb_shinfo(skb)->nr_frags; f++)
		count += TXD_USE_COUNT(skb_frag_size(&skb_shinfo(skb)->
					frags[f]));

	if (ngbe_maybe_stop_tx(tx_ring, count + 3)) {
		tx_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}

	/* software timestamp */
	skb_tx_timestamp(skb);

	/* record the location of the first descriptor for this packet */
	first = &tx_ring->tx_buffer_info[tx_ring->next_to_use];
	first->skb = skb;
	first->bytecount = skb->len;
	first->gso_segs = 1;

#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	if (skb_vlan_tag_present(skb)) {
		tx_flags |= skb_vlan_tag_get(skb) << NGBE_TX_FLAGS_VLAN_SHIFT;
		tx_flags |= NGBE_TX_FLAGS_VLAN;
	/* else if it is a SW VLAN check the next protocol and store the tag */
	} else if (protocol == __constant_htons(ETH_P_8021Q) ||
		   protocol == __constant_htons(ETH_P_8021AD)) {
		struct vlan_hdr *vhdr, _vhdr;
		vhdr = skb_header_pointer(skb, ETH_HLEN, sizeof(_vhdr), &_vhdr);
		if (!vhdr)
			goto out_drop;

		protocol = vhdr->h_vlan_encapsulated_proto;
		tx_flags |= ntohs(vhdr->h_vlan_TCI) << NGBE_TX_FLAGS_VLAN_SHIFT;
	}
#endif /* NETIF_F_HW_VLAN_TX || NETIF_F_HW_VLAN_CTAG_TX */
	//protocol = vlan_get_protocol(skb);

	/* record initial flags and protocol */
	first->tx_flags = tx_flags;
	first->protocol = protocol;

	/* encode packet type */
	dptype = ngbe_tx_encode_ptype(first);

	tso = ngbe_tso(tx_ring, first, &hdr_len, dptype);
	if (tso < 0)
		goto out_drop;
	else if (!tso)
		ngbe_tx_csum(tx_ring, first, dptype);

	ngbe_tx_map(tx_ring, first, hdr_len);

#ifdef HAVE_NETIF_TRANS_UPDATE
	netif_trans_update(tx_ring->netdev);
#else
	tx_ring->netdev->trans_start = jiffies;
#endif
	ngbe_maybe_stop_tx(tx_ring, DESC_NEEDED);

	return NETDEV_TX_OK;

out_drop:
	dev_kfree_skb_any(first->skb);
	first->skb = NULL;

	return NETDEV_TX_OK;
}

netdev_tx_t ngbe_xmit_frame(struct sk_buff *skb,
			     struct net_device *netdev)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbe_ring *tx_ring;

	if (skb->len <= 0) {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	/* The minimum packet size for olinfo paylen is 17 so pad the skb
	 * in order to meet this minimum size requirement.
	 */
	if (skb_put_padto(skb, 17))
		return NETDEV_TX_OK;

#ifdef HAVE_TX_MQ
	tx_ring = adapter->tx_ring[skb->queue_mapping];
#else
	tx_ring = adapter->tx_ring[0];
#endif
	return ngbe_xmit_frame_ring(skb, tx_ring);
}

/**
 * ngbe_set_mac - Change the Ethernet Address of the NIC
 * @netdev: network interface device structure
 * @p: pointer to an address structure
 *
 * Returns 0 on success, negative on failure
 **/
int ngbe_set_mac(struct net_device *netdev, void *p)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbevf_hw *hw = &adapter->hw;
	struct sockaddr *addr = p;
	int err;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	spin_lock_bh(&adapter->mbx_lock);

	err = TCALL(hw, mac.ops.set_rar, 0, addr->sa_data, 0, 1);

	spin_unlock_bh(&adapter->mbx_lock);

	if (err)
		return -EPERM;

	ether_addr_copy(hw->mac.addr, addr->sa_data);
	ether_addr_copy(hw->mac.perm_addr, addr->sa_data);
	ether_addr_copy(netdev->dev_addr, addr->sa_data);

	return 0;
}

/**
 * ngbe_change_mtu - Change the Maximum Transfer Unit
 * @netdev: network interface device structure
 * @new_mtu: new value for maximum frame size
 *
 * Returns 0 on success, negative on failure
 **/
int ngbe_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	struct ngbevf_hw *hw = &adapter->hw;
	int max_frame = new_mtu + ETH_HLEN + ETH_FCS_LEN;
	int ret;

#ifndef HAVE_NETDEVICE_MIN_MAX_MTU
		/* MTU < 68 is an error and causes problems on some kernels */
	if ((new_mtu < 68) || (max_frame > NGBE_MAX_JUMBO_FRAME_SIZE))
		return -EINVAL;
#else
	if ((new_mtu < 68) || (new_mtu > 9414))
		return -EINVAL;
#endif
	/* prevent MTU being changed to a size unsupported by XDP */
	if (adapter->xdp_prog) {
		dev_warn(&adapter->pdev->dev, "MTU cannot be changed while XDP program is loaded\n");
		return -EPERM;
	}

	/* notify the PF of our intent to use this size of frame */
	ret = ngbe_rlpml_set_vf(hw, max_frame);
	if (ret)
		return -EINVAL;

	DPRINTK(PROBE, INFO, "changing MTU from %d to %d\n",
		netdev->mtu, new_mtu);

	/* set new MTU */
	netdev->mtu = new_mtu;

	if (netif_running(netdev))
		ngbevf_reinit_locked(adapter);

	return 0;
}

#ifdef ETHTOOL_OPS_COMPAT
/**
 * ngbe_ioctl -
 * @netdev:
 * @ifreq:
 * @cmd:
 **/
int ngbe_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	switch (cmd) {
	case SIOCETHTOOL:
		return ethtool_ioctl(ifr);
	default:
		return -EOPNOTSUPP;
	}
}
#endif

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
 * Polling 'interrupt' - used by things like netconsole to send skbs
 * without having to re-enable interrupts. It's not called while
 * the interrupt routine is executing.
 */
void ngbe_netpoll(struct net_device *netdev)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	int i;

	/* if interface is down do nothing */
	if (test_bit(__NGBE_DOWN, &adapter->state))
		return;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		adapter->q_vector[i]->netpoll_rx = true;
		ngbe_msix_rings(0, adapter->q_vector[i]);
	}
}
#endif /* CONFIG_NET_POLL_CONTROLLER */


#ifndef USE_REBOOT_NOTIFIER
int ngbe_suspend(struct pci_dev *pdev, pm_message_t __maybe_unused state)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
#ifdef CONFIG_PM
	int retval = 0;
#endif

	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		rtnl_lock();
		ngbe_down(adapter);
		ngbe_free_irq(adapter);
		ngbe_free_all_tx_resources(adapter);
		ngbe_free_all_rx_resources(adapter);
		rtnl_unlock();
	}

	ngbe_clear_interrupt_scheme(adapter);

#ifdef CONFIG_PM
	retval = pci_save_state(pdev);
	if (retval)
		return retval;

#endif
	if (!test_and_set_bit(__NGBE_DISABLED, &adapter->state))
		pci_disable_device(pdev);

	return 0;
}

#ifdef CONFIG_PM
int ngbe_resume(struct pci_dev *pdev)
{
	struct ngbevf_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
	u32 err;

	pci_restore_state(pdev);
	/*
	 * pci_restore_state clears dev->state_saved so call
	 * pci_save_state to restore it.
	 */
	pci_save_state(pdev);

	err = pci_enable_device_mem(pdev);
	if (err) {
		dev_err(&pdev->dev, "Cannot enable PCI device from suspend\n");
		return err;
	}

	adapter->hw.hw_addr = adapter->io_addr;
	smp_mb__before_atomic();
	clear_bit(__NGBE_DISABLED, &adapter->state);
	pci_set_master(pdev);

	ngbe_reset(adapter);

	rtnl_lock();
	err = ngbe_init_interrupt_scheme(adapter);
	if (!err && netif_running(netdev))
		err = ngbe_open(netdev);
	rtnl_unlock();
	if (err)
		return err;

	netif_device_attach(netdev);

	return err;
}
#endif /* CONFIG_PM */

void ngbe_shutdown(struct pci_dev *pdev)
{
	ngbe_suspend(pdev, PMSG_SUSPEND);
}
#endif /* USE_REBOOT_NOTIFIER */

#ifdef HAVE_NDO_GET_STATS64
#ifdef HAVE_VOID_NDO_GET_STATS64
void ngbe_get_stats64(struct net_device *netdev,
				ngbe_net_stats_t *stats)
#else
ngbe_net_stats_t *ngbe_get_stats64(struct net_device *netdev,
						ngbe_net_stats_t *stats)
#endif
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	u64 bytes, packets;
	const struct ngbe_ring *ring;
	int i;
	ngbe_net_stats_t *net_stats = &adapter->net_stats;

	rcu_read_lock();
	for (i = 0; i < adapter->num_rx_queues; i++) {
		unsigned int start;

		ring = adapter->rx_ring[i];
		do {
			start = u64_stats_fetch_begin_irq(&ring->syncp);
			bytes = ring->stats.bytes;
			packets = ring->stats.packets;
		} while (u64_stats_fetch_retry_irq(&ring->syncp, start));
		stats->rx_bytes += bytes;
		stats->rx_packets += packets;
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		unsigned int start;

		ring = adapter->tx_ring[i];
		do {
			start = u64_stats_fetch_begin_irq(&ring->syncp);
			bytes = ring->stats.bytes;
			packets = ring->stats.packets;
		} while (u64_stats_fetch_retry_irq(&ring->syncp, start));
		stats->tx_bytes += bytes;
		stats->tx_packets += packets;
	}
	rcu_read_unlock();

	/* following stats updated by ngbe_watchdog_subtask() */
	stats->multicast	= net_stats->multicast;
	stats->tx_errors	= net_stats->tx_errors;
	stats->tx_dropped	= net_stats->tx_dropped;
	stats->rx_errors	= net_stats->rx_errors;
	stats->rx_dropped	= net_stats->rx_dropped;
	stats->rx_crc_errors	= net_stats->rx_crc_errors;
	stats->rx_length_errors	= net_stats->rx_length_errors;
#ifndef HAVE_VOID_NDO_GET_STATS64
	return stats;
#endif
}
#else /* !HAVE_NDO_GET_STATS64 */
/**
 * ngbe_get_stats - Get System Network Statistics
 * @netdev: network interface device structure
 *
 * Returns the address of the device statistics structure.
 * The statistics are actually updated from the timer callback.
 **/
ngbe_net_stats_t *ngbe_get_stats(struct net_device *netdev)
{
	struct ngbevf_adapter *adapter = netdev_priv(netdev);
	ngbe_net_stats_t *net_stats = &adapter->net_stats;
	const struct ngbe_ring *ring;
	unsigned long bs, ps, i;

	for (bs = 0, ps = 0, i = 0; i < adapter->num_rx_queues; i++) {
		ring = adapter->rx_ring[i];
		bs += ring->stats.bytes;
		ps += ring->stats.packets;
	}
	net_stats->rx_bytes = bs;
	net_stats->rx_packets = ps;

	for (bs = 0, ps = 0, i = 0; i < adapter->num_tx_queues; i++) {
		ring = adapter->tx_ring[i];
		bs += ring->stats.bytes;
		ps += ring->stats.packets;
	}
	net_stats->tx_bytes = bs;
	net_stats->tx_packets = ps;

	/* only return the current stats */
	return net_stats;
}
#endif /* HAVE_NDO_GET_STATS64 */
#ifdef NETIF_F_GSO_PARTIAL

#define NGBE_MAX_MAC_HDR_LEN		127
#define NGBE_MAX_NETWORK_HDR_LEN	511

static netdev_features_t
ngbe_features_check(struct sk_buff *skb, struct net_device *dev,
		       netdev_features_t features)
{
	unsigned int network_hdr_len, mac_hdr_len;

	/* Make certain the headers can be described by a context descriptor */
	mac_hdr_len = skb_network_header(skb) - skb->data;
	if (unlikely(mac_hdr_len > NGBE_MAX_MAC_HDR_LEN))
		return features & ~(NETIF_F_HW_CSUM |
				    NETIF_F_SCTP_CRC |
				    NETIF_F_HW_VLAN_CTAG_TX |
				    NETIF_F_TSO |
				    NETIF_F_TSO6);

	network_hdr_len = skb_checksum_start(skb) - skb_network_header(skb);
	if (unlikely(network_hdr_len >  NGBE_MAX_NETWORK_HDR_LEN))
		return features & ~(NETIF_F_HW_CSUM |
				    NETIF_F_SCTP_CRC |
				    NETIF_F_TSO |
				    NETIF_F_TSO6);

	/* We can only support IPV4 TSO in tunnels if we can mangle the
	 * inner IP ID field, so strip TSO if MANGLEID is not supported.
	 */
	if (skb->encapsulation && !(features & NETIF_F_TSO_MANGLEID))
		features &= ~NETIF_F_TSO;

	return features;
}
#endif /* NETIF_F_GSO_PARTIAL */

#ifdef HAVE_XDP_SUPPORT
static int ngbe_xdp_setup(struct net_device *dev, struct bpf_prog *prog)
{
	int i, frame_size = dev->mtu + ETH_HLEN + ETH_FCS_LEN + VLAN_HLEN;
	struct ngbevf_adapter *adapter = netdev_priv(dev);
	struct bpf_prog *old_prog;

	/* verify ngbevf ring attributes are sufficient for XDP */
	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct ngbe_ring *ring = adapter->rx_ring[i];

		if (frame_size > ngbe_rx_bufsz(ring))
			return -EINVAL;
	}

	old_prog = xchg(&adapter->xdp_prog, prog);

	/* If transitioning XDP modes reconfigure rings */
	if (!!prog != !!old_prog) {
		/* Hardware has to reinitialize queues and interrupts to
		 * match packet buffer alignment. Unfortunately, the
		 * hardware is not flexible enough to do this dynamically.
		 */
		if (netif_running(dev))
			ngbe_close(dev);

		ngbe_clear_interrupt_scheme(adapter);
		ngbe_init_interrupt_scheme(adapter);

		if (netif_running(dev))
			ngbe_open(dev);
	} else {
		for (i = 0; i < adapter->num_rx_queues; i++)
			xchg(&adapter->rx_ring[i]->xdp_prog, adapter->xdp_prog);
	}

	if (old_prog)
		bpf_prog_put(old_prog);

	return 0;
}

#ifdef HAVE_NDO_BPF
static int ngbe_xdp(struct net_device *dev, struct netdev_bpf *xdp)
#else
static int ngbe_xdp(struct net_device *dev, struct netdev_xdp *xdp)
#endif /* HAVE_NDO_BPF */
{
	struct ngbevf_adapter *adapter = netdev_priv(dev);

	switch (xdp->command) {
	case XDP_SETUP_PROG:
		return ngbe_xdp_setup(dev, xdp->prog);
	default:
		return -EINVAL;
	}
}

#endif /* HAVE_XDP_SUPPORT */
#ifdef HAVE_NET_DEVICE_OPS
const struct net_device_ops ngbe_netdev_ops = {
	.ndo_open               = ngbe_open,
	.ndo_stop               = ngbe_close,
	.ndo_start_xmit         = ngbe_xmit_frame,
	.ndo_set_rx_mode        = ngbe_set_rx_mode,
#ifdef HAVE_NDO_GET_STATS64
	.ndo_get_stats64        = ngbe_get_stats64,
#else
	.ndo_get_stats          = ngbe_get_stats,
#endif
	.ndo_validate_addr      = eth_validate_addr,
	.ndo_set_mac_address    = ngbe_set_mac,
#ifdef HAVE_RHEL7_EXTENDED_MIN_MAX_MTU
	.extended.ndo_change_mtu         = ngbe_change_mtu,
#else
	.ndo_change_mtu         = ngbe_change_mtu,
#endif
#ifdef ETHTOOL_OPS_COMPAT
	.ndo_do_ioctl           = ngbe_ioctl,
#endif
	.ndo_tx_timeout         = ngbe_tx_timeout,
#ifdef HAVE_VLAN_RX_REGISTER
	.ndo_vlan_rx_register   = ngbe_vlan_rx_register,
#endif
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	.ndo_vlan_rx_add_vid    = ngbe_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid   = ngbe_vlan_rx_kill_vid,
#endif
#ifndef HAVE_RHEL6_NET_DEVICE_EXTENDED
#ifdef HAVE_NDO_BUSY_POLL
	.ndo_busy_poll          = ngbe_busy_poll_recv,
#endif
#endif /* !HAVE_RHEL6_NET_DEVICE_EXTENDED */
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller    = ngbe_netpoll,
#endif
#if defined(NETIF_F_GSO_PARTIAL)
	.ndo_features_check	= ngbe_features_check,
#elif defined(HAVE_PASSTHRU_FEATURES_CHECK)
	.ndo_features_check	= passthru_features_check,
#endif
#ifdef HAVE_XDP_SUPPORT
#ifdef HAVE_NDO_BPF
	.ndo_bpf		= ngbe_xdp,
#else
	.ndo_xdp		= ngbe_xdp,
#endif /* HAVE_NDO_BPF */
#endif /* HAVE_XDP_SUPPORT */
};
#endif /* HAVE_NET_DEVICE_OPS */

void ngbe_assign_netdev_ops(struct net_device *dev)
{
#ifdef HAVE_NET_DEVICE_OPS
	dev->netdev_ops = &ngbe_netdev_ops;
#else /* HAVE_NET_DEVICE_OPS */
	dev->open = ngbe_open;
	dev->stop = ngbe_close;

	dev->hard_start_xmit = ngbe_xmit_frame;

	dev->get_stats = ngbe_get_stats;
	dev->set_multicast_list = ngbe_set_rx_mode;
	dev->set_mac_address = ngbe_set_mac;
	dev->change_mtu = ngbe_change_mtu;
#ifdef ETHTOOL_OPS_COMPAT
	dev->do_ioctl = ngbe_ioctl;
#endif
#ifdef HAVE_TX_TIMEOUT
	dev->tx_timeout = ngbe_tx_timeout;
#endif
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	dev->vlan_rx_register = ngbe_vlan_rx_register;
	dev->vlan_rx_add_vid = ngbe_vlan_rx_add_vid;
	dev->vlan_rx_kill_vid = ngbe_vlan_rx_kill_vid;
#endif
#ifdef CONFIG_NET_POLL_CONTROLLER
	dev->poll_controller = ngbe_netpoll;
#endif
#endif /* HAVE_NET_DEVICE_OPS */

#ifdef HAVE_RHEL6_NET_DEVICE_EXTENDED
#ifdef CONFIG_NET_RX_BUSY_POLL
	netdev_extended(dev)->ndo_busy_poll = ngbe_busy_poll_recv;
#endif /* CONFIG_NET_RX_BUSY_POLL */
#endif /* HAVE_RHEL6_NET_DEVICE_EXTENDED */

	ngbe_set_ethtool_ops(dev);
	dev->watchdog_timeo = 5 * HZ;
}

static void ngbevf_print_device_info(struct ngbevf_adapter *adapter)
{
	struct ngbevf_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	struct pci_dev *pdev = adapter->pdev;

	if (hw->mac.type == ngbe_mac_em_vf)
		dev_info(pci_dev_to_dev(pdev), "Wangxun Gigabit Virtual Function\n");
	else
		dev_info(pci_dev_to_dev(pdev), "UNKNOWN\n");
	dev_info(pci_dev_to_dev(pdev), "Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       /* MAC address */
	       netdev->dev_addr[0], netdev->dev_addr[1],
	       netdev->dev_addr[2], netdev->dev_addr[3],
	       netdev->dev_addr[4], netdev->dev_addr[5]);
}


static void ngbe_set_features(struct ngbevf_adapter *adapter, u8 fea_flags)
{
	struct net_device *netdev = adapter->netdev;

	/*
	 * netdev->features
	 */
	netdev->features |= NETIF_F_SG |
			    NETIF_F_IP_CSUM;
#ifdef NETIF_F_IPV6_CSUM
	netdev->features |= NETIF_F_IPV6_CSUM;
#endif
	netdev->features |= NETIF_F_SCTP_CSUM;

#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	netdev->features |= NETIF_F_RXCSUM;
#endif

#ifdef NETIF_F_RXHASH
	netdev->features |= NETIF_F_RXHASH;
#endif

#ifdef NETIF_F_TSO
	netdev->features |= NETIF_F_TSO;
#endif

#ifdef NETIF_F_TSO6
	netdev->features |= NETIF_F_TSO6;
#endif
	if (fea_flags & 0x1)
		netdev->features |= NETIF_F_HIGHDMA;

	/*
	 * netdev->hw_features
	 */
#ifdef HAVE_NDO_SET_FEATURES
#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
	set_netdev_hw_features(netdev,
		netdev->features | get_netdev_hw_features(netdev));
#else
	netdev->hw_features |= netdev->features;
	netdev->hw_features |= (adapter->flagsd & NGBE_F_CAP_LRO
			? NETIF_F_LRO : 0);
#endif
#else /* !HAVE_NDO_SET_FEATURES */
#ifdef NETIF_F_GRO
	netdev->features |= NETIF_F_GRO; /* only needed on kernels <2.6.39 */
#endif
#endif /* HAVE_NDO_SET_FEATURES */

	/*
	 * netdev->vlan_features
	 */
#ifdef HAVE_NETDEV_VLAN_FEATURES
	netdev->vlan_features = netdev->features;
#endif


    /*
     * netdev->features
     */     
/* set this bit last since it cannot be part of vlan_features */
#ifdef NETIF_F_HW_VLAN_STAG_TX
	netdev->features |= NETIF_F_HW_VLAN_STAG_TX |
			    NETIF_F_HW_VLAN_STAG_RX |
			    NETIF_F_HW_VLAN_STAG_FILTER;
#endif

#ifdef NETIF_F_HW_VLAN_CTAG_TX
	netdev->features |= NETIF_F_HW_VLAN_CTAG_TX |
			    NETIF_F_HW_VLAN_CTAG_RX |
			    NETIF_F_HW_VLAN_CTAG_FILTER;
#endif

#ifdef NETIF_F_HW_VLAN_TX
	netdev->features |= NETIF_F_HW_VLAN_TX |
			    NETIF_F_HW_VLAN_RX |
			    NETIF_F_HW_VLAN_FILTER;
#endif
}

/**
 * ngbe_probe - Device Initialization Routine
 * @pdev: PCI device information struct
 * @ent: entry in ngbe_pci_tbl
 *
 * Returns 0 on success, negative on failure
 *
 * ngbe_probe initializes an adapter identified by a pci_dev structure.
 * The OS initialization, configuring of the adapter private structure,
 * and a hardware reset occur.
 **/
static int __devinit ngbe_probe(struct pci_dev *pdev,
				   const struct pci_device_id *ent)
{
	struct net_device *netdev;
	struct ngbevf_adapter *adapter = NULL;
#ifdef HAVE_NETDEVICE_MIN_MAX_MTU
	unsigned int min_mtu, max_mtu;
#endif
	struct ngbevf_hw *hw = NULL;
	const struct ngbe_info *ei = ngbe_info_tbl[ent->driver_data];

	static int cards_found;
	int err;
	
	bool disable_dev = false;
	u8 fea_flags = 0;

	err = pci_enable_device(pdev);
	if (err)
		return err;

	if (!dma_set_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(64)) &&
	    !dma_set_coherent_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(64))) {
		fea_flags |= 0x1;
	} else {
		err = dma_set_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(32));
		if (err) {
			err = dma_set_coherent_mask(pci_dev_to_dev(pdev),
						    DMA_BIT_MASK(32));
			if (err) {
				dev_err(pci_dev_to_dev(pdev), "No usable DMA "
					"configuration, aborting\n");
				goto err_dma;
			}
		}
	}

	err = pci_request_regions(pdev, ngbe_driver_name);
	if (err) {
		dev_err(pci_dev_to_dev(pdev),
			"pci_request_regions failed 0x%x\n", err);
		goto err_pci_reg;
	}

	/*for test*/
	pci_enable_pcie_error_reporting(pdev);
	pci_set_master(pdev);

#ifdef HAVE_TX_MQ
	netdev = alloc_etherdev_mq(sizeof(struct ngbevf_adapter), MAX_TX_QUEUES);
#else
	netdev = alloc_etherdev(sizeof(struct ngbevf_adapter));
#endif

	if (!netdev) {
		err = -ENOMEM;
		goto err_alloc_etherdev;
	}
	
	SET_NETDEV_DEV(netdev, &pdev->dev);

	adapter = netdev_priv(netdev);
	debug_adapter = adapter;
	adapter->netdev = netdev;
	adapter->pdev = pdev;
	hw = &adapter->hw;

	hw->back = adapter;
	hw->msg_enable = &adapter->msg_enable;
	hw->pdev = adapter->pdev;

	hw->mac.type = ei->mac;
	adapter->flagsd = ei->flags;
	adapter->msg_enable = DEFAULT_DEBUG_LEVEL;

	/*
	 * call save state here in standalone driver because it relies on
	 * adapter struct to exist, and needs to call netdev_priv
	 */
#ifdef HAVE_PCI_ERS
	pci_save_state(pdev);
#endif

	hw->b4_addr = ioremap(pci_resource_start(pdev, 4),
			      pci_resource_len(pdev, 4));
	if (!hw->b4_addr) {
		err = -EIO;
		goto err_ioremap;
	}

	hw->hw_addr = ioremap(pci_resource_start(pdev, 0),
			      pci_resource_len(pdev, 0));
	adapter->io_addr = hw->hw_addr;
	if (!hw->hw_addr) {
		iounmap(hw->b4_addr);
		err = -EIO;
		goto err_ioremap;
	}

	/* setup the private structure */
	err = ngbe_sw_init(adapter);
	if (err)
		goto err_sw_init;

	ngbe_assign_netdev_ops(netdev);

	strncpy(netdev->name, pci_name(pdev), sizeof(netdev->name) - 1);
	adapter->bd_number = cards_found;

	/* check_options must be called before setup_link_speed to set up
	 * hw->fc completely
	 */
	ngbe_check_options(adapter);

	/* netdev offload capabilities */
	ngbe_set_features(adapter, fea_flags);

	/* The HW MAC address was set and/or determined in sw_init */
	ether_addr_copy(netdev->dev_addr, adapter->hw.mac.addr);
	ether_addr_copy(netdev->perm_addr, adapter->hw.mac.addr);

	if (!is_valid_ether_addr(netdev->dev_addr)) {
		dev_info(pci_dev_to_dev(pdev),
			 "ngbe: invalid MAC address\n");
		err = -EIO;
		goto err_sw_init;
	}

#ifdef HAVE_NETDEVICE_MIN_MAX_MTU
		/* MTU range: 68 - 1504 or 9414 */
		min_mtu = ETH_MIN_MTU;
		switch (adapter->hw.api_version) {
		case ngbe_mbox_api_11:
		case ngbe_mbox_api_12:
		case ngbe_mbox_api_13:
			max_mtu = NGBE_MAX_JUMBO_FRAME_SIZE -
				  (ETH_HLEN + ETH_FCS_LEN);
			break;
		default:
			if (adapter->hw.mac.type != ngbe_mac_em_vf)
				max_mtu = NGBE_MAX_JUMBO_FRAME_SIZE -
					  (ETH_HLEN + ETH_FCS_LEN);
			else
				max_mtu = ETH_DATA_LEN + ETH_FCS_LEN;
			break;
		}
	
#ifdef HAVE_RHEL7_EXTENDED_MIN_MAX_MTU
		netdev->extended->min_mtu = min_mtu;
		netdev->extended->max_mtu = max_mtu;
#else
		netdev->min_mtu = min_mtu;
		netdev->max_mtu = max_mtu;
#endif
#endif

	timer_setup(&adapter->service_timer, ngbe_service_timer, 0);

	if (NGBE_REMOVED(hw->hw_addr)) {
		err = -EIO;
		goto err_sw_init;
	}
	INIT_WORK(&adapter->service_task, ngbe_service_task);
	set_bit(__NGBE_SERVICE_INITED, &adapter->state);
	clear_bit(__NGBE_SERVICE_SCHED, &adapter->state);

	err = ngbe_init_interrupt_scheme(adapter);
	if (err)
		goto err_sw_init;

	strcpy(netdev->name, "eth%d");

	err = register_netdev(netdev);
	if (err)
		goto err_register;

	pci_set_drvdata(pdev, netdev);
	netif_carrier_off(netdev);

	netif_tx_stop_all_queues(netdev);

	ngbevf_print_device_info(adapter);
	ngbe_init_last_counter_stats(adapter);

#ifdef HAVE_NGBE_DEBUG_FS
	ngbe_dbg_adapter_init(adapter);
#endif

#ifdef NETIF_F_GRO
        if (netdev->features & NETIF_F_GRO)
                DPRINTK(PROBE, INFO, "GRO is enabled\n");
#endif

	DPRINTK(PROBE, INFO, "%s\n", ngbe_driver_string);

	cards_found++;
	return 0;

err_register:
	ngbe_clear_interrupt_scheme(adapter);
err_sw_init:
	ngbe_reset_interrupt_capability(adapter);
	iounmap(adapter->io_addr);
err_ioremap:
	disable_dev = !test_and_set_bit(__NGBE_DISABLED, &adapter->state);
	free_netdev(netdev);
err_alloc_etherdev:
	pci_release_regions(pdev);
err_pci_reg:
err_dma:
	if (!adapter || disable_dev)
		pci_disable_device(pdev);
	return err;
}

/**
 * ngbe_remove - Device Removal Routine
 * @pdev: PCI device information struct
 *
 * ngbe_remove is called by the PCI subsystem to alert the driver
 * that it should release a PCI device.  The could be caused by a
 * Hot-Plug event, or because the driver is going to be removed from
 * memory.
 **/
static void __devexit ngbe_remove(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct ngbevf_adapter *adapter;
	bool disable_dev;

	if (!netdev)
		return;

	adapter = netdev_priv(netdev);

#ifdef HAVE_NGBE_DEBUG_FS
	ngbe_dbg_adapter_exit(adapter);
#endif

	set_bit(__NGBE_REMOVING, &adapter->state);
	cancel_work_sync(&adapter->service_task);

	if (netdev->reg_state == NETREG_REGISTERED)
		unregister_netdev(netdev);

	ngbe_clear_interrupt_scheme(adapter);

	iounmap(adapter->hw.hw_addr);
	iounmap(adapter->hw.b4_addr);
	pci_release_regions(pdev);

	DPRINTK(PROBE, INFO, "Remove complete\n");

	disable_dev = !test_and_set_bit(__NGBE_DISABLED, &adapter->state);
	free_netdev(netdev);

	pci_disable_pcie_error_reporting(pdev);

	if (disable_dev)
		pci_disable_device(pdev);
}

/**
 * ngbe_io_error_detected - called when PCI error is detected
 * @pdev: Pointer to PCI device
 * @state: The current pci connection state
 *
 * This function is called after a PCI bus error affecting
 * this device has been detected.
 */
pci_ers_result_t ngbe_io_error_detected(struct pci_dev *pdev,
						  pci_channel_state_t state)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct ngbevf_adapter *adapter = netdev_priv(netdev);

	if (!test_bit(__NGBE_SERVICE_INITED, &adapter->state))
		return PCI_ERS_RESULT_DISCONNECT;

	rtnl_lock();
	netif_device_detach(netdev);

	if (netif_running(netdev))
		ngbe_close_suspend(adapter);

	if (state == pci_channel_io_perm_failure) {
		rtnl_unlock();
		return PCI_ERS_RESULT_DISCONNECT;
	}


	if (!test_and_set_bit(__NGBE_DISABLED, &adapter->state))
		pci_disable_device(pdev);
	rtnl_unlock();

	/* Request a slot slot reset. */
	return PCI_ERS_RESULT_NEED_RESET;
}

/**
 * ngbe_io_slot_reset - called after the pci bus has been reset.
 * @pdev: Pointer to PCI device
 *
 * Restart the card from scratch, as if from a cold-boot. Implementation
 * resembles the first-half of the ngbe_resume routine.
 */
pci_ers_result_t ngbe_io_slot_reset(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct ngbevf_adapter *adapter = netdev_priv(netdev);

	if (pci_enable_device_mem(pdev)) {
		dev_err(&pdev->dev,
			"Cannot re-enable PCI device after reset.\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}

	adapter->hw.hw_addr = adapter->io_addr;
	smp_mb__before_atomic();
	clear_bit(__NGBE_DISABLED, &adapter->state);
	pci_set_master(pdev);

	ngbe_reset(adapter);

	return PCI_ERS_RESULT_RECOVERED;
}

/**
 * ngbe_io_resume - called when traffic can start flowing again.
 * @pdev: Pointer to PCI device
 *
 * This callback is called when the error recovery driver tells us that
 * its OK to resume normal operation. Implementation resembles the
 * second-half of the ngbe_resume routine.
 */
void ngbe_io_resume(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);

	rtnl_lock();
	if (netif_running(netdev))
		ngbe_open(netdev);

	netif_device_attach(netdev);
	rtnl_unlock();
}

struct net_device *ngbevf_hw_to_netdev(const struct ngbevf_hw *hw)
{
	return ((struct ngbevf_adapter *)hw->back)->netdev;
}

struct ngbe_msg *ngbevf_hw_to_msg(const struct ngbevf_hw *hw)
{
	struct ngbevf_adapter *adapter =
		container_of(hw, struct ngbevf_adapter, hw);
	return (struct ngbe_msg *)&adapter->msg_enable;
}


/* PCI Error Recovery (ERS) */
static struct pci_error_handlers ngbe_err_handler = {
	.error_detected = ngbe_io_error_detected,
	.slot_reset = ngbe_io_slot_reset,
	.resume = ngbe_io_resume,
};

static struct pci_driver ngbe_driver = {
	.name     = ngbe_driver_name,
	.id_table = ngbe_pci_tbl,
	.probe    = ngbe_probe,
	.remove   = __devexit_p(ngbe_remove),
#ifdef CONFIG_PM
	/* Power Management Hooks */
	.suspend  = ngbe_suspend,
	.resume   = ngbe_resume,
#endif
#ifndef USE_REBOOT_NOTIFIER
	.shutdown = ngbe_shutdown,
#endif
	.err_handler = &ngbe_err_handler
};

/**
 * ngbe_init_module - Driver Registration Routine
 *
 * ngbe_init_module is the first routine called when the driver is
 * loaded. All it does is register with the PCI subsystem.
 **/
static int __init ngbe_init_module(void)
{
	int ret;
	printk(KERN_INFO "ngbe: %s - version %s\n", ngbe_driver_string,
	       ngbe_driver_version);

	printk(KERN_INFO "%s\n", ngbe_copyright);

#ifdef HAVE_NGBE_DEBUG_FS
	ngbe_dbg_init();
#endif /* HAVE_NGBE_DEBUG_FS */

	ret = pci_register_driver(&ngbe_driver);
	return ret;
}

module_init(ngbe_init_module);

/**
 * ngbe_exit_module - Driver Exit Cleanup Routine
 *
 * ngbe_exit_module is called just before the driver is removed
 * from memory.
 **/
static void __exit ngbe_exit_module(void)
{
	pci_unregister_driver(&ngbe_driver);

#ifdef HAVE_NGBE_DEBUG_FS
	ngbe_dbg_exit();
#endif /* HAVE_NGBE_DEBUG_FS */
}

module_exit(ngbe_exit_module);

MODULE_AUTHOR("WangXun Technology, <linux.nic@trustnetic.com>");
MODULE_DESCRIPTION("WangXun(R) GbE Virtual Function Linux Network Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(NGBE_CONF_VERSION);


/* ngbe_main.c */
