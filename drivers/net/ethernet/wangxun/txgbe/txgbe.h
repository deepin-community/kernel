/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2015 - 2022 Beijing WangXun Technology Co., Ltd. */

#ifndef _TXGBE_H_
#define _TXGBE_H_

#include <net/ip.h>
#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/cpumask.h>
#include <linux/if_vlan.h>
#include <linux/jiffies.h>
#include <linux/phy.h>

#include <linux/timecounter.h>
#include <linux/net_tstamp.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/vmalloc.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>
#include "txgbe_type.h"
#include "txgbe_dcb.h"

#include <linux/sctp.h>
#include <linux/mdio.h>

#if IS_ENABLED(CONFIG_FCOE)
#include "txgbe_fcoe.h"
#endif /* CONFIG_FCOE */

#include <net/xdp.h>

#include <linux/timecounter.h>
#include <linux/clocksource.h>
#include <linux/net_tstamp.h>
#include <linux/ptp_clock_kernel.h>

/* Ether Types */
#define TXGBE_ETH_P_LLDP                        0x88CC
#define TXGBE_ETH_P_CNM                         0x22E7

DECLARE_STATIC_KEY_FALSE(txgbe_xdp_locking_key);

/* TX/RX descriptor defines */
#define TXGBE_DEFAULT_TXD               1024
#define TXGBE_DEFAULT_TX_WORK           256
#define TXGBE_MAX_TXD                   8192
#define TXGBE_MIN_TXD                   128
#define TXGBE_MAX_TX_WORK               65535

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

#define TXGBE_DEFAULT_FCPAUSE           0xFFFF

#define TXGBE_MIN_FCPAUSE               0
#define TXGBE_MAX_FCPAUSE               0xFFFF

/* Supported Rx Buffer Sizes */
#define TXGBE_RXBUFFER_256       256  /* Used for skb receive header */
#define TXGBE_RXBUFFER_2K       2048
#define TXGBE_RXBUFFER_3K       3072
#define TXGBE_RXBUFFER_4K       4096
#define TXGBE_RXBUFFER_1536     1536
#define TXGBE_RXBUFFER_7K       7168
#define TXGBE_RXBUFFER_8K       8192
#define TXGBE_RXBUFFER_15K      15360
#define TXGBE_MAX_RXBUFFER      16384  /* largest size for single descriptor */

#define TXGBE_BP_M_NULL                      0
#define TXGBE_BP_M_SFI                       1
#define TXGBE_BP_M_KR                        2
#define TXGBE_BP_M_KX4                       3
#define TXGBE_BP_M_KX                        4
#define TXGBE_BP_M_NAUTO                     0
#define TXGBE_BP_M_AUTO                      1

#define TXGBE_RX_HDR_SIZE       TXGBE_RXBUFFER_256

#define MAXIMUM_ETHERNET_VLAN_SIZE      (VLAN_ETH_FRAME_LEN + ETH_FCS_LEN)

/* How many Rx Buffers do we bundle into one write to the hardware ? */
#define TXGBE_RX_BUFFER_WRITE   16      /* Must be power of 2 */

#define TXGBE_RX_DMA_ATTR \
	(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)

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

#define TXGBE_VF_MAX_TX_QUEUES          4

struct vf_data_storage {
	struct pci_dev *vfdev;
	u8 __iomem *b4_addr;
	u32 b4_buf[16];
	unsigned char vf_mac_addresses[ETH_ALEN];
	u16 vf_mc_hashes[TXGBE_MAX_VF_MC_ENTRIES];
	u16 num_vf_mc_hashes;
	u16 default_vf_vlan_id;
	u16 vlans_enabled;
	bool clear_to_send;
	bool pf_set_mac;
	u16 pf_vlan; /* When set, guest VLAN config not allowed. */
	u16 pf_qos;
	__be16 vlan_proto;
	u16 min_tx_rate;
	u16 max_tx_rate;
	u16 vlan_count;
	u8 spoofchk_enabled;
	int link_enable;
	int link_state;
	bool rss_query_enabled;

	u8 trusted;
	int xcast_mode;
	unsigned int vf_api;
	u16 ft_filter_idx[TXGBE_MAX_RDB_5T_CTL0_FILTERS];
	u16 queue_max_tx_rate[TXGBE_VF_MAX_TX_QUEUES];
};

struct vf_macvlans {
	struct list_head l;
	int vf;
	bool free;
	bool is_macvlan;
	u8 vf_macvlan[ETH_ALEN];
};

#define TXGBE_MAX_TXD_PWR       14
#define TXGBE_MAX_DATA_PER_TXD  BIT(TXGBE_MAX_TXD_PWR)

/* Tx Descriptors needed, worst case */
#define TXD_USE_COUNT(S)        DIV_ROUND_UP((S), TXGBE_MAX_DATA_PER_TXD)
#define DESC_NEEDED             (MAX_SKB_FRAGS + 4)

#define DESC_RESERVED        96
#define DESC_RESERVED_AML    192

/* wrapper around a pointer to a socket buffer,
 * so a DMA handle can be stored along with the buffer
 */
