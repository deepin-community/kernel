#include <linux/kernel.h>
#include <linux/raid/pq.h>

#include "sxe.h"
#include "sxe_log.h"

u16 sxe_rss_num_get(struct sxe_adapter *adapter)
{
	return (((adapter->cap & SXE_SRIOV_DCB_ENABLE) == \
						SXE_SRIOV_DCB_ENABLE)) ? \
			SXE_NON_RSS_RING_NUM : adapter->ring_f.rss;
}

u16 sxe_rss_mask_get(struct sxe_adapter *adapter)
{
	u16 mask;
	u8  tcs  = sxe_dcb_tc_get(adapter);
	u16 num  = adapter->pool_f.total_num;

	switch (adapter->cap & SXE_SRIOV_DCB_ENABLE) {
	case (SXE_SRIOV_DCB_ENABLE):
		mask = SXE_RSS_DISABLED_MASK;
		break;
	case SXE_DCB_ENABLE:
		mask = (tcs <= SXE_DCB_4_TC) ? \
			SXE_RSS_16Q_MASK : SXE_RSS_8Q_MASK;
		break;
	case SXE_SRIOV_ENABLE:
		mask = (num <= SXE_32_POOL) ? \
			SXE_RSS_4Q_MASK : SXE_RSS_2Q_MASK;
		break;
	default:
		mask = SXE_RSS_16Q_MASK;
		break;
	}

	return mask;
}
u16 sxe_pool_mask_get(struct sxe_adapter *adapter)
{
	u16 mask;
	u8  tcs = sxe_dcb_tc_get(adapter);
	u16 pool_total_num = adapter->pool_f.total_num;

	switch (adapter->cap & SXE_SRIOV_DCB_ENABLE) {
	case (SXE_SRIOV_DCB_ENABLE):
		mask = (tcs > 4) ? SXE_8Q_PER_POOL_MASK : SXE_4Q_PER_POOL_MASK;
		break;
	case SXE_SRIOV_ENABLE:
		mask = (pool_total_num > SXE_32_POOL) ? \
			SXE_2Q_PER_POOL_MASK : SXE_4Q_PER_POOL_MASK;
		break;
	default:
		mask = 0;
		break;
	}

	return mask;
}

void sxe_ring_feature_init(struct sxe_adapter *adapter)
{
	u16 rss, fnav;

	rss = min_t(u16, SXE_RSS_RING_NUM_MAX, num_online_cpus());
	adapter->ring_f.rss_limit = rss;

	fnav = min_t(u16, SXE_FNAV_RING_NUM_MAX, num_online_cpus());

	adapter->ring_f.fnav_limit = fnav;

	adapter->pool_f.pf_num_used = 1;
	adapter->pool_f.pf_num_limit = SXE_NUM_PF_POOL_DEFAULT;

	adapter->tx_ring_ctxt.depth  = SXE_DEFAULT_DESC_CNT;
	adapter->rx_ring_ctxt.depth  = SXE_DEFAULT_DESC_CNT;

	adapter->xdp_ring_ctxt.depth = 0;

	return;
}

