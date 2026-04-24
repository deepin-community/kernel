// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019 - 2022 Beijing WangXun Technology Co., Ltd. */

#include "txgbe.h"
#include <linux/bpf_trace.h>
#include <net/xdp_sock_drv.h>
#include <net/xdp.h>
#include "txgbe_xsk.h"

static void txgbe_disable_txr_hw(struct txgbe_adapter *adapter,
				 struct txgbe_ring *tx_ring)
{
	struct txgbe_hw *hw = &adapter->hw;
	u8 reg_idx = tx_ring->reg_idx;
	u32 txdctl;

	wr32(hw, TXGBE_PX_TR_CFG(reg_idx), TXGBE_PX_TR_CFG_SWFLSH);

	/* delay mechanism from txgbe_disable_tx */
	usleep_range(10000, 20000);

	txdctl = rd32(hw, TXGBE_PX_TR_CFG(reg_idx));

	if (!(txdctl & TXGBE_PX_TR_CFG_ENABLE))
		return;

	e_err(drv, "TXDCTL.ENABLE not cleared within the polling period\n");
}

static void txgbe_disable_rxr_hw(struct txgbe_adapter *adapter,
				 struct txgbe_ring *rx_ring)
{
	struct txgbe_hw *hw = &adapter->hw;
	u8 reg_idx = rx_ring->reg_idx;
	u32 rxdctl;

	rxdctl = rd32(hw, TXGBE_PX_RR_CFG(reg_idx));
	rxdctl &= ~TXGBE_PX_RR_CFG_RR_EN;

	/* write value back with RXDCTL.ENABLE bit cleared */
	wr32(hw, TXGBE_PX_RR_CFG(reg_idx), rxdctl);
	TXGBE_WRITE_FLUSH(hw);

	usleep_range(10000, 20000);
	rxdctl = rd32(hw, TXGBE_PX_RR_CFG(reg_idx));

	if (!(rxdctl & TXGBE_PX_RR_CFG_RR_EN))
		return;

	e_err(drv, "RXDCTL.ENABLE not cleared within the polling period\n");
}

static void txgbe_disable_txr(struct txgbe_adapter *adapter,
			      struct txgbe_ring *tx_ring)
{
	set_bit(__TXGBE_TX_DISABLED, &tx_ring->state);
	txgbe_disable_txr_hw(adapter, tx_ring);
}

static void txgbe_reset_txr_stats(struct txgbe_ring *tx_ring)
{
	memset(&tx_ring->stats, 0, sizeof(tx_ring->stats));
	memset(&tx_ring->tx_stats, 0, sizeof(tx_ring->tx_stats));
}

static void txgbe_reset_rxr_stats(struct txgbe_ring *rx_ring)
{
	memset(&rx_ring->stats, 0, sizeof(rx_ring->stats));
	memset(&rx_ring->rx_stats, 0, sizeof(rx_ring->rx_stats));
}

/**
 * txgbe_txrx_ring_disable - Disable Rx/Tx/XDP Tx rings
 * @adapter: adapter structure
 * @ring: ring index
 *
 * This function disables a certain Rx/Tx/XDP Tx ring. The function
 * assumes that the netdev is running.
 **/
void txgbe_txrx_ring_disable(struct txgbe_adapter *adapter, int ring)
{
	struct txgbe_ring *rx_ring, *tx_ring, *xdp_ring;

	rx_ring = adapter->rx_ring[ring];
	tx_ring = adapter->tx_ring[ring];
	xdp_ring = adapter->xdp_ring[ring];

	txgbe_disable_txr(adapter, tx_ring);
	if (xdp_ring)
		txgbe_disable_txr(adapter, xdp_ring);
	txgbe_disable_rxr_hw(adapter, rx_ring);

	if (xdp_ring)
		synchronize_rcu();

	/* Rx/Tx/XDP Tx share the same napi context. */
	napi_disable(&rx_ring->q_vector->napi);

	txgbe_clean_tx_ring(tx_ring);
	if (xdp_ring)
		txgbe_clean_tx_ring(xdp_ring);
	txgbe_clean_rx_ring(rx_ring);

	txgbe_reset_txr_stats(tx_ring);
	if (xdp_ring)
		txgbe_reset_txr_stats(xdp_ring);
	txgbe_reset_rxr_stats(rx_ring);
}