struct txgbe_tx_buffer {
	union txgbe_tx_desc *next_to_watch;
	u32 next_eop;
	unsigned long time_stamp;
	union {
		struct sk_buff *skb;
		/* XDP uses address ptr on irq_clean */
		struct xdp_frame *xdpf;
	};
	unsigned int bytecount;
	unsigned short gso_segs;
	__be16 protocol;
	DEFINE_DMA_UNMAP_ADDR(dma);
	DEFINE_DMA_UNMAP_LEN(len);
	void *va;
	u32 tx_flags;
};

struct txgbe_rx_buffer {
	struct sk_buff *skb;
	dma_addr_t dma;
	dma_addr_t page_dma;
	union{
		struct {
		struct page *page;
		unsigned int page_offset;
		u16 pagecnt_bias;
		};
		struct {
			bool discard;
			struct xdp_buff *xdp;
		};
	};
};

struct txgbe_queue_stats {
	u64 packets;
	u64 bytes;
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
	__TXGBE_RX_3K_BUFFER,
	__TXGBE_RX_BUILD_SKB_ENABLED,
	__TXGBE_TX_FDIR_INIT_DONE,
	__TXGBE_TX_XPS_INIT_DONE,
	__TXGBE_TX_DETECT_HANG,
	__TXGBE_HANG_CHECK_ARMED,
	__TXGBE_RX_HS_ENABLED,
	__TXGBE_RX_RSC_ENABLED,
	__TXGBE_TX_XDP_RING,
#if IS_ENABLED(CONFIG_FCOE)
	__TXGBE_RX_FCOE,
#endif
	__TXGBE_TX_DISABLED,
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
#define ring_is_rsc_enabled(ring) \
	test_bit(__TXGBE_RX_RSC_ENABLED, &(ring)->state)
#define set_ring_rsc_enabled(ring) \
	set_bit(__TXGBE_RX_RSC_ENABLED, &(ring)->state)
#define clear_ring_rsc_enabled(ring) \
	clear_bit(__TXGBE_RX_RSC_ENABLED, &(ring)->state)
#define ring_is_xdp(ring) \
	test_bit(__TXGBE_TX_XDP_RING, &(ring)->state)
#define set_ring_xdp(ring) \
	set_bit(__TXGBE_TX_XDP_RING, &(ring)->state)
#define clear_ring_xdp(ring) \
	clear_bit(__TXGBE_TX_XDP_RING, &(ring)->state)

struct txgbe_ring {
	struct txgbe_ring *next;        /* pointer to next ring in q_vector */
	struct txgbe_q_vector *q_vector; /* backpointer to host q_vector */
	struct net_device *netdev;      /* netdev ring belongs to */
	struct device *dev;             /* device for DMA mapping */
	struct bpf_prog *xdp_prog;
	struct txgbe_fwd_adapter *accel;
	void *desc;                     /* descriptor ring memory */
	union {
		struct txgbe_tx_buffer *tx_buffer_info;
		struct txgbe_rx_buffer *rx_buffer_info;
	};
	spinlock_t tx_lock;		/* used in XDP mode */
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
	u16 next_to_free;
	u16 rx_offset;

	unsigned long last_rx_timestamp;

	u16 rx_buf_len;
	union {
		union {
			u16 next_to_alloc;
			u16 next_rs_idx;
		};
		struct {
			u8 atr_sample_rate;
			u8 atr_count;
		};
	};

	u16 xdp_tx_active;

	u8 dcb_tc;
	struct txgbe_queue_stats stats;
	struct u64_stats_sync syncp;
	union {
		struct txgbe_tx_queue_stats tx_stats;
		struct txgbe_rx_queue_stats rx_stats;
	};

	struct xdp_rxq_info xdp_rxq;
	struct xsk_buff_pool *xsk_pool;

