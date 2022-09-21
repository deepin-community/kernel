/*
 * WangXun 10 Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */


#ifndef _TXGBE_H_
#define _TXGBE_H_

#ifndef TXGBE_NO_LRO
#include <net/tcp.h>
#else
#include <net/ip.h>
#endif

#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>

#ifdef SIOCETHTOOL
#include <linux/ethtool.h>
#endif
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX) || \
	defined(NETIF_F_HW_VLAN_STAG_TX)
#include <linux/if_vlan.h>
#endif

#include "txgbe_dcb.h"

#include "kcompat.h"

#ifdef HAVE_NDO_BUSY_POLL
#include <net/busy_poll.h>
#define BP_EXTENDED_STATS
#endif

#ifdef HAVE_SCTP
#include <linux/sctp.h>
#endif

#ifdef HAVE_INCLUDE_LINUX_MDIO_H
#include <linux/mdio.h>
#endif

#if IS_ENABLED(CONFIG_FCOE)
#include "txgbe_fcoe.h"
#endif /* CONFIG_FCOE */


#ifdef HAVE_PTP_1588_CLOCK
#ifdef HAVE_INCLUDE_LINUX_TIMECOUNTER_H
#include <linux/timecounter.h>
#endif /* HAVE_INCLUDE_TIMECOUNTER_H */
#include <linux/clocksource.h>
#include <linux/net_tstamp.h>
#include <linux/ptp_clock_kernel.h>
#endif /* HAVE_PTP_1588_CLOCK */

/* Ether Types */
#define TXGBE_ETH_P_LLDP                        0x88CC
#define TXGBE_ETH_P_CNM                         0x22E7

/* TX/RX descriptor defines */
#if defined(DEFAULT_TXD) || defined(DEFAULT_TX_WORK)
#define TXGBE_DEFAULT_TXD               DEFAULT_TXD
#define TXGBE_DEFAULT_TX_WORK           DEFAULT_TX_WORK
#else
#define TXGBE_DEFAULT_TXD               512
#define TXGBE_DEFAULT_TX_WORK   256
#endif
#define TXGBE_MAX_TXD                   8192
#define TXGBE_MIN_TXD                   128

#if (PAGE_SIZE < 8192)
#define TXGBE_DEFAULT_RXD               512
#define TXGBE_DEFAULT_RX_WORK   256
#else
#define TXGBE_DEFAULT_RXD               256
#define TXGBE_DEFAULT_RX_WORK   128
#endif

#define TXGBE_MAX_RXD                   8192
#define TXGBE_MIN_RXD                   128

#define TXGBE_ETH_P_LLDP                0x88CC

/* flow control */
#define TXGBE_MIN_FCRTL                 0x40
#define TXGBE_MAX_FCRTL                 0x7FF80
#define TXGBE_MIN_FCRTH                 0x600
#define TXGBE_MAX_FCRTH                 0x7FFF0
#if defined(DEFAULT_FCPAUSE)
#define TXGBE_DEFAULT_FCPAUSE DEFAULT_FCPAUSE   /*0x3800*/
#else
#define TXGBE_DEFAULT_FCPAUSE   0xFFFF
#endif
#define TXGBE_MIN_FCPAUSE               0
#define TXGBE_MAX_FCPAUSE               0xFFFF

/* Supported Rx Buffer Sizes */
#define TXGBE_RXBUFFER_256       256  /* Used for skb receive header */
#define TXGBE_RXBUFFER_2K       2048
#define TXGBE_RXBUFFER_3K       3072
#define TXGBE_RXBUFFER_4K       4096
#ifdef CONFIG_TXGBE_DISABLE_PACKET_SPLIT
#define TXGBE_RXBUFFER_1536     1536
#define TXGBE_RXBUFFER_7K       7168
#define TXGBE_RXBUFFER_8K       8192
#define TXGBE_RXBUFFER_15K      15360
#endif /* CONFIG_TXGBE_DISABLE_PACKET_SPLIT */
#define TXGBE_MAX_RXBUFFER      16384  /* largest size for single descriptor */

#define TXGBE_BP_M_NULL                      0
#define TXGBE_BP_M_SFI                       1
#define TXGBE_BP_M_KR                        2
#define TXGBE_BP_M_KX4                       3
#define TXGBE_BP_M_KX                        4
#define TXGBE_BP_M_NAUTO                     0
#define TXGBE_BP_M_AUTO                      1


/*
 * NOTE: netdev_alloc_skb reserves up to 64 bytes, NET_IP_ALIGN means we
 * reserve 64 more, and skb_shared_info adds an additional 320 bytes more,
 * this adds up to 448 bytes of extra data.
 *
 * Since netdev_alloc_skb now allocates a page fragment we can use a value
 * of 256 and the resultant skb will have a truesize of 960 or less.
 */
#define TXGBE_RX_HDR_SIZE       TXGBE_RXBUFFER_256

#define MAXIMUM_ETHERNET_VLAN_SIZE      (VLAN_ETH_FRAME_LEN + ETH_FCS_LEN)

/* How many Rx Buffers do we bundle into one write to the hardware ? */
#define TXGBE_RX_BUFFER_WRITE   16      /* Must be power of 2 */

#ifdef HAVE_STRUCT_DMA_ATTRS
#define TXGBE_RX_DMA_ATTR NULL
#else
#define TXGBE_RX_DMA_ATTR \
	(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)
#endif

/* assume the kernel supports 8021p to avoid stripping vlan tags */
#ifdef TXGBE_DISABLE_8021P_SUPPORT
#ifndef HAVE_8021P_SUPPORT
#define HAVE_8021P_SUPPORT
#endif
#endif /* TXGBE_DISABLE_8021P_SUPPORT */

enum txgbe_tx_flags {
	/* cmd_type flags */
	TXGBE_TX_FLAGS_HW_VLAN  = 0x01,
	TXGBE_TX_FLAGS_TSO      = 0x02,
	TXGBE_TX_FLAGS_TSTAMP   = 0x04,

	/* olinfo flags */
	TXGBE_TX_FLAGS_CC       = 0x08,
	TXGBE_TX_FLAGS_IPV4     = 0x10,
	TXGBE_TX_FLAGS_CSUM     = 0x20,
	TXGBE_TX_FLAGS_OUTER_IPV4 = 0x100,
	TXGBE_TX_FLAGS_LINKSEC	= 0x200,
	TXGBE_TX_FLAGS_IPSEC    = 0x400,

	/* software defined flags */
	TXGBE_TX_FLAGS_SW_VLAN  = 0x40,
	TXGBE_TX_FLAGS_FCOE     = 0x80,
};

/* VLAN info */
#define TXGBE_TX_FLAGS_VLAN_MASK        0xffff0000
#define TXGBE_TX_FLAGS_VLAN_PRIO_MASK   0xe0000000
#define TXGBE_TX_FLAGS_VLAN_PRIO_SHIFT  29
#define TXGBE_TX_FLAGS_VLAN_SHIFT       16

#define TXGBE_MAX_RX_DESC_POLL          10

#define TXGBE_MAX_VF_MC_ENTRIES         30
#define TXGBE_MAX_VF_FUNCTIONS          64
#define MAX_EMULATION_MAC_ADDRS         16
#define TXGBE_MAX_PF_MACVLANS           15
#define TXGBE_VF_DEVICE_ID		0x1000

