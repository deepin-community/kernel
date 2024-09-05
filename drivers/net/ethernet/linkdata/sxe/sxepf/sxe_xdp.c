#include "sxe_xdp.h"
#include "sxe_compat.h"
#ifdef HAVE_XDP_SUPPORT
#include "sxe_rx_proc.h"
#include "sxe_pci.h"
#include "sxe_netdev.h"
#include "sxe_tx_proc.h"

DEFINE_STATIC_KEY_FALSE(sxe_xdp_tx_lock_key);

u32 sxe_max_xdp_frame_size(struct sxe_adapter *adapter)
{
	u32 size;

	if (PAGE_SIZE >= SXE_PAGE_SIZE_8KB || adapter->cap & SXE_RX_LEGACY)
		size = SXE_RXBUFFER_2K;
	else
		size = SXE_RXBUFFER_3K;

	return size;
}

static s32 sxe_xdp_setup(struct net_device *dev, struct bpf_prog *prog)
{
	s32 i;
	u32 frame_size = dev->mtu + SXE_ETH_DEAD_LOAD;
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct bpf_prog *old_prog;
	bool need_reset;
	s32 ret = 0;

	if (adapter->cap & (SXE_SRIOV_ENABLE | SXE_DCB_ENABLE)) {
		LOG_ERROR_BDF("sr_iov does not compatiable with xdp or dcb\n");
		ret = -EINVAL;
		goto l_ret;
	}

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		struct sxe_ring *ring = adapter->rx_ring_ctxt.ring[i];

		if (ring_is_lro_enabled(ring)) {
			LOG_ERROR_BDF("rsc does not compatiable with xdp\n");
			ret = -EINVAL;
			goto l_ret;
		}

		if (frame_size > sxe_rx_bufsz(ring)) {
			LOG_ERROR_BDF("xdp frame_size = %u > rx buf size=%u\n",
				frame_size, sxe_rx_bufsz(ring));
			ret = -EINVAL;
			goto l_ret;
		}
	}

	if (nr_cpu_ids > SXE_XDP_RING_NUM_MAX * 2) {
		LOG_ERROR_BDF("nr_cpu_ids=%u > max xdp ring=%u\n",
			nr_cpu_ids, SXE_XDP_RING_NUM_MAX);
		ret = -ENOMEM;
		goto l_ret;
	} else if (nr_cpu_ids > SXE_XDP_RING_NUM_MAX) {
		static_branch_inc(&sxe_xdp_tx_lock_key);
	}

	old_prog = xchg(&adapter->xdp_prog, prog);
	need_reset = (!!prog != !!old_prog);

	LOG_DEBUG_BDF("xdp setup need reset:%s\n",need_reset ? "yes" : "no");
	if (need_reset) {
		ret = sxe_ring_reassign(adapter, sxe_dcb_tc_get(adapter));
		if (ret) {
			LOG_ERROR_BDF("reassign ring err, ret=%d\n",ret);
			rcu_assign_pointer(adapter->xdp_prog, old_prog);
			ret = -EINVAL;
			goto l_ret;
		}
	} else {
		for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
			(void)xchg(&adapter->rx_ring_ctxt.ring[i]->xdp_prog,
				adapter->xdp_prog);
		}
	}

	if (old_prog) {
		bpf_prog_put(old_prog);
	}

#ifdef HAVE_AF_XDP_ZERO_COPY
	if (need_reset && prog) {
		for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
			if (adapter->xdp_ring_ctxt.ring[i]->xsk_pool) {
#ifdef HAVE_NDO_XSK_WAKEUP
				(void)sxe_xsk_wakeup(adapter->netdev, i,
						       XDP_WAKEUP_RX);
#else
				(void)sxe_xsk_async_xmit(adapter->netdev, i);
#endif
			}
		}
	}
#endif
l_ret:
	return ret;
}

#ifndef BPF_WARN_INVALID_XDP_ACTION_API_NEED_3_PARAMS
static inline void
bpf_warn_invalid_xdp_action_compat(__maybe_unused struct net_device *dev,
				__maybe_unused struct bpf_prog *prog, u32 act)
{
	bpf_warn_invalid_xdp_action(act);
}

#define bpf_warn_invalid_xdp_action(dev, prog, act) \
	bpf_warn_invalid_xdp_action_compat(dev, prog, act)

#endif

static s32 sxe_xdp_ring_xmit(struct sxe_ring *ring,
			struct xdp_frame *xdpf)
{
	struct sxe_tx_buffer *tx_buffer;
	union sxe_tx_data_desc *tx_desc;
	u32 len, cmd_type;
	dma_addr_t dma;
	u16 i;
	s32 ret = SXE_XDP_TX;

	len = xdpf->len;

	LOG_DEBUG("xdp ring[%u] xmit data, len=%u\n", ring->idx, len);
	if (unlikely(!sxe_desc_unused(ring))) {
		LOG_ERROR("the unused desc is 0\n");
		ret = SXE_XDP_CONSUMED;
		goto l_ret;
	}

	dma = dma_map_single(ring->dev, xdpf->data, len, DMA_TO_DEVICE);
	if (dma_mapping_error(ring->dev, dma)) {
		LOG_ERROR("dma mapping error\n");
		ret = SXE_XDP_CONSUMED;
		goto l_ret;
	}

	tx_buffer = &ring->tx_buffer_info[ring->next_to_use];
	tx_buffer->bytecount = len;
	tx_buffer->gso_segs = 1;
	tx_buffer->protocol = 0;

	i = ring->next_to_use;
	tx_desc = SXE_TX_DESC(ring, i);

	dma_unmap_len_set(tx_buffer, len, len);
	dma_unmap_addr_set(tx_buffer, dma, dma);
	tx_buffer->xdpf = xdpf;

	tx_desc->read.buffer_addr = cpu_to_le64(dma);

	cmd_type = SXE_TX_DESC_TYPE_DATA |
		   SXE_TX_DESC_DEXT |
		   SXE_TX_DESC_IFCS;
	cmd_type |= len | SXE_TX_DESC_CMD;
	tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type);
	tx_desc->read.olinfo_status =
		cpu_to_le32(len << SXE_TX_DESC_PAYLEN_SHIFT);

	smp_wmb();

	i++;
	if (i == ring->depth) {
		i = 0;
	}

	tx_buffer->next_to_watch = tx_desc;
	ring->next_to_use = i;

