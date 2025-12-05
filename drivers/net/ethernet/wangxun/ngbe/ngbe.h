/* SPDX-License-Identifier: GPL-2.0 */
/* WangXun Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#ifndef _NGBE_H_
#define _NGBE_H_

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/ethtool.h>
#ifdef NETIF_F_HW_VLAN_CTAG_TX
#include <linux/if_vlan.h>
#endif

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/sched.h>

#include "ngbe_type.h"
#include <net/xdp.h>

#include <linux/sctp.h>
#include <linux/aer.h>
#include <linux/pci_hotplug.h>

#include <linux/mdio.h>

#include <linux/timecounter.h>
#include <linux/clocksource.h>
#include <linux/net_tstamp.h>
#include <linux/ptp_clock_kernel.h>

/* Ether Types */
#define NGBE_ETH_P_LLDP 0x88CC
#define NGBE_ETH_P_CNM 0x22E7

/* TX/RX descriptor defines */
#define NGBE_DEFAULT_TXD 1024 /* default ring size */
#define NGBE_DEFAULT_TX_WORK 256
#define NGBE_MAX_TXD 8192
#define NGBE_MIN_TXD 128
#define NGBE_MAX_TX_WORK 65535

#define NGBE_DEFAULT_RXD 512 /* default ring size */
#define NGBE_DEFAULT_RX_WORK 256
#define NGBE_MAX_RXD 8192
#define NGBE_MIN_RXD 128

#define NGBE_ETH_P_LLDP 0x88CC

/* flow control */
#define NGBE_MIN_FCRTL 0x40
#define NGBE_MAX_FCRTL 0x7FF80
#define NGBE_MIN_FCRTH 0x600
#define NGBE_MAX_FCRTH 0x7FFF0
#define NGBE_DEFAULT_FCPAUSE 0xFFFF
#define NGBE_MIN_FCPAUSE 0
#define NGBE_MAX_FCPAUSE 0xFFFF

/* Supported Rx Buffer Sizes */
#define NGBE_RXBUFFER_256 256 /* Used for skb receive header */
#define NGBE_RXBUFFER_2K 2048
#define NGBE_RXBUFFER_3K 3072
#define NGBE_RXBUFFER_4K 4096
#define NGBE_RXBUFFER_1536 1536
#define NGBE_MAX_RXBUFFER 16384 /* largest size for single descriptor */

#define NGBE_STATIC_ITR 6756 /* static itr configure */
#define NGBE_LINK_RETRY 1 /* static itr configure */
#define NGBE_POLL_LINK_STATUS 0

#define NGBE_PCIE_RECOVER 1

#define NGBE_RECOVER_CHECK 1

#define NGBE_DIS_COMP_TIMEOUT 1 /* static itr configure */

#define NGBE_1588_PPS_LEVEL 1

#define NGBE_1588_PPS_WIDTH 120

#define NGBE_1588_TOD_ENABLE 1
/* NOTE: netdev_alloc_skb reserves up to 64 bytes, NET_IP_ALIGN means we
 * reserve 64 more, and skb_shared_info adds an additional 320 bytes more,
 * this adds up to 448 bytes of extra data.
 * Since netdev_alloc_skb now allocates a page fragment we can use a value
 * of 256 and the resultant skb will have a truesize of 960 or less.
 */
#define NGBE_RX_HDR_SIZE NGBE_RXBUFFER_256

#define MAXIMUM_ETHERNET_VLAN_SIZE (VLAN_ETH_FRAME_LEN + ETH_FCS_LEN)

/* How many Rx Buffers do we bundle into one write to the hardware ? */
#define NGBE_RX_BUFFER_WRITE 16 /* Must be power of 2 */

#define NGBE_RX_DMA_ATTR (DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)

enum ngbe_tx_flags {
	/* cmd_type flags */
	NGBE_TX_FLAGS_HW_VLAN = 0x01,
	NGBE_TX_FLAGS_TSO = 0x02,
	NGBE_TX_FLAGS_TSTAMP = 0x04,

	/* olinfo flags */
	NGBE_TX_FLAGS_CC = 0x08,
	NGBE_TX_FLAGS_IPV4 = 0x10,
	NGBE_TX_FLAGS_CSUM = 0x20,
	NGBE_TX_FLAGS_OUTER_IPV4 = 0x100,
	NGBE_TX_FLAGS_LINKSEC = 0x200,
	NGBE_TX_FLAGS_IPSEC = 0x400,

	/* software defined flags */
	NGBE_TX_FLAGS_SW_VLAN = 0x40,
	NGBE_TX_FLAGS_FCOE = 0x80,
};

/* VLAN info */
#define NGBE_TX_FLAGS_VLAN_MASK 0xffff0000
#define NGBE_TX_FLAGS_VLAN_PRIO_MASK 0xe0000000
#define NGBE_TX_FLAGS_VLAN_PRIO_SHIFT 29
#define NGBE_TX_FLAGS_VLAN_SHIFT 16

#define NGBE_MAX_RX_DESC_POLL 10

#define NGBE_MAX_VF_MC_ENTRIES 30
#define NGBE_MAX_VF_FUNCTIONS 8
#define MAX_EMULATION_MAC_ADDRS 16
#define NGBE_MAX_PF_MACVLANS 15
#define NGBE_VF_DEVICE_ID 0x1000

/* must account for pools assigned to VFs. */
#ifdef CONFIG_PCI_IOV
#define VMDQ_P(p) ((p) + adapter->ring_feature[RING_F_VMDQ].offset)
#else
#define VMDQ_P(p) (p)
#endif

