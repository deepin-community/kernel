/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2008 - 2024 Xel Technology. */

#ifndef _XLNID_H_
#define _XLNID_H_

#include <net/ip.h>

#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/aer.h>
#include <linux/mutex.h>
#ifdef SIOCETHTOOL
#include <linux/ethtool.h>
#endif
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
#include <linux/if_vlan.h>
#endif
/* Can't use IS_ENABLED until after kcompat is loaded */

#include "kcompat.h"

#ifdef HAVE_XDP_BUFF_RXQ
#include <net/xdp.h>
#endif

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

#include "xlnid_api.h"

#if IS_ENABLED(CONFIG_NET_DEVLINK)
#include <net/devlink.h>
#endif /* CONFIG_NET_DEVLINK */

#include "xlnid_common.h"

#define DPRINTK(nlevel, klevel, fmt, args...) \
	((NETIF_MSG_##nlevel & adapter->msg_enable) ? \
	(void)(netdev_printk(KERN_##klevel, adapter->netdev, \
	"%s: " fmt, __func__, ## args)) : NULL)

#ifdef HAVE_PTP_1588_CLOCK
#ifdef HAVE_INCLUDE_LINUX_TIMECOUNTER_H
#include <linux/timecounter.h>
#endif /* HAVE_INCLUDE_TIMECOUNTER_H */
#include <linux/clocksource.h>
#include <linux/net_tstamp.h>
#include <linux/ptp_clock_kernel.h>
#endif

#ifndef XLNID_SPI_CONFIG
#ifdef XLNID_SPI_FLASH
/* BYTE_ADDR[0:2]=2+1, TESS[4:15]=500 */
#define XLNID_SPI_CONFIG  0x1f42
#else
/* BYTE_ADDR[0:2]=1+1, TESS[4:15]=400 */
#define XLNID_SPI_CONFIG  0x1901
#endif /* XLNID_SPI_FLASH */
#endif /* XLNID_SPI_CONFIG */


#define XLNID_SPI_CONFIG_BYTE_ADDR (((XLNID_SPI_CONFIG) & 0x7) + 1)
/* SPI FLash Erase subsector cycle time msec */
#define XLNID_SPI_CONFIG_TESS  (((XLNID_SPI_CONFIG) >> 4) & 0xfff)

/* TX/RX descriptor defines */
#define XLNID_DEFAULT_TXD       512
#define XLNID_DEFAULT_TX_WORK       256

#define XLNID_DEFAULT_RXD       512

#define XLNID_MAX_NUM_DESCRIPTORS   4096
#define XLNID_MIN_NUM_DESCRIPTORS   64

#define XLNID_ETH_P_LLDP        0x88CC


/* flow control */
#define XLNID_MIN_FCRTL         0x40
#define XLNID_MAX_FCRTL         0x7FF80
#define XLNID_MIN_FCRTH         0x600
#define XLNID_MAX_FCRTH         0x7FFF0
#define XLNID_DEFAULT_FCPAUSE       0xFFFF
#define XLNID_MIN_FCPAUSE       0
#define XLNID_MAX_FCPAUSE       0xFFFF

/* Supported Rx Buffer Sizes */
#define XLNID_RXBUFFER_256       256    /* Used for skb receive header */
#define XLNID_RXBUFFER_1536 1536
#define XLNID_RXBUFFER_2K   2048
#define XLNID_RXBUFFER_3K   3072
#define XLNID_RXBUFFER_4K   4096
#define XLNID_RXBUFFER_8K   8192
#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
#define XLNID_RXBUFFER_7K   7168
#define XLNID_RXBUFFER_15K  15360
#endif /* CONFIG_XLNID_DISABLE_PACKET_SPLIT */
#define XLNID_MAX_RXBUFFER  16384   /* largest size for single descriptor */

/*  Attempt to maximize the headroom available for incoming frames.  We
		use a 2K buffer for receives and need 1536/1534 to store the data for
		the frame.  This leaves us with 512 bytes of room.  From that we need
		to deduct the space needed for the shared info and the padding needed
		to IP align the frame.

		Note: For cache line sizes 256 or larger this value is going to end
			up negative.  In these cases we should fall back to the 3K
			buffers.
*/
#if (PAGE_SIZE < 8192)
#define XLNID_MAX_2K_FRAME_BUILD_SKB (XLNID_RXBUFFER_1536 - NET_IP_ALIGN)
#define XLNID_2K_TOO_SMALL_WITH_PADDING \
((NET_SKB_PAD + XLNID_RXBUFFER_1536) > SKB_WITH_OVERHEAD(XLNID_RXBUFFER_2K))

static inline int xlnid_compute_pad(int rx_buf_len)
{
	int page_size, pad_size;

	page_size = ALIGN(rx_buf_len, PAGE_SIZE / 2);
	pad_size = SKB_WITH_OVERHEAD(page_size) - rx_buf_len;

	return pad_size;
}

static inline int xlnid_skb_pad(void)
{
	int rx_buf_len;

	/*  If a 2K buffer cannot handle a standard Ethernet frame then
		optimize padding for a 3K buffer instead of a 1.5K buffer.

		For a 3K buffer we need to add enough padding to allow for
		tailroom due to NET_IP_ALIGN possibly shifting us out of
		cache-line alignment.
	*/

	if (XLNID_2K_TOO_SMALL_WITH_PADDING) {
		rx_buf_len = XLNID_RXBUFFER_3K + SKB_DATA_ALIGN(NET_IP_ALIGN);
	}

	else {
		rx_buf_len = XLNID_RXBUFFER_1536;
	}

	/* if needed make room for NET_IP_ALIGN */
	rx_buf_len -= NET_IP_ALIGN;

	return xlnid_compute_pad(rx_buf_len);
}

#define XLNID_SKB_PAD   xlnid_skb_pad()
#else
#define XLNID_SKB_PAD   (NET_SKB_PAD + NET_IP_ALIGN)
#endif

/*
		NOTE: netdev_alloc_skb reserves up to 64 bytes, NET_IP_ALIGN means we
		reserve 64 more, and skb_shared_info adds an additional 320 bytes more,
		this adds up to 448 bytes of extra data.

		Since netdev_alloc_skb now allocates a page fragment we can use a value
		of 256 and the resultant skb will have a truesize of 960 or less.
*/
#define XLNID_RX_HDR_SIZE   XLNID_RXBUFFER_256

#define MAXIMUM_ETHERNET_VLAN_SIZE  (VLAN_ETH_FRAME_LEN + ETH_FCS_LEN)

/* How many Rx Buffers do we bundle into one write to the hardware ? */
#define XLNID_RX_BUFFER_WRITE   16  /* Must be power of 2 */

#ifdef HAVE_STRUCT_DMA_ATTRS
#define XLNID_RX_DMA_ATTR NULL
#else
#define XLNID_RX_DMA_ATTR \
	(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)
#endif

/* assume the kernel supports 8021p to avoid stripping vlan tags */
#ifdef XLNID_DISABLE_8021P_SUPPORT
#ifndef HAVE_8021P_SUPPORT
#define HAVE_8021P_SUPPORT
#endif
#endif /* XLNID_DISABLE_8021P_SUPPORT */

#define XLNID_TX_FLAGS_HW_VLAN          BIT(1)
#define XLNID_TX_FLAGS_SW_VLAN          BIT(2)
#define XLNID_TX_FLAGS_TSO              BIT(3)
#define XLNID_TX_FLAGS_IPV4             BIT(4)
#define XLNID_TX_FLAGS_IPV6             BIT(5)
#define XLNID_TX_FLAGS_CC               BIT(6)
#define XLNID_TX_FLAGS_FSO              BIT(7)
#define XLNID_TX_FLAGS_TSTAMP           BIT(8)
#define XLNID_TX_FLAGS_FD_SB            BIT(9)
#define XLNID_TX_FLAGS_TUNNEL           BIT(10)
#define XLNID_TX_FLAGS_HW_OUTER_VLAN    BIT(11)
#define XLNID_TX_FLAGS_CSUM             BIT(12)

/* VLAN info */
#define XLNID_TX_FLAGS_VLAN_MASK    0xffff0000
#define XLNID_TX_FLAGS_VLAN_PRIO_MASK   0xe0000000
#define XLNID_TX_FLAGS_VLAN_PRIO_SHIFT  29
#define XLNID_TX_FLAGS_VLAN_SHIFT   16

#define XLNID_MAX_RX_DESC_POLL      10

#define XLNID_MAX_VF_MC_ENTRIES     30
#define XLNID_MAX_VF_FUNCTIONS      64
#define XLNID_MAX_VFTA_ENTRIES      128
#define MAX_EMULATION_MAC_ADDRS     16
#define XLNID_MAX_PF_MACVLANS       15

/* must account for pools assigned to VFs. */
#if 0
#define VMDQ_P(p)   ((p) + adapter->ring_feature[RING_F_VMDQ].offset)
#else
#define VMDQ_P(p)   (p)
#endif

#define UPDATE_VF_COUNTER_32bit(reg, last_counter, counter) \
	{                           \
		u32 current_counter = XLNID_READ_REG(hw, reg);  \
		if (current_counter < last_counter)     \
			counter += 0x100000000LL;       \
		last_counter = current_counter;         \
		counter &= 0xFFFFFFFF00000000LL;        \
		counter |= current_counter;         \
	}

#define UPDATE_VF_COUNTER_36bit(reg_lsb, reg_msb, last_counter, counter) \
	{                                \
		u64 current_counter_lsb = XLNID_READ_REG(hw, reg_lsb);   \
		u64 current_counter_msb = XLNID_READ_REG(hw, reg_msb);   \
		u64 current_counter = (current_counter_msb << 32) |  \
			current_counter_lsb;                 \
		if (current_counter < last_counter)          \
			counter += 0x1000000000LL;           \
		last_counter = current_counter;              \
		counter &= 0xFFFFFFF000000000LL;             \
		counter |= current_counter;              \
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
	unsigned char vf_mac_addresses[ETH_ALEN];
	u16 vf_mc_hashes[XLNID_MAX_VF_MC_ENTRIES];
	u16 num_vf_mc_hashes;
	bool clear_to_send;
	struct vf_stats vfstats;
	struct vf_stats last_vfstats;
	struct vf_stats saved_rst_vfstats;
	bool pf_set_mac;
	u16 pf_vlan; /* When set, guest VLAN config not allowed. */
	u16 pf_qos;
	u16 tx_rate;
	int link_enable;
#ifdef HAVE_NDO_SET_VF_LINK_STATE
	int link_state;
#endif
	u8 spoofchk_enabled;
#ifdef HAVE_NDO_SET_VF_RSS_QUERY_EN
	bool rss_query_enabled;
#endif
	u8 trusted;
	int xcast_mode;
	unsigned int vf_api;
	u8 primary_abort_count;
};

struct vf_macvlans {
	struct list_head l;
	int vf;
	bool free;
	bool is_macvlan;
	u8 vf_macvlan[ETH_ALEN];
};

#define XLNID_MAX_TXD_PWR   14
#define XLNID_MAX_DATA_PER_TXD  (1 << XLNID_MAX_TXD_PWR)

/* Tx Descriptors needed, worst case */
#define TXD_USE_COUNT(S)    DIV_ROUND_UP((S), XLNID_MAX_DATA_PER_TXD)
#ifndef MAX_SKB_FRAGS
#define DESC_NEEDED 4
#elif (MAX_SKB_FRAGS < 16)
#define DESC_NEEDED ((MAX_SKB_FRAGS * TXD_USE_COUNT(PAGE_SIZE)) + 4)
#else
#define DESC_NEEDED (MAX_SKB_FRAGS + 4)
#endif

/*  wrapper around a pointer to a socket buffer,
		so a DMA handle can be stored along with the buffer */
struct xlnid_tx_buffer {
	void *next_to_watch;
	unsigned long time_stamp;
	union {
		struct sk_buff *skb;
		struct xdp_frame *xdpf;
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

struct xlnid_rx_buffer {
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
	struct sk_buff *skb;
	dma_addr_t dma;
#endif
#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	union {
		struct {
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
			struct sk_buff *skb;
			dma_addr_t dma;
#endif
			struct page *page;
			__u32 page_offset;
			__u16 pagecnt_bias;
		};
#ifdef HAVE_AF_XDP_ZC_SUPPORT
		struct {
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
			void *addr;
			u64 handle;
#else
			bool discard;
			struct xdp_buff *xdp;
#endif
		};
#endif
	};
#endif /* CONFIG_XLNID_DISABLE_PACKET_SPLIT */
};

struct xlnid_queue_stats {
	u64 packets;
	u64 bytes;
#ifdef BP_EXTENDED_STATS
	u64 yields;
	u64 misses;
	u64 cleaned;
#endif  /* BP_EXTENDED_STATS */
};

struct xlnid_tx_queue_stats {
	u64 restart_queue;
	u64 tx_busy;
	u64 tx_done_old;
};

struct xlnid_rx_queue_stats {
	u64 rsc_count;
	u64 rsc_flush;
	u64 non_eop_descs;
	u64 alloc_rx_page;
	u64 alloc_rx_page_failed;
	u64 alloc_rx_buff_failed;
	u64 csum_err;
	u64 crc_err;
};

#define XLNID_TS_HDR_LEN 8

enum xlnid_ring_state_t {
#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	__XLNID_RX_3K_BUFFER,
	__XLNID_RX_BUILD_SKB_ENABLED,
#endif
	__XLNID_RX_RSC_ENABLED,
	__XLNID_RX_CSUM_UDP_ZERO_ERR,
	__XLNID_TX_FDIR_INIT_DONE,
	__XLNID_TX_XPS_INIT_DONE,
	__XLNID_TX_DETECT_HANG,
	__XLNID_HANG_CHECK_ARMED,
	__XLNID_TX_XDP_RING,
#ifdef HAVE_AF_XDP_ZC_SUPPORT
	__XLNID_TX_DISABLED,
#endif
};

#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT

#define ring_uses_build_skb(ring) \
	test_bit(__XLNID_RX_BUILD_SKB_ENABLED, &(ring)->state)
#endif

#define check_for_tx_hang(ring) \
	test_bit(__XLNID_TX_DETECT_HANG, &(ring)->state)

#define set_check_for_tx_hang(ring) \
	set_bit(__XLNID_TX_DETECT_HANG, &(ring)->state)

#define clear_check_for_tx_hang(ring) \
	clear_bit(__XLNID_TX_DETECT_HANG, &(ring)->state)

#define ring_is_rsc_enabled(ring) \
	test_bit(__XLNID_RX_RSC_ENABLED, &(ring)->state)

#define set_ring_rsc_enabled(ring) \
	set_bit(__XLNID_RX_RSC_ENABLED, &(ring)->state)

#define clear_ring_rsc_enabled(ring) \
	clear_bit(__XLNID_RX_RSC_ENABLED, &(ring)->state)

#define ring_is_xdp(ring) \
	test_bit(__XLNID_TX_XDP_RING, &(ring)->state)

#define set_ring_xdp(ring) \
	set_bit(__XLNID_TX_XDP_RING, &(ring)->state)

#define clear_ring_xdp(ring) \
	clear_bit(__XLNID_TX_XDP_RING, &(ring)->state)
#define netdev_ring(ring) (ring->netdev)
#define ring_queue_index(ring) (ring->queue_index)

#ifdef XLNID_TEST_XL84101

struct xlnid_ring_stats {
#ifdef HAVE_NDO_GET_STATS64
	struct u64_stats_sync syncp;
#endif
	struct xlnid_queue_stats stats;
};
#endif

struct xlnid_ring {
	struct xlnid_ring *next;    /* pointer to next ring in q_vector */
	struct xlnid_q_vector *q_vector;    /* backpointer to host q_vector */
	struct net_device *netdev;  /* netdev ring belongs to */
	struct bpf_prog *xdp_prog;
	struct device *dev;     /* device for DMA mapping */
	void *desc;         /* descriptor ring memory */
	union {
		struct xlnid_tx_buffer *tx_buffer_info;
		struct xlnid_rx_buffer *rx_buffer_info;
	};
	unsigned long state;
	u8 __iomem *tail;
	dma_addr_t dma;         /* phys. address of descriptor ring */
	unsigned int size;      /* length in bytes */

	u16 count;          /* amount of descriptors */

	u8 queue_index; /* needed for multiqueue queue management */
	u8 reg_idx;         /* holds the special value that gets
						the hardware register offset
						associated with this ring, which is
						different for DCB and RSS modes
*/
	u16 next_to_use;
	u16 next_to_clean;

#ifdef HAVE_PTP_1588_CLOCK
	unsigned long last_rx_timestamp;

#endif
	union {
#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
		u16 rx_buf_len;
#else
		union {
			u16 next_to_alloc;
			u16 next_rs_idx;
		};
#endif
		struct {
			u8 atr_sample_rate;
			u8 atr_count;
		};
	};

	u16 flags;
#define XLNID_TXR_FLAGS_L2TAG2  BIT(3)
	struct sk_buff *skb;

#ifdef HAVE_XDP_SUPPORT
#ifdef HAVE_AF_XDP_ZC_SUPPORT
	u16 xdp_tx_active;
#endif /* HAVE_AF_XDP_ZC_SUPPORT */
#endif /* HAVE_XDP_SUPPORT */

	u8 dcb_tc;
	struct xlnid_queue_stats stats;
#ifdef HAVE_NDO_GET_STATS64
	struct u64_stats_sync syncp;
#endif
	union {
		struct xlnid_tx_queue_stats tx_stats;
		struct xlnid_rx_queue_stats rx_stats;
	};
	u16 rx_offset;
	spinlock_t tx_lock;     /* used in XDP mode */
#ifdef HAVE_XDP_BUFF_RXQ
	struct xdp_rxq_info xdp_rxq;
#ifdef HAVE_AF_XDP_ZC_SUPPORT
#ifdef HAVE_NETDEV_BPF_XSK_POOL
	struct xsk_buff_pool *xsk_pool;
#else
	struct xdp_umem *xsk_pool;
#endif
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
	struct zero_copy_allocator zca; /* ZC allocator anchor */
#endif
	u16 ring_idx;           /* {rx, tx, xdp}_ring back reference idx */
	u16 rx_buf_len;
#endif
#endif
} ____cacheline_internodealigned_in_smp;

enum xlnid_ring_f_enum {
	RING_F_NONE = 0,
	RING_F_VMDQ,  /* SR-IOV uses the same ring feature */
	RING_F_RSS,
	RING_F_FDIR,
	RING_F_ARRAY_SIZE  /* must be last in enum set */
};

#define XLNID_MAX_DCB_INDICES       8
#define XLNID_MAX_RSS_INDICES       8
#define XLNID_MAX_VMDQ_INDICES      64
#define XLNID_MAX_FDIR_INDICES      7

#define MAX_RX_QUEUES   (XLNID_MAX_FDIR_INDICES + 1)
#define MAX_TX_QUEUES   (XLNID_MAX_FDIR_INDICES + 1)
#define MAX_XDP_QUEUES  (XLNID_MAX_FDIR_INDICES + 1)

#define WESTLAKE_MAX_FDIR_INDICES   7

#define WESTLAKE_MAX_RX_QUEUES      (WESTLAKE_MAX_FDIR_INDICES + 1)
#define WESTLAKE_MAX_TX_QUEUES      (WESTLAKE_MAX_FDIR_INDICES + 1)
#define WESTLAKE_MAX_XDP_QUEUES     (WESTLAKE_MAX_FDIR_INDICES + 1)

#define LODESTAR_MAX_FDIR_INDICES   15
#define LODESTAR_MAX_RX_QUEUES      (LODESTAR_MAX_FDIR_INDICES + 1)
#define LODESTAR_MAX_TX_QUEUES      (LODESTAR_MAX_FDIR_INDICES + 1)
#define LODESTAR_MAX_XDP_QUEUES     (LODESTAR_MAX_FDIR_INDICES + 1)

DECLARE_STATIC_KEY_FALSE(xlnid_xdp_locking_key);

struct xlnid_ring_feature {
	u16 limit;  /* upper limit on feature indices */
	u16 indices;    /* current value of indices */
	u16 mask;   /* Mask used for feature to ring mapping */
	u16 offset; /* offset to start of feature */
};

#define XLNID_VMDQ_8Q_MASK 0x78
#define XLNID_VMDQ_4Q_MASK 0x7C
#define XLNID_VMDQ_2Q_MASK 0x7E

#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT
/*
		FCoE requires that all Rx buffers be over 2200 bytes in length.  Since
		this is twice the size of a half page we need to double the page order
		for FCoE enabled Rx queues.
*/
static inline unsigned int xlnid_rx_bufsz(struct xlnid_ring __maybe_unused
		*ring)
{
#if MAX_SKB_FRAGS < 8
	return ALIGN(XLNID_MAX_RXBUFFER / MAX_SKB_FRAGS, 1024);
#else

	if (test_bit(__XLNID_RX_3K_BUFFER, &ring->state)) {
		return XLNID_RXBUFFER_3K;
	}

#if (PAGE_SIZE < 8192)

	if (ring_uses_build_skb(ring)) {
		return XLNID_MAX_2K_FRAME_BUILD_SKB;
	}

#endif
	return XLNID_RXBUFFER_2K;
#endif
}

static inline unsigned int xlnid_rx_pg_order(struct xlnid_ring __maybe_unused
		*ring)
{
#if (PAGE_SIZE < 8192)

	if (test_bit(__XLNID_RX_3K_BUFFER, &ring->state)) {
		return 1;
	}

#endif
	return 0;
}

#define xlnid_rx_pg_size(_ring) (PAGE_SIZE << xlnid_rx_pg_order(_ring))

#else
static inline unsigned int xlnid_rx_bufsz(struct xlnid_ring *ring)
{
	return ring->rx_buf_len;
}
#endif
#define XLNID_ITR_ADAPTIVE_MIN_INC  2
#define XLNID_ITR_ADAPTIVE_MIN_USECS    10
#define XLNID_ITR_ADAPTIVE_MAX_USECS    126
#define XLNID_ITR_ADAPTIVE_LATENCY  0x80
#define XLNID_ITR_ADAPTIVE_BULK     0x00
#define XLNID_ITR_ADAPTIVE_MASK_USECS   (XLNID_ITR_ADAPTIVE_LATENCY - \
						XLNID_ITR_ADAPTIVE_MIN_INC)

struct xlnid_ring_container {
	struct xlnid_ring *ring;    /* pointer to linked list of rings */
	unsigned long next_update;  /* jiffies value of last update */
	unsigned int total_bytes;   /* total bytes processed this int */
	unsigned int total_packets; /* total packets processed this int */
	u16 work_limit;         /* total work allowed per interrupt */
	u8 count;           /* total number of rings in vector */
	u8 itr;             /* current ITR setting for ring */
};

#define xlnid_pf_to_dev(pf) (&((pf)->pdev->dev))

/* iterator for handling rings in ring container */
#define xlnid_for_each_ring(pos, head) \
	for (pos = (head).ring; pos != NULL; pos = pos->next)

#define MAX_RX_PACKET_BUFFERS   ((adapter->flags & XLNID_FLAG_DCB_ENABLED) \
					? 8 : 1)
#define MAX_TX_PACKET_BUFFERS   MAX_RX_PACKET_BUFFERS

#define XLNID_IFNAMSIZ (IFNAMSIZ + 9)

/*  MAX_MSIX_Q_VECTORS of these are allocated,
		but we only use one per queue-specific vector.
*/
struct xlnid_q_vector {
	struct xlnid_adapter *adapter;
	int cpu;    /* CPU for DCA */
	u16 v_idx;  /* index of q_vector within array, also used for
				finding the bit in EICR and friends that
				represents the vector for this ring */
	u16 itr;    /* Interrupt throttle rate written to EITR */
	struct xlnid_ring_container rx, tx;

	struct napi_struct napi;
#ifndef HAVE_NETDEV_NAPI_LIST
	struct net_device poll_dev;
#endif
#ifdef HAVE_IRQ_AFFINITY_HINT
	cpumask_t affinity_mask;
#endif
	int node;
	struct rcu_head rcu;        /* to avoid race with update stats on free */
	char name[XLNID_IFNAMSIZ];
	bool netpoll_rx;

#ifdef XLNID_DEBUG_ITR
	unsigned long last_intr_stat;
	unsigned int intr_num;
	unsigned int intr_per_second;
#endif

#ifdef XLNID_TEST_XL84101
	bool rx_drop;
#endif

#ifdef HAVE_NDO_BUSY_POLL
	atomic_t state;
#endif  /* HAVE_NDO_BUSY_POLL */

	/* for dynamic allocation of rings associated with this q_vector */
	struct xlnid_ring ring[0] ____cacheline_internodealigned_in_smp;
};

#ifdef HAVE_NDO_BUSY_POLL
enum xlnid_qv_state_t {
	XLNID_QV_STATE_IDLE = 0,
	XLNID_QV_STATE_NAPI,
	XLNID_QV_STATE_POLL,
	XLNID_QV_STATE_DISABLE
};

static inline void xlnid_qv_init_lock(struct xlnid_q_vector *q_vector)
{
	/* reset state to idle */
	atomic_set(&q_vector->state, XLNID_QV_STATE_IDLE);
}

/* called from the device poll routine to get ownership of a q_vector */
static inline bool xlnid_qv_lock_napi(struct xlnid_q_vector *q_vector)
{
	int rc = atomic_cmpxchg(&q_vector->state, XLNID_QV_STATE_IDLE,
							XLNID_QV_STATE_NAPI);
#ifdef BP_EXTENDED_STATS

	if (rc != XLNID_QV_STATE_IDLE) {
		q_vector->tx.ring->stats.yields++;
	}

#endif

	return rc == XLNID_QV_STATE_IDLE;
}

/* returns true is someone tried to get the qv while napi had it */
static inline void xlnid_qv_unlock_napi(struct xlnid_q_vector *q_vector)
{
	WARN_ON(atomic_read(&q_vector->state) != XLNID_QV_STATE_NAPI);

	/* flush any outstanding Rx frames */
	if (q_vector->napi.gro_list) {
		napi_gro_flush(&q_vector->napi, false);
	}

	/* reset state to idle */
	atomic_set(&q_vector->state, XLNID_QV_STATE_IDLE);
}

/* called from xlnid_low_latency_poll() */
static inline bool xlnid_qv_lock_poll(struct xlnid_q_vector *q_vector)
{
	int rc = atomic_cmpxchg(&q_vector->state, XLNID_QV_STATE_IDLE,
							XLNID_QV_STATE_POLL);
#ifdef BP_EXTENDED_STATS

	if (rc != XLNID_QV_STATE_IDLE) {
		q_vector->rx.ring->stats.yields++;
	}

#endif

	return rc == XLNID_QV_STATE_IDLE;
}

/* returns true if someone tried to get the qv while it was locked */
static inline void xlnid_qv_unlock_poll(struct xlnid_q_vector *q_vector)
{
	WARN_ON(atomic_read(&q_vector->state) != XLNID_QV_STATE_POLL);

	/* reset state to idle */
	atomic_set(&q_vector->state, XLNID_QV_STATE_IDLE);
}

/* true if a socket is polling, even if it did not get the lock */
static inline bool xlnid_qv_busy_polling(struct xlnid_q_vector *q_vector)
{
	return atomic_read(&q_vector->state) == XLNID_QV_STATE_POLL;
}

/* false if QV is currently owned */
static inline bool xlnid_qv_disable(struct xlnid_q_vector *q_vector)
{
	int rc = atomic_cmpxchg(&q_vector->state, XLNID_QV_STATE_IDLE,
							XLNID_QV_STATE_DISABLE);

	return rc == XLNID_QV_STATE_IDLE;
}

#endif /* HAVE_NDO_BUSY_POLL */
#ifdef XLNID_HWMON

#define XLNID_HWMON_TYPE_LOC        0
#define XLNID_HWMON_TYPE_TEMP       1
#define XLNID_HWMON_TYPE_CAUTION    2
#define XLNID_HWMON_TYPE_MAX        3

struct hwmon_attr {
	struct device_attribute dev_attr;
	struct xlnid_hw *hw;
	struct xlnid_thermal_diode_data *sensor;
	char name[12];
};

struct hwmon_buff {
#ifdef HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS
	struct attribute_group group;
	const struct attribute_group *groups[2];
	struct attribute *attrs[XLNID_MAX_SENSORS * 4 + 1];
	struct hwmon_attr hwmon_list[XLNID_MAX_SENSORS * 4];
#else
	struct device *device;
	struct hwmon_attr *hwmon_list;
#endif /* HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS */
	unsigned int n_hwmon;
};
#endif /* XLNID_HWMON */

/*
		microsecond values for various ITR rates shifted by 2 to fit itr register
		with the first 3 bits reserved 0
*/
#define XLNID_MIN_RSC_ITR   24
#define XLNID_100K_ITR      40
#define XLNID_20K_ITR       200
#define XLNID_16K_ITR       248
#define XLNID_12K_ITR       336

/* xlnid_test_staterr - tests bits in Rx descriptor status and error fields */
static inline __le32 xlnid_test_staterr(union xlnid_adv_rx_desc *rx_desc,
										const u32 stat_err_bits)
{
#ifdef XLNID_LODESTAR
	return 0;
#else
	return rx_desc->wb.upper.status_error & cpu_to_le32(stat_err_bits);
#endif
}

/* xlnid_desc_unused - calculate if we have unused descriptors */
static inline u16 xlnid_desc_unused(struct xlnid_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->count) + ntc - ntu - 1;
}

#define XLNID_RX_DESC(R, i) \
	(&(((union xlnid_rx_desc *)((R)->desc))[i]))

#define XLNID_TX_DESC(R, i) \
	(&(((union xlnid_tx_desc *)((R)->desc))[i]))

#define XLNID_TX_CTXTDESC(R, i) \
	(&(((struct xlnid_tx_context_desc *)((R)->desc))[i]))

#define XLNID_MAX_JUMBO_FRAME_SIZE  9728

#define TCP_TIMER_VECTOR    0
#define OTHER_VECTOR    1
#define NON_Q_VECTORS   (OTHER_VECTOR + TCP_TIMER_VECTOR)

#define XLNID_MAX_MSIX_Q_VECTORS    64

struct xlnid_mac_addr {
	u8 addr[ETH_ALEN];
	u16 pool;
	u16 state; /* bitmask */
};

#define XLNID_MAC_STATE_DEFAULT     0x1
#define XLNID_MAC_STATE_MODIFIED    0x2
#define XLNID_MAC_STATE_IN_USE      0x4

#ifdef XLNID_PROCFS
struct xlnid_therm_proc_data {
	struct xlnid_hw *hw;
	struct xlnid_thermal_diode_data *sensor_data;
};

#endif /* XLNID_PROCFS */
/*
		Only for array allocations in our adapter struct.  On westlake,
		we can actually assign 64 queue vectors based on our extended-extended
		interrupt registers.
*/
#define MAX_MSIX_Q_VECTORS  XLNID_MAX_MSIX_Q_VECTORS
#define MAX_MSIX_COUNT      XLNID_MAX_MSIX_VECTORS


#define MIN_MSIX_Q_VECTORS  1
#define MIN_MSIX_COUNT      (MIN_MSIX_Q_VECTORS + NON_Q_VECTORS)

/* default to trying for four seconds */
#define XLNID_TRY_LINK_TIMEOUT  (4 * HZ)
#define XLNID_SFP_POLL_JIFFIES  (2 * HZ)    /* SFP poll every 2 seconds */

#define XLNID_PRIMARY_ABORT_LIMIT   5

/* board specific private data structure */
struct xlnid_adapter {
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
#ifdef HAVE_VLAN_RX_REGISTER
	struct vlan_group *vlgrp; /* must be first, see xlnid_receive_skb */
#else
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
#endif
#endif /* NETIF_F_HW_VLAN_TX || NETIF_F_HW_VLAN_CTAG_TX */
	/* OS defined structs */
	struct net_device *netdev;
	struct bpf_prog *xdp_prog;
	struct pci_dev *pdev;

	unsigned long state;

	/*  Some features need tri-state capability,
		thus the additional *_CAPABLE flags.
	*/
	u32 flags;
#define XLNID_FLAG_MSI_CAPABLE          ((u32)(1 << 0))
#define XLNID_FLAG_MSI_ENABLED          ((u32)(1 << 1))
#define XLNID_FLAG_MSIX_CAPABLE         ((u32)(1 << 2))
#define XLNID_FLAG_MSIX_ENABLED         ((u32)(1 << 3))
#ifndef XLNID_NO_LLI
#define XLNID_FLAG_LLI_PUSH         ((u32)(1 << 4))
#endif

#define XLNID_FLAG_FD_SB_ENABLED        BIT(5)
#define XLNID_FLAG_FD_ATR_ENABLED       BIT(6)
#define XLNID_FLAG_HW_ATR_EVICT_ENABLED BIT(7)

#define XLNID_FLAG_MQ_CAPABLE           ((u32)(1 << 9))
#define XLNID_FLAG_DCB_ENABLED          ((u32)(1 << 10))
#define XLNID_FLAG_VMDQ_ENABLED         ((u32)(1 << 11))
#define XLNID_FLAG_FAN_FAIL_CAPABLE     ((u32)(1 << 12))
#define XLNID_FLAG_NEED_LINK_UPDATE     ((u32)(1 << 13))
#define XLNID_FLAG_NEED_LINK_CONFIG     ((u32)(1 << 14))
#define XLNID_FLAG_FDIR_HASH_CAPABLE        ((u32)(1 << 15))
#define XLNID_FLAG_FDIR_PERFECT_CAPABLE     ((u32)(1 << 16))
#define XLNID_FLAG_SRIOV_CAPABLE        ((u32)(1 << 19))
#define XLNID_FLAG_SRIOV_ENABLED        ((u32)(1 << 20))
#define XLNID_FLAG_SRIOV_REPLICATION_ENABLE ((u32)(1 << 21))
#define XLNID_FLAG_SRIOV_L2SWITCH_ENABLE    ((u32)(1 << 22))
#define XLNID_FLAG_SRIOV_VEPA_BRIDGE_MODE   ((u32)(1 << 23))
#define XLNID_FLAG_RX_HWTSTAMP_ENABLED          ((u32)(1 << 24))
#define XLNID_FLAG_VXLAN_OFFLOAD_CAPABLE    ((u32)(1 << 25))
#define XLNID_FLAG_VXLAN_OFFLOAD_ENABLE     ((u32)(1 << 26))
#define XLNID_FLAG_RX_HWTSTAMP_IN_REGISTER  ((u32)(1 << 27))
#define XLNID_FLAG_MDD_ENABLED          ((u32)(1 << 29))
#define XLNID_FLAG_DCB_CAPABLE          ((u32)(1 << 30))
#define XLNID_FLAG_GENEVE_OFFLOAD_CAPABLE   BIT(31)

	/* preset defaults */
#define XLNID_FLAGS_SKYLAKE_INIT        (XLNID_FLAG_MSI_CAPABLE \
										| XLNID_FLAG_MSIX_CAPABLE \
										| XLNID_FLAG_MQ_CAPABLE)
#define XLNID_FLAGS_WESTLAKE_INIT      (XLNID_FLAG_MSI_CAPABLE \
										| XLNID_FLAG_MSIX_CAPABLE \
										| XLNID_FLAG_MQ_CAPABLE)

	u32 flags2;
#define XLNID_FLAG2_RSC_CAPABLE         ((u32)(1 << 0))
#define XLNID_FLAG2_RSC_ENABLED         ((u32)(1 << 1))
#define XLNID_FLAG2_TEMP_SENSOR_CAPABLE     ((u32)(1 << 3))
#define XLNID_FLAG2_TEMP_SENSOR_EVENT       ((u32)(1 << 4))
#define XLNID_FLAG2_SEARCH_FOR_SFP      ((u32)(1 << 5))
#define XLNID_FLAG2_SFP_NEEDS_RESET     ((u32)(1 << 6))
#define XLNID_FLAG2_FDIR_REQUIRES_REINIT    ((u32)(1 << 8))
#define XLNID_FLAG2_RSS_FIELD_IPV4_UDP      ((u32)(1 << 9))
#define XLNID_FLAG2_RSS_FIELD_IPV6_UDP      ((u32)(1 << 10))
#define XLNID_FLAG2_PTP_PPS_ENABLED     ((u32)(1 << 11))
#define XLNID_FLAG2_EEE_CAPABLE         ((u32)(1 << 14))
#define XLNID_FLAG2_EEE_ENABLED         ((u32)(1 << 15))
#define XLNID_FLAG2_UDP_TUN_REREG_NEEDED    ((u32)(1 << 16))
#define XLNID_FLAG2_PHY_INTERRUPT       ((u32)(1 << 17))
#define XLNID_FLAG2_VLAN_PROMISC        ((u32)(1 << 18))
#define XLNID_FLAG2_RX_LEGACY           ((u32)(1 << 19))

	/* Tx fast path data */
	int num_tx_queues;
	u16 tx_itr_setting;
	u16 tx_work_limit;

#if defined(HAVE_UDP_ENC_RX_OFFLOAD) || defined(HAVE_VXLAN_RX_OFFLOAD)
	__be16 vxlan_port;
#endif /* HAVE_UDP_ENC_RX_OFFLAD || HAVE_VXLAN_RX_OFFLOAD */
#ifdef HAVE_UDP_ENC_RX_OFFLOAD
	__be16 geneve_port;
#endif /* HAVE_UDP_ENC_RX_OFFLOAD */

	/* Rx fast path data */
	int num_rx_queues;
	u16 rx_itr_setting;

	/* XDP */
	int num_xdp_queues;
	struct xlnid_ring *xdp_ring[MAX_XDP_QUEUES];
	unsigned long *af_xdp_zc_qps; /* tracks AF_XDP ZC enabled rings */

	/* TX */
	struct xlnid_ring *tx_ring[MAX_TX_QUEUES] ____cacheline_aligned_in_smp;

	u64 restart_queue;
	u64 lsc_int;
	u32 tx_timeout_count;

	/* RX */
	struct xlnid_ring *rx_ring[MAX_RX_QUEUES];
	int num_rx_pools; /* does not include pools assigned to VFs */
	int num_rx_queues_per_pool;
	u64 hw_csum_rx_error;
	u64 hw_rx_no_dma_resources;
	u64 rsc_total_count;
	u64 rsc_total_flush;
	u64 non_eop_descs;
	u32 alloc_rx_page;
	u32 alloc_rx_page_failed;
	u32 alloc_rx_buff_failed;

	struct xlnid_q_vector *q_vector[MAX_MSIX_Q_VECTORS];

#ifdef HAVE_DCBNL_IEEE
	struct ieee_pfc *xlnid_ieee_pfc;
	struct ieee_ets *xlnid_ieee_ets;
#endif
	//struct xlnid_dcb_config dcb_cfg;
	//struct xlnid_dcb_config temp_dcb_cfg;
	u8 dcb_set_bitmap;
	u8 dcbx_cap;
#ifndef HAVE_MQPRIO
	u8 dcb_tc;
#endif
	enum xlnid_fc_mode last_lfc_mode;

	int num_q_vectors;  /* current number of q_vectors for device */
	int max_q_vectors;  /* upper limit of q_vectors for device */
	struct xlnid_ring_feature ring_feature[RING_F_ARRAY_SIZE];
	struct msix_entry *msix_entries;

#ifndef HAVE_NETDEV_STATS_IN_NETDEV
	struct net_device_stats net_stats;
#endif

#ifdef ETHTOOL_TEST
	u32 test_icr;
	struct xlnid_ring test_tx_ring;
	struct xlnid_ring test_rx_ring;
#endif

	/* structs defined in xlnid_hw.h */
	struct xlnid_hw hw;
	u16 msg_enable;
	struct xlnid_hw_stats stats;
#ifndef XLNID_NO_LLI
	u32 lli_port;
	u32 lli_size;
	u32 lli_etype;
	u32 lli_vlan_pri;
#endif /* XLNID_NO_LLI */

	u32 *config_space;
	u64 tx_busy;
	unsigned int tx_ring_count;
	unsigned int xdp_ring_count;
	unsigned int rx_ring_count;

	u32 link_speed;
	bool link_up;
	bool link_change;

	bool cloud_mode;

	unsigned long sfp_poll_time;
	unsigned long link_check_timeout;

	struct timer_list service_timer;
	struct work_struct service_task;

	struct hlist_head fdir_filter_list;

	/* current, add new in eastlake about filter(should in pf) */
	unsigned long fdir_overflow; /* number of times ATR was backed off */
	union xlnid_atr_input fdir_mask;
	int fdir_filter_count;
	u32 fdir_pballoc;
	u32 atr_sample_rate;
	spinlock_t fdir_perfect_lock;
	u32 fd_add_err;
	u32 fd_atr_cnt;
	u32 fd_inv;

	/*  Book-keeping of side-band filter count per flow-type.
		This is used to detect and handle input set changes for
		respective flow-type.
	*/
	u16 fd_tcp4_filter_cnt;
	u16 fd_udp4_filter_cnt;
	u16 fd_sctp4_filter_cnt;
	u16 fd_ip4_filter_cnt;

	u16 fd_tcp6_filter_cnt;
	u16 fd_udp6_filter_cnt;
	u16 fd_sctp6_filter_cnt;
	u16 fd_ip6_filter_cnt;

	u8 __iomem *io_addr;    /* Mainly for iounmap use */
	u32 wol;

	u16 bd_number;

#ifdef HAVE_BRIDGE_ATTRIBS
	u16 bridge_mode;
#endif

	char eeprom_id[32];
	//u16 eeprom_cap;
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

	struct work_struct ptp_pps_work;
	struct work_struct ptp_extts0_work;
	struct work_struct ptp_extts1_work;

	unsigned long last_overflow_check;
	unsigned long last_rx_ptp_check;
	spinlock_t tmreg_lock;
	struct cyclecounter hw_cc;
	struct timecounter hw_tc;
	u32 base_incval;
	u32 tx_hwtstamp_timeouts;
	u32 tx_hwtstamp_skipped;
	u32 rx_hwtstamp_cleared;

	bool ptp_tx;
	bool ptp_rx;

	void (*ptp_setup_sdp)(struct xlnid_adapter *);
#endif /* HAVE_PTP_1588_CLOCK */

	DECLARE_BITMAP(active_vfs, XLNID_MAX_VF_FUNCTIONS);
	unsigned int num_vfs;
	unsigned int max_vfs;
	struct vf_data_storage *vfinfo;
	int vf_rate_link_speed;
	struct vf_macvlans vf_mvs;
	struct vf_macvlans *mv_list;

	u16 pool_id;
#if 0
	u32 timer_event_accumulator;
	u32 vferr_refcount;
#endif

	struct xlnid_mac_addr *mac_table;

#ifdef XLNID_SYSFS
#ifdef XLNID_HWMON
#ifdef HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS
	struct hwmon_buff *xlnid_hwmon_buff;
#else
	struct hwmon_buff xlnid_hwmon_buff;
#endif /* HAVE_HWMON_DEVICE_REGISTER_WITH_GROUPS */
#endif /* XLNID_HWMON */
#else /* XLNID_SYSFS */

#ifdef XLNID_PROCFS
	struct proc_dir_entry *eth_dir;
	struct proc_dir_entry *info_dir;
	u64 old_lsc;
	struct proc_dir_entry *therm_dir[XLNID_MAX_SENSORS];
	struct xlnid_therm_proc_data therm_data[XLNID_MAX_SENSORS];
#endif /* XLNID_PROCFS */
#endif /* XLNID_SYSFS */

#ifdef HAVE_XLNID_DEBUG_FS
	struct dentry *xlnid_dbg_adapter_pf;
#endif /*HAVE_XLNID_DEBUG_FS*/
	u8 default_up;

	/*  maximum number of RETA entries among all devices supported by xlnid
			driver: currently it's x550 device in non-SRIOV mode
	*/
#define XLNID_MAX_RETA_ENTRIES 512
	u8 rss_indir_tbl[XLNID_MAX_RETA_ENTRIES];

#define XLNID_RSS_KEY_SIZE     40   /* size of RSS Hash Key in bytes */
	u32 *rss_key;

#ifdef XLNID_TEST_XL84101
	u8 rx_disable;
	u8 tx_disable;
	u32 rxsize;
	u32 txsize;
	struct sk_buff_head rxq;
	struct sk_buff_head txq;
	struct xlnid_ring_stats rx_stats[XLNID_XL84101_MAX_QUEUES];
	struct xlnid_ring_stats tx_stats[XLNID_XL84101_MAX_QUEUES];
#endif

#ifdef HAVE_TX_MQ
#ifndef HAVE_NETDEV_SELECT_QUEUE
	unsigned int indices;
#endif
#endif
#ifdef HAVE_AF_XDP_ZC_SUPPORT
	/* AF_XDP zero-copy */
#ifdef HAVE_NETDEV_BPF_XSK_POOL
	struct xsk_buff_pool **xsk_pools;
#else
	struct xdp_umem **xsk_pools;
#endif /* HAVE_NETDEV_BPF_XSK_POOL */
	u16 num_xsk_pools_used;
	u16 num_xsk_pools;
#endif

	/* macsec */
	struct xlnid_macsec_cfg *macsec_cfg;
	struct mutex macsec_mutex;
};

