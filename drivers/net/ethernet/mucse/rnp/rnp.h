/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#ifndef _RNP_H_
#define _RNP_H_

#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/cpumask.h>
#include <linux/aer.h>
#include <linux/if_vlan.h>
#include <linux/jiffies.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/net_tstamp.h>
#include <linux/ptp_clock_kernel.h>

#include "rnp_type.h"
#include "rnp_common.h"
#include "rnp_dcb.h"

/* common prefix used by pr_<> macros */
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define RNP_ALLOC_PAGE_ORDER 0
#define RNP_PAGE_BUFFER_NUMS(ring)                               \
	((1 << RNP_ALLOC_PAGE_ORDER) * PAGE_SIZE /               \
	 ALIGN((rnp_rx_offset(ring) + rnp_rx_bufsz(ring) +       \
		SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) + \
		RNP_RX_HWTS_OFFSET),                             \
	       1024))

#define RNP_DEFAULT_TX_WORK (128)
#define RNP_MIN_TX_WORK (32)
#define RNP_MAX_TX_WORK (512)
#define RNP_MIN_RX_WORK (32)
#define RNP_MAX_RX_WORK (512)
#define RNP_WORK_ALIGN (2)
#define RNP_MIN_TX_FRAME (1)
#define RNP_MAX_TX_FRAME (256)
#define RNP_MIN_TX_USEC (30)
#define RNP_MAX_TX_USEC (10000)

#define RNP_MIN_RX_FRAME (1)
#define RNP_MAX_RX_FRAME (256)
#define RNP_MIN_RX_USEC (10)
#define RNP_MAX_RX_USEC (10000)

#define RNP_MAX_TXD (4096)
#define RNP_MIN_TXD (64)

#define ACTION_TO_MPE (130)
#define MPE_PORT (10)
#define AUTO_ALL_MODES 0
/* TX/RX descriptor defines */
#ifdef FEITENG
#define RNP_DEFAULT_TXD 4096
#else
#define RNP_DEFAULT_TXD 512
#endif

#define RNP_REQ_TX_DESCRIPTOR_MULTIPLE 8
#define RNP_REQ_RX_DESCRIPTOR_MULTIPLE 8

#ifdef FEITENG
#define RNP_DEFAULT_RXD 4096
#else
#define RNP_DEFAULT_RXD 512
#endif
#define RNP_MAX_RXD 4096
#define RNP_MIN_RXD 64

/* flow control */
#define RNP_MIN_FCRTL 0x40
#define RNP_MAX_FCRTL 0x7FF80
#define RNP_MIN_FCRTH 0x600
#define RNP_MAX_FCRTH 0x7FFF0
#define RNP_DEFAULT_FCPAUSE 0xFFFF
#define RNP10_DEFAULT_HIGH_WATER 0x320
#define RNP10_DEFAULT_LOW_WATER 0x270
#define RNP500_DEFAULT_HIGH_WATER 400
#define RNP500_DEFAULT_LOW_WATER 256
#define RNP_MIN_FCPAUSE 0
#define RNP_MAX_FCPAUSE 0xFFFF

/* Supported Rx Buffer Sizes */
#define RNP_RXBUFFER_256 256 /* Used for skb receive header */
#define RNP_RXBUFFER_1536 1536
#define RNP_RXBUFFER_2K 2048
#define RNP_RXBUFFER_3K 3072
#define RNP_RXBUFFER_4K 4096
#define RNP_MAX_RXBUFFER 16384 /* largest size for a single descriptor */
#define RNP_RXBUFFER_MAX (RNP_RXBUFFER_2K)

#define MAX_Q_VECTORS 128

#define RNP_RING_COUNTS_PEER_PF 8
#define RNP_GSO_PARTIAL_FEATURES                                           \
	(NETIF_F_GSO_GRE | NETIF_F_GSO_GRE_CSUM | NETIF_F_GSO_UDP_TUNNEL | \
	 NETIF_F_GSO_UDP_TUNNEL_CSUM)

/*
 * NOTE: netdev_alloc_skb reserves up to 64 bytes, NET_IP_ALIGN means we
 * reserve 64 more, and skb_shared_info adds an additional 320 bytes more,
 * this adds up to 448 bytes of extra data.
 *
 * Since netdev_alloc_skb now allocates a page fragment we can use a value
 * of 256 and the resultant skb will have a truesize of 960 or less.
 */
#define RNP_RX_HDR_SIZE RNP_RXBUFFER_256

#define RNP_ITR_ADAPTIVE_MIN_INC 2
#define RNP_ITR_ADAPTIVE_MIN_USECS 5
#define RNP_ITR_ADAPTIVE_MAX_USECS 800
#define RNP_ITR_ADAPTIVE_LATENCY 0x400
#define RNP_ITR_ADAPTIVE_BULK 0x00
#define RNP_ITR_ADAPTIVE_MASK_USECS \
	(RNP_ITR_ADAPTIVE_LATENCY - RNP_ITR_ADAPTIVE_MIN_INC)

/* How many Rx Buffers do we bundle into one write to the hardware ? */
#ifdef OPTM_WITH_LPAGE
#define RNP_RX_BUFFER_WRITE (PAGE_SIZE / 2048) /* Must be power of 2 */
#else
#define RNP_RX_BUFFER_WRITE 16 /* Must be power of 2 */
#endif
enum rnp_tx_flags {
	/* cmd_type flags */
	RNP_TX_FLAGS_HW_VLAN = 0x01,
	RNP_TX_FLAGS_TSO = 0x02,
	RNP_TX_FLAGS_TSTAMP = 0x04,

	/* olinfo flags */
	RNP_TX_FLAGS_CC = 0x08,
	RNP_TX_FLAGS_IPV4 = 0x10,
	RNP_TX_FLAGS_CSUM = 0x20,

	/* software defined flags */
	RNP_TX_FLAGS_SW_VLAN = 0x40,
	RNP_TX_FLAGS_FCOE = 0x80,
};
#ifndef RNP_MAX_VF_CNT
#define RNP_MAX_VF_CNT 64
#endif