#define UPDATE_VF_COUNTER_32bit(reg, last_counter, counter) \
	{                                                   \
		u32 current_counter = rd32(hw, reg);        \
		if (current_counter < (last_counter))         \
			(counter) += 0x100000000LL;           \
		(last_counter) = current_counter;             \
		(counter) &= 0xFFFFFFFF00000000LL;            \
		(counter) |= current_counter;                 \
	}

#define UPDATE_VF_COUNTER_36bit(reg_lsb, reg_msb, last_counter, counter) \
	{                                                                \
		u64 current_counter_lsb = rd32(hw, (reg_lsb));             \
		u64 current_counter_msb = rd32(hw, (reg_msb));             \
		u64 current_counter = (current_counter_msb << 32) |      \
				      current_counter_lsb;               \
		if (current_counter < (last_counter))                      \
			(counter) += 0x1000000000LL;                       \
		(last_counter) = current_counter;                          \
		(counter) &= 0xFFFFFFF000000000LL;                         \
		(counter) |= current_counter;                              \
	}
DECLARE_STATIC_KEY_FALSE(ngbe_xdp_locking_key);

struct vf_stats {
	u64 gprc;
	u64 gorc;
	u64 gptc;
	u64 gotc;
	u64 mprc;
};

struct vf_data_storage {
	struct pci_dev *vfdev;
	u8 __iomem *b4_addr;
	u32 b4_buf[16];
	unsigned char vf_mac_addresses[ETH_ALEN];
	u16 vf_mc_hashes[NGBE_MAX_VF_MC_ENTRIES];
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
};

struct vf_macvlans {
	struct list_head l;
	int vf;
	bool free;
	bool is_macvlan;
	u8 vf_macvlan[ETH_ALEN];
};

#define NGBE_MAX_TXD_PWR 14
#define NGBE_MAX_DATA_PER_TXD BIT(NGBE_MAX_TXD_PWR)

/* Tx Descriptors needed, worst case */
#define TXD_USE_COUNT(S) DIV_ROUND_UP((S), NGBE_MAX_DATA_PER_TXD)
#define DESC_NEEDED (MAX_SKB_FRAGS + 4)

#define DESC_RESERVED 96

/* wrapper around a pointer to a socket buffer,
 * so a DMA handle can be stored along with the buffer
 */
struct ngbe_tx_buffer {
	union ngbe_tx_desc *next_to_watch;
	unsigned long time_stamp;
	u64 done_time;
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

struct ngbe_rx_buffer {
	struct sk_buff *skb;
	dma_addr_t dma;
	dma_addr_t page_dma;
	union {
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

struct ngbe_queue_stats {
	u64 packets;
	u64 bytes;
};

struct ngbe_tx_queue_stats {
	u64 restart_queue;
	u64 tx_busy;
	u64 tx_done_old;
};

struct ngbe_rx_queue_stats {
	u64 non_eop_descs;
	u64 alloc_rx_page_failed;
	u64 alloc_rx_buff_failed;
	u64 csum_good_cnt;
	u64 csum_err;
};

#define NGBE_TS_HDR_LEN 8
enum ngbe_ring_state_t {
	__NGBE_RX_3K_BUFFER,
	__NGBE_RX_BUILD_SKB_ENABLED,
	__NGBE_TX_XPS_INIT_DONE,
	__NGBE_TX_DETECT_HANG,
	__NGBE_HANG_CHECK_ARMED,
	__NGBE_RX_HS_ENABLED,
	__NGBE_TX_XDP_RING,
	__NGBE_TX_DISABLED,

};

struct ngbe_fwd_adapter {
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
	struct net_device *vdev;
	struct ngbe_adapter *adapter;
	unsigned int tx_base_queue;
	unsigned int rx_base_queue;
	int index; /* pool index on PF */
};

#define ring_uses_build_skb(ring) \
	test_bit(__NGBE_RX_BUILD_SKB_ENABLED, &(ring)->state)

#define ring_is_hs_enabled(ring) test_bit(__NGBE_RX_HS_ENABLED, &(ring)->state)
#define set_ring_hs_enabled(ring) set_bit(__NGBE_RX_HS_ENABLED, &(ring)->state)
#define clear_ring_hs_enabled(ring) \
	clear_bit(__NGBE_RX_HS_ENABLED, &(ring)->state)
#define check_for_tx_hang(ring) test_bit(__NGBE_TX_DETECT_HANG, &(ring)->state)
#define set_check_for_tx_hang(ring) \
	set_bit(__NGBE_TX_DETECT_HANG, &(ring)->state)
#define clear_check_for_tx_hang(ring) \
	clear_bit(__NGBE_TX_DETECT_HANG, &(ring)->state)
#define ring_is_xdp(ring) test_bit(__NGBE_TX_XDP_RING, &(ring)->state)
#define set_ring_xdp(ring) set_bit(__NGBE_TX_XDP_RING, &(ring)->state)
#define clear_ring_xdp(ring) clear_bit(__NGBE_TX_XDP_RING, &(ring)->state)

struct ngbe_ring {
	struct ngbe_ring *next; /* pointer to next ring in q_vector */
	struct ngbe_q_vector *q_vector; /* backpointer to host q_vector */
	struct net_device *netdev; /* netdev ring belongs to */
	struct bpf_prog *xdp_prog;
	struct device *dev; /* device for DMA mapping */
	struct ngbe_fwd_adapter *accel;
	void *desc; /* descriptor ring memory */
	union {
		struct ngbe_tx_buffer *tx_buffer_info;
		struct ngbe_rx_buffer *rx_buffer_info;
	};
	spinlock_t tx_lock; /* used in XDP mode */
	unsigned long state;
	u8 __iomem *tail;
	dma_addr_t dma; /* phys. address of descriptor ring */
	unsigned int size; /* length in bytes */