l_ret:
	return ret;
}

static inline void sxe_xdp_ring_tail_update(struct sxe_ring *ring)
{
	wmb();
	writel(ring->next_to_use, ring->desc.tail);
}

void sxe_xdp_ring_tail_update_locked(struct sxe_ring *ring)
{
	if (static_branch_unlikely(&sxe_xdp_tx_lock_key))
		spin_lock(&ring->tx_lock);
	sxe_xdp_ring_tail_update(ring);
	if (static_branch_unlikely(&sxe_xdp_tx_lock_key))
		spin_unlock(&ring->tx_lock);

}

struct sk_buff *sxe_xdp_run(struct sxe_adapter *adapter,
				     struct sxe_ring *rx_ring,
				     struct xdp_buff *xdp)
{
	s32 ret, act;
	struct bpf_prog *xdp_prog;
	struct xdp_frame *xdpf;
	struct sxe_ring *ring;
	s32 result = SXE_XDP_PASS;

	rcu_read_lock();
	xdp_prog = READ_ONCE(rx_ring->xdp_prog);

	if (!xdp_prog) {
		LOG_INFO_BDF("xdp prog is NULL\n");
		goto xdp_out;
	}

	prefetchw(xdp->data_hard_start);

	act = bpf_prog_run_xdp(xdp_prog, xdp);
	LOG_DEBUG_BDF("xdp run act=0x%x\n", act);
	switch (act) {
	case XDP_PASS:
		break;
	case XDP_TX:
		xdpf = xdp_convert_buff_to_frame(xdp);
		if (unlikely(!xdpf)) {
			result = SXE_XDP_CONSUMED;
			break;
		}

		ring = sxe_xdp_tx_ring_pick(adapter);
		if (static_branch_unlikely(&sxe_xdp_tx_lock_key))
			spin_lock(&ring->tx_lock);
		result = sxe_xdp_ring_xmit(ring, xdpf);
		if (static_branch_unlikely(&sxe_xdp_tx_lock_key))
			spin_unlock(&ring->tx_lock);
		break;
	case XDP_REDIRECT:
		ret = xdp_do_redirect(adapter->netdev, xdp, xdp_prog);
		if (!ret) {
			result = SXE_XDP_REDIR;
		} else {
			result = SXE_XDP_CONSUMED;
		}

		break;
	default:
		bpf_warn_invalid_xdp_action(rx_ring->netdev, xdp_prog, act);
		fallthrough;
	case XDP_ABORTED:
		trace_xdp_exception(rx_ring->netdev, xdp_prog, act);
		fallthrough;
	case XDP_DROP:
		result = SXE_XDP_CONSUMED;
		break;
	}

xdp_out:
	rcu_read_unlock();
	return ERR_PTR(-result);
}

int sxe_xdp_xmit(struct net_device *dev, int budget,
			  struct xdp_frame **frames, u32 flags)
{
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct sxe_ring *ring;
#ifdef XDP_XMIT_FRAME_FAILED_NEED_FREE
	int drops = 0;
#else
	int num_xmit = 0;
#endif
	u32 i;
	int ret;

	if (unlikely(test_bit(SXE_DOWN, &adapter->state))) {
		ret = -ENETDOWN;
		LOG_ERROR_BDF("adapter state:down\n");
		goto l_ret;
	}

	if (unlikely(flags & ~XDP_XMIT_FLAGS_MASK)) {
		ret = -EINVAL;
		LOG_ERROR_BDF("xdp flag not set\n");
		goto l_ret;
	}

	ring = adapter->xdp_prog ? sxe_xdp_tx_ring_pick(adapter) : NULL;
	if (unlikely(!ring)) {
		ret = -ENXIO;
		LOG_ERROR_BDF("xdp ring is not config finish yet\n");
		goto l_ret;
	}

	if (unlikely(test_bit(SXE_TX_DISABLED, &ring->state))) {
		ret = -ENXIO;
		LOG_ERROR_BDF("ring state:disabled\n");
		goto l_ret;
	}

	if (static_branch_unlikely(&sxe_xdp_tx_lock_key)) {
		spin_lock(&ring->tx_lock);
	}
	LOG_DEBUG_BDF("start xdp xmit: ring idx=%u, budget=%u", ring->idx, budget);

	for (i = 0; i < budget; i++) {
		struct xdp_frame *xdpf = frames[i];
		int err;

		err = sxe_xdp_ring_xmit(ring, xdpf);
#ifdef XDP_XMIT_FRAME_FAILED_NEED_FREE
		if (err != SXE_XDP_TX) {
			LOG_INFO_BDF("xdp ring[%u] xmit drop frame[%u]\n",
				ring->idx, i);
			xdp_return_frame_rx_napi(xdpf);
			drops++;
		}
#else
		if (err != SXE_XDP_TX)
			break;
		num_xmit++;
#endif
	}

	if (unlikely(flags & XDP_XMIT_FLUSH)) {
		sxe_xdp_ring_tail_update(ring);
	}

	if (static_branch_unlikely(&sxe_xdp_tx_lock_key)) {
		spin_unlock(&ring->tx_lock);
	}
#ifdef XDP_XMIT_FRAME_FAILED_NEED_FREE
	ret = budget - drops;
#else
	ret = num_xmit;
#endif

l_ret:
	return ret;
}

#ifdef HAVE_AF_XDP_ZERO_COPY
static void sxe_tx_ring_disable(struct sxe_adapter *adapter,
			      struct sxe_ring *tx_ring)
{
	struct sxe_hw *hw = &adapter->hw;
	unsigned long timeout = sxe_get_completion_timeout(adapter);

	set_bit(SXE_TX_DISABLED, &tx_ring->state);
	hw->dbu.ops->tx_ring_disable(hw, tx_ring->reg_idx, timeout);
	return;
}

static void sxe_tx_ring_stats_reset(struct sxe_ring *ring)
{
	memset(&ring->stats, 0, sizeof(ring->stats));
	memset(&ring->tx_stats, 0, sizeof(ring->tx_stats));
	return;
}

static void sxe_rx_ring_stats_reset(struct sxe_ring *ring)
{
	memset(&ring->stats, 0, sizeof(ring->stats));
	memset(&ring->rx_stats, 0, sizeof(ring->rx_stats));
	return;
}

