/*******************************************************************************

  WangXun(R) GbE PCI Express Virtual Function Linux Network Driver
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

#ifndef _NGBE_H_
#define _NGBE_H_

#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>

#ifdef SIOCETHTOOL
#include <linux/ethtool.h>
#endif
#ifdef NETIF_F_HW_VLAN_TX
#include <linux/if_vlan.h>
#endif
#ifdef HAVE_NDO_GET_STATS64
#include <linux/u64_stats_sync.h>
#endif

#include "kcompat.h"

#include "ngbe_type.h"
#include "ngbe_vf.h"

#ifdef CONFIG_NET_RX_BUSY_POLL
#include <net/busy_poll.h>
#define BP_EXTENDED_STATS
#endif

extern const char ngbe_driver_name[];
extern const char ngbe_driver_version[];
extern char ngbe_firmware_version[];
#define PFX "nxgbevf: "
#if 0
#define DPRINTK(nlevel, klevel, fmt, args...) \
	((void)((NETIF_MSG_##nlevel & adapter->msg_enable) && \
	printk(KERN_##klevel PFX "%s: %s: " fmt, adapter->netdev->name, \
		__FUNCTION__ , ## args)))
#endif

#define NGBE_MAX_TXD_PWR       14
#define NGBE_MAX_DATA_PER_TXD  (1 << NGBE_MAX_TXD_PWR)

/* Tx Descriptors needed, worst case */
#define TXD_USE_COUNT(S) DIV_ROUND_UP((S), NGBE_MAX_DATA_PER_TXD)
#define DESC_NEEDED (MAX_SKB_FRAGS + 4)

/* wrapper around a pointer to a socket buffer,
 * so a DMA handle can be stored along with the buffer */
struct ngbe_tx_buffer {
	struct ngbe_tx_desc *next_to_watch;
	unsigned long time_stamp;
	union {
		struct sk_buff *skb;
		/* XDP uses address ptr on irq_clean */
		void *data;
	};
	unsigned int bytecount;
	unsigned short gso_segs;
	__be16 protocol;
	DEFINE_DMA_UNMAP_ADDR(dma);
	DEFINE_DMA_UNMAP_LEN(len);
	u32 tx_flags;
};

struct ngbe_rx_buffer {
	struct sk_buff *skb;
	dma_addr_t dma_addr;
	struct page *page;
#if (BITS_PER_LONG > 32) || (PAGE_SIZE >= 65536)
	__u32 page_offset;
#else
	__u16 page_offset;
#endif
	__u16 pagecnt_bias;
};

struct ngbe_ring_stats {
	u64 packets;
	u64 bytes;
#ifdef BP_EXTENDED_STATS
	u64 yields;
	u64 misses;
	u64 cleaned;
#endif
};

struct ngbe_tx_queue_stats {
	u64 tx_restart_queue;
	u64 tx_busy;
	u64 tx_done_old;
};

struct ngbe_rx_queue_stats {
	u64 alloc_rx_page_failed;
	u64 alloc_rx_buff_failed;
	u64 alloc_rx_page;
	u64 csum_err;
};

enum ngbe_ring_state_t {
	__NGBE_RX_3K_BUFFER,
	__NGBE_RX_BUILD_SKB_ENABLED,
	__NGBE_TX_DETECT_HANG,
	__NGBE_HANG_CHECK_ARMED,
	__NGBE_RX_CSUM_UDP_ZERO_ERR,
	__NGBE_TX_XDP_RING,
	__NGBE_TX_XDP_RING_PRIMED,
};

#define ring_is_xdp(ring) \
		test_bit(__NGBE_TX_XDP_RING, &(ring)->state)
#define set_ring_xdp(ring) \
		set_bit(__NGBE_TX_XDP_RING, &(ring)->state)
#define clear_ring_xdp(ring) \
		clear_bit(__NGBE_TX_XDP_RING, &(ring)->state)

#define check_for_tx_hang(ring) \
	test_bit(__NGBE_TX_DETECT_HANG, &(ring)->state)
#define set_check_for_tx_hang(ring) \
	set_bit(__NGBE_TX_DETECT_HANG, &(ring)->state)
#define clear_check_for_tx_hang(ring) \
	clear_bit(__NGBE_TX_DETECT_HANG, &(ring)->state)