/* must account for pools assigned to VFs. */
#ifdef CONFIG_PCI_IOV
#define VMDQ_P(p)       ((p) + adapter->ring_feature[RING_F_VMDQ].offset)
#else
#define VMDQ_P(p)       (p)
#endif

#define UPDATE_VF_COUNTER_32bit(reg, last_counter, counter)     \
	{                                                       \
		u32 current_counter = rd32(hw, reg);  \
		if (current_counter < last_counter)             \
			counter += 0x100000000LL;               \
		last_counter = current_counter;                 \
		counter &= 0xFFFFFFFF00000000LL;                \
		counter |= current_counter;                     \
	}

#define UPDATE_VF_COUNTER_36bit(reg_lsb, reg_msb, last_counter, counter) \
	{                                                                \
		u64 current_counter_lsb = rd32(hw, reg_lsb);   \
		u64 current_counter_msb = rd32(hw, reg_msb);   \
		u64 current_counter = (current_counter_msb << 32) |      \
			current_counter_lsb;                             \
		if (current_counter < last_counter)                      \
			counter += 0x1000000000LL;                       \
		last_counter = current_counter;                          \
		counter &= 0xFFFFFFF000000000LL;                         \
		counter |= current_counter;                              \
	}

struct vf_stats {
	u64 gprc;
	u64 gorc;
	u64 gptc;
	u64 gotc;
	u64 mprc;
};

struct vf_data_storage {
	struct pci_dev *vfdev;
	u8 IOMEM *b4_addr;
	u32 b4_buf[16];
	unsigned char vf_mac_addresses[ETH_ALEN];
	u16 vf_mc_hashes[TXGBE_MAX_VF_MC_ENTRIES];
	u16 num_vf_mc_hashes;
	u16 default_vf_vlan_id;
	u16 vlans_enabled;
	bool clear_to_send;
	struct vf_stats vfstats;
	struct vf_stats last_vfstats;
	struct vf_stats saved_rst_vfstats;
	bool pf_set_mac;
	u16 pf_vlan; /* When set, guest VLAN config not allowed. */
	u16 pf_qos;
	u16 min_tx_rate;
	u16 max_tx_rate;
	u16 vlan_count;
	u8 spoofchk_enabled;
#ifdef HAVE_NDO_SET_VF_RSS_QUERY_EN
	bool rss_query_enabled;
#endif
	u8 trusted;
	int xcast_mode;
	unsigned int vf_api;
};

struct vf_macvlans {
	struct list_head l;
	int vf;
	bool free;
	bool is_macvlan;
	u8 vf_macvlan[ETH_ALEN];
};

#ifndef TXGBE_NO_LRO
#define TXGBE_LRO_MAX           32      /*Maximum number of LRO descriptors*/
#define TXGBE_LRO_GLOBAL        10

struct txgbe_lro_stats {
	u32 flushed;
	u32 coal;
};

/*
 * txgbe_lro_header - header format to be aggregated by LRO
 * @iph: IP header without options
 * @tcp: TCP header
 * @ts:  Optional TCP timestamp data in TCP options
 *
 * This structure relies on the check above that verifies that the header
 * is IPv4 and does not contain any options.
 */
struct txgbe_lrohdr {
	struct iphdr iph;
	struct tcphdr th;
	__be32 ts[0];
};

struct txgbe_lro_list {
	struct sk_buff_head active;
	struct txgbe_lro_stats stats;
};

#endif /* TXGBE_NO_LRO */
#define TXGBE_MAX_TXD_PWR       14
#define TXGBE_MAX_DATA_PER_TXD  (1 << TXGBE_MAX_TXD_PWR)

/* Tx Descriptors needed, worst case */
#define TXD_USE_COUNT(S)        DIV_ROUND_UP((S), TXGBE_MAX_DATA_PER_TXD)
#ifndef MAX_SKB_FRAGS
#define DESC_NEEDED     4
#elif (MAX_SKB_FRAGS < 16)
#define DESC_NEEDED     ((MAX_SKB_FRAGS * TXD_USE_COUNT(PAGE_SIZE)) + 4)
#else
#define DESC_NEEDED     (MAX_SKB_FRAGS + 4)
#endif

/* wrapper around a pointer to a socket buffer,
 * so a DMA handle can be stored along with the buffer */
struct txgbe_tx_buffer {
	union txgbe_tx_desc *next_to_watch;
	unsigned long time_stamp;
	struct sk_buff *skb;
	unsigned int bytecount;
	unsigned short gso_segs;
	__be16 protocol;
	DEFINE_DMA_UNMAP_ADDR(dma);
	DEFINE_DMA_UNMAP_LEN(len);
	u32 tx_flags;
};

struct txgbe_rx_buffer {
	struct sk_buff *skb;
	dma_addr_t dma;
#ifndef CONFIG_TXGBE_DISABLE_PACKET_SPLIT
	dma_addr_t page_dma;
	struct page *page;
	unsigned int page_offset;
#endif
};

struct txgbe_queue_stats {
	u64 packets;
	u64 bytes;
#ifdef BP_EXTENDED_STATS
	u64 yields;
	u64 misses;
	u64 cleaned;
#endif  /* BP_EXTENDED_STATS */
};

struct txgbe_tx_queue_stats {
	u64 restart_queue;
	u64 tx_busy;
	u64 tx_done_old;
};

struct txgbe_rx_queue_stats {
	u64 rsc_count;
	u64 rsc_flush;
	u64 non_eop_descs;
	u64 alloc_rx_page_failed;
	u64 alloc_rx_buff_failed;
	u64 csum_good_cnt;
	u64 csum_err;
};

#define TXGBE_TS_HDR_LEN 8
enum txgbe_ring_state_t {
#ifndef CONFIG_TXGBE_DISABLE_PACKET_SPLIT
	__TXGBE_RX_3K_BUFFER,
	__TXGBE_RX_BUILD_SKB_ENABLED,
#endif
	__TXGBE_TX_FDIR_INIT_DONE,
	__TXGBE_TX_XPS_INIT_DONE,
	__TXGBE_TX_DETECT_HANG,
	__TXGBE_HANG_CHECK_ARMED,
	__TXGBE_RX_HS_ENABLED,
	__TXGBE_RX_RSC_ENABLED,
#if IS_ENABLED(CONFIG_FCOE)
	__TXGBE_RX_FCOE,
#endif
};

struct txgbe_fwd_adapter {
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
	struct net_device *vdev;
	struct txgbe_adapter *adapter;
	unsigned int tx_base_queue;
	unsigned int rx_base_queue;
	int index; /* pool index on PF */
};

#define ring_uses_build_skb(ring) \
	test_bit(__TXGBE_RX_BUILD_SKB_ENABLED, &(ring)->state)

#define ring_is_hs_enabled(ring) \
	test_bit(__TXGBE_RX_HS_ENABLED, &(ring)->state)
#define set_ring_hs_enabled(ring) \
	set_bit(__TXGBE_RX_HS_ENABLED, &(ring)->state)
#define clear_ring_hs_enabled(ring) \
	clear_bit(__TXGBE_RX_HS_ENABLED, &(ring)->state)
#define check_for_tx_hang(ring) \
	test_bit(__TXGBE_TX_DETECT_HANG, &(ring)->state)
#define set_check_for_tx_hang(ring) \
	set_bit(__TXGBE_TX_DETECT_HANG, &(ring)->state)