/**
 * txgbe_txrx_ring_enable - Enable Rx/Tx/XDP Tx rings
 * @adapter: adapter structure
 * @ring: ring index
 *
 * This function enables a certain Rx/Tx/XDP Tx ring. The function
 * assumes that the netdev is running.
 **/
void txgbe_txrx_ring_enable(struct txgbe_adapter *adapter, int ring)
{
	struct txgbe_ring *rx_ring, *tx_ring, *xdp_ring;

	rx_ring = adapter->rx_ring[ring];
	tx_ring = adapter->tx_ring[ring];
	xdp_ring = adapter->xdp_ring[ring];

	/* Rx/Tx/XDP Tx share the same napi context. */
	napi_enable(&rx_ring->q_vector->napi);

	txgbe_configure_tx_ring(adapter, tx_ring);
	if (xdp_ring)
		txgbe_configure_tx_ring(adapter, xdp_ring);
	txgbe_configure_rx_ring(adapter, rx_ring);

	clear_bit(__TXGBE_TX_DISABLED, &tx_ring->state);
	if (xdp_ring)
		clear_bit(__TXGBE_TX_DISABLED, &xdp_ring->state);
}

struct xsk_buff_pool *txgbe_xsk_umem(struct txgbe_adapter *adapter,
				     struct txgbe_ring *ring)
{
	bool xdp_on = READ_ONCE(adapter->xdp_prog);
	int qid = ring->queue_index;

	if (!adapter->xsk_pools || !adapter->xsk_pools[qid] ||
	    qid >= adapter->num_xsk_pools || !xdp_on ||
	    !test_bit(qid, adapter->af_xdp_zc_qps))
		return NULL;
	return adapter->xsk_pools[qid];
}

static int txgbe_alloc_xsk_umems(struct txgbe_adapter *adapter)
{
	if (adapter->xsk_pools)
		return 0;

	adapter->num_xsk_pools_used = 0;
	adapter->num_xsk_pools = adapter->num_rx_queues;
	adapter->xsk_pools = kcalloc(adapter->num_xsk_pools,
				     sizeof(*adapter->xsk_pools),
				     GFP_KERNEL);
	if (!adapter->xsk_pools) {
		adapter->num_xsk_pools = 0;
		return -ENOMEM;
	}

	return 0;
}

/**
 * txgbe_xsk_any_rx_ring_enabled - Checks if Rx rings have AF_XDP UMEM attached
 * @adapter: adapter
 *
 * Returns true if any of the Rx rings has an AF_XDP UMEM attached
 **/
bool txgbe_xsk_any_rx_ring_enabled(struct txgbe_adapter *adapter)
{
	int i;

	if (!adapter->xsk_pools)
		return false;

	for (i = 0; i < adapter->num_xsk_pools; i++) {
		if (adapter->xsk_pools[i])
			return true;
	}

	return false;
}

static int txgbe_add_xsk_umem(struct txgbe_adapter *adapter,
			      struct xsk_buff_pool *pool,
			      u16 qid)
{
	int err;

	err = txgbe_alloc_xsk_umems(adapter);
	if (err)
		return err;

	adapter->xsk_pools[qid] = pool;
	adapter->num_xsk_pools_used++;

	return 0;
}

static void txgbe_remove_xsk_umem(struct txgbe_adapter *adapter, u16 qid)
{
	adapter->xsk_pools[qid] = NULL;
	adapter->num_xsk_pools_used--;

	if (adapter->num_xsk_pools == 0) {
		kfree(adapter->xsk_pools);
		adapter->xsk_pools = NULL;
		adapter->num_xsk_pools = 0;
	}
}

