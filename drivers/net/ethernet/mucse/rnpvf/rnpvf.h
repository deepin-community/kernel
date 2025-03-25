// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#ifndef _RNPVF_H_
#define _RNPVF_H_

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/timer.h>
#include <linux/io.h>
#include <linux/netdevice.h>
#include <linux/if_vlan.h>

#include "vf.h"

#define RNPVF_ALLOC_PAGE_ORDER 0
#define RNPVF_PAGE_BUFFER_NUMS(ring) \
	(((1 << RNPVF_ALLOC_PAGE_ORDER) * PAGE_SIZE) >> 11)

#define RNPVF_RX_DMA_ATTR (DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)

#if defined(CONFIG_MXGBEVF_FIX_VF_QUEUE) && !defined(FIX_VF_QUEUE)
#define FIX_VF_BUG
#endif

#if defined(CONFIG_MXGBEVF_FIX_MAC_PADDING) && !defined(FIX_MAC_PADDING)
#define FIX_MAC_PADDIN
#endif

#if defined(CONFIG_MXGBEVF_OPTM_WITH_LARGE) && !defined(OPTM_WITH_LARGE)
#define OPTM_WITH_LPAGE
#endif

#if (PAGE_SIZE < 8192)
//error
#ifdef OPTM_WITH_LPAGE
//#error can't open OPTM_WITH_LPAGE with PAGE_SIZE small than 8192
#undef OPTM_WITH_LPAGE
#endif
#endif

struct rnpvf_queue_stats {
	u64 packets;
	u64 bytes;
};

struct rnpvf_tx_queue_stats {
	u64 restart_queue;
	u64 tx_busy;
	u64 tx_done_old;
	u64 clean_desc;
	u64 poll_count;
	u64 irq_more_count;
	u64 vlan_add;
	u64 tx_irq_miss;
	u64 tx_next_to_clean;
	u64 tx_equal_count;
};

struct rnpvf_rx_queue_stats {
	u64 driver_drop_packets;
	u64 rsc_count;
	u64 rsc_flush;
	u64 non_eop_descs;
	u64 alloc_rx_page_failed;
	u64 alloc_rx_buff_failed;
	u64 alloc_rx_page;
	u64 csum_err;
	u64 csum_good;
	u64 poll_again_count;
	u64 poll_count;
	u64 vlan_remove;
	u64 rx_irq_miss;
	u64 rx_next_to_clean;
	u64 rx_equal_count;
};

/* wrapper around a pointer to a socket buffer,
 * so a DMA handle can be stored along with the Buffers
 */
struct rnpvf_tx_buffer {
	struct rnp_tx_desc *next_to_watch;
	unsigned long time_stamp;
	struct sk_buff *skb;
	unsigned int bytecount;
	unsigned short gso_segs;
	bool gso_need_padding;

	__be16 protocol;
	DEFINE_DMA_UNMAP_ADDR(dma);
	DEFINE_DMA_UNMAP_LEN(len);
	union {
		u32 tx_flags;
		struct {
			u16 vlan;
			u16 cmd_flags;
		};
	};
	__le32 mac_ip_len;
	/* for control desc */
	union {
		u32 mss_len_vf_num;
		struct {
			__le16 mss_len;
			u8 vf_num;
			u8 l4_hdr_len;
		};
	};
	union {
		u32 inner_vlan_tunnel_len;
		struct {
			u8 tunnel_hdr_len;
			u8 inner_vlan_l;
			u8 inner_vlan_h;
			u8 resv;
		};
	};
	bool ctx_flag;
};

struct rnpvf_rx_buffer {
	struct sk_buff *skb;
	dma_addr_t dma;
	struct page *page;
#if (BITS_PER_LONG > 32) || (PAGE_SIZE >= 65536)
	__u32 page_offset;
#else
	__u16 page_offset;
#endif
	__u16 pagecnt_bias;
};

enum rnpvf_ring_state_t {
	__RNPVF_RX_3K_BUFFER,
	__RNPVF_RX_BUILD_SKB_ENABLED,
	__RNPVF_TX_FDIR_INIT_DONE,
	__RNPVF_TX_XPS_INIT_DONE,
	__RNPVF_TX_DETECT_HANG,
	__RNPVF_HANG_CHECK_ARMED,
	__RNPVF_RX_CSUM_UDP_ZERO_ERR,
	__RNPVF_RX_FCOE,
};

