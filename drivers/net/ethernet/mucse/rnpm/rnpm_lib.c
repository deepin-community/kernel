// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#include "rnpm.h"
#include "rnpm_sriov.h"
#include "rnpm_common.h"

/**
 * rnpm_cache_ring_sriov - Descriptor ring to register mapping for sriov
 * @adapter: board private structure to initialize
 *
 * SR-IOV doesn't use any descriptor rings but changes the default if
 * no other mapping is used.
 *
 */
static bool rnpm_cache_ring_sriov(struct rnpm_adapter *adapter)
{
	if (!(adapter->flags & RNPM_FLAG_VMDQ_ENABLED))
		return false;
	return true;
}

/**
 * rnpm_cache_ring_rss - Descriptor ring to register mapping for RSS
 * @adapter: board private structure to initialize
 *
 * Cache the descriptor ring offsets for RSS to the assigned rings.
 *
 **/
static bool rnpm_cache_ring_rss(struct rnpm_adapter *adapter)
{
	int i;
	int ring_step = 1;
	int ring_start = 0;
	int ring_alloc = 1;
	struct rnpm_ring *ring;
	struct rnpm_hw *hw = &adapter->hw;

	switch (hw->mode) {
	case MODE_NIC_MODE_1PORT:
		ring_step = 1;
		ring_start = adapter->port;
		ring_alloc = 1;
		break;
	case MODE_NIC_MODE_1PORT_40G:
		ring_step = 1;
		ring_start = adapter->port;
		ring_alloc = 1;
		break;
	case MODE_NIC_MODE_2PORT:
		ring_step = 4;
		ring_start = adapter->port * 2;
		ring_alloc = 2;
		break;
	case MODE_NIC_MODE_4PORT:
		ring_step = 4;
		ring_start = adapter->port;
		ring_alloc = 1;
		break;
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		ring = adapter->tx_ring[i];
		/* reset ring vars */
		ring->rnpm_queue_idx =
			ring_start + (i / ring_alloc) * ring_step +
			((ring_alloc == 1) ? 0 : ((i % 2) ? 1 : 0));
		ring->dma_hw_addr = hw->hw_addr;
		ring->dma_int_stat =
			hw->hw_addr +
			RNPM_DMA_INT_STAT(ring->rnpm_queue_idx);
		ring->dma_int_mask = ring->dma_int_stat + 4;
		ring->dma_int_clr = ring->dma_int_stat + 8;
	}

	for (i = 0; i < adapter->num_rx_queues; i++) {
		ring = adapter->rx_ring[i];
		/* reset ring vars */
		ring->rnpm_queue_idx =
			ring_start + (i / ring_alloc) * ring_step +
			((ring_alloc == 1) ? 0 : ((i % 2) ? 1 : 0));
		ring->dma_hw_addr = hw->hw_addr;
		ring->dma_int_stat =
			hw->hw_addr +
			RNPM_DMA_INT_STAT(ring->rnpm_queue_idx);
		ring->dma_int_mask = ring->dma_int_stat + 4;
		ring->dma_int_clr = ring->dma_int_stat + 8;
	}

	return true;
}

/**
 * rnpm_cache_ring_register - Descriptor ring to register mapping
 * @adapter: board private structure to initialize
 *
 * Once we know the feature-set enabled for the device, we'll cache
 * the register offset the descriptor ring is assigned to.
 *
 * Note, the order the various feature calls is important.  It must start with
 * the "most" features enabled at the same time, then trickle down to the
 * least amount of features turned on at once.
 **/
static void rnpm_cache_ring_register(struct rnpm_adapter *adapter)
{
	/* sriov ring alloc is added before, this maybe no use */
	if (rnpm_cache_ring_sriov(adapter))
		return;
	rnpm_cache_ring_rss(adapter);
}

#define RNPM_RSS_64Q_MASK 0x3F
#define RNPM_RSS_16Q_MASK 0xF
#define RNPM_RSS_8Q_MASK 0x7
#define RNPM_RSS_4Q_MASK 0x3
#define RNPM_RSS_2Q_MASK 0x1
#define RNPM_RSS_DISABLED_MASK 0x0

/**
 * rnpm_set_sriov_queues - Allocate queues for SR-IOV devices
 * @adapter: board private structure to initialize
 *
 * When SR-IOV (Single Root IO Virtualiztion) is enabled, allocate queues
 * and VM pools where appropriate.  If RSS is available, then also try and
 * enable RSS and map accordingly.
 *
 **/
static bool rnpm_set_sriov_queues(struct rnpm_adapter *adapter)
{
	u16 rss_i = adapter->ring_feature[RING_F_RSS].limit;
	u16 rss_m = RNPM_RSS_DISABLED_MASK;
	u16 vmdq_m = 0;

	/* only proceed if SR-IOV is enabled */
	if (!(adapter->flags & RNPM_FLAG_SRIOV_ENABLED))
		return false;

	/* save features for later use */
	adapter->ring_feature[RING_F_VMDQ].indices =
		adapter->max_ring_pair_counts - 1;
	adapter->ring_feature[RING_F_VMDQ].mask = vmdq_m;

	/* limit RSS based on user input and save for later use */
	adapter->ring_feature[RING_F_RSS].indices = rss_i;
	adapter->ring_feature[RING_F_RSS].mask = rss_m;

	adapter->num_rx_queues = PF_RING_CNT_WHEN_IOV_ENABLED;
	adapter->num_tx_queues = PF_RING_CNT_WHEN_IOV_ENABLED;

	/* disable ATR as it is not supported when VMDq is enabled */
	adapter->flags &= ~RNPM_FLAG_FDIR_HASH_CAPABLE;

	return true;
}

u32 rnpm_rss_indir_tbl_entries(struct rnpm_adapter *adapter)
{
	return 128;
}
/**
 * rnpm_set_rss_queues - Allocate queues for RSS
 * @adapter: board private structure to initialize
 *
 * This is our "base" multiqueue mode.  RSS (Receive Side Scaling) will try
 * to allocate one Rx queue per CPU, and if available, one Tx queue per CPU.
 *
 **/
static bool rnpm_set_rss_queues(struct rnpm_adapter *adapter)
{
	struct rnpm_ring_feature *f;
	u16 rss_i;

	f = &adapter->ring_feature[RING_F_RSS];
	rss_i = f->limit;
	/* set limit -> indices */
	f->indices = rss_i;
	/* should init rss mask */
	if (adapter->hw.rss_type == rnpm_rss_n10)
		f->mask = RNPM_RSS_64Q_MASK;
	/* set rss_i -> adapter->num_tx_queues */
	adapter->num_tx_queues =
		min_t(int, rss_i, adapter->max_ring_pair_counts);
	adapter->num_rx_queues = adapter->num_tx_queues;

	rnpm_dbg("[%s] limit:%d indices:%d queues:%d\n",
		 adapter->netdev->name, f->limit, f->indices,
		 adapter->num_tx_queues);

	return true;
}