#define clear_check_for_tx_hang(ring) \
	clear_bit(__TXGBE_TX_DETECT_HANG, &(ring)->state)
#ifndef TXGBE_NO_HW_RSC
#define ring_is_rsc_enabled(ring) \
	test_bit(__TXGBE_RX_RSC_ENABLED, &(ring)->state)
#else
#define ring_is_rsc_enabled(ring)       false
#endif
#define set_ring_rsc_enabled(ring) \
	set_bit(__TXGBE_RX_RSC_ENABLED, &(ring)->state)
#define clear_ring_rsc_enabled(ring) \
	clear_bit(__TXGBE_RX_RSC_ENABLED, &(ring)->state)

struct txgbe_ring {
	struct txgbe_ring *next;        /* pointer to next ring in q_vector */
	struct txgbe_q_vector *q_vector; /* backpointer to host q_vector */
	struct net_device *netdev;      /* netdev ring belongs to */
	struct device *dev;             /* device for DMA mapping */
	struct txgbe_fwd_adapter *accel;
	void *desc;                     /* descriptor ring memory */
	union {
		struct txgbe_tx_buffer *tx_buffer_info;
		struct txgbe_rx_buffer *rx_buffer_info;
	};
	unsigned long state;
	u8 __iomem *tail;
	dma_addr_t dma;                 /* phys. address of descriptor ring */
	unsigned int size;              /* length in bytes */

	u16 count;                      /* amount of descriptors */

	u8 queue_index; /* needed for multiqueue queue management */
	u8 reg_idx;                     /* holds the special value that gets
					 * the hardware register offset
					 * associated with this ring, which is
					 * different for DCB and RSS modes
					 */
	u16 next_to_use;
	u16 next_to_clean;

#ifdef HAVE_PTP_1588_CLOCK
	unsigned long last_rx_timestamp;

#endif
	u16 rx_buf_len;
	union {
#ifndef CONFIG_TXGBE_DISABLE_PACKET_SPLIT
		u16 next_to_alloc;
#endif
		struct {
			u8 atr_sample_rate;
			u8 atr_count;
		};
	};

	u8 dcb_tc;
	struct txgbe_queue_stats stats;
#ifdef HAVE_NDO_GET_STATS64
	struct u64_stats_sync syncp;
#endif
	union {
		struct txgbe_tx_queue_stats tx_stats;
		struct txgbe_rx_queue_stats rx_stats;
	};
} ____cacheline_internodealigned_in_smp;

enum txgbe_ring_f_enum {
	RING_F_NONE = 0,
	RING_F_VMDQ,  /* SR-IOV uses the same ring feature */
	RING_F_RSS,
	RING_F_FDIR,
#if IS_ENABLED(CONFIG_FCOE)
	RING_F_FCOE,
#endif /* CONFIG_FCOE */
	RING_F_ARRAY_SIZE  /* must be last in enum set */
};

#define TXGBE_MAX_DCB_INDICES           8
#define TXGBE_MAX_RSS_INDICES           63
#define TXGBE_MAX_VMDQ_INDICES          64
#define TXGBE_MAX_FDIR_INDICES          63
#if IS_ENABLED(CONFIG_FCOE)
#define TXGBE_MAX_FCOE_INDICES          8
#define MAX_RX_QUEUES   (TXGBE_MAX_FDIR_INDICES + TXGBE_MAX_FCOE_INDICES)
#define MAX_TX_QUEUES   (TXGBE_MAX_FDIR_INDICES + TXGBE_MAX_FCOE_INDICES)
#else
#define MAX_RX_QUEUES   (TXGBE_MAX_FDIR_INDICES + 1)
#define MAX_TX_QUEUES   (TXGBE_MAX_FDIR_INDICES + 1)
#endif /* CONFIG_FCOE */
#define MAX_XDP_QUEUES  (TXGBE_MAX_FDIR_INDICES + 1)

#define TXGBE_MAX_L2A_QUEUES    4
#define TXGBE_BAD_L2A_QUEUE     3

#define TXGBE_MAX_MACVLANS      32
#define TXGBE_MAX_DCBMACVLANS   8

struct txgbe_ring_feature {
	u16 limit;      /* upper limit on feature indices */
	u16 indices;    /* current value of indices */
	u16 mask;       /* Mask used for feature to ring mapping */
	u16 offset;     /* offset to start of feature */
};

#define TXGBE_VMDQ_8Q_MASK 0x78
#define TXGBE_VMDQ_4Q_MASK 0x7C
#define TXGBE_VMDQ_2Q_MASK 0x7E

#ifndef CONFIG_TXGBE_DISABLE_PACKET_SPLIT
/*
 * FCoE requires that all Rx buffers be over 2200 bytes in length.  Since
 * this is twice the size of a half page we need to double the page order
 * for FCoE enabled Rx queues.
 */
static inline unsigned int txgbe_rx_bufsz(struct txgbe_ring __maybe_unused *ring)
{
#if MAX_SKB_FRAGS < 8
	return ALIGN(TXGBE_MAX_RXBUFFER / MAX_SKB_FRAGS, 1024);
#else
#if IS_ENABLED(CONFIG_FCOE)
	if (test_bit(__TXGBE_RX_FCOE, &ring->state))
		return (PAGE_SIZE < 8192) ? TXGBE_RXBUFFER_4K :
					    TXGBE_RXBUFFER_3K;
#endif
	return TXGBE_RXBUFFER_2K;
#endif
}

static inline unsigned int txgbe_rx_pg_order(struct txgbe_ring __maybe_unused *ring)
{
#if IS_ENABLED(CONFIG_FCOE)
	if (test_bit(__TXGBE_RX_FCOE, &ring->state))
		return (PAGE_SIZE < 8192) ? 1 : 0;
#endif
	return 0;
}
#define txgbe_rx_pg_size(_ring) (PAGE_SIZE << txgbe_rx_pg_order(_ring))

#endif
struct txgbe_ring_container {
	struct txgbe_ring *ring;        /* pointer to linked list of rings */
	unsigned int total_bytes;       /* total bytes processed this int */
	unsigned int total_packets;     /* total packets processed this int */
	u16 work_limit;                 /* total work allowed per interrupt */
	u8 count;                       /* total number of rings in vector */
	u8 itr;                         /* current ITR setting for ring */
};

/* iterator for handling rings in ring container */
#define txgbe_for_each_ring(pos, head) \
	for (pos = (head).ring; pos != NULL; pos = pos->next)

#define MAX_RX_PACKET_BUFFERS   ((adapter->flags & TXGBE_FLAG_DCB_ENABLED) \
				 ? 8 : 1)
#define MAX_TX_PACKET_BUFFERS   MAX_RX_PACKET_BUFFERS

/* MAX_MSIX_Q_VECTORS of these are allocated,
 * but we only use one per queue-specific vector.
 */
struct txgbe_q_vector {
	struct txgbe_adapter *adapter;
	int cpu;        /* CPU for DCA */
	u16 v_idx;      /* index of q_vector within array, also used for
			 * finding the bit in EICR and friends that
			 * represents the vector for this ring */
	u16 itr;        /* Interrupt throttle rate written to EITR */
	struct txgbe_ring_container rx, tx;