	dma_addr_t headwb_dma;
	u32 *headwb_mem;
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
#define TXGBE_MAX_XDP_RSS_INDICES	32
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
#define MAX_XDP_QUEUES  32

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

#define TXGBE_RSS_64Q_MASK      0x3F
#define TXGBE_RSS_16Q_MASK      0xF
#define TXGBE_RSS_8Q_MASK       0x7
#define TXGBE_RSS_4Q_MASK       0x3
#define TXGBE_RSS_2Q_MASK       0x1
#define TXGBE_RSS_DISABLED_MASK 0x0

#if (PAGE_SIZE < 8192)
#define TXGBE_MAX_2K_FRAME_BUILD_SKB (TXGBE_RXBUFFER_1536 - NET_IP_ALIGN)
#define TXGBE_2K_TOO_SMALL_WITH_PADDING \
((NET_SKB_PAD + TXGBE_RXBUFFER_1536) > SKB_WITH_OVERHEAD(TXGBE_RXBUFFER_2K))

static inline int txgbe_compute_pad(int rx_buf_len)
{
	int page_size, pad_size;

	page_size = ALIGN(rx_buf_len, PAGE_SIZE / 2);
	pad_size = SKB_WITH_OVERHEAD(page_size) - rx_buf_len;

	return pad_size;
}

static inline int txgbe_skb_pad(void)
{
	int rx_buf_len;

	/* If a 2K buffer cannot handle a standard Ethernet frame then
	 * optimize padding for a 3K buffer instead of a 1.5K buffer.
	 *
	 * For a 3K buffer we need to add enough padding to allow for
	 * tailroom due to NET_IP_ALIGN possibly shifting us out of
	 * cache-line alignment.
	 */
	if (TXGBE_2K_TOO_SMALL_WITH_PADDING)
		rx_buf_len = TXGBE_RXBUFFER_3K + SKB_DATA_ALIGN(NET_IP_ALIGN);
	else
		rx_buf_len = TXGBE_RXBUFFER_1536;

	/* if needed make room for NET_IP_ALIGN */
	rx_buf_len -= NET_IP_ALIGN;

	return txgbe_compute_pad(rx_buf_len);
}

#define TXGBE_SKB_PAD	txgbe_skb_pad()
#else
#define TXGBE_SKB_PAD	(NET_SKB_PAD + NET_IP_ALIGN)
#endif

/* FCoE requires that all Rx buffers be over 2200 bytes in length.  Since
 * this is twice the size of a half page we need to double the page order
 * for FCoE enabled Rx queues.
 */
static inline unsigned int txgbe_rx_bufsz(struct txgbe_ring __maybe_unused *ring)
{
#if MAX_SKB_FRAGS < 8
	return ALIGN(TXGBE_MAX_RXBUFFER / MAX_SKB_FRAGS, 1024);
#else
	if (test_bit(__TXGBE_RX_3K_BUFFER, &ring->state))
		return TXGBE_RXBUFFER_3K;
#if (PAGE_SIZE < 8192)
	if (ring_uses_build_skb(ring))
		return TXGBE_MAX_2K_FRAME_BUILD_SKB;
#endif
	return TXGBE_RXBUFFER_2K;
#endif
}

static inline unsigned int txgbe_rx_pg_order(struct txgbe_ring __maybe_unused *ring)
{
#if (PAGE_SIZE < 8192)
	if (test_bit(__TXGBE_RX_3K_BUFFER, &ring->state))
		return 1;
#endif
	return 0;
}

#define txgbe_rx_pg_size(_ring) (PAGE_SIZE << txgbe_rx_pg_order(_ring))

static inline unsigned int txgbe_rx_offset(struct txgbe_ring *rx_ring)
{
	return ring_uses_build_skb(rx_ring) ? TXGBE_SKB_PAD : 0;
}

struct txgbe_ring_container {
	struct txgbe_ring *ring;        /* pointer to linked list of rings */
	unsigned long next_update;      /* jiffies value of last update */
	unsigned int total_bytes;       /* total bytes processed this int */
	unsigned int total_packets;     /* total packets processed this int */
	u16 work_limit;                 /* total work allowed per interrupt */
	u8 count;                       /* total number of rings in vector */
	u8 itr;                         /* current ITR setting for ring */
};

/* iterator for handling rings in ring container */
#define txgbe_for_each_ring(pos, head) \
	for (pos = (head).ring; pos; pos = pos->next)

#define MAX_RX_PACKET_BUFFERS   ((adapter->flags & TXGBE_FLAG_DCB_ENABLED) \
				 ? 8 : 1)
#define MAX_TX_PACKET_BUFFERS   MAX_RX_PACKET_BUFFERS

/* MAX_MSIX_Q_VECTORS of these are allocated,
 * but we only use one per queue-specific vector.
 */
struct txgbe_q_vector {
	struct txgbe_adapter *adapter;
	int cpu;
	u16 v_idx;
	u16 itr;
	struct txgbe_ring_container rx, tx;

	struct napi_struct napi;
	cpumask_t affinity_mask;

	int numa_node;
	struct rcu_head rcu;
	char name[IFNAMSIZ + 17];
	bool netpoll_rx;

	/* for dynamic allocation of rings associated with this q_vector */
	struct txgbe_ring ring[0] ____cacheline_internodealigned_in_smp;
};

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

/* microsecond values for various ITR rates shifted by 2 to fit itr register
 * with the first 3 bits reserved 0
 */
#define TXGBE_MIN_RSC_ITR       24
#define TXGBE_100K_ITR          40
#define TXGBE_20K_ITR           200
#define TXGBE_16K_ITR           248
#define TXGBE_12K_ITR           336

#define TXGBE_ITR_ADAPTIVE_MIN_INC	2
#define TXGBE_ITR_ADAPTIVE_MIN_USECS	10
#define TXGBE_ITR_ADAPTIVE_MAX_USECS	84
#define TXGBE_ITR_ADAPTIVE_LATENCY	0x80
#define TXGBE_ITR_ADAPTIVE_BULK		0x00
#define TXGBE_ITR_ADAPTIVE_MASK_USECS	(TXGBE_ITR_ADAPTIVE_LATENCY - \
					 TXGBE_ITR_ADAPTIVE_MIN_INC)

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