/**
 * rnpm_set_num_queues - Allocate queues for device, feature dependent
 * @adapter: board private structure to initialize
 *
 * This is the top level queue allocation routine.  The order here is very
 * important, starting with the "most" number of features turned on at once,
 * and ending with the smallest set of features.  This way large combinations
 * can be allocated if they're turned on, and smaller combinations are the
 * fallthrough conditions.
 *
 **/
static void rnpm_set_num_queues(struct rnpm_adapter *adapter)
{
	/* Start with base case */
	adapter->num_tx_queues = 1;
	adapter->num_rx_queues = 1;
	if (rnpm_set_sriov_queues(adapter))
		return;
	/* at last we support rss */
	rnpm_set_rss_queues(adapter);
}

int rnpm_acquire_msix_vectors(struct rnpm_adapter *adapter, int vectors)
{
	int err;
	int vectors_per_port = 0;
	struct rnpm_hw *hw = &adapter->hw;
	struct rnpm_pf_adapter *pf_adapter = adapter->pf_adapter;

	dbg_apt("%s %d\n", __func__, vectors);

#ifdef DISABLE_RX_IRQ
	vectors -= adapter->num_other_vectors;
	adapter->num_q_vectors = min(vectors, adapter->max_q_vectors);
	return 0;
#endif

	switch (hw->mode) {
	case MODE_NIC_MODE_1PORT:
		vectors_per_port = vectors - adapter->num_other_vectors;
		break;
	case MODE_NIC_MODE_1PORT_40G:
		vectors_per_port = vectors - adapter->num_other_vectors;
		break;
	case MODE_NIC_MODE_2PORT:
		vectors_per_port =
			(vectors - adapter->num_other_vectors) / 2;
		break;
	case MODE_NIC_MODE_4PORT:
		vectors_per_port =
			(vectors - adapter->num_other_vectors) / 4;
		break;
	}
	/* if msix is init before, return here */
	adapter->num_q_vectors =
		min(vectors_per_port, adapter->max_q_vectors);
	if (pf_adapter->msix_entries)
		return 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	err = pci_enable_msix_range(adapter->pdev, adapter->msix_entries,
				    vectors, vectors);
#else
	err = pci_enable_msix(adapter->pdev, adapter->msix_entries,
			      vectors);
#endif
	if (err < 0) {
		netdev_err(adapter->netdev,
			   "pci_enable_msix faild: req:%d err:%d\n",
			   vectors, err);
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;
		return -EINVAL;
	}
	/*
	 * Adjust for only the vectors we'll use, which is minimum
	 * of max_msix_q_vectors + NON_Q_VECTORS, or the number of
	 * vectors we were allocated.
	 */

	return 0;
}

static void rnpm_add_ring(struct rnpm_ring *ring,
			  struct rnpm_ring_container *head)
{
	ring->next = head->ring;
	head->ring = ring;
	head->count++;
}

static inline void rnpm_irq_disable_queues(struct rnpm_q_vector *q_vector)
{
	struct rnpm_ring *ring;

	rnpm_for_each_ring(ring, q_vector->tx) {
		rnpm_wr_reg(ring->dma_int_mask,
			    (RX_INT_MASK | TX_INT_MASK));
	}
}

static enum hrtimer_restart irq_miss_check(struct hrtimer *hrtimer)
{
	struct rnpm_q_vector *q_vector;
	struct rnpm_ring *ring;
	struct rnpm_tx_desc *eop_desc;
	struct rnpm_adapter *adapter;

	int tx_next_to_clean;
	int tx_next_to_use;

	struct rnpm_tx_buffer *tx_buffer;
	union rnpm_rx_desc *rx_desc;

	q_vector = container_of(hrtimer, struct rnpm_q_vector,
				irq_miss_check_timer);
	adapter = q_vector->adapter;
	if (test_bit(__RNPM_DOWN, &adapter->state) ||
	    test_bit(__RNPM_RESETTING, &adapter->state))
		goto do_self_napi;
	set_bit(RNPM_IRQ_MISS_HANDLE_DONE, &q_vector->flags);

	rnpm_for_each_ring(ring, q_vector->tx) {
		tx_next_to_clean = ring->next_to_clean;
		tx_next_to_use = ring->next_to_use;
		if (tx_next_to_use != tx_next_to_clean) {
			tx_buffer =
				&ring->tx_buffer_info[tx_next_to_clean];
			eop_desc = tx_buffer->next_to_watch;
			if (eop_desc) {
				if ((eop_desc->vlan_cmd &
				     cpu_to_le32(RNPM_TXD_STAT_DD))) {
					rnpm_irq_disable_queues(q_vector);
					napi_schedule_irqoff(
						&q_vector->napi);
					goto do_self_napi;
				}
			}
		}
	}

	rnpm_for_each_ring(ring, q_vector->rx) {
		rx_desc = RNPM_RX_DESC(ring, ring->next_to_clean);
		if (rx_desc == NULL) {
			/* if one desc is null, mybe the verctor is freed, exit directly */
			goto do_self_napi;
		}

		if (rnpm_test_staterr(rx_desc, RNPM_RXD_STAT_DD)) {
			int size;

			size = le16_to_cpu(rx_desc->wb.len);
			if (size) {
				rnpm_irq_disable_queues(q_vector);
				napi_schedule_irqoff(&q_vector->napi);
			} else {
				struct rnpm_pf_adapter *pf_adapter =
					adapter->pf_adapter;
				set_bit(RNPM_PF_RESET, &pf_adapter->flags);
			}
			goto do_self_napi;
		}
	}

do_self_napi:
	clear_bit(RNPM_IRQ_MISS_HANDLE_DONE, &q_vector->flags);
	return HRTIMER_NORESTART;
}

/**
 * rnpm_alloc_q_vector - Allocate memory for a single interrupt vector
 * @adapter: board private structure to initialize
 * @v_count: q_vectors allocated on adapter, used for ring interleaving
 * @v_idx: index of vector in adapter struct
 * @txr_count: total number of Tx rings to allocate
 * @txr_idx: index of first Tx ring to allocate
 * @rxr_count: total number of Rx rings to allocate
 * @rxr_idx: index of first Rx ring to allocate
 *
 * We allocate one q_vector.  If allocation fails we return -ENOMEM.
 **/
