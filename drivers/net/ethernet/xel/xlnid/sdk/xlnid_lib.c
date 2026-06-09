// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2008 - 2022 Xel Technology. */

#include "xlnid.h"
#include "xlnid_sriov.h"

#ifdef HAVE_TX_MQ
/**
		xlnid_cache_ring_dcb_vmdq - Descriptor ring to register mapping for VMDq
		@adapter: board private structure to initialize

		Cache the descriptor ring offsets for VMDq to the assigned rings.  It
		will also try to cache the proper offsets if RSS/FCoE are enabled along
		with VMDq.

	**/
static bool xlnid_cache_ring_dcb_vmdq(struct xlnid_adapter *adapter)
{
	struct xlnid_ring_feature *vmdq = &adapter->ring_feature[RING_F_VMDQ];
	int i;
	u16 reg_idx;
	u8 tcs = netdev_get_num_tc(adapter->netdev);

	/* verify we have DCB enabled before proceeding */
	if (tcs <= 1) {
		return false;
	}

	/* verify we have VMDq enabled before proceeding */
	if (!(adapter->flags & XLNID_FLAG_VMDQ_ENABLED)) {
		return false;
	}

	/* start at VMDq register offset for SR-IOV enabled setups */
	reg_idx = vmdq->offset * __ALIGN_MASK(1, ~vmdq->mask);

	for (i = 0; i < adapter->num_rx_queues; i++, reg_idx++) {
		/* If we are greater than indices move to next pool */
		if ((reg_idx & ~vmdq->mask) >= tcs) {
			reg_idx = __ALIGN_MASK(reg_idx, ~vmdq->mask);
		}

		adapter->rx_ring[i]->reg_idx = reg_idx;
	}

	reg_idx = vmdq->offset * __ALIGN_MASK(1, ~vmdq->mask);

	for (i = 0; i < adapter->num_tx_queues; i++, reg_idx++) {
		/* If we are greater than indices move to next pool */
		if ((reg_idx & ~vmdq->mask) >= tcs) {
			reg_idx = __ALIGN_MASK(reg_idx, ~vmdq->mask);
		}

		adapter->tx_ring[i]->reg_idx = reg_idx;
	}

	return true;
}

/* xlnid_get_first_reg_idx - Return first register index associated with ring */
static void xlnid_get_first_reg_idx(struct xlnid_adapter *adapter, u8 tc,
									unsigned int *tx, unsigned int *rx)
{
	//  struct xlnid_hw *hw = &adapter->hw;
	struct net_device *dev = adapter->netdev;
	u8 num_tcs = netdev_get_num_tc(dev);

	*tx = 0;
	*rx = 0;

	if (num_tcs > 4) {
		/*
			TCs    : TC0/1 TC2/3 TC4-7
			TxQs/TC:    32    16     8
			RxQs/TC:    16    16    16
		*/
		*rx = tc << 4;

		if (tc < 3) {
			*tx = tc << 5;    /*   0,  32,  64 */
		}

		else if (tc < 5) {
			*tx = (tc + 2) << 4;    /*  80,  96 */
		}

		else {
			*tx = (tc + 8) << 3;    /* 104, 112, 120 */
		}
	}

	else {
		/*
			TCs    : TC0 TC1 TC2/3
			TxQs/TC:  64  32    16
			RxQs/TC:  32  32    32
		*/
		*rx = tc << 5;

		if (tc < 2) {
			*tx = tc << 6;    /*  0,  64 */
		}

		else {
			*tx = (tc + 4) << 4;    /* 96, 112 */
		}
	}
}

/**
		xlnid_cache_ring_dcb - Descriptor ring to register mapping for DCB
		@adapter: board private structure to initialize

		Cache the descriptor ring offsets for DCB to the assigned rings.

	**/
static bool xlnid_cache_ring_dcb(struct xlnid_adapter *adapter)
{
	int tc, offset, rss_i, i;
	unsigned int tx_idx, rx_idx;
	struct net_device *dev = adapter->netdev;
	u8 num_tcs = netdev_get_num_tc(dev);

	if (num_tcs <= 1) {
		return false;
	}

	rss_i = adapter->ring_feature[RING_F_RSS].indices;

	for (tc = 0, offset = 0; tc < num_tcs; tc++, offset += rss_i) {
		xlnid_get_first_reg_idx(adapter, tc, &tx_idx, &rx_idx);

		for (i = 0; i < rss_i; i++, tx_idx++, rx_idx++) {
			adapter->tx_ring[offset + i]->reg_idx = tx_idx;
			adapter->rx_ring[offset + i]->reg_idx = rx_idx;
			adapter->tx_ring[offset + i]->dcb_tc = tc;
			adapter->rx_ring[offset + i]->dcb_tc = tc;
		}
	}

	return true;
}

