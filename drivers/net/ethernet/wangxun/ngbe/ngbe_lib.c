/*
 * WangXun Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 */


#include "ngbe.h"
#include "ngbe_sriov.h"


/**
 * ngbe_cache_ring_vmdq - Descriptor ring to register mapping for VMDq
 * @adapter: board private structure to initialize
 *
 * Cache the descriptor ring offsets for VMDq to the assigned rings.  It
 * will also try to cache the proper offsets if RSS/FCoE/SRIOV are enabled along
 * with VMDq.
 *
 **/
static bool ngbe_cache_ring_vmdq(struct ngbe_adapter *adapter)
{
	struct ngbe_ring_feature *vmdq = &adapter->ring_feature[RING_F_VMDQ];
	//struct ngbe_ring_feature *rss = &adapter->ring_feature[RING_F_RSS];
	int i;
	u16 reg_idx;

	/* only proceed if VMDq is enabled */
	if (!(adapter->flags & NGBE_FLAG_VMDQ_ENABLED))
		return false;

	/* start at VMDq register offset for SR-IOV enabled setups */
	reg_idx = vmdq->offset;

	for (i = 0; i < adapter->num_rx_queues; i++) {

		/* If we are greater than indices move to next pool */
		adapter->rx_ring[i]->reg_idx = reg_idx + i;
	}

	reg_idx = vmdq->offset;
	for (i = 0; i < adapter->num_tx_queues; i++) {

		/* If we are greater than indices move to next pool */
		adapter->tx_ring[i]->reg_idx = reg_idx + i;
	}

	return true;
}

/**
 * ngbe_cache_ring_rss - Descriptor ring to register mapping for RSS
 * @adapter: board private structure to initialize
 *
 * Cache the descriptor ring offsets for RSS, ATR, FCoE, and SR-IOV.
 *
 **/
static bool ngbe_cache_ring_rss(struct ngbe_adapter *adapter)
{
	u16 i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		adapter->rx_ring[i]->reg_idx = i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		adapter->tx_ring[i]->reg_idx = i;

	return true;
}

/**
 * ngbe_cache_ring_register - Descriptor ring to register mapping
 * @adapter: board private structure to initialize
 *
 * Once we know the feature-set enabled for the device, we'll cache
 * the register offset the descriptor ring is assigned to.
 *
 * Note, the order the various feature calls is important.  It must start with
 * the "most" features enabled at the same time, then trickle down to the
 * least amount of features turned on at once.
 **/
static void ngbe_cache_ring_register(struct ngbe_adapter *adapter)
{
	if (ngbe_cache_ring_vmdq(adapter))
		return;

	ngbe_cache_ring_rss(adapter);
}

#define NGBE_RSS_64Q_MASK      0x3F
#define NGBE_RSS_16Q_MASK      0xF
#define NGBE_RSS_8Q_MASK       0x7
#define NGBE_RSS_4Q_MASK       0x3
#define NGBE_RSS_2Q_MASK       0x1
#define NGBE_RSS_DISABLED_MASK 0x0


/**
 * ngbe_set_vmdq_queues: Allocate queues for VMDq devices
 * @adapter: board private structure to initialize
 *
 * When VMDq (Virtual Machine Devices queue) is enabled, allocate queues
 * and VM pools where appropriate.  If RSS is available, then also try and
 * enable RSS and map accordingly.
 *
 **/