static void sxe_dcb_sriov_ring_set(struct sxe_adapter *adapter)
{
	u32 i;
	u8  tcs                         = sxe_dcb_tc_get(adapter);
	u16 pf_pool_num_max             = SXE_TXRX_RING_NUM_MAX / tcs;
	struct sxe_pool_feature *pool_f = &adapter->pool_f;
	struct sxe_ring_feature *ring_f = &adapter->ring_f;

	pool_f->pf_num_used = min_t(u16, pool_f->pf_num_limit, pf_pool_num_max);
	pool_f->total_num   = pool_f->pf_num_used  + pool_f->vf_num_used;

	if (tcs > SXE_DCB_4_TC) {
		pool_f->total_num = min_t(u16, pool_f->total_num, SXE_16_POOL);
	} else {
		pool_f->total_num = min_t(u16, pool_f->total_num, SXE_32_POOL);
	}

	pool_f->pf_num_used = pool_f->total_num - pool_f->vf_num_used;
	ring_f->tc_per_pool = tcs;

	adapter->rx_ring_ctxt.num  = pool_f->pf_num_used * ring_f->tc_per_pool;
	adapter->tx_ring_ctxt.num  = adapter->rx_ring_ctxt.num;
	adapter->xdp_ring_ctxt.num = 0;

	for (i = 0; i < tcs; i++) {
		netdev_set_tc_queue(adapter->netdev, i, SXE_NON_RSS_RING_NUM, i);
	}

	adapter->cap &= ~SXE_FNAV_SAMPLE_ENABLE;

	LOG_INFO_BDF("tcs = %d, pf_num_used = %d, "
		"pool_total_num=%d, tc_per_pool=%d, rx_num=%u, "
		"tx_num=%u, adapter_cap = 0x%x\n",
		tcs, pool_f->pf_num_used, pool_f->total_num,
		ring_f->tc_per_pool, adapter->rx_ring_ctxt.num,
		adapter->tx_ring_ctxt.num, adapter->cap);

	return;
}

static void sxe_dcb_ring_set(struct sxe_adapter *adapter)
{
	u32    i;
	u16    ring_per_tc;
	u8     tcs                      = sxe_dcb_tc_get(adapter);
	struct net_device *dev          = adapter->netdev;
	struct sxe_ring_feature *ring_f = &adapter->ring_f;

	ring_per_tc = dev->num_tx_queues / tcs;
	if (tcs > SXE_DCB_4_TC) {
		ring_per_tc = min_t(u16, ring_per_tc, SXE_8_RING_PER_TC);
	} else {
		ring_per_tc = min_t(u16, ring_per_tc, SXE_16_RING_PER_TC);
	}

	ring_per_tc = min_t(s32, ring_per_tc, ring_f->rss_limit);

	ring_f->ring_per_tc        = ring_per_tc;
	adapter->rx_ring_ctxt.num  = ring_per_tc * tcs;
	adapter->tx_ring_ctxt.num  = adapter->rx_ring_ctxt.num;
	adapter->xdp_ring_ctxt.num = 0;

	for (i = 0; i < tcs; i++) {
		netdev_set_tc_queue(dev, i, ring_per_tc, ring_per_tc * i);
	}

	adapter->cap &= ~SXE_FNAV_SAMPLE_ENABLE;

	LOG_INFO_BDF("tcs = %d, ring_per_tc=%d,"
		"rx_num=%u, tx_num=%u, adapter_cap = 0x%x\n",
		tcs, ring_f->ring_per_tc, adapter->rx_ring_ctxt.num,
		adapter->tx_ring_ctxt.num, adapter->cap);

	return;
}

