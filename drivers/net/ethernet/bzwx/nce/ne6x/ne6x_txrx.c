// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include "ne6x.h"
#include "ne6x_txrx.h"
#include "ne6x_reg.h"

int ne6x_adpt_setup_tx_resources(struct ne6x_adapter *adpt)
{
	int i, err = 0;

	for (i = 0; i < adpt->num_queue && !err; i++) {
		err = ne6x_setup_tx_descriptors(adpt->tx_rings[i]);
		err = ne6x_setup_tg_descriptors(adpt->tg_rings[i]);
		err = ne6x_setup_cq_descriptors(adpt->cq_rings[i]);
		err = ne6x_setup_tx_sgl(adpt->tx_rings[i]);
	}

	return err;
}

int ne6x_adpt_setup_rx_resources(struct ne6x_adapter *adpt)
{
	int i, err = 0;

	for (i = 0; i < adpt->num_queue && !err; i++)
		err = ne6x_setup_rx_descriptors(adpt->rx_rings[i]);

	return err;
}

static inline void ne6x_update_enable_itr(struct ne6x_q_vector *q_vector)
{
	struct ne6x_adapter *adpt = (struct ne6x_adapter *)q_vector->adpt;
	struct ne6x_hw *hw = &adpt->back->hw;

	u64 val = 1ULL << NE6X_VP_CQ_INTSHIFT;

	if (!test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
		struct ne6x_ring *cq_ring = NULL;

		cq_ring = q_vector->cq.ring;
		if (cq_ring->next_to_clean != cq_ring->next_to_use) {
			cq_ring->next_to_clean = cq_ring->next_to_use;
			/* memory barrier updating cq ring tail */
			wmb();
			writeq(cq_ring->next_to_clean, cq_ring->tail);
		}

		if (q_vector->reg_idx < NE6X_PF_VP0_NUM) {
			wr64(hw, NE6X_VPINT_DYN_CTLN(q_vector->reg_idx, NE6X_VP_INT), val);
			wr64(hw, NE6X_VPINT_DYN_CTLN(q_vector->reg_idx, NE6X_VP_INT_MASK), ~(val));
		} else {
			wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(q_vector->reg_idx - NE6X_PF_VP0_NUM,
							  NE6X_VP_INT), val);
			wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(q_vector->reg_idx - NE6X_PF_VP0_NUM,
							  NE6X_VP_INT_MASK), ~(val));
		}
	}
}

int ne6x_napi_poll(struct napi_struct *napi, int budget)
{
	struct ne6x_q_vector *q_vector = container_of(napi, struct ne6x_q_vector, napi);
	struct ne6x_adapt_comm *comm = (struct ne6x_adapt_comm *)q_vector->adpt;
	struct ne6x_ring *ring = NULL;
	bool clean_complete = true;
	int cq_budget = 16;
	int work_done = 0;
	int cleaned = 0;

	if (test_bit(NE6X_ADPT_DOWN, comm->state)) {
		napi_complete(napi);
		return 0;
	}

	ring = q_vector->cq.ring;
	cleaned = ne6x_clean_cq_irq(q_vector, ring, cq_budget);
	if (cleaned >= cq_budget)
		clean_complete = false;

	ring = q_vector->tx.ring;
	if (!ne6x_clean_tx_irq(comm, ring, budget))
		clean_complete = false;

	/* Handle case where we are called by netpoll with a budget of 0 */
	if (budget <= 0)
		goto tx_only;

	ring = q_vector->rx.ring;
	cleaned = ne6x_clean_rx_irq(ring, budget);
	if (cleaned >= budget)
		clean_complete = false;

	work_done += cleaned;

	/* If work not completed, return budget and polling will return */
	if (!clean_complete) {
		int cpu_id = smp_processor_id();

		/* It is possible that the interrupt affinity has changed but,
		 * if the cpu is pegged at 100%, polling will never exit while
		 * traffic continues and the interrupt will be stuck on this
		 * cpu.  We check to make sure affinity is correct before we
		 * continue to poll, otherwise we must stop polling so the
		 * interrupt can move to the correct cpu.
		 */
		if (!cpumask_test_cpu(cpu_id, &q_vector->affinity_mask)) {
			/* Tell napi that we are done polling */
			napi_complete_done(napi, work_done);
			ne6x_update_enable_itr(q_vector);
			/* Return budget-1 so that polling stops */
			return budget - 1;
		}
tx_only:
		return budget;
	}

	/* Work is done so exit the polling mode and re-enable the interrupt */
	napi_complete_done(napi, work_done);
	ne6x_update_enable_itr(q_vector);

	return min(work_done, budget - 1);
}