#define ring_uses_build_skb(ring) \
	test_bit(__RNPVF_RX_BUILD_SKB_ENABLED, &(ring)->state)

/* now tx max 4k for one desc */
#define RNPVF_MAX_TXD_PWR 12
#define RNPVF_MAX_DATA_PER_TXD (1 << RNPVF_MAX_TXD_PWR)
/* Tx Descriptors needed, worst case */
#define TXD_USE_COUNT(S) DIV_ROUND_UP((S), RNPVF_MAX_DATA_PER_TXD)
#define DESC_NEEDED (MAX_SKB_FRAGS + 4)

struct rnpvf_ring {
	struct rnpvf_ring *next; /* pointer to next ring in q_vector */
	struct rnpvf_q_vector *q_vector; /* backpointer to host q_vector */
	struct net_device *netdev; /* netdev ring belongs to */
	struct device *dev; /* device for DMA mapping */
	void *desc; /* descriptor ring memory */
	union {
		struct rnpvf_tx_buffer *tx_buffer_info;
		struct rnpvf_rx_buffer *rx_buffer_info;
	};
	unsigned long last_rx_timestamp;
	unsigned long state;
	u8 __iomem *ring_addr;
	u8 __iomem *hw_addr;
	u8 __iomem *tail;
	u8 __iomem *dma_int_stat;
	u8 __iomem *dma_int_mask;
	u8 __iomem *dma_int_clr;
	dma_addr_t dma; /* phys. address of descriptor ring */
	unsigned int size; /* length in bytes */
	u32 ring_flags;
#define RNPVF_RING_FLAG_DELAY_SETUP_RX_LEN ((u32)(1 << 0))
#define RNPVF_RING_FLAG_CHANGE_RX_LEN ((u32)(1 << 1))
#define RNPVF_RING_FLAG_DO_RESET_RX_LEN ((u32)(1 << 2))
#define RNPVF_RING_SKIP_TX_START ((u32)(1 << 3))
#define RNPVF_RING_NO_TUNNEL_SUPPORT ((u32)(1 << 4))
#define RNPVF_RING_SIZE_CHANGE_FIX ((u32)(1 << 5))
#define RNPVF_RING_SCATER_SETUP ((u32)(1 << 6))
#define RNPVF_RING_STAGS_SUPPORT ((u32)(1 << 7))
#define RNPVF_RING_DOUBLE_VLAN_SUPPORT ((u32)(1 << 8))
#define RNPVF_RING_VEB_MULTI_FIX ((u32)(1 << 9))
#define RNPVF_RING_IRQ_MISS_FIX ((u32)(1 << 10))
#define RNPVF_RING_CHKSM_FIX ((u32)(1 << 11))

	u8 vfnum;
	u8 rnpvf_msix_off;

	u16 count; /* amount of descriptors */

	u8 queue_index; /* queue_index needed for multiqueue queue management */
	u8 rnpvf_queue_idx;

	u16 next_to_use;
	u16 next_to_clean;

	u16 device_id;
#ifdef OPTM_WITH_LPAGE
	u16 rx_page_buf_nums;
	u32 rx_per_buf_mem;
	struct sk_buff *skb;
#endif
	union {
		u16 next_to_alloc;
		struct {
			u8 atr_sample_rate;
			u8 atr_count;
		};
	};

	u8 dcb_tc;
	struct rnpvf_queue_stats stats;
	struct u64_stats_sync syncp;
	union {
		struct rnpvf_tx_queue_stats tx_stats;
		struct rnpvf_rx_queue_stats rx_stats;
	};
} ____cacheline_internodealigned_in_smp;

#define RNPVF_ITR_ADAPTIVE_MIN_INC 2
#define RNPVF_ITR_ADAPTIVE_MIN_USECS 5
#define RNPVF_ITR_ADAPTIVE_MAX_USECS 800
#define RNPVF_ITR_ADAPTIVE_LATENCY 0x400
#define RNPVF_ITR_ADAPTIVE_BULK 0x00
#define RNPVF_ITR_ADAPTIVE_MASK_USECS \
	(RNPVF_ITR_ADAPTIVE_LATENCY - RNPVF_ITR_ADAPTIVE_MIN_INC)