/* Only for array allocations in our adapter struct.
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

#define TXGBE_FLAG_MSI_CAPABLE                  BIT(0)
#define TXGBE_FLAG_MSI_ENABLED                  BIT(1)
#define TXGBE_FLAG_MSIX_CAPABLE                 BIT(2)
#define TXGBE_FLAG_MSIX_ENABLED                 BIT(3)
#define TXGBE_FLAG_LLI_PUSH                     BIT(4)
#define TXGBE_FLAG_IPSEC_ENABLED                BIT(5)
#define TXGBE_FLAG_TPH_ENABLED                  BIT(6)
#define TXGBE_FLAG_TPH_CAPABLE                  BIT(7)
#define TXGBE_FLAG_TPH_ENABLED_DATA             BIT(8)
#define TXGBE_FLAG_MQ_CAPABLE                   BIT(9)
#define TXGBE_FLAG_DCB_ENABLED                  BIT(10)
#define TXGBE_FLAG_VMDQ_ENABLED                 BIT(11)
#define TXGBE_FLAG_FAN_FAIL_CAPABLE             BIT(12)
#define TXGBE_FLAG_NEED_LINK_UPDATE             BIT(13)
#define TXGBE_FLAG_NEED_LINK_CONFIG             BIT(14)
#define TXGBE_FLAG_FDIR_HASH_CAPABLE            BIT(15)
#define TXGBE_FLAG_FDIR_PERFECT_CAPABLE         BIT(16)
#define TXGBE_FLAG_FCOE_CAPABLE                 BIT(17)
#define TXGBE_FLAG_FCOE_ENABLED                 BIT(18)
#define TXGBE_FLAG_SRIOV_CAPABLE                BIT(19)
#define TXGBE_FLAG_SRIOV_ENABLED                BIT(20)
#define TXGBE_FLAG_SRIOV_REPLICATION_ENABLE     BIT(21)
#define TXGBE_FLAG_SRIOV_L2SWITCH_ENABLE        BIT(22)
#define TXGBE_FLAG_SRIOV_VEPA_BRIDGE_MODE       BIT(23)
#define TXGBE_FLAG_RX_HWTSTAMP_ENABLED          BIT(24)
#define TXGBE_FLAG_VXLAN_OFFLOAD_CAPABLE        BIT(25)
#define TXGBE_FLAG_VXLAN_OFFLOAD_ENABLE         BIT(26)
#define TXGBE_FLAG_RX_HWTSTAMP_IN_REGISTER      BIT(27)
#define TXGBE_FLAG_NEED_ETH_PHY_RESET           BIT(28)

#define TXGBE_FLAG_RX_HS_ENABLED                BIT(30)
#define TXGBE_FLAG_LINKSEC_ENABLED              BIT(31)

/* preset defaults */
#define TXGBE_FLAGS_SP_INIT (TXGBE_FLAG_MSI_CAPABLE \
			   | TXGBE_FLAG_MSIX_CAPABLE \
			   | TXGBE_FLAG_MQ_CAPABLE \
			   | TXGBE_FLAG_SRIOV_CAPABLE)

/**
 * txgbe_adapter.flag2
 **/
#define TXGBE_FLAG2_RSC_CAPABLE                 BIT(0)
#define TXGBE_FLAG2_RSC_ENABLED                 BIT(1)
#define TXGBE_FLAG2_TEMP_SENSOR_CAPABLE         BIT(3)
#define TXGBE_FLAG2_TEMP_SENSOR_EVENT           BIT(4)
#define TXGBE_FLAG2_SEARCH_FOR_SFP              BIT(5)
#define TXGBE_FLAG2_SFP_NEEDS_RESET             BIT(6)
#define TXGBE_FLAG2_PF_RESET_REQUESTED          BIT(7)
#define TXGBE_FLAG2_FDIR_REQUIRES_REINIT        BIT(8)
#define TXGBE_FLAG2_RSS_FIELD_IPV4_UDP          BIT(9)
#define TXGBE_FLAG2_RSS_FIELD_IPV6_UDP          BIT(10)
#define TXGBE_FLAG2_RSS_ENABLED                 BIT(12)
#define TXGBE_FLAG2_PTP_PPS_ENABLED             BIT(11)
#define TXGBE_FLAG2_EEE_CAPABLE                 BIT(14)
#define TXGBE_FLAG2_EEE_ENABLED                 BIT(15)
#define TXGBE_FLAG2_VXLAN_REREG_NEEDED          BIT(16)
#define TXGBE_FLAG2_VLAN_PROMISC                BIT(17)
#define TXGBE_FLAG2_DEV_RESET_REQUESTED         BIT(18)
#define TXGBE_FLAG2_RESET_INTR_RECEIVED         BIT(19)
#define TXGBE_FLAG2_GLOBAL_RESET_REQUESTED      BIT(20)
#define TXGBE_FLAG2_CLOUD_SWITCH_ENABLED        BIT(21)
#define TXGBE_FLAG2_MNG_REG_ACCESS_DISABLED     BIT(22)
#define TXGBE_FLAG2_KR_TRAINING                 BIT(24)
#define TXGBE_FLAG2_KR_AUTO                     BIT(25)
#define TXGBE_FLAG2_LINK_DOWN                   BIT(26)
#define TXGBE_FLAG2_KR_PRO_DOWN                 BIT(27)
#define TXGBE_FLAG2_KR_PRO_REINIT               BIT(28)
#define TXGBE_FLAG2_ECC_ERR_RESET               BIT(29)
#define TXGBE_FLAG2_RX_LEGACY					BIT(30)
#define TXGBE_FLAG2_PCIE_NEED_RECOVER           BIT(31)
#define TXGBE_FLAG2_PCIE_NEED_Q_RESET           BIT(30)
#define TXGBE_FLAG2_SERVICE_RUNNING             BIT(13)