static void sxe_sriov_ring_set(struct sxe_adapter *adapter)
{
	u16 num_pool, ring_per_pool;
	struct sxe_pool_feature *pool_f = &adapter->pool_f;
	struct sxe_ring_feature *ring_f = &adapter->ring_f;
	u16    pf_num_used              = pool_f->pf_num_limit;
	u16    max_ring_per_pool        = SXE_TXRX_RING_NUM_MAX / pf_num_used;

	ring_per_pool = min_t(u16, ring_f->rss_limit, max_ring_per_pool);
	num_pool      = pf_num_used + pool_f->vf_num_used;
	num_pool      = min_t(u16, SXE_POOLS_NUM_MAX, num_pool);
	pf_num_used   = num_pool - pool_f->vf_num_used;

	if (num_pool > SXE_32_POOL) {
		ring_per_pool = min_t(u16, ring_per_pool, SXE_2_RING_PER_POOL);
	} else {
		ring_per_pool = (ring_per_pool > SXE_3_RING_PER_POOL) ?
			SXE_4_RING_PER_POOL :
			(ring_per_pool > SXE_1_RING_PER_POOL) ?
			SXE_2_RING_PER_POOL : SXE_1_RING_PER_POOL;
	}

	ring_f->ring_per_pool = ring_per_pool;
	pool_f->total_num     = num_pool;
	pool_f->pf_num_used   = pf_num_used;

	adapter->rx_ring_ctxt.num  = pf_num_used * ring_per_pool;
	adapter->tx_ring_ctxt.num  = adapter->rx_ring_ctxt.num;
	adapter->xdp_ring_ctxt.num = 0;

#ifdef HAVE_MACVLAN_OFFLOAD_SUPPORT
	if (pf_num_used > SXE_NUM_PF_POOL_DEFAULT) {
		netdev_set_num_tc(adapter->netdev, SXE_DCB_1_TC);
	}
#endif
	netdev_set_tc_queue(adapter->netdev, 0, ring_f->ring_per_pool, 0);

	adapter->cap &= ~SXE_FNAV_SAMPLE_ENABLE;

	LOG_INFO_BDF("pf_num_used = %d, pool_total_num=%d, "
		"ring_per_pool=%d, rx_num=%u, tx_num=%u, rss_limit=%u, "
		"adapter_cap = 0x%x\n",
		pool_f->pf_num_used, pool_f->total_num,
		ring_f->ring_per_pool,
		adapter->rx_ring_ctxt.num,
		adapter->tx_ring_ctxt.num,
		ring_f->rss_limit,
		adapter->cap);

	return;
}

static u16 sxe_xdp_queues_num_get(struct sxe_adapter *adapter)
{
	u16 queues = min_t(u16, SXE_XDP_RING_NUM_MAX, nr_cpu_ids);

	return adapter->xdp_prog ? queues : 0;
}

static void sxe_rss_ring_set(struct sxe_adapter *adapter)
{
	u16 rss;
	struct sxe_ring_feature *ring_f = &adapter->ring_f;

	ring_f->rss = ring_f->rss_limit;
	rss         = ring_f->rss;

	adapter->cap &= ~SXE_FNAV_SAMPLE_ENABLE;

	if (rss > SXE_NON_RSS_RING_NUM) {
		adapter->cap |= SXE_RSS_ENABLE;
	}

	if ((adapter->cap & SXE_RSS_ENABLE) &&
		adapter->fnav_ctxt.sample_rate) {
		ring_f->fnav_num = ring_f->fnav_limit;
		rss = ring_f->fnav_num;

		if (!(adapter->cap & SXE_FNAV_SPECIFIC_ENABLE)) {
			adapter->cap |= SXE_FNAV_SAMPLE_ENABLE;
		}
	}

	adapter->rx_ring_ctxt.num  = rss;
	adapter->tx_ring_ctxt.num  = rss;

	adapter->xdp_ring_ctxt.num = sxe_xdp_queues_num_get(adapter);

	LOG_INFO_BDF("rss=%u, rss_limit=%u, fnav_limit=%u "
		"rx_num=%u, tx_num=%u, xdp_num=%u cap=0x%x\n",
		ring_f->rss, ring_f->rss_limit,
		ring_f->fnav_limit,
		adapter->rx_ring_ctxt.num,
		adapter->tx_ring_ctxt.num,
		adapter->xdp_ring_ctxt.num,
		adapter->cap);

	return;
}

void sxe_ring_num_set(struct sxe_adapter *adapter)
{
	adapter->rx_ring_ctxt.num   = SXE_NON_RSS_RING_NUM;
	adapter->tx_ring_ctxt.num   = SXE_NON_RSS_RING_NUM;
	adapter->xdp_ring_ctxt.num    = 0;

	adapter->pool_f.pf_num_used   = 1;
	adapter->ring_f.ring_per_pool = 1;

	switch (adapter->cap & SXE_SRIOV_DCB_ENABLE) {
	case (SXE_SRIOV_DCB_ENABLE):
		sxe_dcb_sriov_ring_set(adapter);
		break;
	case SXE_DCB_ENABLE:
		sxe_dcb_ring_set(adapter);
		break;
	case SXE_SRIOV_ENABLE:
		sxe_sriov_ring_set(adapter);
		break;
	default:
		sxe_rss_ring_set(adapter);
		break;
	}

	LOG_INFO_BDF("set ring num, cap = 0x%x\n", adapter->cap);
	return;
}

