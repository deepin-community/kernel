// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/debugfs.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_io.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_tx.h"
#include "sss_nic_rx.h"

#define SSSNIC_AVG_PKT_SMALL_SIZE      256U

static int sss_nic_napi_poll(struct napi_struct *napi, int budget)
{
	int tx_pkt;
	int rx_pkt;

	struct sss_nic_irq_cfg *nic_irq = container_of(napi, struct sss_nic_irq_cfg, napi);
	struct sss_nic_dev *nic_dev = netdev_priv(nic_irq->netdev);

	rx_pkt = sss_nic_rx_poll(nic_irq->rq, budget);
	tx_pkt = sss_nic_tx_poll(nic_irq->sq, budget);

	if (tx_pkt >= budget || rx_pkt >= budget)
		return budget;

	napi_complete(napi);

	sss_chip_set_msix_state(nic_dev->hwdev, nic_irq->msix_id,
				SSS_MSIX_ENABLE);

	return max(tx_pkt, rx_pkt);
}

static void sss_nic_add_napi(struct sss_nic_irq_cfg *nic_irq, int budget)
{
#ifdef NEED_NETIF_NAPI_ADD_NO_WEIGHT
	netif_napi_add_weight(nic_irq->netdev, &nic_irq->napi, sss_nic_napi_poll, budget);
#else
	netif_napi_add(nic_irq->netdev, &nic_irq->napi, sss_nic_napi_poll, budget);
#endif
	napi_enable(&nic_irq->napi);
}

static void sss_nic_del_napi(struct sss_nic_irq_cfg *nic_irq)
{
	napi_disable(&nic_irq->napi);
	netif_napi_del(&nic_irq->napi);
}

static irqreturn_t sss_nic_qp_irq(int irq, void *data)
{
	struct sss_nic_irq_cfg *nic_irq = (struct sss_nic_irq_cfg *)data;
	struct sss_nic_dev *nic_dev = netdev_priv(nic_irq->netdev);

	sss_chip_clear_msix_resend_bit(nic_dev->hwdev, nic_irq->msix_id, 1);

	napi_schedule(&nic_irq->napi);

	return IRQ_HANDLED;
}

static int sss_nic_request_irq(struct sss_nic_dev *nic_dev, u16 qid)
{
	int ret;
	struct sss_irq_cfg irq_cfg = {0};
	struct sss_nic_irq_cfg *nic_irq = &nic_dev->qp_res.irq_cfg[qid];

	sss_nic_add_napi(nic_irq, nic_dev->poll_budget);

	irq_cfg.coalesc_intr_set = 1;
	irq_cfg.msix_id = nic_irq->msix_id;
	irq_cfg.pending = nic_dev->coal_info[qid].pending_limt;
	irq_cfg.coalesc_timer =
		nic_dev->coal_info[qid].coalesce_timer;
	irq_cfg.resend_timer = nic_dev->coal_info[qid].resend_timer;
	nic_dev->rq_desc_group[qid].last_coal_timer =
		nic_dev->coal_info[qid].coalesce_timer;
	nic_dev->rq_desc_group[qid].last_pending_limt =
		nic_dev->coal_info[qid].pending_limt;
	ret = sss_chip_set_msix_attr(nic_dev->hwdev, irq_cfg, SSS_CHANNEL_NIC);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to set rx msix attr.\n");
		goto out;
	}

	ret = request_irq(nic_irq->irq_id, &sss_nic_qp_irq, 0, nic_irq->irq_name, nic_irq);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_irq->netdev, "Fail to request rx irq\n");
		goto out;
	}

	irq_set_affinity_hint(nic_irq->irq_id, &nic_irq->affinity_mask);

	return 0;

out:
	sss_nic_del_napi(nic_irq);
	return ret;
}

static void sss_nic_release_irq(struct sss_nic_irq_cfg *nic_irq)
{
	irq_set_affinity_hint(nic_irq->irq_id, NULL);
	synchronize_irq(nic_irq->irq_id);
	free_irq(nic_irq->irq_id, nic_irq);
	sss_nic_del_napi(nic_irq);
}

