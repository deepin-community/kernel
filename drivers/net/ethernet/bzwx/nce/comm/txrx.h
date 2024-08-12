/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2024, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _TXRX_H
#define _TXRX_H

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/aer.h>
#include <linux/netdevice.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/hash.h>
#include <linux/string.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/pkt_sched.h>
#include <linux/ipv6.h>
#include <net/checksum.h>
#include <linux/in6.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/if_ether.h>
#include <net/ip6_checksum.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>
#include <linux/if_bridge.h>
#include <linux/iommu.h>

#include "common.h"

#define NE6X_MAX_NUM_DESCRIPTORS     (16 * 1024)
#define NE6X_DEFAULT_NUM_DESCRIPTORS (4 * 1024)
#define NE6X_MIN_NUM_DESCRIPTORS     64
#define NE6X_REQ_DESCRIPTOR_MULTIPLE 32

#define NE6X_MAX_BUFFER_TXD 8
#define NE6X_MIN_TX_LEN     60

#define NE6X_TAIL_REG_NUM      4
#define NE6X_RX_BUFFER_WRITE 32 /* Must be power of 2 */

/* The size limit for a transmit buffer in a descriptor is 15K.
 * In order to align with the read requests we will align the value to
 * the nearest 4K which represents our maximum read request size.
 */
#define NE6X_MAX_READ_REQ_SIZE        4096
#define NE6X_MAX_DATA_PER_TXD         (15500 - 32 - 4 - 1)
#define NE6X_MAX_DATA_PER_TXD_ALIGNED \
	(NE6X_MAX_DATA_PER_TXD & ~(NE6X_MAX_READ_REQ_SIZE - 1))

/* Supported Rx Buffer Sizes (a multiple of 128) */
#define NE6X_PACKET_HDR_PAD ETH_HLEN
#define NE6X_RXBUFFER_256   256
#define NE6X_RXBUFFER_2048  2048
#define NE6X_RXBUFFER_4096  4096 /* Used for large frames w/ padding */
/*CIU buffer max len is 15k*/
#define NE6X_MAX_RXBUFFER 15360 /* largest size for single descriptor */
#define NE6X_MIN_MTU_SIZE 128
#define NE6X_RX_HDR_SIZE NE6X_RXBUFFER_256

#define NE6X_TX_PRIV_TAG_SIZE   32
#define NE6X_TX_DESC_SIZE        32
/* iterator for handling rings in ring container */
#define ne6x_for_each_ring(pos, head) \
	for (pos = (head).ring; pos; pos = pos->next)

#define NE6X_RX_DMA_ATTR (DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)

#define NE6X_RX_DESC(R, i) (&(((union ne6x_rx_desc *)((R)->desc))[i]))
#define NE6X_TX_DESC(R, i) (&(((struct ne6x_tx_desc *)((R)->desc))[i]))
#define NE6X_TX_TAG(R, i)  (&(((struct ne6x_tx_tag *)((R)->desc))[i]))
#define NE6X_CQ_DESC(R, i) (&(((struct ne6x_cq_desc *)((R)->desc))[i]))

#define NE6X_DESC_UNUSED(R) \
	((((R)->next_to_clean > (R)->next_to_use) ? \
		0 : (R)->count) + (R)->next_to_clean -  (R)->next_to_use - 1)

struct ne6x_tx_desc_status {
	/* pkt drop */
	u8 tx_drop_addr     : 1;
	u8 rsv3             : 1;
	u8 rsv2             : 1;
	/* normal */
	u8 tx_done          : 1;
	/* ecc error */
	u8 tx_ecc_err       : 1;
	u8 rsv1             : 1;
	u8 rsv0             : 1;
	/* pcie error */
	u8 tx_pcie_read_err : 1;
};

struct ne6x_tx_desc {
	union {
		/* Hardware write back*/
		struct ne6x_tx_desc_status flags;
		u8                        val;
	} u;

	u8 rsv0          : 1;
	u8 vp            : 7;
	u8 event_trigger : 1;
	u8 chain         : 1;
	u8 transmit_type : 2;
	u8 sop_valid     : 1;
	u8 eop_valid     : 1;
	u8 tso           : 1;
	u8 rsv1          : 1;
	u8 rsv2;
	u8 rsv3;