static int txgbe_xsk_umem_enable(struct txgbe_adapter *adapter,
				 struct xsk_buff_pool *pool,
				 u16 qid)
{
	bool if_running;
	int err;

	if (qid >= adapter->num_rx_queues)
		return -EINVAL;

	if (adapter->xsk_pools) {
		if (qid >= adapter->num_xsk_pools)
			return -EINVAL;
		if (adapter->xsk_pools[qid])
			return -EBUSY;
	}

	err = xsk_pool_dma_map(pool, &adapter->pdev->dev, TXGBE_RX_DMA_ATTR);
	if (err)
		return err;

	if_running = netif_running(adapter->netdev) &&
		     READ_ONCE(adapter->xdp_prog);

	if (if_running)
		txgbe_txrx_ring_disable(adapter, qid);

	/*to avoid xsk fd get issue in some kernel version*/
	msleep(400);

	set_bit(qid, adapter->af_xdp_zc_qps);
	err = txgbe_add_xsk_umem(adapter, pool, qid);
	if (err)
		return err;

	if (if_running) {
		txgbe_txrx_ring_enable(adapter, qid);

		/* Kick start the NAPI context so that receiving will start */
		err = txgbe_xsk_wakeup(adapter->netdev, qid, XDP_WAKEUP_RX);
		if (err)
			return err;
	}

	return 0;
}

static int txgbe_xsk_umem_disable(struct txgbe_adapter *adapter, u16 qid)
{
	bool if_running;

	if (!adapter->xsk_pools || qid >= adapter->num_xsk_pools ||
	    !adapter->xsk_pools[qid])
		return -EINVAL;

	if_running = netif_running(adapter->netdev) &&
		     READ_ONCE(adapter->xdp_prog);

	if (if_running)
		txgbe_txrx_ring_disable(adapter, qid);

	clear_bit(qid, adapter->af_xdp_zc_qps);

	xsk_pool_dma_unmap(adapter->xsk_pools[qid], TXGBE_RX_DMA_ATTR);
	txgbe_remove_xsk_umem(adapter, qid);

	if (if_running)
		txgbe_txrx_ring_enable(adapter, qid);

	return 0;
}

int txgbe_xsk_umem_setup(struct txgbe_adapter *adapter, struct xsk_buff_pool *pool,
			 u16 qid)
{
	return pool ? txgbe_xsk_umem_enable(adapter, pool, qid) :
		txgbe_xsk_umem_disable(adapter, qid);
}

static int txgbe_run_xdp_zc(struct txgbe_adapter *adapter,
			    struct txgbe_ring *rx_ring,
			    struct xdp_buff *xdp)
{
	int err, result = TXGBE_XDP_PASS;
	struct bpf_prog *xdp_prog;
	struct txgbe_ring *ring;
	struct xdp_frame *xdpf;
	u32 act;

	rcu_read_lock();
	xdp_prog = READ_ONCE(rx_ring->xdp_prog);
	act = bpf_prog_run_xdp(xdp_prog, xdp);

	switch (act) {
	case XDP_PASS:
		break;
	case XDP_TX:
		xdpf = xdp_convert_buff_to_frame(xdp);
		if (unlikely(!xdpf)) {
			result = TXGBE_XDP_CONSUMED;
			break;
		}
		ring = adapter->xdp_ring[smp_processor_id() % MAX_XDP_QUEUES];
		if (static_branch_unlikely(&txgbe_xdp_locking_key))
			spin_lock(&ring->tx_lock);
		result = txgbe_xmit_xdp_ring(ring, xdpf);

		if (static_branch_unlikely(&txgbe_xdp_locking_key))
			spin_unlock(&ring->tx_lock);
		break;
	case XDP_REDIRECT:
		err = xdp_do_redirect(rx_ring->netdev, xdp, xdp_prog);
		result = !err ? TXGBE_XDP_REDIR : TXGBE_XDP_CONSUMED;
		break;
	default:
		bpf_warn_invalid_xdp_action(rx_ring->netdev, xdp_prog, act);
		fallthrough;
	case XDP_ABORTED:
		trace_xdp_exception(rx_ring->netdev, xdp_prog, act);
		fallthrough;
	case XDP_DROP:
		result = TXGBE_XDP_CONSUMED;
		break;
	}
	rcu_read_unlock();
	return result;
}