	struct napi_struct napi;
#ifndef HAVE_NETDEV_NAPI_LIST
	struct net_device poll_dev;
#endif
#ifdef HAVE_IRQ_AFFINITY_HINT
	cpumask_t affinity_mask;
#endif
#ifndef TXGBE_NO_LRO
	struct txgbe_lro_list lrolist;   /* LRO list for queue vector*/
#endif
	int numa_node;
	struct rcu_head rcu;    /* to avoid race with update stats on free */
	char name[IFNAMSIZ + 17];
	bool netpoll_rx;

#ifdef HAVE_NDO_BUSY_POLL
	atomic_t state;
#endif  /* HAVE_NDO_BUSY_POLL */

	/* for dynamic allocation of rings associated with this q_vector */
	struct txgbe_ring ring[0] ____cacheline_internodealigned_in_smp;
};

#ifdef HAVE_NDO_BUSY_POLL
enum txgbe_qv_state_t {
	TXGBE_QV_STATE_IDLE = 0,
	TXGBE_QV_STATE_NAPI,
	TXGBE_QV_STATE_POLL,
	TXGBE_QV_STATE_DISABLE
};

static inline void txgbe_qv_init_lock(struct txgbe_q_vector *q_vector)
{
	/* reset state to idle */
	atomic_set(&q_vector->state, TXGBE_QV_STATE_IDLE);
}

/* called from the device poll routine to get ownership of a q_vector */
static inline bool txgbe_qv_lock_napi(struct txgbe_q_vector *q_vector)
{
	int rc = atomic_cmpxchg(&q_vector->state, TXGBE_QV_STATE_IDLE,
				TXGBE_QV_STATE_NAPI);
#ifdef BP_EXTENDED_STATS
	if (rc != TXGBE_QV_STATE_IDLE)
		q_vector->tx.ring->stats.yields++;
#endif

	return rc == TXGBE_QV_STATE_IDLE;
}

/* returns true is someone tried to get the qv while napi had it */
static inline void txgbe_qv_unlock_napi(struct txgbe_q_vector *q_vector)
{
	WARN_ON(atomic_read(&q_vector->state) != TXGBE_QV_STATE_NAPI);

	/* flush any outstanding Rx frames */
	if (q_vector->napi.gro_list)
		napi_gro_flush(&q_vector->napi, false);

	/* reset state to idle */
	atomic_set(&q_vector->state, TXGBE_QV_STATE_IDLE);
}

/* called from txgbe_low_latency_poll() */
static inline bool txgbe_qv_lock_poll(struct txgbe_q_vector *q_vector)
{
	int rc = atomic_cmpxchg(&q_vector->state, TXGBE_QV_STATE_IDLE,
				TXGBE_QV_STATE_POLL);
#ifdef BP_EXTENDED_STATS
	if (rc != TXGBE_QV_STATE_IDLE)
		q_vector->tx.ring->stats.yields++;
#endif
	return rc == TXGBE_QV_STATE_IDLE;
}

/* returns true if someone tried to get the qv while it was locked */
static inline void txgbe_qv_unlock_poll(struct txgbe_q_vector *q_vector)
{
	WARN_ON(atomic_read(&q_vector->state) != TXGBE_QV_STATE_POLL);

	/* reset state to idle */
	atomic_set(&q_vector->state, TXGBE_QV_STATE_IDLE);
}

/* true if a socket is polling, even if it did not get the lock */
static inline bool txgbe_qv_busy_polling(struct txgbe_q_vector *q_vector)
{
	return atomic_read(&q_vector->state) == TXGBE_QV_STATE_POLL;
}

/* false if QV is currently owned */
static inline bool txgbe_qv_disable(struct txgbe_q_vector *q_vector)
{
	int rc = atomic_cmpxchg(&q_vector->state, TXGBE_QV_STATE_IDLE,
				TXGBE_QV_STATE_DISABLE);

	return rc == TXGBE_QV_STATE_IDLE;
}

#endif /* HAVE_NDO_BUSY_POLL */
#ifdef TXGBE_HWMON

#define TXGBE_HWMON_TYPE_TEMP           0
#define TXGBE_HWMON_TYPE_ALARMTHRESH    1
#define TXGBE_HWMON_TYPE_DALARMTHRESH   2

struct hwmon_attr {
	struct device_attribute dev_attr;
	struct txgbe_hw *hw;
	struct txgbe_thermal_diode_data *sensor;
	char name[19];
};

struct hwmon_buff {
	struct device *device;
	struct hwmon_attr *hwmon_list;
	unsigned int n_hwmon;
};
#endif /* TXGBE_HWMON */

/*
 * microsecond values for various ITR rates shifted by 2 to fit itr register
 * with the first 3 bits reserved 0
 */
#define TXGBE_MIN_RSC_ITR       24
#define TXGBE_100K_ITR          40
#define TXGBE_20K_ITR           200
#define TXGBE_16K_ITR           248
#define TXGBE_12K_ITR           336

/* txgbe_test_staterr - tests bits in Rx descriptor status and error fields */
static inline __le32 txgbe_test_staterr(union txgbe_rx_desc *rx_desc,
					const u32 stat_err_bits)
{
	return rx_desc->wb.upper.status_error & cpu_to_le32(stat_err_bits);
}

/* txgbe_desc_unused - calculate if we have unused descriptors */
static inline u16 txgbe_desc_unused(struct txgbe_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->count) + ntc - ntu - 1;
}

#define TXGBE_RX_DESC(R, i)     \
	(&(((union txgbe_rx_desc *)((R)->desc))[i]))
#define TXGBE_TX_DESC(R, i)     \
	(&(((union txgbe_tx_desc *)((R)->desc))[i]))
#define TXGBE_TX_CTXTDESC(R, i) \
	(&(((struct txgbe_tx_context_desc *)((R)->desc))[i]))

#define TXGBE_MAX_JUMBO_FRAME_SIZE      9432 /* max payload 9414 */
#if IS_ENABLED(CONFIG_FCOE)
/* use 3K as the baby jumbo frame size for FCoE */
#define TXGBE_FCOE_JUMBO_FRAME_SIZE     3072
#endif /* CONFIG_FCOE */

#define TCP_TIMER_VECTOR        0
#define OTHER_VECTOR    1
#define NON_Q_VECTORS   (OTHER_VECTOR + TCP_TIMER_VECTOR)

#define TXGBE_MAX_MSIX_Q_VECTORS_SAPPHIRE       64

struct txgbe_mac_addr {
	u8 addr[ETH_ALEN];
	u16 state; /* bitmask */
	u64 pools;
};

#define TXGBE_MAC_STATE_DEFAULT         0x1
#define TXGBE_MAC_STATE_MODIFIED        0x2
#define TXGBE_MAC_STATE_IN_USE          0x4

#ifdef TXGBE_PROCFS
struct txgbe_therm_proc_data {
	struct txgbe_hw *hw;
	struct txgbe_thermal_diode_data *sensor_data;
};
#endif

/*
 * Only for array allocations in our adapter struct.
 * we can actually assign 64 queue vectors based on our extended-extended
 * interrupt registers.
 */
#define MAX_MSIX_Q_VECTORS      TXGBE_MAX_MSIX_Q_VECTORS_SAPPHIRE
#define MAX_MSIX_COUNT          TXGBE_MAX_MSIX_VECTORS_SAPPHIRE

#define MIN_MSIX_Q_VECTORS      1
#define MIN_MSIX_COUNT          (MIN_MSIX_Q_VECTORS + NON_Q_VECTORS)