	u16 count; /* amount of descriptors */

	u8 queue_index; /* needed for multiqueue queue management */
	u8 reg_idx;
	/* holds the special value that gets
	 * the hardware register offset
	 * associated with this ring, which is
	 * different for DCB and RSS modes
	 */
	u16 next_to_use;
	u16 next_to_clean;
	u16 next_to_free;

	unsigned long last_rx_timestamp;

	u16 rx_buf_len;
	union {
		u16 next_to_alloc;
		struct {
			u8 atr_sample_rate;
			u8 atr_count;
		};
	};

	u16 xdp_tx_active;

	u8 dcb_tc;
	struct ngbe_queue_stats stats;
	struct u64_stats_sync syncp;
	union {
		struct ngbe_tx_queue_stats tx_stats;
		struct ngbe_rx_queue_stats rx_stats;
	};
	struct xdp_rxq_info xdp_rxq;
	struct xsk_buff_pool *xsk_pool;
} ____cacheline_internodealigned_in_smp;

enum ngbe_ring_f_enum {
	RING_F_NONE = 0,
	RING_F_VMDQ, /* SR-IOV uses the same ring feature */
	RING_F_RSS,
	RING_F_ARRAY_SIZE /* must be last in enum set */
};

#define TGB_MAX_RX_QUEUES 16
#define NGBE_MAX_TX_QUEUES 16

#define NGBE_MAX_RSS_INDICES 8
#define NGBE_MAX_VMDQ_INDICES 8
#define NGBE_MAX_FDIR_INDICES 8
#define MAX_RX_QUEUES 8
#define MAX_TX_QUEUES 8
#define MAX_XDP_QUEUES 4
#define NGBE_MAX_L2A_QUEUES 4
#define NGBE_BAD_L2A_QUEUE 3

#define NGBE_MAX_MACVLANS 8

struct ngbe_ring_feature {
	u16 limit; /* upper limit on feature indices */
	u16 indices; /* current value of indices */
	u16 mask; /* Mask used for feature to ring mapping */
	u16 offset; /* offset to start of feature */
};

#if (PAGE_SIZE < 8192)
#define NGBE_2K_TOO_SMALL_WITH_PADDING        \
	((NET_SKB_PAD + NGBE_RXBUFFER_1536) > \
	 SKB_WITH_OVERHEAD(NGBE_RXBUFFER_2K))

static inline int ngbe_compute_pad(int rx_buf_len)
{
	int page_size, pad_size;

	page_size = ALIGN(rx_buf_len, PAGE_SIZE / 2);
	pad_size = SKB_WITH_OVERHEAD(page_size) - rx_buf_len;

	return pad_size;
}

static inline int ngbe_skb_pad(void)
{
	int rx_buf_len;

	/* If a 2K buffer cannot handle a standard Ethernet frame then
	 * optimize padding for a 3K buffer instead of a 1.5K buffer.
	 *
	 * For a 3K buffer we need to add enough padding to allow for
	 * tailroom due to NET_IP_ALIGN possibly shifting us out of
	 * cache-line alignment.
	 */
	if (NGBE_2K_TOO_SMALL_WITH_PADDING)
		rx_buf_len = NGBE_RXBUFFER_3K + SKB_DATA_ALIGN(NET_IP_ALIGN);
	else
		rx_buf_len = NGBE_RXBUFFER_1536;

	/* if needed make room for NET_IP_ALIGN */
	rx_buf_len -= NET_IP_ALIGN;

	return ngbe_compute_pad(rx_buf_len);
}

#define NGBE_SKB_PAD ngbe_skb_pad()
#else
#define NGBE_SKB_PAD (NET_SKB_PAD + NET_IP_ALIGN)
#endif

/* FCoE requires that all Rx buffers be over 2200 bytes in length.  Since
 * this is twice the size of a half page we need to double the page order
 * for FCoE enabled Rx queues.
 */
static inline unsigned int ngbe_rx_bufsz(struct ngbe_ring __maybe_unused *ring)
{
	if (test_bit(__NGBE_RX_3K_BUFFER, &ring->state))
		return NGBE_RXBUFFER_3K;
	return NGBE_RXBUFFER_2K;
}

static inline unsigned int
ngbe_rx_pg_order(struct ngbe_ring __maybe_unused *ring)
{
#if (PAGE_SIZE < 8192)
	if (test_bit(__NGBE_RX_3K_BUFFER, &ring->state))
		return 1;
#endif
	return 0;
}

#define ngbe_rx_pg_size(_ring) (PAGE_SIZE << ngbe_rx_pg_order(_ring))

static inline unsigned int ngbe_rx_offset(struct ngbe_ring *rx_ring)
{
	if (rx_ring->xdp_prog)
#if (PAGE_SIZE < 8192)
		return 192;
#else
		return XDP_PACKET_HEADROOM;
#endif
	else
		return 0;
}

struct ngbe_ring_container {
	struct ngbe_ring *ring; /* pointer to linked list of rings */
	unsigned int total_bytes; /* total bytes processed this int */
	unsigned int total_packets; /* total packets processed this int */
	u16 work_limit; /* total work allowed per interrupt */
	u8 count; /* total number of rings in vector */
	u8 itr; /* current ITR setting for ring */
};

/* iterator for handling rings in ring container */
#define ngbe_for_each_ring(pos, head) \
	for (pos = (head).ring; pos; pos = pos->next)

#define MAX_RX_PACKET_BUFFERS ((adapter->flags & NGBE_FLAG_DCB_ENABLED) ? 8 : 1)
#define MAX_TX_PACKET_BUFFERS MAX_RX_PACKET_BUFFERS

/* MAX_MSIX_Q_VECTORS of these are allocated,
 * but we only use one per queue-specific vector.
 */
struct ngbe_q_vector {
	struct ngbe_adapter *adapter;
	int cpu; /* CPU for DCA */
	u16 v_idx;
	/* index of q_vector within array, also used for
	 * finding the bit in EICR and friends that
	 * represents the vector for this ring
	 */
	u16 itr; /* Interrupt throttle rate written to EITR */
	struct ngbe_ring_container rx, tx;