void ne6x_adpt_clear_rings(struct ne6x_adapter *adpt)
{
	int i;

	if (adpt->tx_rings && adpt->tx_rings[0]) {
		for (i = 0; i < adpt->num_queue; i++) {
			kfree_rcu(adpt->tx_rings[i], rcu);
			adpt->tx_rings[i] = NULL;
			adpt->rx_rings[i] = NULL;
			adpt->cq_rings[i] = NULL;
		}
	}
}

int ne6x_alloc_rings(struct ne6x_adapter *adpt)
{
	struct ne6x_pf *pf = adpt->back;
	struct ne6x_ring *ring;
	int i, qpv = 4;

	/* Set basic values in the rings to be used later during open() */
	for (i = 0; i < adpt->num_queue; i++) {
		/* allocate space for both Tx and Rx in one shot */
		ring = kcalloc(qpv, sizeof(*ring), GFP_KERNEL);
		if (!ring)
			goto err_out;

		ring->queue_index = i;
		ring->reg_idx = adpt->base_queue + i;
		ring->netdev = adpt->netdev;
		ring->dev = &pf->pdev->dev;
		ring->adpt = adpt;
		ring->count = adpt->num_tx_desc;
		ring->size = 0;
		adpt->tx_rings[i] = ring++;

		ring->queue_index = i;
		ring->reg_idx = adpt->base_queue + i;
		ring->netdev = adpt->netdev;
		ring->dev = &pf->pdev->dev;
		ring->adpt = adpt;
		ring->count = adpt->num_cq_desc;
		ring->size = 0;
		adpt->cq_rings[i] = ring++;

		ring->queue_index = i;
		ring->reg_idx = adpt->base_queue + i;
		ring->netdev = adpt->netdev;
		ring->dev = &pf->pdev->dev;
		ring->adpt = adpt;
		ring->count = adpt->num_rx_desc;
		ring->size = 0;
		adpt->rx_rings[i] = ring++;

		ring->queue_index = i;
		ring->reg_idx = adpt->base_queue + i;
		ring->netdev = adpt->netdev;
		ring->dev = &pf->pdev->dev;
		ring->adpt = adpt;
		ring->count = adpt->num_tg_desc;
		ring->size = 0;
		adpt->tg_rings[i] = ring;
	}

	return 0;

err_out:
	ne6x_adpt_clear_rings(adpt);
	return -ENOMEM;
}

static int ne6x_configure_tx_ring(struct ne6x_ring *ring)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(ring->netdev);
	u16 pf_q = adpt->base_queue + ring->queue_index;
	union ne6x_sq_base_addr sq_base_addr;
	struct ne6x_hw *hw = &adpt->back->hw;
	union ne6x_sq_cfg sq_cfg;

	/* SRIOV mode VF Config OR SRIOV disabled PF Config */
	if (pf_q < NE6X_PF_VP0_NUM) {
		sq_base_addr.val = rd64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_SQ_BASE_ADDR));
		sq_base_addr.reg.csr_sq_base_addr_vp = ring->dma;
		wr64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_SQ_BASE_ADDR), sq_base_addr.val);

		sq_cfg.val = rd64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_SQ_CFG));
		sq_cfg.reg.csr_sq_len_vp = ring->count;
		sq_cfg.reg.csr_tdq_pull_en = 0x1;
		sq_cfg.reg.csr_sqevt_write_back_vp = 0x0;
		sq_cfg.reg.csr_send_pd_revers_en = 0x0;
		wr64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_SQ_CFG), sq_cfg.val);

		wr64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_SQ_HD_POINTER), 0x0);

		/* cache tail off for easier writes later */
		ring->tail = (u64 *)&((u64 *)hw->hw_addr2)[NE6X_BAR2_VP_TDQ(pf_q, 0x0) >> 3];
	} else {
		/* SRIOV mode PF Config */
		sq_base_addr.val = rd64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM,
								     NE6X_SQ_BASE_ADDR));
		sq_base_addr.reg.csr_sq_base_addr_vp = ring->dma;
		wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM,
						  NE6X_SQ_BASE_ADDR),
			  sq_base_addr.val);

		sq_cfg.val =
			rd64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM,
							  NE6X_SQ_CFG));
		sq_cfg.reg.csr_sq_len_vp = ring->count;
		sq_cfg.reg.csr_tdq_pull_en = 0x1;
		sq_cfg.reg.csr_sqevt_write_back_vp = 0x0;
		sq_cfg.reg.csr_send_pd_revers_en = 0x0;
		wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM, NE6X_SQ_CFG), sq_cfg.val);

		wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM, NE6X_SQ_HD_POINTER), 0x0);

		/* cache tail off for easier writes later */
		ring->tail = (u64 *)&((u64 *)hw->hw_addr2)[NE6X_BAR2_VP_TDQ(pf_q, 0x0) >> 3];
	}

	return 0;
}

