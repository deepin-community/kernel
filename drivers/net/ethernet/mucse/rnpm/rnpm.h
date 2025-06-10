/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#ifndef _RNPM_H_
#define _RNPM_H_
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
#include "rnpm_type.h"
#include "rnpm_common.h"

#ifdef RNPM_OPTM_WITH_LPAGE
#ifdef CONFIG_RNPM_DISABLE_PACKET_SPLIT
#error "RNPM_OPTM_WITH_LPAGE exclude from CONFIG_RNPM_DISABLE_PACKET_SPLIT "
#endif
#endif

/* common prefix used by pr_<> macros */
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#define RNPM_ALLOC_PAGE_ORDER (0)
#define RNPM_PAGE_BUFFER_NUMS(ring)                              \
	((1 << RNPM_ALLOC_PAGE_ORDER) * PAGE_SIZE /              \
	 ALIGN((rnpm_rx_offset(ring) + rnpm_rx_bufsz(ring) +     \
		SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) + \
		RNPM_RX_HWTS_OFFSET),                            \
	       1024))

/* TX/RX descriptor defines */
#ifdef FEITENG
#define RNPM_DEFAULT_TXD (1024)
#else
#define RNPM_DEFAULT_TXD (1024)
#endif

#define RNPM_N400_DEFAULT_TXD (256)

#define RNPM_DEFAULT_TX_WORK (256)
#define RNPM_MIN_TX_WORK (32)
#define RNPM_MAX_TX_WORK (512)
#define RNPM_MIN_RX_WORK (32)
#define RNPM_MAX_RX_WORK (512)
#define RNPM_WORK_ALIGN (2)
#define RNPM_MIN_TX_FRAME (1)
#define RNPM_MAX_TX_FRAME (256)
#define RNPM_MIN_TX_USEC (30)
#define RNPM_MAX_TX_USEC (10000)

#define RNPM_DEFAULT_HIGH_RX_USEC (1600)
#define RNPM_DEFAULT_LOW_RX_USEC (200)

#ifndef RNPM_IRQ_CHECK_USEC
#define RNPM_IRQ_CHECK_USEC 1000
#endif

#define RNPM_MIN_RX_FRAME (1)
#define RNPM_MAX_RX_FRAME (256)
#define RNPM_MIN_RX_USEC (10)
#define RNPM_MAX_RX_USEC (10000)

#define RNP_MAX_VF_FUNCTIONS 64

#define RNPM_MAX_TXD (8192)
#define RNPM_MIN_TXD (64)

#define RNPM_DEFAULT_SAMPLE_INTERVAL (10)
#define RNPM_DEFAULT_ENABLE (1)
#define RNPM_DEFAULT_DISABLE (0)
#define RNPM_DEFAULT_NAPI_BUDGE (64)
#define RNPM_REQ_TX_DESCRIPTOR_MULTIPLE (8)
#define RNPM_REQ_RX_DESCRIPTOR_MULTIPLE (8)

#ifdef FEITENG
#define RNPM_DEFAULT_RXD (1024)
#else
#define RNPM_DEFAULT_RXD (1024)
#endif

#define RNPM_MAX_RXD (8192)
#define RNPM_MIN_RXD (64)

/* Phy */
#define AUTO_ALL_MODES 0

/* flow control */
#define RNPM_MIN_FCRTL (0x40)
#define RNPM_MAX_FCRTL (0x7FF80)
#define RNPM_MIN_FCRTH (0x600)
#define RNPM_MAX_FCRTH (0x7FFF0)
#define RNPM_DEFAULT_FCPAUSE (0xffff)
#define RNPM_DEFAULT_HIGH_WATER (0x320)
#define RNPM_DEFAULT_LOW_WATER (0x270)
#define RNPM_MIN_FCPAUSE (0)
#define RNPM_MAX_FCPAUSE (0xFFFF)

/* Supported Rx Buffer Sizes */
#define RNPM_RXBUFFER_256 256
#define RNPM_RXBUFFER_1536 1536
#define RNPM_RXBUFFER_2K 2048
#define RNPM_RXBUFFER_3K 3072
#define RNPM_RXBUFFER_4K 4096
#define RNPM_MAX_RXBUFFER 16384 /* largest size for a single descriptor */
#define RNPM_RXBUFFER_MAX (RNPM_RXBUFFER_2K)

#ifdef CONFIG_RNPM_DISABLE_PACKET_SPLIT
#define RNPM_RXBUFFER_7K 7168
#define RNPM_RXBUFFER_8K 8192
#define RNPM_RXBUFFER_15K 15360
#endif /* CONFIG_RNPM_DISABLE_PACKET_SPLIT */

#define MAX_Q_VECTORS 128
#define RNPM_RING_COUNTS_PEER_PF 8

#define RNPM_GSO_PARTIAL_FEATURES                 \
	(NETIF_F_GSO_GRE | NETIF_F_GSO_GRE_CSUM | \
	 NETIF_F_GSO_UDP_TUNNEL | NETIF_F_GSO_UDP_TUNNEL_CSUM)
/*
 * NOTE: netdev_alloc_skb reserves up to 64 bytes, NET_IP_ALIGN means we
 * reserve 64 more, and skb_shared_info adds an additional 320 bytes more,
 * this adds up to 448 bytes of extra data.
 *
 * Since netdev_alloc_skb now allocates a page fragment we can use a value
 * of 256 and the resultant skb will have a truesize of 960 or less.
 */
#define RNPM_RX_HDR_SIZE RNPM_RXBUFFER_256

#define RNPM_ITR_ADAPTIVE_MIN_INC 2
#define RNPM_ITR_ADAPTIVE_MIN_USECS 8
#define RNPM_ITR_ADAPTIVE_MAX_USECS 800
#define RNPM_ITR_ADAPTIVE_LATENCY 0x400
#define RNPM_ITR_ADAPTIVE_BULK 0x00