/* amlite: dma reset */
#define TXGBE_FLAG2_DMA_RESET_REQUESTED         BIT(2)

#define TXGBE_FLAG3_PHY_EVENT                   BIT(0)
#define TXGBE_FLAG3_TEMP_SENSOR_INPROGRESS      BIT(1)

#define TXGBE_SET_FLAG(_input, _flag, _result) \
	(((_flag) <= (_result)) ? \
	 ((u32)((_input) & (_flag)) * ((_result) / (_flag))) : \
	 ((u32)((_input) & (_flag)) / ((_flag) / (_result))))

enum txgbe_isb_idx {
	TXGBE_ISB_HEADER,
	TXGBE_ISB_MISC,
	TXGBE_ISB_VEC0,
	TXGBE_ISB_VEC1,
	TXGBE_ISB_MAX
};

#define TXGBE_PHY_FEC_RS	BIT(0)
#define TXGBE_PHY_FEC_BASER	BIT(1)
#define TXGBE_PHY_FEC_OFF	BIT(2)
#define TXGBE_PHY_FEC_AUTO (TXGBE_PHY_FEC_OFF | TXGBE_PHY_FEC_BASER |\
			   TXGBE_PHY_FEC_RS)

/* board specific private data structure */
struct txgbe_adapter {
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];

	/* OS defined structs */
	struct net_device *netdev;
	struct bpf_prog *xdp_prog;
	struct pci_dev *pdev;

	unsigned long state;
	u32 bp_link_mode;
	u32 curbp_link_mode;
	/* Some features need tri-state capability,
	 * thus the additional *_CAPABLE flags.
	 */
	u32 flags;
	u32 flags2;
	u32 flags3;
	u8 tx_unidir_mode;
	u8  an73_mode;
	u8  backplane_an;
	u8  an73;
	u8  autoneg;
	u16 ffe_main;
	u16 ffe_pre;
	u16 ffe_post;
	u8  ffe_set;
	u16 fec_mode;
	u8  backplane_mode;
	u8  backplane_auto;
	struct phytxeq aml_txeq;
	bool an_done;
	u32 fsm;

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

	bool lro_before_xdp;
	u16 old_rss_limit;
	/* XDP */
	int num_xdp_queues;
	struct txgbe_ring *xdp_ring[MAX_XDP_QUEUES];
	unsigned long *af_xdp_zc_qps;

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

	struct ieee_pfc *txgbe_ieee_pfc;
	struct ieee_ets *txgbe_ieee_ets;
	struct txgbe_dcb_config dcb_cfg;
	struct txgbe_dcb_config temp_dcb_cfg;
	u8 dcb_set_bitmap;
	u8 dcbx_cap;
	u8 dcb_tc;

	enum txgbe_fc_mode last_lfc_mode;

	int num_q_vectors;      /* current number of q_vectors for device */
	int max_q_vectors;      /* upper limit of q_vectors for device */
	struct txgbe_ring_feature ring_feature[RING_F_ARRAY_SIZE];
	struct msix_entry *msix_entries;
	struct net_device_stats net_stats;

	u64 test_icr;
	struct txgbe_ring test_tx_ring;
	struct txgbe_ring test_rx_ring;

	/* structs defined in txgbe_hw.h */
	struct txgbe_hw hw;
	u16 msg_enable;
	struct txgbe_hw_stats stats;
	u32 lli_port;
	u32 lli_size;
	u32 lli_etype;
	u32 lli_vlan_pri;

	u32 *config_space;
	u64 tx_busy;
	unsigned int tx_ring_count;
	unsigned int xdp_ring_count;
	unsigned int rx_ring_count;

	u32 link_speed;
	u32 speed;
	bool link_up;
	unsigned long sfp_poll_time;
	unsigned long link_check_timeout;

	/* mutex for amblite phy access */
	struct mutex e56_lock;

	struct timer_list service_timer;
	struct work_struct service_task;
	struct work_struct sfp_sta_task;
	struct work_struct temp_task;
	struct hlist_head fdir_filter_list;
	unsigned long fdir_overflow; /* number of times ATR was backed off */
	union txgbe_atr_input fdir_mask;
	int fdir_filter_count;
	u32 fdir_pballoc;
	u32 atr_sample_rate;

	/* spinlock for flow director */
	spinlock_t fdir_perfect_lock;

	struct txgbe_etype_filter_info etype_filter_info;
	struct txgbe_5tuple_filter_info ft_filter_info;

#if IS_ENABLED(CONFIG_FCOE)
	struct txgbe_fcoe fcoe;