static int rnpm_alloc_q_vector(struct rnpm_adapter *adapter,
			       int eth_queue_idx, int v_idx, int r_idx,
			       int r_count, int step)
{
	struct rnpm_q_vector *q_vector;
	struct rnpm_ring *ring;
	struct rnpm_hw *hw = &adapter->hw;
	int node = NUMA_NO_NODE;
	int cpu = -1;
	int ring_count, size;
	int txr_count, rxr_count, idx;
	int rxr_idx = r_idx, txr_idx = r_idx;

	DPRINTK(PROBE, INFO,
		"eth_queue_idx:%d v_idx:%d(off:%d) ring:%d ring_cnt:%d step:%d\n",
		eth_queue_idx, v_idx, adapter->vector_off, r_idx, r_count,
		step);
	txr_count = rxr_count = r_count;
	ring_count = txr_count + rxr_count;
	/* alloc ring memory together with q_vector */
	size = sizeof(struct rnpm_q_vector) +
	       (sizeof(struct rnpm_ring) * ring_count);

	/* should minis adapter->vector_off */
	if (cpu_online(v_idx - adapter->vector_off)) {
		cpu = v_idx - adapter->vector_off;
		node = cpu_to_node(cpu);
	}

	/* allocate q_vector and rings */
	q_vector = kzalloc_node(size, GFP_KERNEL, node);
	if (!q_vector)
		q_vector = kzalloc(size, GFP_KERNEL);
	if (!q_vector)
		return -ENOMEM;

	cpumask_copy(&q_vector->affinity_mask, cpu_possible_mask);
	/* setup affinity mask and node */
#ifdef RNPM_ADAPTER_QUEUE_IRQ_AFFINITY_START_CPU0
	if (cpu != -1)
		cpumask_set_cpu(cpu, &q_vector->affinity_mask);
#endif
	q_vector->numa_node = node;
	q_vector->irq_check_usecs = RNPM_IRQ_CHECK_USEC;
	hrtimer_init(&q_vector->irq_miss_check_timer, CLOCK_MONOTONIC,
		     HRTIMER_MODE_REL_PINNED);
	q_vector->irq_miss_check_timer.function = irq_miss_check;
	netif_napi_add_weight(adapter->netdev, &q_vector->napi, rnpm_poll,
			      adapter->napi_budge);
	adapter->q_vector[v_idx - adapter->vector_off] = q_vector;
	q_vector->adapter = adapter;
	q_vector->v_idx = v_idx;
	q_vector->tx.work_limit = adapter->tx_work_limit;

	q_vector->rx.itr = q_vector->itr = adapter->rx_frames;
#ifdef CONFIG_HZ
	q_vector->factor = DIV_ROUND_UP(1000, CONFIG_HZ);
#else
	q_vector->factor = 1;
#endif

	ring = q_vector->ring;
	for (idx = 0; idx < txr_count; idx++) {
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;
		ring->q_vector = q_vector;
		rnpm_add_ring(ring, &q_vector->tx);
		ring->count = adapter->tx_ring_item_count;
		ring->queue_index = eth_queue_idx + idx;
		ring->rnpm_queue_idx = txr_idx;
		ring->dma_int_stat =
			hw->hw_addr +
			RNPM_DMA_INT_STAT(ring->rnpm_queue_idx);
		ring->dma_int_mask = ring->dma_int_stat + 4;
		ring->dma_int_clr = ring->dma_int_stat + 8;
		ring->device_id = adapter->pdev->device;
		ring->pfvfnum = hw->pfvfnum;
		adapter->tx_ring[ring->queue_index] = ring;
		txr_idx += step;

		rnpm_dbg(
			"\t\t%s:vector[%d] <--RNPM TxRing:%d, eth_queue:%d\n",
			adapter->netdev->name, v_idx, ring->rnpm_queue_idx,
			ring->queue_index);
		ring++;
	}

	for (idx = 0; idx < rxr_count; idx++) {
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;
		ring->q_vector = q_vector;
		rnpm_add_ring(ring, &q_vector->rx);
		ring->count = adapter->rx_ring_item_count;
		ring->queue_index = eth_queue_idx + idx;
		ring->rnpm_queue_idx = rxr_idx;
		ring->dma_int_stat =
			hw->hw_addr +
			RNPM_DMA_INT_STAT(ring->rnpm_queue_idx);
		ring->dma_int_mask = ring->dma_int_stat + 4;
		ring->dma_int_clr = ring->dma_int_stat + 8;
		ring->device_id = adapter->pdev->device;
		ring->pfvfnum = hw->pfvfnum;

		adapter->rx_ring[ring->queue_index] = ring;
		rnpm_dbg(
			"\t\t%s:vector[%d] <--RNPM RxRing:%d, eth_queue:%d\n",
			adapter->netdev->name, v_idx, ring->rnpm_queue_idx,
			ring->queue_index);
		rxr_idx += step;
		ring++;
	}

	return 0;
}

/**
 * rnpm_free_q_vector - Free memory allocated for specific interrupt vector
 * @adapter: board private structure to initialize
 * @v_idx: Index of vector to be freed
 *
 * This function frees the memory allocated to the q_vector.  In addition if
 * NAPI is enabled it will delete any references to the NAPI struct prior
 * to freeing the q_vector.
 **/
static void rnpm_free_q_vector(struct rnpm_adapter *adapter, int v_idx)
{
	struct rnpm_q_vector *q_vector = adapter->q_vector[v_idx];
	struct rnpm_ring *ring;

	dbg_apt("v_idx:%d\n", v_idx);

	hrtimer_cancel(&q_vector->irq_miss_check_timer);
	rnpm_for_each_ring(ring, q_vector->tx)
		adapter->tx_ring[ring->queue_index] = NULL;
	rnpm_for_each_ring(ring, q_vector->rx)
		adapter->rx_ring[ring->queue_index] = NULL;
	adapter->q_vector[v_idx] = NULL;
	netif_napi_del(&q_vector->napi);
	kfree_rcu(q_vector, rcu);
}

/**
 * rnpm_alloc_q_vectors - Allocate memory for interrupt vectors
 * @adapter: board private structure to initialize
 *
 * We allocate one q_vector per queue interrupt.  If allocation fails we
 * return -ENOMEM.
 **/