/* How many Rx Buffers do we bundle into one write to the hardware ? */
#ifdef RNPM_OPTM_WITH_LPAGE
#define RNPM_RX_BUFFER_WRITE (PAGE_SIZE / 2048) /* Must be power of 2 */
#else
#define RNPM_RX_BUFFER_WRITE 16 /* Must be power of 2 */
#endif
enum rnpm_tx_flags {
	/* cmd_type flags */
	RNPM_TX_FLAGS_HW_VLAN = 0x01,
	RNPM_TX_FLAGS_TSO = 0x02,
	RNPM_TX_FLAGS_TSTAMP = 0x04,

	/* olinfo flags */
	RNPM_TX_FLAGS_CC = 0x08,
	RNPM_TX_FLAGS_IPV4 = 0x10,
	RNPM_TX_FLAGS_CSUM = 0x20,

	/* software defined flags */
	RNPM_TX_FLAGS_SW_VLAN = 0x40,
	RNPM_TX_FLAGS_FCOE = 0x80,
};

/* modify this in asic version */
#define RNPM_MAX_VF_CNT 64

#define RNPM_RX_RATE_HIGH 450000
#define RNPM_RX_COAL_TIME_HIGH 128
#define RNPM_RX_SIZE_THRESH 1024
#define RNPM_RX_RATE_THRESH (1000000 / RNPM_RX_COAL_TIME_HIGH)
#define RNPM_SAMPLE_INTERVAL 0
#define RNPM_AVG_PKT_SMALL 256

#define RNPM_MAX_VF_MC_ENTRIES 30
#define RNPM_MAX_VF_FUNCTIONS RNPM_MAX_VF_CNT
#define RNPM_MAX_VFTA_ENTRIES 128
#define MAX_EMULATION_MAC_ADDRS 16
#define RNPM_MAX_PF_MACVLANS 15
#define PF_RING_CNT_WHEN_IOV_ENABLED 2
#define VMDQ_P(p) ((p) + adapter->ring_feature[RING_F_VMDQ].offset)

struct vf_data_storage {
	unsigned char vf_mac_addresses[ETH_ALEN];
	u16 vf_mc_hashes[RNPM_MAX_VF_MC_ENTRIES];
	u16 num_vf_mc_hashes;
	u16 default_vf_vlan_id;
	u16 vlans_enabled;
	bool clear_to_send;
	bool pf_set_mac;
	u16 pf_vlan; /* When set, guest VLAN config not allowed. */
	u16 pf_qos;
	u16 tx_rate;
	u16 vlan_count;
	u8 spoofchk_enabled;
	unsigned int vf_api;
};

struct vf_macvlans {
	struct list_head l;
	int vf;
	int rar_entry;
	bool free;
	bool is_macvlan;
	u8 vf_macvlan[ETH_ALEN];
};

/* tx max 4k for one desc */
#define RNPM_MAX_TXD_PWR 12
#define RNPM_MAX_DATA_PER_TXD (1 << RNPM_MAX_TXD_PWR)

/* Tx Descriptors needed, worst case */
#define TXD_USE_COUNT(S) DIV_ROUND_UP((S), RNPM_MAX_DATA_PER_TXD)
#define DESC_NEEDED (MAX_SKB_FRAGS + 4)

/* wrapper around a pointer to a socket buffer,
 * so a DMA handle can be stored along with the buffers
 */
struct rnpm_tx_buffer {
	struct rnpm_tx_desc *next_to_watch;
	unsigned long time_stamp;
	struct sk_buff *skb;
	unsigned int bytecount;
	unsigned short gso_segs;
#ifdef RNPM_FIX_MAC_PADDING
	bool gso_need_padding;
#endif
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
	u32 type_tucmd;
	bool ctx_flag;
};

struct rnpm_rx_buffer {
	struct sk_buff *skb;
	dma_addr_t dma;
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
	struct page *page;
#if (BITS_PER_LONG > 32) || (PAGE_SIZE >= 65536)
	__u32 page_offset;
#else
	__u16 page_offset;
#endif
	__u16 pagecnt_bias;
#endif
};

struct rnpm_queue_stats {
	u64 packets;
	u64 bytes;
};

struct rnpm_tx_queue_stats {
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
	u64 tx_irq_miss;
	u64 tx_next_to_clean;
	u64 tx_equal_count;
};

struct rnpm_rx_queue_stats {
	u64 driver_drop_packets;
	u64 rsc_count;
	u64 rsc_flush;
	u64 non_eop_descs;
	u64 alloc_rx_page_failed;
	u64 alloc_rx_buff_failed;
	u64 csum_err;
	u64 csum_good;
	u64 poll_again_count;
	u64 vlan_remove;
	u64 alloc_rx_page;
	u64 rx_irq_miss;
	u64 rx_next_to_clean;
	u64 rx_equal_count;
	u64 rx_poll_packets;
	u64 rx_poll_avg_packets;
	u64 rx_poll_itr;
};

enum rnpm_ring_state_t {
#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
	__RNPM_RX_3K_BUFFER,
	__RNPM_RX_BUILD_SKB_ENABLED,
#endif
	__RNPM_TX_FDIR_INIT_DONE,
	__RNPM_TX_XPS_INIT_DONE,
	__RNPM_TX_DETECT_HANG,
	__RNPM_HANG_CHECK_ARMED,
	__RNPM_RX_CSUM_UDP_ZERO_ERR,
	__RNPM_RX_FCOE,
};

#ifndef CONFIG_RNPM_DISABLE_PACKET_SPLIT
#define ring_uses_build_skb(ring) \
	test_bit(__RNPM_RX_BUILD_SKB_ENABLED, &(ring)->state)
#endif

#define check_for_tx_hang(ring) \
	test_bit(__RNPM_TX_DETECT_HANG, &(ring)->state)
#define set_check_for_tx_hang(ring) \
	set_bit(__RNPM_TX_DETECT_HANG, &(ring)->state)
#define clear_check_for_tx_hang(ring) \
	clear_bit(__RNPM_TX_DETECT_HANG, &(ring)->state)