	u8 l3_csum : 1;
	u8 l3_ofst : 7;
	u8 l4_csum : 1;
	u8 l4_ofst : 7;
	u8 pld_ofst;

	__le64 mop_cnt : 24;
	__le64 sop_cnt : 16;
	__le64 rsv4    : 8;
	__le64 mss     : 16;
	__le64 buffer_mop_addr;
	__le64 buffer_sop_addr;
};

struct ne6x_tx_tag {
	u8 resv0;
	u8 tag_pi1       : 1;
	u8 resv1         : 7;
	u8 l3_csum       : 1;
	u8 l4_csum       : 1;
	u8 vxl_l3_csum   : 1;
	u8 vxl_l4_csum   : 1;
	u8 tag_resv      : 3;
	u8 tag_pi0       : 1;
	u8 tag_vport;
	u16 tag_vlan1; /* 1q vlan */
	u16 tag_vlan2; /* 1ad vlan */

	__le64 resv2       : 32;
	__le64 tag_num     : 16;
	__le64 tag_mss     : 16; /* mss */

	u8  l3_ofst;
	u8  l4_ofst;
	u16 l4_len; /* l4hdr + pld_size */
	u8  vxl_l3_ofst;
	u8  vxl_l4_ofst;
	u16 vxl_l4_len; /* l4hdr + pld_size */

	__le64 resv3;
};

struct ne6x_tx_buf {
	struct ne6x_tx_desc *next_to_watch;
	struct sk_buff     *skb;
	u32                 bytecount;
	u8                  jumbo_frame;  /* fragment when bytecount > 15.5KB*/
	u8                  jumbo_finsh; /* when last frame of jumbo packet transmitted, set it 1 */
	u16                 rsv;
	int                 napi_budget; /* when bytecount > 15.5KB, accumulating NPAI trigger count
					  *  in transmit irq handler
					  */
	u16                 gso_segs;
	dma_addr_t          tag_dma;
	DEFINE_DMA_UNMAP_ADDR(dma);
	DEFINE_DMA_UNMAP_LEN(len);
};

struct ne6x_rx_desc_status {
	u8 rx_mem_err    : 1; /* MOP_MEM_ADDR/SOP_MEM_ADDR/MOP_MEM_LEN=0, pkt need drop */
	u8 rx_mem_ovflow : 1; /* SOP_MEM_OVFLOW ==1, mop have pkt */
	u8 rsv           : 1;
	u8 rx_eop        : 1; /* EOP flag */
	u8 rx_csum_err   : 1; /* checksum error */
	u8 rx_err        : 1; /* Not enough descriptors */
	u8 rx_mem_used   : 1; /* MEM_USED, Normal */
	u8 pd_type       : 1; /* 0 ingress pd, 1 egress pd */
};

#define NE6X_RX_DESC_STATUS_EOF_SHIFT 3
#define NE6X_RX_DESC_STATUS_ERR_SHIFT 0

/* Receive Descriptor */
union ne6x_rx_desc {
	struct {
		u8     rsv3;
		u8     rsv2 : 1;
		u8     vp   : 7;
		__le16 mop_mem_len;
		__le16 sop_mem_len;
		__le16 rsv1;
		__le64 buffer_sop_addr;
		__le64 buffer_mop_addr;

		__le64 rsv0;
	} w; /* write */

	struct {
		union {
			struct ne6x_rx_desc_status flags;
			u8                        val;
		} u;
		u8     rsv2 : 1;
		u8     vp   : 7;
		u8     pd[24];
		__le16 rsv0;
		__le16 rsv1;
		__le16 pkt_len;
	} wb; /* Writeback */
};

struct ne6x_tx_cq_desc {
	u8  cq_tx_stats;
	u16 cq_tx_offset;
} __packed;

struct ne6x_rx_cq_desc {
	u8  cq_rx_stats;
	u16 cq_rx_len;
	u16 cq_rx_offset;
} __packed;

struct ne6x_cq_desc {
	u8 ctype : 1;
	u8 rsv0  : 3;
	u8 num   : 4;
	u8 rsv1;

	union {
		struct ne6x_tx_cq_desc tx_cq[10];
		struct ne6x_rx_cq_desc rx_cq[6];
		u8                    data[30];
	} payload;
};

struct ne6x_rx_buf {
	dma_addr_t   dma;
	struct page *page;
	u32          page_offset;
	u16          pagecnt_bias;
};

