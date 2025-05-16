/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_RX_H
#define HINIC3_RX_H

#ifdef HAVE_PAGE_POOL_SUPPORT
#include <net/page_pool/helpers.h>
#endif

#include <linux/types.h>
#include <linux/device.h>
#include <linux/mm_types.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/u64_stats_sync.h>

#include "hinic3_nic_io.h"
#include "hinic3_nic_qp.h"
#include "hinic3_nic_dev.h"

/* rx cqe checksum err */
#define HINIC3_RX_CSUM_IP_CSUM_ERR	BIT(0)
#define HINIC3_RX_CSUM_TCP_CSUM_ERR	BIT(1)
#define HINIC3_RX_CSUM_UDP_CSUM_ERR	BIT(2)
#define HINIC3_RX_CSUM_IGMP_CSUM_ERR	BIT(3)
#define HINIC3_RX_CSUM_ICMPV4_CSUM_ERR	BIT(4)
#define HINIC3_RX_CSUM_ICMPV6_CSUM_ERR	BIT(5)
#define HINIC3_RX_CSUM_SCTP_CRC_ERR	BIT(6)
#define HINIC3_RX_CSUM_HW_CHECK_NONE	BIT(7)
#define HINIC3_RX_CSUM_IPSU_OTHER_ERR	BIT(8)

#define HINIC3_HEADER_DATA_UNIT 2
#define HINIC3_CQE_LEN 32

#define HINIC3_COMPACT_CQE_8B 8
#define HINIC3_COMPACT_CQE_16B 16

#define HINIC3_RQ_CQE_SEPARATE	0
#define HINIC3_RQ_CQE_INTEGRATE	1

struct hinic3_rxq_stats {
	u64				packets;
	u64				bytes;
	u64				errors;
	u64				csum_errors;
	u64				other_errors;
	u64				dropped;
	u64				xdp_dropped;
	u64				rx_buf_empty;

	u64				alloc_skb_err;
	u64				alloc_rx_buf_err;
	u64				xdp_large_pkt;
	u64				restore_drop_sge;
	u64				rsvd2;
#ifdef HAVE_NDO_GET_STATS64
	struct u64_stats_sync		syncp;
#else
	struct u64_stats_sync_empty	syncp;
#endif
};

struct hinic3_rx_info {
	dma_addr_t		buf_dma_addr;

	struct hinic3_rq_cqe	*cqe;
	dma_addr_t		cqe_dma;
	struct page		*page;
#ifdef HAVE_PAGE_POOL_SUPPORT
	struct page_pool *page_pool;
#endif
	u32			page_offset;
	u32			rsvd1;
	struct hinic3_rq_wqe	*rq_wqe;
	struct sk_buff		*saved_skb;
	u32			skb_len;
	u32			rsvd2;
};

struct hinic3_rxq {
	struct net_device	*netdev;

	u16			q_id;
	u16			rsvd1;
	u32			q_depth;
	u32			q_mask;

	u16			buf_len;
	u16			rsvd2;
	u32			rx_buff_shift;
	u32			dma_rx_buff_size;

	struct hinic3_rxq_stats	rxq_stats;
	u32			cons_idx;
	u32			delta;

	u32			irq_id;
	u16			msix_entry_idx;
	u16			rsvd3;

	struct hinic3_rx_info	*rx_info;
	struct hinic3_io_queue	*rq;
#ifdef HAVE_XDP_SUPPORT
	struct bpf_prog		*xdp_prog;
#endif

	struct hinic3_irq	*irq_cfg;
	u16			next_to_alloc;
	u16			next_to_update;
	struct device		*dev; /* device for DMA mapping */

	u64			status;
	dma_addr_t		cqe_start_paddr;
	void			*cqe_start_vaddr;

	u64			last_moder_packets;
	u64			last_moder_bytes;
	u8			last_coalesc_timer_cfg;
	u8			last_pending_limt;
	u16			restore_buf_num;
	u32			rsvd5;
	u64			rsvd6;

	u32			last_sw_pi;
	u32			last_sw_ci;

	u32			last_hw_ci;
	u8			rx_check_err_cnt;
	u8			rxq_print_times;
	u16			restore_pi;

	u64			last_packets;
} ____cacheline_aligned;

struct hinic3_dyna_rxq_res {
	u16			next_to_alloc;
	struct hinic3_rx_info	*rx_info;
	dma_addr_t		cqe_start_paddr;
	void			*cqe_start_vaddr;
#ifdef HAVE_PAGE_POOL_SUPPORT
	struct page_pool	*page_pool;
#endif
};

int hinic3_alloc_rxqs(struct net_device *netdev);

void hinic3_free_rxqs(struct net_device *netdev);

int hinic3_alloc_rxqs_res(struct hinic3_nic_dev *nic_dev, u16 num_rq,
			  u32 rq_depth, struct hinic3_dyna_rxq_res *rxqs_res);

void hinic3_free_rxqs_res(struct hinic3_nic_dev *nic_dev, u16 num_rq,
			  u32 rq_depth, struct hinic3_dyna_rxq_res *rxqs_res);

int hinic3_configure_rxqs(struct hinic3_nic_dev *nic_dev, u16 num_rq,
			  u32 rq_depth, struct hinic3_dyna_rxq_res *rxqs_res);

int hinic3_rx_configure(struct net_device *netdev, u8 dcb_en);

void hinic3_rx_remove_configure(struct net_device *netdev);

int hinic3_rx_poll(struct hinic3_rxq *rxq, int budget);

void hinic3_rxq_get_stats(struct hinic3_rxq *rxq,
			  struct hinic3_rxq_stats *stats);

void hinic3_rxq_clean_stats(struct hinic3_rxq_stats *rxq_stats);

void hinic3_rx_get_cqe_info(void *rx_cqe, void *cqe_info, u8 cqe_mode);

void hinic3_rx_get_compact_cqe_info(void *rx_cqe, void *cqe_info, u8 cqe_mode);

void hinic3_rxq_check_work_handler(struct work_struct *work);

#endif
