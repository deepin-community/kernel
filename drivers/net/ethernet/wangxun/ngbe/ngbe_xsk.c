// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 1999 - 2022 Intel Corporation. */

#include "ngbe.h"

#include <linux/bpf_trace.h>
#include <net/xdp_sock_drv.h>
#include <net/xdp.h>

#include "ngbe_xsk.h"
static void ngbe_disable_txr_hw(struct ngbe_adapter *adapter,
				struct ngbe_ring *tx_ring)
{
	struct ngbe_hw *hw = &adapter->hw;
	u8 reg_idx = tx_ring->reg_idx;
	u32 txdctl;

	wr32(hw, NGBE_PX_TR_CFG(reg_idx), NGBE_PX_TR_CFG_SWFLSH);

	/* delay mechanism from ngbe_disable_tx */
	msleep(20);

	txdctl = rd32(hw, NGBE_PX_TR_CFG(reg_idx));

	if (!(txdctl & NGBE_PX_TR_CFG_ENABLE))
		return;

	e_err(drv, "TXDCTL.ENABLE not cleared within the polling period\n");
}

static void ngbe_disable_rxr_hw(struct ngbe_adapter *adapter,
				struct ngbe_ring *rx_ring)
{
	struct ngbe_hw *hw = &adapter->hw;
	u8 reg_idx = rx_ring->reg_idx;
	u32 rxdctl;

	rxdctl = rd32(hw, NGBE_PX_RR_CFG(reg_idx));
	rxdctl &= ~NGBE_PX_RR_CFG_RR_EN;

	/* write value back with RXDCTL.ENABLE bit cleared */
	wr32(hw, NGBE_PX_RR_CFG(reg_idx), rxdctl);
	NGBE_WRITE_FLUSH(hw);

	msleep(20);
	rxdctl = rd32(hw, NGBE_PX_RR_CFG(reg_idx));

	if (!(rxdctl & NGBE_PX_RR_CFG_RR_EN))
		return;

	e_err(drv, "RXDCTL.ENABLE not cleared within the polling period\n");
}

static void ngbe_disable_txr(struct ngbe_adapter *adapter,
			     struct ngbe_ring *tx_ring)
{
	set_bit(__NGBE_TX_DISABLED, &tx_ring->state);
	ngbe_disable_txr_hw(adapter, tx_ring);
}

static void ngbe_reset_txr_stats(struct ngbe_ring *tx_ring)
{
	memset(&tx_ring->stats, 0, sizeof(tx_ring->stats));
	memset(&tx_ring->tx_stats, 0, sizeof(tx_ring->tx_stats));
}

static void ngbe_reset_rxr_stats(struct ngbe_ring *rx_ring)
{
	memset(&rx_ring->stats, 0, sizeof(rx_ring->stats));
	memset(&rx_ring->rx_stats, 0, sizeof(rx_ring->rx_stats));
}

/**
 * ngbe_txrx_ring_disable - Disable Rx/Tx/XDP Tx rings
 * @adapter: adapter structure
 * @ring: ring index
 * This function disables a certain Rx/Tx/XDP Tx ring. The function
 * assumes that the netdev is running.
 **/
void ngbe_txrx_ring_disable(struct ngbe_adapter *adapter, int ring)
{
	struct ngbe_ring *rx_ring, *tx_ring, *xdp_ring;

	rx_ring = adapter->rx_ring[ring];
	tx_ring = adapter->tx_ring[ring];
	xdp_ring = adapter->xdp_ring[ring];

	ngbe_disable_txr(adapter, tx_ring);
	if (xdp_ring)
		ngbe_disable_txr(adapter, xdp_ring);
	ngbe_disable_rxr_hw(adapter, rx_ring);

	if (ring_is_xdp(tx_ring))
		synchronize_rcu();

	/* Rx/Tx/XDP Tx share the same napi context. */
	napi_disable(&rx_ring->q_vector->napi);

	ngbe_clean_tx_ring(tx_ring);
	if (xdp_ring)
		ngbe_clean_tx_ring(xdp_ring);
	ngbe_clean_rx_ring(rx_ring);

	ngbe_reset_txr_stats(tx_ring);
	if (xdp_ring)
		ngbe_reset_txr_stats(xdp_ring);
	ngbe_reset_rxr_stats(rx_ring);
}