static bool ngbe_set_vmdq_queues(struct ngbe_adapter *adapter)
{
	u16 vmdq_i = adapter->ring_feature[RING_F_VMDQ].limit;
	u16 vmdq_m = 0;
	u16 rss_i = adapter->ring_feature[RING_F_RSS].limit;
	u16 rss_m = NGBE_RSS_DISABLED_MASK;

	/* only proceed if VMDq is enabled */
	if (!(adapter->flags & NGBE_FLAG_VMDQ_ENABLED))
		return false;

	/* Add starting offset to total pool count */
	vmdq_i += adapter->ring_feature[RING_F_VMDQ].offset;

	/* double check we are limited to maximum pools */
	vmdq_i = min_t(u16, NGBE_MAX_VMDQ_INDICES, vmdq_i);

	/* when VMDQ on, disable RSS */
	rss_i = 1;

	/* remove the starting offset from the pool count */
	vmdq_i -= adapter->ring_feature[RING_F_VMDQ].offset;

	/* save features for later use */
	adapter->ring_feature[RING_F_VMDQ].indices = vmdq_i;
	adapter->ring_feature[RING_F_VMDQ].mask = vmdq_m;

	/* limit RSS based on user input and save for later use */
	adapter->ring_feature[RING_F_RSS].indices = rss_i;
	adapter->ring_feature[RING_F_RSS].mask = rss_m;

	adapter->queues_per_pool = rss_i;
	adapter->num_rx_queues = vmdq_i * rss_i;

#ifdef HAVE_TX_MQ
	adapter->num_tx_queues = vmdq_i * rss_i;
#else
	adapter->num_tx_queues = vmdq_i;
#endif /* HAVE_TX_MQ */

	return true;
}

/**
 * ngbe_set_rss_queues: Allocate queues for RSS
 * @adapter: board private structure to initialize
 *
 * This is our "base" multiqueue mode.  RSS (Receive Side Scaling) will try
 * to allocate one Rx queue per CPU, and if available, one Tx queue per CPU.
 *
 **/
static bool ngbe_set_rss_queues(struct ngbe_adapter *adapter)
{
	struct ngbe_ring_feature *f;
	u16 rss_i;

	/* set mask for 16 queue limit of RSS */
	f = &adapter->ring_feature[RING_F_RSS];
	rss_i = f->limit;

	f->indices = rss_i;
	f->mask = NGBE_RSS_8Q_MASK;

	adapter->num_rx_queues = rss_i;
#ifdef HAVE_TX_MQ
	adapter->num_tx_queues = rss_i;
#endif

	return true;
}

/*
 * ngbe_set_num_queues: Allocate queues for device, feature dependent
 * @adapter: board private structure to initialize
 *
 * This is the top level queue allocation routine.  The order here is very
 * important, starting with the "most" number of features turned on at once,
 * and ending with the smallest set of features.  This way large combinations
 * can be allocated if they're turned on, and smaller combinations are the
 * fallthrough conditions.
 *
 **/
static void ngbe_set_num_queues(struct ngbe_adapter *adapter)
{
	/* Start with base case */
	adapter->num_rx_queues = 1;
	adapter->num_tx_queues = 1;
	adapter->queues_per_pool = 1;

	if (ngbe_set_vmdq_queues(adapter))
		return;

	ngbe_set_rss_queues(adapter);

}

/**
 * ngbe_acquire_msix_vectors - acquire MSI-X vectors
 * @adapter: board private structure
 *
 * Attempts to acquire a suitable range of MSI-X vector interrupts. Will
 * return a negative error code if unable to acquire MSI-X vectors for any
 * reason.
 */