static inline unsigned int xlnid_determine_xdp_q_idx(unsigned int cpu)
{
	if (static_key_enabled((struct static_key *)&xlnid_xdp_locking_key)) {
		return cpu % WESTLAKE_MAX_XDP_QUEUES;
	}

	else {
		return cpu;
	}
}

static inline
struct xlnid_ring *xlnid_determine_xdp_ring(struct xlnid_adapter *adapter)
{
	unsigned int index = xlnid_determine_xdp_q_idx(smp_processor_id());

	return adapter->xdp_ring[index];
}

static inline u8 xlnid_max_rss_indices(struct xlnid_adapter *adapter)
{
	return XLNID_MAX_RSS_INDICES;
}

struct xlnid_fdir_filter {
	struct  hlist_node fdir_node;
	union xlnid_atr_input filter;
	u16 sw_idx;
	u64 action;
};

enum xlnid_state_t {
	__XLNID_TESTING,
	__XLNID_RESETTING,
	__XLNID_DOWN,
	__XLNID_DISABLED,
	__XLNID_REMOVING,
	__XLNID_SERVICE_SCHED,
	__XLNID_SERVICE_INITED,
	__XLNID_IN_SFP_INIT,
	__XLNID_FD_FLUSH_REQUESTED,
	__XLNID_FD_ATR_AUTO_DISABLED,
	__XLNID_FD_SB_AUTO_DISABLED,
#ifdef HAVE_PTP_1588_CLOCK
	__XLNID_PTP_RUNNING,
	__XLNID_PTP_TX_IN_PROGRESS,
#endif
	__XLNID_RESET_REQUESTED,
};