#define netdev_ring(ring) (ring->netdev)
struct rnpm_ring {
	struct rnpm_ring *next; /* pointer to next ring in q_vector */
	struct rnpm_q_vector *q_vector; /* backpointer to host q_vector */
	struct net_device *netdev; /* netdev ring belongs to */
	struct device *dev; /* device for DMA mapping */
	void *desc; /* descriptor ring memory */
	union {
		struct rnpm_tx_buffer *tx_buffer_info;
		struct rnpm_rx_buffer *rx_buffer_info;
	};
	unsigned long last_rx_timestamp;
	unsigned long state;
	u8 __iomem *tail;
	u8 __iomem *dma_hw_addr;
	u8 __iomem *dma_int_stat;
	u8 __iomem *dma_int_mask;
	u8 __iomem *dma_int_clr;
	dma_addr_t dma; /* phys. address of descriptor ring */
	unsigned int size; /* length in bytes */
	u32 ring_flags;
#define RNPM_RING_FLAG_DELAY_SETUP_RX_LEN ((u32)(1 << 0))
#define RNPM_RING_FLAG_CHANGE_RX_LEN ((u32)(1 << 1))
#define RNPM_RING_FLAG_DO_RESET_RX_LEN ((u32)(1 << 2))
	u8 pfvfnum;
	u8 gso_padto_bytes;

	u16 count; /* amount of descriptors */
	u16 temp_count;
	u16 reset_count;

	u8 queue_index; /* queue_index needed for multiqueue queue management */
	u8 rnpm_queue_idx; /*the real ring,used by dma*/
	u16 next_to_use;
	u16 next_to_clean;

	u16 device_id;
#ifdef RNPM_OPTM_WITH_LPAGE
	u16 rx_page_buf_nums;
	u32 rx_per_buf_mem;
	struct sk_buff *skb;
#endif
	union {
#ifdef CONFIG_RNPM_DISABLE_PACKET_SPLIT
		u16 rx_buf_len;
#else
		u16 next_to_alloc;
#endif
		struct {
			u8 atr_sample_rate;
			u8 atr_count;
		};
	};

	u8 dcb_tc;
	struct rnpm_queue_stats stats;
	struct u64_stats_sync syncp;
	union {
		struct rnpm_tx_queue_stats tx_stats;
		struct rnpm_rx_queue_stats rx_stats;
	};
} ____cacheline_internodealigned_in_smp;

#define RING2ADAPT(ring) netdev_priv((ring)->netdev)

enum rnpm_ring_f_enum {
	RING_F_NONE = 0,
	RING_F_VMDQ, /* SR-IOV uses the same ring feature */
	RING_F_RSS,
	RING_F_FDIR,
	RING_F_ARRAY_SIZE /* must be last in enum set */
};

#define MAX_RX_QUEUES (128)
#define MAX_TX_QUEUES (128)
#define MAX_PORT_NUM (4) /* one pf has 4 ports max */
struct rnpm_ring_feature {
	u16 limit; /* upper limit on feature indices */
	u16 indices; /* current value of indices */
	u16 mask; /* Mask used for feature to ring mapping */
	u16 offset; /* offset to start of feature */
} ____cacheline_internodealigned_in_smp;

#define RNPM_n10_VMDQ_8Q_MASK 0x78
#define RNPM_n10_VMDQ_4Q_MASK 0x7C
#define RNPM_n10_VMDQ_2Q_MASK 0x7E

/*
 * FCoE requires that all Rx buffers be over 2200 bytes in length.  Since
 * this is twice the size of a half page we need to double the page order
 * for FCoE enabled Rx queues.
 */
static inline unsigned int rnpm_rx_bufsz(struct rnpm_ring *ring)
{
	/* rx-desc trans max half page(2048), for jumbo frame sg is needed */
	return RNPM_RXBUFFER_1536 - NET_IP_ALIGN;
}

/* SG , rx-desc use one page */
static inline unsigned int rnpm_rx_pg_order(struct rnpm_ring *ring)
{
	return 0;
}
#define rnpm_rx_pg_size(_ring) (PAGE_SIZE << rnpm_rx_pg_order(_ring))

struct rnpm_ring_container {
	struct rnpm_ring *ring; /* pointer to linked list of rings */
	unsigned long next_update; /* jiffies value of last update */
	unsigned int total_bytes; /* total bytes processed this int */
	unsigned int total_packets; /* total packets processed this int */
	/* record rnpm poll function exec times in one jiffies */
	unsigned int poll_times;
	u16 work_limit; /* total work allowed per interrupt */
	u8 count; /* total number of rings in vector */
	u16 itr; /* current ITR/MSIX vector setting for ring */
};

/* iterator for handling rings in ring container */
#define rnpm_for_each_ring(pos, head) \
	for (pos = (head).ring; pos != NULL; pos = pos->next)

#define MAX_RX_PACKET_BUFFERS \
	((adapter->flags & RNPM_FLAG_DCB_ENABLED) ? 8 : 1)
#define MAX_TX_PACKET_BUFFERS MAX_RX_PACKET_BUFFERS

/* MAX_Q_VECTORS of these are allocated,
 * but we only use one per queue-specific vector.
 */
struct rnpm_q_vector {
	int new_rx_count;
	int old_rx_count;
	struct rnpm_adapter *adapter;
	int factor;
	u16 v_idx;
	u16 itr;
	struct rnpm_ring_container rx, tx;

	struct napi_struct napi;
	cpumask_t affinity_mask;
	struct irq_affinity_notify affinity_notify;
	int numa_node;
	struct rcu_head rcu; /* to avoid race with update stats on free */
	int irq_check_usecs;
	struct hrtimer irq_miss_check_timer;
#define RNPM_IRQ_MISS_HANDLE_DONE ((u32)(1 << 0))
	unsigned long flags;
	char name[IFNAMSIZ + 9];
	/* for dynamic allocation of rings associated with this q_vector */
	struct rnpm_ring ring[0] ____cacheline_internodealigned_in_smp;
};

#define RNPM_HWMON_TYPE_LOC 0
#define RNPM_HWMON_TYPE_TEMP 1
#define RNPM_HWMON_TYPE_CAUTION 2
#define RNPM_HWMON_TYPE_MAX 3
#define RNPM_HWMON_TYPE_NAME 4