static void sxe_txrx_ring_disable(struct sxe_adapter *adapter,
						u32 ring_idx)
{
	struct sxe_ring *rx_ring, *tx_ring, *xdp_ring;
	struct sxe_hw *hw = &adapter->hw;
	unsigned long timeout = sxe_get_completion_timeout(adapter);

	rx_ring = adapter->rx_ring_ctxt.ring[ring_idx];
	tx_ring = adapter->tx_ring_ctxt.ring[ring_idx];
	xdp_ring = adapter->xdp_ring_ctxt.ring[ring_idx];

	sxe_tx_ring_disable(adapter, tx_ring);
	if (xdp_ring) {
		sxe_tx_ring_disable(adapter, xdp_ring);
	}

	hw->dbu.ops->rx_ring_disable(hw, rx_ring->reg_idx, timeout);

	if (xdp_ring) {
		synchronize_rcu();
	}

	napi_disable(&rx_ring->irq_data->napi);

	sxe_tx_ring_buffer_clean(tx_ring);
	if (xdp_ring) {
		sxe_tx_ring_buffer_clean(xdp_ring);
	}

	sxe_rx_ring_buffer_clean(rx_ring);

	sxe_tx_ring_stats_reset(tx_ring);
	if (xdp_ring) {
		sxe_tx_ring_stats_reset(xdp_ring);
	}

	sxe_rx_ring_stats_reset(rx_ring);

	return;
}

void sxe_txrx_ring_enable(struct sxe_adapter *adapter, u32 ring_idx)
{
	struct sxe_ring *rx_ring, *tx_ring, *xdp_ring;

	rx_ring = adapter->rx_ring_ctxt.ring[ring_idx];
	tx_ring = adapter->tx_ring_ctxt.ring[ring_idx];
	xdp_ring = adapter->xdp_ring_ctxt.ring[ring_idx];

	sxe_tx_ring_attr_configure(adapter, tx_ring);

	if (xdp_ring) {
		sxe_tx_ring_attr_configure(adapter, xdp_ring);
	}

	sxe_rx_ring_attr_configure(adapter, rx_ring);

	clear_bit(SXE_TX_DISABLED, &tx_ring->state);
	if (xdp_ring) {
		clear_bit(SXE_TX_DISABLED, &xdp_ring->state);
	}

	napi_enable(&rx_ring->irq_data->napi);

	return;
}

static void sxe_irq_queues_rearm(struct sxe_adapter *adapter, u64 qmask)
{
	struct sxe_hw *hw = &adapter->hw;

	hw->irq.ops->ring_irq_trigger(hw, qmask);
	return;
}

static void sxe_xdp_tx_buffer_clean(struct sxe_ring *tx_ring,
				      struct sxe_tx_buffer *tx_buffer)
{
	xdp_return_frame(tx_buffer->xdpf);
	dma_unmap_single(tx_ring->dev,
			 dma_unmap_addr(tx_buffer, dma),
			 dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
	dma_unmap_len_set(tx_buffer, len, 0);
}

static bool sxe_zc_xdp_ring_xmit(struct sxe_ring *xdp_ring, u32 budget)
{
	union sxe_tx_data_desc *tx_desc = NULL;
	struct sxe_tx_buffer *tx_bi;
#ifndef XSK_UMEM_CONSUME_TX_NEED_3_PARAMS
	struct xdp_desc desc;
#endif
	dma_addr_t dma;
	u32 len;
	u32 cmd_type;
	bool work_done = true;

	LOG_DEBUG("entry xdp zc ring xmit: ring[%u], budget=%u\n",
			xdp_ring->idx, budget);

	while (budget-- > 0) {
		if (unlikely(!sxe_desc_unused(xdp_ring))) {
			work_done = false;
			break;
		}

		if (!netif_carrier_ok(xdp_ring->netdev)) {
			break;
		}

#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
		if (!xsk_tx_peek_desc(xdp_ring->xsk_pool, &desc))
			break;

		dma = xsk_buff_raw_get_dma(xdp_ring->xsk_pool, desc.addr);
		len = desc.len;

		xsk_buff_raw_dma_sync_for_device(xdp_ring->xsk_pool, dma, len);
#else
#ifndef XSK_UMEM_CONSUME_TX_NEED_3_PARAMS
		if (!xsk_umem_consume_tx(xdp_ring->xsk_pool, &desc)) {
			break;
		}

		dma = xdp_umem_get_dma(xdp_ring->xsk_pool, desc.addr);
		len = desc.len;
#else
		if (!xsk_umem_consume_tx(xdp_ring->xsk_pool, &dma, &len)) {
			break;
		}
#endif
		dma_sync_single_for_device(xdp_ring->dev, dma, len,
							   DMA_BIDIRECTIONAL);
#endif

		tx_bi = &xdp_ring->tx_buffer_info[xdp_ring->next_to_use];
		tx_bi->bytecount = len;
		tx_bi->xdpf = NULL;
		tx_bi->gso_segs = 1;

		tx_desc = SXE_TX_DESC(xdp_ring, xdp_ring->next_to_use);
		tx_desc->read.buffer_addr = cpu_to_le64(dma);

		cmd_type = SXE_TX_DESC_TYPE_DATA |
			   SXE_TX_DESC_DEXT |
			   SXE_TX_DESC_IFCS;
		cmd_type |= len | SXE_TX_DESC_CMD;
		tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type);
		tx_desc->read.olinfo_status =
			cpu_to_le32(len << SXE_TX_DESC_PAYLEN_SHIFT);

		xdp_ring->next_to_use++;
		if (xdp_ring->next_to_use == xdp_ring->depth) {
			xdp_ring->next_to_use = 0;
		}

	}

	if (tx_desc) {
		sxe_xdp_ring_tail_update(xdp_ring);
		xsk_tx_release(xdp_ring->xsk_pool);
	}

	return !!budget && work_done;
}

bool sxe_xdp_tx_ring_irq_clean(struct sxe_irq_data *irq_data,
			    struct sxe_ring *xdp_ring, int napi_budget)
{
	u16 ntc = xdp_ring->next_to_clean, ntu = xdp_ring->next_to_use;
	u32 total_packets = 0, total_bytes = 0;
	union sxe_tx_data_desc *tx_desc;
	struct sxe_tx_buffer *tx_bi;
	u32 xsk_frames = 0;
	struct sxe_adapter *adapter = irq_data->adapter;