static int rnpm_alloc_q_vectors(struct rnpm_adapter *adapter)
{
	int v_idx = adapter->vector_off;
	struct rnpm_hw *hw = &adapter->hw;
	int ring_idx = 0;
	int r_remaing =
		min_t(int, adapter->num_tx_queues, adapter->num_rx_queues);
	int ring_step = 1;
	int err, ring_cnt, v_remaing = adapter->num_q_vectors;
	int q_vector_nums = 0;

	if ((adapter->flags & RNPM_FLAG_SRIOV_ENABLED)) {
		ring_idx = 0;
		ring_idx = adapter->max_ring_pair_counts -
			   ring_step * PF_RING_CNT_WHEN_IOV_ENABLED;
		r_remaing = PF_RING_CNT_WHEN_IOV_ENABLED;
	}
	BUG_ON(adapter->num_q_vectors == 0);
	ring_idx = adapter->port;
	adapter->eth_queue_idx = 0;
	switch (hw->mode) {
	case MODE_NIC_MODE_1PORT:
		ring_step = 1;
		break;
	case MODE_NIC_MODE_1PORT_40G:
		ring_step = 1;
		break;
	case MODE_NIC_MODE_2PORT:
		ring_step = 2;
		break;
	case MODE_NIC_MODE_4PORT:
		ring_step = 4;
		break;
	}

	rnpm_dbg("r_remaing:%d, ring_step:%d num_q_vectors:%d\n",
		 r_remaing, ring_step, v_remaing);

	/* can support muti rings in one q_vector */
	for (; r_remaing > 0 && v_remaing > 0; v_remaing--) {
		ring_cnt = DIV_ROUND_UP(r_remaing, v_remaing);
		err = rnpm_alloc_q_vector(adapter, adapter->eth_queue_idx,
					  v_idx, ring_idx, ring_cnt,
					  ring_step);
		if (err)
			goto err_out;
		ring_idx += ring_step * ring_cnt;
		r_remaing -= ring_cnt;
		v_idx++;
		q_vector_nums++;
		adapter->eth_queue_idx += ring_cnt;
	}
	/* should fix the real used q_vectors_nums */
	adapter->num_q_vectors = q_vector_nums;
	return 0;
err_out:
	adapter->num_tx_queues = 0;
	adapter->num_rx_queues = 0;
	adapter->num_q_vectors = 0;
	while (v_idx--)
		rnpm_free_q_vector(adapter, v_idx);
	return -ENOMEM;
}

/**
 * rnpm_free_q_vectors - Free memory allocated for interrupt vectors
 * @adapter: board private structure to initialize
 *
 * This function frees the memory allocated to the q_vectors.  In addition if
 * NAPI is enabled it will delete any references to the NAPI struct prior
 * to freeing the q_vector.
 **/
static void rnpm_free_q_vectors(struct rnpm_adapter *adapter)
{
	int v_idx = adapter->num_q_vectors;

	adapter->num_rx_queues = 0;
	adapter->num_tx_queues = 0;
	adapter->num_q_vectors = 0;
	while (v_idx--)
		rnpm_free_q_vector(adapter, v_idx);
}

static void rnpm_reset_interrupt_capability(struct rnpm_adapter *adapter)
{
	kfree(adapter->msix_entries);
	adapter->msix_entries = NULL;
}

/**
 * rnpm_set_interrupt_capability - set MSI-X or MSI if supported
 * @adapter: board private structure to initialize
 *
 * Attempt to configure the interrupts using the best available
 * capabilities of the hardware and the kernel.
 **/
static int rnpm_set_interrupt_capability(struct rnpm_adapter *adapter)
{
	struct rnpm_pf_adapter *pf_adapter = adapter->pf_adapter;
	int v_budget, err = 0;
	int msix_offset;

	v_budget = adapter->max_ring_pair_counts;
	/* in one ring mode should reset v_budget */
	v_budget = min_t(int, v_budget, num_online_cpus());
	v_budget = min_t(int, v_budget, adapter->max_msix_counts);

	adapter->msix_entries =
		kcalloc(v_budget, sizeof(struct msix_entry), GFP_KERNEL);
	if (!adapter->msix_entries) {
		netdev_err(adapter->netdev, "alloc msix_entries faild!\n");
		return -EINVAL;
	}
	dbg_apt("[%s] adapter:%p msix_entry:%p vector:%d\n", __func__,
		adapter, adapter->msix_entries, adapter->vector_off);
	msix_offset = adapter->vector_off;
	memcpy((u8 *)adapter->msix_entries,
	       (u8 *)pf_adapter->msix_entries +
		       sizeof(struct msix_entry) * msix_offset,
	       sizeof(struct msix_entry) * v_budget);

	adapter->num_q_vectors = min(v_budget, adapter->max_q_vectors);

	rnpm_dbg(
		"adapter%d alloc vectors: cnt:%d [%d~%d] num_q_vectors:%d msix_offset %d\n",
		adapter->bd_number, v_budget, adapter->vector_off,
		adapter->vector_off + v_budget - 1, adapter->num_q_vectors,
		msix_offset);
	return err;
}

/**
 * rnpm_init_interrupt_scheme - Determine proper interrupt scheme
 * @adapter: board private structure to initialize
 *
 * We determine which interrupt scheme to use based on...
 * - Hardware queue count (num_*_queues)
 *   - defined by miscellaneous hardware support/features (RSS, etc.)
 **/
int rnpm_init_interrupt_scheme(struct rnpm_adapter *adapter)
{
	int err;

	/* Number of supported queues */
	rnpm_set_num_queues(adapter);
	/* Set interrupt mode */
	rnpm_set_interrupt_capability(adapter);
	err = rnpm_alloc_q_vectors(adapter);
	if (err) {
		e_dev_err("Unable to allocate memory for queue vectors\n");
		goto err_alloc_q_vectors;
	}
	rnpm_cache_ring_register(adapter);
	DPRINTK(PROBE, INFO,
		"Multiqueue %s: Rx Queue count = %u, Tx Queue count = %u\n\n",
		(adapter->num_rx_queues > 1) ? "Enabled" : "Disabled",
		adapter->num_rx_queues, adapter->num_tx_queues);
	set_bit(__RNPM_DOWN, &adapter->state);
	return 0;
err_alloc_q_vectors:
	rnpm_reset_interrupt_capability(adapter);
	return err;
}

/**
 * rnpm_clear_interrupt_scheme - Clear the current interrupt scheme settings
 * @adapter: board private structure to clear interrupt scheme on
 *
 * We go through and clear interrupt specific resources and reset the structure
 * to pre-load conditions
 **/