	struct napi_struct napi;
	cpumask_t affinity_mask;
	int numa_node;
	struct rcu_head rcu; /* to avoid race with update stats on free */
	char name[IFNAMSIZ + 17];
	bool netpoll_rx;

	/* for dynamic allocation of rings associated with this q_vector */
	struct ngbe_ring ring[0] ____cacheline_internodealigned_in_smp;
};

#define NGBE_HWMON_TYPE_TEMP 0
#define NGBE_HWMON_TYPE_ALARMTHRESH 1
#define NGBE_HWMON_TYPE_DALARMTHRESH 2

struct hwmon_attr {
	struct device_attribute dev_attr;
	struct ngbe_hw *hw;
	struct ngbe_thermal_diode_data *sensor;
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
#define NGBE_70K_ITR 57
#define NGBE_20K_ITR 200
#define NGBE_4K_ITR 1024
#define NGBE_7K_ITR 595

/* ngbe_test_staterr - tests bits in Rx descriptor status and error fields */
static inline __le32 ngbe_test_staterr(union ngbe_rx_desc *rx_desc,
				       const u32 stat_err_bits)
{
	return rx_desc->wb.upper.status_error & cpu_to_le32(stat_err_bits);
}

/* ngbe_desc_unused - calculate if we have unused descriptors */
static inline u16 ngbe_desc_unused(struct ngbe_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->count) + ntc - ntu - 1;
}

#define NGBE_RX_DESC(R, i) (&(((union ngbe_rx_desc *)((R)->desc))[i]))
#define NGBE_TX_DESC(R, i) (&(((union ngbe_tx_desc *)((R)->desc))[i]))
#define NGBE_TX_CTXTDESC(R, i) \
	(&(((struct ngbe_tx_context_desc *)((R)->desc))[i]))

#define NGBE_MAX_JUMBO_FRAME_SIZE 9432 /* max payload 9414 */
#define TCP_TIMER_VECTOR 0
#define OTHER_VECTOR 1
#define NON_Q_VECTORS (OTHER_VECTOR + TCP_TIMER_VECTOR)

#define NGBE_MAX_MSIX_Q_VECTORS_EMERALD 9

struct ngbe_mac_addr {
	u8 addr[ETH_ALEN];
	u16 state; /* bitmask */
	u64 pools;
};

#define NGBE_MAC_STATE_DEFAULT 0x1
#define NGBE_MAC_STATE_MODIFIED 0x2
#define NGBE_MAC_STATE_IN_USE 0x4

/* Only for array allocations in our adapter struct.
 * we can actually assign 64 queue vectors based on our extended-extended
 * interrupt registers.
 */
#define MAX_MSIX_Q_VECTORS NGBE_MAX_MSIX_Q_VECTORS_EMERALD
#define MAX_MSIX_COUNT NGBE_MAX_MSIX_VECTORS_EMERALD

#define MIN_MSIX_Q_VECTORS 1
#define MIN_MSIX_COUNT (MIN_MSIX_Q_VECTORS + NON_Q_VECTORS)

/* default to trying for four seconds */
#define NGBE_TRY_LINK_TIMEOUT (4 * HZ)
#define NGBE_SFP_POLL_JIFFIES (2 * HZ) /* SFP poll every 2 seconds */

/**
 * ngbe_adapter.flag
 **/
#define NGBE_FLAG_MSI_CAPABLE ((u32)BIT(0))
#define NGBE_FLAG_MSI_ENABLED ((u32)BIT(1))
#define NGBE_FLAG_MSIX_CAPABLE ((u32)BIT(2))
#define NGBE_FLAG_MSIX_ENABLED ((u32)BIT(3))
#define NGBE_FLAG_LLI_PUSH ((u32)BIT(4))

#define NGBE_FLAG_TPH_ENABLED ((u32)BIT(6))
#define NGBE_FLAG_TPH_CAPABLE ((u32)BIT(7))
#define NGBE_FLAG_TPH_ENABLED_DATA ((u32)BIT(8))

#define NGBE_FLAG_MQ_CAPABLE ((u32)BIT(9))
#define NGBE_FLAG_DCB_ENABLED ((u32)BIT(10))
#define NGBE_FLAG_VMDQ_ENABLED ((u32)BIT(11))
#define NGBE_FLAG_FAN_FAIL_CAPABLE ((u32)BIT(12))
#define NGBE_FLAG_NEED_LINK_UPDATE ((u32)BIT(13))
#define NGBE_FLAG_NEED_ANC_CHECK ((u32)BIT(14))
#define NGBE_FLAG_FDIR_HASH_CAPABLE ((u32)BIT(15))
#define NGBE_FLAG_FDIR_PERFECT_CAPABLE ((u32)BIT(16))
#define NGBE_FLAG_SRIOV_CAPABLE ((u32)BIT(19))
#define NGBE_FLAG_SRIOV_ENABLED ((u32)BIT(20))
#define NGBE_FLAG_SRIOV_REPLICATION_ENABLE ((u32)BIT(21))
#define NGBE_FLAG_SRIOV_L2SWITCH_ENABLE ((u32)BIT(22))
#define NGBE_FLAG_SRIOV_VEPA_BRIDGE_MODE ((u32)BIT(23))
#define NGBE_FLAG_RX_HWTSTAMP_ENABLED ((u32)BIT(24))
#define NGBE_FLAG_VXLAN_OFFLOAD_CAPABLE ((u32)BIT(25))
#define NGBE_FLAG_VXLAN_OFFLOAD_ENABLE ((u32)BIT(26))
#define NGBE_FLAG_RX_HWTSTAMP_IN_REGISTER ((u32)BIT(27))
#define NGBE_FLAG_NEED_ETH_PHY_RESET ((u32)BIT(28))
#define NGBE_FLAG_RX_HS_ENABLED ((u32)BIT(30))
#define NGBE_FLAG_LINKSEC_ENABLED ((u32)BIT(31))
#define NGBE_FLAG_IPSEC_ENABLED ((u32)BIT(5))

/* preset defaults */
#define NGBE_FLAGS_SP_INIT                                \
	(NGBE_FLAG_MSI_CAPABLE | NGBE_FLAG_MSIX_CAPABLE | \
	 NGBE_FLAG_MQ_CAPABLE | NGBE_FLAG_SRIOV_CAPABLE)

/**
 * ngbe_adapter.flag2
 **/
#define NGBE_FLAG2_RSC_CAPABLE BIT(0)
#define NGBE_FLAG2_RSC_ENABLED BIT(1)
#define NGBE_FLAG2_TEMP_SENSOR_CAPABLE BIT(3)
#define NGBE_FLAG2_TEMP_SENSOR_EVENT BIT(4)
#define NGBE_FLAG2_SEARCH_FOR_SFP BIT(5)
#define NGBE_FLAG2_SFP_NEEDS_RESET BIT(6)
#define NGBE_FLAG2_PF_RESET_REQUESTED BIT(7)
#define NGBE_FLAG2_FDIR_REQUIRES_REINIT BIT(8)
#define NGBE_FLAG2_RSS_FIELD_IPV4_UDP BIT(9)
#define NGBE_FLAG2_RSS_FIELD_IPV6_UDP BIT(10)
#define NGBE_FLAG2_RSS_ENABLED BIT(12)
#define NGBE_FLAG2_PTP_PPS_ENABLED BIT(11)
#define NGBE_FLAG2_EEE_CAPABLE BIT(14)
#define NGBE_FLAG2_EEE_ENABLED BIT(15)
#define NGBE_FLAG2_VXLAN_REREG_NEEDED BIT(16)
#define NGBE_FLAG2_VLAN_PROMISC BIT(17)
#define NGBE_FLAG2_DEV_RESET_REQUESTED BIT(18)
#define NGBE_FLAG2_RESET_INTR_RECEIVED BIT(19)
#define NGBE_FLAG2_GLOBAL_RESET_REQUESTED BIT(20)
#define NGBE_FLAG2_MNG_REG_ACCESS_DISABLED BIT(22)
#define NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP BIT(23)
#define NGBE_FLAG2_ECC_ERR_RESET BIT(24)
#define NGBE_FLAG2_NEED_Q_RESET BIT(30)
#define NGBE_FLAG2_PCIE_NEED_RECOVER BIT(31)

#define NGBE_SET_FLAG(_input, _flag, _result)                               \
	(((_flag) <= (_result)) ? ((u32)((_input) & (_flag)) * ((_result) / (_flag))) : \
			      ((u32)((_input) & (_flag)) / ((_flag) / (_result))))

enum ngbe_isb_idx {
	NGBE_ISB_HEADER,
	NGBE_ISB_MISC,
	NGBE_ISB_VEC0,
	NGBE_ISB_VEC1,
	NGBE_ISB_MAX
};

/* board specific private data structure */
struct ngbe_adapter {
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
	/* OS defined structs */
	struct net_device *netdev;
	struct bpf_prog *xdp_prog;
	struct pci_dev *pdev;