#endif /* HAVE_TX_MQ */
/**
		xlnid_cache_ring_vmdq - Descriptor ring to register mapping for VMDq
		@adapter: board private structure to initialize

		Cache the descriptor ring offsets for VMDq to the assigned rings.  It
		will also try to cache the proper offsets if RSS/FCoE/SRIOV are enabled along
		with VMDq.

	**/
static bool xlnid_cache_ring_vmdq(struct xlnid_adapter *adapter)
{
	struct xlnid_ring_feature *vmdq = &adapter->ring_feature[RING_F_VMDQ];
	struct xlnid_ring_feature *rss = &adapter->ring_feature[RING_F_RSS];
	int i;
	u16 reg_idx;

	/* only proceed if VMDq is enabled */
	if (!(adapter->flags & XLNID_FLAG_VMDQ_ENABLED)) {
		return false;
	}

	/* start at VMDq register offset for SR-IOV enabled setups */
	reg_idx = vmdq->offset * __ALIGN_MASK(1, ~vmdq->mask);

	for (i = 0; i < adapter->num_rx_queues; i++, reg_idx++) {
		/* If we are greater than indices move to next pool */
		if ((reg_idx & ~vmdq->mask) >= rss->indices) {
			reg_idx = __ALIGN_MASK(reg_idx, ~vmdq->mask);
		}

		adapter->rx_ring[i]->reg_idx = reg_idx;
	}

	reg_idx = vmdq->offset * __ALIGN_MASK(1, ~vmdq->mask);

	for (i = 0; i < adapter->num_tx_queues; i++, reg_idx++) {
		/* If we are greater than indices move to next pool */
		if ((reg_idx & rss->mask) >= rss->indices) {
			reg_idx = __ALIGN_MASK(reg_idx, ~vmdq->mask);
		}

		adapter->tx_ring[i]->reg_idx = reg_idx;
	}

	return true;
}

/**
		xlnid_cache_ring_rss - Descriptor ring to register mapping for RSS
		@adapter: board private structure to initialize

		Cache the descriptor ring offsets for RSS, ATR, FCoE, and SR-IOV.

	**/
static bool xlnid_cache_ring_rss(struct xlnid_adapter *adapter)
{
	int i, reg_idx;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		adapter->rx_ring[i]->reg_idx = i;
	}

	for (i = 0, reg_idx = 0; i < adapter->num_tx_queues; i++, reg_idx++) {
		adapter->tx_ring[i]->reg_idx = reg_idx;
	}

	for (i = 0; i < adapter->num_xdp_queues; i++, reg_idx++) {
		adapter->xdp_ring[i]->reg_idx = reg_idx;
	}

	return true;
}

/**
		xlnid_cache_ring_register - Descriptor ring to register mapping
		@adapter: board private structure to initialize

		Once we know the feature-set enabled for the device, we'll cache
		the register offset the descriptor ring is assigned to.

		Note, the order the various feature calls is important.  It must start with
		the "most" features enabled at the same time, then trickle down to the
		least amount of features turned on at once.
	**/
static void xlnid_cache_ring_register(struct xlnid_adapter *adapter)
{
#ifdef HAVE_TX_MQ

	if (xlnid_cache_ring_dcb_vmdq(adapter)) {
		return;
	}

	if (xlnid_cache_ring_dcb(adapter)) {
		return;
	}

#endif

	if (xlnid_cache_ring_vmdq(adapter)) {
		return;
	}

	xlnid_cache_ring_rss(adapter);
}

static int xlnid_xdp_queues(struct xlnid_adapter *adapter)
{
#ifdef HAVE_XDP_SUPPORT
	return adapter->xdp_prog ? nr_cpu_ids : 0;
#else
	return 0;
#endif
}

#define XLNID_RSS_64Q_MASK 0x3F
#define XLNID_RSS_16Q_MASK 0xF
#define XLNID_RSS_8Q_MASK 0x7
#define XLNID_RSS_4Q_MASK 0x3
#define XLNID_RSS_2Q_MASK 0x1
#define XLNID_RSS_DISABLED_MASK 0x0

#ifdef HAVE_TX_MQ
/**
		xlnid_set_dcb_vmdq_queues: Allocate queues for VMDq devices w/ DCB
		@adapter: board private structure to initialize

		When VMDq (Virtual Machine Devices queue) is enabled, allocate queues
		and VM pools where appropriate.  Also assign queues based on DCB
		priorities and map accordingly..

	**/