void rnpm_clear_interrupt_scheme(struct rnpm_adapter *adapter)
{
	adapter->num_tx_queues = 0;
	adapter->num_rx_queues = 0;

	rnpm_free_q_vectors(adapter);
	rnpm_reset_interrupt_capability(adapter);
}

/**
 * rnpm_tx_ctxtdesc - Send a control desc to hw
 * @tx_ring: target ring of this control desc
 * @mss_seg_len: mss length
 * @l4_hdr_len:  l4 length
 * @tunnel_hdr_len: tunnel_hdr_len
 * @inner_vlan_tag: inner_vlan_tag
 * @type_tucmd: cmd
 *
 **/
void rnpm_tx_ctxtdesc(struct rnpm_ring *tx_ring, u32 mss_len_vf_num,
		      u32 inner_vlan_tunnel_len, u32 type_tucmd)
{
	struct rnpm_tx_ctx_desc *context_desc;
	u16 i = tx_ring->next_to_use;

	context_desc = RNPM_TX_CTXTDESC(tx_ring, i);

	i++;
	tx_ring->next_to_use = (i < tx_ring->count) ? i : 0;
	type_tucmd |= RNPM_TXD_CTX_CTRL_DESC;
	if (inner_vlan_tunnel_len & 0x00ffff00)
		type_tucmd |= RNPM_TXD_CMD_INNER_VLAN;
	context_desc->mss_len_vf_num = cpu_to_le32(mss_len_vf_num);
	context_desc->inner_vlan_tunnel_len =
		cpu_to_le32(inner_vlan_tunnel_len);
	context_desc->resv_cmd = cpu_to_le32(type_tucmd);
	context_desc->resv = 0;
#ifdef RNPM_IOV_VEB_BUG_NOT_FIXED
	if (tx_ring->q_vector->adapter->flags & RNPM_FLAG_SRIOV_ENABLED)
		context_desc->inner_vlan_tunnel_len |= VF_VEB_MARK;
#endif
	buf_dump_line("ctx  ", __LINE__, context_desc,
		      sizeof(*context_desc));
}

void rnpm_maybe_tx_ctxtdesc(struct rnpm_ring *tx_ring,
			    struct rnpm_tx_buffer *first, u32 type_tucmd)
{
	struct rnpm_adapter *adapter = netdev_priv((tx_ring)->netdev);

	if (first->ctx_flag) {
		if (adapter->priv_flags & RNPM_PRIV_FLAG_TX_PADDING) {
			if (!first->gso_need_padding)
				type_tucmd |= RNPM_TXD_MTI_CRC_PAD_CTRL;
			first->gso_need_padding = false;
		}
		rnpm_tx_ctxtdesc(tx_ring, first->mss_len_vf_num,
				 first->inner_vlan_tunnel_len, type_tucmd);
	}
}

void rnpm_store_reta(struct rnpm_adapter *adapter)
{
	u32 i, reta_entries = rnpm_rss_indir_tbl_entries(adapter);
	struct rnpm_hw *hw = &adapter->hw;
	u32 reta = 0, port_offset;
	u32 port = adapter->port;
	struct rnpm_ring *rx_ring;

	for (i = 0; i < reta_entries; i++) {
		if (adapter->flags & RNPM_FLAG_SRIOV_ENABLED) {
			reta = adapter->rss_indir_tbl[i];
		} else {
			rx_ring =
				adapter->rx_ring[adapter->rss_indir_tbl[i]];
			if (adapter->flags & RNPM_FLAG_RXHASH_DISABLE) {
				reta = adapter->port;
			} else {
				port_offset = rd32(
					hw, RNPM_ETH_TC_PORT_OFFSET_TABLE(
						    adapter->port));
				reta = rx_ring->rnpm_queue_idx -
				       port_offset;
			}
		}
		if (hw->rss_type == rnpm_rss_n10)
			wr32(hw, RNPM_ETH_RSS_INDIR_TBL(port, i), reta);
	}
}

void rnpm_store_key(struct rnpm_pf_adapter *pf_adapter)
{
	u8 *key = pf_adapter->rss_key;
	int key_len = RNPM_RSS_KEY_SIZE;
	u8 key_temp[RNPM_RSS_KEY_SIZE];
	u32 *value, i;

	for (i = 0; i < key_len; i++)
		*(key_temp + key_len - i - 1) = *(key + i);
	value = (u32 *)key_temp;
	for (i = 0; i < key_len; i = i + 4)
		rnpm_wr_reg(pf_adapter->hw_addr + RNPM_ETH_RSS_KEY + i,
			    *(value + i / 4));
}

int rnpm_init_rss_key(struct rnpm_pf_adapter *pf_adapter)
{
	unsigned long flags;
#ifdef DEBUG_RSS
	u8 temp[] = { 0xca, 0xf9, 0x8f, 0x24, 0xc2, 0x10, 0x50, 0x22,
		      0x1f, 0x6c, 0xec, 0xc8, 0xd5, 0x9d, 0x8c, 0xa6,
		      0x96, 0x0b, 0x50, 0xf9, 0x24, 0x89, 0x74, 0x96,
		      0xf2, 0xbd, 0xbe, 0xbc, 0x5c, 0x81, 0xb2, 0x06,
		      0x3d, 0xb4, 0x08, 0x56, 0xca, 0x0c, 0x62, 0x1a };
#endif
	u32 iov_en = 0;

	spin_lock_irqsave(&pf_adapter->key_setup_lock, flags);
	if (!pf_adapter->rss_key_setup_flag) {
#ifdef DEBUG_RSS
		memcpy(pf_adapter->rss_key, temp, RNPM_RSS_KEY_SIZE);
#else
		netdev_rss_key_fill(pf_adapter->rss_key,
				    RNPM_RSS_KEY_SIZE);
#endif
		pf_adapter->rss_key_setup_flag = 1;
	}
	rnpm_store_key(pf_adapter);
	wr32(pf_adapter, RNPM_ETH_RSS_CONTROL,
	     RNPM_ETH_ENABLE_RSS_ONLY | iov_en);
	spin_unlock_irqrestore(&pf_adapter->key_setup_lock, flags);
	return 0;
}