	unsigned long state;

	/* Some features need tri-state capability,
	 * thus the additional *_CAPABLE flags.
	 */
	u32 flags;
	u32 flags2;
	u32 led_conf;
	u32 gphy_efuse[2];

	/* Tx fast path data */
	int num_tx_queues;
	u16 tx_itr_setting;
	u16 tx_work_limit;

	/* Rx fast path data */
	int num_rx_queues;
	u16 rx_itr_setting;
	u16 rx_work_limit;
	/* XDP */
	int num_xdp_queues;
	struct ngbe_ring *xdp_ring[MAX_XDP_QUEUES];
	unsigned long *af_xdp_zc_qps;

	unsigned int num_vmdqs; /* does not include pools assigned to VFs */
	unsigned int queues_per_pool;

	/* TX */
	struct ngbe_ring *tx_ring[MAX_TX_QUEUES] ____cacheline_aligned_in_smp;

	u64 restart_queue;
	u64 lsc_int;
	u32 tx_timeout_count;

	/* RX */
	struct ngbe_ring *rx_ring[MAX_RX_QUEUES];
	u64 hw_csum_rx_error;
	u64 hw_csum_rx_good;
	u64 hw_rx_no_dma_resources;
	u64 non_eop_descs;
	u32 alloc_rx_page_failed;
	u32 alloc_rx_buff_failed;

	struct ngbe_q_vector *q_vector[MAX_MSIX_Q_VECTORS];

	struct ieee_pfc *ngbe_ieee_pfc;
	struct ieee_ets *ngbe_ieee_ets;
	enum ngbe_fc_mode last_lfc_mode;
	int num_q_vectors; /* current number of q_vectors for device */
	int max_q_vectors; /* upper limit of q_vectors for device */
	struct ngbe_ring_feature ring_feature[RING_F_ARRAY_SIZE];
	u16 irq_remap_offset;
	struct msix_entry *msix_entries;
	u16 old_rss_limit;