int ne6x_adpt_configure_tx(struct ne6x_adapter *adpt)
{
	int err = 0;
	u16 i;

	for (i = 0; (i < adpt->num_queue) && !err; i++)
		err = ne6x_configure_tx_ring(adpt->tx_rings[i]);

	return err;
}

static int ne6x_configure_cq_ring(struct ne6x_ring *ring)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(ring->netdev);
	u16 pf_q = adpt->base_queue + ring->queue_index;
	union ne6x_cq_base_addr cq_base_addr;
	struct ne6x_hw *hw = &adpt->back->hw;
	union ne6x_cq_cfg cq_cfg;

	/* SRIOV enabled VF config OR SRIOV disabled PF config */
	if (pf_q < NE6X_PF_VP0_NUM) {
		cq_base_addr.val = rd64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_CQ_BASE_ADDR));
		cq_base_addr.reg.csr_cq_base_addr_vp = ring->dma;
		wr64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_CQ_BASE_ADDR), cq_base_addr.val);

		cq_cfg.val = rd64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_CQ_CFG));
		cq_cfg.reg.csr_cq_len_vp = ring->count;
		cq_cfg.reg.csr_cq_merge_time_vp = 7;
		cq_cfg.reg.csr_cq_merge_size_vp = 7;
		wr64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_CQ_CFG), cq_cfg.val);

		wr64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_CQ_TAIL_POINTER), 0x0);

		/* cache tail for quicker writes, and clear the reg before use */
		ring->tail = (void __iomem *)hw->hw_addr0 +
			     (NE6X_VPINT_DYN_CTLN(pf_q, NE6X_CQ_HD_POINTER));
		writeq(0, ring->tail);
	} else {
		/* SRIOV enable PF config */
		cq_base_addr.val = rd64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM,
								     NE6X_CQ_BASE_ADDR));
		cq_base_addr.reg.csr_cq_base_addr_vp = ring->dma;
		wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM, NE6X_CQ_BASE_ADDR),
			  cq_base_addr.val);

		cq_cfg.val = rd64_bar4(hw,
				       NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM, NE6X_CQ_CFG));
		cq_cfg.reg.csr_cq_len_vp = ring->count;
		cq_cfg.reg.csr_cq_merge_time_vp = 7;
		cq_cfg.reg.csr_cq_merge_size_vp = 7;
		wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM, NE6X_CQ_CFG),
			  cq_cfg.val);

		wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM,
						  NE6X_CQ_TAIL_POINTER), 0x0);

		/* cache tail for quicker writes, and clear the reg before use */
		ring->tail = (void __iomem *)hw->hw_addr4 +
			     (NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM, NE6X_CQ_HD_POINTER));
		writeq(0, ring->tail);
	}

	return 0;
}

int ne6x_adpt_configure_cq(struct ne6x_adapter *adpt)
{
	int err = 0;
	u16 i;
	/* set up individual rings */
	for (i = 0; i < adpt->num_queue && !err; i++)
		err = ne6x_configure_cq_ring(adpt->cq_rings[i]);

	return 0;
}