static void sxe_dcb_sriov_ring_reg_map(struct sxe_adapter *adapter)
{
	u32    i;
	u16    reg_idx;
	u8     tcs       = sxe_dcb_tc_get(adapter);
	u16    pool      = 0;
	u16    pool_mask = sxe_pool_mask_get(adapter);
	struct sxe_pool_feature *pool_f = &adapter->pool_f;

	reg_idx = pool_f->vf_num_used * SXE_HW_RING_IN_POOL(pool_mask);

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++, reg_idx++) {
		if ((reg_idx & ~pool_mask) >= tcs) {
			pool++;
			reg_idx = __ALIGN_MASK(reg_idx, ~pool_mask);
		}

		adapter->rx_ring_ctxt.ring[i]->reg_idx = reg_idx;
		adapter->rx_ring_ctxt.ring[i]->netdev  = pool ? \
						NULL : adapter->netdev;
	}

	reg_idx = pool_f->vf_num_used * SXE_HW_RING_IN_POOL(pool_mask);

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++, reg_idx++) {
		if ((reg_idx & ~pool_mask) >= tcs)
			reg_idx = __ALIGN_MASK(reg_idx, ~pool_mask);
		adapter->tx_ring_ctxt.ring[i]->reg_idx = reg_idx;
	}

	LOG_INFO_BDF("dcb sriov ring to reg mapping\n");
	return;
}

static void sxe_first_reg_idx_get(u8 tcs, u8 tc_idx,
					u32 *tx_idx, u32 *rx_idx)
{
	if (tcs > SXE_DCB_4_TC) {
		*rx_idx = tc_idx << SXE_8TC_RX_RING_SHIFT_4;
		if (tc_idx < SXE_TC_IDX3) {
			*tx_idx = tc_idx << SXE_TC2_TX_RING_SHIFT_5; 
		} else if (tc_idx < SXE_TC_IDX5) {
			*tx_idx = (tc_idx + SXE_TX_RING_OFFSET_2) << \
				SXE_TC4_TX_RING_SHIFT_4; 
		} else {
			*tx_idx = (tc_idx + SXE_TX_RING_OFFSET_8) << \
				SXE_TC5_TX_RING_SHIFT_3; 
		}

	} else {
		*rx_idx = tc_idx << SXE_4TC_RX_RING_SHIFT_5;
		if (tc_idx < SXE_TC_IDX2) {
			*tx_idx = tc_idx << SXE_TC1_TX_RING_SHIFT_6;	
		} else {
			*tx_idx = (tc_idx + SXE_TX_RING_OFFSET_4) << \
					SXE_TC4_TX_RING_SHIFT_4;	
		}
	}

	return;
}

static void sxe_dcb_ring_reg_map(struct sxe_adapter *adapter)
{
	u32 i, offset;
	u16 ring_per_tc;
	u32 tx_idx = 0;
	u32 rx_idx = 0;
	u32 tc_idx = 0;
	u8  tcs    = sxe_dcb_tc_get(adapter);

	ring_per_tc = adapter->ring_f.ring_per_tc;

	for (offset = 0; tc_idx < tcs; tc_idx++, offset += ring_per_tc) {
		sxe_first_reg_idx_get(tcs, tc_idx, &tx_idx, &rx_idx);
		for (i = 0; i < ring_per_tc; i++, tx_idx++, rx_idx++) {
			adapter->tx_ring_ctxt.ring[offset + i]->reg_idx = tx_idx;
			adapter->rx_ring_ctxt.ring[offset + i]->reg_idx = rx_idx;
			adapter->rx_ring_ctxt.ring[offset + i]->netdev  =
								adapter->netdev;
			adapter->tx_ring_ctxt.ring[offset + i]->tc_idx  = tc_idx;
			adapter->rx_ring_ctxt.ring[offset + i]->tc_idx  = tc_idx;
		}
	}

	LOG_INFO_BDF("dcb ring to reg mapping\n");
	return;
}