static int ngbe_acquire_msix_vectors(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	int i, vectors, vector_threshold;

	if (!(adapter->flags & NGBE_FLAG_MSIX_CAPABLE))
		return -EOPNOTSUPP;

	/* We start by asking for one vector per queue pair */
	vectors = max(adapter->num_rx_queues, adapter->num_tx_queues);

	/* It is easy to be greedy for MSI-X vectors. However, it really
	 * doesn't do much good if we have a lot more vectors than CPUs. We'll
	 * be somewhat conservative and only ask for (roughly) the same number
	 * of vectors as there are CPUs.
	 */
	vectors = min_t(int, vectors, num_online_cpus());

	/* Some vectors are necessary for non-queue interrupts */
	vectors += NON_Q_VECTORS;

	/* Hardware can only support a maximum of hw.mac->max_msix_vectors.
	 * With features such as RSS and VMDq, we can easily surpass the
	 * number of Rx and Tx descriptor queues supported by our device.
	 * Thus, we cap the maximum in the rare cases where the CPU count also
	 * exceeds our vector limit
	 */
	vectors = min_t(int, vectors, hw->mac.max_msix_vectors);

	/* We want a minimum of two MSI-X vectors for (1) a TxQ[0] + RxQ[0]
	 * handler, and (2) an Other (Link Status Change, etc.) handler.
	 */
	vector_threshold = MIN_MSIX_COUNT;

	/* we need to alloc (7vfs+1pf+1misc) or (8vfs+1misc) msix entries */
	if (adapter->flags2 & NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP) {
		vectors += adapter->ring_feature[RING_F_VMDQ].offset;
	}

	adapter->msix_entries = kcalloc(vectors,
					sizeof(struct msix_entry),
					GFP_KERNEL);
	if (!adapter->msix_entries)
		return -ENOMEM;

	for (i = 0; i < vectors; i++)
		adapter->msix_entries[i].entry = i;

	vectors = pci_enable_msix_range(adapter->pdev, adapter->msix_entries,
					vector_threshold, vectors);
	if (vectors < 0) {
		/* A negative count of allocated vectors indicates an error in
		 * acquiring within the specified range of MSI-X vectors */
		e_dev_warn("Failed to allocate MSI-X interrupts. Err: %d\n",
			   vectors);

		adapter->flags &= ~NGBE_FLAG_MSIX_ENABLED;
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;

		return vectors;
	}

	if (adapter->flags2 & NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP) {
		if (vectors < 9) {
			adapter->flags2 &= ~NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP;
			e_dev_warn("Remain available irqs < 9. Disable MISC IRQ REMAP.\n");
		}
		else
			vectors -= adapter->ring_feature[RING_F_VMDQ].offset;
	}

	/* we successfully allocated some number of vectors within our
	 * requested range.
	 */
	adapter->flags |= NGBE_FLAG_MSIX_ENABLED;

	/* Adjust for only the vectors we'll use, which is minimum
	 * of max_q_vectors, or the number of vectors we were allocated.
	 */
	vectors -= NON_Q_VECTORS;
	adapter->num_q_vectors = min_t(int, vectors, adapter->max_q_vectors);

	return 0;
}

static void ngbe_add_ring(struct ngbe_ring *ring,
			   struct ngbe_ring_container *head)
{
	ring->next = head->ring;
	head->ring = ring;
	head->count++;
}

/**
 * ngbe_alloc_q_vector - Allocate memory for a single interrupt vector
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
static int ngbe_alloc_q_vector(struct ngbe_adapter *adapter,
				unsigned int v_count, unsigned int v_idx,
				unsigned int txr_count, unsigned int txr_idx,
				unsigned int rxr_count, unsigned int rxr_idx)
{
	struct ngbe_q_vector *q_vector;
	struct ngbe_ring *ring;
	int node = -1;
#ifdef HAVE_IRQ_AFFINITY_HINT
	int cpu = -1;
	u8 tcs = netdev_get_num_tc(adapter->netdev);
#endif

	int ring_count, size;

	/* note this will allocate space for the ring structure as well! */
	ring_count = txr_count + rxr_count;
	size = sizeof(struct ngbe_q_vector) +
	       (sizeof(struct ngbe_ring) * ring_count);

#ifdef HAVE_IRQ_AFFINITY_HINT
	/* customize cpu for Flow Director mapping */
	if ((tcs <= 1) && !(adapter->flags & NGBE_FLAG_VMDQ_ENABLED)) {
		u16 rss_i = adapter->ring_feature[RING_F_RSS].indices;
		if (rss_i > 1 && adapter->atr_sample_rate) {
			if (cpu_online(v_idx)) {
				cpu = v_idx;
				node = cpu_to_node(cpu);
			}
		}
	}

#endif
	/* allocate q_vector and rings */
	q_vector = kzalloc_node(size, GFP_KERNEL, node);
	if (!q_vector)
		q_vector = kzalloc(size, GFP_KERNEL);
	if (!q_vector)
		return -ENOMEM;

	/* setup affinity mask and node */
#ifdef HAVE_IRQ_AFFINITY_HINT
	if (cpu != -1)
		cpumask_set_cpu(cpu, &q_vector->affinity_mask);
#endif
	q_vector->numa_node = node;

	/* initialize CPU for DCA */
	q_vector->cpu = -1;