bool txgbe_alloc_rx_buffers_zc(struct txgbe_ring *rx_ring, u16 count)
{
	union txgbe_rx_desc *rx_desc;
	struct txgbe_rx_buffer *bi;
	u16 i = rx_ring->next_to_use;
	dma_addr_t dma;
	bool ok = true;

	/* nothing to do */
	if (!count)
		return true;

	rx_desc = TXGBE_RX_DESC(rx_ring, i);
	bi = &rx_ring->rx_buffer_info[i];
	i -= rx_ring->count;

	do {
		bi->xdp = xsk_buff_alloc(rx_ring->xsk_pool);
		if (!bi->xdp) {
			ok = false;
			break;
		}

		dma = xsk_buff_xdp_get_dma(bi->xdp);

		/* Refresh the desc even if buffer_addrs didn't change
		 * because each write-back erases this info.
		 */
		rx_desc->read.pkt_addr = cpu_to_le64(dma);

		rx_desc++;
		bi++;
		i++;
		if (unlikely(!i)) {
			rx_desc = TXGBE_RX_DESC(rx_ring, 0);
			bi = rx_ring->rx_buffer_info;
			i -= rx_ring->count;
		}

		/* clear the length for the next_to_use descriptor */
		rx_desc->wb.upper.length = 0;

		count--;
	} while (count);

	i += rx_ring->count;

	if (rx_ring->next_to_use != i) {
		rx_ring->next_to_use = i;

		/* Force memory writes to complete before letting h/w
		 * know there are new descriptors to fetch.  (Only
		 * applicable for weak-ordered memory model archs,
		 * such as IA-64).
		 */
		wmb();
		writel(i, rx_ring->tail);
	}

	return ok;
}

static struct sk_buff *txgbe_construct_skb_zc(struct txgbe_ring *rx_ring,
					      struct txgbe_rx_buffer *bi,
					      struct xdp_buff *xdp)
{
	struct xdp_buff *xdp_buffer = bi->xdp;
	unsigned int metasize = xdp_buffer->data - xdp_buffer->data_meta;
	unsigned int datasize = xdp_buffer->data_end - xdp_buffer->data;
	struct sk_buff *skb;

	/* allocate a skb to store the frags */
	skb = __napi_alloc_skb(&rx_ring->q_vector->napi,
			       xdp_buffer->data_end - xdp_buffer->data_hard_start,
			       GFP_ATOMIC | __GFP_NOWARN);
	if (unlikely(!skb))
		return NULL;

	skb_reserve(skb, xdp_buffer->data - xdp_buffer->data_hard_start);
	memcpy(__skb_put(skb, datasize), xdp_buffer->data, datasize);
	if (metasize)
		skb_metadata_set(skb, metasize);
	xsk_buff_free(xdp_buffer);
	bi->xdp = NULL;

	return skb;
}

static void txgbe_inc_ntc(struct txgbe_ring *rx_ring)
{
	u32 ntc = rx_ring->next_to_clean + 1;

	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;
	prefetch(TXGBE_RX_DESC(rx_ring, ntc));
}