/* default to trying for four seconds */
#define TXGBE_TRY_LINK_TIMEOUT  (4 * HZ)
#define TXGBE_SFP_POLL_JIFFIES  (2 * HZ)        /* SFP poll every 2 seconds */

/**
 * txgbe_adapter.flag
 **/
#define TXGBE_FLAG_MSI_CAPABLE                  (u32)(1 << 0)
#define TXGBE_FLAG_MSI_ENABLED                  (u32)(1 << 1)
#define TXGBE_FLAG_MSIX_CAPABLE                 (u32)(1 << 2)
#define TXGBE_FLAG_MSIX_ENABLED                 (u32)(1 << 3)
#ifndef TXGBE_NO_LLI
#define TXGBE_FLAG_LLI_PUSH                     (u32)(1 << 4)
#endif

#define TXGBE_FLAG_TPH_ENABLED                  (u32)(1 << 6)
#define TXGBE_FLAG_TPH_CAPABLE                  (u32)(1 << 7)
#define TXGBE_FLAG_TPH_ENABLED_DATA             (u32)(1 << 8)

#define TXGBE_FLAG_MQ_CAPABLE                   (u32)(1 << 9)
#define TXGBE_FLAG_DCB_ENABLED                  (u32)(1 << 10)
#define TXGBE_FLAG_VMDQ_ENABLED                 (u32)(1 << 11)
#define TXGBE_FLAG_FAN_FAIL_CAPABLE             (u32)(1 << 12)
#define TXGBE_FLAG_NEED_LINK_UPDATE             (u32)(1 << 13)
#define TXGBE_FLAG_NEED_LINK_CONFIG             (u32)(1 << 14)
#define TXGBE_FLAG_FDIR_HASH_CAPABLE            (u32)(1 << 15)
#define TXGBE_FLAG_FDIR_PERFECT_CAPABLE         (u32)(1 << 16)
#if IS_ENABLED(CONFIG_FCOE)
#define TXGBE_FLAG_FCOE_CAPABLE                 (u32)(1 << 17)
#define TXGBE_FLAG_FCOE_ENABLED                 (u32)(1 << 18)
#endif /* CONFIG_FCOE */
#define TXGBE_FLAG_SRIOV_CAPABLE                (u32)(1 << 19)
#define TXGBE_FLAG_SRIOV_ENABLED                (u32)(1 << 20)
#define TXGBE_FLAG_SRIOV_REPLICATION_ENABLE     (u32)(1 << 21)
#define TXGBE_FLAG_SRIOV_L2SWITCH_ENABLE        (u32)(1 << 22)
#define TXGBE_FLAG_SRIOV_VEPA_BRIDGE_MODE       (u32)(1 << 23)
#define TXGBE_FLAG_RX_HWTSTAMP_ENABLED          (u32)(1 << 24)
#define TXGBE_FLAG_VXLAN_OFFLOAD_CAPABLE        (u32)(1 << 25)
#define TXGBE_FLAG_VXLAN_OFFLOAD_ENABLE         (u32)(1 << 26)
#define TXGBE_FLAG_RX_HWTSTAMP_IN_REGISTER      (u32)(1 << 27)
#define TXGBE_FLAG_NEED_ETH_PHY_RESET           (u32)(1 << 28)
#define TXGBE_FLAG_RX_HS_ENABLED                (u32)(1 << 30)
#define TXGBE_FLAG_LINKSEC_ENABLED              (u32)(1 << 31)
#define TXGBE_FLAG_IPSEC_ENABLED                (u32)(1 << 5)

/* preset defaults */
#define TXGBE_FLAGS_SP_INIT (TXGBE_FLAG_MSI_CAPABLE \
			   | TXGBE_FLAG_MSIX_CAPABLE \
			   | TXGBE_FLAG_MQ_CAPABLE \
			   | TXGBE_FLAG_SRIOV_CAPABLE)

/**
 * txgbe_adapter.flag2
 **/
#ifndef TXGBE_NO_HW_RSC
#define TXGBE_FLAG2_RSC_CAPABLE                 (1U << 0)
#define TXGBE_FLAG2_RSC_ENABLED                 (1U << 1)
#else
#define TXGBE_FLAG2_RSC_CAPABLE                 (0U)
#define TXGBE_FLAG2_RSC_ENABLED                 (0U)
#endif
#define TXGBE_FLAG2_TEMP_SENSOR_CAPABLE         (1U << 3)
#define TXGBE_FLAG2_TEMP_SENSOR_EVENT           (1U << 4)
#define TXGBE_FLAG2_SEARCH_FOR_SFP              (1U << 5)
#define TXGBE_FLAG2_SFP_NEEDS_RESET             (1U << 6)
#define TXGBE_FLAG2_PF_RESET_REQUESTED          (1U << 7)
#define TXGBE_FLAG2_FDIR_REQUIRES_REINIT        (1U << 8)
#define TXGBE_FLAG2_RSS_FIELD_IPV4_UDP          (1U << 9)
#define TXGBE_FLAG2_RSS_FIELD_IPV6_UDP          (1U << 10)
#define TXGBE_FLAG2_RSS_ENABLED                 (1U << 12)
#define TXGBE_FLAG2_PTP_PPS_ENABLED             (1U << 11)
#define TXGBE_FLAG2_EEE_CAPABLE                 (1U << 14)
#define TXGBE_FLAG2_EEE_ENABLED                 (1U << 15)
#define TXGBE_FLAG2_VXLAN_REREG_NEEDED          (1U << 16)
#define TXGBE_FLAG2_DEV_RESET_REQUESTED         (1U << 18)
#define TXGBE_FLAG2_RESET_INTR_RECEIVED         (1U << 19)
#define TXGBE_FLAG2_GLOBAL_RESET_REQUESTED      (1U << 20)
#define TXGBE_FLAG2_CLOUD_SWITCH_ENABLED        (1U << 21)
#define TXGBE_FLAG2_MNG_REG_ACCESS_DISABLED     (1U << 22)
#define KR                                      (1U << 23)
#define TXGBE_FLAG2_KR_TRAINING                 (1U << 24)
#define TXGBE_FLAG2_KR_AUTO                     (1U << 25)
#define TXGBE_FLAG2_LINK_DOWN                   (1U << 26)
#define TXGBE_FLAG2_KR_PRO_DOWN                 (1U << 27)
#define TXGBE_FLAG2_KR_PRO_REINIT               (1U << 28)
#define TXGBE_FLAG2_PCIE_NEED_RECOVER           (1U << 31)


#define TXGBE_SET_FLAG(_input, _flag, _result) \
	((_flag <= _result) ? \
	 ((u32)(_input & _flag) * (_result / _flag)) : \
	 ((u32)(_input & _flag) / (_flag / _result)))

enum txgbe_isb_idx {
	TXGBE_ISB_HEADER,
	TXGBE_ISB_MISC,
	TXGBE_ISB_VEC0,
	TXGBE_ISB_VEC1,
	TXGBE_ISB_MAX
};

/* board specific private data structure */
struct txgbe_adapter {
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX) ||\
	defined(NETIF_F_HW_VLAN_STAG_TX)
#ifdef HAVE_VLAN_RX_REGISTER
	struct vlan_group *vlgrp; /* must be first, see txgbe_receive_skb */