struct hwmon_attr {
	struct device_attribute dev_attr;
	struct rnpm_hw *hw;
	struct rnpm_thermal_diode_data *sensor;
	char name[12];
};

struct hwmon_buff {
	struct attribute_group group;
	const struct attribute_group *groups[2];
	struct attribute *attrs[RNPM_MAX_SENSORS * 4 + 1];
	struct hwmon_attr hwmon_list[RNPM_MAX_SENSORS * 4];
	unsigned int n_hwmon;
};

/* rnpm_test_staterr - tests bits in Rx descriptor status and error fields */
static inline __le16 rnpm_test_staterr(union rnpm_rx_desc *rx_desc,
				       const u16 stat_err_bits)
{
	return rx_desc->wb.cmd & cpu_to_le16(stat_err_bits);
}

static inline __le16 rnpm_get_stat(union rnpm_rx_desc *rx_desc,
				   const u16 stat_mask)
{
	return rx_desc->wb.cmd & cpu_to_le16(stat_mask);
}

static inline u16 rnpm_desc_unused(struct rnpm_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->count) + ntc - ntu - 1;
}

static inline u16 rnpm_desc_unused_rx(struct rnpm_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->count) + ntc - ntu - 16;
}

#define RNPM_RX_DESC(R, i) (&(((union rnpm_rx_desc *)((R)->desc))[i]))
#define RNPM_TX_DESC(R, i) (&(((struct rnpm_tx_desc *)((R)->desc))[i]))
#define RNPM_TX_CTXTDESC(R, i) \
	(&(((struct rnpm_tx_ctx_desc *)((R)->desc))[i]))

#define RNPM_MAX_JUMBO_FRAME_SIZE 9590 /* Maximum Supported Size 9.5KB */
#define RNPM_MIN_MTU 68

#define OTHER_VECTOR 1
#define NON_Q_VECTORS (OTHER_VECTOR)

/* default to trying for four seconds */
#define RNPM_TRY_LINK_TIMEOUT (4 * HZ)

#define RNPM_MAX_USER_PRIO (8)
#define RNPM_MAX_TCS_NUM (3)
struct rnpm_pfc_cfg {
	u8 pfc_max; /* hardware can enabled max pfc channel */
	u8 hw_pfc_map; /* enable the prio channel bit */
	u8 pfc_num; /* at present enabled the pfc-channel num */
	u8 pfc_en; /* enabled the pfc feature or not */
};

struct rnpm_dcb_cfg {
	u8 tc_num;
	u16 delay; /* pause time */
	u8 dcb_en; /* enabled the dcb feature or not */
	u8 dcbx_mode;
	struct rnpm_pfc_cfg pfc_cfg;
	u64 requests[RNPM_MAX_TCS_NUM];
	u64 indications[RNPM_MAX_TCS_NUM];
	enum rnpm_fc_mode last_lfc_mode;
};

struct mbx_fw_cmd_reply;

typedef void (*cookie_cb)(struct mbx_fw_cmd_reply *reply, void *priv);

enum cookie_stat {
	COOKIE_FREE = 0,
	COOKIE_FREE_WAIT_TIMEOUT,
	COOKIE_ALLOCED,
};

struct mbx_req_cookie {
	u64 alloced_jiffies;
	enum cookie_stat stat;

	cookie_cb cb;
	int timeout_jiffes;
	int errcode;

	wait_queue_head_t wait;

	int done;
	int priv_len;
#define MAX_PRIV_LEN 64
	char priv[MAX_PRIV_LEN];
};

struct mbx_req_cookie_pool {
#define MAX_COOKIES_ITEMS (20 * 400)
	struct mbx_req_cookie cookies[MAX_COOKIES_ITEMS];
	int next_idx;
};

/* board pf adapter */
struct rnpm_pf_adapter {
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
	spinlock_t vlan_setup_lock;
	spinlock_t drop_setup_lock;
	spinlock_t dummy_setup_lock;
	spinlock_t pf_setup_lock;

	struct timer_list service_timer;
	struct work_struct service_task;

#define RNPM_PF_RESET ((u32)(1 << 0))
#define RNPM_PF_SET_MTU ((u32)(1 << 1))
#define RNPM_PF_LINK_CHANGE ((u32)(1 << 2))
#define RNPM_PF_SERVICE_SKIP_HANDLE ((u32)(1 << 3))

	unsigned long flags;

	struct pci_dev *pdev;
	struct rnpm_adapter *adapter[MAX_PORT_NUM];
	bool force_10g_1g_speed_ablity;
	int register_sequence[MAX_PORT_NUM];
	int adapter_cnt;
	u8 __iomem *hw_bar2;
	u8 __iomem *hw_addr;
	u8 __iomem *hw_addr4;
	u8 __iomem *hw_bar0;
	u32 board_type;
	u32 port_valid; /* only used in 8 ports */
	u32 port_names; /* only used in 8 ports */
	u32 bd_number;
	u8 __iomem *rpu_addr;
	u8 rpu_inited;
	u8 rpu_need_stop;
	/* msix table */
	struct msix_entry *msix_entries;
	int max_msix_counts[MAX_PORT_NUM];
	int other_irq;

	u8 pn[33];
	u8 sn[33];

	spinlock_t key_setup_lock;
	/* size of RSS Hash Key in bytes */
#define RNPM_RSS_KEY_SIZE 40
	u8 rss_key[RNPM_RSS_KEY_SIZE];
	u32 rss_key_setup_flag;
	u8 default_rx_ring;
	/* multicast addr */
	spinlock_t mc_setup_lock;
	u32 mcft_size;
	u32 mc_filter_type;
	u32 mc_location;
	/* netdev rx status */
	u32 num_mc_addrs[MAX_PORT_NUM];
	u32 mta_in_use[MAX_PORT_NUM];
#define RNPM_MAX_MTA 128
	u32 mta_shadow[RNPM_MAX_MTA];
	u32 fctrl[MAX_PORT_NUM];
	u32 vlan_filter_status[MAX_PORT_NUM];
	spinlock_t vlan_filter_lock;
	u32 vlan_status_true;
	/* priv_flags used by multiports */
	u32 priv_flags;
	spinlock_t priv_flags_lock;