/* How many Rx Buffers do we bundle into one write to the hardware ? */
#define RNPVF_RX_BUFFER_WRITE 16 /* Must be power of 2 */
#define RNPVF_VF_MAX_TX_QUEUES 32
#define RNPVF_VF_MAX_RX_QUEUES 32
#define MAX_RX_QUEUES RNPVF_VF_MAX_RX_QUEUES
#define MAX_TX_QUEUES RNPVF_VF_MAX_TX_QUEUES

#ifndef RNPVF_PKT_TIMEOUT
#define RNPVF_PKT_TIMEOUT 30
#endif

#ifndef RNPVF_RX_PKT_POLL_BUDGET
#define RNPVF_RX_PKT_POLL_BUDGET 64
#endif

#ifndef RNPVF_TX_PKT_POLL_BUDGET
#define RNPVF_TX_PKT_POLL_BUDGET 0x30
#endif

#ifndef RNPVF_PKT_TIMEOUT_TX
#define RNPVF_PKT_TIMEOUT_TX 100
#endif

#define RNPVF_MIN_RX_WORK (32)
#define RNPVF_DEFAULT_RX_WORK (64)
#define RNPVF_MAX_RX_WORK (512)
#define RNPVF_WORK_ALIGN (2)
#define RNPVF_MIN_TX_FRAME (1)
#define RNPVF_MAX_TX_FRAME (256)
#define RNPVF_MIN_TX_USEC (30)
#define RNPVF_MAX_TX_USEC (10000)

#define RNPVF_MIN_RX_FRAME (1)
#define RNPVF_MAX_RX_FRAME (256)
#define RNPVF_MIN_RX_USEC (10)
#define RNPVF_MAX_RX_USEC (10000)

#define RNPVF_MIN_TX_WORK (32)
#define RNPVF_MAX_TX_WORK (512)
#define RNPVF_DEFAULT_TX_WORK 256
#define RNPVF_DEFAULT_TXD 512
#define RNPVF_DEFAULT_RXD 512
#define RNPVF_MAX_TXD 4096
#define RNPVF_MIN_TXD 256
#define RNPVF_MAX_RXD 4096
#define RNPVF_MIN_RXD 256

#ifndef TSRN10_RX_DEFAULT_BURST
#define TSRN10_RX_DEFAULT_BURST 16
#endif

#ifndef TSRN10_RX_DEFAULT_LINE
#define TSRN10_RX_DEFAULT_LINE 64
#endif

#define TSRN10_TX_DEFAULT_BURST 8

/* Supported Rx Buffer Sizes */
#define RNPVF_RXBUFFER_256 256 /* Used for packet split */
#define RNPVF_RXBUFFER_2K 2048
#define RNPVF_RXBUFFER_1536 1536
#define RNPVF_RXBUFFER_3K 3072
#define RNPVF_RXBUFFER_4K 4096
#define RNPVF_RXBUFFER_8K 8192
#define RNPVF_RXBUFFER_10K 10240

#define RNPVF_RX_HDR_SIZE RNPVF_RXBUFFER_256

#define MAXIMUM_ETHERNET_VLAN_SIZE (VLAN_ETH_FRAME_LEN + ETH_FCS_LEN)

#define RNPVF_TX_FLAGS_CSUM ((u32)(1))
#define RNPVF_TX_FLAGS_VLAN ((u32)(1 << 1))
#define RNPVF_TX_FLAGS_TSO ((u32)(1 << 2))
#define RNPVF_TX_FLAGS_IPV4 ((u32)(1 << 3))
#define RNPVF_TX_FLAGS_FCOE ((u32)(1 << 4))
#define RNPVF_TX_FLAGS_FSO ((u32)(1 << 5))
#define RNPVF_TX_FLAGS_VLAN_MASK 0xffff0000
#define RNPVF_TX_FLAGS_VLAN_PRIO_MASK 0x0000e000
#define RNPVF_TX_FLAGS_VLAN_SHIFT 16

#define RNPVF_GSO_PARTIAL_FEATURES                \
	(NETIF_F_GSO_GRE | NETIF_F_GSO_GRE_CSUM | \
	 NETIF_F_GSO_UDP_TUNNEL | NETIF_F_GSO_UDP_TUNNEL_CSUM)