#define RNP_RX_RATE_HIGH 450000
#define RNP_RX_COAL_TIME_HIGH 128
#define RNP_RX_SIZE_THRESH 1024
#define RNP_RX_RATE_THRESH (1000000 / RNP_RX_COAL_TIME_HIGH)
#define RNP_SAMPLE_INTERVAL 0
#define RNP_AVG_PKT_SMALL 256

#define RNP_MAX_VF_MC_ENTRIES 30
#define RNP_MAX_VF_FUNCTIONS RNP_MAX_VF_CNT
#define RNP_MAX_VFTA_ENTRIES 128
#define MAX_EMULATION_MAC_ADDRS 16
#define RNP_MAX_PF_MACVLANS_N10 15
//#define RNP_MAX_PF_MACVLANS 15
#define PF_RING_CNT_WHEN_IOV_ENABLED 2
#define VMDQ_P(p) ((p) + adapter->ring_feature[RING_F_VMDQ].offset)

enum vf_link_state {
	rnp_link_state_on,
	rnp_link_state_auto,
	rnp_link_state_off,

};

struct vf_data_storage {
	unsigned char vf_mac_addresses[ETH_ALEN];
	u16 vf_mc_hashes[RNP_MAX_VF_MC_ENTRIES];
	u16 num_vf_mc_hashes;
	u16 default_vf_vlan_id;
	u16 vlans_enabled;
	bool clear_to_send;
	bool pf_set_mac;
	u16 pf_vlan; /* When set, guest VLAN config not allowed. */
	u16 vf_vlan; // vf just can set 1 vlan
	u16 pf_qos;
	u16 tx_rate;
	int link_state;
	u16 vlan_count;
	u8 spoofchk_enabled;
	u8 trusted;
	bool promisc_mode;
	unsigned long status;
	unsigned int vf_api;
};

enum vf_state_t {
	__VF_MBX_USED,
};

struct vf_macvlans {
	struct list_head l;
	int vf;
	int rar_entry;
	bool free;
	bool is_macvlan;
	u8 vf_macvlan[ETH_ALEN];
};

/* now tx max 4k for one desc */
// feiteng use 12k can get better netperf performance
#define RNP_MAX_TXD_PWR 12
#define RNP_MAX_DATA_PER_TXD (1 << RNP_MAX_TXD_PWR)
//#define RNP_MAX_DATA_PER_TXD (12 * 1024)

/* Tx Descriptors needed, worst case */
#define TXD_USE_COUNT(S) DIV_ROUND_UP((S), RNP_MAX_DATA_PER_TXD)
#define DESC_NEEDED (MAX_SKB_FRAGS + 4)

/* wrapper around a pointer to a socket buffer,
 * so a DMA handle can be stored along with the buffers
 */
struct rnp_tx_buffer {
	struct rnp_tx_desc *next_to_watch;
	unsigned long time_stamp;
	struct sk_buff *skb;
	unsigned int bytecount;
	unsigned short gso_segs;
	bool gso_need_padding;

	__be16 protocol;
	__be16 priv_tags;
	DEFINE_DMA_UNMAP_ADDR(dma);
	DEFINE_DMA_UNMAP_LEN(len);
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

struct rnp_rx_buffer {
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

struct rnp_queue_stats {
	u64 packets;
	u64 bytes;
};

struct rnp_tx_queue_stats {
	u64 restart_queue;
	u64 tx_busy;
	u64 tx_done_old;
	u64 clean_desc;
	u64 poll_count;
	u64 irq_more_count;
	u64 send_bytes;
	u64 send_bytes_to_hw;
	u64 todo_update;
	u64 send_done_bytes;
	u64 vlan_add;
	u64 tx_next_to_clean;
	u64 tx_irq_miss;
	u64 tx_equal_count;
	u64 tx_clean_times;
	u64 tx_clean_count;
};

struct rnp_rx_queue_stats {
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
	u64 vlan_remove;
	u64 rx_next_to_clean;
	u64 rx_irq_miss;
	u64 rx_equal_count;
	u64 rx_clean_times;
	u64 rx_clean_count;
};

enum rnp_ring_state_t {
	__RNP_RX_3K_BUFFER,
	__RNP_RX_BUILD_SKB_ENABLED,
	__RNP_TX_FDIR_INIT_DONE,
	__RNP_TX_XPS_INIT_DONE,
	__RNP_TX_DETECT_HANG,
	__RNP_HANG_CHECK_ARMED,
	__RNP_RX_CSUM_UDP_ZERO_ERR,
	__RNP_RX_FCOE,
};

#define ring_uses_build_skb(ring) \
	test_bit(__RNP_RX_BUILD_SKB_ENABLED, &(ring)->state)
#define check_for_tx_hang(ring) test_bit(__RNP_TX_DETECT_HANG, &(ring)->state)
#define set_check_for_tx_hang(ring) \
	set_bit(__RNP_TX_DETECT_HANG, &(ring)->state)
#define clear_check_for_tx_hang(ring) \
	clear_bit(__RNP_TX_DETECT_HANG, &(ring)->state)
struct rnp_ring {
	struct rnp_ring *next; /* pointer to next ring in q_vector */
	struct rnp_q_vector *q_vector; /* backpointer to host q_vector */
	struct net_device *netdev; /* netdev ring belongs to */
	struct device *dev; /* device for DMA mapping */
	void *desc; /* descriptor ring memory */
	union {
		struct rnp_tx_buffer *tx_buffer_info;
		struct rnp_rx_buffer *rx_buffer_info;
	};
	unsigned long last_rx_timestamp;
	unsigned long state;
	u8 __iomem *ring_addr;
	u8 __iomem *tail;
	u8 __iomem *dma_int_stat;
	u8 __iomem *dma_int_mask;
	u8 __iomem *dma_int_clr;
	dma_addr_t dma; /* phys. address of descriptor ring */
	unsigned int size; /* length in bytes */
	u32 ring_flags;
#define RNP_RING_FLAG_DELAY_SETUP_RX_LEN ((u32)(1 << 0))
#define RNP_RING_FLAG_CHANGE_RX_LEN ((u32)(1 << 1))
#define RNP_RING_FLAG_DO_RESET_RX_LEN ((u32)(1 << 2))
#define RNP_RING_SKIP_TX_START ((u32)(1 << 3))
#define RNP_RING_NO_TUNNEL_SUPPORT ((u32)(1 << 4))
#define RNP_RING_SIZE_CHANGE_FIX ((u32)(1 << 5))
#define RNP_RING_SCATER_SETUP ((u32)(1 << 6))
#define RNP_RING_STAGS_SUPPORT ((u32)(1 << 7))
#define RNP_RING_DOUBLE_VLAN_SUPPORT ((u32)(1 << 8))
#define RNP_RING_VEB_MULTI_FIX ((u32)(1 << 9))
#define RNP_RING_IRQ_MISS_FIX ((u32)(1 << 10))
#define RNP_RING_OUTER_VLAN_FIX ((u32)(1 << 11))
#define RNP_RING_CHKSM_FIX ((u32)(1 << 12))
#define RNP_RING_LOWER_ITR ((u32)(1 << 13))
	u8 pfvfnum;

