/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_RX_DEFINE_H
#define SSS_NIC_RX_DEFINE_H

#include <linux/types.h>

#include "sss_kernel.h"
#include "sss_nic_qp_define.h"
#include "sss_nic_io_define.h"
#include "sss_nic_irq_define.h"

struct sss_nic_rq_stats {
	u64	rx_packets;
	u64	rx_bytes;
	u64	errors;
	u64	csum_errors;
	u64	other_errors;
	u64	rx_dropped;
	u64	xdp_dropped;
	u64	rx_buf_errors;

	u64	alloc_rx_dma_err;
	u64	alloc_skb_err;
	u64	reset_drop_sge;
	u64	large_xdp_pkts;
	u64	rsvd2;

#ifdef HAVE_NDO_GET_STATS64
	struct u64_stats_sync		stats_sync;
#else
	struct u64_stats_sync_empty	stats_sync;
#endif
};

struct sss_nic_rx_desc {
	dma_addr_t			buf_daddr;
	dma_addr_t			cqe_daddr;
	struct sss_nic_rqe	*rqe;
	struct sss_nic_cqe	*cqe;
	struct page			*page;
	u32					page_offset;
};

struct sss_nic_rq_desc {
	struct net_device	*netdev;
	struct device		*dev; /* device for DMA mapping */

	u32			irq_id;
	u16			msix_id;

	u16			qid;
	u32			qid_mask;
	u32			q_depth;

	u32			buff_size_shift;
	u32			dma_buff_size;
	u16			buf_len;
	u16			rsvd;

	u16			backup_pi;
	u16			pi;
	u32			last_sw_pi;
	u32			last_sw_ci;
	u32			last_hw_ci;
	u32			ci;
	u16			reset_pi;
	u16			reset_wqe_num;
	u32			delta;

	u64			last_rx_bytes;
	u64			last_rx_pkts;
	u64			rx_pkts;

	unsigned long	status;

	u8			last_pending_limt;
	u8			last_coal_timer;

	u8			print_err_cnt;
	u8			check_err_cnt;

	struct sss_nic_irq_cfg	*irq_cfg;

	struct sss_nic_rq_stats	stats;

	struct sss_nic_rx_desc	*rx_desc_group;
	struct sss_nic_io_queue	*rq;

#ifdef HAVE_XDP_SUPPORT
	struct bpf_prog		*xdp_prog;
#endif

	void			*cqe_vaddr;
	dma_addr_t		cqe_paddr;
} ____cacheline_aligned;

struct sss_nic_rq_resource {
	u16			page_num;
	u8			rsvd[6];
	struct sss_nic_rx_desc	*rx_desc_group;
	void		*cqe_vaddr;
	dma_addr_t	cqe_paddr;
};

#define SSSNIC_RQ_STATS_INC(rq_desc, field)			\
do {							\
	u64_stats_update_begin(&(rq_desc)->stats.stats_sync);	\
	(rq_desc)->stats.field++;				\
	u64_stats_update_end(&(rq_desc)->stats.stats_sync);	\
} while (0)

#endif