struct rnpvf_ring_container {
	struct rnpvf_ring *ring; /* pointer to linked list of rings */
	unsigned long next_update; /* jiffies value of last update */
	unsigned int total_bytes; /* total bytes processed this int */
	unsigned int total_packets; /* total packets processed this int */
	unsigned int total_packets_old;
	u8 count; /* total number of rings in vector */
	u8 itr; /* current ITR setting for ring */
	u16 add_itr;
};

/* iterator for handling rings in ring container */
#define rnpvf_for_each_ring(pos, head) \
	for (pos = (head).ring; pos != NULL; pos = pos->next)

/* MAX_MSIX_Q_VECTORS of these are allocated,
 * but we only use one per queue-specific vector.
 */
struct rnpvf_q_vector {
	int old_rx_count;
	int new_rx_count;
	int large_times;
	int small_times;
	int too_small_times;
	int middle_time;
	struct rnpvf_adapter *adapter;
	u16 v_idx;
	/* index of q_vector within array, also used for
	 * finding the bit in EICR and friends that
	 * represents the vector for this rings
	 */
	struct rnpvf_ring_container rx, tx;

	struct napi_struct napi;
	cpumask_t affinity_mask;
	int numa_node;
	u16 itr_rx;
	u16 itr_tx;
	struct rcu_head rcu; /* to avoid race with update stats on free */
	u32 vector_flags;
#define RNPVF_QVECTOR_FLAG_IRQ_MISS_CHECK ((u32)(1 << 0))
#define RNPVF_QVECTOR_FLAG_ITR_FEATURE ((u32)(1 << 1))
#define RNPVF_QVECTOR_FLAG_REDUCE_TX_IRQ_MISS ((u32)(1 << 2))

	int irq_check_usecs;
	struct hrtimer irq_miss_check_timer;

	char name[IFNAMSIZ + 9];

	/* for dynamic allocation of rings associated with this q_vector */
	struct rnpvf_ring ring[0] ____cacheline_internodealigned_in_smp;
};

static inline __le16 rnpvf_test_staterr(union rnp_rx_desc *rx_desc,
					const u16 stat_err_bits)
{
	return rx_desc->wb.cmd & cpu_to_le16(stat_err_bits);
}

static inline __le16 rnpvf_get_stat(union rnp_rx_desc *rx_desc,
				    const u16 stat_mask)
{
	return rx_desc->wb.cmd & cpu_to_le16(stat_mask);
}

static inline u16 rnpvf_desc_unused(struct rnpvf_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->count) + ntc - ntu - 1;
}

/*
 * microsecond values for various ITR rates shifted by 2 to fit itr register
 * with the first 3 bits reserved 0
 */
#define RNPVF_MIN_RSC_ITR 24
#define RNPVF_100K_ITR 40
#define RNPVF_20K_ITR 200
#define RNPVF_10K_ITR 400
#define RNPVF_8K_ITR 500

/* Helper macros to switch between ints/sec and what the register uses.
 * And yes, it's the same math going both ways.  The lowest value
 * supported by all of the rnp hardware is 8.
 */
#define EITR_INTS_PER_SEC_TO_REG(_eitr) \
	((_eitr) ? (1000000000 / ((_eitr) * 256)) : 8)
#define EITR_REG_TO_INTS_PER_SEC EITR_INTS_PER_SEC_TO_REG

#define RNPVF_DESC_UNUSED(R)                                          \
	((((R)->next_to_clean > (R)->next_to_use) ? 0 : (R)->count) + \
	 (R)->next_to_clean - (R)->next_to_use - 1)

#define RNPVF_RX_DESC(R, i) (&(((union rnp_rx_desc *)((R)->desc))[i]))
#define RNPVF_TX_DESC(R, i) (&(((struct rnp_tx_desc *)((R)->desc))[i]))
#define RNPVF_TX_CTXTDESC(R, i) \
	(&(((struct rnp_tx_ctx_desc *)((R)->desc))[i]))

#define RNPVF_N10_MAX_JUMBO_FRAME_SIZE \
	9590 /* Maximum Supported Size 9.5KB */
#define RNPVF_N500_MAX_JUMBO_FRAME_SIZE \
	9722 /* Maximum Supported Size 9.5KB */
#define RNPVF_MIN_MTU 68