struct xlnid_cb {
#ifdef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	union {             /* Union defining head/tail partner */
		struct sk_buff *head;
		struct sk_buff *tail;
	};
#endif
	dma_addr_t dma;
#ifdef HAVE_VLAN_RX_REGISTER
	u16 vid;            /* VLAN tag */
#endif
	u16 append_cnt;             /* number of skb's appended */
#ifndef CONFIG_XLNID_DISABLE_PACKET_SPLIT
	bool    page_released;
#endif
};
#define XLNID_CB(skb) ((struct xlnid_cb *)(skb)->cb)

#ifdef XLNID_SYSFS
void xlnid_sysfs_exit(struct xlnid_adapter *adapter);
int xlnid_sysfs_init(struct xlnid_adapter *adapter);
#endif /* XLNID_SYSFS */
#ifdef XLNID_PROCFS
void xlnid_procfs_exit(struct xlnid_adapter *adapter);
int xlnid_procfs_init(struct xlnid_adapter *adapter);
int xlnid_procfs_topdir_init(void);
void xlnid_procfs_topdir_exit(void);
#endif /* XLNID_PROCFS */

extern struct dcbnl_rtnl_ops xlnid_dcbnl_ops;
int xlnid_copy_dcb_cfg(struct xlnid_adapter *adapter, int tc_max);