	u8 lane_link_status;
	struct mutex mbx_lock;

	unsigned long state;
	u32 timer_count;
	struct rnpm_hw hw;
	char name[60];
	u32 mpe_shm;
	void *csl_dma_buf;
	dma_addr_t csl_dma_phy;
	int csl_dma_size;
	struct mbx_req_cookie_pool cookie_pool;
	int cpu_l3_cache_size;
};

enum priv_bits {
	mac_loopback = 0,
	switch_loopback = 1,
	veb_enable = 4,
	padding_enable = 8,
	padding_debug_enable = 0x10,
};

/* ring debug infomation */
struct rnpm_d_ringinfo {
	u16 txring_start;
	u16 txring_end;
	u16 rxring_start;
	u16 rxring_end;
	bool txring_vaild;
	bool rxring_vaild;
};

/* board specific private data structure */
struct rnpm_adapter {
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
	struct rnpm_pf_adapter *pf_adapter;
	/* OS defined structs */
	struct net_device *netdev;
	bool rm_mode;
	bool netdev_registered;
	u32 lane_intr_en;
	struct pci_dev *pdev;
	bool quit_poll_thread;
	struct task_struct *rx_poll_thread;
	unsigned long state;
	/* hw not Supported well */
	unsigned long last_moder_packets[MAX_RX_QUEUES];
	unsigned long last_moder_tx_packets;
	unsigned long last_moder_bytes[MAX_RX_QUEUES];
	unsigned long last_moder_jiffies;
	int last_moder_time[MAX_RX_QUEUES];
	u32 timer_count;
	u32 service_count;

	/* only rx itr is Supported */
	u32 rx_usecs;
	u32 rx_frames;
	u32 tx_usecs;
	u32 tx_frames;
	u32 pkt_rate_low;
	u16 rx_usecs_low;
	u32 pkt_rate_high;
	u16 rx_usecs_high;
	u32 sample_interval;
	u32 adaptive_rx_coal;
	u32 adaptive_tx_coal;
	u32 auto_rx_coal;

	int lane;
	int speed;
	u8 fake_force_1000m;

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

	u8 an : 1;
	u8 fec : 1;
	u8 link_traing : 1;
	u8 duplex : 1;

	/* Some features need tri-state capability,
	 * thus the additional *_CAPABLE flags.
	 */
	u32 vf_num_for_pf;
	u32 flags;
#define RNPM_FLAG_MSI_CAPABLE ((u32)(1 << 0))
#define RNPM_FLAG_MSI_ENABLED ((u32)(1 << 1))
#define RNPM_FLAG_MSIX_CAPABLE ((u32)(1 << 2))
#define RNPM_FLAG_MSIX_ENABLED ((u32)(1 << 3))
#define RNPM_FLAG_RX_1BUF_CAPABLE ((u32)(1 << 4))
#define RNPM_FLAG_RX_PS_CAPABLE ((u32)(1 << 5))
#define RNPM_FLAG_RX_PS_ENABLED ((u32)(1 << 6))
#define RNPM_FLAG_IN_NETPOLL ((u32)(1 << 7))
#define RNPM_FLAG_DCA_ENABLED ((u32)(1 << 8))
#define RNPM_FLAG_DCA_CAPABLE ((u32)(1 << 9))
#define RNPM_FLAG_IMIR_ENABLED ((u32)(1 << 10))
#define RNPM_FLAG_MQ_CAPABLE ((u32)(1 << 11))
#define RNPM_FLAG_DCB_ENABLED ((u32)(1 << 12))
#define RNPM_FLAG_VMDQ_CAPABLE ((u32)(1 << 13))
#define RNPM_FLAG_VMDQ_ENABLED ((u32)(1 << 14))
#define RNPM_FLAG_FAN_FAIL_CAPABLE ((u32)(1 << 15))
#define RNPM_FLAG_NEED_LINK_UPDATE ((u32)(1 << 16))
#define RNPM_FLAG_NEED_LINK_CONFIG ((u32)(1 << 17))
#define RNPM_FLAG_FDIR_HASH_CAPABLE ((u32)(1 << 18))
#define RNPM_FLAG_FDIR_PERFECT_CAPABLE ((u32)(1 << 19))
#define RNPM_FLAG_FCOE_CAPABLE ((u32)(1 << 20))
#define RNPM_FLAG_FCOE_ENABLED ((u32)(1 << 21))
#define RNPM_FLAG_SRIOV_CAPABLE ((u32)(1 << 22))
#define RNPM_FLAG_SRIOV_ENABLED ((u32)(1 << 23))
#define RNPM_FLAG_MUTIPORT_ENABLED ((u32)(1 << 24))
	/* only in mutiport mode */
#define RNPM_FLAG_RXHASH_DISABLE ((u32)(1 << 25))
#define RNPM_FLAG_VXLAN_OFFLOAD_CAPABLE ((u32)(1 << 26))
#define RNPM_FLAG_VXLAN_OFFLOAD_ENABLE ((u32)(1 << 27))
#define RNPM_FLAG_SWITCH_LOOPBACK_EN ((u32)(1 << 28))