#else /* HAVE_VLAN_RX_REGISTER */
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
#endif /* HAVE_VLAN_RX_REGISTER */
#endif
	/* OS defined structs */
	struct net_device *netdev;
	struct pci_dev *pdev;

	unsigned long state;

	/* Some features need tri-state capability,
	 * thus the additional *_CAPABLE flags.
	 */
	u32 flags;
	u32 flags2;
	u8  an73_mode;
	u8  vf_mode;
	u8  backplane_an;
	u8  an73;
	u8  an37;
	u16 ffe_main;
	u16 ffe_pre;
	u16 ffe_post;
	u8  ffe_set;
	u8  backplane_mode;
	u8  backplane_auto;

	bool cloud_mode;

	/* Tx fast path data */
	int num_tx_queues;
	u16 tx_itr_setting;
	u16 tx_work_limit;

	/* Rx fast path data */
	int num_rx_queues;
	u16 rx_itr_setting;
	u16 rx_work_limit;

	unsigned int num_vmdqs; /* does not include pools assigned to VFs */
	unsigned int queues_per_pool;

	/* XDP */
	int num_xdp_queues;
	struct txgbe_ring *xdp_ring[MAX_XDP_QUEUES];

	/* TX */
	struct txgbe_ring *tx_ring[MAX_TX_QUEUES] ____cacheline_aligned_in_smp;

	u64 restart_queue;
	u64 lsc_int;
	u32 tx_timeout_count;

	/* RX */
	struct txgbe_ring *rx_ring[MAX_RX_QUEUES];
	int num_rx_pools; /* does not include pools assigned to VFs */
	int num_rx_queues_per_pool;
	u64 hw_csum_rx_error;
	u64 hw_csum_rx_good;
	u64 hw_rx_no_dma_resources;
	u64 rsc_total_count;
	u64 rsc_total_flush;
	u64 non_eop_descs;
	u32 alloc_rx_page_failed;
	u32 alloc_rx_buff_failed;

	struct txgbe_q_vector *q_vector[MAX_MSIX_Q_VECTORS];

#ifdef HAVE_DCBNL_IEEE
	struct ieee_pfc *txgbe_ieee_pfc;
	struct ieee_ets *txgbe_ieee_ets;
#endif
	struct txgbe_dcb_config dcb_cfg;
	struct txgbe_dcb_config temp_dcb_cfg;
	u8 dcb_set_bitmap;
	u8 dcbx_cap;
#ifndef HAVE_MQPRIO
	u8 dcb_tc;
#endif
	enum txgbe_fc_mode last_lfc_mode;

	int num_q_vectors;      /* current number of q_vectors for device */
	int max_q_vectors;      /* upper limit of q_vectors for device */
	struct txgbe_ring_feature ring_feature[RING_F_ARRAY_SIZE];
	struct msix_entry *msix_entries;

#ifndef HAVE_NETDEV_STATS_IN_NETDEV
	struct net_device_stats net_stats;
#endif
#ifndef TXGBE_NO_LRO
	struct txgbe_lro_stats lro_stats;
#endif

#ifdef ETHTOOL_TEST
	u64 test_icr;
	struct txgbe_ring test_tx_ring;
	struct txgbe_ring test_rx_ring;
#endif

	/* structs defined in txgbe_hw.h */
	struct txgbe_hw hw;
	u16 msg_enable;
	struct txgbe_hw_stats stats;
#ifndef TXGBE_NO_LLI
	u32 lli_port;
	u32 lli_size;
	u32 lli_etype;
	u32 lli_vlan_pri;
#endif /* TXGBE_NO_LLI */

	u32 *config_space;
	u64 tx_busy;
	unsigned int tx_ring_count;
	unsigned int rx_ring_count;

	u32 link_speed;
	bool link_up;
	unsigned long sfp_poll_time;
	unsigned long link_check_timeout;

	struct timer_list service_timer;
	struct work_struct service_task;
#ifdef POLL_LINK_STATUS
	struct timer_list link_check_timer;
#endif
	struct hlist_head fdir_filter_list;
	unsigned long fdir_overflow; /* number of times ATR was backed off */
	union txgbe_atr_input fdir_mask;
	int fdir_filter_count;
	u32 fdir_pballoc;
	u32 atr_sample_rate;
	spinlock_t fdir_perfect_lock;

#if IS_ENABLED(CONFIG_FCOE)
	struct txgbe_fcoe fcoe;
#endif /* CONFIG_FCOE */
	u8 __iomem *io_addr;    /* Mainly for iounmap use */
	u32 wol;

	u16 bd_number;

#ifdef HAVE_BRIDGE_ATTRIBS
	u16 bridge_mode;
#endif

	char eeprom_id[32];
	u16 eeprom_cap;
	bool netdev_registered;
	u32 interrupt_event;
#ifdef HAVE_ETHTOOL_SET_PHYS_ID
	u32 led_reg;
#endif

#ifdef HAVE_PTP_1588_CLOCK
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_caps;
	struct work_struct ptp_tx_work;
	struct sk_buff *ptp_tx_skb;
	struct hwtstamp_config tstamp_config;
	unsigned long ptp_tx_start;
	unsigned long last_overflow_check;
	unsigned long last_rx_ptp_check;
	spinlock_t tmreg_lock;
	struct cyclecounter hw_cc;
	struct timecounter hw_tc;
	u32 base_incval;
	u32 tx_hwtstamp_timeouts;
	u32 tx_hwtstamp_skipped;
	u32 rx_hwtstamp_cleared;
	void (*ptp_setup_sdp) (struct txgbe_adapter *);
#endif /* HAVE_PTP_1588_CLOCK */

	DECLARE_BITMAP(active_vfs, TXGBE_MAX_VF_FUNCTIONS);
	unsigned int num_vfs;
	struct vf_data_storage *vfinfo;
	struct vf_macvlans vf_mvs;
	struct vf_macvlans *mv_list;
#ifdef CONFIG_PCI_IOV
	u32 timer_event_accumulator;
	u32 vferr_refcount;
#endif
	struct txgbe_mac_addr *mac_table;
#if defined(HAVE_UDP_ENC_RX_OFFLOAD) || defined(HAVE_VXLAN_RX_OFFLOAD)
	__le16 vxlan_port;
#endif /* HAVE_UDP_ENC_RX_OFFLAD || HAVE_VXLAN_RX_OFFLOAD */
#ifdef HAVE_UDP_ENC_RX_OFFLOAD
	__le16 geneve_port;
#endif /* HAVE_UDP_ENC_RX_OFFLOAD */
#ifdef TXGBE_SYSFS
#ifdef TXGBE_HWMON
	struct hwmon_buff txgbe_hwmon_buff;
#endif /* TXGBE_HWMON */
#else /* TXGBE_SYSFS */
#ifdef TXGBE_PROCFS
	struct proc_dir_entry *eth_dir;
	struct proc_dir_entry *info_dir;
	u64 old_lsc;
	struct proc_dir_entry *therm_dir;
	struct txgbe_therm_proc_data therm_data;
#endif /* TXGBE_PROCFS */
#endif /* TXGBE_SYSFS */

#ifdef HAVE_TXGBE_DEBUG_FS
	struct dentry *txgbe_dbg_adapter;
#endif /*HAVE_TXGBE_DEBUG_FS*/
	u8 default_up;
#ifdef HAVE_TX_MQ
#ifndef HAVE_NETDEV_SELECT_QUEUE
	unsigned int indices;
#endif /* !HAVE_NETDEV_SELECT_QUEUE*/
#endif /* HAVE_TX_MQ */
	unsigned long fwd_bitmask; /* bitmask indicating in use pools */
	unsigned long tx_timeout_last_recovery;
	u32 tx_timeout_recovery_level;