	u16 count; /* amount of descriptors */
	u16 temp_count;
	u16 reset_count;

	u8 queue_index; /* queue_index needed for multiqueue queue management */
	u8 rnp_queue_idx; /* the real ring,used by dma */
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
	struct rnp_queue_stats stats;
	struct u64_stats_sync syncp;
	union {
		struct rnp_tx_queue_stats tx_stats;
		struct rnp_rx_queue_stats rx_stats;
	};
} ____cacheline_internodealigned_in_smp;

#define RING2ADAPT(ring) netdev_priv((ring)->netdev)

enum rnp_ring_f_enum {
	RING_F_NONE = 0,
	RING_F_VMDQ, /* SR-IOV uses the same ring feature */
	RING_F_RSS,
	RING_F_FDIR,

	RING_F_ARRAY_SIZE /* must be last in enum set */
};

#define RNP_MAX_RSS_INDICES 128
#define RNP_MAX_RSS_INDICES_UV3P 8
#define RNP_MAX_VMDQ_INDICES 64
#define RNP_MAX_FDIR_INDICES 63 /* based on q_vector limit */
#define RNP_MAX_FCOE_INDICES 8
#define MAX_RX_QUEUES (128)
#define MAX_TX_QUEUES (128)
struct rnp_ring_feature {
	u16 limit; /* upper limit on feature indices */
	u16 indices; /* current value of indices */
	u16 mask; /* Mask used for feature to ring mapping */
	u16 offset; /* offset to start of feature */
} ____cacheline_internodealigned_in_smp;

#define RNP_n10_VMDQ_8Q_MASK 0x78
#define RNP_n10_VMDQ_4Q_MASK 0x7C
#define RNP_n10_VMDQ_2Q_MASK 0x7E

/*
 * FCoE requires that all Rx buffers be over 2200 bytes in length.  Since
 * this is twice the size of a half page we need to double the page order
 * for FCoE enabled Rx queues.
 */
static inline unsigned int rnp_rx_bufsz(struct rnp_ring *ring)
{
	return (RNP_RXBUFFER_1536 - NET_IP_ALIGN);
}

static inline unsigned int rnp_rx_pg_order(struct rnp_ring *ring)
{
	/* fixed 1 page */
	/* we don't support 3k buffer */
	return 0;
}
#define rnp_rx_pg_size(_ring) (PAGE_SIZE << rnp_rx_pg_order(_ring))

struct rnp_ring_container {
	struct rnp_ring *ring; /* pointer to linked list of rings */
	unsigned long next_update; /* jiffies value of last update */
	unsigned int total_bytes; /* total bytes processed this int */
	unsigned int total_packets; /* total packets processed this int */
	unsigned int total_packets_old;
	u16 work_limit; /* total work allowed per interrupt */
	u16 count; /* total number of rings in vector */
	u16 itr; /* current ITR/MSIX vector setting for ring */
	u16 add_itr;
};

/* iterator for handling rings in ring container */
#define rnp_for_each_ring(pos, head) \
	for (pos = (head).ring; pos != NULL; pos = pos->next)

#define MAX_RX_PACKET_BUFFERS ((adapter->flags & RNP_FLAG_DCB_ENABLED) ? 8 : 1)
#define MAX_TX_PACKET_BUFFERS MAX_RX_PACKET_BUFFERS

/* MAX_Q_VECTORS of these are allocated,
 * but we only use one per queue-specific vector.
 */

#define SUPPORT_IRQ_AFFINITY_CHANGE
struct rnp_q_vector {
	int old_rx_count;
	int new_rx_count;
	int new_tx_count;
	int large_times;
	int small_times;
	int too_small_times;
	int middle_time;
	int large_times_tx;
	int small_times_tx;
	int too_small_times_tx;
	int middle_time_tx;
	struct rnp_adapter *adapter;
	int v_idx;
	/* index of q_vector within array, also used for
	 * finding the bit in EICR and friends that
	 * represents the vector for this ring */
	u16 itr_rx;
	u16 itr_tx;
	struct rnp_ring_container rx, tx;

	struct napi_struct napi;
	cpumask_t affinity_mask;
	struct irq_affinity_notify affinity_notify;
	int numa_node;
	struct rcu_head rcu; /* to avoid race with update stats on free */

	u32 vector_flags;
#define RNP_QVECTOR_FLAG_IRQ_MISS_CHECK ((u32)(1 << 0))
#define RNP_QVECTOR_FLAG_ITR_FEATURE ((u32)(1 << 1))
#define RNP_QVECTOR_FLAG_REDUCE_TX_IRQ_MISS ((u32)(1 << 2))
	int irq_check_usecs;
	struct hrtimer irq_miss_check_timer;

	char name[IFNAMSIZ + 9];

