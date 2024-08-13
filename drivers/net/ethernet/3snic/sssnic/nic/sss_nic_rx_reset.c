// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/u64_stats_sync.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/sctp.h>
#include <linux/pkt_sched.h>
#include <linux/ipv6.h>
#include <linux/module.h>
#include <linux/compiler.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_io.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_rss.h"
#include "sss_nic_rx.h"
#include "sss_nic_cfg.h"

#define SSSNIC_RQ_GET_ERR_CNT_THRESHOLD		3
#define SSSNIC_RQ_CHECK_ERR_CNT_THRESHOLD	2
#define SSSNIC_RQ_PRINT_CNT_THRESHOLD		3

static inline void sss_nic_fill_wqe_sge(struct sss_nic_rx_desc *rx_desc,
					u8 wqe_type)
{
	dma_addr_t dma_addr = rx_desc->buf_daddr + rx_desc->page_offset;
	struct sss_nic_rqe *rqe = rx_desc->rqe;

	if (unlikely(wqe_type == SSSNIC_EXTEND_RQ_WQE)) {
		rqe->extend_rqe.bd_sect.sge.low_addr =
			sss_hw_be32(lower_32_bits(dma_addr));
		rqe->extend_rqe.bd_sect.sge.high_addr =
			sss_hw_be32(upper_32_bits(dma_addr));
	} else {
		rqe->normal_rqe.bd_lo_addr =
			sss_hw_be32(lower_32_bits(dma_addr));
		rqe->normal_rqe.bd_hi_addr =
			sss_hw_be32(upper_32_bits(dma_addr));
	}
}

static inline void sss_nic_free_wqe_buffer(struct sss_nic_dev *nic_dev,
					   struct sss_nic_rx_desc *rx_desc)
{
	if (rx_desc->buf_daddr) {
		dma_unmap_page(nic_dev->dev_hdl, rx_desc->buf_daddr,
			       nic_dev->rx_dma_buff_size, DMA_FROM_DEVICE);
		rx_desc->buf_daddr = 0;
	}

	if (rx_desc->page) {
		__free_pages(rx_desc->page, nic_dev->page_order);
		rx_desc->page = NULL;
	}
}

static inline int sss_nic_fill_idle_wqe(struct sss_nic_rq_desc *rq_desc,
					u32 wqebb_num, u32 start_pi)
{
	u32 pi = start_pi;
	u32 i;
	struct sss_nic_rx_desc *rx_desc = NULL;
	struct sss_nic_dev *nic_dev = netdev_priv(rq_desc->netdev);

	for (i = 0; i < wqebb_num; i++) {
		rx_desc = &rq_desc->rx_desc_group[pi];

		if (unlikely(!sss_nic_rx_alloc_dma_page(nic_dev, rx_desc))) {
			rq_desc->reset_pi = (u16)((rq_desc->reset_pi + i) & rq_desc->qid_mask);
			SSSNIC_RQ_STATS_INC(rq_desc, alloc_rx_dma_err);
			return -ENOMEM;
		}

		sss_nic_fill_wqe_sge(rx_desc, rq_desc->rq->wqe_type);

		pi = (u16)((pi + 1) & rq_desc->qid_mask);
		rq_desc->reset_wqe_num++;
	}

	return 0;
}

static int sss_nic_reset_rq(struct sss_nic_dev *nic_dev, u16 qid, u16 hw_ci)
{
	int ret;
	u32 i;
	u32 total;
	u32 ci;
	u32 pi;
	struct sss_nic_rq_desc *rq_desc = &nic_dev->rq_desc_group[qid];
	u32 idle_wqebb = rq_desc->delta - rq_desc->reset_wqe_num;
	struct sss_nic_rx_desc *rx_desc = NULL;

	if (rq_desc->delta < rq_desc->reset_wqe_num)
		return -EINVAL;

	if (rq_desc->reset_wqe_num == 0)
		rq_desc->reset_pi = rq_desc->pi;

	ci = rq_desc->ci & rq_desc->qid_mask;
	total = ci + rq_desc->q_depth - rq_desc->pi;
	if ((total % rq_desc->q_depth) != rq_desc->delta)
		return -EINVAL;

	ret = sss_nic_fill_idle_wqe(rq_desc, idle_wqebb, rq_desc->reset_pi);
	if (ret)
		return ret;

	nic_info(nic_dev->dev_hdl, "Reset rq: rq %u, restore_buf_num:%u\n", qid,
		 rq_desc->reset_wqe_num);

	pi = (hw_ci + rq_desc->q_depth - 1) & rq_desc->qid_mask;
	rx_desc = &rq_desc->rx_desc_group[pi];
	sss_nic_free_wqe_buffer(nic_dev, rx_desc);

	rq_desc->delta = 1;
	rq_desc->reset_wqe_num = 0;
	rq_desc->pi = (u16)pi;
	rq_desc->backup_pi = rq_desc->pi;
	rq_desc->ci = (u16)((rq_desc->pi + 1) & rq_desc->qid_mask);

	for (i = 0; i < rq_desc->q_depth; i++) {
		if (!SSSNIC_GET_RX_DONE(sss_hw_cpu32(rq_desc->rx_desc_group[i].cqe->state)))
			continue;

		rq_desc->rx_desc_group[i].cqe->state = 0;
		SSSNIC_RQ_STATS_INC(rq_desc, reset_drop_sge);
	}

	ret = sss_nic_cache_out_qp_resource(nic_dev->nic_io);
	if (ret) {
		SSSNIC_CLEAR_NIC_DEV_FLAG(nic_dev, SSSNIC_RXQ_RECOVERY);
		return ret;
	}

	sss_nic_write_db(rq_desc->rq, rq_desc->qid & (SSSNIC_DCB_COS_MAX - 1),
			 RQ_CFLAG_DP, (u16)((u32)rq_desc->pi << rq_desc->rq->wqe_type));

	return 0;
}