int txgbe_clean_rx_irq_zc(struct txgbe_q_vector *q_vector,
			  struct txgbe_ring *rx_ring,
			  const int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	struct txgbe_adapter *adapter = q_vector->adapter;
	u16 cleaned_count = txgbe_desc_unused(rx_ring);
	unsigned int xdp_res, xdp_xmit = 0;
	bool failure = false;
	struct sk_buff *skb;

	while (likely(total_rx_packets < budget)) {
		union txgbe_rx_desc *rx_desc;
		struct txgbe_rx_buffer *bi;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= TXGBE_RX_BUFFER_WRITE) {
			failure = failure ||
				  !txgbe_alloc_rx_buffers_zc(rx_ring,
							     cleaned_count);
			cleaned_count = 0;
		}

		rx_desc = TXGBE_RX_DESC(rx_ring, rx_ring->next_to_clean);
		size = le16_to_cpu(rx_desc->wb.upper.length);
		if (!size)
			break;

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();

		bi = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];

		if (unlikely(!txgbe_test_staterr(rx_desc,
						 TXGBE_RXD_STAT_EOP))) {
			struct txgbe_rx_buffer *next_bi;

			xsk_buff_free(bi->xdp);
			bi->xdp = NULL;

			txgbe_inc_ntc(rx_ring);
			next_bi =
			       &rx_ring->rx_buffer_info[rx_ring->next_to_clean];

			next_bi->discard = true;
			continue;
		}

		if (unlikely(bi->discard)) {
			xsk_buff_free(bi->xdp);
			bi->xdp = NULL;
			bi->discard = false;
			txgbe_inc_ntc(rx_ring);
			continue;
		}

		bi->xdp->data_end = bi->xdp->data + size;
		xsk_buff_dma_sync_for_cpu(bi->xdp, rx_ring->xsk_pool);
		xdp_res = txgbe_run_xdp_zc(adapter, rx_ring, bi->xdp);

		if (xdp_res) {
			if (xdp_res & (TXGBE_XDP_TX | TXGBE_XDP_REDIR))
				xdp_xmit |= xdp_res;
			else
				xsk_buff_free(bi->xdp);

			bi->xdp = NULL;
			total_rx_packets++;
			total_rx_bytes += size;

			cleaned_count++;
			txgbe_inc_ntc(rx_ring);
			continue;
		}

		/* XDP_PASS path */
		skb = txgbe_construct_skb_zc(rx_ring, bi, bi->xdp);
		if (!skb) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			break;
		}

		cleaned_count++;
		txgbe_inc_ntc(rx_ring);

		if (eth_skb_pad(skb))
			continue;

		total_rx_bytes += skb->len;
		total_rx_packets++;

		txgbe_process_skb_fields(rx_ring, rx_desc, skb);
		txgbe_rx_skb(q_vector, rx_ring, rx_desc, skb);
	}

	if (xdp_xmit & TXGBE_XDP_REDIR)
		xdp_do_flush_map();

	if (xdp_xmit & TXGBE_XDP_TX) {
		int index = smp_processor_id() % adapter->num_xdp_queues;
		struct txgbe_ring *ring = adapter->xdp_ring[index];

		if (static_branch_unlikely(&txgbe_xdp_locking_key))
			spin_lock(&ring->tx_lock);

		/* update tail pointer */
		wmb();
		writel(ring->next_to_use, ring->tail);
		if (static_branch_unlikely(&txgbe_xdp_locking_key))
			spin_unlock(&ring->tx_lock);
	}

	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	u64_stats_update_end(&rx_ring->syncp);
	q_vector->rx.total_packets += total_rx_packets;
	q_vector->rx.total_bytes += total_rx_bytes;

	return failure ? budget : (int)total_rx_packets;
}

void txgbe_xsk_clean_rx_ring(struct txgbe_ring *rx_ring)
{
	struct txgbe_rx_buffer *bi;
	u16 i;

	for (i = 0; i < rx_ring->count; i++) {
		bi = &rx_ring->rx_buffer_info[i];

		if (!bi->xdp)
			continue;

		xsk_buff_free(bi->xdp);
		bi->xdp = NULL;
	}
}