	LOG_DEBUG_BDF("entry xdp tx irq: ring[%u], ntc=%u, ntu=%u\n",
			xdp_ring->idx, ntc, ntu);
	tx_bi = &xdp_ring->tx_buffer_info[ntc];
	tx_desc = SXE_TX_DESC(xdp_ring, ntc);

	while (ntc != ntu) {
		if (!(tx_desc->wb.status & cpu_to_le32(SXE_TX_DESC_STAT_DD))) {
			break;
		}

		total_bytes += tx_bi->bytecount;
		total_packets += tx_bi->gso_segs;

		if (tx_bi->xdpf) {
			sxe_xdp_tx_buffer_clean(xdp_ring, tx_bi);
		} else {
			xsk_frames++;
		}

		tx_bi->xdpf = NULL;

		tx_bi++;
		tx_desc++;
		ntc++;
		if (unlikely(ntc == xdp_ring->depth)) {
			ntc = 0;
			tx_bi = xdp_ring->tx_buffer_info;
			tx_desc = SXE_TX_DESC(xdp_ring, 0);
		}

		prefetch(tx_desc);
	}

	xdp_ring->next_to_clean = ntc;

	u64_stats_update_begin(&xdp_ring->syncp);
	xdp_ring->stats.bytes += total_bytes;
	xdp_ring->stats.packets += total_packets;
	u64_stats_update_end(&xdp_ring->syncp);

	irq_data->tx.irq_rate.total_bytes += total_bytes;
	irq_data->tx.irq_rate.total_packets += total_packets;

	if (xsk_frames) {
		LOG_INFO_BDF("tx xsk frames=%u\n", xsk_frames);
		xsk_tx_completed(xdp_ring->xsk_pool, xsk_frames);
	}

#ifdef HAVE_NDO_XSK_WAKEUP
	if (xsk_uses_need_wakeup(xdp_ring->xsk_pool))
		xsk_set_tx_need_wakeup(xdp_ring->xsk_pool);
#endif

	return sxe_zc_xdp_ring_xmit(xdp_ring, irq_data->tx.work_limit);
}

#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
bool sxe_zc_rx_ring_buffers_alloc(struct sxe_ring *rx_ring, u16 cleaned_count)
#else
static __always_inline bool __sxe_zc_rx_buffers_alloc(
			struct sxe_ring *rx_ring, u16 cleaned_count,
			bool alloc(struct sxe_ring *rx_ring,
			struct sxe_rx_buffer *bi))