static bool sss_nic_rq_is_normal(struct sss_nic_rq_desc *rq_desc,
				 struct sss_nic_rq_pc_info check_info)
{
	u32 status;
	u32 sw_ci = rq_desc->ci & rq_desc->qid_mask;

	if (check_info.hw_pi != check_info.hw_ci ||
	    check_info.hw_ci != rq_desc->last_hw_ci)
		return true;

	if (rq_desc->stats.rx_packets != rq_desc->rx_pkts ||
	    rq_desc->pi != rq_desc->last_sw_pi)
		return true;

	status = SSSNIC_GET_RQ_CQE_STATUS(rq_desc, sw_ci);
	if (SSSNIC_GET_RX_DONE(status))
		return true;

	if (sw_ci != rq_desc->last_sw_ci || rq_desc->pi != check_info.hw_pi)
		return true;

	return false;
}

void sss_nic_rq_watchdog_handler(struct work_struct *work)
{
	int ret;
	u16 qid;
	struct sss_nic_rq_pc_info *check_info = NULL;
	struct sss_nic_rq_desc *rq_desc = NULL;
	struct delayed_work *delay = to_delayed_work(work);
	struct sss_nic_dev *nic_dev = container_of(delay, struct sss_nic_dev, rq_watchdog_work);
	u64 size = sizeof(*check_info) * nic_dev->qp_res.qp_num;

	if (!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_INTF_UP))
		return;

	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_RXQ_RECOVERY))
		queue_delayed_work(nic_dev->workq, &nic_dev->rq_watchdog_work, HZ);

	if (!size)
		return;
	check_info = kzalloc(size, GFP_KERNEL);
	if (!check_info)
		return;

	ret = sss_nic_rq_hw_pc_info(nic_dev, check_info, nic_dev->qp_res.qp_num,
				    nic_dev->rq_desc_group[0].rq->wqe_type);
	if (ret) {
		nic_dev->get_rq_fail_cnt++;
		if (nic_dev->get_rq_fail_cnt >= SSSNIC_RQ_GET_ERR_CNT_THRESHOLD)
			SSSNIC_CLEAR_NIC_DEV_FLAG(nic_dev, SSSNIC_RXQ_RECOVERY);
		goto free_rq_info;
	}

	for (qid = 0; qid < nic_dev->qp_res.qp_num; qid++) {
		rq_desc = &nic_dev->rq_desc_group[qid];
		if (!sss_nic_rq_is_normal(rq_desc, check_info[qid])) {
			rq_desc->check_err_cnt++;
			if (rq_desc->check_err_cnt < SSSNIC_RQ_CHECK_ERR_CNT_THRESHOLD)
				continue;

			if (rq_desc->print_err_cnt <= SSSNIC_RQ_PRINT_CNT_THRESHOLD) {
				nic_warn(nic_dev->dev_hdl,
					 "Rq handle: rq(%u) wqe abnormal, hw_pi:%u, hw_ci:%u, sw_pi:%u, sw_ci:%u delta:%u\n",
					 qid, check_info[qid].hw_pi, check_info[qid].hw_ci,
					 rq_desc->pi,
					 rq_desc->ci & rq_desc->qid_mask, rq_desc->delta);
				rq_desc->print_err_cnt++;
			}

			ret = sss_nic_reset_rq(nic_dev, qid, check_info[qid].hw_ci);
			if (ret)
				continue;
		}

		rq_desc->last_hw_ci = check_info[qid].hw_ci;
		rq_desc->rx_pkts = rq_desc->stats.rx_packets;
		rq_desc->last_sw_pi = rq_desc->pi;
		rq_desc->last_sw_ci = rq_desc->ci & rq_desc->qid_mask;
		rq_desc->print_err_cnt = 0;
		rq_desc->check_err_cnt = 0;
	}

	nic_dev->get_rq_fail_cnt = 0;

free_rq_info:
	kfree(check_info);
}