static bool txgbe_xmit_zc(struct txgbe_ring *xdp_ring, unsigned int budget)
{
	unsigned int sent_frames = 0, total_bytes = 0;
	union txgbe_tx_desc *tx_desc = NULL;
	u16 ntu = xdp_ring->next_to_use;
	struct txgbe_tx_buffer *tx_bi;
	bool work_done = true;
	struct xdp_desc desc;
	dma_addr_t dma;
	u32 cmd_type;

	while (budget-- > 0) {
		if (unlikely(!txgbe_desc_unused(xdp_ring))) {
			work_done = false;
			break;
		}

		if (!netif_carrier_ok(xdp_ring->netdev))
			break;

		if (!xsk_tx_peek_desc(xdp_ring->xsk_pool, &desc))
			break;

		dma = xsk_buff_raw_get_dma(xdp_ring->xsk_pool, desc.addr);
		xsk_buff_raw_dma_sync_for_device(xdp_ring->xsk_pool, dma,
						 desc.len);

		tx_bi = &xdp_ring->tx_buffer_info[ntu];
		tx_bi->bytecount = desc.len;
		tx_bi->gso_segs = 1;
		tx_bi->xdpf = NULL;

		tx_desc = TXGBE_TX_DESC(xdp_ring, ntu);
		tx_desc->read.olinfo_status = 0;
		tx_desc->read.buffer_addr = cpu_to_le64(dma);

		/* put descriptor type bits */
		cmd_type = txgbe_tx_cmd_type(tx_bi->tx_flags);
		cmd_type |= desc.len | TXGBE_TXD_CMD;
		tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type);
		tx_desc->read.olinfo_status =
			cpu_to_le32(desc.len << TXGBE_TXD_PAYLEN_SHIFT);

		/* memory fence read desc */
		smp_wmb();
		tx_bi->next_to_watch = tx_desc;
		tx_bi->next_eop = ntu;

		xdp_ring->next_rs_idx = ntu;
		ntu++;
		if (ntu == xdp_ring->count)
			ntu = 0;
		xdp_ring->next_to_use = ntu;

		sent_frames++;
		total_bytes += tx_bi->bytecount;
	}
	if (tx_desc) {
		cmd_type |= TXGBE_TXD_RS;
		tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type);
		/* update tail pointer */
		wmb();
		writel(xdp_ring->next_to_use, xdp_ring->tail);
		xsk_tx_release(xdp_ring->xsk_pool);

		u64_stats_update_begin(&xdp_ring->syncp);
		xdp_ring->stats.bytes += total_bytes;
		xdp_ring->stats.packets += sent_frames;
		u64_stats_update_end(&xdp_ring->syncp);
		xdp_ring->q_vector->tx.total_bytes += total_bytes;
		xdp_ring->q_vector->tx.total_packets += sent_frames;
	}

	return (budget > 0) && work_done;
}

static void txgbe_clean_xdp_tx_buffer(struct txgbe_ring *tx_ring,
				      struct txgbe_tx_buffer *tx_bi)
{
	xdp_return_frame(tx_bi->xdpf);
	tx_ring->xdp_tx_active--;
	dma_unmap_single(tx_ring->dev,
			 dma_unmap_addr(tx_bi, dma),
			 dma_unmap_len(tx_bi, len), DMA_TO_DEVICE);
	dma_unmap_len_set(tx_bi, len, 0);
	tx_bi->va = NULL;
}

bool txgbe_clean_xdp_tx_irq(struct txgbe_q_vector *q_vector,
			    struct txgbe_ring *tx_ring)
{
	u32 next_rs_idx = tx_ring->next_rs_idx;
	union txgbe_tx_desc *next_rs_desc;
	u32 ntc = tx_ring->next_to_clean;
	struct txgbe_tx_buffer *tx_bi;
	u16 frames_ready = 0;
	u32 xsk_frames = 0;
	u16 i;
	struct txgbe_adapter *adapter = q_vector->adapter;
	struct txgbe_hw *hw = &adapter->hw;
	u32 head = 0;
	u32 temp = tx_ring->next_to_clean;

	if (hw->mac.type == txgbe_mac_aml || hw->mac.type == txgbe_mac_aml40)
		head = *tx_ring->headwb_mem;