struct ngbe_ring {
	struct ngbe_ring *next;
	struct ngbe_q_vector *q_vector; /* backpointer to host q_vector */
	struct net_device *netdev; /* netdev ring belongs to */
	struct bpf_prog *xdp_prog;
	struct device *dev; /* device for DMA mapping */
	void *desc; /* descriptor ring memory */
	union {
		struct ngbe_tx_buffer *tx_buffer_info;
		struct ngbe_rx_buffer *rx_buffer_info;
	};
	unsigned long state;
	u8 __iomem *tail;
	dma_addr_t dma_addr; /* phys. address of descriptor ring */
	unsigned int size; /* length in bytes */

	u16 count; /* amount of descriptors */

	u8 que_idx; /* software netdev-relative queue offset */
	u8 reg_idx; /* hardware global-absolute ring offset */
	struct sk_buff *skb;
	u16 next_to_use;
	u16 next_to_clean;
	u16 next_to_alloc;

	struct ngbe_ring_stats stats;
#ifdef HAVE_NDO_GET_STATS64
	struct u64_stats_sync   syncp;
#endif
	union {
		struct ngbe_tx_queue_stats tx_stats;
		struct ngbe_rx_queue_stats rx_stats;
	};
#ifdef HAVE_XDP_BUFF_RXQ
	struct xdp_rxq_info xdp_rxq;
#endif
} ____cacheline_internodealigned_in_smp;

/* How many Rx Buffers do we bundle into one write to the hardware ? */
#define NGBE_RX_BUFFER_WRITE 16      /* Must be power of 2 */

#define MAX_RX_QUEUES (NGBE_VF_MAX_RX_QUEUES)
#define MAX_TX_QUEUES (NGBE_VF_MAX_TX_QUEUES)
#define MAX_XDP_QUEUES NGBE_VF_MAX_TX_QUEUES
#define NGBE_MAX_RSS_QUEUES		2
#define NGBE_VFRETA_SIZE	64	/* 64 entries */
#define NGBE_RSS_HASH_KEY_SIZE	40
#define NGBE_VFRSSRK_REGS		10	/* 10 registers for RSS key */

#define NGBE_DEFAULT_TXD   128
#define NGBE_DEFAULT_RXD   128
#define NGBE_MAX_TXD       4096
#define NGBE_MIN_TXD       64
#define NGBE_MAX_RXD       4096
#define NGBE_MIN_RXD       64

/* Supported Rx Buffer Sizes */
#define NGBE_RXBUFFER_256    (256)    /* Used for packet split */
#define NGBE_RXBUFFER_2048   (2048)
#define NGBE_RXBUFFER_3072   (3072)


/* largest size for single descriptor */
#define NGBE_MAX_RXBUFFER    NGBE_RXBUFFER_16384

#define NGBE_RX_HDR_SIZE NGBE_RXBUFFER_256
#define NGBE_RX_BUF_SIZE NGBE_RXBUFFER_2048
#define NGBE_RX_HDR_SIZE NGBE_RXBUFFER_256

#define MAXIMUM_ETHERNET_VLAN_SIZE (VLAN_ETH_FRAME_LEN + ETH_FCS_LEN)

#define NGBE_SKB_PAD		(NET_SKB_PAD + NET_IP_ALIGN)
#if (PAGE_SIZE < 8192)
#define NGBE_MAX_FRAME_BUILD_SKB \
	(SKB_WITH_OVERHEAD(NGBE_RXBUFFER_2048) - NGBE_SKB_PAD)
#else
#define NGBE_MAX_FRAME_BUILD_SKB	NGBE_RXBUFFER_2048
#endif

#define NGBE_TX_FLAGS_CSUM             (u32)(1)
#define NGBE_TX_FLAGS_VLAN             (u32)(1 << 1)
#define NGBE_TX_FLAGS_TSO              (u32)(1 << 2)
#define NGBE_TX_FLAGS_IPV4             (u32)(1 << 3)

#define NGBE_TX_FLAGS_VLAN_MASK        0xffff0000
#define NGBE_TX_FLAGS_VLAN_PRIO_MASK   0x0000e000
#define NGBE_TX_FLAGS_VLAN_SHIFT       16