#ifndef NGBE_NO_LRO
	/* initialize LRO */
	__skb_queue_head_init(&q_vector->lrolist.active);

#endif
	/* initialize NAPI */
	netif_napi_add(adapter->netdev, &q_vector->napi,
					ngbe_poll, 64);
#ifndef HAVE_NETIF_NAPI_ADD_CALLS_NAPI_HASH_ADD
#ifdef HAVE_NDO_BUSY_POLL
	napi_hash_add(&q_vector->napi);
#endif
#endif /*HAVE_NETIF_NAPI_ADD_CALLS_NAPI_HASH_ADD*/

#ifdef HAVE_NDO_BUSY_POLL
	/* initialize busy poll */
	atomic_set(&q_vector->state, NGBE_QV_STATE_DISABLE);

#endif
	/* tie q_vector and adapter together */
	adapter->q_vector[v_idx] = q_vector;
	q_vector->adapter = adapter;
	q_vector->v_idx = v_idx;

	/* initialize work limits */
	q_vector->tx.work_limit = adapter->tx_work_limit;
	q_vector->rx.work_limit = adapter->rx_work_limit;

	/* initialize pointer to rings */
	ring = q_vector->ring;

	/* intialize ITR */
	if (txr_count && !rxr_count) {
		/* tx only vector */
		if (adapter->tx_itr_setting == 1)
			q_vector->itr = NGBE_7K_ITR;
		else
			q_vector->itr = adapter->tx_itr_setting;
	} else {
		/* rx or rx/tx vector */
		if (adapter->rx_itr_setting == 1)
			q_vector->itr = NGBE_7K_ITR;
		else
			q_vector->itr = adapter->rx_itr_setting;
	}

	while (txr_count) {
		/* assign generic ring traits */
		ring->dev = pci_dev_to_dev(adapter->pdev);
		ring->netdev = adapter->netdev;

		/* configure backlink on ring */
		ring->q_vector = q_vector;

		/* update q_vector Tx values */
		ngbe_add_ring(ring, &q_vector->tx);

		/* apply Tx specific ring traits */
		ring->count = adapter->tx_ring_count;
		if (adapter->num_vmdqs > 1)
			ring->queue_index =
				txr_idx % adapter->queues_per_pool;
		else
			ring->queue_index = txr_idx;

		/* assign ring to adapter */
		adapter->tx_ring[txr_idx] = ring;

		/* update count and index */
		txr_count--;
		txr_idx += v_count;

		/* push pointer to next ring */
		ring++;
	}

	while (rxr_count) {
		/* assign generic ring traits */
		ring->dev = pci_dev_to_dev(adapter->pdev);
		ring->netdev = adapter->netdev;

		/* configure backlink on ring */
		ring->q_vector = q_vector;

		/* update q_vector Rx values */
		ngbe_add_ring(ring, &q_vector->rx);

		/* apply Rx specific ring traits */
		ring->count = adapter->rx_ring_count;
		if (adapter->num_vmdqs > 1)
			ring->queue_index =
				rxr_idx % adapter->queues_per_pool;
		else
			ring->queue_index = rxr_idx;

		/* assign ring to adapter */
		adapter->rx_ring[rxr_idx] = ring;

		/* update count and index */
		rxr_count--;
		rxr_idx += v_count;

		/* push pointer to next ring */
		ring++;
	}

	return 0;
}

/**
 * ngbe_free_q_vector - Free memory allocated for specific interrupt vector
 * @adapter: board private structure to initialize
 * @v_idx: Index of vector to be freed
 *
 * This function frees the memory allocated to the q_vector.  In addition if
 * NAPI is enabled it will delete any references to the NAPI struct prior
 * to freeing the q_vector.
 **/
static void ngbe_free_q_vector(struct ngbe_adapter *adapter, int v_idx)
{
	struct ngbe_q_vector *q_vector = adapter->q_vector[v_idx];
	struct ngbe_ring *ring;

	ngbe_for_each_ring(ring, q_vector->tx)
		adapter->tx_ring[ring->queue_index] = NULL;

	ngbe_for_each_ring(ring, q_vector->rx)
		adapter->rx_ring[ring->queue_index] = NULL;

	adapter->q_vector[v_idx] = NULL;
#ifdef HAVE_NDO_BUSY_POLL
	napi_hash_del(&q_vector->napi);
#endif
	netif_napi_del(&q_vector->napi);
#ifndef NGBE_NO_LRO
	__skb_queue_purge(&q_vector->lrolist.active);
#endif
	kfree_rcu(q_vector, rcu);
}