#define MAX_MSIX_VECTORS 4
#define OTHER_VECTOR 1
#define NON_Q_VECTORS (OTHER_VECTOR)

#define MAX_MSIX_Q_VECTORS 2

#define MIN_MSIX_Q_VECTORS 1
#define MIN_MSIX_COUNT (MIN_MSIX_Q_VECTORS + NON_Q_VECTORS)

enum phy_type {
	PHY_TYPE_NONE = 0,
	PHY_TYPE_1G_BASE_KX,
	PHY_TYPE_RGMII,
	PHY_TYPE_10G_BASE_KR,
	PHY_TYPE_25G_BASE_KR,
	PHY_TYPE_40G_BASE_KR4,
};

struct rnpvf_hw {
	void *back;
	u8 __iomem *hw_addr;
	u8 __iomem *hw_addr_bar0;
	u8 __iomem *ring_msix_base;
	u8 vfnum;
#define VF_NUM_MASK 0x3f
	struct pci_dev *pdev;

	u16 device_id;
	u16 vendor_id;
	u16 subsystem_device_id;
	u16 subsystem_vendor_id;

	enum rnp_board_type board_type;

	u32 dma_version;
	u16 min_length;
	u16 max_length;

	u16 queue_ring_base;
	u32 tx_items_count;
	u32 rx_items_count;
	u16 mac_type;
	u16 phy_type;
	int mtu;
	u16 link;
	u16 speed;

	struct rnpvf_hw_operations ops;
	struct rnp_mac_info mac;
	struct rnp_fc_info fc;
	struct rnp_mbx_info mbx;

	bool adapter_stopped;
	u32 api_version;
	int fw_version;
	int usecstocount;
#define PF_FEATURE_VLAN_FILTER BIT(0)
#define PF_NCSI_EN BIT(1)
#define PF_MAC_SPOOF BIT(2)
	u32 pf_feature;

	int mode;
#define RNPVF_NET_FEATURE_SG ((u32)(1 << 0))
#define RNPVF_NET_FEATURE_TX_CHECKSUM ((u32)(1 << 1))
#define RNPVF_NET_FEATURE_RX_CHECKSUM ((u32)(1 << 2))
#define RNPVF_NET_FEATURE_TSO ((u32)(1 << 3))
#define RNPVF_NET_FEATURE_TX_UDP_TUNNEL (1 << 4)
#define RNPVF_NET_FEATURE_VLAN_FILTER (1 << 5)
#define RNPVF_NET_FEATURE_VLAN_OFFLOAD (1 << 6)
#define RNPVF_NET_FEATURE_RX_NTUPLE_FILTER (1 << 7)
#define RNPVF_NET_FEATURE_TCAM (1 << 8)
#define RNPVF_NET_FEATURE_RX_HASH (1 << 9)
#define RNPVF_NET_FEATURE_RX_FCS (1 << 10)
#define RNPVF_NET_FEATURE_HW_TC (1 << 11)
#define RNPVF_NET_FEATURE_USO (1 << 12)
#define RNPVF_NET_FEATURE_STAG_FILTER (1 << 13)
#define RNPVF_NET_FEATURE_STAG_OFFLOAD (1 << 14)

	u32 feature_flags;
};
#define VFNUM(mbx, num) ((num) & mbx->vf_num_mask)

enum irq_mode_enum {
	irq_mode_msix,
	irq_mode_msi,
	irq_mode_legency,
};

/* board specific private data structure */
struct rnpvf_adapter {
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
#define GET_VFNUM_FROM_BAR0 BIT(0)
	u16 status;
	u16 vf_vlan;
	struct timer_list watchdog_timer;
	u16 bd_number;
	struct work_struct reset_task;
	bool promisc_mode;

	/* Interrupt Throttle Rate */
	u16 rx_itr_setting;
	u16 tx_itr_setting;

	u16 rx_usecs;
	u16 rx_frames;
	u16 tx_usecs;
	u16 tx_frames;
	u32 pkt_rate_low;
	u16 rx_usecs_low;
	u32 pkt_rate_high;
	u16 rx_usecs_high;
	u32 sample_interval;
	u32 adaptive_rx_coal;
	u32 adaptive_tx_coal;
	u32 auto_rx_coal;
	u32 napi_budge;
	u32 tx_work_limit;
	/* TX */
	struct rnpvf_ring
		*tx_ring[MAX_TX_QUEUES] ____cacheline_aligned_in_smp;
	int tx_ring_item_count;
	int num_q_vectors;
	int num_tx_queues;
	u64 restart_queue;
	u64 hw_csum_tx_good;
	u64 lsc_int;
	u64 hw_tso_ctxt;
	u64 hw_tso6_ctxt;
	u32 tx_timeout_count;