#define TXGBE_MAX_RETA_ENTRIES 128
	u8 rss_indir_tbl[TXGBE_MAX_RETA_ENTRIES];
#define TXGBE_RSS_KEY_SIZE     40
	u32 rss_key[TXGBE_RSS_KEY_SIZE / sizeof(u32)];

	void *ipsec;

	/* misc interrupt status block */
	dma_addr_t isb_dma;
	u32 *isb_mem;
	u32 isb_tag[TXGBE_ISB_MAX];
};

static inline u32 txgbe_misc_isb(struct txgbe_adapter *adapter,
				 enum txgbe_isb_idx idx)
{
	u32 cur_tag = 0;
	u32 cur_diff = 0;
    
    cur_tag = adapter->isb_mem[TXGBE_ISB_HEADER];
    cur_diff = cur_tag - adapter->isb_tag[idx];

	adapter->isb_tag[idx] = cur_tag;

	return adapter->isb_mem[idx];
}

static inline u8 txgbe_max_rss_indices(struct txgbe_adapter *adapter)
{
	return TXGBE_MAX_RSS_INDICES;
}

struct txgbe_fdir_filter {
	struct  hlist_node fdir_node;
	union txgbe_atr_input filter;
	u16 sw_idx;
	u16 action;
};

enum txgbe_state_t {
	__TXGBE_TESTING,
	__TXGBE_RESETTING,
	__TXGBE_DOWN,
	__TXGBE_HANGING,
	__TXGBE_DISABLED,
	__TXGBE_REMOVING,
	__TXGBE_SERVICE_SCHED,
	__TXGBE_SERVICE_INITED,
	__TXGBE_IN_SFP_INIT,
#ifdef HAVE_PTP_1588_CLOCK
	__TXGBE_PTP_RUNNING,
	__TXGBE_PTP_TX_IN_PROGRESS,
#endif
};

struct txgbe_cb {
#ifdef CONFIG_TXGBE_DISABLE_PACKET_SPLIT
	union {                         /* Union defining head/tail partner */
		struct sk_buff *head;
		struct sk_buff *tail;
	};
#endif
	dma_addr_t dma;
#ifndef TXGBE_NO_LRO
	__be32  tsecr;                  /* timestamp echo response */
	u32     tsval;                  /* timestamp value in host order */
	u32     next_seq;               /* next expected sequence number */
	u16     free;                   /* 65521 minus total size */
	u16     mss;                    /* size of data portion of packet */
#endif /* TXGBE_NO_LRO */
#ifdef HAVE_VLAN_RX_REGISTER
	u16     vid;                    /* VLAN tag */
#endif
	u16     append_cnt;             /* number of skb's appended */
#ifndef CONFIG_TXGBE_DISABLE_PACKET_SPLIT
	bool    page_released;
	bool    dma_released;
#endif
};
#define TXGBE_CB(skb) ((struct txgbe_cb *)(skb)->cb)

/* ESX txgbe CIM IOCTL definition */

#ifdef TXGBE_SYSFS
void txgbe_sysfs_exit(struct txgbe_adapter *adapter);
int txgbe_sysfs_init(struct txgbe_adapter *adapter);
#endif /* TXGBE_SYSFS */
#ifdef TXGBE_PROCFS
void txgbe_procfs_exit(struct txgbe_adapter *adapter);
int txgbe_procfs_init(struct txgbe_adapter *adapter);
int txgbe_procfs_topdir_init(void);
void txgbe_procfs_topdir_exit(void);
#endif /* TXGBE_PROCFS */

extern struct dcbnl_rtnl_ops dcbnl_ops;
int txgbe_copy_dcb_cfg(struct txgbe_adapter *adapter, int tc_max);

u8 txgbe_dcb_txq_to_tc(struct txgbe_adapter *adapter, u8 index);

/* needed by txgbe_main.c */
int txgbe_validate_mac_addr(u8 *mc_addr);
void txgbe_check_options(struct txgbe_adapter *adapter);
void txgbe_assign_netdev_ops(struct net_device *netdev);

/* needed by txgbe_ethtool.c */
extern char txgbe_driver_name[];
extern const char txgbe_driver_version[];

void txgbe_irq_disable(struct txgbe_adapter *adapter);
void txgbe_irq_enable(struct txgbe_adapter *adapter, bool queues, bool flush);
int txgbe_open(struct net_device *netdev);
int txgbe_close(struct net_device *netdev);
void txgbe_up(struct txgbe_adapter *adapter);
void txgbe_down(struct txgbe_adapter *adapter);
void txgbe_reinit_locked(struct txgbe_adapter *adapter);
void txgbe_reset(struct txgbe_adapter *adapter);
void txgbe_set_ethtool_ops(struct net_device *netdev);
int txgbe_setup_rx_resources(struct txgbe_ring *);
int txgbe_setup_tx_resources(struct txgbe_ring *);
void txgbe_free_rx_resources(struct txgbe_ring *);
void txgbe_free_tx_resources(struct txgbe_ring *);
void txgbe_configure_rx_ring(struct txgbe_adapter *,
				    struct txgbe_ring *);
void txgbe_configure_tx_ring(struct txgbe_adapter *,
				    struct txgbe_ring *);
void txgbe_update_stats(struct txgbe_adapter *adapter);
int txgbe_init_interrupt_scheme(struct txgbe_adapter *adapter);
void txgbe_reset_interrupt_capability(struct txgbe_adapter *adapter);
void txgbe_set_interrupt_capability(struct txgbe_adapter *adapter);
void txgbe_clear_interrupt_scheme(struct txgbe_adapter *adapter);
bool txgbe_is_txgbe(struct pci_dev *pcidev);
netdev_tx_t txgbe_xmit_frame_ring(struct sk_buff *,
					 struct txgbe_adapter *,
					 struct txgbe_ring *);
void txgbe_unmap_and_free_tx_resource(struct txgbe_ring *,
					     struct txgbe_tx_buffer *);
void txgbe_alloc_rx_buffers(struct txgbe_ring *, u16);
void txgbe_configure_rscctl(struct txgbe_adapter *adapter,
				   struct txgbe_ring *);
void txgbe_clear_rscctl(struct txgbe_adapter *adapter,
			       struct txgbe_ring *);
void txgbe_clear_vxlan_port(struct txgbe_adapter *);
void txgbe_set_rx_mode(struct net_device *netdev);
int txgbe_write_mc_addr_list(struct net_device *netdev);
int txgbe_setup_tc(struct net_device *dev, u8 tc);
void txgbe_tx_ctxtdesc(struct txgbe_ring *, u32, u32, u32, u32);
void txgbe_do_reset(struct net_device *netdev);
void txgbe_write_eitr(struct txgbe_q_vector *q_vector);
int txgbe_poll(struct napi_struct *napi, int budget);
void txgbe_disable_rx_queue(struct txgbe_adapter *adapter,
				   struct txgbe_ring *);
void txgbe_vlan_strip_enable(struct txgbe_adapter *adapter);
void txgbe_vlan_strip_disable(struct txgbe_adapter *adapter);
#ifdef ETHTOOL_OPS_COMPAT
int ethtool_ioctl(struct ifreq *ifr);
#endif