#endif
{
	union sxe_rx_data_desc *rx_desc;
	struct sxe_rx_buffer *bi;
	u16 i = rx_ring->next_to_use;
	bool ret = true;
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
	dma_addr_t dma;
#endif
	if (!cleaned_count) {
		LOG_ERROR("the cleaned count is 0\n");
		return true;
	}

	rx_desc = SXE_RX_DESC(rx_ring, i);
	bi = &rx_ring->rx_buffer_info[i];
	i -= rx_ring->depth;

	do {
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
		bi->xdp = xsk_buff_alloc(rx_ring->xsk_pool);
		if (!bi->xdp) {
#else
		if (!alloc(rx_ring, bi)) {
#endif
			ret = false;
			break;
		}
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
		dma = xsk_buff_xdp_get_dma(bi->xdp);
		rx_desc->read.pkt_addr = cpu_to_le64(dma);
#else

		dma_sync_single_range_for_device(rx_ring->dev, bi->dma,
						 bi->page_offset,
						 rx_ring->rx_buf_len,
						 DMA_BIDIRECTIONAL);
		rx_desc->read.pkt_addr = cpu_to_le64(bi->dma);
#endif

		rx_desc++;
		bi++;
		i++;
		if (unlikely(!i)) {
			rx_desc = SXE_RX_DESC(rx_ring, 0);
			bi = rx_ring->rx_buffer_info;
			i -= rx_ring->depth;
		}

		rx_desc->wb.upper.length = 0;

		cleaned_count--;
	} while (cleaned_count);

	i += rx_ring->depth;

	if (rx_ring->next_to_use != i) {
		rx_ring->next_to_use = i;
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
		rx_ring->next_to_alloc = i;
#endif
		wmb();
		writel(i, rx_ring->desc.tail);
	}

	return ret;
}

#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
static bool sxe_zc_buffer_alloc(struct sxe_ring *rx_ring,
				  struct sxe_rx_buffer *bi)
{
	struct xdp_umem *umem = rx_ring->xsk_pool;
	void *addr = bi->addr;
	u64 handle, hr;

	if (addr)
		return true;

	if (!xsk_umem_peek_addr(umem, &handle)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	hr = umem->headroom + XDP_PACKET_HEADROOM;

	bi->dma = xdp_umem_get_dma(umem, handle);
	bi->dma += hr;

	bi->addr = xdp_umem_get_data(umem, handle);
	bi->addr += hr;

	bi->handle = handle + umem->headroom;

	xsk_umem_discard_addr(umem);
	return true;
}

static bool sxe_fast_zc_rx_buffers_alloc(struct sxe_ring *rx_ring,
					   u16 count)
{
	return __sxe_zc_rx_buffers_alloc(rx_ring, count,
						sxe_zc_buffer_alloc);
}

static bool sxe_slow_zc_rx_buffers_alloc(struct sxe_ring *rx_ring,
				       struct sxe_rx_buffer *bi)
{
	struct xdp_umem *umem = rx_ring->xsk_pool;
	u64 handle, hr;
	bool is_alloced;

	if (!xsk_umem_peek_addr_rq(umem, &handle)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		is_alloced = false;
		goto l_ret;
	}

	handle &= rx_ring->xsk_pool->chunk_mask;

	hr = umem->headroom + XDP_PACKET_HEADROOM;

	bi->dma = xdp_umem_get_dma(umem, handle);
	bi->dma += hr;

	bi->addr = xdp_umem_get_data(umem, handle);
	bi->addr += hr;

	bi->handle = handle + umem->headroom;

	xsk_umem_discard_addr_rq(umem);
	is_alloced = true;

l_ret:
	return is_alloced;
}

void sxe_zc_rx_ring_buffers_alloc(struct sxe_ring *rx_ring,
						u16 count)
{
	__sxe_zc_rx_buffers_alloc(rx_ring, count,
				    sxe_slow_zc_rx_buffers_alloc);
}

static struct sxe_rx_buffer *sxe_zc_rx_buffer_get(
				struct sxe_ring *rx_ring, u32 size)
{
	struct sxe_rx_buffer *bi;

	bi = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];

	dma_sync_single_range_for_cpu(rx_ring->dev,
				      bi->dma, 0,
				      size,
				      DMA_BIDIRECTIONAL);

	return bi;
}

static void sxe_zc_rx_buffer_reuse(struct sxe_ring *rx_ring,
				     struct sxe_rx_buffer *old_buf)
{
	u16 nta = rx_ring->next_to_alloc;
	struct sxe_rx_buffer *new_buf;

	new_buf = &rx_ring->rx_buffer_info[rx_ring->next_to_alloc];
	nta++;
	rx_ring->next_to_alloc = (nta < rx_ring->depth) ? nta : 0;

	new_buf->dma = old_buf->dma;
	new_buf->addr = old_buf->addr;
	new_buf->handle = old_buf->handle;

	old_buf->addr = NULL;
	old_buf->skb = NULL;
}

void sxe_zca_free(struct zero_copy_allocator *alloc,
					unsigned long handle_addr)
{
	struct sxe_rx_buffer *bi;
	struct sxe_ring *rx_ring;
	u64 hr, mask;
	u16 nta;

	rx_ring = container_of(alloc, struct sxe_ring, zca);
	hr = rx_ring->xsk_pool->headroom + XDP_PACKET_HEADROOM;
	mask = rx_ring->xsk_pool->chunk_mask;

	nta = rx_ring->next_to_alloc;
	bi = rx_ring->rx_buffer_info;

	nta++;
	rx_ring->next_to_alloc = (nta < rx_ring->depth) ? nta : 0;

	handle_addr &= mask;

	bi->dma = xdp_umem_get_dma(rx_ring->xsk_pool, handle_addr);
	bi->dma += hr;

	bi->addr = xdp_umem_get_data(rx_ring->xsk_pool, handle_addr);
	bi->addr += hr;

	bi->handle = (u64)handle_addr + rx_ring->xsk_pool->headroom;
	return;
}
#endif

static void sxe_ntc_update(struct sxe_ring *rx_ring)
{
	u32 ntc = rx_ring->next_to_clean + 1;

	ntc = (ntc < rx_ring->depth) ? ntc : 0;
	rx_ring->next_to_clean = ntc;
	prefetch(SXE_RX_DESC(rx_ring, ntc));
	return;
}

static s32 sxe_zc_xdp_run(struct sxe_adapter *adapter,
			    struct sxe_ring *rx_ring,
			    struct xdp_buff *xdp)
{
	s32 err, ret = SXE_XDP_PASS;
	struct bpf_prog *xdp_prog;
	struct xdp_frame *xdpf;
	struct sxe_ring *ring;
	u32 act;

	rcu_read_lock();
	xdp_prog = READ_ONCE(rx_ring->xdp_prog);
	act = bpf_prog_run_xdp(xdp_prog, xdp);
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
	xdp->handle += xdp->data - xdp->data_hard_start;
#endif
	LOG_DEBUG_BDF("zc xdp run act=0x%x\n",act);

	switch (act) {
	case XDP_PASS:
		break;
	case XDP_TX:
		xdpf = xdp_convert_buff_to_frame(xdp);
		if (unlikely(!xdpf)) {
			ret = SXE_XDP_CONSUMED;
			break;
		}
		ring = sxe_xdp_tx_ring_pick(adapter);
		if (static_branch_unlikely(&sxe_xdp_tx_lock_key))
			spin_lock(&ring->tx_lock);
		ret = sxe_xdp_ring_xmit(ring, xdpf);
		if (static_branch_unlikely(&sxe_xdp_tx_lock_key))
			spin_unlock(&ring->tx_lock);
		break;
	case XDP_REDIRECT:
		err = xdp_do_redirect(rx_ring->netdev, xdp, xdp_prog);
		ret = !err ? SXE_XDP_REDIR : SXE_XDP_CONSUMED;
		break;
	default:
		bpf_warn_invalid_xdp_action(rx_ring->netdev, xdp_prog, act);
		fallthrough;
	case XDP_ABORTED:
		trace_xdp_exception(rx_ring->netdev, xdp_prog, act);
		fallthrough;
	case XDP_DROP:
		ret = SXE_XDP_CONSUMED;
		break;
	}

	rcu_read_unlock();
	return ret;
}

static struct sk_buff *sxe_zc_skb_construct(struct sxe_ring *rx_ring,
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
						struct sxe_rx_buffer *bi)
#else
						struct sxe_rx_buffer *bi,
						struct xdp_buff *xdp)
#endif
{
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
	struct xdp_buff *xdp = bi->xdp;
#endif

	u32 metasize = xdp->data - xdp->data_meta;
	u32 datasize = xdp->data_end - xdp->data;
	struct sk_buff *skb;

	skb = __napi_alloc_skb(&rx_ring->irq_data->napi,
				xdp->data_end - xdp->data_hard_start,
				GFP_ATOMIC | __GFP_NOWARN);
	if (unlikely(!skb)) {
		LOG_ERROR("[xdp] zc skb alloc failed\n");
		goto l_ret;
	}

	skb_reserve(skb, xdp->data - xdp->data_hard_start);
	memcpy(__skb_put(skb, datasize), xdp->data, datasize);
	if (metasize) {
		skb_metadata_set(skb, metasize);
	}
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
	xsk_buff_free(xdp);
	bi->xdp = NULL;
#else
	sxe_zc_rx_buffer_reuse(rx_ring, bi);
#endif
l_ret:
	return skb;
}

#ifdef XSK_BUFF_DMA_SYNC_API_NEED_1_PARAM
static inline void
xsk_buff_dma_sync_for_cpu_compat(struct xdp_buff *xdp,
			      void __always_unused *pool)
{
	xsk_buff_dma_sync_for_cpu(xdp);
}

#define xsk_buff_dma_sync_for_cpu(xdp, pool) \
		xsk_buff_dma_sync_for_cpu_compat(xdp, pool)
#endif