	u64 test_icr;
	struct ngbe_ring test_tx_ring;
	struct ngbe_ring test_rx_ring;

	/* structs defined in ngbe_hw.h */
	struct ngbe_hw hw;
	u16 msg_enable;
	struct ngbe_hw_stats stats;
	u32 lli_port;
	u32 lli_size;
	u32 lli_etype;
	u32 lli_vlan_pri;

	struct ngbe_queue_stats old_rx_qstats[MAX_RX_QUEUES];
	struct ngbe_queue_stats old_tx_qstats[MAX_TX_QUEUES];
	struct ngbe_tx_queue_stats old_tx_stats[MAX_TX_QUEUES];
	struct ngbe_rx_queue_stats old_rx_stats[MAX_RX_QUEUES];

	u32 *config_space;
	u64 tx_busy;
	unsigned int tx_ring_count;
	unsigned int xdp_ring_count;
	unsigned int rx_ring_count;

	u32 link_speed;
	u32 speed;
	bool pf_running;
	bool link_up;
	bool need_delay;
	unsigned long sfp_poll_time;
	unsigned long link_check_timeout;

	struct timer_list service_timer;
	struct work_struct service_task;
	struct timer_list link_check_timer;

	u32 atr_sample_rate;
	u8 __iomem *io_addr; /* Mainly for iounmap use */
	u32 wol;

	spinlock_t mbx_lock; /*used in pf vf mbx*/
	u16 bd_number;

	u16 bridge_mode;

	char eeprom_id[32];
	u16 eeprom_cap;
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
	spinlock_t tmreg_lock; /*used to ptp tx*/
	struct cyclecounter hw_cc;
	struct timecounter hw_tc;
	u32 base_incval;
	u32 tx_hwtstamp_timeouts;
	u32 tx_hwtstamp_skipped;
	u32 rx_hwtstamp_cleared;
	u64 pps_edge_start;
	u64 pps_edge_end;
	u64 sec_to_cc;
	u8 pps_enabled;
	void (*ptp_setup_sdp)(struct ngbe_adapter *adapter);

	DECLARE_BITMAP(active_vfs, NGBE_MAX_VF_FUNCTIONS);
	unsigned int num_vfs;
	unsigned int max_vfs;
	struct vf_data_storage *vfinfo;
	struct vf_macvlans vf_mvs;
	struct vf_macvlans *mv_list;
#ifdef CONFIG_PCI_IOV
	u32 timer_event_accumulator;
	u32 vferr_refcount;
#endif
	struct ngbe_mac_addr *mac_table;
	__le16 vxlan_port;
	__le16 geneve_port;
	struct hwmon_buff ngbe_hwmon_buff;

	struct dentry *ngbe_dbg_adapter;
	u8 default_up;
	unsigned long fwd_bitmask; /* bitmask indicating in use pools */
	unsigned long tx_timeout_last_recovery;
	u32 tx_timeout_recovery_level;

#define NGBE_MAX_RETA_ENTRIES 128
	u8 rss_indir_tbl[NGBE_MAX_RETA_ENTRIES];
#define NGBE_RSS_KEY_SIZE 40
	u32 rss_key[NGBE_RSS_KEY_SIZE / sizeof(u32)];

	void *ipsec;

	/* misc interrupt status block */
	dma_addr_t isb_dma;
	u32 *isb_mem;
	u32 isb_tag[NGBE_ISB_MAX];

	u32 hang_cnt;
	u64 eth_priv_flags;
#define NGBE_ETH_PRIV_FLAG_LLDP BIT(0)
	/* AF_XDP zero-copy */
	struct xsk_buff_pool **xsk_pools;
	u16 num_xsk_pools_used;
	u16 num_xsk_pools;
	bool cmplt_to_dis;
};

static inline u32 ngbe_misc_isb(struct ngbe_adapter *adapter,
				enum ngbe_isb_idx idx)
{
	u32 cur_tag = 0;

	cur_tag = adapter->isb_mem[NGBE_ISB_HEADER];
	adapter->isb_tag[idx] = cur_tag;