int rnpm_init_rss_table(struct rnpm_adapter *adapter)
{
	int rx_nums = adapter->num_rx_queues;
	int i, j, port_offset;
	struct rnpm_hw *hw = &adapter->hw;
	struct rnpm_ring *rx_ring;
	u32 reta = 0;
	u32 reta_entries = rnpm_rss_indir_tbl_entries(adapter);
	u32 port = adapter->port;

	for (i = 0, j = 0; i < reta_entries; i++) {
		if (!adapter->rss_tbl_setup_flag)
			adapter->rss_indir_tbl[i] = j;
		if (adapter->flags & RNPM_FLAG_SRIOV_ENABLED)
			reta = j;
		else {
			rx_ring =
				adapter->rx_ring[adapter->rss_indir_tbl[i]];
			if (adapter->flags & RNPM_FLAG_RXHASH_DISABLE) {
				reta = adapter->port;
			} else {
				port_offset = rd32(
					hw, RNPM_ETH_TC_PORT_OFFSET_TABLE(
						    adapter->port));
				reta = rx_ring->rnpm_queue_idx -
				       port_offset;
			}
		}
		if (hw->rss_type == rnpm_rss_n10)
			wr32(hw, RNPM_ETH_RSS_INDIR_TBL(port, i), reta);
		j = (j + 1) % rx_nums;
	}
	adapter->rss_tbl_setup_flag = 1;
	for (i = 0, j = 0; i < reta_entries; i++) {
		dbg_apt("indir %d table is %d\n", i,
			adapter->rss_indir_tbl[i]);
		dbg_apt("reg %x is %d\n", RNPM_ETH_RSS_INDIR_TBL(port, i),
			rd32(hw, RNPM_ETH_RSS_INDIR_TBL(port, i)));
	}
	return 0;
}

void rnpm_setup_dma_rx(struct rnpm_adapter *adapter, int count_in_dw)
{
	struct rnpm_hw *hw = &adapter->hw;
	u32 data;

	data = rd32(hw, RNPM_DMA_CONFIG);
	data &= (0x00000ffff);
	data |= (count_in_dw << 16);
	wr32(hw, RNPM_DMA_CONFIG, data);
}

static void rnpm_setup_layer2_remapping(struct rnpm_hw *hw,
					union rnpm_atr_input *input,
					u16 hw_id, u8 queue)
{
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	u8 offset = adapter->port;

	drection_dbg("try to eable layer2 %x\n",
		     input->layer2_format.proto);
	wr32(hw, RNPM_ETH_LAYER2_ETQF(hw_id),
	     (0x1 << 31) | (ntohs(input->layer2_format.proto)));
	if (queue == RNPM_FDIR_DROP_QUEUE)
		wr32(hw, RNPM_ETH_LAYER2_ETQS(hw_id), (0x1 << 31));
	else
		wr32(hw, RNPM_ETH_LAYER2_ETQS(hw_id),
		     (0x1 << 30) | ((queue - offset) << 20));
}

static void rnpm_setup_tuple5_remapping(struct rnpm_hw *hw,
					union rnpm_atr_input *input,
					u16 hw_id, u8 queue)
{
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	u8 offset = adapter->port;
	u32 port = 0;
	u8 mask_temp = 0;
	u8 l4_proto_type = 0;

	drection_dbg("try to eable tuple 5 %x\n", hw_id);
	if (input->formatted.src_ip[0] != 0)
		wr32(hw, RNPM_ETH_TUPLE5_SAQF(hw_id),
		     htonl(input->formatted.src_ip[0]));
	else
		mask_temp |= RNPM_SRC_IP_MASK;

	if (input->formatted.dst_ip[0] != 0) {
		wr32(hw, RNPM_ETH_TUPLE5_DAQF(hw_id),
		     htonl(input->formatted.dst_ip[0]));
	} else {
		mask_temp |= RNPM_DST_IP_MASK;
	}

	if (input->formatted.src_port != 0)
		port |= (htons(input->formatted.src_port));
	else
		mask_temp |= RNPM_SRC_PORT_MASK;

	if (input->formatted.dst_port != 0)
		port |= (htons(input->formatted.dst_port) << 16);
	else
		mask_temp |= RNPM_DST_PORT_MASK;

	if (port != 0)
		wr32(hw, RNPM_ETH_TUPLE5_SDPQF(hw_id), port);

	switch (input->formatted.flow_type) {
	case RNPM_ATR_FLOW_TYPE_TCPV4:
		l4_proto_type = IPPROTO_TCP;
		break;
	case RNPM_ATR_FLOW_TYPE_UDPV4:
		l4_proto_type = IPPROTO_UDP;
		break;
	case RNPM_ATR_FLOW_TYPE_SCTPV4:
		l4_proto_type = IPPROTO_SCTP;
		break;
	case RNPM_ATR_FLOW_TYPE_IPV4:
		l4_proto_type = input->formatted.inner_mac[0];
		break;
	default:
		l4_proto_type = 0;
	}

	if (l4_proto_type == 0)
		mask_temp |= RNPM_L4_PROTO_MASK;

	wr32(hw, RNPM_ETH_TUPLE5_FTQF(hw_id),
	     (1 << 31) | (mask_temp << 25) | (l4_proto_type << 16) | 0x3);
	/* setup action */
	if (queue == RNPM_FDIR_DROP_QUEUE)
		wr32(hw, RNPM_ETH_TUPLE5_POLICY(hw_id), (0x1 << 31));
	else
		wr32(hw, RNPM_ETH_TUPLE5_POLICY(hw_id),
		     ((0x1 << 30) | ((queue - offset) << 20)));
}

struct tcam_val {
	unsigned short dst_port;
	unsigned short src_port;
	unsigned int dst_ip;
	unsigned int src_ip;

	unsigned char proto;
	unsigned char ring_num;
	unsigned char rvd1;
	unsigned char port_num : 4;
	unsigned char mark_valid : 1;
	unsigned char port_valid : 1;
	unsigned char ring_valid : 1;
	unsigned char flag : 1;
};

struct tcam_mask {
	unsigned short dst_port_mask;
	unsigned short src_port_mask;
	unsigned int dst_ip_mask;
	unsigned int src_ip_mask;

	unsigned char proto_mask;
	unsigned char ring_num_mask;
	unsigned char rvd1;
	unsigned char action_mask;
};

#define RPU_TCAM_ENABLE_REG(hw) (hw->hw_addr + 0x18024)
#define RPU_TCAM_CONFIG_ENABLE_REG(hw) (hw->hw_addr + 0x38050)
#define RPU_TCAM_MODE_REG(hw) (hw->hw_addr + 0xe0000)
#define RPU_TCAM_CACHE_REG(hw) (hw->hw_addr + 0xe0004)
#define RPU_TCAM_POLICY_BASE_REG(hw) (hw->hw_addr + 0xc0000)
#define RNPM_IPSEC_TCAM_POLICY_BASE_ADDR(hw) (hw->hw_addr + 0xc0000)
#define RNPM_IPSEC_TCAM_GLB_BLOCK (0x40)
#define RNPM_IPSEC_TCAM_IN_BLOCK (0x2)
#define RNPM_IPSEC_TCAM_CNT (4096)
#define RNPM_IPSEC_TCAM_MASK_DEF_VAL (0xFFFFFFFF)

