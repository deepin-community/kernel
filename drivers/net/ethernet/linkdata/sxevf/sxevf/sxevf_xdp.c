#include "sxevf_xdp.h"
#include "sxe_compat.h"

#ifdef HAVE_XDP_SUPPORT
#include "sxevf_rx_proc.h"
#include "sxevf_netdev.h"
#include "sxevf_tx_proc.h"

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

static s32 sxevf_xdp_setup(struct net_device *dev, struct bpf_prog *prog)
{
	s32 ret = 0;
	u32 i, frame_size = dev->mtu + SXEVF_ETH_DEAD_LOAD;
	struct sxevf_adapter *adapter = netdev_priv(dev);
	struct bpf_prog *old_prog;

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		struct sxevf_ring *ring = adapter->rx_ring_ctxt.ring[i];

		if (frame_size > sxevf_rx_bufsz(ring)) {
			ret = -EINVAL;
			LOG_ERROR_BDF("frame size[%u] too lagre for ring[%u],"
			" buffer size=%u\n", frame_size, ring->idx,
			sxevf_rx_bufsz(ring));
			goto l_ret;
		}
	}

	old_prog = xchg(&adapter->xdp_prog, prog);

	if (!!prog != !!old_prog) {
		LOG_DEBUG_BDF("xdp prog changed from %s to %s\n",
			old_prog ? "exist" : "empty", prog ? "exist" : "empty");
		if (netif_running(dev)) {
			sxevf_close(dev);
		}

		sxevf_ring_irq_exit(adapter);

		sxevf_ring_irq_init(adapter);

		if (netif_running(dev)) {
			sxevf_open(dev);
		}
	} else {
		LOG_DEBUG_BDF("xdp prog changed from %p to %p\n", old_prog, prog);
		for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
			xchg(&adapter->rx_ring_ctxt.ring[i]->xdp_prog,
				adapter->xdp_prog);
		}
	}

	if (old_prog) {
		bpf_prog_put(old_prog);
	}

l_ret:
	return ret;
}

s32 sxevf_xdp(struct net_device *dev, struct netdev_bpf *xdp)
{
	s32 ret = 0;
	struct sxevf_adapter *adapter = netdev_priv(dev);

	switch (xdp->command) {
	case XDP_SETUP_PROG:
		LOG_DEBUG_BDF("xdp command setup, prog=%p\n", xdp->prog);
		ret = sxevf_xdp_setup(dev, xdp->prog);
		break;
#ifdef HAVE_XDP_QUERY_PROG
	case XDP_QUERY_PROG:
		xdp->prog_id = adapter->xdp_prog ?
			       adapter->xdp_prog->aux->id : 0;
		LOG_DEBUG_BDF("xdp command query, prog_id=%u\n", xdp->prog_id);
		break;
#endif
	default:
		LOG_DEBUG_BDF("invalid xdp command = 0x%x\n", xdp->command);
		ret = -EINVAL;
	}

	return ret;
}

static s32 sxevf_xdp_ring_xmit(struct sxevf_ring *ring,
				 struct xdp_buff *xdp)
{
	s32 ret = SXEVF_XDP_TX;
	struct sxevf_tx_buffer *tx_buffer;
	union sxevf_tx_data_desc *tx_desc;
	struct sxevf_tx_context_desc *context_desc;
	u32 len, cmd_type;
	dma_addr_t dma;
	u16 i;
	struct sxevf_adapter *adapter = netdev_priv(ring->netdev);

	len = xdp->data_end - xdp->data;
	LOG_DEBUG_BDF("xdp ring[%u] xmit, len=%u\n", ring->idx, len);

	if (unlikely(!sxevf_desc_unused(ring))) {
		LOG_ERROR_BDF("ring[%u] desc unsed ring empty\n", ring->idx);
		ret = SXEVF_XDP_CONSUMED;
		goto l_ret;
	}

	dma = dma_map_single(ring->dev, xdp->data, len, DMA_TO_DEVICE);
	if (dma_mapping_error(ring->dev, dma)) {
		LOG_ERROR_BDF("ring[%u] dma mapping error\n", ring->idx);
		ret = SXEVF_XDP_CONSUMED;
		goto l_ret;
	}

	i = ring->next_to_use;
	tx_buffer = &ring->tx_buffer_info[i];

	dma_unmap_len_set(tx_buffer, len, len);
	dma_unmap_addr_set(tx_buffer, dma, dma);
	tx_buffer->data = xdp->data;
	tx_buffer->bytecount = len;
	tx_buffer->gso_segs = 1;
	tx_buffer->protocol = 0;

	if (!test_bit(SXEVF_TX_XDP_RING_PRIMED, &ring->state)) {
		LOG_DEBUG_BDF("ring[%u] not setup xdp, setup it\n", ring->idx);

		set_bit(SXEVF_TX_XDP_RING_PRIMED, &ring->state);

		context_desc = SXEVF_TX_CTXTDESC(ring, 0);
		context_desc->vlan_macip_lens	=
			cpu_to_le32(ETH_HLEN << SXEVF_TX_CTXTD_MACLEN_SHIFT);
		context_desc->type_tucmd_mlhl	=
			cpu_to_le32(SXEVF_TXD_DTYP_CTXT);
		context_desc->mss_l4len_idx	= 0;

		i = 1;
	}

	cmd_type = SXEVF_TX_DESC_TYPE_DATA |
		   SXEVF_TX_DESC_DEXT |
		   SXEVF_TX_DESC_IFCS;
	cmd_type |= len | SXEVF_TX_DESC_CMD;

	tx_desc = SXEVF_TX_DESC(ring, i);
	tx_desc->read.buffer_addr = cpu_to_le64(dma);

	tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type);
	tx_desc->read.olinfo_status =
			cpu_to_le32((len << SXEVF_TX_DESC_PAYLEN_SHIFT) |
				    SXEVF_TXD_CC);

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

struct sk_buff *sxevf_xdp_run(struct sxevf_adapter *adapter,
				       struct sxevf_ring  *rx_ring,
				       struct xdp_buff *xdp)
{
	int result = SXEVF_XDP_PASS;
	struct sxevf_ring *xdp_ring;
	struct bpf_prog *xdp_prog;
	u32 act;

	rcu_read_lock();
	xdp_prog = READ_ONCE(rx_ring->xdp_prog);

	if (!xdp_prog) {
		LOG_INFO_BDF("rx_ring[%u] xdp prog is NULL\n", rx_ring->idx);
		goto xdp_out;
	}

	act = bpf_prog_run_xdp(xdp_prog, xdp);
	LOG_DEBUG_BDF("rx_ring[%u] xdp run result=0x%x\n",rx_ring->idx, act);
	switch (act) {
	case XDP_PASS:
		break;
	case XDP_TX:
		xdp_ring = adapter->xdp_ring_ctxt.ring[rx_ring->idx];
		result = sxevf_xdp_ring_xmit(xdp_ring, xdp);
		break;
	default:
		bpf_warn_invalid_xdp_action(rx_ring->netdev, xdp_prog, act);
		fallthrough;
	case XDP_ABORTED:
		trace_xdp_exception(rx_ring->netdev, xdp_prog, act);
		fallthrough;
	case XDP_DROP:
		result = SXEVF_XDP_CONSUMED;
		break;
	}
xdp_out:
	rcu_read_unlock();
	return ERR_PTR(-result);
}
#else
struct sk_buff *sxevf_xdp_run(struct sxevf_adapter *adapter,
				       struct sxevf_ring  *rx_ring,
				       struct xdp_buff *xdp)
{
	return ERR_PTR(-SXEVF_XDP_PASS);
}
#endif 