/**
 * ngbe_alloc_q_vectors - Allocate memory for interrupt vectors
 * @adapter: board private structure to initialize
 *
 * We allocate one q_vector per queue interrupt.  If allocation fails we
 * return -ENOMEM.
 **/
static int ngbe_alloc_q_vectors(struct ngbe_adapter *adapter)
{
	unsigned int q_vectors = adapter->num_q_vectors;
	unsigned int rxr_remaining = adapter->num_rx_queues;
	unsigned int txr_remaining = adapter->num_tx_queues;
	unsigned int rxr_idx = 0, txr_idx = 0, v_idx = 0;
	int err;

	if (q_vectors >= (rxr_remaining + txr_remaining)) {
		for (; rxr_remaining; v_idx++) {
			err = ngbe_alloc_q_vector(adapter, q_vectors, v_idx,
						   0, 0, 1, rxr_idx);
			if (err)
				goto err_out;

			/* update counts and index */
			rxr_remaining--;
			rxr_idx++;
		}
	}

	for (; v_idx < q_vectors; v_idx++) {
		int rqpv = DIV_ROUND_UP(rxr_remaining, q_vectors - v_idx);
		int tqpv = DIV_ROUND_UP(txr_remaining, q_vectors - v_idx);
		err = ngbe_alloc_q_vector(adapter, q_vectors, v_idx,
					   tqpv, txr_idx,
					   rqpv, rxr_idx);

		if (err)
			goto err_out;

		/* update counts and index */
		rxr_remaining -= rqpv;
		txr_remaining -= tqpv;
		rxr_idx++;
		txr_idx++;
	}

	return 0;

err_out:
	adapter->num_tx_queues = 0;
	adapter->num_rx_queues = 0;
	adapter->num_q_vectors = 0;

	while (v_idx--)
		ngbe_free_q_vector(adapter, v_idx);

	return -ENOMEM;
}

/**
 * ngbe_free_q_vectors - Free memory allocated for interrupt vectors
 * @adapter: board private structure to initialize
 *
 * This function frees the memory allocated to the q_vectors.  In addition if
 * NAPI is enabled it will delete any references to the NAPI struct prior
 * to freeing the q_vector.
 **/
static void ngbe_free_q_vectors(struct ngbe_adapter *adapter)
{
	int v_idx = adapter->num_q_vectors;

	adapter->num_tx_queues = 0;
	adapter->num_rx_queues = 0;
	adapter->num_q_vectors = 0;

	while (v_idx--)
		ngbe_free_q_vector(adapter, v_idx);
}

void ngbe_reset_interrupt_capability(struct ngbe_adapter *adapter)
{
	if (adapter->flags & NGBE_FLAG_MSIX_ENABLED) {
		adapter->flags &= ~NGBE_FLAG_MSIX_ENABLED;
		pci_disable_msix(adapter->pdev);
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;
	} else if (adapter->flags & NGBE_FLAG_MSI_ENABLED) {
		adapter->flags &= ~NGBE_FLAG_MSI_ENABLED;
		pci_disable_msi(adapter->pdev);
	}
}

/**
 * ngbe_set_interrupt_capability - set MSI-X or MSI if supported
 * @adapter: board private structure to initialize
 *
 * Attempt to configure the interrupts using the best available
 * capabilities of the hardware and the kernel.
 **/