static bool xlnid_set_dcb_vmdq_queues(struct xlnid_adapter *adapter)
{
	int i;
	u16 vmdq_i = adapter->ring_feature[RING_F_VMDQ].limit;
	u16 vmdq_m = 0;
	u8 tcs = netdev_get_num_tc(adapter->netdev);

	/* verify we have DCB enabled before proceeding */
	if (tcs <= 1) {
		return false;
	}

	/* verify we have VMDq enabled before proceeding */
	if (!(adapter->flags & XLNID_FLAG_VMDQ_ENABLED)) {
		return false;
	}

	/* Add starting offset to total pool count */
	vmdq_i += adapter->ring_feature[RING_F_VMDQ].offset;

	/* 16 pools w/ 8 TC per pool */
	if (tcs > 4) {
		vmdq_i = min_t(u16, vmdq_i, 16);
		vmdq_m = XLNID_VMDQ_8Q_MASK;
		/* 32 pools w/ 4 TC per pool */
	}

	else {
		vmdq_i = min_t(u16, vmdq_i, 32);
		vmdq_m = XLNID_VMDQ_4Q_MASK;
	}

	/* remove the starting offset from the pool count */
	vmdq_i -= adapter->ring_feature[RING_F_VMDQ].offset;

	/* save features for later use */
	adapter->ring_feature[RING_F_VMDQ].indices = vmdq_i;
	adapter->ring_feature[RING_F_VMDQ].mask = vmdq_m;

	/*
		We do not support DCB, VMDq, and RSS all simultaneously
		so we will disable RSS since it is the lowest priority
	*/
	adapter->ring_feature[RING_F_RSS].indices = 1;
	adapter->ring_feature[RING_F_RSS].mask = XLNID_RSS_DISABLED_MASK;

	adapter->num_rx_pools = vmdq_i;
	adapter->num_rx_queues_per_pool = tcs;

	adapter->num_tx_queues = vmdq_i * tcs;
	adapter->num_xdp_queues = 0;
	adapter->num_rx_queues = vmdq_i * tcs;

	/* disable ATR as it is not supported when VMDq is enabled */
	adapter->flags &= ~XLNID_FLAG_FDIR_HASH_CAPABLE;

	/* configure TC to queue mapping */
	for (i = 0; i < tcs; i++) {
		netdev_set_tc_queue(adapter->netdev, i, 1, i);
	}

	return true;
}

/**
		xlnid_set_dcb_queues: Allocate queues for a DCB-enabled device
		@adapter: board private structure to initialize

		When DCB (Data Center Bridging) is enabled, allocate queues for
		each traffic class.  If multiqueue isn't available, then abort DCB
		initialization.

		This function handles all combinations of DCB, RSS, and FCoE.

	**/
static bool xlnid_set_dcb_queues(struct xlnid_adapter *adapter)
{
	struct net_device *dev = adapter->netdev;
	struct xlnid_ring_feature *f;
	int rss_i, rss_m, i;
	int tcs;

	/* Map queue offset and counts onto allocated tx queues */
	tcs = netdev_get_num_tc(dev);

	if (tcs <= 1) {
		return false;
	}

	/* determine the upper limit for our current DCB mode */
#ifndef HAVE_NETDEV_SELECT_QUEUE
	rss_i = adapter->indices;
#else
	rss_i = dev->num_tx_queues / tcs;
#endif

	if (tcs > 4) {
		/* 8 TC w/ 8 queues per TC */
		rss_i = min_t(u16, rss_i, 8);
		rss_m = XLNID_RSS_8Q_MASK;
	}

	else {
		/* 4 TC w/ 16 queues per TC */
		rss_i = min_t(u16, rss_i, 16);
		rss_m = XLNID_RSS_16Q_MASK;
	}

	/* set RSS mask and indices */
	f = &adapter->ring_feature[RING_F_RSS];
	rss_i = min_t(u16, rss_i, f->limit);
	f->indices = rss_i;
	f->mask = rss_m;

	/* disable ATR as it is not supported when DCB is enabled */
	adapter->flags &= ~XLNID_FLAG_FDIR_HASH_CAPABLE;

	for (i = 0; i < tcs; i++) {
		netdev_set_tc_queue(dev, i, rss_i, rss_i * i);
	}

	adapter->num_tx_queues = rss_i * tcs;
	adapter->num_xdp_queues = 0;
	adapter->num_rx_queues = rss_i * tcs;

	return true;
}

#endif
/**
		xlnid_set_vmdq_queues: Allocate queues for VMDq devices
		@adapter: board private structure to initialize

		When VMDq (Virtual Machine Devices queue) is enabled, allocate queues
		and VM pools where appropriate.  If RSS is available, then also try and
		enable RSS and map accordingly.

	**/