#define ring_uses_large_buffer(ring) \
	test_bit(__NGBE_RX_3K_BUFFER, &(ring)->state)
#define set_ring_uses_large_buffer(ring) \
	set_bit(__NGBE_RX_3K_BUFFER, &(ring)->state)
#define clear_ring_uses_large_buffer(ring) \
	clear_bit(__NGBE_RX_3K_BUFFER, &(ring)->state)

#define ring_uses_build_skb(ring) \
	test_bit(__NGBE_RX_BUILD_SKB_ENABLED, &(ring)->state)
#define set_ring_build_skb_enabled(ring) \
	set_bit(__NGBE_RX_BUILD_SKB_ENABLED, &(ring)->state)
#define clear_ring_build_skb_enabled(ring) \
	clear_bit(__NGBE_RX_BUILD_SKB_ENABLED, &(ring)->state)

static inline unsigned int ngbe_rx_bufsz(struct ngbe_ring __maybe_unused *ring)
{
#if (PAGE_SIZE < 8192)
	if (ring_uses_large_buffer(ring))
		return NGBE_RXBUFFER_3072;

	if (ring_uses_build_skb(ring))
		return NGBE_MAX_FRAME_BUILD_SKB;
#endif
	return NGBE_RXBUFFER_2048;
}

static inline unsigned int ngbe_rx_pg_order(struct ngbe_ring *ring)
{
#if (PAGE_SIZE < 8192)
	if (ring_uses_large_buffer(ring))
		return 1;
#endif
	return 0;
}
#define ngbe_rx_pg_size(_ring) (PAGE_SIZE << ngbe_rx_pg_order(_ring))

#define check_for_tx_hang(ring) \
	test_bit(__NGBE_TX_DETECT_HANG, &(ring)->state)
#define set_check_for_tx_hang(ring) \
	set_bit(__NGBE_TX_DETECT_HANG, &(ring)->state)
#define clear_check_for_tx_hang(ring) \
	clear_bit(__NGBE_TX_DETECT_HANG, &(ring)->state)




#define NGBE_TX_FLAGS_CSUM             (u32)(1)
#define NGBE_TX_FLAGS_VLAN             (u32)(1 << 1)
#define NGBE_TX_FLAGS_TSO              (u32)(1 << 2)
#define NGBE_TX_FLAGS_IPV4             (u32)(1 << 3)


#define NGBE_TX_FLAGS_VLAN_MASK        0xffff0000
#define NGBE_TX_FLAGS_VLAN_PRIO_MASK   0x0000e000
#define NGBE_TX_FLAGS_VLAN_SHIFT       16

struct ngbe_ring_container {
	struct ngbe_ring *ring;      /* pointer to linked list of rings */
	unsigned int total_bytes;       /* total bytes processed this int */
	unsigned int total_packets;     /* total packets processed this int */
	u8 count;                       /* total number of rings in vector */
	u16 itr;                        /* current ITR setting for ring */
};

/* iterator for handling rings in ring container */
#define ngbe_for_each_ring(pos, head) \
	for (pos = (head).ring; pos != NULL; pos = pos->next)

/* MAX_Q_VECTORS of these are allocated,
 * but we only use one per queue-specific vector.
 */
struct ngbe_q_vector {
	struct ngbevf_adapter *adapter;
	u16 v_idx;              /* index of q_vector within array, also used for
				 * finding the bit in EICR and friends that
				 * represents the vector for this ring */
	u16 itr;                /* Interrupt throttle rate written to EITR */
	struct napi_struct napi;
#ifndef HAVE_NETDEV_NAPI_LIST
	struct net_device poll_dev;
#endif
	struct ngbe_ring_container rx, tx;
	struct rcu_head rcu;    /* to avoid race with update stats on free */
	char name[IFNAMSIZ + 9];
	bool netpoll_rx;

#ifdef CONFIG_NET_RX_BUSY_POLL
	unsigned int state;
#define NGBE_QV_STATE_IDLE           0
#define NGBE_QV_STATE_NAPI           1    /* NAPI owns this QV */
#define NGBE_QV_STATE_POLL           2    /* poll owns this QV */
#define NGBE_QV_STATE_DISABLED       4    /* QV is disabled */
#define NGBE_QV_OWNED (NGBE_QV_STATE_NAPI | NGBE_QV_STATE_POLL)
#define NGBE_QV_LOCKED (NGBE_QV_OWNED | NGBE_QV_STATE_DISABLED)
#define NGBE_QV_STATE_NAPI_YIELD     8    /* NAPI yielded this QV */
#define NGBE_QV_STATE_POLL_YIELD     16   /* poll yielded this QV */
#define NGBE_QV_YIELD (NGBE_QV_STATE_NAPI_YIELD | NGBE_QV_STATE_POLL_YIELD)
#define NGBE_QV_USER_PEND (NGBE_QV_STATE_POLL | NGBE_QV_STATE_POLL_YIELD)
	spinlock_t lock;
#endif /* CONFIG_NET_RX_BUSY_POLL */