	u32 flags2;
#define RNPM_FLAG2_RSC_CAPABLE ((u32)(1 << 0))
#define RNPM_FLAG2_RSC_ENABLED ((u32)(1 << 1))
#define RNPM_FLAG2_TEMP_SENSOR_CAPABLE ((u32)(1 << 2))
#define RNPM_FLAG2_TEMP_SENSOR_EVENT ((u32)(1 << 3))
#define RNPM_FLAG2_SEARCH_FOR_SFP ((u32)(1 << 4))
#define RNPM_FLAG2_SFP_NEEDS_RESET ((u32)(1 << 5))
#define RNPM_FLAG2_RESET_REQUESTED ((u32)(1 << 6))
#define RNPM_FLAG2_FDIR_REQUIRES_REINIT ((u32)(1 << 7))
#define RNPM_FLAG2_RSS_FIELD_IPV4_UDP ((u32)(1 << 8))
#define RNPM_FLAG2_RSS_FIELD_IPV6_UDP ((u32)(1 << 9))
#define RNPM_FLAG2_PTP_ENABLED ((u32)(1 << 10))
#define RNPM_FLAG2_PTP_PPS_ENABLED ((u32)(1 << 11))
#define RNPM_FLAG2_BRIDGE_MODE_VEB ((u32)(1 << 12))
#define RNPM_FLAG2_VLAN_STAGS_ENABLED ((u32)(1 << 13))
#define RNPM_FLAG2_UDP_TUN_REREG_NEEDED ((u32)(1 << 14))
#define RNPM_FLAG2_TX_RATE_SETUP ((u32)(1 << 14))
	u32 flags_feature;
#define RNPM_FLAG_DELAY_UPDATE_VLAN_FILTER ((u32)(1 << 0))
#define RNPM_FLAG_DELAY_UPDATE_VLAN_TABLE ((u32)(1 << 1))
#define RNPM_FLAG_DELAY_UPDATE_MUTICAST_TABLE ((u32)(1 << 1))
	u32 priv_flags;
#define RNPM_PRIV_FLAG_MAC_LOOPBACK BIT(0)
#define RNPM_PRIV_FLAG_SWITCH_LOOPBACK BIT(1)
#define RNPM_PRIV_FLAG_VEB_ENABLE BIT(2)
#define RNPM_PRIV_FLAG_PCIE_CACHE_ALIGN_PATCH BIT(3)
#define RNPM_PRIV_FLAG_PADDING_DEBUG BIT(4)
#define RNPM_PRIV_FLAG_PTP_DEBUG BIT(5)
#define RNPM_PRIV_FLAG_SIMUATE_DOWN BIT(6)
#define RNPM_PRIV_FLAG_TO_RPU BIT(7)
#define RNPM_PRIV_FLAG_LEN_ERR BIT(8)
#define RNPM_PRIV_FLAG_FW_10G_1G_AUTO_DETCH_EN BIT(9)
#define RNPM_PRIV_FLAG_TX_PADDING BIT(13)
#define RNPM_PRIV_FLAG_FORCE_SPEED_ABLIY BIT(14)
#define RNPM_PRIV_FLAG_LLDP_EN_STAT BIT(15)
	unsigned int num_tx_queues;
	unsigned int max_ring_pair_counts;
	unsigned int max_msix_counts;
	u16 tx_work_limit;
	__be16 vxlan_port;
	__be16 geneve_port;
	int num_rx_queues;
	u16 rx_itr_setting;
	u32 eth_queue_idx;
	u32 max_rate[MAX_TX_QUEUES];
	/* TX */
	struct rnpm_ring
		*tx_ring[MAX_TX_QUEUES] ____cacheline_aligned_in_smp;
	int tx_ring_item_count;
	u64 restart_queue;
	u64 lsc_int;
	u32 tx_timeout_count;

	/* RX */
	struct rnpm_ring *rx_ring[MAX_RX_QUEUES];
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
	struct rnpm_q_vector *q_vector[MAX_Q_VECTORS];
	/*used for IEEE 1588 ptp clock start*/
	const struct rnpm_hwtimestamp *hwts_ops;
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_clock_ops;
	struct sk_buff *ptp_tx_skb;
	struct hwtstamp_config tstamp_config;
	u32 ptp_config_value;
	spinlock_t ptp_lock; /* Used to protect the SYSTIME registers. */

	u64 clk_ptp_rate; /*uint is HZ 1MHz＝1 000 000Hz*/
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

	/* DCB parameters */
	struct rnpm_dcb_cfg dcb_cfg;
	u8 prio_tc_map[RNPM_MAX_USER_PRIO];
	u8 num_tc;

	int num_q_vectors; /* current number of q_vectors for device */
	int max_q_vectors; /* true count of q_vectors for device */
	struct rnpm_ring_feature ring_feature[RING_F_ARRAY_SIZE];
	struct msix_entry *msix_entries;

	u32 test_icr;
	struct rnpm_ring test_tx_ring;
	struct rnpm_ring test_rx_ring;

	/* structs defined in rnpm_hw.h */
	struct rnpm_hw hw;
	u16 msg_enable;
	struct rnpm_hw_stats hw_stats;

	u64 tx_busy;
	bool is_mac_loopback;
	u32 link_speed;
	bool link_up;
	unsigned long link_check_timeout;

	struct timer_list service_timer;
	struct work_struct service_task;

	/* fdir relative */
	struct hlist_head fdir_filter_list;
	unsigned long fdir_overflow; /* number of times ATR was backed off */
	union rnpm_atr_input fdir_mask;
	int fdir_mode;
	int fdir_filter_count;
	int layer2_count;
	int layer2_count_max;
	int layer2_offset;
	int tuple_5_count;
	int tuple_5_count_max;
	int tuple_5_offset;
	u32 fdir_pballoc;
	u32 atr_sample_rate;
	spinlock_t fdir_perfect_lock;

	u32 wol;
	u16 bd_number;
	u16 vector_off;
	u16 stags_vid;
	/* maintain */
	char *maintain_buf;
	int maintain_buf_len;
	void *maintain_dma_buf;
	dma_addr_t maintain_dma_phy;
	int maintain_dma_size;
	int maintain_in_bytes;

	/* SR-IOV */
	DECLARE_BITMAP(active_vfs, RNPM_MAX_VF_FUNCTIONS);
	unsigned int num_vfs;
	struct vf_data_storage *vfinfo;
	int vf_rate_link_speed;
	struct vf_macvlans vf_mvs;
	struct vf_macvlans *mv_list;
	u32 timer_event_accumulator;
	u32 vferr_refcount;
	struct kobject *info_kobj;
	struct hwmon_buff *rnpm_hwmon_buff;
#ifdef CONFIG_DEBUG_FS
	struct dentry *rnpm_dbg_adapter;
#endif /*CONFIG_DEBUG_FS*/
	u8 default_up;
	u8 port;
	u8 portid_of_card; /* port num in card*/
#define RNPM_MAX_RETA_ENTRIES 512
	u8 rss_indir_tbl[RNPM_MAX_RETA_ENTRIES];
	u32 rss_tbl_setup_flag;
	bool dma2_in_1pf;
	u8 uc_off;
	u8 uc_num;
	char name[60];
	bool mac_ipc_off;
	struct rnpm_d_ringinfo d_ringinfo;
};