	/* RX */
	struct rnpvf_ring *rx_ring[MAX_RX_QUEUES];
	int rx_ring_item_count;
	int num_rx_queues;
	u64 hw_csum_rx_error;
	u64 hw_rx_no_dma_resources;
	u64 hw_csum_rx_good;
	u64 non_eop_descs;

	u32 alloc_rx_page_failed;
	u32 alloc_rx_buff_failed;

	int vector_off;
	int num_other_vectors;
	int irq_mode;
	struct rnpvf_q_vector *q_vector[MAX_MSIX_VECTORS];

	int num_msix_vectors;
	struct msix_entry *msix_entries;

	u32 dma_channels;
	/* the real used dma ring channels */

	/* Some features need tri-state capability,
	 * thus the additional *_CAPABLE flags.
	 */
	u32 flags;
#define RNPVF_FLAG_IN_WATCHDOG_TASK ((u32)(1))
#define RNPVF_FLAG_IN_NETPOLL ((u32)(1 << 1))
#define RNPVF_FLAG_PF_SET_VLAN ((u32)(1 << 2))
#define RNPVF_FLAG_PF_UPDATE_MTU ((u32)(1 << 3))
#define RNPVF_FLAG_PF_UPDATE_MAC ((u32)(1 << 4))
#define RNPVF_FLAG_PF_UPDATE_VLAN ((u32)(1 << 5))
#define RNPVF_FLAG_PF_RESET ((u32)(1 << 6))
#define RNPVF_FLAG_PF_RESET_REQ ((u32)(1 << 7))
#define RNPVF_FLAG_MSI_CAPABLE ((u32)(1 << 8))
#define RNPVF_FLAG_MSI_ENABLED ((u32)(1 << 9))
#define RNPVF_FLAG_MSIX_CAPABLE ((u32)(1 << 10))
#define RNPVF_FLAG_MSIX_ENABLED ((u32)(1 << 11))
#define RNPVF_FLAG_RX_CHKSUM_ENABLED ((u32)(1 << 12))
#define RNPVF_FLAG_TX_VLAN_OFFLOAD ((u32)(1 << 13))
#define RNPVF_FLAG_RX_VLAN_OFFLOAD ((u32)(1 << 14))

	u32 priv_flags;
#define RNPVF_PRIV_FLAG_FT_PADDING BIT(0)
#define RNPVF_PRIV_FLAG_PADDING_DEBUG BIT(1)
#define RNPVF_PRIV_FLAG_FCS_ON BIT(2)
#define RNPVF_PRIV_FLAG_TX_PADDING BIT(3)

	/* OS defined structs */
	struct net_device *netdev;
	struct pci_dev *pdev;

	/* structs defined in rnp_vf.h */
	struct rnpvf_hw hw;
	u16 msg_enable;
	struct rnpvf_hw_stats stats;
	struct rnpvf_hw_stats_own hw_stats;
	u64 zero_base;
	/* Interrupt Throttle Rate */
	u32 eitr_param;

	unsigned long state;
	u64 tx_busy;
	u32 link_speed;
	bool link_up;

	struct work_struct watchdog_task;

	u8 port;

	spinlock_t mbx_lock;
	char name[60];
};

enum rnpvf_state_t {
	__RNPVF_TESTING,
	__RNPVF_RESETTING,
	__RNPVF_DOWN,
	__RNPVF_REMOVE,
	__RNPVF_MBX_POLLING,
	__RNPVF_LINK_DOWN
};

struct rnpvf_cb {
	union { /* Union defining head/tail partner */
		struct sk_buff *head;
		struct sk_buff *tail;
	};
	dma_addr_t dma;
	u16 append_cnt;
	bool page_released;
};
#define RNPVF_CB(skb) ((struct rnpvf_cb *)(skb)->cb)

#define RING2ADAPT(ring) netdev_priv((ring)->netdev)

