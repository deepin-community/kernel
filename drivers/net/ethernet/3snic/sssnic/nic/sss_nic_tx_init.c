// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <net/xfrm.h>
#include <linux/netdevice.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/tcp.h>
#include <linux/sctp.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>
#include <linux/u64_stats_sync.h>
#include <linux/module.h>
#include <linux/vmalloc.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_io.h"
#include "sss_nic_cfg.h"
#include "sss_nic_vf_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_rss_cfg.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_tx.h"

#define SSSNIC_SQ_EXTRA_SGE						18

#define SSSNIC_FLUSH_SQ_TIMEOUT					1000

#define SSSNIC_STOP_SQ_WAIT_TIME_MIN			900
#define SSSNIC_STOP_SQ_WAIT_TIME_MAX			1000
#define SSSNIC_STOP_SQ_WAIT_TIME_FORCE_MIN		9900
#define SSSNIC_STOP_SQ_WAIT_TIME_FORCE_MAX		10000

#define SSSNIC_SQ_WQEBB_BD	(SSSNIC_SQ_WQEBB_SIZE / 16)

int sss_nic_alloc_sq_resource(struct sss_nic_dev *nic_dev,
			      struct sss_nic_qp_resource *qp_res)
{
	struct sss_nic_sq_resource *sq_res = NULL;
	int qid;
	int id;
	u64 bds_size;
	u64 len;

	for (qid = 0; qid < qp_res->qp_num; qid++) {
		sq_res = &qp_res->sq_res_group[qid];
		bds_size = sizeof(*sq_res->dma_group) *
			   (qp_res->sq_depth * SSSNIC_SQ_WQEBB_BD + SSSNIC_SQ_EXTRA_SGE);
		sq_res->dma_group = kzalloc(bds_size, GFP_KERNEL);
		if (!sq_res->dma_group) {
			nicif_err(nic_dev, drv, nic_dev->netdev,
				  "Fail to allocate sq %d dma info\n", qid);
			goto error;
		}

		len = sizeof(*sq_res->tx_desc_group) * qp_res->sq_depth;
		sq_res->tx_desc_group = kzalloc(len, GFP_KERNEL);
		if (!sq_res->tx_desc_group) {
			kfree(sq_res->dma_group);
			sq_res->dma_group = NULL;
			nicif_err(nic_dev, drv, nic_dev->netdev,
				  "Fail to alloc sq %d tx desc\n", qid);
			goto error;
		}
	}

	return 0;

error:
	for (id = 0; id < qid; id++) {
		sq_res = &qp_res->sq_res_group[id];
		kfree(sq_res->dma_group);
		kfree(sq_res->tx_desc_group);
		sq_res->dma_group = NULL;
		sq_res->tx_desc_group = NULL;
	}

	return -ENOMEM;
}

void sss_nic_free_sq_resource(struct sss_nic_dev *nic_dev,
			      struct sss_nic_qp_resource *qp_res)
{
	struct sss_nic_sq_resource *sq_res = NULL;
	u16 qid;

	for (qid = 0; qid < qp_res->qp_num; qid++) {
		sq_res = &qp_res->sq_res_group[qid];

		sss_nic_free_all_skb(nic_dev, qp_res->sq_depth, sq_res->tx_desc_group);
		kfree(sq_res->dma_group);
		kfree(sq_res->tx_desc_group);
		sq_res->dma_group = NULL;
		sq_res->tx_desc_group = NULL;
	}
}

void sss_nic_init_all_sq(struct sss_nic_dev *nic_dev,
			 struct sss_nic_qp_resource *qp_res)
{
	struct sss_nic_sq_resource *sq_res = NULL;
	struct sss_nic_sq_desc *sq_desc = NULL;
	u16 qid;
	u32 did;

	for (qid = 0; qid < qp_res->qp_num; qid++) {
		sq_desc = &nic_dev->sq_desc_group[qid];
		sq_res = &qp_res->sq_res_group[qid];

		sq_desc->q_depth = qp_res->sq_depth;
		sq_desc->qid_mask = qp_res->sq_depth - 1;

		sq_desc->tx_desc_group = sq_res->tx_desc_group;
		for (did = 0; did < qp_res->sq_depth; did++)
			sq_desc->tx_desc_group[did].dma_group =
				&sq_res->dma_group[did * SSSNIC_SQ_WQEBB_BD];

		sq_desc->sq = &nic_dev->nic_io->sq_group[qid];
	}
}

int sss_nic_alloc_sq_desc_group(struct sss_nic_dev *nic_dev)
{
	struct sss_nic_sq_desc *sq_desc = NULL;
	struct sss_nic_sq_stats *sq_stats = NULL;
	u16 sq_num = nic_dev->max_qp_num;
	u16 qid;

	nic_dev->sq_desc_group = kcalloc(sq_num, sizeof(*nic_dev->sq_desc_group), GFP_KERNEL);
	if (!nic_dev->sq_desc_group)
		return -ENOMEM;

	for (qid = 0; qid < sq_num; qid++) {
		sq_desc = &nic_dev->sq_desc_group[qid];
		sq_stats = &sq_desc->stats;
		sq_desc->qid = qid;
		sq_desc->dev = nic_dev->dev_hdl;
		sq_desc->netdev = nic_dev->netdev;
		sq_desc->qid_mask = nic_dev->qp_res.sq_depth - 1;
		sq_desc->q_depth = nic_dev->qp_res.sq_depth;
		u64_stats_init(&sq_stats->stats_sync);
	}

	return 0;
}

void sss_nic_free_sq_desc_group(struct sss_nic_dev *nic_dev)
{
	kfree(nic_dev->sq_desc_group);
	nic_dev->sq_desc_group = NULL;
}

static bool sss_nic_sq_is_null(struct sss_nic_io_queue *sq)
{
	u16 sw_pi = sss_nic_get_sq_local_pi(sq);
	u16 hw_ci = sss_nic_get_sq_hw_ci(sq);

	return sw_pi == hw_ci;
}

static int sss_nic_stop_sq(struct sss_nic_dev *nic_dev, u16 qid)
{
	int ret;
	unsigned long timeout;
	struct sss_nic_io_queue *sq = nic_dev->sq_desc_group[qid].sq;

	timeout = msecs_to_jiffies(SSSNIC_FLUSH_SQ_TIMEOUT) + jiffies;
	do {
		if (sss_nic_sq_is_null(sq))
			return 0;

		usleep_range(SSSNIC_STOP_SQ_WAIT_TIME_MIN, SSSNIC_STOP_SQ_WAIT_TIME_MAX);
	} while (time_before(jiffies, timeout));

	timeout = msecs_to_jiffies(SSSNIC_FLUSH_SQ_TIMEOUT) + jiffies;
	do {
		if (sss_nic_sq_is_null(sq))
			return 0;

		ret = sss_nic_force_drop_tx_pkt(nic_dev);
		if (ret != 0)
			break;

		usleep_range(SSSNIC_STOP_SQ_WAIT_TIME_FORCE_MIN,
			     SSSNIC_STOP_SQ_WAIT_TIME_FORCE_MAX);
	} while (time_before(jiffies, timeout));

	if (!sss_nic_sq_is_null(sq))
		return -EFAULT;

	return 0;
}

void sss_nic_flush_all_sq(struct sss_nic_dev *nic_dev)
{
	u16 qid = 0;
	int ret = 0;

	for (qid = 0; qid < nic_dev->qp_res.qp_num; qid++) {
		ret = sss_nic_stop_sq(nic_dev, qid);
		if (ret != 0)
			nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to stop sq%u\n", qid);
	}
}