static int ne6x_configure_rx_ring(struct ne6x_ring *ring)
{
	struct ne6x_adapter *adpt = ne6x_netdev_to_adpt(ring->netdev);
	u16 pf_q = adpt->base_queue + ring->queue_index;
	union ne6x_rq_block_cfg rq_block_cfg;
	union ne6x_rq_base_addr rq_base_addr;
	struct ne6x_hw *hw = &adpt->back->hw;
	union ne6x_rq_cfg rc_cfg;
	u16 rxmax = 0;

	ring->rx_buf_len = adpt->rx_buf_len;

	if (pf_q < NE6X_PF_VP0_NUM) {
		rq_base_addr.val = rd64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_RQ_BASE_ADDR));
		rq_base_addr.reg.csr_rq_base_addr_vp = ring->dma;
		wr64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_RQ_BASE_ADDR), rq_base_addr.val);

		rxmax = min_t(u16, adpt->max_frame, ring->rx_buf_len);
		rq_block_cfg.val = rd64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_RQ_BLOCK_CFG));
		rq_block_cfg.reg.csr_rdq_mop_len = rxmax;
		rq_block_cfg.reg.csr_rdq_sop_len = 0;
		wr64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_RQ_BLOCK_CFG), rq_block_cfg.val);

		rc_cfg.val = rd64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_RQ_CFG));
		rc_cfg.reg.csr_rq_len_vp = ring->count;
		rc_cfg.reg.csr_rdq_pull_en = 0x1;
		rc_cfg.reg.csr_rqevt_write_back_vp = 0x0;
		rc_cfg.reg.csr_recv_pd_type_vp = 0x0;
		rc_cfg.reg.csr_recv_pd_revers_en = 0x0;
		wr64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_RQ_CFG), rc_cfg.val);

		wr64(hw, NE6X_VPINT_DYN_CTLN(pf_q, NE6X_RQ_HD_POINTER), 0x0);

		/* cache tail for quicker writes, and clear the reg before use */
		ring->tail = (u64 *)&((u64 *)hw->hw_addr2)[NE6X_BAR2_VP_RDQ(pf_q, 0x0) >> 3];
	} else {
		/* SRIOV enabled PF Config */
		rq_base_addr.val = rd64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM,
								     NE6X_RQ_BASE_ADDR));
		rq_base_addr.reg.csr_rq_base_addr_vp = ring->dma;
		wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM, NE6X_RQ_BASE_ADDR),
			  rq_base_addr.val);

		rxmax = min_t(u16, adpt->max_frame, ring->rx_buf_len);
		rq_block_cfg.val = rd64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM,
								     NE6X_RQ_BLOCK_CFG));
		rq_block_cfg.reg.csr_rdq_mop_len = rxmax;
		rq_block_cfg.reg.csr_rdq_sop_len = 0;
		wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM,
						  NE6X_RQ_BLOCK_CFG),
			  rq_block_cfg.val);

		rc_cfg.val =
			rd64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM,
							  NE6X_RQ_CFG));
		rc_cfg.reg.csr_rq_len_vp = ring->count;
		rc_cfg.reg.csr_rdq_pull_en = 0x1;
		rc_cfg.reg.csr_rqevt_write_back_vp = 0x0;
		rc_cfg.reg.csr_recv_pd_type_vp = 0x0;
		rc_cfg.reg.csr_recv_pd_revers_en = 0x0;
		wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM, NE6X_RQ_CFG), rc_cfg.val);

		wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(pf_q - NE6X_PF_VP0_NUM, NE6X_RQ_HD_POINTER), 0x0);

		/* cache tail for quicker writes, and clear the reg before use */
		ring->tail = (u64 *)&((u64 *)hw->hw_addr2)[NE6X_BAR2_VP_RDQ(pf_q, 0x0) >> 3];
	}

	return 0;
}

int ne6x_adpt_configure_rx(struct ne6x_adapter *adpt)
{
	int err = 0;
	u16 i;

	adpt->max_frame = NE6X_MAX_RXBUFFER;
	adpt->rx_buf_len = (PAGE_SIZE < 8192) ? NE6X_RXBUFFER_4096 : NE6X_RXBUFFER_4096;

	/* set up individual rings */
	for (i = 0; i < adpt->num_queue && !err; i++)
		err = ne6x_configure_rx_ring(adpt->rx_rings[i]);

	return err;
}

netdev_tx_t ne6x_lan_xmit_frame(struct sk_buff *skb, struct net_device *netdev)
{
	struct ne6x_netdev_priv *np = netdev_priv(netdev);
	struct ne6x_adapter *adpt = np->adpt;
	struct ne6x_ring *tx_ring = adpt->tx_rings[skb->queue_mapping];
	struct ne6x_ring *tag_ring = adpt->tg_rings[skb->queue_mapping];
	struct sk_buff *trailer;
	int tailen = 4;
	int nsg;
	bool jumbo_frame = true;

	/* hardware can't handle really short frames, hardware padding works
	 * beyond this point
	 */
	if (skb_put_padto(skb, NE6X_MIN_TX_LEN))
		return NETDEV_TX_OK;

	/* single packet add 4 byte to CRC */
	if (skb->len < NE6X_MAX_DATA_PER_TXD) {
		nsg = skb_cow_data(skb, tailen, &trailer);
		if (unlikely(nsg < 0)) {
			netdev_err(adpt->netdev, "TX: skb_cow_data() returned %d\n", nsg);
			return nsg;
		}

		pskb_put(skb, trailer, tailen);
		jumbo_frame = false;
	}

	if (netdev->gso_max_size < skb->len)
		netdev_err(adpt->netdev, "%s: skb->len = %d > 15360\n", __func__, skb->len);

	return ne6x_xmit_frame_ring(skb, tx_ring, tag_ring, jumbo_frame);
}