static void sxe_sriov_ring_reg_map(struct sxe_adapter *adapter)
{
	u32    i;
	u16    reg_idx;
	u16    pool                     = 0;
	u16    pool_mask                = sxe_pool_mask_get(adapter);
	u16    rss_mask                 = sxe_rss_mask_get(adapter);
	struct sxe_pool_feature *pool_f = &adapter->pool_f;
	struct sxe_ring_feature *ring_f = &adapter->ring_f;

	reg_idx = pool_f->vf_num_used * SXE_HW_RING_IN_POOL(pool_mask);
	for (i = 0; i < adapter->rx_ring_ctxt.num; i++, reg_idx++) {
		if ((reg_idx & ~pool_mask) >= ring_f->ring_per_pool) {
			pool++;
			reg_idx = __ALIGN_MASK(reg_idx, ~pool_mask);
		}
		adapter->rx_ring_ctxt.ring[i]->reg_idx = reg_idx;
		adapter->rx_ring_ctxt.ring[i]->netdev  = pool ? \
						NULL : adapter->netdev;
		LOG_INFO_BDF("rx ring idx[%u] map to reg idx[%d]\n", i, reg_idx);
	}

	reg_idx = pool_f->vf_num_used * SXE_HW_RING_IN_POOL(pool_mask);
	for (i = 0; i < adapter->tx_ring_ctxt.num; i++, reg_idx++) {
		if ((reg_idx & rss_mask) >= ring_f->ring_per_pool){
			reg_idx = __ALIGN_MASK(reg_idx, ~pool_mask);
		}
		adapter->tx_ring_ctxt.ring[i]->reg_idx = reg_idx;
		LOG_INFO_BDF("tx ring idx[%u] map to reg idx[%d]\n", i, reg_idx);
	}

	LOG_INFO_BDF("sriov ring to reg mapping\n");
	return;
}

static void sxe_rss_ring_reg_map(struct sxe_adapter *adapter)
{
	u32 i, reg_idx;

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		adapter->rx_ring_ctxt.ring[i]->reg_idx = i;
		adapter->rx_ring_ctxt.ring[i]->netdev  = adapter->netdev;
	}

	for (i = 0, reg_idx = 0; i < adapter->tx_ring_ctxt.num; i++, reg_idx++) {
		adapter->tx_ring_ctxt.ring[i]->reg_idx = reg_idx;
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++, reg_idx++) {
		adapter->xdp_ring_ctxt.ring[i]->reg_idx = reg_idx;
	}

	LOG_INFO_BDF("rss ring to reg mapping\n");
	return;
}

void sxe_ring_reg_map(struct sxe_adapter *adapter)
{
	SXE_BUG_ON(!adapter);

	adapter->rx_ring_ctxt.ring[0]->reg_idx = 0;
	adapter->tx_ring_ctxt.ring[0]->reg_idx = 0;

	switch (adapter->cap & SXE_SRIOV_DCB_ENABLE) {
	case (SXE_SRIOV_DCB_ENABLE):
		sxe_dcb_sriov_ring_reg_map(adapter);
		break;
	case SXE_DCB_ENABLE:
		sxe_dcb_ring_reg_map(adapter);
		break;
	case SXE_SRIOV_ENABLE:
		sxe_sriov_ring_reg_map(adapter);
		break;
	default:
		sxe_rss_ring_reg_map(adapter);
		break;
	}

	LOG_INFO_BDF("ring to reg mapping, cap = %x\n", adapter->cap);
	return;
}