	/* for dynamic allocation of rings associated with this q_vector */
	struct ngbe_ring ring[0] ____cacheline_internodealigned_in_smp;
};
#ifdef CONFIG_NET_RX_BUSY_POLL
static inline void ngbe_qv_init_lock(struct ngbe_q_vector *q_vector)
{

	spin_lock_init(&q_vector->lock);
	q_vector->state = NGBE_QV_STATE_IDLE;
}

/* called from the device poll routine to get ownership of a q_vector */
static inline bool ngbe_qv_lock_napi(struct ngbe_q_vector *q_vector)
{
	int rc = true;
	spin_lock_bh(&q_vector->lock);
	if (q_vector->state & NGBE_QV_LOCKED) {
		WARN_ON(q_vector->state & NGBE_QV_STATE_NAPI);
		q_vector->state |= NGBE_QV_STATE_NAPI_YIELD;
		rc = false;
#ifdef BP_EXTENDED_STATS
		q_vector->tx.ring->stats.yields++;
#endif
	} else {
		/* we don't care if someone yielded */
		q_vector->state = NGBE_QV_STATE_NAPI;
	}
	spin_unlock_bh(&q_vector->lock);
	return rc;
}

/* returns true is someone tried to get the qv while napi had it */
static inline bool ngbe_qv_unlock_napi(struct ngbe_q_vector *q_vector)
{
	int rc = false;
	spin_lock_bh(&q_vector->lock);
	WARN_ON(q_vector->state & (NGBE_QV_STATE_POLL |
				   NGBE_QV_STATE_NAPI_YIELD));

	if (q_vector->state & NGBE_QV_STATE_POLL_YIELD)
		rc = true;
	/* reset state to idle, unless QV is disabled */
	q_vector->state &= NGBE_QV_STATE_DISABLED;
	spin_unlock_bh(&q_vector->lock);
	return rc;
}

/* called from ngbe_low_latency_poll() */
static inline bool ngbe_qv_lock_poll(struct ngbe_q_vector *q_vector)
{
	int rc = true;
	spin_lock_bh(&q_vector->lock);
	if ((q_vector->state & NGBE_QV_LOCKED)) {
		q_vector->state |= NGBE_QV_STATE_POLL_YIELD;
		rc = false;
#ifdef BP_EXTENDED_STATS
		q_vector->rx.ring->stats.yields++;
#endif
	} else {
		/* preserve yield marks */
		q_vector->state |= NGBE_QV_STATE_POLL;
	}
	spin_unlock_bh(&q_vector->lock);
	return rc;
}

/* returns true if someone tried to get the qv while it was locked */
static inline bool ngbe_qv_unlock_poll(struct ngbe_q_vector *q_vector)
{
	int rc = false;
	spin_lock_bh(&q_vector->lock);
	WARN_ON(q_vector->state & (NGBE_QV_STATE_NAPI));

	if (q_vector->state & NGBE_QV_STATE_POLL_YIELD)
		rc = true;
	/* reset state to idle, unless QV is disabled */
	q_vector->state &= NGBE_QV_STATE_DISABLED;
	spin_unlock_bh(&q_vector->lock);
	return rc;
}

/* true if a socket is polling, even if it did not get the lock */
static inline bool ngbe_qv_busy_polling(struct ngbe_q_vector *q_vector)
{
	WARN_ON(!(q_vector->state & NGBE_QV_OWNED));
	return q_vector->state & NGBE_QV_USER_PEND;
}

