// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include "ne6xvf.h"
#include "ne6xvf_txrx.h"

/**
 * ne6xvf_update_enable_itr - Update itr and re-enable MSIX interrupt
 * @vsi: the VSI we care about
 * @q_vector: q_vector for which itr is being updated and interrupt enabled
 *
 **/
static inline void ne6xvf_update_enable_itr(struct ne6x_q_vector *q_vector)
{
	struct ne6xvf_adapter *adpt = (struct ne6xvf_adapter *)q_vector->adpt;
	struct ne6xvf_hw *hw = &adpt->hw;

	if (!test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
		struct ne6x_ring *cq_ring = NULL;

		cq_ring = q_vector->cq.ring;
		if (cq_ring->next_to_clean != cq_ring->next_to_use) {
			cq_ring->next_to_clean = cq_ring->next_to_use;
			/* memory barrier updating cq ring tail */
			wmb();
			writeq(cq_ring->next_to_clean, cq_ring->tail);
		}

		wr64(hw, NE6XVF_REG_ADDR(q_vector->reg_idx, NE6X_VP_INT),
		     (1ULL << NE6X_VP_CQ_INTSHIFT));
		wr64(hw, NE6XVF_REG_ADDR(q_vector->reg_idx, NE6X_VP_INT_MASK),
		     ~(1ULL << NE6X_VP_CQ_INTSHIFT));
	}
}

/**
 * ne6xvf_unmap_and_free_tx_resource - Release a Tx buffer
 * @ring:      the ring that owns the buffer
 * @tx_buffer: the buffer to free
 **/
void ne6xvf_unmap_and_free_tx_resource(struct ne6x_ring *ring, struct ne6x_tx_buf *tx_buffer)
{
	if (tx_buffer->skb) {
		dev_kfree_skb_any(tx_buffer->skb);
		if (dma_unmap_len(tx_buffer, len))
			dma_unmap_single(ring->dev, dma_unmap_addr(tx_buffer, dma),
					 dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
	} else if (dma_unmap_len(tx_buffer, len)) {
		dma_unmap_page(ring->dev, dma_unmap_addr(tx_buffer, dma),
			       dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
	}
	tx_buffer->next_to_watch = NULL;
	tx_buffer->skb = NULL;
	dma_unmap_len_set(tx_buffer, len, 0);
}

/**
 * ne6xvf_napi_poll - NAPI polling Rx/Tx cleanup routine
 * @napi: napi struct with our devices info in it
 * @budget: amount of work driver is allowed to do this pass, in packets
 *
 * This function will clean all queues associated with a q_vector.
 *
 * Returns the amount of work done
 **/
int ne6xvf_napi_poll(struct napi_struct *napi, int budget)
{
	struct ne6x_q_vector *q_vector = container_of(napi, struct ne6x_q_vector, napi);
	struct ne6x_adapt_comm *comm = (struct ne6x_adapt_comm *)q_vector->adpt;
	struct ne6x_ring *ring = NULL;
	bool clean_complete = true;
	int cq_budget = 16;
	int work_done = 0;
	int cleaned = 0;

	ring = q_vector->cq.ring;

	if (test_bit(NE6X_ADPT_DOWN, comm->state)) {
		napi_complete(napi);
		return 0;
	}

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
			ne6xvf_update_enable_itr(q_vector);
			/* Return budget-1 so that polling stops */
			return budget - 1;
		}
tx_only:
		return budget;
	}

	/* Work is done so exit the polling mode and re-enable the interrupt */
	napi_complete_done(napi, work_done);
	ne6xvf_update_enable_itr(q_vector);

	return min(work_done, budget - 1);
}

netdev_tx_t ne6xvf_lan_xmit_frame(struct sk_buff *skb, struct net_device *netdev)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	struct ne6x_ring *tx_ring = &adapter->tx_rings[skb->queue_mapping];
	struct ne6x_ring *tag_ring = &adapter->tg_rings[skb->queue_mapping];
	struct sk_buff *trailer;
	int tailen, nsg;
	bool jumbo_frame = true;

	tailen = 4;

	if (skb_put_padto(skb, NE6X_MIN_TX_LEN))
		return NETDEV_TX_OK;

	if (skb->len < NE6X_MAX_DATA_PER_TXD) {
		nsg = skb_cow_data(skb, tailen, &trailer);
		if (unlikely(nsg < 0)) {
			netdev_err(netdev, "TX: skb_cow_data() returned %d\n", nsg);
			return nsg;
		}

		pskb_put(skb, trailer, tailen);
		jumbo_frame = false;
	}

	if (netdev->gso_max_size < skb->len)
		netdev_err(netdev, "%s: skb->len = %d > 15360\n", __func__, skb->len);

	return ne6x_xmit_frame_ring(skb, tx_ring, tag_ring, jumbo_frame);
}