int sxe_zc_rx_ring_irq_clean(struct sxe_irq_data *irq_data,
			  struct sxe_ring *rx_ring,
			  const int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0;
	struct sxe_adapter *adapter = irq_data->adapter;
	u16 cleaned_count = sxe_desc_unused(rx_ring);
	unsigned int xdp_res, xdp_xmit = 0;
	bool failure = false;
	struct sk_buff *skb;
	struct sxe_ring_stats stats;

#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
	struct xdp_buff xdp;
#ifndef HAVE_NO_XDP_BUFF_RXQ
	xdp.rxq = &rx_ring->xdp_rxq;
#endif
#endif
	LOG_DEBUG_BDF("entry xdp zc rx irq clean:irq=%u, ring_idx=%u, ring_reg_idx=%u, "
		"ring_tc_idx=%u, next_to_clean=%u, next_to_use=%u, budget=%u\n",
		irq_data->irq_idx, rx_ring->idx, rx_ring->reg_idx, rx_ring->tc_idx,
		rx_ring->next_to_clean, rx_ring->next_to_use, budget);

	while (likely(total_rx_packets < budget)) {
		union sxe_rx_data_desc *rx_desc;
		struct sxe_rx_buffer *bi;
		unsigned int size;

		if (cleaned_count >= SXE_RX_BUFFER_WRITE) {
			failure = failure ||
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
				!sxe_zc_rx_ring_buffers_alloc(rx_ring,
							     cleaned_count);
#else
				!sxe_fast_zc_rx_buffers_alloc(rx_ring,
								 cleaned_count);
#endif
			cleaned_count = 0;
		}

		rx_desc = SXE_RX_DESC(rx_ring, rx_ring->next_to_clean);
		size = le16_to_cpu(rx_desc->wb.upper.length);
		if (!size) {
			break;
		}

		LOG_DEBUG_BDF("process xdp zc rx_desc[%u], write back info:"
				"status_error=0x%x, length=%u, vlan=%u\n",
				rx_ring->next_to_clean,
				le16_to_cpu(rx_desc->wb.upper.status_error),
				le16_to_cpu(rx_desc->wb.upper.length),
				le16_to_cpu(rx_desc->wb.upper.vlan));

		dma_rmb();
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
		bi = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
#else
		bi = sxe_zc_rx_buffer_get(rx_ring, size);
#endif
		if (unlikely(!sxe_status_err_check(rx_desc,
						 SXE_RXD_STAT_EOP))) {
			struct sxe_rx_buffer *next_bi;
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
			xsk_buff_free(bi->xdp);
			bi->xdp = NULL;
#else
			sxe_zc_rx_buffer_reuse(rx_ring, bi);
#endif
			sxe_ntc_update(rx_ring);
			next_bi =
			       &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
			next_bi->discard = true;
#else
			next_bi->skb = ERR_PTR(-EINVAL);
#endif
			continue;
		}

#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
		if (unlikely(bi->discard)) {
			xsk_buff_free(bi->xdp);
			bi->xdp = NULL;
			bi->discard = false;
#else
		if (unlikely(bi->skb)) {
			sxe_zc_rx_buffer_reuse(rx_ring, bi);
#endif
			sxe_ntc_update(rx_ring);
			continue;
		}

#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
		bi->xdp->data_end = bi->xdp->data + size;
		xsk_buff_dma_sync_for_cpu(bi->xdp, rx_ring->xsk_pool);
		xdp_res = sxe_zc_xdp_run(adapter, rx_ring, bi->xdp);
#else
		xdp.data = bi->addr;
		xdp.data_meta = xdp.data;
		xdp.data_hard_start = xdp.data - XDP_PACKET_HEADROOM;
		xdp.data_end = xdp.data + size;
		xdp.handle = bi->handle;
		xdp_res = sxe_zc_xdp_run(adapter, rx_ring, &xdp);
#endif
		LOG_DEBUG_BDF("ring[%u] xdp res=0x%x\n", rx_ring->idx, xdp_res);
		if (xdp_res) {
			if (xdp_res & (SXE_XDP_TX | SXE_XDP_REDIR)) {
				xdp_xmit |= xdp_res;
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
				bi->addr = NULL;
				bi->skb = NULL;
#endif
			} else {
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
				xsk_buff_free(bi->xdp);
#else
				sxe_zc_rx_buffer_reuse(rx_ring, bi);
#endif
			}

#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
			bi->xdp = NULL;
#endif
			total_rx_packets++;
			total_rx_bytes += size;

			cleaned_count++;
			sxe_ntc_update(rx_ring);
			continue;
		}

#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
		skb = sxe_zc_skb_construct(rx_ring, bi);
#else
		skb = sxe_zc_skb_construct(rx_ring, bi, &xdp);
#endif
		if (!skb) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			break;
		}

		cleaned_count++;
		sxe_ntc_update(rx_ring);

		if (eth_skb_pad(skb))
			continue;

		total_rx_bytes += skb->len;
		total_rx_packets++;

		sxe_skb_fields_process(rx_ring, rx_desc, skb);
		sxe_rx_skb_deliver(irq_data, skb);
	}

	if (xdp_xmit & SXE_XDP_REDIR) {
		LOG_DEBUG_BDF("ring[%u] do xdp redir\n", rx_ring->idx);
		xdp_do_flush_map();
	}

	if (xdp_xmit & SXE_XDP_TX) {
		struct sxe_ring *ring = sxe_xdp_tx_ring_pick(adapter);
		sxe_xdp_ring_tail_update_locked(ring);
		LOG_DEBUG_BDF("ring[%u] do xdp tx and tx ring idx=%u\n",
			rx_ring->idx, ring->idx);
	}

	stats.packets = total_rx_packets;
	stats.bytes = total_rx_bytes;
	sxe_rx_pkt_stats_update(rx_ring, &irq_data->rx.irq_rate, &stats);

#ifdef HAVE_NDO_XSK_WAKEUP
	if (xsk_uses_need_wakeup(rx_ring->xsk_pool)) {
		if (failure || rx_ring->next_to_clean == rx_ring->next_to_use)
			xsk_set_rx_need_wakeup(rx_ring->xsk_pool);
		else
			xsk_clear_rx_need_wakeup(rx_ring->xsk_pool);

		return (int)total_rx_packets;
	}
#endif

	return failure ? budget : (int)total_rx_packets;
}

#ifdef HAVE_NDO_XSK_WAKEUP
int sxe_xsk_wakeup(struct net_device *dev, u32 qid,
				u32 __maybe_unused flags)