static int sss_nic_set_hw_coal(struct sss_nic_dev *nic_dev,
			       u16 qid, u8 coal_timer_cfg, u8 pending_limt)
{
	int ret;
	struct sss_irq_cfg cmd_irq_cfg = {0};

	cmd_irq_cfg.coalesc_intr_set = 1;
	cmd_irq_cfg.msix_id = nic_dev->qp_res.irq_cfg[qid].msix_id;
	cmd_irq_cfg.pending = pending_limt;
	cmd_irq_cfg.coalesc_timer = coal_timer_cfg;
	cmd_irq_cfg.resend_timer =
		nic_dev->coal_info[qid].resend_timer;

	ret = sss_chip_set_msix_attr(nic_dev->hwdev, cmd_irq_cfg, SSS_CHANNEL_NIC);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to modify moderation for Queue: %u\n", qid);
		return ret;
	}

	return 0;
}

static void sss_nic_calculate_intr_coal(struct sss_nic_intr_coal_info *coal_info,
					u64 rx_rate, u8 *coal_timer_cfg, u8 *pending_limt)
{
	if (rx_rate < coal_info->pkt_rate_low) {
		*pending_limt = coal_info->rx_pending_limt_low;
		*coal_timer_cfg = coal_info->rx_usecs_low;
	} else if (rx_rate > coal_info->pkt_rate_high) {
		*pending_limt = coal_info->rx_pending_limt_high;
		*coal_timer_cfg = coal_info->rx_usecs_high;
	} else {
		u8 rx_pending_limt = coal_info->rx_pending_limt_high -
				     coal_info->rx_pending_limt_low;
		u8 rx_usecs = coal_info->rx_usecs_high - coal_info->rx_usecs_low;
		u64 rx_rate_diff = rx_rate - coal_info->pkt_rate_low;
		u64 pkt_rate = coal_info->pkt_rate_high - coal_info->pkt_rate_low;

		*pending_limt = (u8)(rx_rate_diff * rx_pending_limt / pkt_rate +
				coal_info->rx_pending_limt_low);
		*coal_timer_cfg = (u8)(rx_rate_diff * rx_usecs / pkt_rate +
				coal_info->rx_usecs_low);
	}
}

static void sss_nic_update_intr_coal(struct sss_nic_dev *nic_dev,
				     u16 qid, u64 rx_rate, u64 tx_rate, u64 avg_pkt_size)
{
	u8 pending_limt;
	u8 coal_timer_cfg;
	struct sss_nic_intr_coal_info *coal_info = NULL;

	coal_info = &nic_dev->coal_info[qid];

	if (rx_rate > SSSNIC_RX_RATE_THRESH && avg_pkt_size > SSSNIC_AVG_PKT_SMALL_SIZE) {
		sss_nic_calculate_intr_coal(coal_info, rx_rate, &coal_timer_cfg, &pending_limt);
	} else {
		pending_limt = coal_info->rx_pending_limt_low;
		coal_timer_cfg = SSSNIC_LOWEST_LATENCY;
	}

	if (coal_timer_cfg == nic_dev->rq_desc_group[qid].last_coal_timer &&
	    pending_limt == nic_dev->rq_desc_group[qid].last_pending_limt)
		return;

	if (!SSS_CHANNEL_RES_VALID(nic_dev) || qid >= nic_dev->qp_res.qp_num)
		return;

	(void)sss_nic_set_hw_coal(nic_dev, qid, coal_timer_cfg, pending_limt);

	nic_dev->rq_desc_group[qid].last_pending_limt = pending_limt;
	nic_dev->rq_desc_group[qid].last_coal_timer = coal_timer_cfg;
}