static bool xlnid_set_vmdq_queues(struct xlnid_adapter *adapter)
{
	u16 vmdq_i = adapter->ring_feature[RING_F_VMDQ].limit;
	u16 vmdq_m = 0;
	u16 rss_i = adapter->ring_feature[RING_F_RSS].limit;
	u16 rss_m = XLNID_RSS_DISABLED_MASK;

	/* only proceed if VMDq is enabled */
	if (!(adapter->flags & XLNID_FLAG_VMDQ_ENABLED)) {
		return false;
	}

	/* Add starting offset to total pool count */
	vmdq_i += adapter->ring_feature[RING_F_VMDQ].offset;

	/* double check we are limited to maximum pools */
	vmdq_i = min_t(u16, XLNID_MAX_VMDQ_INDICES, vmdq_i);

	/* 64 pool mode with 2 queues per pool */
	if (vmdq_i > 32) {
		vmdq_m = XLNID_VMDQ_2Q_MASK;
		rss_m = XLNID_RSS_2Q_MASK;
		rss_i = min_t(u16, rss_i, 2);
		/* 32 pool mode with up to 4 queues per pool */
	}

	else {
		vmdq_m = XLNID_VMDQ_4Q_MASK;
		rss_m = XLNID_RSS_4Q_MASK;
		/* We can support 4, 2, or 1 queues */
		rss_i = (rss_i > 3) ? 4 : (rss_i > 1) ? 2 : 1;
	}

	/* remove the starting offset from the pool count */
	vmdq_i -= adapter->ring_feature[RING_F_VMDQ].offset;

	/* save features for later use */
	adapter->ring_feature[RING_F_VMDQ].indices = vmdq_i;
	adapter->ring_feature[RING_F_VMDQ].mask = vmdq_m;

	/* limit RSS based on user input and save for later use */
	adapter->ring_feature[RING_F_RSS].indices = rss_i;
	adapter->ring_feature[RING_F_RSS].mask = rss_m;

	adapter->num_rx_pools = vmdq_i;
	adapter->num_rx_queues_per_pool = rss_i;

	adapter->num_rx_queues = vmdq_i * rss_i;
#ifdef HAVE_TX_MQ
	adapter->num_tx_queues = vmdq_i * rss_i;
#else
	adapter->num_tx_queues = vmdq_i;
#endif /* HAVE_TX_MQ */
	adapter->num_xdp_queues = 0;

	/* disable ATR as it is not supported when VMDq is enabled */
	adapter->flags &= ~XLNID_FLAG_FDIR_HASH_CAPABLE;

	return true;
}

/**
		xlnid_set_rss_queues: Allocate queues for RSS
		@adapter: board private structure to initialize

		This is our "base" multiqueue mode.  RSS (Receive Side Scaling) will try
		to allocate one Rx queue per CPU, and if available, one Tx queue per CPU.

	**/
static bool xlnid_set_rss_queues(struct xlnid_adapter *adapter)
{
	struct xlnid_ring_feature *f;
	u16 rss_i;

	/* set mask for 16 queue limit of RSS */
	f = &adapter->ring_feature[RING_F_RSS];
	rss_i = f->limit;

	f->indices = rss_i;
	f->mask = XLNID_RSS_16Q_MASK;

	/* disable ATR by default, it will be configured below */
	adapter->flags &= ~XLNID_FLAG_FDIR_HASH_CAPABLE;

	/*
		Use Flow Director in addition to RSS to ensure the best
		distribution of flows across cores, even when an FDIR flow
		isn't matched.
	*/
	if (rss_i > 1 && adapter->atr_sample_rate) {
		f = &adapter->ring_feature[RING_F_FDIR];

		rss_i = f->indices = f->limit;

		if (!(adapter->flags & XLNID_FLAG_FDIR_PERFECT_CAPABLE)) {
			adapter->flags |= XLNID_FLAG_FDIR_HASH_CAPABLE;
		}
	}

	adapter->num_rx_queues = rss_i;
#ifdef HAVE_TX_MQ
	adapter->num_tx_queues = rss_i;
#endif
	adapter->num_xdp_queues = xlnid_xdp_queues(adapter);

	return true;
}

/*
		xlnid_set_num_queues: Allocate queues for device, feature dependent
		@adapter: board private structure to initialize

		This is the top level queue allocation routine.  The order here is very
		important, starting with the "most" number of features turned on at once,
		and ending with the smallest set of features.  This way large combinations
		can be allocated if they're turned on, and smaller combinations are the
		fallthrough conditions.

	**/
static void xlnid_set_num_queues(struct xlnid_adapter *adapter)
{
	/* Start with base case */
	adapter->num_rx_queues = 1;
	adapter->num_tx_queues = 1;
	adapter->num_xdp_queues = 0;
	adapter->num_rx_pools = adapter->num_rx_queues;
	adapter->num_rx_queues_per_pool = 1;

#ifdef HAVE_TX_MQ

	if (xlnid_set_dcb_vmdq_queues(adapter)) {
		return;
	}

	if (xlnid_set_dcb_queues(adapter)) {
		return;
	}

#endif

	if (xlnid_set_vmdq_queues(adapter)) {
		return;
	}

	xlnid_set_rss_queues(adapter);
}