	if (hw->mac.type == txgbe_mac_aml || hw->mac.type == txgbe_mac_aml40) {
		/* we have caught up to head, no work left to do */
		if (temp == head) {
			goto out_xmit;
		} else if (head > temp && !(next_rs_idx >= temp && (next_rs_idx < head))) {
			goto out_xmit;
		} else if (!(next_rs_idx >= temp || (next_rs_idx < head))) {
			goto out_xmit;
		} else {
			if (next_rs_idx >= ntc)
				frames_ready = next_rs_idx - ntc;
			else
				frames_ready = next_rs_idx + tx_ring->count - ntc;
		}
	} else {
		next_rs_desc = TXGBE_TX_DESC(tx_ring, next_rs_idx);
		if (next_rs_desc->wb.status &
		cpu_to_le32(TXGBE_TXD_STAT_DD)) {
			if (next_rs_idx >= ntc)
				frames_ready = next_rs_idx - ntc;
			else
				frames_ready = next_rs_idx + tx_ring->count - ntc;
		}
	}

	if (!frames_ready)
		goto out_xmit;

	if (likely(!tx_ring->xdp_tx_active)) {
		xsk_frames = frames_ready;
	} else {
		for (i = 0; i < frames_ready; i++) {
			tx_bi = &tx_ring->tx_buffer_info[ntc];

			if (tx_bi->xdpf)
				txgbe_clean_xdp_tx_buffer(tx_ring, tx_bi);
			else
				xsk_frames++;

			tx_bi->xdpf = NULL;

			++ntc;
			if (ntc >= tx_ring->count)
				ntc = 0;
		}
	}

	tx_ring->next_to_clean += frames_ready;
	if (unlikely(tx_ring->next_to_clean >= tx_ring->count))
		tx_ring->next_to_clean -= tx_ring->count;

	if (xsk_frames)
		xsk_tx_completed(tx_ring->xsk_pool, xsk_frames);

out_xmit:
	return txgbe_xmit_zc(tx_ring, q_vector->tx.work_limit);
}

int txgbe_xsk_wakeup(struct net_device *dev, u32 qid, u32 __maybe_unused flags)
{
	struct txgbe_adapter *adapter = netdev_priv(dev);
	struct txgbe_ring *ring;

	if (test_bit(__TXGBE_DOWN, &adapter->state))
		return -ENETDOWN;
	if (!READ_ONCE(adapter->xdp_prog))
		return -ENXIO;

	if (qid >= adapter->num_xdp_queues)
		return -ENXIO;

	if (!adapter->xsk_pools || !adapter->xsk_pools[qid])
		return -ENXIO;

	ring = adapter->xdp_ring[qid];
	if (!napi_if_scheduled_mark_missed(&ring->q_vector->napi)) {
		if (likely(napi_schedule_prep(&ring->q_vector->napi)))
			__napi_schedule(&ring->q_vector->napi);
	}

	return 0;
}

void txgbe_xsk_clean_tx_ring(struct txgbe_ring *tx_ring)
{
	unsigned long size = sizeof(struct txgbe_tx_buffer) * tx_ring->count;
	u16 ntc = tx_ring->next_to_clean, ntu = tx_ring->next_to_use;
	struct txgbe_tx_buffer *tx_bi;
	u32 xsk_frames = 0;

	while (ntc != ntu) {
		tx_bi = &tx_ring->tx_buffer_info[ntc];

		if (tx_bi->xdpf)
			txgbe_clean_xdp_tx_buffer(tx_ring, tx_bi);
		else
			xsk_frames++;

		tx_bi->xdpf = NULL;

		ntc++;
		if (ntc == tx_ring->count)
			ntc = 0;
	}

	if (xsk_frames)
		xsk_tx_completed(tx_ring->xsk_pool, xsk_frames);

	memset(tx_ring->tx_buffer_info, 0, size);

	/* Zero out the descriptor ring */
	memset(tx_ring->desc, 0, tx_ring->size);
}