u8 xlnid_dcb_txq_to_tc(struct xlnid_adapter *adapter, u8 index);

/* needed by xlnid_main.c */
int xlnid_validate_mac_addr(u8 *mc_addr);
void xlnid_check_options(struct xlnid_adapter *adapter);
void xlnid_assign_netdev_ops(struct net_device *netdev);

/* needed by xlnid_ethtool.c */
#ifdef HAVE_NON_CONST_PCI_DRIVER_NAME
extern char xlnid_driver_name[];
#else
extern const char xlnid_driver_name[];
#endif
extern const char xlnid_driver_version[];

int xlnid_open(struct net_device *netdev);
int xlnid_close(struct net_device *netdev);
void xlnid_up(struct xlnid_adapter *adapter);
void xlnid_down(struct xlnid_adapter *adapter);
void xlnid_reinit_locked(struct xlnid_adapter *adapter);
void xlnid_reset(struct xlnid_adapter *adapter);
void xlnid_set_ethtool_ops(struct net_device *netdev);
void xlnid_set_macsec_ops(struct net_device *netdev);
int xlnid_setup_rx_resources(struct xlnid_adapter *, struct xlnid_ring *);
int xlnid_setup_tx_resources(struct xlnid_ring *);
void xlnid_free_rx_resources(struct xlnid_ring *);
void xlnid_free_tx_resources(struct xlnid_ring *);
void xlnid_configure_rx_ring(struct xlnid_adapter *,
								struct xlnid_ring *);