/**
 * ngbe_txrx_ring_enable - Enable Rx/Tx/XDP Tx rings
 * @adapter: adapter structure
 * @ring: ring index
 * This function enables a certain Rx/Tx/XDP Tx ring. The function
 * assumes that the netdev is running.
 **/
void ngbe_txrx_ring_enable(struct ngbe_adapter *adapter, int ring)
{
	struct ngbe_ring *rx_ring, *tx_ring, *xdp_ring;

	rx_ring = adapter->rx_ring[ring];
	tx_ring = adapter->tx_ring[ring];
	xdp_ring = adapter->xdp_ring[ring];

	/* Rx/Tx/XDP Tx share the same napi context. */
	napi_enable(&rx_ring->q_vector->napi);

	ngbe_configure_tx_ring(adapter, tx_ring);
	if (xdp_ring)
		ngbe_configure_tx_ring(adapter, xdp_ring);
	ngbe_configure_rx_ring(adapter, rx_ring);

	clear_bit(__NGBE_TX_DISABLED, &tx_ring->state);
	if (ring_is_xdp(tx_ring))
		clear_bit(__NGBE_TX_DISABLED, &xdp_ring->state);
}

struct xsk_buff_pool *ngbe_xsk_umem(struct ngbe_adapter *adapter,
				    struct ngbe_ring *ring)
{
	bool xdp_on = READ_ONCE(adapter->xdp_prog);
	int qid = ring->queue_index;

	if (!adapter->xsk_pools || !adapter->xsk_pools[qid] ||
	    qid >= adapter->num_xsk_pools || !xdp_on ||
	    !test_bit(qid, adapter->af_xdp_zc_qps))
		return NULL;
	return adapter->xsk_pools[qid];
}

static int ngbe_alloc_xsk_umems(struct ngbe_adapter *adapter)
{
	if (adapter->xsk_pools)
		return 0;

	adapter->num_xsk_pools_used = 0;
	adapter->num_xsk_pools = adapter->num_rx_queues;
	adapter->xsk_pools = kcalloc(adapter->num_xsk_pools,
				     sizeof(*adapter->xsk_pools), GFP_KERNEL);
	if (!adapter->xsk_pools) {
		adapter->num_xsk_pools = 0;
		return -ENOMEM;
	}

	return 0;
}

/**
 * ngbe_xsk_any_rx_ring_enabled - Checks if Rx rings have AF_XDP UMEM attached
 * @adapter: adapter
 * Returns true if any of the Rx rings has an AF_XDP UMEM attached
 **/
bool ngbe_xsk_any_rx_ring_enabled(struct ngbe_adapter *adapter)
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

static int ngbe_add_xsk_umem(struct ngbe_adapter *adapter,
			     struct xsk_buff_pool *pool, u16 qid)
{
	int err;

	err = ngbe_alloc_xsk_umems(adapter);
	if (err)
		return err;

	adapter->xsk_pools[qid] = pool;
	adapter->num_xsk_pools_used++;

	return 0;
}

static void ngbe_remove_xsk_umem(struct ngbe_adapter *adapter, u16 qid)
{
	adapter->xsk_pools[qid] = NULL;

	adapter->num_xsk_pools_used--;

	if (adapter->num_xsk_pools == 0) {
		kfree(adapter->xsk_pools);
		adapter->xsk_pools = NULL;
		adapter->num_xsk_pools = 0;
	}
}

static int ngbe_xsk_umem_enable(struct ngbe_adapter *adapter,
				struct xsk_buff_pool *pool, u16 qid)
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

	err = xsk_pool_dma_map(pool, &adapter->pdev->dev, NGBE_RX_DMA_ATTR);

	if (err)
		return err;

	if_running = netif_running(adapter->netdev) &&
		     READ_ONCE(adapter->xdp_prog);

	if (if_running)
		ngbe_txrx_ring_disable(adapter, qid);

	set_bit(qid, adapter->af_xdp_zc_qps);
	err = ngbe_add_xsk_umem(adapter, pool, qid);
	if (err)
		return err;

	if (if_running) {
		ngbe_txrx_ring_enable(adapter, qid);
		/* Kick start the NAPI context so that receiving will start */

		err = ngbe_xsk_wakeup(adapter->netdev, qid, XDP_WAKEUP_RX);

		if (err)
			return err;
	}

	return 0;
}