/**
		xlnid_acquire_msix_vectors - acquire MSI-X vectors
		@adapter: board private structure

		Attempts to acquire a suitable range of MSI-X vector interrupts. Will
		return a negative error code if unable to acquire MSI-X vectors for any
		reason.
*/
static int xlnid_acquire_msix_vectors(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;
	int i, vectors, vector_threshold;

	if (!(adapter->flags & XLNID_FLAG_MSIX_CAPABLE)) {
		return -EOPNOTSUPP;
	}

	/*  We start by asking for one vector per queue pair with XDP queues
		being stacked with TX queues.
	*/
	vectors = max(adapter->num_rx_queues, adapter->num_tx_queues);
	vectors = max(vectors, adapter->num_xdp_queues);

	/*  It is easy to be greedy for MSI-X vectors. However, it really
		doesn't do much good if we have a lot more vectors than CPUs. We'll
		be somewhat conservative and only ask for (roughly) the same number
		of vectors as there are CPUs.
	*/
	vectors = min_t(int, vectors, num_online_cpus());

	/* Some vectors are necessary for non-queue interrupts */
	vectors += NON_Q_VECTORS;

	/*  Hardware can only support a maximum of hw.mac->max_msix_vectors.
		With features such as RSS and VMDq, we can easily surpass the
		number of Rx and Tx descriptor queues supported by our device.
		Thus, we cap the maximum in the rare cases where the CPU count also
		exceeds our vector limit
	*/
	vectors = min_t(int, vectors, hw->mac.max_msix_vectors);

	/*  We want a minimum of two MSI-X vectors for (1) a TxQ[0] + RxQ[0]
		handler, and (2) an Other (Link Status Change, etc.) handler.
	*/
	vector_threshold = MIN_MSIX_COUNT;

	adapter->msix_entries =
		kcalloc(vectors, sizeof(struct msix_entry), GFP_KERNEL);

	if (!adapter->msix_entries) {
		return -ENOMEM;
	}

	for (i = 0; i < vectors; i++) {
		adapter->msix_entries[i].entry = i;
	}

	vectors = pci_enable_msix_range(adapter->pdev, adapter->msix_entries,
									vector_threshold, vectors);

	if (vectors < 0) {
		/*  A negative count of allocated vectors indicates an error in
			acquiring within the specified range of MSI-X vectors
		*/
		e_dev_warn("Failed to allocate MSI-X interrupts. Err: %d\n",
					vectors);

		adapter->flags &= ~XLNID_FLAG_MSIX_ENABLED;
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;

		return vectors;
	}

	/*  we successfully allocated some number of vectors within our
		requested range.
	*/
	adapter->flags |= XLNID_FLAG_MSIX_ENABLED;

	/*  Adjust for only the vectors we'll use, which is minimum
		of max_q_vectors, or the number of vectors we were allocated.
	*/
	vectors -= NON_Q_VECTORS;
	adapter->num_q_vectors = min_t(int, vectors, adapter->max_q_vectors);

	return 0;
}

static void xlnid_add_ring(struct xlnid_ring *ring,
							struct xlnid_ring_container *head)
{
	ring->next = head->ring;
	head->ring = ring;
	head->count++;
}

/**
		xlnid_alloc_q_vector - Allocate memory for a single interrupt vector
		@adapter: board private structure to initialize
		@v_count: q_vectors allocated on adapter, used for ring interleaving
		@v_idx: index of vector in adapter struct
		@txr_count: total number of Tx rings to allocate
		@txr_idx: index of first Tx ring to allocate
		@xdp_count: total number of XDP rings to allocate
		@xdp_idx: index of first XDP ring to allocate
		@rxr_count: total number of Rx rings to allocate
		@rxr_idx: index of first Rx ring to allocate

		We allocate one q_vector.  If allocation fails we return -ENOMEM.
	**/