	/* for dynamic allocation of rings associated with this q_vector */
	struct rnp_ring ring[0] ____cacheline_internodealigned_in_smp;
};

static inline __le16 rnp_test_ext_cmd(union rnp_rx_desc *rx_desc,
				      const u16 stat_err_bits)
{
	return rx_desc->wb.rev1 & cpu_to_le16(stat_err_bits);
}

#ifdef RNP_HWMON

#define RNP_HWMON_TYPE_LOC 0
#define RNP_HWMON_TYPE_TEMP 1
#define RNP_HWMON_TYPE_CAUTION 2
#define RNP_HWMON_TYPE_MAX 3
#define RNP_HWMON_TYPE_NAME 4

struct hwmon_attr {
	struct device_attribute dev_attr;
	struct rnp_hw *hw;
	struct rnp_thermal_diode_data *sensor;
	char name[12];
};

struct hwmon_buff {
	struct attribute_group group;
	const struct attribute_group *groups[2];
	struct attribute *attrs[RNP_MAX_SENSORS * 4 + 1];
	struct hwmon_attr hwmon_list[RNP_MAX_SENSORS * 4];
	unsigned int n_hwmon;
};
#endif /* RNPM_HWMON */

/* 
	rnp_test_staterr - tests bits in Rx descriptor status and error fields 
*/
static inline __le16 rnp_test_staterr(union rnp_rx_desc *rx_desc,
				      const u16 stat_err_bits)
{
	return rx_desc->wb.cmd & cpu_to_le16(stat_err_bits);
}

static inline __le16 rnp_get_stat(union rnp_rx_desc *rx_desc,
				  const u16 stat_mask)
{
	return rx_desc->wb.cmd & cpu_to_le16(stat_mask);
}

static inline u16 rnp_desc_unused(struct rnp_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->count) + ntc - ntu - 1;
}

static inline u16 rnp_desc_unused_rx(struct rnp_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->count) + ntc - ntu - 1;
}

#define RNP_RX_DESC(R, i) (&(((union rnp_rx_desc *)((R)->desc))[i]))
#define RNP_TX_DESC(R, i) (&(((struct rnp_tx_desc *)((R)->desc))[i]))
#define RNP_TX_CTXTDESC(R, i) (&(((struct rnp_tx_ctx_desc *)((R)->desc))[i]))

#define RNP_MAX_JUMBO_FRAME_SIZE 9590 /* Maximum Supported Size 9.5KB */
#define RNP_MIN_MTU 68
#define RNP500_MAX_JUMBO_FRAME_SIZE 9722 /* Maximum Supported Size 9728 */

#define OTHER_VECTOR 1
#define NON_Q_VECTORS (OTHER_VECTOR)

/* default to trying for four seconds */
#define RNP_TRY_LINK_TIMEOUT (4 * HZ)

#define RNP_MAX_USER_PRIO (8)
#define RNP_MAX_TCS_NUM (4)
struct rnp_pfc_cfg {
	u8 pfc_max; /* hardware can enabled max pfc channel */
	u8 hw_pfc_map; /* enable the prio channel bit */
	u8 pfc_num; /* at present enabled the pfc-channel num */
	u8 pfc_en; /* enabled the pfc feature or not */
};

struct rnp_dcb_num_tcs {
	u8 pg_tcs;
	u8 pfc_tcs;
};

struct rnp_dcb_cfg {
	u8 tc_num;
	u16 delay; /* pause time */
	u8 dcb_en; /* enabled the dcb feature or not */
	u8 dcbx_mode;
	struct rnp_pfc_cfg pfc_cfg;
	struct rnp_dcb_num_tcs num_tcs;
	/* statistic info */
	u64 requests[RNP_MAX_TCS_NUM];
	u64 indications[RNP_MAX_TCS_NUM];
	enum rnp_fc_mode last_lfc_mode;
};

struct rnp_pps_cfg {
	bool available;
	struct timespec64 start;
	struct timespec64 period;
};

enum rss_func_mode_enum {
	rss_func_top,
	rss_func_xor,
	rss_func_order,
};

enum outer_vlan_type_enum {
	outer_vlan_type_88a8,
	outer_vlan_type_9100,
	outer_vlan_type_9200,
	outer_vlan_type_max,
};

enum irq_mode_enum {
	irq_mode_legency,
	irq_mode_msi,
	irq_mode_msix,
};

/* board specific private data structure */
struct rnp_adapter {
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
	unsigned long active_vlans_stags[BITS_TO_LONGS(VLAN_N_VID)];
	/* OS defined structs */
	u16 vf_vlan;
	u16 vlan_count;
	int miss_time;
	struct net_device *netdev;
	struct pci_dev *pdev;
	bool quit_poll_thread;
	struct task_struct *rx_poll_thread;
	unsigned long state;
	spinlock_t link_stat_lock;

	/* this var is used for auto itr modify */
	/* hw not Supported well */
	unsigned long last_moder_packets[MAX_RX_QUEUES];
	unsigned long last_moder_tx_packets;
	unsigned long last_moder_bytes[MAX_RX_QUEUES];
	unsigned long last_moder_jiffies;
	int last_moder_time[MAX_RX_QUEUES];
	/* only rx itr is Supported */
	int usecendcount;
	u16 rx_usecs;
	u16 rx_usecs_usr_set;
	u16 rx_frames;
	u16 usecstocount;
	u16 tx_frames;
	u16 tx_usecs;
	u16 tx_usecs_usr_set;
	u32 pkt_rate_low;
	u16 rx_usecs_low;
	u32 pkt_rate_high;
	u16 rx_usecs_high;
	u32 sample_interval;
	u32 adaptive_rx_coal;
	u32 adaptive_tx_coal;
	u32 auto_rx_coal;
	int napi_budge;
	union {
		int phy_addr;
		struct {
			u8 mod_abs : 1;
			u8 fault : 1;
			u8 tx_dis : 1;
			u8 los : 1;
		} sfp;
	};

	struct {
		u32 main;
		u32 pre;
		u32 post;
		u32 tx_boost;
	} si;

	int speed;