static int ngbe_xsk_umem_disable(struct ngbe_adapter *adapter, u16 qid)
{
	bool if_running;

	if (!adapter->xsk_pools || qid >= adapter->num_xsk_pools ||
	    !adapter->xsk_pools[qid])
		return -EINVAL;

	if_running = netif_running(adapter->netdev) &&
		     READ_ONCE(adapter->xdp_prog);

	if (if_running)
		ngbe_txrx_ring_disable(adapter, qid);

	clear_bit(qid, adapter->af_xdp_zc_qps);

	xsk_pool_dma_unmap(adapter->xsk_pools[qid], NGBE_RX_DMA_ATTR);

	ngbe_remove_xsk_umem(adapter, qid);

	if (if_running)
		ngbe_txrx_ring_enable(adapter, qid);

	return 0;
}

int ngbe_xsk_umem_setup(struct ngbe_adapter *adapter,
			struct xsk_buff_pool *pool, u16 qid)
{
	return pool ? ngbe_xsk_umem_enable(adapter, pool, qid) :
		      ngbe_xsk_umem_disable(adapter, qid);
}

static int ngbe_run_xdp_zc(struct ngbe_adapter *adapter,
			   struct ngbe_ring *rx_ring, struct xdp_buff *xdp)
{
	int err, result = NGBE_XDP_PASS;
	struct bpf_prog *xdp_prog;
	struct ngbe_ring *ring;
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
			result = NGBE_XDP_CONSUMED;
			break;
		}
		ring = adapter->xdp_ring[smp_processor_id() %
					 adapter->num_xdp_queues];
		if (static_branch_unlikely(&ngbe_xdp_locking_key))
			spin_lock(&ring->tx_lock);
		result = ngbe_xmit_xdp_ring(ring, xdpf);
		if (static_branch_unlikely(&ngbe_xdp_locking_key))
			spin_unlock(&ring->tx_lock);
		break;
	case XDP_REDIRECT:
		err = xdp_do_redirect(rx_ring->netdev, xdp, xdp_prog);
		result = !err ? NGBE_XDP_REDIR : NGBE_XDP_CONSUMED;
		break;
	default:
		bpf_warn_invalid_xdp_action(rx_ring->netdev, xdp_prog, act);
		fallthrough;
	case XDP_ABORTED:
		trace_xdp_exception(rx_ring->netdev, xdp_prog, act);
		/* handle aborts by dropping packet */
		fallthrough;
	case XDP_DROP:
		result = NGBE_XDP_CONSUMED;
		break;
	}
	rcu_read_unlock();
	return result;
}