static void sxe_add_ring(struct sxe_ring *ring,
			struct sxe_list *head)
{
	ring->next = head->next;
	head->next = ring;
	head->cnt++;

	return;
}

void sxe_tx_ring_init(struct sxe_adapter *adapter, u16 base,
				    u16 cnt, u16 ring_idx, u16 irq_idx)
{
	struct sxe_irq_data *irq_data = adapter->irq_ctxt.irq_data[irq_idx];
	struct sxe_ring *ring = &(irq_data->ring[base]);
	u16 txr_idx = ring_idx;

	LOG_INFO_BDF("irq_idx:%u tx_ring_cnt:%u base:%u ring_idx:%u.\n",
		 irq_idx, cnt, base, ring_idx);

	while(cnt) {
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;
		ring->irq_data = irq_data;

		sxe_add_ring(ring, &irq_data->tx.list);
		irq_data->tx.irq_rate.next_update = jiffies + 1;

		ring->depth = adapter->tx_ring_ctxt.depth;
		ring->idx = txr_idx;

		WRITE_ONCE(adapter->tx_ring_ctxt.ring[txr_idx], ring);
		cnt--;
		txr_idx += adapter->irq_ctxt.ring_irq_num;
		ring++;
	}

	return;
}

void sxe_xdp_ring_init(struct sxe_adapter *adapter, u16 base,
				      u16 cnt, u16 ring_idx, u16 irq_idx)
{
	struct sxe_irq_data *irq_data = adapter->irq_ctxt.irq_data[irq_idx];
	struct sxe_ring *ring = &(irq_data->ring[base]);
	u16 xdp_idx = ring_idx;

	LOG_INFO_BDF("irq_idx:%u xdp_ring_cnt:%u base:%u ring_idx:%u.\n",
		 irq_idx, cnt, base, ring_idx);

	while(cnt) {
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;
		ring->irq_data = irq_data;

		irq_data->tx.xdp_ring = ring;

		ring->depth = adapter->tx_ring_ctxt.depth;
		ring->idx = xdp_idx;
		set_ring_xdp(ring);
		spin_lock_init(&ring->tx_lock);

		WRITE_ONCE(adapter->xdp_ring_ctxt.ring[xdp_idx], ring);

		cnt--;
		xdp_idx++;
		ring++;
	}

	return;
}

void sxe_rx_ring_init(struct sxe_adapter *adapter, u16 base,
				     u16  cnt, u16 ring_idx, u16 irq_idx)
{
	struct sxe_irq_data *irq_data = adapter->irq_ctxt.irq_data[irq_idx];
	struct sxe_ring *ring = &(irq_data->ring[base]);
	u16 rxr_idx = ring_idx;

	LOG_INFO_BDF("irq_idx:%u rx_ring_cnt:%u base:%u ring_idx:%u.\n",
		 irq_idx, cnt, base, ring_idx);

	while(cnt) {
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;
		ring->irq_data = irq_data;

		sxe_add_ring(ring, &irq_data->rx.list);
		irq_data->rx.irq_rate.next_update = jiffies + 1;

		ring->depth = adapter->rx_ring_ctxt.depth;
		ring->idx = rxr_idx;

		WRITE_ONCE(adapter->rx_ring_ctxt.ring[rxr_idx], ring);
		cnt--;
		rxr_idx += adapter->irq_ctxt.ring_irq_num;
		ring++;
	}

	return;
}

void sxe_ring_stats_init(struct sxe_adapter *adapter)
{
	u32 i;

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		u64_stats_init(&adapter->rx_ring_ctxt.ring[i]->syncp);
	}

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		u64_stats_init(&adapter->tx_ring_ctxt.ring[i]->syncp);
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		u64_stats_init(&adapter->xdp_ring_ctxt.ring[i]->syncp);
	}

	return;
}