	return cpu_to_le32(adapter->isb_mem[idx]);
}

static inline u8 ngbe_max_rss_indices(struct ngbe_adapter *adapter)
{
	if (adapter->xdp_prog)
		return NGBE_MAX_RSS_INDICES / 2;
	return NGBE_MAX_RSS_INDICES;
}

enum ngbe_state_t {
	__NGBE_TESTING,
	__NGBE_RESETTING,
	__NGBE_DOWN,
	__NGBE_HANGING,
	__NGBE_DISABLED,
	__NGBE_REMOVING,
	__NGBE_SERVICE_SCHED,
	__NGBE_SERVICE_INITED,
	__NGBE_IN_SFP_INIT,
	__NGBE_NO_PHY_SET,
	__NGBE_PTP_RUNNING,
	__NGBE_PTP_TX_IN_PROGRESS,
};

struct ngbe_cb {
	dma_addr_t dma;
	u16 append_cnt; /* number of skb's appended */
	bool page_released;
	bool dma_released;
};

#define NGBE_CB(skb) ((struct ngbe_cb *)(skb)->cb)

/* ESX ngbe CIM IOCTL definition */

void ngbe_sysfs_exit(struct ngbe_adapter *adapter);
int ngbe_sysfs_init(struct ngbe_adapter *adapter);
/* needed by ngbe_main.c */
void ngbe_check_options(struct ngbe_adapter *adapter);
void ngbe_assign_netdev_ops(struct net_device *netdev);

/* needed by ngbe_ethtool.c */
extern char ngbe_driver_name[];
extern const char ngbe_driver_version[];

void ngbe_irq_disable(struct ngbe_adapter *adapter);
void ngbe_irq_enable(struct ngbe_adapter *adapter, bool queues, bool flush);
int ngbe_open(struct net_device *netdev);
int ngbe_close(struct net_device *netdev);
void ngbe_up(struct ngbe_adapter *adapter);
void ngbe_down(struct ngbe_adapter *adapter);
void ngbe_reinit_locked(struct ngbe_adapter *adapter);
void ngbe_reset(struct ngbe_adapter *adapter);
void ngbe_set_ethtool_ops(struct net_device *netdev);
int ngbe_setup_rx_resources(struct ngbe_ring *rx_ring);
int ngbe_setup_tx_resources(struct ngbe_ring *tx_ring);
void ngbe_free_rx_resources(struct ngbe_ring *rx_ring);
void ngbe_free_tx_resources(struct ngbe_ring *tx_ring);
void ngbe_configure_rx_ring(struct ngbe_adapter *adapter,
			    struct ngbe_ring *ring);
void ngbe_configure_tx_ring(struct ngbe_adapter *adapter,
			    struct ngbe_ring *ring);
void ngbe_update_stats(struct ngbe_adapter *adapter);
int ngbe_init_interrupt_scheme(struct ngbe_adapter *adapter);
void ngbe_reset_interrupt_capability(struct ngbe_adapter *adapter);
void ngbe_set_interrupt_capability(struct ngbe_adapter *adapter);
void ngbe_clear_interrupt_scheme(struct ngbe_adapter *adapter);
netdev_tx_t ngbe_xmit_frame_ring(struct sk_buff *skb,
				 struct ngbe_adapter __maybe_unused *adapter,
				 struct ngbe_ring *tx_ring);
void ngbe_unmap_and_free_tx_resource(struct ngbe_ring *ring,
				     struct ngbe_tx_buffer *tx_buffer);
void ngbe_alloc_rx_buffers(struct ngbe_ring *rx_ring, u16 cleaned_count);

void ngbe_set_rx_mode(struct net_device *netdev);
int ngbe_write_mc_addr_list(struct net_device *netdev);
int ngbe_setup_tc(struct net_device *dev, u8 tc);
void ngbe_tx_ctxtdesc(struct ngbe_ring *tx_ring, u32 vlan_macip_lens,
		      u32 fcoe_sof_eof, u32 type_tucmd, u32 mss_l4len_idx);
void ngbe_do_reset(struct net_device *netdev);
void ngbe_write_eitr(struct ngbe_q_vector *q_vector);
int ngbe_poll(struct napi_struct *napi, int budget);
void ngbe_disable_rx_queue(struct ngbe_adapter *adapter,
			   struct ngbe_ring *ring);
void ngbe_vlan_strip_enable(struct ngbe_adapter *adapter);
void ngbe_vlan_strip_disable(struct ngbe_adapter *adapter);
void ngbe_print_tx_hang_status(struct ngbe_adapter *adapter);

void ngbe_dbg_adapter_init(struct ngbe_adapter *adapter);
void ngbe_dbg_adapter_exit(struct ngbe_adapter *adapter);
void ngbe_dbg_init(void);
void ngbe_dbg_exit(void);
void ngbe_dump(struct ngbe_adapter *adapter);

static inline struct netdev_queue *txring_txq(const struct ngbe_ring *ring)
{
	return netdev_get_tx_queue(ring->netdev, ring->queue_index);
}

int ngbe_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd);
int ngbe_write_uc_addr_list(struct net_device *netdev, int pool);
void ngbe_full_sync_mac_table(struct ngbe_adapter *adapter);
int ngbe_add_mac_filter(struct ngbe_adapter *adapter, const u8 *addr, u16 pool);
int ngbe_del_mac_filter(struct ngbe_adapter *adapter, const u8 *addr, u16 pool);
int ngbe_available_rars(struct ngbe_adapter *adapter);
void ngbe_vlan_mode(struct net_device *netdev, u32 features);

void ngbe_ptp_init(struct ngbe_adapter *adapter);
void ngbe_ptp_stop(struct ngbe_adapter *adapter);
void ngbe_ptp_suspend(struct ngbe_adapter *adapter);
void ngbe_ptp_overflow_check(struct ngbe_adapter *adapter);
void ngbe_ptp_rx_hang(struct ngbe_adapter *adapter);
void ngbe_ptp_rx_hwtstamp(struct ngbe_adapter *adapter, struct sk_buff *skb);
int ngbe_ptp_set_ts_config(struct ngbe_adapter *adapter, struct ifreq *ifr);
int ngbe_ptp_get_ts_config(struct ngbe_adapter *adapter, struct ifreq *ifr);
void ngbe_ptp_start_cyclecounter(struct ngbe_adapter *adapter);
void ngbe_ptp_reset(struct ngbe_adapter *adapter);
void ngbe_ptp_check_pps_event(struct ngbe_adapter *adapter);
#ifdef CONFIG_PCI_IOV
void ngbe_sriov_reinit(struct ngbe_adapter *adapter);
#endif

void ngbe_set_rx_drop_en(struct ngbe_adapter *adapter);

u32 ngbe_rss_indir_tbl_entries(struct ngbe_adapter *adapter);
void ngbe_store_reta(struct ngbe_adapter *adapter);

void ngbe_clean_tx_ring(struct ngbe_ring *tx_ring);
void ngbe_clean_rx_ring(struct ngbe_ring *rx_ring);
u32 ngbe_tx_cmd_type(u32 tx_flags);

void ngbe_release_hw_control(struct ngbe_adapter *adapter);
void ngbe_get_hw_control(struct ngbe_adapter *adapter);

/**
 * interrupt masking operations. each bit in PX_ICn correspond to a interrupt.
 * disable a interrupt by writing to PX_IMS with the corresponding bit=1
 * enable a interrupt by writing to PX_IMC with the corresponding bit=1
 * trigger a interrupt by writing to PX_ICS with the corresponding bit=1
 **/
//#define NGBE_INTR_ALL (~0ULL)
#define NGBE_INTR_ALL 0x1FF
#define NGBE_INTR_MISC(A) (1ULL << (A)->num_q_vectors)
#define NGBE_INTR_MISC_VMDQ(A) \
	(1ULL << ((A)->num_q_vectors + (A)->ring_feature[RING_F_VMDQ].offset))
#define NGBE_INTR_QALL(A) (NGBE_INTR_MISC(A) - 1)
#define NGBE_INTR_Q(i) (1ULL << (i))
static inline void ngbe_intr_enable(struct ngbe_hw *hw, u64 qmask)
{
	u32 mask;

	mask = (qmask & 0xFFFFFFFF);
	if (mask)
		wr32(hw, NGBE_PX_IMC, mask);
}

static inline void ngbe_intr_disable(struct ngbe_hw *hw, u64 qmask)
{
	u32 mask;

	mask = (qmask & 0xFFFFFFFF);
	if (mask)
		wr32(hw, NGBE_PX_IMS, mask);
}

static inline void ngbe_intr_trigger(struct ngbe_hw *hw, u64 qmask)
{
	u32 mask;

	mask = (qmask & 0xFFFFFFFF);
	if (mask)
		wr32(hw, NGBE_PX_ICS, mask);
}

#define NGBE_RING_SIZE(R) ((R)->count < NGBE_MAX_TXD ? (R)->count / 128 : 0)

#define NGBE_CPU_TO_BE16(_x) cpu_to_be16(_x)
#define NGBE_BE16_TO_CPU(_x) be16_to_cpu(_x)
#define NGBE_CPU_TO_BE32(_x) cpu_to_be32(_x)
#define NGBE_BE32_TO_CPU(_x) be32_to_cpu(_x)

#define msec_delay(_x) msleep(_x)

#define usec_delay(_x) udelay(_x)

#define NGBE_NAME "ngbe"

/* #define DBG 1 */

static inline struct device *pci_dev_to_dev(struct pci_dev *pdev)
{
	return &pdev->dev;
}

#define NGBE_SFP_DETECT_RETRIES        2

struct ngbe_hw;
struct ngbe_msg {
	u16 msg_enable;
};

struct net_device *ngbe_hw_to_netdev(const struct ngbe_hw *hw);
struct ngbe_msg *ngbe_hw_to_msg(const struct ngbe_hw *hw);

#define hw_dbg(hw, format, arg...) \
	netdev_dbg(ngbe_hw_to_netdev(hw), format, ## arg)
#define hw_err(hw, format, arg...) \
	netdev_err(ngbe_hw_to_netdev(hw), format, ## arg)
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

#define NGBE_FAILED_READ_CFG_DWORD 0xffffffffU
#define NGBE_FAILED_READ_CFG_WORD  0xffffU
#define NGBE_FAILED_READ_CFG_BYTE  0xffU

extern u32 ngbe_read_reg(struct ngbe_hw *hw, u32 reg, bool quiet);
extern u16 ngbe_read_pci_cfg_word(struct ngbe_hw *hw, u32 reg);
extern void ngbe_write_pci_cfg_word(struct ngbe_hw *hw, u32 reg, u16 value);

#define NGBE_READ_PCIE_WORD ngbe_read_pci_cfg_word
#define NGBE_WRITE_PCIE_WORD ngbe_write_pci_cfg_word
#define NGBE_R32_Q(h, r) ngbe_read_reg(h, r, true)

#ifndef writeq
#define writeq(val, addr)                             \
	do {                                          \
		writel((u32)(val), addr);             \
		writel((u32)(val >> 32), (addr + 4)); \
	} while (0)
#endif

#define NGBE_EEPROM_GRANT_ATTEMPS 100
#define NGBE_HTONL(_i) htonl(_i)
#define NGBE_NTOHL(_i) ntohl(_i)
#define NGBE_NTOHS(_i) ntohs(_i)
#define NGBE_CPU_TO_LE32(_i) cpu_to_le32(_i)
#define NGBE_LE32_TO_CPUS(_i) le32_to_cpus(_i)

enum {
	NGBE_ERROR_SOFTWARE,
	NGBE_ERROR_POLLING,
	NGBE_ERROR_INVALID_STATE,
	NGBE_ERROR_UNSUPPORTED,
	NGBE_ERROR_ARGUMENT,
	NGBE_ERROR_CAUTION,
};

#define ERROR_REPORT(level, format, arg...) do {                               \
	switch (level) {                                                       \
	case NGBE_ERROR_SOFTWARE:                                             \
	case NGBE_ERROR_CAUTION:                                              \
	case NGBE_ERROR_POLLING:                                              \
		netif_warn(ngbe_hw_to_msg(hw), drv, ngbe_hw_to_netdev(hw),   \
			   format, ## arg);                                    \
		break;                                                         \
	case NGBE_ERROR_INVALID_STATE:                                        \
	case NGBE_ERROR_UNSUPPORTED:                                          \
	case NGBE_ERROR_ARGUMENT:                                             \
		netif_err(ngbe_hw_to_msg(hw), hw, ngbe_hw_to_netdev(hw),     \
			  format, ## arg);                                     \
		break;                                                         \
	default:                                                               \
		break;                                                         \
	}                                                                      \
} while (0)

#define ERROR_REPORT1 ERROR_REPORT
#define ERROR_REPORT2 ERROR_REPORT
#define ERROR_REPORT3 ERROR_REPORT

#endif /* _NGBE_H_ */
