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

static void sss_nic_rx_free_dma_page(struct sss_nic_dev *nic_dev,
				     struct sss_nic_rx_desc *rx_desc)
{
	if (rx_desc->buf_daddr != 0) {
		dma_unmap_page(nic_dev->dev_hdl, rx_desc->buf_daddr,
			       nic_dev->rx_dma_buff_size, DMA_FROM_DEVICE);
		rx_desc->buf_daddr = 0;
	}

	if (rx_desc->page) {
		__free_pages(rx_desc->page, nic_dev->page_order);
		rx_desc->page = NULL;
	}
}

static u32 sss_nic_rx_alloc_dma_buffer(struct sss_nic_dev *nic_dev,
				       u32 rq_depth, struct sss_nic_rx_desc *rx_desc_group)
{
	u32 i;

	for (i = 0; i < rq_depth - 1; i++)
		if (!sss_nic_rx_alloc_dma_page(nic_dev, &rx_desc_group[i]))
			break;

	return i;
}

static void sss_nic_rx_free_dma_buffer(struct sss_nic_dev *nic_dev,
				       u32 rq_depth, struct sss_nic_rx_desc *rx_desc_group)
{
	u32 id;

	for (id = 0; id < rq_depth; id++)
		sss_nic_rx_free_dma_page(nic_dev, &rx_desc_group[id]);
}

static void _sss_nic_free_rq_resource(struct sss_nic_dev *nic_dev,
				      struct sss_nic_rq_resource *rq_res, u32 rq_depth)
{
	u64 size = sizeof(struct sss_nic_cqe) * rq_depth;

	sss_nic_rx_free_dma_buffer(nic_dev, rq_depth, rq_res->rx_desc_group);
	dma_free_coherent(nic_dev->dev_hdl, size, rq_res->cqe_vaddr, rq_res->cqe_paddr);
	kfree(rq_res->rx_desc_group);
	rq_res->cqe_vaddr = NULL;
	rq_res->rx_desc_group = NULL;
}

int sss_nic_alloc_rq_res_group(struct sss_nic_dev *nic_dev,
			       struct sss_nic_qp_resource *qp_res)
{
	int i;
	int id;
	u32 page_num;
	u64 size;
	u64 cqe_dma_size = sizeof(struct sss_nic_cqe) * qp_res->rq_depth;
	struct sss_nic_rq_resource *rq_res = NULL;

	for (id = 0; id < qp_res->qp_num; id++) {
		rq_res = &qp_res->rq_res_group[id];
		rq_res->cqe_vaddr = dma_zalloc_coherent(nic_dev->dev_hdl, cqe_dma_size,
							&rq_res->cqe_paddr, GFP_KERNEL);
		if (!rq_res->cqe_vaddr) {
			nicif_err(nic_dev, drv, nic_dev->netdev,
				  "Fail to alloc cqe dma buf, rq%d\n", id);
			goto alloc_cqe_dma_err;
		}

		size = sizeof(*rq_res->rx_desc_group) * qp_res->rq_depth;
		rq_res->rx_desc_group = kzalloc(size, GFP_KERNEL);
		if (!rq_res->rx_desc_group) {
			nicif_err(nic_dev, drv, nic_dev->netdev,
				  "Fail to alloc rx info, rq%d\n", id);
			goto alloc_rqe_desc_group_err;
		}

		page_num = sss_nic_rx_alloc_dma_buffer(nic_dev, qp_res->rq_depth,
						       rq_res->rx_desc_group);
		if (page_num == 0) {
			nicif_err(nic_dev, drv, nic_dev->netdev,
				  "Fail to alloc rx buffer, rq%d\n", id);
			goto alloc_rx_buf_err;
		}
		rq_res->page_num = (u16)page_num;
	}
	return 0;

alloc_rx_buf_err:
	kfree(rq_res->rx_desc_group);
	rq_res->rx_desc_group = NULL;

alloc_rqe_desc_group_err:
	dma_free_coherent(nic_dev->dev_hdl, cqe_dma_size, rq_res->cqe_vaddr,
			  rq_res->cqe_paddr);
	rq_res->cqe_vaddr = NULL;

alloc_cqe_dma_err:
	for (i = 0; i < id; i++)
		_sss_nic_free_rq_resource(nic_dev, &qp_res->rq_res_group[i],
					  qp_res->rq_depth);

	return -ENOMEM;
}

void sss_nic_free_rq_res_group(struct sss_nic_dev *nic_dev,
			       struct sss_nic_qp_resource *qp_res)
{
	int id;

	for (id = 0; id < qp_res->qp_num; id++)
		_sss_nic_free_rq_resource(nic_dev, &qp_res->rq_res_group[id],
					  qp_res->rq_depth);
}

static void sss_nic_init_rq_desc(struct sss_nic_rq_desc *rq_desc,
				 struct sss_nic_qp_resource *qp_res,
				 struct sss_nic_rq_resource *rq_res,
				 struct sss_irq_desc *irq_desc)
{
	u32 id;
	dma_addr_t dma_addr;
	struct sss_nic_cqe *rq_cqe;