	u8 an : 1;
	u8 fec : 1;
	u8 link_traing : 1;
	u8 duplex : 1;
	u8 rpu_inited : 1;

	/* Some features need tri-state capability,
	 * thus the additional *_CAPABLE flags.
	 */
	u32 vf_num_for_pf;
	u32 flags;
#define RNP_FLAG_MSI_CAPABLE ((u32)(1 << 0))
#define RNP_FLAG_MSI_ENABLED ((u32)(1 << 1))
#define RNP_FLAG_MSIX_CAPABLE ((u32)(1 << 2))
#define RNP_FLAG_MSIX_ENABLED ((u32)(1 << 3))
#define RNP_FLAG_RX_1BUF_CAPABLE ((u32)(1 << 4))
#define RNP_FLAG_RX_PS_CAPABLE ((u32)(1 << 5))
#define RNP_FLAG_RX_PS_ENABLED ((u32)(1 << 6))
#define RNP_FLAG_IN_NETPOLL ((u32)(1 << 7))
#define RNP_FLAG_DCA_ENABLED ((u32)(1 << 8))
#define RNP_FLAG_DCA_CAPABLE ((u32)(1 << 9))
#define RNP_FLAG_IMIR_ENABLED ((u32)(1 << 10))
#define RNP_FLAG_MQ_CAPABLE ((u32)(1 << 11))
#define RNP_FLAG_DCB_ENABLED ((u32)(1 << 12))
#define RNP_FLAG_VMDQ_CAPABLE ((u32)(1 << 13))
#define RNP_FLAG_VMDQ_ENABLED ((u32)(1 << 14))
#define RNP_FLAG_FAN_FAIL_CAPABLE ((u32)(1 << 15))
#define RNP_FLAG_NEED_LINK_UPDATE ((u32)(1 << 16))
#define RNP_FLAG_NEED_LINK_CONFIG ((u32)(1 << 17))
#define RNP_FLAG_FDIR_HASH_CAPABLE ((u32)(1 << 18))
#define RNP_FLAG_FDIR_PERFECT_CAPABLE ((u32)(1 << 19))
#define RNP_FLAG_FCOE_CAPABLE ((u32)(1 << 20))
#define RNP_FLAG_FCOE_ENABLED ((u32)(1 << 21))
#define RNP_FLAG_SRIOV_CAPABLE ((u32)(1 << 22))
#define RNP_FLAG_SRIOV_ENABLED ((u32)(1 << 23))
#define RNP_FLAG_VXLAN_OFFLOAD_CAPABLE ((u32)(1 << 24))
#define RNP_FLAG_VXLAN_OFFLOAD_ENABLE ((u32)(1 << 25))
#define RNP_FLAG_SWITCH_LOOPBACK_EN ((u32)(1 << 26))
#define RNP_FLAG_SRIOV_INIT_DONE ((u32)(1 << 27))
#define RNP_FLAG_IN_IRQ ((u32)(1 << 28))
#define RNP_FLAG_VF_INIT_DONE ((u32)(1 << 29))
#define RNP_FLAG_LEGACY_CAPABLE ((u32)(1 << 30))
#define RNP_FLAG_LEGACY_ENABLED ((u32)(1 << 31))
	u32 flags2;
#define RNP_FLAG2_RSC_CAPABLE ((u32)(1 << 0))
#define RNP_FLAG2_RSC_ENABLED ((u32)(1 << 1))
#define RNP_FLAG2_TEMP_SENSOR_CAPABLE ((u32)(1 << 2))
#define RNP_FLAG2_TEMP_SENSOR_EVENT ((u32)(1 << 3))
#define RNP_FLAG2_SEARCH_FOR_SFP ((u32)(1 << 4))
#define RNP_FLAG2_SFP_NEEDS_RESET ((u32)(1 << 5))
#define RNP_FLAG2_RESET_REQUESTED ((u32)(1 << 6))
#define RNP_FLAG2_FDIR_REQUIRES_REINIT ((u32)(1 << 7))
#define RNP_FLAG2_RSS_FIELD_IPV4_UDP ((u32)(1 << 8))
#define RNP_FLAG2_RSS_FIELD_IPV6_UDP ((u32)(1 << 9))
#define RNP_FLAG2_PTP_ENABLED ((u32)(1 << 10))
#define RNP_FLAG2_PTP_PPS_ENABLED ((u32)(1 << 11))
#define RNP_FLAG2_BRIDGE_MODE_VEB ((u32)(1 << 12))
#define RNP_FLAG2_VLAN_STAGS_ENABLED ((u32)(1 << 13))
#define RNP_FLAG2_UDP_TUN_REREG_NEEDED ((u32)(1 << 14))
#define RNP_FLAG2_RESET_PF ((u32)(1 << 15))
#define RNP_FLAG2_CHKSM_FIX ((u32)(1 << 16))