struct rnpm_fdir_filter {
	struct hlist_node fdir_node;
	union rnpm_atr_input filter;
	u16 sw_idx;
	u16 hw_idx;
	u32 vf_num;
	u64 action;
};

enum rnpm_state_t {
	__RNPM_TESTING,
	__RNPM_RESETTING,
	__RNPM_DOWN,
	__RNPM_SERVICE_SCHED,
	__RNPM_IN_SFP_INIT,
	__RNPM_READ_I2C,
	__RNPM_PTP_TX_IN_PROGRESS,
	__RNPM_SERVICE_IN_PROGRESS,
	__RNPM_REMOVING,
};

struct rnpm_cb {
	union { /* Union defining head/tail partner */
		struct sk_buff *head;
		struct sk_buff *tail;
	};
	dma_addr_t dma;
	u16 append_cnt;
	bool page_released;
};
#define RNPM_CB(skb) ((struct rnpm_cb *)(skb)->cb)

enum rnpm_boards {
	board_n10,
	board_n400_4x1G,
};

extern char rnpm_driver_name[];
extern const char rnpm_driver_version[];
extern struct rnpm_info rnpm_n10_info;
extern struct rnpm_info rnpm_n400_4x1G_info;

extern void rnpm_up(struct rnpm_adapter *adapter);
extern void rnpm_down(struct rnpm_adapter *adapter);
extern void rnpm_reinit_locked(struct rnpm_adapter *adapter);
extern void rnpm_reset(struct rnpm_adapter *adapter);
extern void rnpm_set_ethtool_ops(struct net_device *netdev);
extern int rnpm_setup_rx_resources(struct rnpm_ring *ring,
				   struct rnpm_adapter *adapter);
extern int rnpm_setup_tx_resources(struct rnpm_ring *ring,
				   struct rnpm_adapter *adapter);
extern void rnpm_free_rx_resources(struct rnpm_ring *ring);
extern void rnpm_free_tx_resources(struct rnpm_ring *ring);
extern void rnpm_configure_rx_ring(struct rnpm_adapter *adapter,
				   struct rnpm_ring *ring);
extern void rnpm_configure_tx_ring(struct rnpm_adapter *adapter,
				   struct rnpm_ring *ring);
extern void rnpm_disable_rx_queue(struct rnpm_adapter *adapter,
				  struct rnpm_ring *ring);
extern void rnpm_update_stats(struct rnpm_adapter *adapter);
extern int rnpm_init_interrupt_scheme(struct rnpm_adapter *adapter);
extern int rnpm_wol_supported(struct rnpm_adapter *adapter, u16 device_id,
			      u16 subdevice_id);
extern void rnpm_clear_interrupt_scheme(struct rnpm_adapter *adapter);
extern netdev_tx_t rnpm_xmit_frame_ring(struct sk_buff *skb,
					struct rnpm_adapter *adapter,
					struct rnpm_ring *ring);
extern void rnpm_unmap_and_free_tx_resource(struct rnpm_ring *ring,
					    struct rnpm_tx_buffer *buffer);
extern int rnpm_poll(struct napi_struct *napi, int budget);
extern int ethtool_ioctl(struct ifreq *ifr);
extern s32 rnpm_reinit_fdir_tables_n10(struct rnpm_hw *hw);
extern void rnpm_set_rx_mode(struct net_device *netdev);
extern int rnpm_setup_tx_maxrate(void __iomem *ioaddr,
				 struct rnpm_ring *tx_ring, u64 max_rate,
				 int samples_1sec);
extern int rnpm_setup_tc(struct net_device *dev, u8 tc);
extern int rnpm_open(struct net_device *netdev);
extern int rnpm_close(struct net_device *netdev);
extern void rnpm_service_event_schedule(struct rnpm_adapter *adapter);
void rnpm_tx_ctxtdesc(struct rnpm_ring *tx_ring, u32 mss_len_vf_num,
		      u32 inner_vlan_tunnel_len, u32 type_tucmd);
void rnpm_maybe_tx_ctxtdesc(struct rnpm_ring *tx_ring,
			    struct rnpm_tx_buffer *first, u32 type_tucmd);
extern void rnpm_store_reta(struct rnpm_adapter *adapter);
extern void rnpm_store_key(struct rnpm_pf_adapter *pf_adapter);
extern int rnpm_init_rss_key(struct rnpm_pf_adapter *pf_adapter);
extern int rnpm_init_rss_table(struct rnpm_adapter *adapter);
extern void rnpm_setup_dma_rx(struct rnpm_adapter *adapter,
			      int count_in_dw);
extern s32 rnpm_fdir_write_perfect_filter(int fdir_mode,
					  struct rnpm_hw *hw,
					  union rnpm_atr_input *filter,
					  u16 hw_id, u8 queue);
extern s32 rnpm_fdir_erase_perfect_filter(int fdir_mode,
					  struct rnpm_hw *hw,
					  union rnpm_atr_input *input,
					  u16 hw_id);
extern u32 rnpm_rss_indir_tbl_entries(struct rnpm_adapter *adapter);
extern u32 rnpm_tx_desc_unused_sw(struct rnpm_ring *tx_ring);
extern u32 rnpm_tx_desc_unused_hw(struct rnpm_hw *hw,
				  struct rnpm_ring *tx_ring);
extern s32 rnpm_disable_rxr_maxrate(struct net_device *netdev,
				    u8 queue_index);
extern s32 rnpm_enable_rxr_maxrate(struct net_device *netdev,
				   u8 queue_index, u32 maxrate);