/* false if QV is currently owned */
static inline bool ngbe_qv_disable(struct ngbe_q_vector *q_vector)
{
	int rc = true;
	spin_lock_bh(&q_vector->lock);
	if (q_vector->state & NGBE_QV_OWNED)
	    rc = false;
	q_vector->state |= NGBE_QV_STATE_DISABLED;
	spin_unlock_bh(&q_vector->lock);
	return rc;
}

#endif /* CONFIG_NET_RX_BUSY_POLL */

/*
 * microsecond values for various ITR rates shifted by 2 to fit itr register
 * with the first 3 bits reserved 0
 */
#define NGBE_MIN_RSC_ITR       (0x003)
#define NGBE_100K_ITR          (0x005)
#define NGBE_20K_ITR           (0x019)
#define NGBE_12K_ITR           (0x02A)

#define NGBE_DEF_INT_RATE      (1)
#define NGBE_MAX_INT_RATE      (488281)
#define NGBE_MIN_INT_RATE      (956)

static inline u16 ngbe_desc_unused(struct ngbe_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->count) + ntc - ntu - 1;
}

#define NGBE_RX_DESC(R, i)       \
	(&(((union ngbe_rx_desc *)((R)->desc))[i]))
#define NGBE_TX_DESC(R, i)       \
	(&(((struct ngbe_tx_desc *)((R)->desc))[i]))
#define NGBE_TX_CTXTDESC(R, i)           \
	(&(((struct ngbe_adv_tx_context_desc *)((R)->desc))[i]))

#define NGBE_MAX_JUMBO_FRAME_SIZE        9432

#define NON_Q_VECTORS (1)
#define MAX_Q_VECTORS (2)

#define MIN_MSIX_COUNT (1 + NON_Q_VECTORS)
#ifdef HAVE_STRUCT_DMA_ATTRS
#define NGBE_RX_DMA_ATTR NULL
#else
#define NGBE_RX_DMA_ATTR \
	(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)
#endif

/* board specific private data structure */
#define NGBE_F_CAP_RX_CSUM             (1UL << 0)
#define NGBE_F_CAP_LRO	               (1UL << 1)
#define NGBE_F_REQ_RESET               (1UL << 2)
#define NGBE_F_REQ_QUEUE_RESET         (1UL << 3)
#define NGBE_F_ENA_RSS_IPV4UDP         (1UL << 4)
#define NGBE_F_ENA_RSS_IPV6UDP         (1UL << 5)

struct ngbevf_adapter {
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
#ifdef HAVE_VLAN_RX_REGISTER
	struct vlan_group *vlgrp; /* must be first, see ngbe_receive_skb */
#else
	/* this field must be first, see ngbe_process_skb_fields */
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
#endif
#endif /* NETIF_F_HW_VLAN_TX || NETIF_F_HW_VLAN_CTAG_TX */

	struct net_device *netdev;
	struct bpf_prog *xdp_prog;
	struct pci_dev *pdev;

	unsigned long state;

	u32 flagsd; /* flags define: CAP */
	u32 flagss; /* flags status: ENA */
	/* XDP */
	u16 xdp_ring_count;
	u16 num_xdp_queues;

	/* Tx hotpath */
	u16 tx_ring_count;
	u16 num_tx_queues;
	u16 tx_itr_setting;

	/* Rx hotpath */
	u16 rx_ring_count;
	u16 num_rx_queues;
	u16 rx_itr_setting;

	/* Rings, Tx first since it is accessed in hotpath */
	struct ngbe_ring *tx_ring[MAX_TX_QUEUES]; /* One per active queue */
	struct ngbe_ring *xdp_ring[MAX_XDP_QUEUES];
	struct ngbe_ring *rx_ring[MAX_RX_QUEUES]; /* One per active queue */

	/* interrupt vector accounting */
	struct ngbe_q_vector *q_vector[MAX_Q_VECTORS];
	int num_q_vectors;
	struct msix_entry *msix_entries;

	/* interrupt masks */
	u32 eims_enable_mask;
	u32 eims_other;

	/* stats */
	u64 tx_busy;
	u64 restart_queue;
	u64 hw_rx_no_dma_resources;
	u64 hw_csum_rx_error;
	u64 alloc_rx_page_failed;
	u64 alloc_rx_buff_failed;
	u64 alloc_rx_page;

#ifndef HAVE_NETDEV_STATS_IN_NETDEV
	struct net_device_stats net_stats;
#endif