#else
int sxe_xsk_async_xmit(struct net_device *dev, u32 qid)
#endif
{
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct sxe_ring *ring;
	int ret = 0;

	if (test_bit(SXE_DOWN, &adapter->state)) {
		ret = -ENETDOWN;
		goto l_ret;
	}

	if (!READ_ONCE(adapter->xdp_prog)) {
		LOG_ERROR_BDF("xdp prog not setup\n");
		ret = -ENXIO;
		goto l_ret;
	}

	if (qid >= adapter->xdp_ring_ctxt.num) {
		LOG_ERROR_BDF("xdp queue id=%u >= xdp ring num=%u\n",
				qid, adapter->xdp_ring_ctxt.num);
		ret = -ENXIO;
		goto l_ret;
	}

	ring = adapter->xdp_ring_ctxt.ring[qid];
	if (test_bit(SXE_TX_DISABLED, &ring->state))
		{
		ret = -ENETDOWN;
		goto l_ret;
	}

	if (!ring->xsk_pool) {
		LOG_ERROR_BDF("xdp ring=%u do not have xsk_pool\n", qid);
		ret = -ENXIO;
		goto l_ret;
	}

	if (!napi_if_scheduled_mark_missed(&ring->irq_data->napi)) {

		u64 eics = BIT_ULL(ring->irq_data->irq_idx);
		sxe_irq_queues_rearm(adapter, eics);
	}

l_ret:
	return ret;
}

void sxe_xsk_tx_ring_clean(struct sxe_ring *tx_ring)
{
	u16 ntc = tx_ring->next_to_clean, ntu = tx_ring->next_to_use;
	struct sxe_tx_buffer *tx_bi;
	u32 xsk_frames = 0;

	while (ntc != ntu) {
		tx_bi = &tx_ring->tx_buffer_info[ntc];

		if (tx_bi->xdpf) {
			sxe_xdp_tx_buffer_clean(tx_ring, tx_bi);
		} else {
			xsk_frames++;
		}

		tx_bi->xdpf = NULL;

		ntc++;
		if (ntc == tx_ring->depth) {
			ntc = 0;
		}
	}

	if (xsk_frames) {
		xsk_tx_completed(tx_ring->xsk_pool, xsk_frames);
	}

	return;
}

#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
void sxe_xsk_rx_ring_clean(struct sxe_ring *rx_ring)
{
	struct sxe_rx_buffer *bi;
	u16 i;

	for (i = 0; i < rx_ring->depth; i++) {
		bi = &rx_ring->rx_buffer_info[i];

		if (!bi->xdp)
			continue;

		xsk_buff_free(bi->xdp);
		bi->xdp = NULL;
	}
}
#else
void sxe_xsk_rx_ring_clean(struct sxe_ring *rx_ring)
{
	u16 i = rx_ring->next_to_clean;
	struct sxe_rx_buffer *bi = &rx_ring->rx_buffer_info[i];

	while (i != rx_ring->next_to_alloc) {
		xsk_umem_fq_reuse(rx_ring->xsk_pool, bi->handle);
		i++;
		bi++;
		if (i == rx_ring->depth) {
			i = 0;
			bi = rx_ring->rx_buffer_info;
		}
	}

	return;
}
#endif

#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
static s32 sxe_xsk_umem_dma_map(struct sxe_adapter *adapter,
				  struct xdp_umem *umem)
{
	struct device *dev = &adapter->pdev->dev;
	u32 i, j;
	dma_addr_t dma;
	s32 ret = 0;

	for (i = 0; i < umem->npgs; i++) {
		dma = dma_map_page_attrs(dev, umem->pgs[i], 0, PAGE_SIZE,
					 DMA_BIDIRECTIONAL, SXE_RX_DMA_ATTR);
		if (dma_mapping_error(dev, dma)) {
			goto out_unmap;
		}

		umem->pages[i].dma = dma;
	}

	goto l_ret;

out_unmap:
	for (j = 0; j < i; j++) {
		dma_unmap_page_attrs(dev, umem->pages[i].dma, PAGE_SIZE,
				     DMA_BIDIRECTIONAL, SXE_RX_DMA_ATTR);
		umem->pages[i].dma = 0;
	}
	ret = -SXE_ERR_CONFIG;

l_ret:
	return ret;
}

static void sxe_xsk_umem_dma_unmap(struct sxe_adapter *adapter,
				     struct xdp_umem *umem)
{
	struct device *dev = &adapter->pdev->dev;
	u32 i;

	for (i = 0; i < umem->npgs; i++) {
		dma_unmap_page_attrs(dev, umem->pages[i].dma, PAGE_SIZE,
				     DMA_BIDIRECTIONAL, SXE_RX_DMA_ATTR);

		umem->pages[i].dma = 0;
	}

	return;
}
#endif

#ifdef HAVE_NETDEV_BPF_XSK_BUFF_POOL
static s32 sxe_xsk_pool_enable(struct sxe_adapter *adapter,
				 struct xsk_buff_pool *pool,
				 u16 qid)

#else
static s32 sxe_xsk_pool_enable(struct sxe_adapter *adapter,
				 struct xdp_umem *pool,
				 u16 qid)
#endif
{
	struct net_device *netdev = adapter->netdev;
#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
	struct xdp_umem_fq_reuse *reuseq;
#endif
	bool if_running;
	s32 ret;

	LOG_DEBUG_BDF("xdp xsk umem enable qid=%u\n", qid);
	if (qid >= adapter->rx_ring_ctxt.num) {
		LOG_ERROR_BDF("xdp queue id[%u] >= rx ring num[%u]\n",
			qid, adapter->rx_ring_ctxt.num);
		ret = -EINVAL;
		goto l_ret;
	}

	if (qid >= netdev->real_num_rx_queues ||
	    qid >= netdev->real_num_tx_queues) {
	   	LOG_ERROR_BDF("xdp queue id[%u] >= real rx ring num[%u]"
			"or real tx ring num[%u]\n",qid,
			netdev->real_num_rx_queues, netdev->real_num_tx_queues);
		ret = -EINVAL;
		goto l_ret;
	}
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
	ret = xsk_pool_dma_map(pool, &adapter->pdev->dev, SXE_RX_DMA_ATTR);
#else
	reuseq = xsk_reuseq_prepare(adapter->rx_ring_ctxt.ring[0]->depth);
	if (!reuseq) {
		LOG_ERROR_BDF("xdp xsk umem fill queue reuse in queue 0 is NULL\n");
		ret = -ENOMEM;
		goto l_ret;
	}
	xsk_reuseq_free(xsk_reuseq_swap(pool, reuseq));
	ret = sxe_xsk_umem_dma_map(adapter, pool);
#endif
	if (ret) {
		LOG_ERROR_BDF("xdp xsk umem[%p] dma map error,ret=%d\n", pool, ret);
		goto l_ret;
	}

	if_running = netif_running(adapter->netdev) &&
			sxe_xdp_adapter_enabled(adapter);

	if (if_running) {
		sxe_txrx_ring_disable(adapter, qid);
	}

	set_bit(qid, adapter->af_xdp_zc_qps);
	LOG_DEBUG_BDF("xdp queue id[%u] set af xdp zc qps=0x%lx\n",
			qid, *adapter->af_xdp_zc_qps);

	if (if_running) {
		sxe_txrx_ring_enable(adapter, qid);
#ifdef HAVE_NDO_XSK_WAKEUP
		ret = sxe_xsk_wakeup(adapter->netdev, qid, XDP_WAKEUP_RX);
#else
		ret = sxe_xsk_async_xmit(adapter->netdev, qid);
#endif
		if (ret) {
			clear_bit(qid, adapter->af_xdp_zc_qps);
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
			xsk_pool_dma_unmap(pool, SXE_RX_DMA_ATTR);
#else
			sxe_xsk_umem_dma_unmap(adapter, pool);
#endif
			LOG_ERROR_BDF("async xmit in queue id[%u] error,ret=%d\n",
				qid, ret);
		}
	}

l_ret:
	return ret;
}