struct ne6x_q_stats {
	u64 packets;
	u64 bytes;
};

struct ne6x_txq_stats {
	u64 restart_q;
	u64 tx_busy;
	u64 tx_linearize;
	u64 csum_err;
	u64 csum_good;
	u64 tx_pcie_read_err;
	u64 tx_ecc_err;
	u64 tx_drop_addr;
};

struct ne6x_rxq_stats {
	u64 non_eop_descs;
	u64 alloc_page_failed;
	u64 alloc_buf_failed;
	u64 page_reuse_count;
	u64 csum_err;
	u64 csum_good;
	u64 rx_mem_error;
	u64 rx_err;
};

struct ne6x_cq_stats {
	u64 cq_num;
	u64 tx_num;
	u64 rx_num;
};

#define NE6X_SG_SOP_FLAG          BIT(0)
#define NE6X_SG_EOP_FLAG          BIT(1)
#define NE6X_SG_FST_SG_FLAG       BIT(13)
#define NE6X_SG_LST_SG_FLAG       BIT(14)
#define NE6X_SG_JUMBO_FLAG        BIT(15)
#define NE6X_SG_FRAG_FLAG         BIT(4)
#define NE6X_MAX_DESC_NUM_PER_SKB 16

struct ne6x_sg_info {
	void *p;
	u16 offset;
	u16 len;
	u16 flag;
	u16 base_mss_no;
};

struct ne6x_sg_list {
	u16 sg_num;
	u16 mss;
	u16 sgl_mss_cnt;
	struct ne6x_sg_info sg[NE6X_MAX_DESC_NUM_PER_SKB];
};

/* descriptor ring, associated with a adapter */
struct ne6x_ring {
	/* CL1 - 1st cacheline starts here */
	void *adpt;
	struct ne6x_ring     *next;     /* pointer to next ring in q_vector */
	void                 *desc;     /* Descriptor ring memory */
	struct device        *dev;      /* Used for DMA mapping */
	struct net_device    *netdev;   /* netdev ring maps to */
	struct ne6x_q_vector *q_vector; /* Backreference to associated vector */

	u64 __iomem        *tail;

	struct ne6x_sg_list  *sgl;

	union {
		struct ne6x_tx_buf *tx_buf;
		struct ne6x_rx_buf *rx_buf;
	};

	u16 count;   /* Number of descriptors */
	u16 reg_idx; /* HW register index of the ring */

	/* used in interrupt processing */
	u16 next_to_use;
	u16 next_to_clean;
	u16 next_to_alloc;
	u16 cq_last_expect;

	u16 queue_index; /* Queue number of ring */
	u16 rx_buf_len;

	/* stats structs */
	struct ne6x_q_stats    stats;
	struct u64_stats_sync syncp;

	union {
		struct ne6x_txq_stats tx_stats;
		struct ne6x_rxq_stats rx_stats;
		struct ne6x_cq_stats  cq_stats;
	};

	struct rcu_head rcu; /* to avoid race on free */
	dma_addr_t      dma;  /* physical address of ring */
	unsigned int    size; /* length of descriptor ring in bytes */
	struct sk_buff *skb; /* When ne6x_clean_rx_ring_irq() must
			      * return before it sees the EOP for
			      * the current packet, we save that skb
			      * here and resume receiving this
			      * packet the next time
			      * ne6x_clean_rx_ring_irq() is called
			      * for this ring.
			      */
} ____cacheline_internodealigned_in_smp;

struct ne6x_ring_container {
	/* head of linked-list of rings */
	struct ne6x_ring *ring;
	unsigned int     total_bytes;   /* total bytes processed this int */
	unsigned int     total_packets; /* total packets processed this int */
	u16              count;
};