	u32 tx_timeout_count;

	struct ngbevf_hw hw;

	/* statistic states */
	ngbe_net_stats_t net_stats;
	struct ngbe_sw_stats sw_stats;
	struct ngbevf_hw_stats stats, last_stats, base_stats, reset_stats;

	u32 *config_space;

	u16 bd_number;

#define DEFAULT_DEBUG_LEVEL (0x7)
	u16 msg_enable;

	u8 __iomem *io_addr;
	u32 link_speed;
	bool link_up;
	bool dev_closed;

	struct timer_list service_timer;
	struct work_struct service_task;

	spinlock_t mbx_lock;
	unsigned long last_reset;

#ifdef HAVE_NGBE_DEBUG_FS
	struct dentry *ngbe_dbg_adapter;
#endif

	u32 rss_key[NGBE_VFRSSRK_REGS];
	u8 rss_indir_tbl[NGBE_VFRETA_SIZE];
	u32 flags;
#define NGBE_FLAG_RX_CSUM_ENABLED		BIT(1)
#define NGBE_FLAGS_LEGACY_RX			BIT(2)
#define NGBE_FLAG_RSS_FIELD_IPV4_UDP		BIT(4)
#define NGBE_FLAG_RSS_FIELD_IPV6_UDP		BIT(5)
};

struct ngbe_info {
	enum ngbe_mac_type     mac;
	unsigned int            flags;
};

enum ixbgevf_state_t {
	__NGBE_TESTING,
	__NGBE_RESETTING,
	__NGBE_DOWN,
	__NGBE_DISABLED,
	__NGBE_REMOVING,
	__NGBE_SERVICE_SCHED,
	__NGBE_SERVICE_INITED,
	__NGBE_RESET_REQUESTED,
	__NGBE_QUEUE_RESET_REQUESTED,
};

enum ngbe_xcast_modes {
	NGBE_XCAST_MODE_NONE = 0,
	NGBE_XCAST_MODE_MULTI,
	NGBE_XCAST_MODE_ALLMULTI,
};

#ifdef HAVE_VLAN_RX_REGISTER
struct ngbe_cb {
	u16 vid;                        /* VLAN tag */
};
#define NGBE_CB(skb) ((struct ngbe_cb *)(skb)->cb)
#endif

/* needed by ngbe_main.c */
extern void ngbe_check_options(struct ngbevf_adapter *adapter);

extern void ngbe_free_irq(struct ngbevf_adapter *adapter);
extern void ngbe_configure(struct ngbevf_adapter *adapter);
extern int ngbe_request_irq(struct ngbevf_adapter *adapter);
extern void ngbe_up_complete(struct ngbevf_adapter *adapter);

extern int ngbe_open(struct net_device *netdev);
extern int ngbe_close(struct net_device *netdev);

extern void ngbe_up(struct ngbevf_adapter *adapter);
extern void ngbe_down(struct ngbevf_adapter *adapter);
extern void ngbevf_reinit_locked(struct ngbevf_adapter *adapter);
extern void ngbe_reset(struct ngbevf_adapter *adapter);
extern void ngbe_set_ethtool_ops(struct net_device *netdev);
extern int ngbe_setup_rx_resources(struct ngbevf_adapter *adapter, struct ngbe_ring *);
extern int ngbe_setup_tx_resources(struct ngbe_ring *);
extern void ngbe_free_rx_resources(struct ngbe_ring *);
extern void ngbe_free_tx_resources(struct ngbe_ring *);
extern void ngbe_update_stats(struct ngbevf_adapter *adapter);
extern void ngbe_write_eitr(struct ngbe_q_vector *);
extern void ngbe_reset_interrupt_capability(struct ngbevf_adapter *adapter);
extern int ngbe_init_interrupt_scheme(struct ngbevf_adapter *adapter);
extern bool ngbe_is_ngbevf(struct pci_dev *pcidev);

#ifdef ETHTOOL_OPS_COMPAT
extern int ethtool_ioctl(struct ifreq *ifr);

#endif
extern void ngbe_napi_add_all(struct ngbevf_adapter *adapter);
extern void ngbe_napi_del_all(struct ngbevf_adapter *adapter);