	u32 priv_flags;
#define RNP_PRIV_FLAG_MAC_LOOPBACK BIT(0)
#define RNP_PRIV_FLAG_SWITCH_LOOPBACK BIT(1)
#define RNP_PRIV_FLAG_VEB_ENABLE BIT(2)
#define RNP_PRIV_FLAG_FT_PADDING BIT(3)
#define RNP_PRIV_FLAG_PADDING_DEBUG BIT(4)
#define RNP_PRIV_FLAG_PTP_DEBUG BIT(5)
#define RNP_PRIV_FLAG_SIMUATE_DOWN BIT(6)
#define RNP_PRIV_FLAG_VXLAN_INNER_MATCH BIT(7)
#define RNP_PRIV_FLAG_ULTRA_SHORT BIT(8)
#define RNP_PRIV_FLAG_DOUBLE_VLAN BIT(9)
#define RNP_PRIV_FLAG_TCP_SYNC BIT(10)
#define RNP_PRIV_FLAG_PAUSE_OWN BIT(11)
#define RNP_PRIV_FLAG_JUMBO BIT(12)
#define RNP_PRIV_FLAG_TX_PADDING BIT(13)
#define RNP_PRIV_FLAG_RX_ALL BIT(14)
#define RNP_PRIV_FLAG_REC_HDR_LEN_ERR BIT(15)
#define RNP_PRIV_FLAG_RX_FCS BIT(16)
#define RNP_PRIV_FLAG_DOUBLE_VLAN_RECEIVE BIT(17)
#define RNP_PRIV_FLGA_TEST_TX_HANG BIT(18)
#define RNP_PRIV_FLAG_RX_SKIP_EN BIT(19)
#define RNP_PRIV_FLAG_TCP_SYNC_PRIO BIT(20)
#define RNP_PRIV_FLAG_REMAP_PRIO BIT(21)
#define RNP_PRIV_FLAG_8023_PRIO BIT(22)
#define RNP_PRIV_FLAG_SRIOV_VLAN_MODE BIT(23)
#define RNP_PRIV_FLAG_REMAP_MODE BIT(24)
#define RNP_PRIV_FLAG_LLDP_EN_STAT BIT(25)
#define RNP_PRIV_FLAG_LINK_DOWN_ON_CLOSE BIT(26)
#define RNP_PRIV_FLAG_LINK_DOWN_BEFORE BIT(27)

#define PRIV_DATA_EN BIT(7)
	int rss_func_mode;
	int outer_vlan_type;
	int tcp_sync_queue;
	int priv_skip_count;
	u64 rx_drop_status;
	int drop_time;
	/* Tx fast path data */
	unsigned int num_tx_queues;
	unsigned int max_ring_pair_counts;
	u16 tx_work_limit;
	__be16 vxlan_port;
	/* Rx fast path data */
	int num_rx_queues;
	u16 rx_itr_setting;
	u32 eth_queue_idx;
	u32 max_rate[MAX_TX_QUEUES];
	/* TX */
	struct rnp_ring *tx_ring[MAX_TX_QUEUES] ____cacheline_aligned_in_smp;
	int tx_ring_item_count;
	u64 restart_queue;
	u64 lsc_int;
	u32 tx_timeout_count;
	/* RX */
	struct rnp_ring *rx_ring[MAX_RX_QUEUES];
	int rx_ring_item_count;
	u64 hw_csum_rx_error;
	u64 hw_csum_rx_good;
	u64 hw_rx_no_dma_resources;
	u64 rsc_total_count;
	u64 rsc_total_flush;
	u64 non_eop_descs;
	u32 alloc_rx_page_failed;
	u32 alloc_rx_buff_failed;
	int num_other_vectors;
	int irq_mode;
	struct rnp_q_vector *q_vector[MAX_Q_VECTORS];
	/* used for IEEE 1588 ptp clock start */
	u8 __iomem *ptp_addr;
	int gmac4;
	const struct rnp_hwtimestamp *hwts_ops;
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_clock_ops;
	struct sk_buff *ptp_tx_skb;
	struct hwtstamp_config tstamp_config;
	u32 ptp_config_value;
	spinlock_t ptp_lock; /* Used to protect the SYSTIME registers. */
	u64 clk_ptp_rate; /* uint is HZ 1MHz＝1 000 000Hz */
	u32 sub_second_inc;
	u32 systime_flags;
	struct timespec64 ptp_prev_hw_time;
	unsigned int default_addend;
	bool ptp_tx_en;
	bool ptp_rx_en;
	struct work_struct tx_hwtstamp_work;
	unsigned long tx_hwtstamp_start;
	unsigned long tx_hwtstamp_skipped;
	unsigned long tx_timeout_factor;
	u64 tx_hwtstamp_timeouts;
	/*used for IEEE 1588 ptp clock end */
	/* DCB parameters */
	struct rnp_dcb_cfg dcb_cfg;
	u8 prio_tc_map[RNP_MAX_USER_PRIO * 2];
	u8 num_tc;
	int num_q_vectors; /* current number of q_vectors for device */
	int max_q_vectors; /* true count of q_vectors for device */
	struct rnp_ring_feature ring_feature[RING_F_ARRAY_SIZE];
	struct msix_entry *msix_entries;
	u32 test_icr;
	struct rnp_ring test_tx_ring;
	struct rnp_ring test_rx_ring;
	/* structs defined in rnp_hw.h */
	struct rnp_hw hw;
	u16 msg_enable;
	struct rnp_hw_stats hw_stats;
	u64 tx_busy;
	u32 link_speed;
	bool link_up;
	bool duplex_status;
	u32 link_speed_old;
	bool link_up_old;
	bool duplex_old;
	unsigned long link_check_timeout;
	struct timer_list service_timer;
	struct work_struct service_task;
	/* fdir relative */
	struct hlist_head fdir_filter_list;
	unsigned long fdir_overflow; /* number of times ATR was backed off */
	union rnp_atr_input fdir_mask;
	int fdir_mode;
	int fdir_filter_count;
	int layer2_count;
	int tuple_5_count;
	u32 fdir_pballoc;
	u32 atr_sample_rate;
	spinlock_t fdir_perfect_lock;
	u8 __iomem *io_addr_bar0;
	u8 __iomem *io_addr;
	u32 wol;
	u16 bd_number;
	u16 q_vector_off;
	u16 eeprom_verh;
	u16 eeprom_verl;
	u16 eeprom_cap;
	u16 stags_vid;
	u32 sysfs_tx_ring_num;
	u32 sysfs_rx_ring_num;
	u32 sysfs_tx_desc_num;
	u32 sysfs_rx_desc_num;
	u32 interrupt_event;
	u32 led_reg;
	/* maintain */
	char *maintain_buf;
	int maintain_buf_len;
	void *maintain_dma_buf;
	dma_addr_t maintain_dma_phy;
	int maintain_dma_size;
	int maintain_in_bytes;
	/* SR-IOV */
	DECLARE_BITMAP(active_vfs, RNP_MAX_VF_FUNCTIONS);
	unsigned int num_vfs;
	struct vf_data_storage *vfinfo;
	int vf_rate_link_speed;
	struct vf_macvlans vf_mvs;
	struct vf_macvlans *mv_list;
	u32 timer_event_accumulator;
	u32 vferr_refcount;
	struct kobject *info_kobj;
#ifdef RNP_SYSFS
#ifdef RNP_HWMON
	struct hwmon_buff *rnp_hwmon_buff;
#endif /* RNP_HWMON */
#endif /* RNPM_SYSFS */
#ifdef CONFIG_DEBUG_FS
	struct dentry *rnp_dbg_adapter;
#endif /*CONFIG_DEBUG_FS*/
	u8 default_up;
	u8 port; /* nr_pf_port: 0 or 1 */
	u8 portid_of_card; /* port num in card*/
#define RNP_MAX_RETA_ENTRIES 512
	u8 rss_indir_tbl[RNP_MAX_RETA_ENTRIES];
#define RNP_MAX_TC_ENTRIES 8
	u8 rss_tc_tbl[RNP_MAX_TC_ENTRIES];
	int rss_indir_tbl_num;
	int rss_tc_tbl_num;
	u32 rss_tbl_setup_flag;
#define RNP_RSS_KEY_SIZE 40 /* size of RSS Hash Key in bytes */
	u8 rss_key[RNP_RSS_KEY_SIZE];
	u32 rss_key_setup_flag;
	u32 sysfs_is_phy_ext_reg;
	u32 sysfs_phy_reg;
	u32 sysfs_bar4_reg_val;
	u32 sysfs_bar4_reg_addr;
	u32 sysfs_pcs_lane_num;
	int sysfs_input_arg_cnt;
	bool dma2_in_1pf;
	char name[60];
	void *csl_dma_buf;
	dma_addr_t csl_dma_phy;
	int csl_dma_size;
};