#endif /* CONFIG_FCOE */
	u8 __iomem *io_addr;
	u32 wol;

	u16 bd_number;

	u16 bridge_mode;

	u8 fec_link_mode;
	u8 cur_fec_link;
	bool link_valid;
	u32 etrack_id;
	char eeprom_id[32];
	char fl_version[16];
	char fw_version[64];
	bool netdev_registered;
	u32 interrupt_event;
	u32 led_reg;

	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_caps;
	struct work_struct ptp_tx_work;
	struct sk_buff *ptp_tx_skb;
	struct hwtstamp_config tstamp_config;
	unsigned long ptp_tx_start;
	unsigned long last_overflow_check;
	unsigned long last_rx_ptp_check;

	/* ptp spin lock */
	spinlock_t tmreg_lock;
	struct cyclecounter hw_cc;
	struct timecounter hw_tc;
	u32 base_incval;
	u32 tx_hwtstamp_timeouts;
	u32 tx_hwtstamp_skipped;
	u32 rx_hwtstamp_cleared;
	void (*ptp_setup_sdp)(struct txgbe_adapter *adapter);
	u64 pps_edge_start;
	u64 pps_edge_end;
	u64 sec_to_cc;
	u8 pps_enabled;

	DECLARE_BITMAP(active_vfs, TXGBE_MAX_VF_FUNCTIONS);
	unsigned int num_vfs;
	unsigned int max_vfs;
	struct vf_data_storage *vfinfo;
	int vf_rate_link_speed;
	struct vf_macvlans vf_mvs;
	struct vf_macvlans *mv_list;
#ifdef CONFIG_PCI_IOV
	u32 timer_event_accumulator;
	u32 vferr_refcount;
#endif
	struct txgbe_mac_addr *mac_table;
	__le16 vxlan_port;
	__le16 geneve_port;

	struct hwmon_buff txgbe_hwmon_buff;

	struct dentry *txgbe_dbg_adapter;
	u8 default_up;
	unsigned int indices;

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

	u64 eth_priv_flags;
#define TXGBE_ETH_PRIV_FLAG_LLDP		BIT(0)
#define TXGBE_ETH_PRIV_FLAG_LEGACY_RX	BIT(1)

	/* AF_XDP zero-copy */
	struct xsk_buff_pool **xsk_pools;
	u16 num_xsk_pools_used;
	u16 num_xsk_pools;

	bool cmplt_to_dis;
	u8 i2c_eeprom[512];
	u32 eeprom_len;
	u32 eeprom_type;

	/* amlite: new SW-FW mbox */
/*	u32 swfw_mbox_buf[64]; */
	u8 swfw_index;
	u8 desc_reserved;

	int amlite_temp;

	int vlan_rate_link_speed;
	DECLARE_BITMAP(limited_vlans, 4096);
	int active_vlan_limited;
	int queue_rate_limit[64]; // From back to front
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
	if (adapter->xdp_prog)
		return TXGBE_MAX_XDP_RSS_INDICES;
	return TXGBE_MAX_RSS_INDICES;
}

struct txgbe_fdir_filter {
	struct  hlist_node fdir_node;
	union txgbe_atr_input filter;
	u16 sw_idx;
	u64 action;
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
	__TXGBE_PTP_RUNNING,
	__TXGBE_PTP_TX_IN_PROGRESS,
	__TXGBE_SWFW_BUSY,
};

struct txgbe_cb {
	dma_addr_t dma;

	u16     append_cnt;             /* number of skb's appended */
	bool    page_released;
	bool    dma_released;
};

#define TXGBE_CB(skb) ((struct txgbe_cb *)(skb)->cb)

#define TXGBE_1588_PPS_WIDTH 100
#define TXGBE_1588_TOD_ENABLE 1
#define TXGBE_1588_PPS_LEVEL 1

/* ESX txgbe CIM IOCTL definition */
void txgbe_sysfs_exit(struct txgbe_adapter *adapter);
int txgbe_sysfs_init(struct txgbe_adapter *adapter);

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

void txgbe_service_event_schedule(struct txgbe_adapter *adapter);
void txgbe_irq_disable(struct txgbe_adapter *adapter);
void txgbe_irq_enable(struct txgbe_adapter *adapter, bool queues, bool flush);
int txgbe_open(struct net_device *netdev);
int txgbe_close(struct net_device *netdev);
void txgbe_up(struct txgbe_adapter *adapter);
void txgbe_down(struct txgbe_adapter *adapter);
void txgbe_reinit_locked(struct txgbe_adapter *adapter);
void txgbe_reset(struct txgbe_adapter *adapter);
void txgbe_set_ethtool_ops(struct net_device *netdev);
int txgbe_setup_rx_resources(struct txgbe_ring *ring);
int txgbe_setup_tx_resources(struct txgbe_ring *ring);
void txgbe_free_rx_resources(struct txgbe_ring *ring);
void txgbe_free_tx_resources(struct txgbe_ring *ring);
void txgbe_configure_rx_ring(struct txgbe_adapter *adapter,
			     struct txgbe_ring *ring);
void txgbe_configure_tx_ring(struct txgbe_adapter *adapter,
			     struct txgbe_ring *ring);
void txgbe_update_stats(struct txgbe_adapter *adapter);
int txgbe_init_interrupt_scheme(struct txgbe_adapter *adapter);
void txgbe_reset_interrupt_capability(struct txgbe_adapter *adapter);
void txgbe_set_interrupt_capability(struct txgbe_adapter *adapter);
void txgbe_clear_interrupt_scheme(struct txgbe_adapter *adapter);
bool txgbe_is_txgbe(struct pci_dev *pcidev);
netdev_tx_t txgbe_xmit_frame_ring(struct sk_buff *skb,
				  struct txgbe_adapter *adapter,
					 struct txgbe_ring *ring);