static s32 sxe_xsk_pool_disable(struct sxe_adapter *adapter, u16 qid)
{
#ifdef HAVE_NETDEV_BPF_XSK_BUFF_POOL
	struct xsk_buff_pool *pool;
#else
	struct xdp_umem *pool;
#endif
	bool if_running;
	s32 ret = 0;

	LOG_DEBUG_BDF("xdp xsk umem disable qid=%u\n", qid);

#ifdef HAVE_NETDEV_BPF_XSK_BUFF_POOL
	pool = xsk_get_pool_from_qid(adapter->netdev, qid);
#else
	pool = xdp_get_umem_from_qid(adapter->netdev, qid);
#endif
	if (!pool) {
		LOG_ERROR_BDF("xdp xsk get umem error,qid=%u\n", qid);
		ret = -EINVAL;
		goto l_ret;
	}

	if_running = netif_running(adapter->netdev) &&
		     sxe_xdp_adapter_enabled(adapter);

	if (if_running) {
		sxe_txrx_ring_disable(adapter, qid);
	}

	clear_bit(qid, adapter->af_xdp_zc_qps);
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
	xsk_pool_dma_unmap(pool, SXE_RX_DMA_ATTR);
#else
	sxe_xsk_umem_dma_unmap(adapter, pool);
#endif
	if (if_running) {
		sxe_txrx_ring_enable(adapter, qid);
	}

l_ret:
	return ret;
}

static s32 sxe_xsk_pool_setup(struct sxe_adapter *adapter,
#ifdef HAVE_NETDEV_BPF_XSK_BUFF_POOL
					struct xsk_buff_pool *pool,
#else
					struct xdp_umem *pool,
#endif
					u16 qid)
{
	return pool ? sxe_xsk_pool_enable(adapter, pool, qid) :
		sxe_xsk_pool_disable(adapter, qid);
}

#ifdef HAVE_NETDEV_BPF_XSK_BUFF_POOL
struct xsk_buff_pool *sxe_xsk_pool_get(struct sxe_adapter *adapter,
				struct sxe_ring *ring)
{
	struct xsk_buff_pool *pool = NULL;
#else
struct xdp_umem *sxe_xsk_pool_get(struct sxe_adapter *adapter,
				struct sxe_ring *ring)
{
	struct xdp_umem *pool = NULL;
#endif
	bool xdp_on = !!READ_ONCE(adapter->xdp_prog);
	u16 qid = ring->ring_idx;

	if (!xdp_on || !test_bit(qid, adapter->af_xdp_zc_qps)) {
		LOG_DEBUG_BDF("xdp state=%s or queue id[%u] not set xdp\n",
			xdp_on ? "on" : "off", qid);
		goto l_ret;
	}
#ifdef HAVE_NETDEV_BPF_XSK_BUFF_POOL
	pool = xsk_get_pool_from_qid(adapter->netdev, qid);
#else
	pool = xdp_get_umem_from_qid(adapter->netdev, qid);
#endif
l_ret:
	return pool;
}
#endif 

int sxe_xdp(struct net_device *dev, struct netdev_bpf *xdp)
{
	struct sxe_adapter *adapter = netdev_priv(dev);
	s32 ret;

	switch (xdp->command) {
	case XDP_SETUP_PROG:
		LOG_DEBUG_BDF("xdp setup prog, prog=%p\n",xdp->prog);
		ret = sxe_xdp_setup(dev, xdp->prog);
		break;
#ifdef HAVE_XDP_QUERY_PROG
	case XDP_QUERY_PROG:
		LOG_DEBUG_BDF("xdp query prog\n");
		xdp->prog_id = adapter->xdp_prog ?
			adapter->xdp_prog->aux->id : 0;
		ret = 0;
		break;
#endif

#ifdef HAVE_AF_XDP_ZERO_COPY
	case XDP_SETUP_XSK_POOL:
#ifdef HAVE_NETDEV_BPF_XSK_BUFF_POOL
		LOG_DEBUG_BDF("xdp setup xsk pool pool=%p, queue_id=%u\n",
			xdp->xsk.pool, xdp->xsk.queue_id);
		ret = sxe_xsk_pool_setup(adapter, xdp->xsk.pool,
					    xdp->xsk.queue_id);
#else
		LOG_DEBUG_BDF("xdp setup xsk umem umem=%p, queue_id=%u\n",
			xdp->xsk.umem, xdp->xsk.queue_id);
		ret = sxe_xsk_pool_setup(adapter, xdp->xsk.umem,
					    xdp->xsk.queue_id);
#endif
		break;
#endif

	default:
		LOG_DEBUG_BDF("invalid xdp cmd= %u\n",xdp->command);
		ret = -EINVAL;
	}

	return ret;
}

#else 
struct sk_buff *sxe_xdp_run(struct sxe_adapter *adapter,
				     struct sxe_ring *rx_ring,
				     struct xdp_buff *xdp)
{
	return ERR_PTR(-SXE_XDP_PASS);
}

#endif