int rnpm_esp_tcam_enable(struct rnpm_pf_adapter *pf_adapter)
{
	struct rnpm_hw *hw = NULL;
	int i = 0;

	if (pf_adapter == NULL) {
		dev_err(&pf_adapter->pdev->dev, "%s Params Input Error!\n",
			__func__);
		return -EINVAL;
	}

	hw = &pf_adapter->hw;
	rnpm_wr_reg(RPU_TCAM_ENABLE_REG(hw), 0x1);
	rnpm_wr_reg(RPU_TCAM_CONFIG_ENABLE_REG(hw), 0x1);
	rnpm_wr_reg(RPU_TCAM_MODE_REG(hw), 0x1);
	rnpm_wr_reg(RPU_TCAM_CACHE_REG(hw), 0x0);

	rnpm_wr_reg(RPU_TCAM_MODE_REG(hw), 0x2);
	for (i = 0; i < (0x20 * 4096 / 4); i++)
		rnpm_wr_reg(RPU_TCAM_POLICY_BASE_REG(hw) + i * 4, 0x0);
	rnpm_wr_reg(RPU_TCAM_MODE_REG(hw), 0x1);
	return 0;
}

void rnpm_set_esp_to_mpe_tcam_rule(struct rnpm_pf_adapter *pf_adapter)
{
	int g_cnt = 0, i_cnt = 0;
	void *val_addr = NULL, *mask_addr = NULL;
	u32 a_val[4] = { 0 }, a_mask[4] = { 0 };

	struct tcam_val t_val;
	struct tcam_mask t_mask;
	struct rnpm_hw *hw = NULL;

	if (pf_adapter == NULL) {
		dev_err(&pf_adapter->pdev->dev, "%s Params Input Error!\n",
			__func__);
		return;
	}

	memset(&t_val, 0, sizeof(t_val));
	memset(&t_mask, 0, sizeof(t_mask));

	hw = &pf_adapter->hw;
	if (rnpm_is_pf1(pf_adapter->pdev))
		t_val.port_num = 0xa;
	else
		t_val.port_num = 0xb;
	t_val.mark_valid = 0;
	t_val.port_valid = 1;
	t_val.ring_valid = 0;
	t_val.flag = 0;
	t_val.proto = 50;
	t_mask.src_ip_mask = 0;
	t_mask.proto_mask = 0xff;
	val_addr = RNPM_IPSEC_TCAM_POLICY_BASE_ADDR(hw) + g_cnt * 0x40 +
		   i_cnt * 0x10;
	mask_addr = RNPM_IPSEC_TCAM_POLICY_BASE_ADDR(hw) + g_cnt * 0x40 +
		    i_cnt * 0x10 + 0x20;
	rnpm_wr_reg(RPU_TCAM_MODE_REG(hw), 0x2);
	memcpy(&a_val, &t_val, sizeof(t_val));
	memcpy(&a_mask, &t_mask, sizeof(t_val));
	iowrite32(a_val[0], val_addr + 0x0);
	iowrite32(a_val[1], val_addr + 0x4);
	iowrite32(a_val[2], val_addr + 0x8);
	iowrite32(a_val[3], val_addr + 0xc);
	iowrite32(a_mask[0], mask_addr + 0x0);
	iowrite32(a_mask[1], mask_addr + 0x4);
	iowrite32(a_mask[2], mask_addr + 0x8);
	iowrite32(a_mask[3], mask_addr + 0xc);
	rnpm_wr_reg(RPU_TCAM_MODE_REG(hw), 0x1);
}

static void rnpm_setup_tuple5_remapping_tcam(struct rnpm_hw *hw,
					     union rnpm_atr_input *input,
					     u16 hw_id, u8 queue)
{
	u32 port = 0;
	u32 port_mask = 0;
	u8 l4_proto_type = 0;
	u8 l4_proto_mask = 0xff;
	u32 action = 0;
	u32 mark = 0;
	struct rnpm_adapter *adapter = (struct rnpm_adapter *)hw->back;
	u8 offset = adapter->port;

	wr32(hw, RNPM_TCAM_MODE, 2);
	drection_dbg("try to eable tcam %x\n", hw_id);
	if (input->formatted.src_ip[0] != 0) {
		wr32(hw, RNPM_TCAM_SAQF(hw_id),
		     htonl(input->formatted.src_ip[0]));
		wr32(hw, RNPM_TCAM_SAQF_MASK(hw_id),
		     htonl(input->formatted.src_ip_mask[0]));
	} else {
		wr32(hw, RNPM_TCAM_SAQF(hw_id), 0);
		wr32(hw, RNPM_TCAM_SAQF_MASK(hw_id), 0);
	}
	if (input->formatted.dst_ip[0] != 0) {
		wr32(hw, RNPM_TCAM_DAQF(hw_id),
		     htonl(input->formatted.dst_ip[0]));
		wr32(hw, RNPM_TCAM_DAQF_MASK(hw_id),
		     htonl(input->formatted.dst_ip_mask[0]));
	} else {
		wr32(hw, RNPM_TCAM_DAQF(hw_id), 0);
		wr32(hw, RNPM_TCAM_DAQF_MASK(hw_id), 0);
	}
	if (input->formatted.src_port != 0) {
		port |= (htons(input->formatted.src_port) << 16);
		port_mask |= (htons(input->formatted.src_port_mask) << 16);
	}

	if (input->formatted.dst_port != 0) {
		port |= (htons(input->formatted.dst_port));
		port_mask |= (htons(input->formatted.src_port_mask));
	}

	/* setup src & dst port */
	if (port != 0) {
		wr32(hw, RNPM_TCAM_SDPQF(hw_id), port);
		wr32(hw, RNPM_TCAM_SDPQF_MASK(hw_id), port_mask);
	} else {
		wr32(hw, RNPM_TCAM_SDPQF(hw_id), 0);
		wr32(hw, RNPM_TCAM_SDPQF_MASK(hw_id), 0);
	}

	switch (input->formatted.flow_type) {
	case RNPM_ATR_FLOW_TYPE_TCPV4:
		l4_proto_type = IPPROTO_TCP;
		break;
	case RNPM_ATR_FLOW_TYPE_UDPV4:
		l4_proto_type = IPPROTO_UDP;
		break;
	case RNPM_ATR_FLOW_TYPE_SCTPV4:
		l4_proto_type = IPPROTO_SCTP;
		break;
	case RNPM_ATR_FLOW_TYPE_IPV4:
		l4_proto_type = input->formatted.inner_mac[0];
		l4_proto_mask = input->formatted.inner_mac_mask[0];
		break;
	default:
		l4_proto_type = 0;
		l4_proto_mask = 0;
	}

	if (l4_proto_type != 0) {
		action |= l4_proto_type;
		mark |= l4_proto_mask;
	}

	/* setup action */
	if (queue == RNPM_FDIR_DROP_QUEUE) {
		wr32(hw, RNPM_TCAM_APQF(hw_id), (0x1 << 31) | action);
		wr32(hw, RNPM_TCAM_APQF_MASK(hw_id), mark);
	} else {
		/* setup ring_number */
		wr32(hw, RNPM_TCAM_APQF(hw_id),
		     ((0x1 << 30) | ((queue - offset) << 16) | action));
		wr32(hw, RNPM_TCAM_APQF_MASK(hw_id), mark);
	}
	wr32(hw, RNPM_TCAM_MODE, 1);
}