void txgbe_unmap_and_free_tx_resource(struct txgbe_ring *ring,
				      struct txgbe_tx_buffer *tx_buffer);
bool txgbe_alloc_rx_buffers(struct txgbe_ring *rx_ring, u16 cleaned_count);
void txgbe_configure_rscctl(struct txgbe_adapter *adapter,
			    struct txgbe_ring *ring);
void txgbe_clear_rscctl(struct txgbe_adapter *adapter,
			struct txgbe_ring *ring);
void txgbe_clear_vxlan_port(struct txgbe_adapter *adapter);
void txgbe_set_rx_mode(struct net_device *netdev);
int txgbe_write_mc_addr_list(struct net_device *netdev);
int txgbe_setup_tc(struct net_device *dev, u8 tc);
void txgbe_tx_ctxtdesc(struct txgbe_ring *tx_ring, u32 vlan_macip_lens,
		       u32 fcoe_sof_eof, u32 type_tucmd, u32 mss_l4len_idx);
void txgbe_do_reset(struct net_device *netdev);
void txgbe_write_eitr(struct txgbe_q_vector *q_vector);
int txgbe_poll(struct napi_struct *napi, int budget);
void txgbe_disable_rx_queue(struct txgbe_adapter *adapter,
			    struct txgbe_ring *ring);
void txgbe_vlan_strip_enable(struct txgbe_adapter *adapter);
void txgbe_vlan_strip_disable(struct txgbe_adapter *adapter);
void txgbe_print_tx_hang_status(struct txgbe_adapter *adapter);

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

int txgbe_fcoe_ddp_target(struct net_device *netdev, u16 xid,
			  struct scatterlist *sgl, unsigned int sgc);

int txgbe_fcoe_ddp_put(struct net_device *netdev, u16 xid);
int txgbe_setup_fcoe_ddp_resources(struct txgbe_adapter *adapter);
void txgbe_free_fcoe_ddp_resources(struct txgbe_adapter *adapter);
int txgbe_fcoe_enable(struct net_device *netdev);
int txgbe_fcoe_disable(struct net_device *netdev);
#endif /* CONFIG_FCOE */
#if IS_ENABLED(CONFIG_DCB)
u8 txgbe_fcoe_getapp(struct net_device *netdev);
u8 txgbe_fcoe_get_tc(struct txgbe_adapter *adapter);
int txgbe_fcoe_get_wwn(struct net_device *netdev, u64 *wwn, int type);
int txgbe_fcoe_ddp_enable(struct txgbe_adapter *adapter);
void txgbe_fcoe_ddp_disable(struct txgbe_adapter *adapter);
#endif /* CONFIG_FCOE */

void txgbe_dbg_adapter_init(struct txgbe_adapter *adapter);
void txgbe_dbg_adapter_exit(struct txgbe_adapter *adapter);
void txgbe_dbg_init(void);
void txgbe_dbg_exit(void);
void txgbe_dump(struct txgbe_adapter *adapter);
void txgbe_setup_reta(struct txgbe_adapter *adapter);

static inline struct netdev_queue *txring_txq(const struct txgbe_ring *ring)
{
	return netdev_get_tx_queue(ring->netdev, ring->queue_index);
}

#if IS_ENABLED(CONFIG_DCB)
s32 txgbe_dcb_hw_ets(struct txgbe_hw *hw, struct ieee_ets *ets, int max_frame);
#endif /* CONFIG_DCB */

int txgbe_wol_supported(struct txgbe_adapter *adapter);
int txgbe_get_settings(struct net_device *netdev,
		       struct ethtool_cmd *ecmd);
int txgbe_write_uc_addr_list(struct net_device *netdev, int pool);
void txgbe_full_sync_mac_table(struct txgbe_adapter *adapter);
int txgbe_add_mac_filter(struct txgbe_adapter *adapter,
			 const u8 *addr, u16 pool);
int txgbe_del_mac_filter(struct txgbe_adapter *adapter,
			 const u8 *addr, u16 pool);
int txgbe_available_rars(struct txgbe_adapter *adapter);
void txgbe_vlan_mode(struct net_device *netdev, u32 features);
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

#ifdef CONFIG_PCI_IOV
void txgbe_sriov_reinit(struct txgbe_adapter *adapter);
#endif

void txgbe_set_rx_drop_en(struct txgbe_adapter *adapter);

u32 txgbe_rss_indir_tbl_entries(struct txgbe_adapter *adapter);
void txgbe_store_reta(struct txgbe_adapter *adapter);
void txgbe_store_vfreta(struct txgbe_adapter *adapter);

int txgbe_setup_isb_resources(struct txgbe_adapter *adapter);
void txgbe_free_isb_resources(struct txgbe_adapter *adapter);
void txgbe_configure_isb(struct txgbe_adapter *adapter);

void txgbe_clean_tx_ring(struct txgbe_ring *tx_ring);
void txgbe_clean_rx_ring(struct txgbe_ring *rx_ring);
u32 txgbe_tx_cmd_type(u32 tx_flags);
void txgbe_free_headwb_resources(struct txgbe_ring *ring);
u16 txgbe_frac_to_bi(u16 frac, u16 denom, int max_bits);
int txgbe_link_mbps(struct txgbe_adapter *adapter);