#ifdef HAVE_NGBE_DEBUG_FS
void ngbe_dbg_adapter_init(struct ngbevf_adapter *adapter);
void ngbe_dbg_adapter_exit(struct ngbevf_adapter *adapter);
void ngbe_dbg_init(void);
void ngbe_dbg_exit(void);
#endif /* HAVE_NGBE_DEBUG_FS */
void ngbe_dump(struct ngbevf_adapter *adapter);

/**
 * register operations
 **/
/* read register */
#define NGBE_DEAD_READ_RETRIES     10
#define NGBE_DEAD_READ_REG         0xdeadbeefU
#define NGBE_DEAD_READ_REG64       0xdeadbeefdeadbeefULL
#define NGBE_FAILED_READ_REG       0xffffffffU
#define NGBE_FAILED_READ_REG64     0xffffffffffffffffULL

static inline bool NGBE_REMOVED(void __iomem *addr)
{
	return unlikely(!addr);
}

static inline u32
ngbe_rd32(u8 __iomem *base, u32 reg)
{
	return readl(base + reg);
}

static inline u32
rd32(struct ngbevf_hw *hw, u32 reg)
{
	u8 __iomem *base = READ_ONCE(hw->hw_addr);
	u32 val = NGBE_FAILED_READ_REG;

	if (unlikely(!base))
		return val;

	val = ngbe_rd32(base, reg);

	return val;
}
#define rd32a(a, reg, offset) ( \
	rd32((a), (reg) + ((offset) << 2)))

static inline u32
rd32m(struct ngbevf_hw *hw, u32 reg, u32 mask)
{
	u8 __iomem *base = READ_ONCE(hw->hw_addr);
	u32 val = NGBE_FAILED_READ_REG;

	if (unlikely(!base))
		return val;

	val = ngbe_rd32(base, reg);
	if (unlikely(val == NGBE_FAILED_READ_REG))
		return val;

	return val & mask;
}

/* write register */
static inline void
ngbe_wr32(u8 __iomem *base, u32 reg, u32 val)
{
	writel(val, base + reg);;
}

static inline void
wr32(struct ngbevf_hw *hw, u32 reg, u32 val)
{
	u8 __iomem *base = READ_ONCE(hw->hw_addr);

	if (unlikely(!base))
		return;

	ngbe_wr32(base, reg, val);
}
#define wr32a(a, reg, off, val) \
	wr32((a), (reg) + ((off) << 2), (val))

static inline void
wr32m(struct ngbevf_hw *hw, u32 reg, u32 mask, u32 field)
{
	u8 __iomem *base = READ_ONCE(hw->hw_addr);
	u32 val;

	if (unlikely(!base))
		return;

	val = ngbe_rd32(base, reg);
	if (unlikely(val == NGBE_FAILED_READ_REG))
		return;

	val = ((val & ~mask) | (field & mask));
	ngbe_wr32(base, reg, val);
}

/* poll register */
#define NGBE_VF_INIT_TIMEOUT   200 /* Number of retries to clear RSTI */
#define NGBE_MDIO_TIMEOUT 1000
#define NGBE_I2C_TIMEOUT  1000
#define NGBE_SPI_TIMEOUT  1000
static inline s32
po32m(struct ngbevf_hw *hw, u32 reg,
		u32 mask, u32 field, u16 time, u16 loop)
{
	bool msec = false;

	if (time/loop > 1000*MAX_UDELAY_MS) {
		msec = true;
		time /= 1000;
	}

	do {
		u32 val = rd32(hw, reg);

		if (val == NGBE_FAILED_READ_REG)
			return NGBE_ERR_REG_ACCESS;

		if ((val != NGBE_DEAD_READ_REG) &&
		    (val & mask) == (field & mask))
			break;
		else if (--loop == 0)
			break;

		if (msec)
			mdelay(time);
		else
			udelay(time);
	} while (true);

	return (loop > 0 ? 0 : -NGBE_ERR_REG_TMOUT);
}

#define ngbe_flush(a) rd32(a, NGBE_VXSTATUS)

static inline struct netdev_queue *txring_txq(const struct ngbe_ring *ring)
{
	return netdev_get_tx_queue(ring->netdev, ring->que_idx);
}

#endif /* _NGBE_H_ */