struct device_list_own {
	unsigned short vendor;
	unsigned short device;
};

struct rnp_fdir_filter {
	struct hlist_node fdir_node;
	union rnp_atr_input filter;
	u16 sw_idx;
	u16 hw_idx;
	u32 vf_num;
	u64 action;
};

enum rnp_state_t {
	__RNP_TESTING,
	__RNP_RESETTING,
	__RNP_DOWN,
	__RNP_SERVICE_SCHED,
	__RNP_IN_SFP_INIT,
	__RNP_READ_I2C,
	__RNP_PTP_TX_IN_PROGRESS,
	__RNP_USE_VFINFI,
	__RNP_IN_IRQ,
	__RNP_REMOVE,
	__RNP_SERVICE_CHECK,
};

struct rnp_cb {
	union { /* Union defining head/tail partner */
		struct sk_buff *head;
		struct sk_buff *tail;
	};
	dma_addr_t dma;
	u16 append_cnt;
	bool page_released;
};
#define RNP_CB(skb) ((struct rnp_cb *)(skb)->cb)

enum rnp_boards {
	board_n10_709_1pf_2x10G,
	board_vu440s,
	board_n10,
	board_n400,
};

#if IS_ENABLED(CONFIG_DCB)
extern const struct dcbnl_rtnl_ops dcbnl_ops;
#endif

extern char rnp_driver_name[];
extern const char rnp_driver_version[];

extern void rnp_up(struct rnp_adapter *adapter);
extern void rnp_down(struct rnp_adapter *adapter);
extern void rnp_reinit_locked(struct rnp_adapter *adapter);
extern void rnp_reset(struct rnp_adapter *adapter);
extern void rnp_set_ethtool_ops(struct net_device *netdev);
extern int rnp_setup_rx_resources(struct rnp_ring *, struct rnp_adapter *);
extern int rnp_setup_tx_resources(struct rnp_ring *, struct rnp_adapter *);
extern void rnp_free_rx_resources(struct rnp_ring *);
extern void rnp_free_tx_resources(struct rnp_ring *);
extern void rnp_configure_rx_ring(struct rnp_adapter *, struct rnp_ring *);
extern void rnp_configure_tx_ring(struct rnp_adapter *, struct rnp_ring *);
extern void rnp_disable_rx_queue(struct rnp_adapter *adapter,
				 struct rnp_ring *);
extern void rnp_update_stats(struct rnp_adapter *adapter);
extern int rnp_init_interrupt_scheme(struct rnp_adapter *adapter);
extern int rnp_wol_supported(struct rnp_adapter *adapter, u16 device_id,
			     u16 subdevice_id);
extern void rnp_clear_interrupt_scheme(struct rnp_adapter *adapter);
extern netdev_tx_t rnp_xmit_frame_ring(struct sk_buff *, struct rnp_adapter *,
				       struct rnp_ring *, bool);
extern void rnp_alloc_rx_buffers(struct rnp_ring *, u16);
extern int rnp_poll(struct napi_struct *napi, int budget);
extern int ethtool_ioctl(struct ifreq *ifr);
extern void rnp_release_hw_control(struct rnp_adapter *adapter);
extern void rnp_get_hw_control(struct rnp_adapter *adapter);
extern s32 rnp_fdir_write_perfect_filter(int fdir_mode, struct rnp_hw *hw,
					 union rnp_atr_input *filter, u16 hw_id,
					 u8 queue, bool prio_flag);
extern void rnp_set_rx_mode(struct net_device *netdev);
#ifdef CONFIG_RNP_DCB
extern void rnp_set_rx_drop_en(struct rnp_adapter *adapter);
#endif
extern int rnp_setup_tx_maxrate(struct rnp_ring *tx_ring, u64 max_rate,
				int sample_interval);
extern int rnp_setup_tc(struct net_device *dev, u8 tc);
void rnp_check_options(struct rnp_adapter *adapter);
extern int rnp_open(struct net_device *netdev);
extern int rnp_close(struct net_device *netdev);
void rnp_tx_ctxtdesc(struct rnp_ring *tx_ring, u32 mss_len_vf_num,
		     u32 inner_vlan_tunnel_len, int ignore_vlan, bool crc_pad);
void rnp_maybe_tx_ctxtdesc(struct rnp_ring *tx_ring,
			   struct rnp_tx_buffer *first, u32 type_tucmd);