bool ngbe_alloc_rx_buffers_zc(struct ngbe_ring *rx_ring, u16 count)
{
	u16 i = rx_ring->next_to_use;
	union ngbe_rx_desc *rx_desc;
	struct ngbe_rx_buffer *bi;
	dma_addr_t dma;
	bool ok = true;

	/* nothing to do */
	if (!count)
		return true;

	rx_desc = NGBE_RX_DESC(rx_ring, i);
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
			rx_desc = NGBE_RX_DESC(rx_ring, 0);
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

		/* update next to alloc since we have filled the ring */
		rx_ring->next_to_alloc = i;

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

static struct sk_buff *ngbe_construct_skb_zc(struct ngbe_ring *rx_ring,
					     struct ngbe_rx_buffer *bi)
{
	struct xdp_buff *xdp_buffer = bi->xdp;
	unsigned int metasize = xdp_buffer->data - xdp_buffer->data_meta;
	unsigned int datasize = xdp_buffer->data_end - xdp_buffer->data;
	struct sk_buff *skb;

	/* allocate a skb to store the frags */
	skb = __napi_alloc_skb(&rx_ring->q_vector->napi,
			       xdp_buffer->data_end -
				       xdp_buffer->data_hard_start,
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

static void ngbe_inc_ntc(struct ngbe_ring *rx_ring)
{
	u32 ntc = rx_ring->next_to_clean + 1;

	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;
	prefetch(NGBE_RX_DESC(rx_ring, ntc));
}

int ngbe_clean_rx_irq_zc(struct ngbe_q_vector *q_vector,
			 struct ngbe_ring *rx_ring, const int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	struct ngbe_adapter *adapter = q_vector->adapter;
	u16 cleaned_count = ngbe_desc_unused(rx_ring);
	unsigned int xdp_res, xdp_xmit = 0;
	bool failure = false;
	struct sk_buff *skb;

	while (likely(total_rx_packets < budget)) {
		union ngbe_rx_desc *rx_desc;
		struct ngbe_rx_buffer *bi;
		unsigned int size;

		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= NGBE_RX_BUFFER_WRITE) {
			failure = failure ||
				  !ngbe_alloc_rx_buffers_zc(rx_ring, cleaned_count);
			cleaned_count = 0;
		}

		rx_desc = NGBE_RX_DESC(rx_ring, rx_ring->next_to_clean);
		size = le16_to_cpu(rx_desc->wb.upper.length);
		if (!size)
			break;

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();

		bi = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];

		if (unlikely(!ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_EOP))) {
			struct ngbe_rx_buffer *next_bi;

			xsk_buff_free(bi->xdp);
			bi->xdp = NULL;
			ngbe_inc_ntc(rx_ring);
			next_bi =
				&rx_ring->rx_buffer_info[rx_ring->next_to_clean];
			next_bi->discard = true;
			continue;
		}

		if (unlikely(bi->discard)) {
			xsk_buff_free(bi->xdp);
			bi->xdp = NULL;
			bi->discard = false;

			ngbe_inc_ntc(rx_ring);
			continue;
		}

		bi->xdp->data_end = bi->xdp->data + size;
		xsk_buff_dma_sync_for_cpu(bi->xdp, rx_ring->xsk_pool);
		xdp_res = ngbe_run_xdp_zc(adapter, rx_ring, bi->xdp);

		if (xdp_res) {
			if (xdp_res & (NGBE_XDP_TX | NGBE_XDP_REDIR))
				xdp_xmit |= xdp_res;
			else
				xsk_buff_free(bi->xdp);
			total_rx_packets++;
			total_rx_bytes += size;

			cleaned_count++;
			ngbe_inc_ntc(rx_ring);
			continue;
		}

		/* XDP_PASS path */
		skb = ngbe_construct_skb_zc(rx_ring, bi);
		if (!skb) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			break;
		}

		cleaned_count++;
		ngbe_inc_ntc(rx_ring);

		if (eth_skb_pad(skb))
			continue;

		total_rx_bytes += skb->len;
		total_rx_packets++;

		ngbe_process_skb_fields(rx_ring, rx_desc, skb);
		ngbe_rx_skb(q_vector, rx_ring, rx_desc, skb);
	}

	if (xdp_xmit & NGBE_XDP_REDIR)
		xdp_do_flush();

	if (xdp_xmit & NGBE_XDP_TX) {
		struct ngbe_ring *ring =
			adapter->xdp_ring[smp_processor_id() %
					  adapter->num_xdp_queues];
		if (static_branch_unlikely(&ngbe_xdp_locking_key))
			spin_lock(&ring->tx_lock);
		/* Force memory writes to complete before letting h/w know there
		 * are new descriptors to fetch.
		 */
		wmb();
		writel(ring->next_to_use, ring->tail);
		if (static_branch_unlikely(&ngbe_xdp_locking_key))
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

void ngbe_xsk_clean_rx_ring(struct ngbe_ring *rx_ring)
{
	struct ngbe_rx_buffer *bi;
	u16 i;

	for (i = 0; i < rx_ring->count; i++) {
		bi = &rx_ring->rx_buffer_info[i];

		if (!bi->xdp)
			continue;

		xsk_buff_free(bi->xdp);
		bi->xdp = NULL;
	}
}

static bool ngbe_xmit_zc(struct ngbe_ring *xdp_ring, unsigned int budget)
{
	union ngbe_tx_desc *tx_desc = NULL;
	u16 ntu = xdp_ring->next_to_use;
	struct ngbe_tx_buffer *tx_bi;
	bool work_done = true;
	struct xdp_desc desc;
	dma_addr_t dma;
	u32 cmd_type;

	while (budget-- > 0) {
		if (unlikely(!ngbe_desc_unused(xdp_ring))) {
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
		tx_desc = NGBE_TX_DESC(xdp_ring, ntu);
		tx_desc->read.olinfo_status = 0;
		tx_desc->read.buffer_addr = cpu_to_le64(dma);

		/* put descriptor type bits */
		cmd_type = ngbe_tx_cmd_type(tx_bi->tx_flags);
		cmd_type |= desc.len | NGBE_TXD_CMD;
		tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type);
		tx_desc->read.olinfo_status =
			cpu_to_le32(desc.len << NGBE_TXD_PAYLEN_SHIFT);
		/* Force memory writes to complete before letting h/w know there
		 * are new descriptors to fetch.
		 */
		smp_wmb();
		tx_bi->next_to_watch = tx_desc;
		ntu++;
		if (ntu == xdp_ring->count)
			ntu = 0;
		xdp_ring->next_to_use = ntu;
	}
	if (tx_desc) {
		/* Force memory writes to complete before letting h/w know there
		 * are new descriptors to fetch.
		 */
		wmb();
		writel(xdp_ring->next_to_use, xdp_ring->tail);
		xsk_tx_release(xdp_ring->xsk_pool);
	}

	return (budget > 0) && work_done;
}

static void ngbe_clean_xdp_tx_buffer(struct ngbe_ring *tx_ring,
				     struct ngbe_tx_buffer *tx_bi)
{
	xdp_return_frame(tx_bi->xdpf);
	tx_ring->xdp_tx_active--;
	dma_unmap_single(tx_ring->dev, dma_unmap_addr(tx_bi, dma),
			 dma_unmap_len(tx_bi, len), DMA_TO_DEVICE);
	dma_unmap_len_set(tx_bi, len, 0);
	tx_bi->va = NULL;
}

bool ngbe_clean_xdp_tx_irq(struct ngbe_q_vector *q_vector,
			   struct ngbe_ring *tx_ring)
{
	u32 ntu = tx_ring->next_to_use, ntc = tx_ring->next_to_clean;
	union ngbe_tx_desc *tx_desc;
	struct ngbe_tx_buffer *tx_bi;
	unsigned int total_packets = 0, total_bytes = 0;
	u32 xsk_frames = 0;

	tx_bi = &tx_ring->tx_buffer_info[ntc];
	tx_desc = NGBE_TX_DESC(tx_ring, ntc);
	while (ntc != ntu) {
		if (!(tx_desc->wb.status & cpu_to_le32(NGBE_TXD_STAT_DD)))
			break;

		total_bytes += tx_bi->bytecount;
		total_packets += tx_bi->gso_segs;

		if (tx_bi->xdpf)
			ngbe_clean_xdp_tx_buffer(tx_ring, tx_bi);
		else
			xsk_frames++;

		tx_bi->xdpf = NULL;

		tx_bi++;
		tx_desc++;
		ntc++;
		if (unlikely(ntc == tx_ring->count)) {
			ntc = 0;
			tx_bi = tx_ring->tx_buffer_info;
			tx_desc = NGBE_TX_DESC(tx_ring, 0);
		}

		/* issue prefetch for next Tx descriptor */
		prefetch(tx_desc);
	}
	tx_ring->next_to_clean = ntc;

	if (unlikely(tx_ring->next_to_clean >= tx_ring->count))
		tx_ring->next_to_clean -= tx_ring->count;

	u64_stats_update_begin(&tx_ring->syncp);
	tx_ring->stats.bytes += total_bytes;
	tx_ring->stats.packets += total_packets;
	u64_stats_update_end(&tx_ring->syncp);
	tx_ring->q_vector->tx.total_bytes += total_bytes;
	tx_ring->q_vector->tx.total_packets += total_packets;

	if (xsk_frames)
		xsk_tx_completed(tx_ring->xsk_pool, xsk_frames);
	return ngbe_xmit_zc(tx_ring, q_vector->tx.work_limit);
}

int ngbe_xsk_wakeup(struct net_device *dev, u32 qid, u32 __maybe_unused flags)
{
	struct ngbe_adapter *adapter = netdev_priv(dev);
	struct ngbe_ring *ring;

	if (test_bit(__NGBE_DOWN, &adapter->state))
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

void ngbe_xsk_clean_tx_ring(struct ngbe_ring *tx_ring)
{
	u16 ntc = tx_ring->next_to_clean, ntu = tx_ring->next_to_use;
	struct ngbe_tx_buffer *tx_bi;
	u32 xsk_frames = 0;
	unsigned long size = sizeof(struct ngbe_tx_buffer) * tx_ring->count;

	while (ntc != ntu) {
		tx_bi = &tx_ring->tx_buffer_info[ntc];

		if (tx_bi->xdpf)
			ngbe_clean_xdp_tx_buffer(tx_ring, tx_bi);
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