void ngbe_set_interrupt_capability(struct ngbe_adapter *adapter)
{
	int err;

	/* We will try to get MSI-X interrupts first */
	if (!ngbe_acquire_msix_vectors(adapter))
		return;

	/* At this point, we do not have MSI-X capabilities. We need to
	 * reconfigure or disable various features which require MSI-X
	 * capability.
	 */
	/* Disable VMDq support */
	e_dev_warn("Disabling VMQd support\n");
	adapter->flags &= ~NGBE_FLAG_VMDQ_ENABLED;

#ifdef CONFIG_PCI_IOV
	/* Disable SR-IOV support */
	e_dev_warn("Disabling SR-IOV support\n");
	ngbe_disable_sriov(adapter);
	if (adapter->flags2 & NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP)
		adapter->flags2 &= ~NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP;
#endif /* CONFIG_PCI_IOV */

	/* Disable RSS */
	e_dev_warn("Disabling RSS support\n");
	adapter->ring_feature[RING_F_RSS].limit = 1;

	/* recalculate number of queues now that many features have been
	 * changed or disabled.
	 */
	ngbe_set_num_queues(adapter);
	adapter->num_q_vectors = 1;

	if (!(adapter->flags & NGBE_FLAG_MSI_CAPABLE))
		return;

	err = pci_enable_msi(adapter->pdev);
	if (err)
		e_dev_warn("Failed to allocate MSI interrupt, falling back to "
		"legacy. Error: %d\n",
			   err);
	else
		adapter->flags |= NGBE_FLAG_MSI_ENABLED;
}

/**
 * ngbe_init_interrupt_scheme - Determine proper interrupt scheme
 * @adapter: board private structure to initialize
 *
 * We determine which interrupt scheme to use based on...
 * - Kernel support (MSI, MSI-X)
 *   - which can be user-defined (via MODULE_PARAM)
 * - Hardware queue count (num_*_queues)
 *   - defined by miscellaneous hardware support/features (RSS, etc.)
 **/
int ngbe_init_interrupt_scheme(struct ngbe_adapter *adapter)
{
	int err;

	/* if assigned vfs >= 7, the PF queue irq remain seq 0 and misc irq move from 
	 * seq 1 to seq 8. it needs extra processions.
	 */
	if (adapter->num_vfs >= NGBE_MAX_VF_FUNCTIONS - 1) {
		adapter->flags2 |= NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP;
	}

	/* Number of supported queues */
	ngbe_set_num_queues(adapter);

	/* Set interrupt mode */
	ngbe_set_interrupt_capability(adapter);

	/* Allocate memory for queues */
	err = ngbe_alloc_q_vectors(adapter);
	if (err) {
		e_err(probe, "Unable to allocate memory for queue vectors\n");
		ngbe_reset_interrupt_capability(adapter);
		return err;
	}

	ngbe_cache_ring_register(adapter);

	set_bit(__NGBE_DOWN, &adapter->state);

	return 0;
}

/**
 * ngbe_clear_interrupt_scheme - Clear the current interrupt scheme settings
 * @adapter: board private structure to clear interrupt scheme on
 *
 * We go through and clear interrupt specific resources and reset the structure
 * to pre-load conditions
 **/
void ngbe_clear_interrupt_scheme(struct ngbe_adapter *adapter)
{
	ngbe_free_q_vectors(adapter);
	ngbe_reset_interrupt_capability(adapter);

	/* remove this flags */
	if (adapter->flags2 & NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP) {
		adapter->flags2 &= ~NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP;
	}
}

void ngbe_tx_ctxtdesc(struct ngbe_ring *tx_ring, u32 vlan_macip_lens,
		       u32 fcoe_sof_eof, u32 type_tucmd, u32 mss_l4len_idx)
{
	struct ngbe_tx_context_desc *context_desc;
	u16 i = tx_ring->next_to_use;

	context_desc = NGBE_TX_CTXTDESC(tx_ring, i);

	i++;
	tx_ring->next_to_use = (i < tx_ring->count) ? i : 0;

	/* set bits to identify this as an advanced context descriptor */
	type_tucmd |= NGBE_TXD_DTYP_CTXT;
	context_desc->vlan_macip_lens   = cpu_to_le32(vlan_macip_lens);
	context_desc->seqnum_seed       = cpu_to_le32(fcoe_sof_eof);
	context_desc->type_tucmd_mlhl   = cpu_to_le32(type_tucmd);
	context_desc->mss_l4len_idx     = cpu_to_le32(mss_l4len_idx);
}