	rq_desc->irq_id = irq_desc->irq_id;
	rq_desc->msix_id = irq_desc->msix_id;
	rq_desc->pi = 0;
	rq_desc->backup_pi = rq_res->page_num;
	rq_desc->q_depth = qp_res->rq_depth;
	rq_desc->delta = rq_desc->q_depth;
	rq_desc->qid_mask = rq_desc->q_depth - 1;
	rq_desc->ci = 0;
	rq_desc->last_sw_pi = rq_desc->q_depth - 1;
	rq_desc->last_sw_ci = 0;
	rq_desc->last_hw_ci = 0;
	rq_desc->check_err_cnt = 0;
	rq_desc->print_err_cnt = 0;
	rq_desc->rx_pkts = 0;
	rq_desc->reset_wqe_num = 0;
	rq_desc->rx_desc_group = rq_res->rx_desc_group;

	dma_addr = rq_res->cqe_paddr;
	rq_cqe = (struct sss_nic_cqe *)rq_res->cqe_vaddr;
	for (id = 0; id < qp_res->rq_depth; id++) {
		rq_desc->rx_desc_group[id].cqe = rq_cqe;
		rq_desc->rx_desc_group[id].cqe_daddr = dma_addr;
		dma_addr += sizeof(*rq_desc->rx_desc_group[id].cqe);
		rq_cqe++;
	}
}

static void sss_nic_fill_cqe_sge(struct sss_nic_rq_desc *rq_desc)
{
	struct net_device *netdev = rq_desc->netdev;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	struct sss_nic_rx_desc *rx_desc = NULL;
	struct sss_nic_rqe *rqe = NULL;
	u32 i;

	for (i = 0; i < rq_desc->q_depth; i++) {
		rx_desc = &rq_desc->rx_desc_group[i];
		rqe = sss_wq_wqebb_addr(&rq_desc->rq->wq, (u16)i);

		if (rq_desc->rq->wqe_type == SSSNIC_EXTEND_RQ_WQE) {
			sss_set_sge(&rqe->extend_rqe.cqe_sect.sge, rx_desc->cqe_daddr,
				    (sizeof(struct sss_nic_cqe) >> SSSNIC_CQE_SIZE_SHIFT));

			rqe->extend_rqe.bd_sect.sge.len = nic_dev->rx_buff_len;
		} else {
			rqe->normal_rqe.cqe_lo_addr = lower_32_bits(rx_desc->cqe_daddr);
			rqe->normal_rqe.cqe_hi_addr = upper_32_bits(rx_desc->cqe_daddr);
		}

		rx_desc->rqe = rqe;
	}
}

int sss_nic_init_rq_desc_group(struct sss_nic_dev *nic_dev,
			       struct sss_nic_qp_resource *qp_res)
{
	struct sss_nic_rq_desc *rq_desc = NULL;
	u16 qid;
	u32 pkt;

	nic_dev->get_rq_fail_cnt = 0;
	for (qid = 0; qid < qp_res->qp_num; qid++) {
		rq_desc = &nic_dev->rq_desc_group[qid];
		rq_desc->rq = &nic_dev->nic_io->rq_group[rq_desc->qid];

		sss_nic_init_rq_desc(rq_desc, qp_res, &qp_res->rq_res_group[qid],
				     &nic_dev->irq_desc_group[qid]);

		sss_nic_fill_cqe_sge(rq_desc);

		pkt = sss_nic_fill_bd_sge(rq_desc);
		if (pkt == 0) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to fill rx buffer\n");
			return -ENOMEM;
		}
	}

	return 0;
}

void sss_nic_free_rq_desc_group(struct sss_nic_dev *nic_dev)
{
	kfree(nic_dev->rq_desc_group);
	nic_dev->rq_desc_group = NULL;
}

int sss_nic_alloc_rq_desc_group(struct sss_nic_dev *nic_dev)
{
	struct sss_nic_rq_desc *rq_desc = NULL;
	u16 rq_num = nic_dev->max_qp_num;
	u16 i;

	nic_dev->rq_desc_group = kcalloc(rq_num, sizeof(*nic_dev->rq_desc_group), GFP_KERNEL);
	if (!nic_dev->rq_desc_group)
		return -ENOMEM;

	for (i = 0; i < rq_num; i++) {
		rq_desc = &nic_dev->rq_desc_group[i];
		rq_desc->dev = nic_dev->dev_hdl;
		rq_desc->netdev = nic_dev->netdev;
		rq_desc->qid = i;
		rq_desc->qid_mask = nic_dev->qp_res.rq_depth - 1;
		rq_desc->q_depth = nic_dev->qp_res.rq_depth;
		rq_desc->dma_buff_size = nic_dev->rx_dma_buff_size;
		rq_desc->buff_size_shift = (u32)ilog2(nic_dev->rx_buff_len);
		rq_desc->buf_len = nic_dev->rx_buff_len;
		u64_stats_init(&rq_desc->stats.stats_sync);
	}

	return 0;
}

int sss_nic_update_rx_rss(struct sss_nic_dev *nic_dev)
{
	int ret;

	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_RSS_ENABLE)) {
		ret = sss_nic_update_rss_cfg(nic_dev);
		if (ret != 0) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to init rss\n");
			return -EFAULT;
		}
	}

	return 0;
}

void sss_nic_reset_rx_rss(struct net_device *netdev)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (test_bit(SSSNIC_RSS_ENABLE, &nic_dev->flags) != 0)
		sss_nic_reset_rss_cfg(nic_dev);
}