enum rnpvf_boards {
	board_n10,
	board_n500,
};

extern const struct rnpvf_info rnpvf_82599_vf_info;
extern const struct rnpvf_info rnpvf_X540_vf_info;
extern const struct rnp_mbx_operations rnpvf_mbx_ops;

/* needed by ethtool.c */
extern char rnpvf_driver_name[];
extern const char rnpvf_driver_version[];

extern void rnpvf_up(struct rnpvf_adapter *adapter);
extern void rnpvf_down(struct rnpvf_adapter *adapter);
extern void rnpvf_reinit_locked(struct rnpvf_adapter *adapter);
extern void rnpvf_reset(struct rnpvf_adapter *adapter);
extern void rnpvf_set_ethtool_ops(struct net_device *netdev);
extern int rnpvf_setup_rx_resources(struct rnpvf_adapter *,
				    struct rnpvf_ring *);
extern int rnpvf_setup_tx_resources(struct rnpvf_adapter *,
				    struct rnpvf_ring *);
extern void rnpvf_free_rx_resources(struct rnpvf_adapter *,
				    struct rnpvf_ring *);
extern void rnpvf_free_tx_resources(struct rnpvf_adapter *,
				    struct rnpvf_ring *);
extern void rnpvf_update_stats(struct rnpvf_adapter *adapter);
extern int ethtool_ioctl(struct ifreq *ifr);
extern void rnpvf_remove_mbx_irq(struct rnpvf_adapter *adapter);
extern void rnpvf_clear_interrupt_scheme(struct rnpvf_adapter *adapter);
extern int rnpvf_register_mbx_irq(struct rnpvf_adapter *adapter);
extern int rnpvf_init_interrupt_scheme(struct rnpvf_adapter *adapter);
extern int rnpvf_close(struct net_device *netdev);
extern int rnpvf_open(struct net_device *netdev);

extern void rnp_napi_add_all(struct rnpvf_adapter *adapter);
extern void rnp_napi_del_all(struct rnpvf_adapter *adapter);

extern int rnpvf_sysfs_init(struct net_device *ndev);
extern void rnpvf_sysfs_exit(struct net_device *ndev);

static inline int rnpvf_is_pf1(struct pci_dev *pdev)
{
	return ((pdev->devfn) ? 1 : 0);
}

static inline struct netdev_queue *
txring_txq(const struct rnpvf_ring *ring)
{
	return netdev_get_tx_queue(ring->netdev, ring->queue_index);
}
/*
 * FCoE requires that all Rx buffers be over 2200 bytes in length.  Since
 * this is twice the size of a half page we need to double the page order
 * for FCoE enabled Rx queues.
 */
static inline unsigned int rnpvf_rx_bufsz(struct rnpvf_ring *ring)
{
	/* 1 rx-desc trans max half page(2048), for jumbo frame sg is needed */
	return RNPVF_RXBUFFER_1536;
}

/* SG , 1 rx-desc use one page */
static inline unsigned int rnpvf_rx_pg_order(struct rnpvf_ring *ring)
{
	return 0;
}
#define rnpvf_rx_pg_size(_ring) (PAGE_SIZE << rnpvf_rx_pg_order(_ring))

static inline u32 rnpvf_rx_desc_used_hw(struct rnpvf_hw *hw,
					struct rnpvf_ring *rx_ring)
{
	u32 head = ring_rd32(rx_ring, RNP_DMA_REG_RX_DESC_BUF_HEAD);
	u32 tail = ring_rd32(rx_ring, RNP_DMA_REG_RX_DESC_BUF_TAIL);
	u16 count = rx_ring->count;

	return ((tail >= head) ? (count - tail + head) : (head - tail));
}

static inline u32 rnpvf_tx_desc_unused_hw(struct rnpvf_hw *hw,
					  struct rnpvf_ring *tx_ring)
{
	u32 head = ring_rd32(tx_ring, RNP_DMA_REG_TX_DESC_BUF_HEAD);
	u32 tail = ring_rd32(tx_ring, RNP_DMA_REG_TX_DESC_BUF_TAIL);
	u16 count = tx_ring->count;

	return ((tail > head) ? (count - tail + head) : (head - tail));
}

#define IS_VALID_VID(vid) ((vid) >= 0 && (vid) < 4096)

#endif /* _RNPVF_H_ */