extern void rnp_store_reta(struct rnp_adapter *adapter);
extern void rnp_store_key(struct rnp_adapter *adapter);
extern int rnp_init_rss_key(struct rnp_adapter *adapter);
extern int rnp_init_rss_table(struct rnp_adapter *adapter);
extern void rnp_setup_dma_rx(struct rnp_adapter *adapter, int count_in_dw);
extern s32 rnp_fdir_erase_perfect_filter(int fdir_mode, struct rnp_hw *hw,
					 union rnp_atr_input *input, u16 hw_id);
extern u32 rnp_rss_indir_tbl_entries(struct rnp_adapter *adapter);
extern u32 rnp_tx_desc_unused_sw(struct rnp_ring *tx_ring);
extern u32 rnp_tx_desc_unused_hw(struct rnp_hw *hw, struct rnp_ring *tx_ring);
extern s32 rnp_disable_rxr_maxrate(struct net_device *netdev, u8 queue_index);
extern s32 rnp_enable_rxr_maxrate(struct net_device *netdev, u8 queue_index,
				  u32 maxrate);
extern u32 rnp_rx_desc_used_hw(struct rnp_hw *hw, struct rnp_ring *rx_ring);
extern void rnp_do_reset(struct net_device *netdev);
#ifdef RNP_HWMON
extern void rnp_sysfs_exit(struct rnp_adapter *adapter);
extern int rnp_sysfs_init(struct rnp_adapter *adapter);
#endif /* CONFIG_RNP_HWMON */
#ifdef CONFIG_DEBUG_FS
extern void rnp_dbg_adapter_init(struct rnp_adapter *adapter);
extern void rnp_dbg_adapter_exit(struct rnp_adapter *adapter);
extern void rnp_dbg_init(void);
extern void rnp_dbg_exit(void);
#else
static inline void rnp_dbg_adapter_init(struct rnp_adapter *adapter)
{
}
static inline void rnp_dbg_adapter_exit(struct rnp_adapter *adapter)
{
}
static inline void rnp_dbg_init(void)
{
}
static inline void rnp_dbg_exit(void)
{
}
#endif /* CONFIG_DEBUG_FS */
static inline struct netdev_queue *txring_txq(const struct rnp_ring *ring)
{
	return netdev_get_tx_queue(ring->netdev, ring->queue_index);
}

void rnp_service_event_schedule(struct rnp_adapter *adapter);
extern void rnp_ptp_init(struct rnp_adapter *adapter);
extern void rnp_ptp_stop(struct rnp_adapter *adapter);
extern void rnp_ptp_overflow_check(struct rnp_adapter *adapter);
extern void rnp_ptp_rx_hang(struct rnp_adapter *adapter);
extern void __rnp_ptp_rx_hwtstamp(struct rnp_q_vector *q_vector,
				  struct sk_buff *skb);
static inline void rnp_ptp_rx_hwtstamp(struct rnp_ring *rx_ring,
				       union rnp_rx_desc *rx_desc,
				       struct sk_buff *skb)
{
	if (unlikely(!rnp_test_staterr(rx_desc, RNP_RXD_STAT_PTP)))
		return;
	/*
	 * Update the last_rx_timestamp timer in order to enable watchdog check
	 * for error case of latched timestamp on a dropped packet.
	 */
	rx_ring->last_rx_timestamp = jiffies;
}

static inline int ignore_veb_vlan(struct rnp_adapter *adapter,
				  union rnp_rx_desc *rx_desc)
{
	if (unlikely((adapter->flags & RNP_FLAG_SRIOV_ENABLED) &&
		     (cpu_to_le16(rx_desc->wb.rev1) & VEB_VF_IGNORE_VLAN))) {
		return 1;
	}
	return 0;
}

static inline int ignore_veb_pkg_err(struct rnp_adapter *adapter,
				     union rnp_rx_desc *rx_desc)
{
	if (unlikely((adapter->flags & RNP_FLAG_SRIOV_ENABLED) &&
		     (cpu_to_le16(rx_desc->wb.rev1) & VEB_VF_PKG))) {
		return 1;
	}
	return 0;
}

int rnp_update_ethtool_fdir_entry(struct rnp_adapter *adapter,
				  struct rnp_fdir_filter *input, u16 sw_idx);

static inline int rnp_is_pf1(struct rnp_hw *hw)
{
	return !!(hw->pfvfnum & BIT(PF_BIT));
}

static inline int rnp_is_pf0(struct rnp_hw *hw)
{
	return !rnp_is_pf1(hw);
}

static inline int rnp_get_fuc(struct pci_dev *pdev)
{
	return pdev->devfn;
}

extern void rnp_service_task(struct work_struct *work);
extern void rnp_sysfs_exit(struct rnp_adapter *adapter);
extern int rnp_sysfs_init(struct rnp_adapter *adapter);

#ifdef CONFIG_PCI_IOV
void rnp_sriov_reinit(struct rnp_adapter *adapter);
#endif

#define SET_BIT(n, var) (var = (var | (1 << n)))
#define CLR_BIT(n, var) (var = (var & (~(1 << n))))
#define CHK_BIT(n, var) (var & (1 << n))
#define RNP_RX_DMA_ATTR (DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)

static inline bool rnp_removed(void __iomem *addr)
{
	return unlikely(!addr);
}
#define RNP_REMOVED(a) rnp_removed(a)
int rnp_fw_msg_handler(struct rnp_adapter *adapter);

int rnp500_fw_update(struct rnp_hw *hw, int partition, const u8 *fw_bin,
		     int bytes);

int rnp_fw_update(struct rnp_hw *hw, int partition, const u8 *fw_bin,
		  int bytes);
#define RNPM_FW_VERSION_NEW_ETHTOOL 0x00050010
static inline bool rnp_fw_is_old_ethtool(struct rnp_hw *hw)
{
	return hw->fw_version >= RNPM_FW_VERSION_NEW_ETHTOOL ? false : true;
}

#endif /* _RNP_H_ */