#if IS_ENABLED(CONFIG_FCOE)
void txgbe_configure_fcoe(struct txgbe_adapter *adapter);
int txgbe_fso(struct txgbe_ring *tx_ring,
		     struct txgbe_tx_buffer *first,
		     u8 *hdr_len);
int txgbe_fcoe_ddp(struct txgbe_adapter *adapter,
			  union txgbe_rx_desc *rx_desc,
			  struct sk_buff *skb);
int txgbe_fcoe_ddp_get(struct net_device *netdev, u16 xid,
			      struct scatterlist *sgl, unsigned int sgc);
#ifdef HAVE_NETDEV_OPS_FCOE_DDP_TARGET
int txgbe_fcoe_ddp_target(struct net_device *netdev, u16 xid,
				 struct scatterlist *sgl, unsigned int sgc);
#endif /* HAVE_NETDEV_OPS_FCOE_DDP_TARGET */
int txgbe_fcoe_ddp_put(struct net_device *netdev, u16 xid);
int txgbe_setup_fcoe_ddp_resources(struct txgbe_adapter *adapter);
void txgbe_free_fcoe_ddp_resources(struct txgbe_adapter *adapter);
#ifdef HAVE_NETDEV_OPS_FCOE_ENABLE
int txgbe_fcoe_enable(struct net_device *netdev);
int txgbe_fcoe_disable(struct net_device *netdev);
#else
int txgbe_fcoe_ddp_enable(struct txgbe_adapter *adapter);
void txgbe_fcoe_ddp_disable(struct txgbe_adapter *adapter);
#endif /* HAVE_NETDEV_OPS_FCOE_ENABLE */
#if IS_ENABLED(CONFIG_DCB)
#ifdef HAVE_DCBNL_OPS_GETAPP
u8 txgbe_fcoe_getapp(struct net_device *netdev);
#endif /* HAVE_DCBNL_OPS_GETAPP */
u8 txgbe_fcoe_setapp(struct txgbe_adapter *adapter, u8 up);
#endif /* CONFIG_DCB */
u8 txgbe_fcoe_get_tc(struct txgbe_adapter *adapter);
#ifdef HAVE_NETDEV_OPS_FCOE_GETWWN
int txgbe_fcoe_get_wwn(struct net_device *netdev, u64 *wwn, int type);
#endif
#endif /* CONFIG_FCOE */

#ifdef HAVE_TXGBE_DEBUG_FS
void txgbe_dbg_adapter_init(struct txgbe_adapter *adapter);
void txgbe_dbg_adapter_exit(struct txgbe_adapter *adapter);
void txgbe_dbg_init(void);
void txgbe_dbg_exit(void);
#endif /* HAVE_TXGBE_DEBUG_FS */
void txgbe_dump(struct txgbe_adapter *adapter);

static inline struct netdev_queue *txring_txq(const struct txgbe_ring *ring)
{
	return netdev_get_tx_queue(ring->netdev, ring->queue_index);
}

#if IS_ENABLED(CONFIG_DCB)
#ifdef HAVE_DCBNL_IEEE
s32 txgbe_dcb_hw_ets(struct txgbe_hw *hw, struct ieee_ets *ets, int max_frame);
#endif /* HAVE_DCBNL_IEEE */
#endif /* CONFIG_DCB */

int txgbe_wol_supported(struct txgbe_adapter *adapter);
int txgbe_get_settings(struct net_device *netdev,
			      struct ethtool_cmd *ecmd);
int txgbe_write_uc_addr_list(struct net_device *netdev, int pool);
void txgbe_full_sync_mac_table(struct txgbe_adapter *adapter);
int txgbe_add_mac_filter(struct txgbe_adapter *adapter,
				u8 *addr, u16 pool);
int txgbe_del_mac_filter(struct txgbe_adapter *adapter,
				u8 *addr, u16 pool);
int txgbe_available_rars(struct txgbe_adapter *adapter);
#ifndef HAVE_VLAN_RX_REGISTER
void txgbe_vlan_mode(struct net_device *, u32);
#endif

#ifdef HAVE_PTP_1588_CLOCK
void txgbe_ptp_init(struct txgbe_adapter *adapter);
void txgbe_ptp_stop(struct txgbe_adapter *adapter);
void txgbe_ptp_suspend(struct txgbe_adapter *adapter);
void txgbe_ptp_overflow_check(struct txgbe_adapter *adapter);
void txgbe_ptp_rx_hang(struct txgbe_adapter *adapter);
void txgbe_ptp_rx_hwtstamp(struct txgbe_adapter *adapter, struct sk_buff *skb);
int txgbe_ptp_set_ts_config(struct txgbe_adapter *adapter, struct ifreq *ifr);
int txgbe_ptp_get_ts_config(struct txgbe_adapter *adapter, struct ifreq *ifr);
void txgbe_ptp_start_cyclecounter(struct txgbe_adapter *adapter);
void txgbe_ptp_reset(struct txgbe_adapter *adapter);
void txgbe_ptp_check_pps_event(struct txgbe_adapter *adapter);
#endif /* HAVE_PTP_1588_CLOCK */
#ifdef CONFIG_PCI_IOV
void txgbe_sriov_reinit(struct txgbe_adapter *adapter);
#endif

void txgbe_set_rx_drop_en(struct txgbe_adapter *adapter);

u32 txgbe_rss_indir_tbl_entries(struct txgbe_adapter *adapter);
void txgbe_store_reta(struct txgbe_adapter *adapter);

/**
 * interrupt masking operations. each bit in PX_ICn correspond to a interrupt.
 * disable a interrupt by writing to PX_IMS with the corresponding bit=1
 * enable a interrupt by writing to PX_IMC with the corresponding bit=1
 * trigger a interrupt by writing to PX_ICS with the corresponding bit=1
 **/
#define TXGBE_INTR_ALL (~0ULL)
#define TXGBE_INTR_MISC(A) (1ULL << (A)->num_q_vectors)
#define TXGBE_INTR_QALL(A) (TXGBE_INTR_MISC(A) - 1)
#define TXGBE_INTR_Q(i) (1ULL << (i))
static inline void txgbe_intr_enable(struct txgbe_hw *hw, u64 qmask)
{
	u32 mask;

	mask = (qmask & 0xFFFFFFFF);
	if (mask)
		wr32(hw, TXGBE_PX_IMC(0), mask);
	mask = (qmask >> 32);
	if (mask)
		wr32(hw, TXGBE_PX_IMC(1), mask);

	/* skip the flush */
}

static inline void txgbe_intr_disable(struct txgbe_hw *hw, u64 qmask)
{
	u32 mask;

	mask = (qmask & 0xFFFFFFFF);
	if (mask)
		wr32(hw, TXGBE_PX_IMS(0), mask);
	mask = (qmask >> 32);
	if (mask)
		wr32(hw, TXGBE_PX_IMS(1), mask);

	/* skip the flush */
}

static inline void txgbe_intr_trigger(struct txgbe_hw *hw, u64 qmask)
{
	u32 mask;

	mask = (qmask & 0xFFFFFFFF);
	if (mask)
		wr32(hw, TXGBE_PX_ICS(0), mask);
	mask = (qmask >> 32);
	if (mask)
		wr32(hw, TXGBE_PX_ICS(1), mask);

	/* skip the flush */
}

#define TXGBE_RING_SIZE(R) ((R)->count < TXGBE_MAX_TXD ? (R)->count / 128 : 0)

#endif /* _TXGBE_H_ */