static void sss_nic_adjust_coal_work(struct work_struct *work)
{
	u16 qid;
	u64 avg_pkt_size;
	u64 tx_pkts;
	u64 tx_rate;
	u64 rx_bytes;
	u64 rx_pkts;
	u64 rx_rate;
	struct delayed_work *delay = to_delayed_work(work);
	struct sss_nic_dev *nic_dev =
		container_of(delay, struct sss_nic_dev, moderation_task);
	unsigned long period;

	if (!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_INTF_UP))
		return;

	queue_delayed_work(nic_dev->workq, &nic_dev->moderation_task,
			   SSSNIC_MODERATONE_DELAY);
	period = (unsigned long)(jiffies - nic_dev->last_jiffies);

	if (nic_dev->use_adaptive_rx_coalesce == 0 || period == 0)
		return;

	for (qid = 0; qid < nic_dev->qp_res.qp_num; qid++) {
		rx_bytes = nic_dev->rq_desc_group[qid].stats.rx_bytes -
			   nic_dev->rq_desc_group[qid].last_rx_bytes;
		rx_pkts = nic_dev->rq_desc_group[qid].stats.rx_packets -
			  nic_dev->rq_desc_group[qid].last_rx_pkts;
		avg_pkt_size = (rx_pkts != 0) ? (rx_bytes / rx_pkts) : 0;
		rx_rate = rx_pkts * HZ / period;

		tx_pkts = nic_dev->sq_desc_group[qid].stats.tx_packets -
			  nic_dev->sq_desc_group[qid].last_tx_pkts;
		tx_rate = tx_pkts * HZ / period;

		nic_dev->rq_desc_group[qid].last_rx_bytes =
			nic_dev->rq_desc_group[qid].stats.rx_bytes;
		nic_dev->rq_desc_group[qid].last_rx_pkts =
			nic_dev->rq_desc_group[qid].stats.rx_packets;
		nic_dev->sq_desc_group[qid].last_tx_bytes =
			nic_dev->sq_desc_group[qid].stats.tx_bytes;
		nic_dev->sq_desc_group[qid].last_tx_pkts =
			nic_dev->sq_desc_group[qid].stats.tx_packets;

		sss_nic_update_intr_coal(nic_dev, qid, rx_rate, tx_rate, avg_pkt_size);
	}

	nic_dev->last_jiffies = jiffies;
}

static void sss_nic_dev_irq_cfg_init(struct sss_nic_dev *nic_dev, u16 qid)
{
	struct sss_irq_desc *irq_desc = &nic_dev->irq_desc_group[qid];
	struct sss_nic_irq_cfg *nic_irq = &nic_dev->qp_res.irq_cfg[qid];

	nic_irq->netdev = nic_dev->netdev;
	nic_irq->msix_id = irq_desc->msix_id;
	nic_irq->irq_id = irq_desc->irq_id;
	nic_irq->sq = &nic_dev->sq_desc_group[qid];
	nic_irq->rq = &nic_dev->rq_desc_group[qid];
	nic_dev->rq_desc_group[qid].irq_cfg = nic_irq;
}

static void __sss_nic_release_qp_irq(struct sss_nic_dev *nic_dev,
				     struct sss_nic_irq_cfg *nic_irq)
{
	sss_chip_set_msix_state(nic_dev->hwdev, nic_irq->msix_id, SSS_MSIX_DISABLE);
	sss_chip_set_msix_auto_mask(nic_dev->hwdev,
				    nic_irq->msix_id, SSS_CLR_MSIX_AUTO_MASK);
	sss_nic_release_irq(nic_irq);
}

int sss_nic_request_qp_irq(struct sss_nic_dev *nic_dev)
{
	u16 i;
	u16 qid;
	u32 cpuid;
	int ret;
	struct sss_nic_irq_cfg *nic_irq = NULL;

	for (qid = 0; qid < nic_dev->qp_res.qp_num; qid++) {
		nic_irq = &nic_dev->qp_res.irq_cfg[qid];
		sss_nic_dev_irq_cfg_init(nic_dev, qid);

		cpuid = cpumask_local_spread(qid, dev_to_node(nic_dev->dev_hdl));
		cpumask_set_cpu(cpuid, &nic_irq->affinity_mask);

		ret = snprintf(nic_irq->irq_name, sizeof(nic_irq->irq_name),
			       "%s_qp%u", nic_dev->netdev->name, qid);
		if (ret < 0) {
			ret = -EINVAL;
			goto out;
		}

		ret = sss_nic_request_irq(nic_dev, qid);
		if (ret != 0) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to request rx irq\n");
			goto out;
		}

		sss_chip_set_msix_auto_mask(nic_dev->hwdev, nic_irq->msix_id,
					    SSS_SET_MSIX_AUTO_MASK);
		sss_chip_set_msix_state(nic_dev->hwdev, nic_irq->msix_id,
					SSS_MSIX_ENABLE);
	}

	INIT_DELAYED_WORK(&nic_dev->moderation_task, sss_nic_adjust_coal_work);

	return 0;

out:
	for (i = 0; i < qid; i++)
		__sss_nic_release_qp_irq(nic_dev, &nic_dev->qp_res.irq_cfg[i]);

	return ret;
}

void sss_nic_release_qp_irq(struct sss_nic_dev *nic_dev)
{
	u16 qid;

	for (qid = 0; qid < nic_dev->qp_res.qp_num; qid++)
		__sss_nic_release_qp_irq(nic_dev, &nic_dev->qp_res.irq_cfg[qid]);
}