void xlnid_configure_tx_ring(struct xlnid_adapter *,
								struct xlnid_ring *);
void xlnid_update_stats(struct xlnid_adapter *adapter);
int xlnid_init_interrupt_scheme(struct xlnid_adapter *adapter);
void xlnid_reset_interrupt_capability(struct xlnid_adapter *adapter);
void xlnid_set_interrupt_capability(struct xlnid_adapter *adapter);
void xlnid_clear_interrupt_scheme(struct xlnid_adapter *adapter);
bool xlnid_is_xlnid(struct pci_dev *pcidev);
netdev_tx_t xlnid_xmit_frame_ring(struct sk_buff *,
									struct xlnid_adapter *,
									struct xlnid_ring *);
void xlnid_alloc_rx_buffers(struct xlnid_ring *, u16);
#if defined(HAVE_UDP_ENC_RX_OFFLOAD) || defined(HAVE_VXLAN_RX_OFFLOAD)
void xlnid_clear_udp_tunnel_port(struct xlnid_adapter *, u32);
#endif
void xlnid_set_rx_mode(struct net_device *netdev);
int xlnid_write_mc_addr_list(struct net_device *netdev);
int xlnid_setup_tc(struct net_device *dev, u8 tc);
void xlnid_tx_ctxtdesc(struct xlnid_ring *, u32, u32, u32, u32);
void xlnid_do_reset(struct net_device *netdev);
void xlnid_write_eitr(struct xlnid_q_vector *q_vector);
int xlnid_poll(struct napi_struct *napi, int budget);
void xlnid_disable_rx_queue(struct xlnid_adapter *adapter);
void xlnid_disable_tx_queue(struct xlnid_adapter *adapter);
void xlnid_vlan_strip_enable(struct xlnid_adapter *adapter);
void xlnid_vlan_strip_disable(struct xlnid_adapter *adapter);
#ifdef ETHTOOL_OPS_COMPAT
int ethtool_ioctl(struct ifreq *ifr);
#endif