static int xlnid_alloc_q_vector(struct xlnid_adapter *adapter,
								unsigned int v_count, unsigned int v_idx,
								unsigned int txr_count, unsigned int txr_idx,
								unsigned int xdp_count, unsigned int xdp_idx,
								unsigned int rxr_count, unsigned int rxr_idx)
{
	struct xlnid_q_vector *q_vector;
	struct xlnid_ring *ring;
	int node = -1;
#ifdef HAVE_IRQ_AFFINITY_HINT
	int cpu = -1;
	u8 tcs = netdev_get_num_tc(adapter->netdev);
#endif
	int ring_count;

	/* note this will allocate space for the ring structure as well! */
	ring_count = txr_count + rxr_count + xdp_count;

#ifdef HAVE_IRQ_AFFINITY_HINT

	/* customize cpu for Flow Director mapping */
	if ((tcs <= 1) && !(adapter->flags & XLNID_FLAG_VMDQ_ENABLED)) {
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
	q_vector = kzalloc_node(struct_size(q_vector, ring, ring_count),
							GFP_KERNEL, node);

	if (!q_vector)
		q_vector = kzalloc(struct_size(q_vector, ring, ring_count),
							GFP_KERNEL);

	if (!q_vector) {
		return -ENOMEM;
	}

	/* setup affinity mask and node */
#ifdef HAVE_IRQ_AFFINITY_HINT

	if (cpu != -1) {
		cpumask_set_cpu(cpu, &q_vector->affinity_mask);
	}

#endif
	q_vector->node = node;

	/* initialize CPU for DCA */
	q_vector->cpu = -1;

	/* initialize NAPI */
	netif_napi_add(adapter->netdev, &q_vector->napi, xlnid_poll);
#ifndef HAVE_NETIF_NAPI_ADD_CALLS_NAPI_HASH_ADD
#ifdef HAVE_NDO_BUSY_POLL
	napi_hash_add(&q_vector->napi);
#endif
#endif

#ifdef HAVE_NDO_BUSY_POLL
	/* initialize busy poll */
	atomic_set(&q_vector->state, XLNID_QV_STATE_DISABLE);

#endif
	/* tie q_vector and adapter together */
	adapter->q_vector[v_idx] = q_vector;
	q_vector->adapter = adapter;
	q_vector->v_idx = v_idx;

	/* initialize work limits */
	q_vector->tx.work_limit = adapter->tx_work_limit;

	/* Initialize setting for adaptive ITR */
	q_vector->tx.itr = XLNID_ITR_ADAPTIVE_MAX_USECS |
						XLNID_ITR_ADAPTIVE_LATENCY;
	q_vector->rx.itr = XLNID_ITR_ADAPTIVE_MAX_USECS |
						XLNID_ITR_ADAPTIVE_LATENCY;

	/* intialize ITR */
	if (txr_count && !rxr_count) {
		/* tx only vector */
		if (adapter->tx_itr_setting == 1) {
			q_vector->itr = XLNID_12K_ITR;
		}

		else {
			q_vector->itr = adapter->tx_itr_setting;
		}
	}

	else {
		/* rx or rx/tx vector */
		if (adapter->rx_itr_setting == 1) {
			q_vector->itr = XLNID_20K_ITR;
		}

		else {
			q_vector->itr = adapter->rx_itr_setting;
		}
	}

	/* initialize pointer to rings */
	ring = q_vector->ring;

	while (txr_count) {
		/* assign generic ring traits */
		ring->dev = pci_dev_to_dev(adapter->pdev);
		ring->netdev = adapter->netdev;

		/* configure backlink on ring */
		ring->q_vector = q_vector;

		/* update q_vector Tx values */
		xlnid_add_ring(ring, &q_vector->tx);

		/* apply Tx specific ring traits */
		ring->count = adapter->tx_ring_count;
		ring->queue_index = txr_idx;

		/* assign ring to adapter */
		adapter->tx_ring[txr_idx] = ring;

		/* update count and index */
		txr_count--;
		txr_idx += v_count;

		/* push pointer to next ring */
		ring++;
	}

	while (xdp_count) {
		/* assign generic ring traits */
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;

		/* configure backlink on ring */
		ring->q_vector = q_vector;

		/* update q_vector Tx values */
		xlnid_add_ring(ring, &q_vector->tx);

		/* apply Tx specific ring traits */
		ring->count = adapter->tx_ring_count;
		ring->queue_index = xdp_idx;
		set_ring_xdp(ring);

		spin_lock_init(&ring->tx_lock);

		/* assign ring to adapter */
		adapter->xdp_ring[xdp_idx] = ring;

		/* update count and index */
		xdp_count--;
		xdp_idx++;

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
		xlnid_add_ring(ring, &q_vector->rx);

		/*
			UDP frames with a 0 checksum
			can be marked as checksum errors.
		*/
		set_bit(__XLNID_RX_CSUM_UDP_ZERO_ERR, &ring->state);

		/* apply Rx specific ring traits */
		ring->count = adapter->rx_ring_count;
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
		xlnid_free_q_vector - Free memory allocated for specific interrupt vector
		@adapter: board private structure to initialize
		@v_idx: Index of vector to be freed

		This function frees the memory allocated to the q_vector.  In addition if
		NAPI is enabled it will delete any references to the NAPI struct prior
		to freeing the q_vector.
	**/
static void xlnid_free_q_vector(struct xlnid_adapter *adapter, int v_idx)
{
	struct xlnid_q_vector *q_vector = adapter->q_vector[v_idx];
	struct xlnid_ring *ring;

	xlnid_for_each_ring(ring, q_vector->tx) {
		if (ring_is_xdp(ring)) {
			adapter->xdp_ring[ring->queue_index] = NULL;
		}

		else {
			adapter->tx_ring[ring->queue_index] = NULL;
		}
	}

	if (static_key_enabled((struct static_key *)&xlnid_xdp_locking_key)) {
		static_branch_dec(&xlnid_xdp_locking_key);
	}

	xlnid_for_each_ring(ring, q_vector->rx)
	adapter->rx_ring[ring->queue_index] = NULL;

	adapter->q_vector[v_idx] = NULL;
#ifdef HAVE_NDO_BUSY_POLL
	napi_hash_del(&q_vector->napi);
#endif
	netif_napi_del(&q_vector->napi);
	kfree_rcu(q_vector, rcu);
}

/**
		xlnid_alloc_q_vectors - Allocate memory for interrupt vectors
		@adapter: board private structure to initialize

		We allocate one q_vector per queue interrupt.  If allocation fails we
		return -ENOMEM.
	**/
static int xlnid_alloc_q_vectors(struct xlnid_adapter *adapter)
{
	unsigned int q_vectors = adapter->num_q_vectors;
	unsigned int rxr_remaining = adapter->num_rx_queues;
	unsigned int txr_remaining = adapter->num_tx_queues;
	unsigned int xdp_remaining = adapter->num_xdp_queues;
	unsigned int rxr_idx = 0, txr_idx = 0, xdp_idx = 0, v_idx = 0;
	int err;
#ifdef HAVE_AF_XDP_ZC_SUPPORT
	int i;
#endif

	if (q_vectors >= (rxr_remaining + txr_remaining + xdp_remaining)) {
		for (; rxr_remaining; v_idx++) {
			err = xlnid_alloc_q_vector(adapter, q_vectors, v_idx, 0,
										0, 0, 0, 1, rxr_idx);

			if (err) {
				goto err_out;
			}

			/* update counts and index */
			rxr_remaining--;
			rxr_idx++;
		}
	}

	for (; v_idx < q_vectors; v_idx++) {
		int rqpv = DIV_ROUND_UP(rxr_remaining, q_vectors - v_idx);
		int tqpv = DIV_ROUND_UP(txr_remaining, q_vectors - v_idx);
		int xqpv = DIV_ROUND_UP(xdp_remaining, q_vectors - v_idx);

		err = xlnid_alloc_q_vector(adapter, q_vectors, v_idx, tqpv,
									txr_idx, xqpv, xdp_idx, rqpv,
									rxr_idx);

		if (err) {
			goto err_out;
		}

		/* update counts and index */
		rxr_remaining -= rqpv;
		txr_remaining -= tqpv;
		xdp_remaining -= xqpv;
		rxr_idx++;
		txr_idx++;
		xdp_idx += xqpv;
	}

#ifdef HAVE_AF_XDP_ZC_SUPPORT

	for (i = 0; i < adapter->num_rx_queues; i++) {
		if (adapter->rx_ring[i]) {
			adapter->rx_ring[i]->ring_idx = i;
		}
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		if (adapter->tx_ring[i]) {
			adapter->tx_ring[i]->ring_idx = i;
		}
	}

	for (i = 0; i < adapter->num_xdp_queues; i++) {
		if (adapter->xdp_ring[i]) {
			adapter->xdp_ring[i]->ring_idx = i;
		}
	}

#endif /* HAVE_AF_XDP_ZC_SUPPORT */
	return XLNID_SUCCESS;

err_out:
	adapter->num_tx_queues = 0;
	adapter->num_xdp_queues = 0;
	adapter->num_rx_queues = 0;
	adapter->num_q_vectors = 0;

	while (v_idx--) {
		xlnid_free_q_vector(adapter, v_idx);
	}

	return -ENOMEM;
}

/**
		xlnid_free_q_vectors - Free memory allocated for interrupt vectors
		@adapter: board private structure to initialize

		This function frees the memory allocated to the q_vectors.  In addition if
		NAPI is enabled it will delete any references to the NAPI struct prior
		to freeing the q_vector.
	**/
static void xlnid_free_q_vectors(struct xlnid_adapter *adapter)
{
	int v_idx = adapter->num_q_vectors;

	adapter->num_tx_queues = 0;
	adapter->num_xdp_queues = 0;
	adapter->num_rx_queues = 0;
	adapter->num_q_vectors = 0;

	while (v_idx--) {
		xlnid_free_q_vector(adapter, v_idx);
	}
}

void xlnid_reset_interrupt_capability(struct xlnid_adapter *adapter)
{
	if (adapter->flags & XLNID_FLAG_MSIX_ENABLED) {
		adapter->flags &= ~XLNID_FLAG_MSIX_ENABLED;
		pci_disable_msix(adapter->pdev);
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;
	}

	else if (adapter->flags & XLNID_FLAG_MSI_ENABLED) {
		adapter->flags &= ~XLNID_FLAG_MSI_ENABLED;
		pci_disable_msi(adapter->pdev);
	}
}

/**
		xlnid_set_interrupt_capability - set MSI-X or MSI if supported
		@adapter: board private structure to initialize

		Attempt to configure the interrupts using the best available
		capabilities of the hardware and the kernel.
	**/
void xlnid_set_interrupt_capability(struct xlnid_adapter *adapter)
{
	int err;

	/* We will try to get MSI-X interrupts first */
	if (!xlnid_acquire_msix_vectors(adapter)) {
		return;
	}

	/*  At this point, we do not have MSI-X capabilities. We need to
		reconfigure or disable various features which require MSI-X
		capability.
	*/

	/* Disable DCB unless we only have a single traffic class */
	if (netdev_get_num_tc(adapter->netdev) > 1) {
		e_dev_warn(
			"Number of DCB TCs exceeds number of available queues. Disabling DCB support.\n");
		netdev_reset_tc(adapter->netdev);

		adapter->flags &= ~XLNID_FLAG_DCB_ENABLED;
		//adapter->temp_dcb_cfg.pfc_mode_enable = false;
		//adapter->dcb_cfg.pfc_mode_enable = false;
	}

	//adapter->dcb_cfg.num_tcs.pg_tcs = 1;
	//adapter->dcb_cfg.num_tcs.pfc_tcs = 1;

	/* Disable VMDq support */
	e_dev_warn("Disabling VMQd support\n");
	adapter->flags &= ~XLNID_FLAG_VMDQ_ENABLED;

	/* Disable RSS */
	e_dev_warn("Disabling RSS support\n");
	adapter->ring_feature[RING_F_RSS].limit = 1;

	/*  recalculate number of queues now that many features have been
		changed or disabled.
	*/
	xlnid_set_num_queues(adapter);
	adapter->num_q_vectors = 1;

	if (!(adapter->flags & XLNID_FLAG_MSI_CAPABLE)) {
		return;
	}

	err = pci_enable_msi(adapter->pdev);

	if (err)
		e_dev_warn(
			"Failed to allocate MSI interrupt, falling back to legacy. Error: %d\n",
			err);
	else {
		adapter->flags |= XLNID_FLAG_MSI_ENABLED;
	}
}

/**
		xlnid_init_interrupt_scheme - Determine proper interrupt scheme
		@adapter: board private structure to initialize

		We determine which interrupt scheme to use based on...
		- Kernel support (MSI, MSI-X)
		- which can be user-defined (via MODULE_PARAM)
		- Hardware queue count (num_*_queues)
		- defined by miscellaneous hardware support/features (RSS, etc.)
	**/
int xlnid_init_interrupt_scheme(struct xlnid_adapter *adapter)
{
	int err;

	/* Number of supported queues */
	xlnid_set_num_queues(adapter);

	/* Set interrupt mode */
	xlnid_set_interrupt_capability(adapter);

	/* Allocate memory for queues */
	err = xlnid_alloc_q_vectors(adapter);

	if (err) {
		e_err(probe, "Unable to allocate memory for queue vectors\n");
		xlnid_reset_interrupt_capability(adapter);
		return err;
	}

	xlnid_cache_ring_register(adapter);

#ifdef HAVE_XDP_SUPPORT
	e_dev_info(
		"Multiqueue %s: Rx Queue count = %u, Tx Queue count = %u XDP Queue count = %u\n",
		(adapter->num_rx_queues > 1) ? "Enabled" : "Disabled",
		adapter->num_rx_queues, adapter->num_tx_queues,
		adapter->num_xdp_queues);
#else
	e_dev_info("Multiqueue %s: Rx Queue count = %u, Tx Queue count = %u\n",
				(adapter->num_rx_queues > 1) ? "Enabled" : "Disabled",
				adapter->num_rx_queues, adapter->num_tx_queues);
#endif
	set_bit(__XLNID_DOWN, &adapter->state);

	return XLNID_SUCCESS;
}

/**
		xlnid_clear_interrupt_scheme - Clear the current interrupt scheme settings
		@adapter: board private structure to clear interrupt scheme on

		We go through and clear interrupt specific resources and reset the structure
		to pre-load conditions
	**/
void xlnid_clear_interrupt_scheme(struct xlnid_adapter *adapter)
{
	adapter->num_tx_queues = 0;
	adapter->num_xdp_queues = 0;
	adapter->num_rx_queues = 0;

	xlnid_free_q_vectors(adapter);
	xlnid_reset_interrupt_capability(adapter);
}

void xlnid_tx_ctxtdesc(struct xlnid_ring *tx_ring, u32 vlan_macip_lens,
						u32 fcoe_sof_eof, u32 type_tucmd, u32 mss_l4len_idx)
{
	struct xlnid_adv_tx_context_desc *context_desc;
	u16 i = tx_ring->next_to_use;

	context_desc = XLNID_TX_CTXTDESC(tx_ring, i);

	i++;
	tx_ring->next_to_use = (i < tx_ring->count) ? i : 0;

	/* set bits to identify this as an advanced context descriptor */
	type_tucmd |= XLNID_TXD_CMD_DEXT | XLNID_ADVTXD_DTYP_CTXT;

	context_desc->vlan_macip_lens = cpu_to_le32(vlan_macip_lens);
	context_desc->seqnum_seed = cpu_to_le32(fcoe_sof_eof);
	context_desc->type_tucmd_mlhl = cpu_to_le32(type_tucmd);
	context_desc->mss_l4len_idx = cpu_to_le32(mss_l4len_idx);
}