s32 rnpm_fdir_write_perfect_filter(int fdir_mode, struct rnpm_hw *hw,
				   union rnpm_atr_input *filter, u16 hw_id,
				   u8 queue)
{
	if (filter->formatted.flow_type == RNPM_ATR_FLOW_TYPE_ETHER) {
		rnpm_setup_layer2_remapping(hw, filter, hw_id, queue);
	} else {
		if (fdir_mode != fdir_mode_tcam)
			rnpm_setup_tuple5_remapping(hw, filter, hw_id,
						    queue);
		else
			rnpm_setup_tuple5_remapping_tcam(hw, filter, hw_id,
							 queue);
	}
	return 0;
}

int rnpm_card_partially_supported_10g_1g_sfp(
	struct rnpm_pf_adapter *pf_adapter)
{
	if (pf_adapter && (pf_adapter->hw.ablity_speed == SPEED_10000))
		return 1;
	return 0;
}

s32 rnpm_fdir_erase_perfect_filter(int fdir_mode, struct rnpm_hw *hw,
				   union rnpm_atr_input *input, u16 hw_id)
{
	/* just diable filter */
	if (input->formatted.flow_type == RNPM_ATR_FLOW_TYPE_ETHER) {
		wr32(hw, RNPM_ETH_LAYER2_ETQF(hw_id), 0);
		dbg_hw("disble layer2 %d\n", hw_id);
	} else {
		if (fdir_mode != fdir_mode_tcam) {
			wr32(hw, RNPM_ETH_TUPLE5_FTQF(hw_id), 0);
			dbg_hw("disble tuple5 %d\n", hw_id);
		} else {
			/* earase tcam */
			wr32(hw, RNPM_TCAM_MODE, 2);
			wr32(hw, RNPM_TCAM_SAQF(hw_id), 0);
			wr32(hw, RNPM_TCAM_SAQF_MASK(hw_id), 0);
			wr32(hw, RNPM_TCAM_DAQF(hw_id), 0);
			wr32(hw, RNPM_TCAM_DAQF_MASK(hw_id), 0);
			wr32(hw, RNPM_TCAM_SDPQF(hw_id), 0);
			wr32(hw, RNPM_TCAM_SDPQF_MASK(hw_id), 0);
			wr32(hw, RNPM_TCAM_APQF(hw_id), 0);
			wr32(hw, RNPM_TCAM_APQF_MASK(hw_id), 0);
			wr32(hw, RNPM_TCAM_MODE, 1);
			/* update tcam cache */
			wr32(hw, RNPM_TCAM_CACHE_ADDR_CLR, 0);
			wr32(hw, RNPM_TCAM_CACHE_REQ_CLR, 0);
		}
	}

	return 0;
}

u32 rnpm_tx_desc_unused_sw(struct rnpm_ring *tx_ring)
{
	u16 ntu = tx_ring->next_to_use;
	u16 ntc = tx_ring->next_to_clean;
	u16 count = tx_ring->count;

	return ((ntu >= ntc) ? (count - ntu + ntc) : (ntc - ntu));
}

u32 rnpm_rx_desc_used_hw(struct rnpm_hw *hw, struct rnpm_ring *rx_ring)
{
	u32 head = rd32(hw, RNPM_DMA_REG_RX_DESC_BUF_HEAD(
				    rx_ring->rnpm_queue_idx));
	u32 tail = rd32(hw, RNPM_DMA_REG_RX_DESC_BUF_TAIL(
				    rx_ring->rnpm_queue_idx));
	u16 count = rx_ring->count;

	return ((tail >= head) ? (count - tail + head) : (head - tail));
}

u32 rnpm_tx_desc_unused_hw(struct rnpm_hw *hw, struct rnpm_ring *tx_ring)
{
	u32 head = rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_HEAD(
				    tx_ring->rnpm_queue_idx));
	u32 tail = rd32(hw, RNPM_DMA_REG_TX_DESC_BUF_TAIL(
				    tx_ring->rnpm_queue_idx));
	u16 count = tx_ring->count;

	return ((tail >= head) ? (count - tail + head) : (head - tail));
}

s32 rnpm_disable_rxr_maxrate(struct net_device *netdev, u8 queue_index)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	struct rnpm_ring *rx_ring = adapter->rx_ring[queue_index];
	u32 reg_idx = rx_ring->rnpm_queue_idx;

	/* disable which dma ring in maxrate limit mode */
	wr32(hw, RNPM_SELECT_RING_EN(reg_idx), 0);
	/* Clear Tx Ring maxrate */
	wr32(hw, RNPM_RX_RING_MAXRATE(reg_idx), 0);
	return 0;
}

s32 rnpm_enable_rxr_maxrate(struct net_device *netdev, u8 queue_index,
			    u32 maxrate)
{
	struct rnpm_adapter *adapter = netdev_priv(netdev);
	struct rnpm_hw *hw = &adapter->hw;
	struct rnpm_ring *rx_ring = adapter->rx_ring[queue_index];
	u32 reg_idx = rx_ring->rnpm_queue_idx;
	u32 real_rate = maxrate / 16;

	if (!real_rate)
		return -EINVAL;

	wr32(hw, RNPM_RING_FC_ENABLE, true);
	/* disable which dma ring in maxrate limit mode */
	wr32(hw, RNPM_SELECT_RING_EN(reg_idx), true);
	/* Clear Tx Ring maxrate */
	wr32(hw, RNPM_RX_RING_MAXRATE(reg_idx), real_rate);
	return 0;
}