int txgbe_find_nth_limited_vlan(struct txgbe_adapter *adapter, int vlan);
void txgbe_del_vlan_limit(struct txgbe_adapter *adapter, int vlan);
void txgbe_set_vlan_limit(struct txgbe_adapter *adapter, int vlan, int rate_limit);
void txgbe_check_vlan_rate_limit(struct txgbe_adapter *adapter);

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

#define TXGBE_CPU_TO_BE16(_x) cpu_to_be16(_x)
#define TXGBE_BE16_TO_CPU(_x) be16_to_cpu(_x)
#define TXGBE_CPU_TO_BE32(_x) cpu_to_be32(_x)
#define TXGBE_BE32_TO_CPU(_x) be32_to_cpu(_x)

#define msec_delay(_x) msleep(_x)
#define usec_delay(_x) udelay(_x)

#define TXGBE_NAME "txgbe"

struct txgbe_hw;
struct txgbe_msg {
	u16 msg_enable;
};

static inline struct device *pci_dev_to_dev(struct pci_dev *pdev)
{
	return &pdev->dev;
}

struct net_device *txgbe_hw_to_netdev(const struct txgbe_hw *hw);
struct txgbe_msg *txgbe_hw_to_msg(const struct txgbe_hw *hw);

#define hw_dbg(hw, format, arg...) \
	netdev_dbg(txgbe_hw_to_netdev(hw), format, ## arg)
#define hw_err(hw, format, arg...) \
	netdev_err(txgbe_hw_to_netdev(hw), format, ## arg)
#define e_dev_info(format, arg...) \
	dev_info(pci_dev_to_dev(adapter->pdev), format, ## arg)
#define e_dev_warn(format, arg...) \
	dev_warn(pci_dev_to_dev(adapter->pdev), format, ## arg)
#define e_dev_err(format, arg...) \
	dev_err(pci_dev_to_dev(adapter->pdev), format, ## arg)
#define e_dev_notice(format, arg...) \
	dev_notice(pci_dev_to_dev(adapter->pdev), format, ## arg)
#define e_dbg(msglvl, format, arg...) \
	netif_dbg(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_info(msglvl, format, arg...) \
	netif_info(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_err(msglvl, format, arg...) \
	netif_err(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_warn(msglvl, format, arg...) \
	netif_warn(adapter, msglvl, adapter->netdev, format, ## arg)
#define e_crit(msglvl, format, arg...) \
	netif_crit(adapter, msglvl, adapter->netdev, format, ## arg)

#define TXGBE_FAILED_READ_CFG_DWORD 0xffffffffU
#define TXGBE_FAILED_READ_CFG_WORD  0xffffU
#define TXGBE_FAILED_READ_CFG_BYTE  0xffU

u32 txgbe_read_reg(struct txgbe_hw *hw, u32 reg, bool quiet);
u16 txgbe_read_pci_cfg_word(struct txgbe_hw *hw, u32 reg);
void txgbe_write_pci_cfg_word(struct txgbe_hw *hw, u32 reg, u16 value);

#define TXGBE_READ_PCIE_WORD txgbe_read_pci_cfg_word
#define TXGBE_WRITE_PCIE_WORD txgbe_write_pci_cfg_word
#define TXGBE_R32_Q(h, r) txgbe_read_reg(h, r, true)

#define TXGBE_EEPROM_GRANT_ATTEMPS 100
#define TXGBE_HTONL(_i) htonl(_i)
#define TXGBE_NTOHL(_i) ntohl(_i)
#define TXGBE_NTOHS(_i) ntohs(_i)
#define TXGBE_CPU_TO_LE32(_i) cpu_to_le32(_i)
#define TXGBE_LE32_TO_CPUS(_i) le32_to_cpus(_i)

enum {
	TXGBE_ERROR_SOFTWARE,
	TXGBE_ERROR_POLLING,
	TXGBE_ERROR_INVALID_STATE,
	TXGBE_ERROR_UNSUPPORTED,
	TXGBE_ERROR_ARGUMENT,
	TXGBE_ERROR_CAUTION,
};

#define ERROR_REPORT(level, format, arg...) do {                               \
	switch (level) {                                                       \
	case TXGBE_ERROR_SOFTWARE:                                             \
	case TXGBE_ERROR_CAUTION:                                              \
	case TXGBE_ERROR_POLLING:                                              \
		netif_warn(txgbe_hw_to_msg(hw), drv, txgbe_hw_to_netdev(hw),   \
			   format, ## arg);                                    \
		break;                                                         \
	case TXGBE_ERROR_INVALID_STATE:                                        \
	case TXGBE_ERROR_UNSUPPORTED:                                          \
	case TXGBE_ERROR_ARGUMENT:                                             \
		netif_err(txgbe_hw_to_msg(hw), hw, txgbe_hw_to_netdev(hw),     \
			  format, ## arg);                                     \
		break;                                                         \
	default:                                                               \
		break;                                                         \
	}                                                                      \
} while (0)

#define ERROR_REPORT1 ERROR_REPORT
#define ERROR_REPORT2 ERROR_REPORT
#define ERROR_REPORT3 ERROR_REPORT

#endif /* _TXGBE_H_ */