extern u32 rnpm_rx_desc_used_hw(struct rnpm_hw *hw,
				struct rnpm_ring *rx_ring);
extern void rnpm_do_reset(struct net_device *netdev);
extern void rnpm_sysfs_exit(struct rnpm_adapter *adapter);
extern int rnpm_sysfs_init(struct rnpm_adapter *adapter, int port);
#ifdef CONFIG_DEBUG_FS
extern void rnpm_dbg_adapter_init(struct rnpm_adapter *adapter);
extern void rnpm_dbg_adapter_exit(struct rnpm_adapter *adapter);
extern void rnpm_dbg_init(void);
extern void rnpm_dbg_exit(void);
#else
static inline void rnpm_dbg_adapter_init(struct rnpm_adapter *adapter)
{
}
static inline void rnpm_dbg_adapter_exit(struct rnpm_adapter *adapter)
{
}
static inline void rnpm_dbg_init(void)
{
}
static inline void rnpm_dbg_exit(void)
{
}
#endif /* CONFIG_DEBUG_FS */
static inline struct netdev_queue *txring_txq(const struct rnpm_ring *ring)
{
	return netdev_get_tx_queue(ring->netdev, ring->queue_index);
}

extern void rnpm_ptp_init(struct rnpm_adapter *adapter);
extern void rnpm_ptp_stop(struct rnpm_adapter *adapter);
extern void rnpm_ptp_overflow_check(struct rnpm_adapter *adapter);
extern void rnpm_ptp_rx_hang(struct rnpm_adapter *adapter);
extern void __rnpm_ptp_rx_hwtstamp(struct rnpm_q_vector *q_vector,
				   struct sk_buff *skb);
static inline void rnpm_ptp_rx_hwtstamp(struct rnpm_ring *rx_ring,
					union rnpm_rx_desc *rx_desc,
					struct sk_buff *skb)
{
	if (unlikely(!rnpm_test_staterr(rx_desc, RNPM_RXD_STAT_PTP)))
		return;
	rx_ring->last_rx_timestamp = jiffies;
}

static inline int ignore_veb_pkg_err(struct rnpm_adapter *adapter,
				     union rnpm_rx_desc *rx_desc)
{
#ifdef RNPM_IOV_VEB_BUG_NOT_FIXED
	if (unlikely((adapter->flags & RNPM_FLAG_SRIOV_ENABLED) &&
		     (cpu_to_le16(rx_desc->wb.mark) & VEB_VF_PKG)))
		return 1;
#endif
	return 0;
}

int rnpm_update_ethtool_fdir_entry(struct rnpm_adapter *adapter,
				   struct rnpm_fdir_filter *input,
				   u16 sw_idx);

static inline bool rnpm_is_pf1(struct pci_dev *pdev)
{
	struct rnpm_pf_adapter *pf_adapter = pci_get_drvdata(pdev);
	u16 vf_num = -1;
	u32 pfvfnum_reg;

#define PF_NUM_REG_N10 (0x75f000)
	pfvfnum_reg = (PF_NUM_REG_N10 & (pci_resource_len(pdev, 0) - 1));
	vf_num = readl(pf_adapter->hw_bar0 + pfvfnum_reg);
#define VF_NUM_MASK_TEMP (0x400)
#define VF_NUM_OFF (4)
	return !!((vf_num & VF_NUM_MASK_TEMP) >> VF_NUM_OFF);
}

extern void rnpm_service_task(struct work_struct *work);
extern void rnpm_sysfs_exit(struct rnpm_adapter *adapter);
extern int rnpm_sysfs_init(struct rnpm_adapter *adapter, int port);
void rnpm_sriov_reinit(struct rnpm_adapter *adapter);
#define SET_BIT(n, var) (var = (var | (1 << n)))
#define CLR_BIT(n, var) (var = (var & (~(1 << n))))
#define CHK_BIT(n, var) (var & (1 << n))

#define RNPM_RX_DMA_ATTR (DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)
static inline bool rnpm_removed(void __iomem *addr)
{
	return unlikely(!addr);
}
#define RNPM_REMOVED(a) rnpm_removed(a)
static inline bool rnpm_port_is_valid(struct rnpm_pf_adapter *pf_adapter,
				      int i)
{
	bool b = false;

	if (i >= MAX_PORT_NUM)
		return false;
	b = !!(pf_adapter->port_valid & (1 << i));
	return b;
}

int rnpm_set_clause73_autoneg_enable(struct net_device *netdev,
				     int enable);
int rnpm_card_partially_supported_10g_1g_sfp(
	struct rnpm_pf_adapter *pf_adapter);

#define RNPM_FW_VERSION_NEW_ETHTOOL 0x00050010
static inline bool rnpm_fw_is_old_ethtool(struct rnpm_hw *hw)
{
	return hw->fw_version >= RNPM_FW_VERSION_NEW_ETHTOOL ? false :
							       true;
}
void rnpm_control_mac_rx(struct rnpm_adapter *adapter, bool on);

static inline int Hamming_weight_1(u32 n)
{
	int count_ = 0;

	while (n != 0) {
		n &= (n - 1);
		count_++;
	}
	return count_;
}

#define RNPM_WOL_GET_SUPPORTED(adapter) \
	(!!(adapter->wol & (BIT(0) << adapter->port)))
#define RNPM_WOL_GET_STATUS(adapter) \
	(!!(adapter->wol & (BIT(4) << adapter->port)))
#define RNPM_WOL_SET_SUPPORTED(adapter) \
	(adapter->wol |= BIT(0) << adapter->port)
#define RNPM_WOL_SET_STATUS(adapter) \
	(adapter->wol |= BIT(4) << adapter->port)
#define RNPM_WOL_CLEAR_STATUS(adapter) \
	(adapter->wol &= ~(BIT(4) << adapter->port))

int rnpm_esp_tcam_enable(struct rnpm_pf_adapter *pf_adapter);
void rnpm_set_esp_to_mpe_tcam_rule(struct rnpm_pf_adapter *pf_adapter);

#endif /* _RNPM_H_ */