#ifdef HAVE_XLNID_DEBUG_FS
void xlnid_dbg_adapter_init(struct xlnid_adapter *adapter);
void xlnid_dbg_adapter_exit(struct xlnid_adapter *adapter);
void xlnid_dbg_init(const char *driver_name);
void xlnid_dbg_exit(void);
#endif /* HAVE_XLNID_DEBUG_FS */

static inline struct netdev_queue *txring_txq(const struct xlnid_ring *ring)
{
	return netdev_get_tx_queue(ring->netdev, ring->queue_index);
}

void xlnid_full_sync_mac_table(struct xlnid_adapter *adapter);
int xlnid_add_mac_filter(struct xlnid_adapter *adapter,
							const u8 *addr, u16 queue);
int xlnid_del_mac_filter(struct xlnid_adapter *adapter,
							const u8 *addr, u16 queue);
int xlnid_available_rars(struct xlnid_adapter *adapter, u16 pool);
#ifndef HAVE_VLAN_RX_REGISTER
void xlnid_vlan_mode(struct net_device *, u32);
#else
#if 0
int xlnid_find_vlvf_entry(struct xlnid_hw *hw, u32 vlan);
#endif
#endif
#ifdef HAVE_PTP_1588_CLOCK
void xlnid_ptp_init(struct xlnid_adapter *adapter);
void xlnid_ptp_stop(struct xlnid_adapter *adapter);
void xlnid_ptp_suspend(struct xlnid_adapter *adapter);
void xlnid_ptp_overflow_check(struct xlnid_adapter *adapter);
void xlnid_ptp_rx_hang(struct xlnid_adapter *adapter);
void xlnid_ptp_tx_hang(struct xlnid_adapter *adapter);
void xlnid_ptp_rx_pktstamp(struct xlnid_q_vector *q_vector,
							struct sk_buff *skb);