union rx_ol_flags {
	u32 ol_flags;		/* Offload Feature Bits. */
	struct {
#if defined(__BIG_ENDIAN_BITFIELD)
		u32 ol_flag_rx_vlan        :1;
		u32 rx_ip_cksum_bad        :1;
		u32 rx_ip_cksum_good       :1;
		u32 rx_l4_cksum_bad        :1;
		u32 rx_l4_cksum_good       :1;
		u32 rx_rss_hash            :1;
		u32 rx_qinq                :1;
		u32 rx_lro                 :1;
		u32 rx_vlan_striped        :1;
		u32 rx_qinq_striped        :1;
		u32 rx_dvlan               :1;
		u32 rx_vlan_bad            :1;
		u32 rx_inner_ip_cksum_bad  :1;
		u32 rx_inner_ip_cksum_good :1;
		u32 rx_inner_l4_cksum_bad  :1;
		u32 rx_inner_l4_cksum_good :1;
		u32 rx_tnl_csum            :1;
		u32 rsv0                   :1;
		u32 tag_num                :8;
		u32 rsv1                   :6;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
		u32 rsv1                   :6;
		u32 tag_num                :8;
		u32 rsv0                   :1;
		u32 rx_tnl_csum            :1;
		u32 rx_vlan_striped        :1;
		u32 rx_qinq_striped        :1;
		u32 rx_dvlan               :1;
		u32 rx_vlan_bad            :1;
		u32 rx_inner_ip_cksum_bad  :1;
		u32 rx_inner_ip_cksum_good :1;
		u32 rx_inner_l4_cksum_bad  :1;
		u32 rx_inner_l4_cksum_good :1;
		u32 ol_flag_rx_vlan        :1;
		u32 rx_ip_cksum_bad        :1;
		u32 rx_ip_cksum_good       :1;
		u32 rx_l4_cksum_bad        :1;
		u32 rx_l4_cksum_good       :1;
		u32 rx_rss_hash            :1;
		u32 rx_qinq                :1;
		u32 rx_lro                 :1;
#endif
	} flag_bits;
};

struct rx_hdr_info {
	union rx_ol_flags ol_flag;
	u32 rss_hash;			/* RSS Hash Value */
	u32 vlan_tci_outer:16;		/* VLAN Outer Tag Control Identifier */
	u32 vlan_tci:16;		/* VLAN Tag Control Identifier */
};

#define NE6X_INT_NAME_STR_LEN (IFNAMSIZ + 16)

/* struct that defines an interrupt vector */
struct ne6x_q_vector {
	void *adpt;

	u16 v_idx; /* index in the adpt->q_vector array. */
	u16 reg_idx;

	struct napi_struct napi;

	struct ne6x_ring_container rx;
	struct ne6x_ring_container tx;
	struct ne6x_ring_container cq;
	struct ne6x_ring_container tg;

	u8 num_ringpairs; /* total number of ring pairs in vector */

	cpumask_t affinity_mask;
	struct irq_affinity_notify affinity_notify;

	char name[NE6X_INT_NAME_STR_LEN];
} ____cacheline_internodealigned_in_smp;

#define DESC_NEEDED (MAX_SKB_FRAGS + 6)

static inline unsigned int ne6x_rx_pg_order(struct ne6x_ring *ring)
{
#if (PAGE_SIZE < 8192)
	if (ring->rx_buf_len > (PAGE_SIZE / 2))
		return 1;
#endif
	return 0;
}

#define ne6x_rx_pg_size(_ring) (PAGE_SIZE << ne6x_rx_pg_order(_ring))

static inline struct netdev_queue *txring_txq(const struct ne6x_ring *ring)
{
	return netdev_get_tx_queue(ring->netdev, ring->queue_index);
}

int ne6x_clean_cq_irq(struct ne6x_q_vector *q_vector, struct ne6x_ring *cq_ring, int napi_budget);
int ne6x_clean_rx_irq(struct ne6x_ring *rx_ring, int budget);
int ne6x_clean_tx_irq(struct ne6x_adapt_comm *comm, struct ne6x_ring *tx_ring, int napi_budget);
netdev_tx_t ne6x_xmit_frame_ring(struct sk_buff *skb, struct ne6x_ring *tx_ring,
				 struct ne6x_ring *tag_ring, bool jumbo_frame);
void ne6x_tail_update(struct ne6x_ring *ring, int val);
int ne6x_setup_tx_descriptors(struct ne6x_ring *tx_ring);
int ne6x_setup_rx_descriptors(struct ne6x_ring *rx_ring);
int ne6x_setup_cq_descriptors(struct ne6x_ring *cq_ring);
int ne6x_setup_tg_descriptors(struct ne6x_ring *tg_ring);
int ne6x_setup_tx_sgl(struct ne6x_ring *tx_ring);

#endif
