// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

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

#include "hinic3_hw.h"
#include "hinic3_crm.h"
#include "hinic3_nic_io.h"
#include "hinic3_nic_dev.h"
#include "hinic3_tx.h"
#include "hinic3_rx.h"

int hinic3_poll(struct napi_struct *napi, int budget)
{
	int tx_pkts, rx_pkts;
	struct hinic3_irq *irq_cfg =
		container_of(napi, struct hinic3_irq, napi);
	struct hinic3_nic_dev *nic_dev = netdev_priv(irq_cfg->netdev);

	rx_pkts = hinic3_rx_poll(irq_cfg->rxq, budget);

	tx_pkts = hinic3_tx_poll(irq_cfg->txq, budget);
	if (tx_pkts >= budget || rx_pkts >= budget)
		return budget;

	napi_complete(napi);

	hinic3_set_msix_state(nic_dev->hwdev, irq_cfg->msix_entry_idx,
			      HINIC3_MSIX_ENABLE);

	return max(tx_pkts, rx_pkts);
}

static void qp_add_napi(struct hinic3_irq *irq_cfg)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(irq_cfg->netdev);

	netif_napi_add_weight(nic_dev->netdev, &irq_cfg->napi,
			      hinic3_poll, nic_dev->poll_weight);
	napi_enable(&irq_cfg->napi);
	irq_cfg->napi_reign = NAPI_IS_REGIN;
}

void qp_del_napi(struct hinic3_irq *irq_cfg)
{
	if (irq_cfg->napi_reign == NAPI_IS_REGIN) {
		napi_disable(&irq_cfg->napi);
		netif_napi_del(&irq_cfg->napi);
		irq_cfg->napi_reign = NAPI_NOT_REGIN;
	}
}

static irqreturn_t qp_irq(int irq, void *data)
{
	struct hinic3_irq *irq_cfg = (struct hinic3_irq *)data;
	struct hinic3_nic_dev *nic_dev = netdev_priv(irq_cfg->netdev);

	hinic3_misx_intr_clear_resend_bit(nic_dev->hwdev,
					  irq_cfg->msix_entry_idx, 1);

	napi_schedule(&irq_cfg->napi);

	return IRQ_HANDLED;
}

static int hinic3_request_irq(struct hinic3_irq *irq_cfg, u16 q_id)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(irq_cfg->netdev);
	struct interrupt_info info = {0};
	int err;

	qp_add_napi(irq_cfg);

	info.msix_index = irq_cfg->msix_entry_idx;
	info.lli_set = 0;
	info.interrupt_coalesc_set = 1;
	info.pending_limt = nic_dev->intr_coalesce[q_id].pending_limt;
	info.coalesc_timer_cfg =
		nic_dev->intr_coalesce[q_id].coalesce_timer_cfg;
	info.resend_timer_cfg = nic_dev->intr_coalesce[q_id].resend_timer_cfg;
	nic_dev->rxqs[q_id].last_coalesc_timer_cfg =
			nic_dev->intr_coalesce[q_id].coalesce_timer_cfg;
	nic_dev->rxqs[q_id].last_pending_limt =
			nic_dev->intr_coalesce[q_id].pending_limt;
	err = hinic3_set_interrupt_cfg(nic_dev->hwdev, info,
				       HINIC3_CHANNEL_NIC);
	if (err) {
		nicif_err(nic_dev, drv, irq_cfg->netdev,
			  "Failed to set RX interrupt coalescing attribute.\n");
		qp_del_napi(irq_cfg);
		return err;
	}

	err = request_irq(irq_cfg->irq_id, &qp_irq, 0,
			  irq_cfg->irq_name, irq_cfg);
	if (err) {
		nicif_err(nic_dev, drv, irq_cfg->netdev, "Failed to request Rx irq\n");
		qp_del_napi(irq_cfg);
		return err;
	}

	irq_set_affinity_hint(irq_cfg->irq_id, &irq_cfg->affinity_mask);

	return 0;
}

static void hinic3_release_irq(struct hinic3_irq *irq_cfg)
{
	irq_set_affinity_hint(irq_cfg->irq_id, NULL);
	synchronize_irq(irq_cfg->irq_id);
	free_irq(irq_cfg->irq_id, irq_cfg);
	qp_del_napi(irq_cfg);
}

int hinic3_qps_irq_init(struct hinic3_nic_dev *nic_dev)
{
	struct pci_dev *pdev = nic_dev->pdev;
	struct irq_info *qp_irq_info = NULL;
	struct hinic3_irq *irq_cfg = NULL;
	u16 q_id, i;
	u32 local_cpu;
	int err;

	for (q_id = 0; q_id < nic_dev->q_params.num_qps; q_id++) {
		qp_irq_info = &nic_dev->qps_irq_info[q_id];
		irq_cfg = &nic_dev->q_params.irq_cfg[q_id];

		irq_cfg->irq_id = qp_irq_info->irq_id;
		irq_cfg->msix_entry_idx = qp_irq_info->msix_entry_idx;
		irq_cfg->netdev = nic_dev->netdev;
		irq_cfg->txq = &nic_dev->txqs[q_id];
		irq_cfg->rxq = &nic_dev->rxqs[q_id];
		nic_dev->rxqs[q_id].irq_cfg = irq_cfg;

		local_cpu = cpumask_local_spread(q_id, dev_to_node(&pdev->dev));
		cpumask_set_cpu(local_cpu, &irq_cfg->affinity_mask);

		snprintf(irq_cfg->irq_name, sizeof(irq_cfg->irq_name),
			 "%s_qp%u", nic_dev->netdev->name, q_id);

		err = hinic3_request_irq(irq_cfg, q_id);
		if (err) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Failed to request Rx irq\n");
			goto req_tx_irq_err;
		}

		hinic3_set_msix_auto_mask_state(nic_dev->hwdev,
						irq_cfg->msix_entry_idx,
						HINIC3_SET_MSIX_AUTO_MASK);
		hinic3_set_msix_state(nic_dev->hwdev, irq_cfg->msix_entry_idx,
				      HINIC3_MSIX_ENABLE);
	}

	INIT_DELAYED_WORK(&nic_dev->moderation_task,
			  hinic3_auto_moderation_work);

	return 0;

req_tx_irq_err:
	for (i = 0; i < q_id; i++) {
		irq_cfg = &nic_dev->q_params.irq_cfg[i];
		hinic3_set_msix_state(nic_dev->hwdev, irq_cfg->msix_entry_idx,
				      HINIC3_MSIX_DISABLE);
		hinic3_set_msix_auto_mask_state(nic_dev->hwdev,
						irq_cfg->msix_entry_idx,
						HINIC3_CLR_MSIX_AUTO_MASK);
		hinic3_release_irq(irq_cfg);
	}

	return err;
}

void hinic3_qps_irq_deinit(struct hinic3_nic_dev *nic_dev)
{
	struct hinic3_irq *irq_cfg = NULL;
	u16 q_id;

	for (q_id = 0; q_id < nic_dev->q_params.num_qps; q_id++) {
		irq_cfg = &nic_dev->q_params.irq_cfg[q_id];
		hinic3_set_msix_state(nic_dev->hwdev, irq_cfg->msix_entry_idx,
				      HINIC3_MSIX_DISABLE);
		hinic3_set_msix_auto_mask_state(nic_dev->hwdev,
						irq_cfg->msix_entry_idx,
						HINIC3_CLR_MSIX_AUTO_MASK);
		hinic3_release_irq(irq_cfg);
	}
}