void xlnid_ptp_rx_rgtstamp(struct xlnid_q_vector *q_vector,
							struct sk_buff *skb);
static inline void xlnid_ptp_rx_hwtstamp(struct xlnid_ring *rx_ring,
		union xlnid_adv_rx_desc *rx_desc,
		struct sk_buff *skb)
{
#if 0

	if (unlikely(xlnid_test_staterr(rx_desc, XLNID_RXD_STAT_TSIP))) {
		xlnid_ptp_rx_pktstamp(rx_ring->q_vector, skb);
		return;
	}

#endif

	if (unlikely(!xlnid_test_staterr(rx_desc, XLNID_RXDADV_STAT_TS))) {
		return;
	}

	xlnid_ptp_rx_rgtstamp(rx_ring->q_vector, skb);

	/*  Update the last_rx_timestamp timer in order to enable watchdog check
		for error case of latched timestamp on a dropped packet.
	*/
	rx_ring->last_rx_timestamp = jiffies;
}

int xlnid_ptp_get_ts_config(struct xlnid_adapter *adapter, struct ifreq *ifr);
int xlnid_ptp_set_ts_config(struct xlnid_adapter *adapter, struct ifreq *ifr);
void xlnid_ptp_start_cyclecounter(struct xlnid_adapter *adapter);
void xlnid_ptp_reset(struct xlnid_adapter *adapter);
void xlnid_ptp_check_pps_event(struct xlnid_adapter *adapter);
#endif /* HAVE_PTP_1588_CLOCK */

#if 0
void xlnid_sriov_reinit(struct xlnid_adapter *adapter);
#endif

u32 xlnid_rss_indir_tbl_entries(struct xlnid_adapter *adapter);
void xlnid_store_key(struct xlnid_adapter *adapter);
void xlnid_store_reta(struct xlnid_adapter *adapter);

void xlnid_set_rx_drop_en(struct xlnid_adapter *adapter);

#endif /* _XLNID_H_ */
