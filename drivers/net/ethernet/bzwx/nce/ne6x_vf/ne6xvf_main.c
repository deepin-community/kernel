// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include <uapi/linux/if.h>
#include <net/geneve.h>
#include <net/vxlan.h>

#include "ne6xvf.h"
#include "ne6xvf_osdep.h"
#include "ne6xvf_virtchnl.h"
#include "ne6xvf_txrx.h"
#include "version.h"

#define SUMMARY \
	"Chengdu BeiZhongWangXin Ethernet Connection N5/N6 Series Virtual Function Linux Driver"
#define COPYRIGHT "Copyright (c)  2020 - 2023 Chengdu BeiZhongWangXin Technology Co., Ltd."

char ne6xvf_driver_name[] = "ncevf";
static const char ne6xvf_driver_string[] = SUMMARY;

const char ne6xvf_driver_version[] = VERSION;
static const char ne6xvf_copyright[] = COPYRIGHT;

static const struct pci_device_id ne6xvf_pci_tbl[] = {
	{PCI_VDEVICE(BZWX, 0x501a), 0},
	{PCI_VDEVICE(BZWX, 0x601a), 0},
	/* required last entry */
	{0,}
};

MODULE_DEVICE_TABLE(pci, ne6xvf_pci_tbl);

MODULE_AUTHOR("Chengdu BeiZhongWangXin Technology Co., Ltd., <support@bzwx-kj.com>");
MODULE_DESCRIPTION(SUMMARY);
MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);

static const struct net_device_ops ne6xvf_netdev_ops;
struct workqueue_struct *ne6xvf_wq;
static void ne6xvf_sync_features(struct net_device *netdev);

struct ne6xvf_adapter *ne6xvf_pdev_to_adapter(struct pci_dev *pdev)
{
	return netdev_priv(pci_get_drvdata(pdev));
}

void ne6xvf_schedule_reset(struct ne6xvf_adapter *adapter)
{
	adapter->flags |= NE6XVF_FLAG_RESET_NEEDED;
	mod_delayed_work(ne6xvf_wq, &adapter->watchdog_task, 0);
}

static void ne6xvf_tx_timeout(struct net_device *netdev, __always_unused unsigned int txqueue)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	adapter->tx_timeout_count++;
	ne6xvf_schedule_reset(adapter);
}

/**
 * nce_get_vsi_stats_struct - Get System Network Statistics
 * @vsi: the VSI we care about
 *
 * Returns the address of the device statistics structure.
 * The statistics are actually updated from the service task.
 **/

struct net_device_stats *nce_get_vsi_stats_struct(struct ne6xvf_adapter *adapter)
{
	if (adapter->netdev)
		return &adapter->netdev->stats;
	else
		return &adapter->net_stats;
}

/**
 * nce_update_pf_stats - Update PF port stats counters
 * @pf: PF whose stats needs to be updated
 */
void ne6xvf_update_pf_stats(struct ne6xvf_adapter *adapter)
{
	struct net_device_stats *ns; /* netdev stats */
	struct ne6x_ring *tx_ring;
	struct ne6x_ring *rx_ring;
	u64 bytes, packets;
	u64 rx_p, rx_b;
	u64 tx_p, tx_b;
	u16 i;

	if (test_bit(NE6X_ADPT_DOWN, adapter->comm.state))
		return;

	ns = nce_get_vsi_stats_struct(adapter);

	rx_p = 0;
	rx_b = 0;
	tx_p = 0;
	tx_b = 0;

	rcu_read_lock();
	for (i = 0; i < adapter->num_active_queues; i++) {
		/* locate Tx ring */
		tx_ring = &adapter->tx_rings[i];

		packets = tx_ring->stats.packets;
		bytes = tx_ring->stats.bytes;

		tx_b += bytes;
		tx_p += packets;

		rx_ring = &adapter->rx_rings[i];

		packets = rx_ring->stats.packets;
		bytes = rx_ring->stats.bytes;
		rx_b += bytes;
		rx_p += packets;
	}
	rcu_read_unlock();

	ns->rx_packets = rx_p;
	ns->rx_bytes = rx_b;
	ns->tx_packets = tx_p;
	ns->tx_bytes = tx_b;

	adapter->net_stats.rx_packets = rx_p;
	adapter->net_stats.tx_packets = rx_b;
	adapter->net_stats.rx_bytes = rx_b;
	adapter->net_stats.tx_bytes = tx_b;
}

bool ne6xvf_is_remove_in_progress(struct ne6xvf_adapter *adapter)
{
	return test_bit(__NE6XVF_IN_REMOVE_TASK, &adapter->crit_section);
}

static void ne6xvf_sdk_task(struct work_struct *work)
{
	struct ne6xvf_adapter *adapter = container_of(work, struct ne6xvf_adapter, sdk_task);
	struct ne6xvf_hw *hw = &adapter->hw;
	struct ne6xvf_arq_event_info event;
	enum ne6xvf_status ret, v_ret;
	enum virtchnl_ops v_op;
	u16 pending = 1u;

	if (ne6xvf_is_remove_in_progress(adapter))
		return;

	if (adapter->flags & NE6XVF_FLAG_PF_COMMS_FAILED)
		goto out;

	event.buf_len = NE6XVF_MAX_AQ_BUF_SIZE;
	event.msg_buf = kzalloc(event.buf_len, GFP_KERNEL);
	if (!event.msg_buf)
		goto out;

	do {
		ret = ne6xvf_clean_arq_element(hw, &event, &pending);
		v_op = (enum virtchnl_ops)le32_to_cpu(event.snap.type);
		v_ret = (enum ne6xvf_status)le32_to_cpu(event.snap.state);

		if (ret || !v_op)
			break; /* No event to process or error cleaning ARQ */

		while (test_and_set_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section))
			usleep_range(500, 1000);

		ne6xvf_virtchnl_completion(adapter, v_op, v_ret, event.msg_buf, event.msg_len);
		clear_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section);
		if (pending != 0)
			memset(event.msg_buf, 0, NE6XVF_MAX_AQ_BUF_SIZE);
	} while (pending);

	if ((adapter->flags & (NE6XVF_FLAG_RESET_PENDING | NE6XVF_FLAG_RESET_NEEDED)) ||
	    adapter->state == __NE6XVF_RESETTING)
		goto freedom;

freedom:
	kfree(event.msg_buf);

out:
	return;
}

static int ne6xvf_check_reset_complete(struct ne6xvf_hw *hw)
{
	u64 rstat;
	int i;

	for (i = 0; i < NE6XVF_RESET_WAIT_COMPLETE_COUNT; i++) {
		rstat = rd64(hw, NE6XVF_REG_ADDR(0, NE6X_VP_RELOAD));
		if (rstat)
			return 0;

		usleep_range(10, 20);
	}

	return 0;
}

int ne6xvf_init_sdk_mbx(struct ne6xvf_hw *hw)
{
	union u_ne6x_mbx_snap_buffer_data mbx_buffer;
	union u_ne6x_mbx_snap_buffer_data usnap;
	u64 val;

	if (hw->mbx.init_flag)
		return -1;

	hw->mbx.sq_data.state = NE6X_MAL_VF_DETECT_STATE_NEW_SNAPSHOT;
	hw->mbx.sq_data.type = VIRTCHNL_OP_UNKNOWN;
	hw->mbx.init_flag = 0x1;

	val = NE6XVF_READ_REG(hw, NE6XVF_REG_ADDR(0, NE6X_VP_INT));
	if (val & 0x2) {
		usnap.val = NE6XVF_READ_REG(hw, NE6XVF_REG_ADDR(0, NE6XVF_PF_MAILBOX_DATA));
		mbx_buffer.snap.state = usnap.snap.state;
		mbx_buffer.snap.type = usnap.snap.type;

		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(0, NE6XVF_MAILBOX_DATA), mbx_buffer.val);
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(0, NE6X_VP_INT), 0x2);
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(0, NE6XVF_DB_STATE), 0x1);
	}

	usleep_range(10, 20);
	val = NE6XVF_READ_REG(hw, NE6XVF_REG_ADDR(0, NE6X_VP_INT));

	if (val & 0x1)
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(0, NE6X_VP_INT), 0x1);

	return 0;
}

static void ne6xvf_startup(struct ne6xvf_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pdev;
	struct ne6xvf_hw *hw = &adapter->hw;
	int ret;

	WARN_ON(adapter->state != __NE6XVF_STARTUP);

	adapter->flags &= ~NE6XVF_FLAG_PF_COMMS_FAILED;
	adapter->flags &= ~NE6XVF_FLAG_RESET_PENDING;

	ret = ne6xvf_check_reset_complete(hw);
	if (ret) {
		dev_info(&pdev->dev, "Device is still in reset (%d), retrying\n", ret);
		goto err;
	}

	ret = ne6xvf_init_sdk_mbx(hw);
	if (ret) {
		dev_err(&pdev->dev, "Failed to init SDK (%d)\n", ret);
		goto err;
	}

	ne6xvf_change_state(adapter, __NE6XVF_INIT_GET_RESOURCES);

	return;

err:
	ne6xvf_change_state(adapter, __NE6XVF_INIT_FAILED);
}

/**
 * ne6xvf_parse_vf_resource_msg - parse response from VIRTCHNL_OP_GET_VF_RESOURCES
 * @adapter: board private structure
 */
int ne6xvf_parse_vf_resource_msg(struct ne6xvf_adapter *adapter)
{
	int i, num_req_queues = adapter->num_req_queues;

	for (i = 0; i < adapter->vf_res->num_vsis; i++) {
		if (adapter->vf_res->vsi_res[i].vsi_type == NE6XVF_VIRTCHNL_VSI_SRIOV)
			adapter->vsi_res = &adapter->vf_res->vsi_res[i];
	}

	if (!adapter->vsi_res) {
		dev_err(&adapter->pdev->dev, "No LAN VSI found\n");
		return -ENODEV;
	}

	if (num_req_queues && num_req_queues > adapter->vsi_res->num_queue_pairs) {
		/* Problem.  The PF gave us fewer queues than what we had
		 * negotiated in our request.  Need a reset to see if we can't
		 * get back to a working state.
		 */
		dev_err(&adapter->pdev->dev, "Requested %d queues, but PF only gave us %d.\n",
			num_req_queues, adapter->vsi_res->num_queue_pairs);
		adapter->flags |= NE6XVF_FLAG_REINIT_MSIX_NEEDED;
		adapter->num_req_queues = adapter->vsi_res->num_queue_pairs;
		ne6xvf_schedule_reset(adapter);

		return -EAGAIN;
	}
	adapter->num_req_queues = 0;
	set_bit(NE6X_ADPT_DOWN, adapter->comm.state);
	return 0;
}

/**
 * ne6xvf_init_get_resources - third step of driver startup
 * @adapter: board private structure
 *
 * Function process __NE6XVF_INIT_GET_RESOURCES driver state and
 * finishes driver initialization procedure.
 * When success the state is changed to __NE6XVF_DOWN
 * when fails the state is changed to __NE6XVF_INIT_FAILED
 **/
static void ne6xvf_init_get_resources(struct ne6xvf_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pdev;
	int ret;

	WARN_ON(adapter->state != __NE6XVF_INIT_GET_RESOURCES);

	if (!adapter->vf_res) {
		adapter->vf_res = kzalloc(struct_size(adapter->vf_res, vsi_res, 1), GFP_KERNEL);
		if (!adapter->vf_res)
			goto err;
	}

	adapter->hw_feature = 0x00;
	ret = ne6xvf_send_vf_config_msg(adapter, true);
	if (ret) {
		dev_err(&pdev->dev, "Unable to send config request (%d)\n", ret);
		goto err;
	}

	ret = ne6xvf_get_vf_config(adapter);
	if (ret == NE6XVF_ERR_ADMIN_QUEUE_NO_WORK) {
		ret = ne6xvf_send_vf_config_msg(adapter, true);
		goto err_alloc;
	} else if (ret == NE6XVF_ERR_PARAM) {
		/* We only get ERR_PARAM if the device is in a very bad
		 * state or if we've been disabled for previous bad
		 * behavior. Either way, we're done now.
		 */
		dev_err(&pdev->dev,
			"Unable to get VF config due to PF error condition, not retrying\n");
		return;
	}

	if (ret) {
		dev_err(&pdev->dev, "Unable to get VF config (%d)\n", ret);
		goto err_alloc;
	}

	ret = ne6xvf_parse_vf_resource_msg(adapter);
	if (ret) {
		dev_err(&pdev->dev, "Failed to parse VF resource message from PF (%d)\n", ret);
		goto err_alloc;
	}

	ne6xvf_change_state(adapter, __NE6XVF_INIT_EXTENDED_CAPS);
	return;

err_alloc:
	kfree(adapter->vf_res);
	adapter->vf_res = NULL;
err:
	ne6xvf_change_state(adapter, __NE6XVF_INIT_FAILED);
}

/**
 * ne6xvf_napi_disable_all - disable NAPI on all queue vectors
 * @adapter: board private structure
 **/
static void ne6xvf_napi_disable_all(struct ne6xvf_adapter *adapter)
{
	int q_vectors = adapter->num_msix_vectors;
	struct ne6x_q_vector *q_vector;
	int q_idx;

	for (q_idx = 0; q_idx < q_vectors; q_idx++) {
		q_vector = &adapter->q_vectors[q_idx];
		napi_disable(&q_vector->napi);
	}
}

static void ne6xvf_free_queues(struct ne6xvf_adapter *adapter)
{
	if (!adapter->vsi_res)
		return;

	adapter->num_active_queues = 0;
	kfree(adapter->tg_rings);
	adapter->tg_rings = NULL;
	kfree(adapter->cq_rings);
	adapter->cq_rings = NULL;
	kfree(adapter->tx_rings);
	adapter->tx_rings = NULL;
	kfree(adapter->rx_rings);
	adapter->rx_rings = NULL;
}

/**
 * ne6xvf_alloc_queues - Allocate memory for all rings
 * @adapter: board private structure to initialize
 *
 * We allocate one ring per queue at run-time since we don't know the
 * number of queues at compile-time.  The polling_netdev array is
 * intended for Multiqueue, but should work fine with a single queue.
 **/
static int ne6xvf_alloc_queues(struct ne6xvf_adapter *adapter)
{
	int i, num_active_queues;

	/* If we're in reset reallocating queues we don't actually know yet for
	 * certain the PF gave us the number of queues we asked for but we'll
	 * assume it did.  Once basic reset is finished we'll confirm once we
	 * start negotiating config with PF.
	 */
	if (adapter->num_req_queues)
		num_active_queues = adapter->num_req_queues;
	else
		num_active_queues = min_t(int, adapter->vsi_res->num_queue_pairs,
					  (int)(num_online_cpus()));

	adapter->tg_rings = kcalloc(num_active_queues, sizeof(struct ne6x_ring), GFP_KERNEL);
	adapter->cq_rings = kcalloc(num_active_queues, sizeof(struct ne6x_ring), GFP_KERNEL);

	adapter->tx_rings = kcalloc(num_active_queues, sizeof(struct ne6x_ring), GFP_KERNEL);
	if (!adapter->tx_rings)
		goto err_out;

	adapter->rx_rings = kcalloc(num_active_queues, sizeof(struct ne6x_ring), GFP_KERNEL);
	if (!adapter->rx_rings)
		goto err_out;

	for (i = 0; i < num_active_queues; i++) {
		struct ne6x_ring *tg_ring;
		struct ne6x_ring *cq_ring;
		struct ne6x_ring *tx_ring;
		struct ne6x_ring *rx_ring;

		tg_ring = &adapter->tg_rings[i];
		tg_ring->queue_index = i;
		tg_ring->netdev = adapter->netdev;
		tg_ring->dev = pci_dev_to_dev(adapter->pdev);
		tg_ring->adpt  = adapter;
		tg_ring->count = adapter->tx_desc_count;

		cq_ring = &adapter->cq_rings[i];
		cq_ring->queue_index = i;
		cq_ring->netdev = adapter->netdev;
		cq_ring->dev = pci_dev_to_dev(adapter->pdev);
		cq_ring->adpt  = adapter;
		cq_ring->count = adapter->cq_desc_count;

		tx_ring = &adapter->tx_rings[i];
		tx_ring->queue_index = i;
		tx_ring->netdev = adapter->netdev;
		tx_ring->dev = pci_dev_to_dev(adapter->pdev);
		tx_ring->adpt  = adapter;
		tx_ring->count = adapter->tx_desc_count;

		rx_ring = &adapter->rx_rings[i];
		rx_ring->queue_index = i;
		rx_ring->netdev = adapter->netdev;
		rx_ring->dev = pci_dev_to_dev(adapter->pdev);
		rx_ring->adpt  = adapter;
		rx_ring->count = adapter->rx_desc_count;
	}

	adapter->max_queues = num_active_queues;
	adapter->num_active_queues = adapter->max_queues;

	return 0;

err_out:
	ne6xvf_free_queues(adapter);
	return -ENOMEM;
}

static void ne6xvf_irq_disable(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_hw *hw = &adapter->hw;
	int i;

	if (!adapter->msix_entries)
		return;

	for (i = 0; i < adapter->num_msix_vectors; i++) {
		wr64(hw, NE6XVF_REG_ADDR(i, NE6X_VP_INT_MASK), 0xffffffffffffffff);
		synchronize_irq(adapter->msix_entries[i].vector);
	}
}

static void ne6xvf_free_traffic_irqs(struct ne6xvf_adapter *adapter)
{
	int vector, irq_num, q_vectors;

	if (!adapter->msix_entries)
		return;

	q_vectors = adapter->num_active_queues;

	for (vector = 0; vector < q_vectors; vector++) {
		irq_num = adapter->msix_entries[vector].vector;
		irq_set_affinity_notifier(irq_num, NULL);
		irq_set_affinity_hint(irq_num, NULL);
		free_irq(irq_num, &adapter->q_vectors[vector]);
	}
}

static void ne6xvf_free_q_vectors(struct ne6xvf_adapter *adapter)
{
	int q_idx, num_q_vectors;
	int napi_vectors;

	if (!adapter->q_vectors)
		return;

	num_q_vectors = adapter->num_msix_vectors;
	napi_vectors = adapter->num_active_queues;

	for (q_idx = 0; q_idx < num_q_vectors; q_idx++) {
		struct ne6x_q_vector *q_vector = &adapter->q_vectors[q_idx];

		if (q_idx < napi_vectors)
			netif_napi_del(&q_vector->napi);
	}

	kfree(adapter->q_vectors);
	adapter->q_vectors = NULL;
}

/**
 * ne6xvf_disable_vf - disable a VF that failed to reset
 * @adapter: private adapter structure
 *
 * Helper function to shut down the VF when a reset never finishes.
 **/
static void ne6xvf_disable_vf(struct ne6xvf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct ne6xvf_vlan_filter *fv, *fvtmp;
	struct ne6xvf_mac_filter *f, *ftmp;

	/* reset never finished */
	adapter->flags |= NE6XVF_FLAG_PF_COMMS_FAILED;

	/* We don't use netif_running() because it may be true prior to
	 * ndo_open() returning, so we can't assume it means all our open
	 * tasks have finished, since we're not holding the rtnl_lock here.
	 */
	if (!test_bit(NE6X_ADPT_DOWN, adapter->comm.state)) {
		set_bit(NE6X_ADPT_DOWN, adapter->comm.state);
		netif_carrier_off(netdev);
		netif_tx_disable(netdev);
		adapter->link_up = false;
		ne6xvf_irq_disable(adapter);
		ne6xvf_napi_disable_all(adapter);
		ne6xvf_free_traffic_irqs(adapter);
		ne6xvf_free_all_tg_resources(adapter);
		ne6xvf_free_all_cq_resources(adapter);
		ne6xvf_free_all_tx_resources(adapter);
		ne6xvf_free_all_rx_resources(adapter);
	}

	spin_lock_bh(&adapter->mac_vlan_list_lock);

	/* Delete all of the filters */
	list_for_each_entry_safe(f, ftmp, &adapter->mac_filter_list, list) {
		list_del(&f->list);
		kfree(f);
	}

	list_for_each_entry_safe(fv, fvtmp, &adapter->vlan_filter_list, list) {
		list_del(&fv->list);
		kfree(fv);
	}

	spin_unlock_bh(&adapter->mac_vlan_list_lock);

	ne6xvf_reset_interrupt_capability(adapter);
	ne6xvf_free_q_vectors(adapter);
	ne6xvf_free_queues(adapter);
	memset(adapter->vf_res, 0, struct_size(adapter->vf_res, vsi_res, 1));
	adapter->netdev->flags &= ~IFF_UP;
	adapter->flags &= ~NE6XVF_FLAG_RESET_PENDING;
	ne6xvf_change_state(adapter, __NE6XVF_DOWN);
	clear_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section);

	dev_info(&adapter->pdev->dev, "Reset task did not complete, VF disabled\n");
}

/**
 * ne6xvf_acquire_msix_vectors - Setup the MSIX capability
 * @adapter: board private structure
 * @vectors: number of vectors to request
 *
 * Work with the OS to set up the MSIX vectors needed.
 *
 * Returns 0 on success, negative on failure
 **/
static int ne6xvf_acquire_msix_vectors(struct ne6xvf_adapter *adapter, int vectors)
{
	int v_actual;

	/* We'll want at least 3 (vector_threshold):
	 * 0) Other (Admin Queue and link, mostly)
	 * 1) TxQ[0] Cleanup
	 * 2) RxQ[0] Cleanup
	 *
	 * The more we get, the more we will assign to Tx/Rx Cleanup
	 * for the separate queues...where Rx Cleanup >= Tx Cleanup.
	 * Right now, we simply care about how many we'll get; we'll
	 * set them up later while requesting irq's.
	 */
	v_actual = pci_enable_msix_range(adapter->pdev, adapter->msix_entries, 1, vectors);
	if (v_actual != vectors) {
		dev_err(&adapter->pdev->dev, "Unable to allocate MSI-X interrupts: %d\n", v_actual);
		kfree(adapter->msix_entries);
		adapter->msix_entries = NULL;
		pci_disable_msi(adapter->pdev);
		return v_actual;
	}

	adapter->num_msix_vectors = v_actual;

	return 0;
}

/**
 * ne6xvf_set_interrupt_capability - set MSI-X or FAIL if not supported
 * @adapter: board private structure to initialize
 *
 * Attempt to configure the interrupts using the best available
 * capabilities of the hardware and the kernel.
 **/
static int ne6xvf_set_interrupt_capability(struct ne6xvf_adapter *adapter)
{
	int vector, v_budget;
	int err = 0;

	if (!adapter->vsi_res)
		return -EIO;

	v_budget = adapter->num_active_queues;
	adapter->msix_entries = kcalloc(v_budget, sizeof(struct msix_entry), GFP_KERNEL);
	if (!adapter->msix_entries) {
		err = -ENOMEM;
		goto out;
	}

	for (vector = 0; vector < v_budget; vector++)
		adapter->msix_entries[vector].entry = vector;

	dev_info(&adapter->pdev->dev, "v_budget:%d, adapter->vf_res->max_vectors: %d\n", v_budget,
		 adapter->vf_res->max_vectors);
	err = ne6xvf_acquire_msix_vectors(adapter, v_budget);
out:
	netif_set_real_num_rx_queues(adapter->netdev, v_budget);
	netif_set_real_num_tx_queues(adapter->netdev, v_budget);

	return err;
}

/**
 * ne6xvf_fill_rss_lut - Fill the lut with default values
 * @adapter: board private structure
 **/
void ne6xvf_fill_rss_lut(struct ne6xvf_adapter *adapter)
{
	u16 i;

	for (i = 0; i < adapter->rss_info.ind_table_size; i++)
		adapter->rss_info.ind_table[i] = i % adapter->num_active_queues;
}

/**
 * ne6xvf_init_rss - Prepare for RSS
 * @adapter: board private structure
 *
 * Return 0 on success, negative on failure
 **/
static int ne6xvf_init_rss(struct ne6xvf_adapter *adapter)
{
	struct ne6x_rss_info *rss_info = &adapter->rss_info;

	/* begin rss info */
	rss_info->hash_type = NE6X_RSS_HASH_TYPE_IPV4_TCP |
			      NE6X_RSS_HASH_TYPE_IPV4_UDP |
			      NE6X_RSS_HASH_TYPE_IPV4 |
			      NE6X_RSS_HASH_TYPE_IPV6_TCP |
			      NE6X_RSS_HASH_TYPE_IPV6_UDP |
			      NE6X_RSS_HASH_TYPE_IPV6;
	rss_info->hash_func = NE6X_RSS_HASH_FUNC_TOEPLITZ;
	rss_info->hash_key_size = NE6X_RSS_MAX_KEY_SIZE;
	rss_info->ind_table_size = NE6X_RSS_MAX_IND_TABLE_SIZE;
	ne6xvf_fill_rss_lut(adapter);
	netdev_rss_key_fill((void *)&adapter->rss_info.hash_key[0],
			    adapter->rss_info.hash_key_size);
	adapter->aq_required |= NE6XVF_FLAG_AQ_CONFIGURE_RSS;
	adapter->aq_required |= NE6XVF_FLAG_AQ_CHANGED_RSS;

	return 0;
}

/**
 * ne6xvf_alloc_q_vectors - Allocate memory for interrupt vectors
 * @adapter: board private structure to initialize
 *
 * We allocate one q_vector per queue interrupt.  If allocation fails we
 * return -ENOMEM.
 **/
static int ne6xvf_alloc_q_vectors(struct ne6xvf_adapter *adapter)
{
	struct ne6x_q_vector *q_vector;
	int q_idx, num_q_vectors;

	num_q_vectors = adapter->num_active_queues;
	adapter->q_vectors = kcalloc(num_q_vectors, sizeof(*q_vector), GFP_KERNEL);
	if (!adapter->q_vectors)
		return -ENOMEM;

	for (q_idx = 0; q_idx < num_q_vectors; q_idx++) {
		q_vector = &adapter->q_vectors[q_idx];
		q_vector->adpt = adapter;
		q_vector->v_idx = q_idx;
		q_vector->reg_idx = q_idx;
		cpumask_copy(&q_vector->affinity_mask, cpu_possible_mask);
		netif_napi_add(adapter->netdev, &q_vector->napi, ne6xvf_napi_poll);
	}

	return 0;
}

/**
 * ne6xvf_init_interrupt_scheme - Determine if MSIX is supported and init
 * @adapter: board private structure to initialize
 *
 **/
int ne6xvf_init_interrupt_scheme(struct ne6xvf_adapter *adapter)
{
	int err;

	err = ne6xvf_alloc_queues(adapter);
	if (err) {
		dev_err(&adapter->pdev->dev, "Unable to allocate memory for queues\n");
		goto err_alloc_queues;
	}

	rtnl_lock();
	err = ne6xvf_set_interrupt_capability(adapter);
	rtnl_unlock();
	if (err) {
		dev_err(&adapter->pdev->dev, "Unable to setup interrupt capabilities\n");
		goto err_set_interrupt;
	}

	err = ne6xvf_alloc_q_vectors(adapter);
	if (err) {
		dev_err(&adapter->pdev->dev, "Unable to allocate memory for queue vectors\n");
		goto err_alloc_q_vectors;
	}

	dev_info(&adapter->pdev->dev, "Multiqueue %s: Queue pair count = %u",
		 (adapter->num_active_queues > 1) ? "Enabled" : "Disabled",
		 adapter->num_active_queues);

	return 0;

err_alloc_q_vectors:
	ne6xvf_reset_interrupt_capability(adapter);
err_set_interrupt:
	ne6xvf_free_queues(adapter);
err_alloc_queues:
	return err;
}

/**
 * ne6xvf_map_vector_to_cq - associate irqs with complete queues
 * @adapter: board private structure
 * @v_idx: interrupt number
 * @r_idx: queue number
 **/
static void ne6xvf_map_vector_to_cq(struct ne6xvf_adapter *adapter, int v_idx, int r_idx)
{
	struct ne6x_q_vector *q_vector = &adapter->q_vectors[v_idx];
	struct ne6x_ring *cq_ring = &adapter->cq_rings[r_idx];

	cq_ring->q_vector = q_vector;
	cq_ring->next = q_vector->cq.ring;
	q_vector->cq.ring = cq_ring;
	q_vector->cq.count++;
}

/**
 * ne6xvf_map_vector_to_rxq - associate irqs with rx queues
 * @adapter: board private structure
 * @v_idx: interrupt number
 * @r_idx: queue number
 **/
static void ne6xvf_map_vector_to_rxq(struct ne6xvf_adapter *adapter, int v_idx, int r_idx)
{
	struct ne6x_q_vector *q_vector = &adapter->q_vectors[v_idx];
	struct ne6x_ring *rx_ring = &adapter->rx_rings[r_idx];

	rx_ring->q_vector = q_vector;
	rx_ring->next = q_vector->rx.ring;
	q_vector->rx.ring = rx_ring;
	q_vector->rx.count++;
}

/**
 * ne6xvf_map_vector_to_txq - associate irqs with tx queues
 * @adapter: board private structure
 * @v_idx: interrupt number
 * @t_idx: queue number
 **/
static void ne6xvf_map_vector_to_txq(struct ne6xvf_adapter *adapter, int v_idx, int t_idx)
{
	struct ne6x_q_vector *q_vector = &adapter->q_vectors[v_idx];
	struct ne6x_ring *tx_ring = &adapter->tx_rings[t_idx];

	tx_ring->q_vector = q_vector;
	tx_ring->next = q_vector->tx.ring;
	q_vector->tx.ring = tx_ring;
	q_vector->tx.count++;
	q_vector->num_ringpairs++;
}

/**
 * ne6xvf_map_rings_to_vectors - Maps descriptor rings to vectors
 * @adapter: board private structure to initialize
 *
 * This function maps descriptor rings to the queue-specific vectors
 * we were allotted through the MSI-X enabling code.  Ideally, we'd have
 * one vector per ring/queue, but on a constrained vector budget, we
 * group the rings as "efficiently" as possible.  You would add new
 * mapping configurations in here.
 **/
static void ne6xvf_map_rings_to_vectors(struct ne6xvf_adapter *adapter)
{
	int rings_remaining = adapter->num_active_queues;
	int q_vectors;
	int ridx;

	q_vectors = adapter->num_msix_vectors;

	for (ridx = 0; ridx < rings_remaining; ridx++) {
		ne6xvf_map_vector_to_cq(adapter, ridx, ridx);
		ne6xvf_map_vector_to_rxq(adapter, ridx, ridx);
		ne6xvf_map_vector_to_txq(adapter, ridx, ridx);
	}
}

/**
 * ne6xvf_setup_all_tg_resources - allocate all queues Tg resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int ne6xvf_setup_all_tg_resources(struct ne6xvf_adapter *adapter)
{
	int i, err = 0;

	for (i = 0; i < adapter->num_active_queues; i++) {
		adapter->tg_rings[i].count = adapter->tx_desc_count;
		err = ne6x_setup_tg_descriptors(&adapter->tg_rings[i]);
		if (!err)
			continue;

		dev_err(&adapter->pdev->dev, "tg Allocation for complete Queue %u failed\n", i);
		break;
	}

	return err;
}

/**
 * ne6xvf_setup_all_cq_resources - allocate all queues Tx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int ne6xvf_setup_all_cq_resources(struct ne6xvf_adapter *adapter)
{
	int i, err = 0;

	for (i = 0; i < adapter->num_active_queues; i++) {
		adapter->cq_rings[i].count = adapter->tx_desc_count;
		err = ne6x_setup_cq_descriptors(&adapter->cq_rings[i]);
		if (!err)
			continue;

		dev_err(&adapter->pdev->dev, "Allocation for complete Queue %u failed\n", i);
		break;
	}

	return err;
}

/**
 * ne6xvf_setup_all_tx_resources - allocate all queues Tx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int ne6xvf_setup_all_tx_resources(struct ne6xvf_adapter *adapter)
{
	int i, err = 0;

	for (i = 0; i < adapter->num_active_queues; i++) {
		adapter->tx_rings[i].count = adapter->tx_desc_count;
		err = ne6x_setup_tx_descriptors(&adapter->tx_rings[i]);
		err |= ne6x_setup_tx_sgl(&adapter->tx_rings[i]);
		if (!err)
			continue;

		dev_err(&adapter->pdev->dev, "Allocation for Tx Queue %u failed\n", i);
		break;
	}

	return err;
}

/**
 * ne6xvf_setup_all_rx_resources - allocate all queues Rx resources
 * @adapter: board private structure
 *
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 *
 * Return 0 on success, negative on failure
 **/
static int ne6xvf_setup_all_rx_resources(struct ne6xvf_adapter *adapter)
{
	int i, err = 0;

	for (i = 0; i < adapter->num_active_queues; i++) {
		adapter->rx_rings[i].count = adapter->rx_desc_count;
		err = ne6x_setup_rx_descriptors(&adapter->rx_rings[i]);
		if (!err)
			continue;

		dev_err(&adapter->pdev->dev, "Allocation for Rx Queue %u failed\n", i);
		break;
	}

	return err;
}

/**
 * ne6xvf_msix_clean_rings - MSIX mode Interrupt Handler
 * @irq: interrupt number
 * @data: pointer to a q_vector
 **/
static irqreturn_t ne6xvf_msix_clean_rings(int irq, void *data)
{
	struct ne6x_q_vector *q_vector = data;
	struct ne6xvf_adapter *adpt = (struct ne6xvf_adapter *)q_vector->adpt;
	u64 val;

	if (!q_vector->tx.ring && !q_vector->rx.ring && !q_vector->cq.ring)
		return IRQ_HANDLED;

	napi_schedule_irqoff(&q_vector->napi);
	val = rd64(&adpt->hw, NE6XVF_REG_ADDR(q_vector->reg_idx, NE6X_VP_INT_MASK));
	val |= 1ULL << NE6X_VP_CQ_INTSHIFT;
	wr64(&adpt->hw, NE6XVF_REG_ADDR(q_vector->reg_idx, NE6X_VP_INT_MASK), val);

	return IRQ_HANDLED;
}

/**
 * ne6xvf_irq_affinity_notify - Callback for affinity changes
 * @notify: context as to what irq was changed
 * @mask: the new affinity mask
 *
 * This is a callback function used by the irq_set_affinity_notifier function
 * so that we may register to receive changes to the irq affinity masks.
 **/
static void ne6xvf_irq_affinity_notify(struct irq_affinity_notify *notify, const cpumask_t *mask)
{
	struct ne6x_q_vector *q_vector;

	q_vector = container_of(notify, struct ne6x_q_vector, affinity_notify);
	cpumask_copy(&q_vector->affinity_mask, mask);
}

/**
 * ne6xvf_irq_affinity_release - Callback for affinity notifier release
 * @ref: internal core kernel usage
 *
 * This is a callback function used by the irq_set_affinity_notifier function
 * to inform the current notification subscriber that they will no longer
 * receive notifications.
 **/
static void ne6xvf_irq_affinity_release(struct kref *ref) {}

/**
 * ne6xvf_request_traffic_irqs - Initialize MSI-X interrupts
 * @adapter: board private structure
 * @basename: device basename
 *
 * Allocates MSI-X vectors for tx and rx handling, and requests
 * interrupts from the kernel.
 **/
static int ne6xvf_request_traffic_irqs(struct ne6xvf_adapter *adapter, char *basename)
{
	unsigned int rx_int_idx = 0, tx_int_idx = 0;
	unsigned int vector, q_vectors;
	int irq_num, err;
	int cpu;

	ne6xvf_irq_disable(adapter);
	/* Decrement for Other and TCP Timer vectors */
	q_vectors = adapter->num_active_queues;

	for (vector = 0; vector < q_vectors; vector++) {
		struct ne6x_q_vector *q_vector = &adapter->q_vectors[vector];

		irq_num = adapter->msix_entries[vector].vector;

		if (q_vector->tx.ring && q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name), "ne6xvf-%s-TxRx-%u",
				 basename, rx_int_idx++);
			tx_int_idx++;
		} else if (q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name),
				 "ne6xvf-%s-rx-%u", basename,
				 rx_int_idx++);
		} else if (q_vector->tx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name),
				 "ne6xvf-%s-tx-%u", basename,
				 tx_int_idx++);
		} else {
			/* skip this unused q_vector */
			continue;
		}

		err = request_irq(irq_num, ne6xvf_msix_clean_rings, 0, q_vector->name, q_vector);
		if (err) {
			dev_info(&adapter->pdev->dev, "Request_irq failed, error: %d\n", err);
			goto free_queue_irqs;
		}

		/* register for affinity change notifications */
		q_vector->affinity_notify.notify = ne6xvf_irq_affinity_notify;
		q_vector->affinity_notify.release = ne6xvf_irq_affinity_release;
		irq_set_affinity_notifier(irq_num, &q_vector->affinity_notify);

		/* Spread the IRQ affinity hints across online CPUs. Note that
		 * get_cpu_mask returns a mask with a permanent lifetime so
		 * it's safe to use as a hint for irq_set_affinity_hint.
		 */
		cpu = cpumask_local_spread(q_vector->v_idx, -1);
		irq_set_affinity_hint(irq_num, get_cpu_mask(cpu));
	}

	return 0;

free_queue_irqs:
	while (vector) {
		vector--;
		irq_num = adapter->msix_entries[vector].vector;
		irq_set_affinity_notifier(irq_num, NULL);
		irq_set_affinity_hint(irq_num, NULL);
		free_irq(irq_num, &adapter->q_vectors[vector]);
	}

	return err;
}

/**
 * ne6xvf_configure_queues
 * @adapter: adapter structure
 *
 * Request that the PF set up our (previously allocated) queues.
 **/
void ne6xvf_configure_queues(struct ne6xvf_adapter *adapter)
{
	unsigned int rx_buf_len = NE6X_RXBUFFER_2048;
	struct ne6xvf_hw *hw = &adapter->hw;
	union ne6x_sq_base_addr sq_base_addr;
	union ne6x_rq_base_addr rq_base_addr;
	union ne6x_rq_block_cfg rq_block_cfg;
	union ne6x_cq_base_addr cq_base_addr;
	union ne6x_cq_cfg cq_cfg;
	union ne6x_sq_cfg sq_cfg;
	union ne6x_rq_cfg rc_cfg;
	int i;

	/* Legacy Rx will always default to a 2048 buffer size. */
#if (PAGE_SIZE < 8192)
	if (!(adapter->flags & NE6XVF_FLAG_LEGACY_RX))
		/* For jumbo frames on systems with 4K pages we have to use
		 * an order 1 page, so we might as well increase the size
		 * of our Rx buffer to make better use of the available space
		 */
		rx_buf_len = NE6X_RXBUFFER_4096;
#endif

	for (i = 0; i < adapter->num_active_queues; i++)
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_VP_RELOAD), 0x1);

	usleep_range(100, 120);

	for (i = 0; i < adapter->num_active_queues; i++) {
		/* cq */
		/* cache tail for quicker writes, and clear the reg before use */
		adapter->cq_rings[i].tail = (u64 __iomem *)(hw->hw_addr0 + NE6XVF_QC_TAIL1(i));
		adapter->cq_rings[i].reg_idx = hw->dev_caps.base_queue + i;

		cq_base_addr.val = NE6XVF_READ_REG(hw, NE6XVF_REG_ADDR(i, NE6X_CQ_BASE_ADDR));
		cq_base_addr.reg.csr_cq_base_addr_vp = adapter->cq_rings[i].dma;
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_CQ_BASE_ADDR), cq_base_addr.val);

		cq_cfg.val = NE6XVF_READ_REG(hw, NE6XVF_REG_ADDR(i, NE6X_CQ_CFG));
		cq_cfg.reg.csr_cq_len_vp = adapter->cq_rings[i].count;
		cq_cfg.reg.csr_cq_merge_time_vp = 7;
		cq_cfg.reg.csr_cq_merge_size_vp = 7;
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_CQ_CFG), cq_cfg.val);
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_CQ_TAIL_POINTER), 0x0);
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_CQ_HD_POINTER), 0x0);

		/* tx */
		/* cache tail off for easier writes later */
		adapter->tx_rings[i].tail = (u64 __iomem *)(hw->hw_addr2 + NE6XVF_QTX_TAIL1(i));
		adapter->tx_rings[i].reg_idx = hw->dev_caps.base_queue + i;

		sq_base_addr.val = NE6XVF_READ_REG(hw, NE6XVF_REG_ADDR(i, NE6X_SQ_BASE_ADDR));
		sq_base_addr.reg.csr_sq_base_addr_vp = adapter->tx_rings[i].dma;
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_SQ_BASE_ADDR), sq_base_addr.val);

		sq_cfg.val = NE6XVF_READ_REG(hw, NE6XVF_REG_ADDR(i, NE6X_SQ_CFG));
		sq_cfg.reg.csr_sq_len_vp = adapter->tx_rings[i].count;
		sq_cfg.reg.csr_tdq_pull_en = 0x1;
		sq_cfg.reg.csr_sqevt_write_back_vp = 0x0;
		sq_cfg.reg.csr_send_pd_revers_en = 0x0;
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_SQ_CFG), sq_cfg.val);
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_SQ_HD_POINTER), 0x0);

		/* rx */
		/* cache tail for quicker writes, and clear the reg before use */
		adapter->rx_rings[i].tail = (u64 __iomem *)(hw->hw_addr2 + NE6XVF_QRX_TAIL1(i));
		adapter->rx_rings[i].rx_buf_len = rx_buf_len;
		adapter->rx_rings[i].reg_idx = hw->dev_caps.base_queue + i;

		rq_base_addr.val = NE6XVF_READ_REG(hw, NE6XVF_REG_ADDR(i, NE6X_RQ_BASE_ADDR));
		rq_base_addr.reg.csr_rq_base_addr_vp = adapter->rx_rings[i].dma;
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_RQ_BASE_ADDR), rq_base_addr.val);

		rq_block_cfg.val = NE6XVF_READ_REG(hw, NE6XVF_REG_ADDR(i, NE6X_RQ_BLOCK_CFG));
		rq_block_cfg.reg.csr_rdq_mop_len = adapter->rx_rings[i].rx_buf_len;
		rq_block_cfg.reg.csr_rdq_sop_len = 0;
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_RQ_BLOCK_CFG), rq_block_cfg.val);

		rc_cfg.val = NE6XVF_READ_REG(hw, NE6XVF_REG_ADDR(i, NE6X_RQ_CFG));
		rc_cfg.reg.csr_rq_len_vp = adapter->rx_rings[i].count;
		rc_cfg.reg.csr_rdq_pull_en = 0x1;
		rc_cfg.reg.csr_rqevt_write_back_vp = 0x0;
		rc_cfg.reg.csr_recv_pd_type_vp = 0x0;
		rc_cfg.reg.csr_recv_pd_revers_en = 0x0;
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_RQ_CFG), rc_cfg.val);
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_RQ_HD_POINTER), 0x0);
	}

	for (i = 0; i < adapter->num_active_queues; i++)
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_VP_RELOAD), 0x0);

	usleep_range(100, 120);
}

/**
 * ne6xvf_configure - set up transmit and receive data structures
 * @adapter: board private structure
 **/
static void ne6xvf_configure(struct ne6xvf_adapter *adapter)
{
	int i;

	ne6xvf_configure_queues(adapter);

	adapter->aq_required |= NE6XVF_FLAG_AQ_CONFIGURE_QUEUES;

	for (i = 0; i < adapter->num_active_queues; i++) {
		struct ne6x_ring *ring = &adapter->rx_rings[i];

		ne6x_alloc_rx_buffers(ring, NE6X_DESC_UNUSED(ring));
		usleep_range(1000, 2000);
	}
}

/**
 * ne6xvf_napi_enable_all - enable NAPI on all queue vectors
 * @adapter: board private structure
 **/
static void ne6xvf_napi_enable_all(struct ne6xvf_adapter *adapter)
{
	int q_vectors = adapter->num_msix_vectors;
	struct ne6x_q_vector *q_vector;
	int q_idx;

	for (q_idx = 0; q_idx < q_vectors; q_idx++) {
		struct napi_struct *napi;

		q_vector = &adapter->q_vectors[q_idx];
		napi = &q_vector->napi;
		napi_enable(napi);
	}
}

/**
 * ne6xvf_up_complete - Finish the last steps of bringing up a connection
 * @adapter: board private structure
 *
 * Expects to be called while holding the __NE6XVF_IN_CRITICAL_TASK bit lock.
 **/
static void ne6xvf_up_complete(struct ne6xvf_adapter *adapter)
{
	ne6xvf_change_state(adapter, __NE6XVF_RUNNING);
	clear_bit(NE6X_ADPT_DOWN, adapter->comm.state);

	ne6xvf_napi_enable_all(adapter);

	adapter->aq_required |= NE6XVF_FLAG_AQ_ENABLE_QUEUES;
	mod_delayed_work(ne6xvf_wq, &adapter->watchdog_task, 0);
}

/**
 * ne6xvf_reinit_interrupt_scheme - Reallocate queues and vectors
 * @adapter: board private structure
 *
 * Returns 0 on success, negative on failure
 **/
static int ne6xvf_reinit_interrupt_scheme(struct ne6xvf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int err;

	if (!test_bit(NE6X_ADPT_DOWN, adapter->comm.state))
		ne6xvf_free_traffic_irqs(adapter);

	ne6xvf_reset_interrupt_capability(adapter);
	ne6xvf_free_q_vectors(adapter);
	ne6xvf_free_queues(adapter);

	err =  ne6xvf_init_interrupt_scheme(adapter);
	if (err)
		goto err;

	netif_tx_stop_all_queues(netdev);

	set_bit(NE6X_ADPT_DOWN, adapter->comm.state);

	ne6xvf_map_rings_to_vectors(adapter);
err:
	return err;
}

static void ne6xvf_get_port_link_status(struct ne6xvf_adapter *adapter);

/**
 * ne6xvf_handle_reset - Handle hardware reset
 * @adapter: pointer to ne6xvf_adapter
 *
 * During reset we need to shut down and reinitialize the admin queue
 * before we can use it to communicate with the PF again. We also clear
 * and reinit the rings because that context is lost as well.
 *
 * This function is called in the __NE6XVF_RESETTING driver state. If a reset
 * is detected and completes, the driver state changed to __NE6XVF_RUNNING or
 * __NE6XVF_DOWN, else driver state will remain in __NE6XVF_RESETTING.
 *
 * The function is called with the NE6XVF_FLAG_RESET_PENDING flag set and it is
 * cleared when a reset is detected and completes.
 **/
static void ne6xvf_handle_reset(struct ne6xvf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct ne6xvf_hw *hw = &adapter->hw;
	bool running;
	int err, i;

	/* We don't use netif_running() because it may be true prior to
	 * ndo_open() returning, so we can't assume it means all our open
	 * tasks have finished, since we're not holding the rtnl_lock here.
	 */
	running = (adapter->last_state == __NE6XVF_RUNNING);

	if (running) {
		netdev->flags &= ~IFF_UP;
		netif_carrier_off(netdev);
		netif_tx_stop_all_queues(netdev);
		adapter->link_up = false;
		ne6xvf_napi_disable_all(adapter);
	}

	pci_set_master(adapter->pdev);
	pci_restore_msi_state(adapter->pdev);

	ne6xvf_irq_disable(adapter);

	for (i = 0; i < adapter->num_msix_vectors; i++)
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_VP_RELOAD), 0x1);

	usleep_range(100, 120);

	/* free the Tx/Rx rings and descriptors, might be better to just
	 * re-use them sometime in the future
	 */
	ne6xvf_free_all_tg_resources(adapter);
	ne6xvf_free_all_cq_resources(adapter);
	ne6xvf_free_all_rx_resources(adapter);
	ne6xvf_free_all_tx_resources(adapter);

	/* Set the queues_disabled flag when VF is going through reset
	 * to avoid a race condition especially for ADQ i.e. when a VF ADQ is
	 * configured, PF resets the VF to allocate ADQ resources. When this
	 * happens there's a possibility to hit a condition where VF is in
	 * running state but the queues haven't been enabled yet. So wait for
	 * virtchnl success message for enable queues and then unset this flag.
	 * Don't allow the link to come back up until that happens.
	 */
	adapter->flags |= NE6XVF_FLAG_QUEUES_DISABLED;

	adapter->current_op = VIRTCHNL_OP_UNKNOWN;
	adapter->aq_required = 0;

	err = ne6xvf_reinit_interrupt_scheme(adapter);
	if (err)
		goto reset_err;

	adapter->aq_required |= NE6XVF_FLAG_AQ_GET_CONFIG;
	adapter->aq_required |= NE6XVF_FLAG_AQ_MAP_VECTORS;

	/* We were running when the reset started, so we need
	 * to restore some state here.
	 */
	if (running) {
		err = ne6xvf_setup_all_tg_resources(adapter);
		if (err)
			goto reset_err;

		err = ne6xvf_setup_all_cq_resources(adapter);
		if (err)
			goto reset_err;

		/* allocate transmit descriptors */
		err = ne6xvf_setup_all_tx_resources(adapter);
		if (err)
			goto reset_err;

		/* allocate receive descriptors */
		err = ne6xvf_setup_all_rx_resources(adapter);
		if (err)
			goto reset_err;

		if ((adapter->flags & NE6XVF_FLAG_REINIT_MSIX_NEEDED) ||
		    (adapter->flags & NE6XVF_FLAG_REINIT_ITR_NEEDED)) {
			err = ne6xvf_request_traffic_irqs(adapter, netdev->name);
			if (err)
				goto reset_err;

			adapter->flags &= ~NE6XVF_FLAG_REINIT_MSIX_NEEDED;
		}

		ne6xvf_configure(adapter);

		/* ne6xvf_up_complete() will switch device back
		 * to __NE6XVF_RUNNING
		 */
		ne6xvf_up_complete(adapter);

		ne6xvf_irq_enable(adapter, true);

		ne6xvf_get_port_link_status(adapter);

		netdev->flags |= IFF_UP;
	} else {
		ne6xvf_change_state(adapter, __NE6XVF_DOWN);
	}

	adapter->flags &= ~NE6XVF_FLAG_REINIT_ITR_NEEDED;

	return;

reset_err:
	if (running) {
		set_bit(NE6X_ADPT_DOWN, adapter->comm.state);
		ne6xvf_free_traffic_irqs(adapter);
		netdev->flags &= ~IFF_UP;
	}

	dev_err(&adapter->pdev->dev, "failed to allocate resources during reinit\n");
	ne6xvf_disable_vf(adapter);
}

/**
 * ne6xvf_init_process_extended_caps - Part of driver startup
 * @adapter: board private structure
 *
 * Function processes __NE6XVF_INIT_EXTENDED_CAPS driver state. This state
 * handles negotiating capabilities for features which require an additional
 * message.
 *
 * Once all extended capabilities exchanges are finished, the driver will
 * transition into __NE6XVF_INIT_CONFIG_ADAPTER.
 */
static void ne6xvf_init_process_extended_caps(struct ne6xvf_adapter *adapter)
{
	WARN_ON(adapter->state != __NE6XVF_INIT_EXTENDED_CAPS);

	/* When we reach here, no further extended capabilities exchanges are
	 * necessary, so we finally transition into __NE6XVF_INIT_CONFIG_ADAPTER
	 */
	adapter->vsi_res->num_queue_pairs = adapter->vf_res->num_queue_pairs;
	adapter->hw_feature = 0x00;
	ne6xvf_change_state(adapter, __NE6XVF_INIT_CONFIG_ADAPTER);
}

/**
 * ne6xvf_process_config - Process the config information we got from the PF
 * @adapter: board private structure
 *
 * Verify that we have a valid config struct, and set up our netdev features
 * and our VSI struct.
 **/
int ne6xvf_process_config(struct ne6xvf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	netdev_features_t csumo_features;
	netdev_features_t vlano_features;
	netdev_features_t dflt_features;
	netdev_features_t tso_features;

	dflt_features = NETIF_F_SG |
			NETIF_F_HIGHDMA |
			NETIF_F_RXHASH;

	csumo_features = NETIF_F_RXCSUM |
			 NETIF_F_IP_CSUM |
			 NETIF_F_SCTP_CRC |
			 NETIF_F_IPV6_CSUM;

	vlano_features = NETIF_F_HW_VLAN_CTAG_FILTER |
			 NETIF_F_HW_VLAN_CTAG_TX |
			 NETIF_F_HW_VLAN_CTAG_RX;

	/* Enable CTAG/STAG filtering by default in Double VLAN Mode (DVM) */
	tso_features = NETIF_F_TSO |
		       NETIF_F_TSO_ECN |
		       NETIF_F_TSO6 |
		       NETIF_F_GSO_GRE |
		       NETIF_F_GSO_UDP_TUNNEL |
		       NETIF_F_LRO |
		       NETIF_F_LOOPBACK |
		       NETIF_F_GSO_GRE_CSUM |
		       NETIF_F_GSO_UDP_TUNNEL_CSUM |
		       NETIF_F_GSO_PARTIAL |
		       NETIF_F_GSO_IPXIP4 |
		       NETIF_F_GSO_IPXIP6 |
		       NETIF_F_GSO_UDP_L4 |
		       NETIF_F_GSO_SCTP |
		       0;

	netdev->gso_partial_features |= NETIF_F_GSO_UDP_TUNNEL_CSUM | NETIF_F_GSO_GRE_CSUM;

	/* set features that user can change */
	netdev->hw_features = dflt_features | csumo_features | vlano_features | tso_features;

	/* add support for HW_CSUM on packets with MPLS header */
	netdev->mpls_features = NETIF_F_HW_CSUM;

	netdev->hw_features |= NETIF_F_HW_L2FW_DOFFLOAD;

	/* enable features */
	netdev->features |= netdev->hw_features;
	/* encap and VLAN devices inherit default, csumo and tso features */
	netdev->hw_enc_features |= dflt_features | csumo_features | tso_features;
	netdev->vlan_features |= dflt_features | csumo_features | tso_features;
	netdev->hw_features |= NETIF_F_HW_TC;

	/* advertise support but don't enable by default since only one type of
	 * VLAN offload can be enabled at a time (i.e. CTAG or STAG). When one
	 * type turns on the other has to be turned off. This is enforced by the
	 * nce_fix_features() ndo callback.
	 */
	netdev->hw_features |= NETIF_F_HW_VLAN_STAG_RX |
			       NETIF_F_HW_VLAN_STAG_TX |
			       NETIF_F_HW_VLAN_STAG_FILTER;

	netdev->gso_max_size = 65535;
	netdev->features = netdev->hw_features;
	ne6xvf_sync_features(netdev);

	return 0;
}

/**
 * ne6xvf_init_config_adapter - last part of driver startup
 * @adapter: board private structure
 *
 * After all the supported capabilities are negotiated, then the
 * __NE6XVF_INIT_CONFIG_ADAPTER state will finish driver initialization.
 */
static void ne6xvf_init_config_adapter(struct ne6xvf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct pci_dev *pdev = adapter->pdev;
	int ret;

	WARN_ON(adapter->state != __NE6XVF_INIT_CONFIG_ADAPTER);

	if (ne6xvf_process_config(adapter))
		goto err;

	adapter->current_op = VIRTCHNL_OP_UNKNOWN;

	adapter->flags |= NE6XVF_FLAG_RX_CSUM_ENABLED;

	netdev->netdev_ops = &ne6xvf_netdev_ops;
	ne6xvf_set_ethtool_ops(netdev);
	netdev->watchdog_timeo = 5 * HZ;

	netdev->min_mtu = NE6X_MIN_MTU_SIZE;
	netdev->max_mtu = NE6X_MAX_RXBUFFER - ETH_HLEN - ETH_FCS_LEN;

	if (!is_valid_ether_addr(adapter->hw.mac.addr)) {
		dev_info(&pdev->dev, "Invalid MAC address %pM, using random\n",
			 adapter->hw.mac.addr);
		eth_hw_addr_random(netdev);
		ether_addr_copy(adapter->hw.mac.addr, netdev->dev_addr);
	} else {
		eth_hw_addr_set(netdev, adapter->hw.mac.addr);
		ether_addr_copy(netdev->perm_addr, adapter->hw.mac.addr);
	}

	adapter->tx_desc_count = ALIGN(NE6X_DEFAULT_NUM_DESCRIPTORS, NE6X_REQ_DESCRIPTOR_MULTIPLE);
	adapter->rx_desc_count = ALIGN(NE6X_DEFAULT_NUM_DESCRIPTORS, NE6X_REQ_DESCRIPTOR_MULTIPLE);
	adapter->cq_desc_count = adapter->tx_desc_count + adapter->rx_desc_count;
	ret = ne6xvf_init_interrupt_scheme(adapter);
	if (ret)
		goto err_sw_init;

	ne6xvf_map_rings_to_vectors(adapter);

	netif_carrier_off(netdev);
	adapter->link_up = false;
	if (!adapter->netdev_registered) {
		ret = ne6xvf_register_netdev(adapter);
		if (ret)
			goto err_register;
	}
	adapter->netdev_registered = true;

	netif_tx_stop_all_queues(netdev);
	ne6xvf_change_state(adapter, __NE6XVF_DOWN);
	set_bit(NE6X_ADPT_DOWN, adapter->comm.state);

	wake_up(&adapter->down_waitqueue);
	ne6xvf_init_rss(adapter);
	adapter->trusted = 0;
	return;

err_register:
err_sw_init:
	ne6xvf_reset_interrupt_capability(adapter);
err:
	ne6xvf_change_state(adapter, __NE6XVF_INIT_FAILED);
}

/**
 * ne6xvf_process_aq_command - process aq_required flags
 * and sends aq command
 * @adapter: pointer to ne6xvf adapter structure
 *
 * Returns 0 on success
 * Returns error code if no command was sent
 * or error code if the command failed.
 **/
static int ne6xvf_process_aq_command(struct ne6xvf_adapter *adapter)
{
	struct ne6xvf_arq_event_info event = {.buf_len = 0, .msg_buf = NULL};

	if (adapter->aq_required & NE6XVF_FLAG_AQ_GET_CONFIG)
		return ne6xvf_send_vf_config_msg(adapter, false);

	if (adapter->aq_required & NE6XVF_FLAG_AQ_CONFIGURE_HW_OFFLOAD)
		return ne6xvf_send_vf_offload_msg(adapter);

	if (adapter->aq_required & NE6XVF_FLAG_AQ_CONFIGURE_RSS) {
		ne6xvf_config_rss_info(adapter);
		return 0;
	}

	if (adapter->aq_required & NE6XVF_FLAG_AQ_CHANGED_RSS) {
		ne6xvf_changed_rss(adapter);
		return 0;
	}

	if (adapter->aq_required & NE6XVF_FLAG_AQ_CONFIGURE_QUEUES) {
		if (ne6xvf_request_queues(adapter, adapter->num_active_queues) == 0) {
			usleep_range(50, 100);
			if (ne6xvf_poll_virtchnl_msg(adapter, &event,
						     VIRTCHNL_OP_REQUEST_QUEUES) == 0) {
				adapter->current_op = VIRTCHNL_OP_UNKNOWN;
				adapter->aq_required &= ~NE6XVF_FLAG_AQ_CONFIGURE_QUEUES;
			}
		}
		return 0;
	}
	if (adapter->aq_required & NE6XVF_FLAG_AQ_ENABLE_QUEUES) {
		ne6xvf_enable_queues(adapter);
		adapter->aq_required &= ~NE6XVF_FLAG_AQ_ENABLE_QUEUES;
		return 0;
	}

	if (adapter->aq_required & NE6XVF_FLAG_AQ_GET_PORT_LINK_STATUS) {
		ne6xvf_vchanel_get_port_link_status(adapter);
		return 0;
	}

	if (adapter->aq_required & NE6XVF_FLAG_AQ_SET_VF_MAC) {
		ne6xvf_set_vf_addr(adapter);
		return 0;
	}

	if (adapter->aq_required & NE6XVF_FLAG_AQ_ADD_MAC_FILTER) {
		ne6xvf_add_ether_addrs(adapter);
		return 0;
	}

	if (adapter->aq_required & NE6XVF_FLAG_AQ_DEL_MAC_FILTER) {
		ne6xvf_del_ether_addrs(adapter);
		return 0;
	}

	if (adapter->aq_required & NE6XVF_FLAG_AQ_ADD_VLAN_FILTER) {
		ne6xvf_add_vlans(adapter);
		adapter->aq_required &= ~NE6XVF_FLAG_AQ_ADD_VLAN_FILTER;
		return 0;
	}

	if (adapter->aq_required & NE6XVF_FLAG_AQ_DEL_VLAN_FILTER) {
		ne6xvf_del_vlans(adapter);
		adapter->aq_required &= ~NE6XVF_FLAG_AQ_DEL_VLAN_FILTER;
		return 0;
	}

	if (adapter->aq_required & NE6XVF_FLAG_AQ_REQUEST_PROMISC) {
		adapter->aq_required &= ~NE6XVF_FLAG_AQ_REQUEST_PROMISC;
		ne6xvf_set_promiscuous(adapter);

		return 0;
	}
	return -EAGAIN;
}

/**
 *  ne6xvf_asq_done - check if FW has processed the Admin Send Queue
 *  @hw: pointer to the hw struct
 *
 *  Returns true if the firmware has processed all descriptors on the
 *  admin send queue. Returns false if there are still requests pending.
 **/
bool ne6xvf_asq_done(struct ne6xvf_hw *hw)
{
	return 1;
}

/**
 *  ne6xvf_register_netdev - register netdev
 *  @adapter: pointer to the ne6xvf_adapter struct
 *
 *  Returns 0 if register netdev success
 **/
int ne6xvf_register_netdev(struct ne6xvf_adapter *adapter)
{
	char newname[IFNAMSIZ] = {0};
	int ret;
	u16  domain_num;

	domain_num = pci_domain_nr(adapter->pdev->bus);

	/* There are some pcie device with the same bus number but with different
	 * pcie domain, the name of netdev should contain pcie domain number
	 */
	if (domain_num)
		sprintf(newname, "enP%dp%ds0f%dv%d", domain_num, adapter->hw.bus.bus_id,
			adapter->hw.dev_caps.lport,
			adapter->hw.dev_caps.vf_id % adapter->hw.dev_caps.num_vf_per_pf);
	else
		sprintf(newname, "enp%ds0f%dv%d", adapter->hw.bus.bus_id,
			adapter->hw.dev_caps.lport,
			adapter->hw.dev_caps.vf_id % adapter->hw.dev_caps.num_vf_per_pf);

	strcpy(&adapter->netdev->name[0], newname);
	dev_info(&adapter->pdev->dev, "name: %s\n", newname);
	ret = register_netdev(adapter->netdev);
	if (ret) {
		sprintf(newname, "enp%ds0f%dv%%d", adapter->hw.bus.bus_id,
			adapter->hw.dev_caps.lport);
		strcpy(&adapter->netdev->name[0], newname);
		ret = register_netdev(adapter->netdev);
	}
	return ret;
}

static void ne6xvf_watchdog_task(struct work_struct *work)
{
	struct ne6xvf_adapter *adapter = container_of(work, struct ne6xvf_adapter,
						      watchdog_task.work);
	struct ne6xvf_hw *hw = &adapter->hw;

	if (ne6xvf_is_remove_in_progress(adapter))
		return;

	if (test_and_set_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section))
		goto restart_watchdog;

	if (adapter->flags & NE6XVF_FLAG_PF_COMMS_FAILED)
		ne6xvf_change_state(adapter, __NE6XVF_COMM_FAILED);

	if (adapter->flags & NE6XVF_FLAG_RESET_NEEDED && adapter->state != __NE6XVF_RESETTING) {
		adapter->flags &= ~NE6XVF_FLAG_RESET_NEEDED;
		ne6xvf_change_state(adapter, __NE6XVF_RESETTING);
		adapter->aq_required = 0;
		adapter->current_op = VIRTCHNL_OP_UNKNOWN;
	}
	switch (adapter->state) {
	case __NE6XVF_INIT_FAILED:
		/* Try again from failed step */
		ne6xvf_change_state(adapter, adapter->last_state);
		clear_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section);
		queue_delayed_work(ne6xvf_wq, &adapter->watchdog_task, HZ);
		return;
	case __NE6XVF_COMM_FAILED:
		adapter->aq_required = 0;
		adapter->current_op = VIRTCHNL_OP_UNKNOWN;
		clear_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section);
		queue_delayed_work(ne6xvf_wq, &adapter->watchdog_task, msecs_to_jiffies(10));
		return;
	case __NE6XVF_RESETTING:
		ne6xvf_handle_reset(adapter);
		clear_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section);
		queue_work(ne6xvf_wq, &adapter->watchdog_task.work);
		return;
	case __NE6XVF_DOWN:
	case __NE6XVF_DOWN_PENDING:
	case __NE6XVF_TESTING:
	case __NE6XVF_RUNNING:
		if (adapter->current_op) {
			if (!ne6xvf_asq_done(hw)) {
				dev_dbg(&adapter->pdev->dev, "Admin queue timeout\n");
				ne6xvf_send_api_ver(adapter);
			}
		} else {
			int ret = ne6xvf_process_aq_command(adapter);

			/* An error will be returned if no commands were
			 * processed; use this opportunity to update stats
			 * if the error isn't -ENOTSUPP
			 */
			if (ret && ret != -EOPNOTSUPP && adapter->state == __NE6XVF_RUNNING)
				ne6xvf_request_stats(adapter);
		}
		break;
	case __NE6XVF_REMOVE:
		clear_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section);
		return;
	default:
		break;
	}
	clear_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section);

restart_watchdog:
	queue_work(ne6xvf_wq, &adapter->sdk_task);
	if (adapter->aq_required)
		queue_delayed_work(ne6xvf_wq, &adapter->watchdog_task, msecs_to_jiffies(20));
	else
		queue_delayed_work(ne6xvf_wq, &adapter->watchdog_task, msecs_to_jiffies(1000));
}

inline void ne6xvf_init_spinlock_d(struct ne6xvf_spinlock *sp)
{
	mutex_init((struct mutex *)sp);
}

void ne6xvf_acquire_spinlock_d(struct ne6xvf_spinlock *sp)
{
	mutex_lock((struct mutex *)sp);
}

void ne6xvf_release_spinlock_d(struct ne6xvf_spinlock *sp)
{
	mutex_unlock((struct mutex *)sp);
}

void ne6xvf_destroy_spinlock_d(struct ne6xvf_spinlock *sp)
{
	mutex_destroy((struct mutex *)sp);
}

/**
 * ne6xvf_find_filter - Search filter list for specific mac filter
 * @adapter: board private structure
 * @macaddr: the MAC address
 *
 * Returns ptr to the filter object or NULL. Must be called while holding the
 * mac_vlan_list_lock.
 **/
static struct ne6xvf_mac_filter *ne6xvf_find_filter(struct ne6xvf_adapter *adapter,
						    const u8 *macaddr)
{
	struct ne6xvf_mac_filter *f;

	if (!macaddr)
		return NULL;

	list_for_each_entry(f, &adapter->mac_filter_list, list) {
		if (ether_addr_equal(macaddr, f->macaddr))
			return f;
	}

	return NULL;
}

/**
 * ne6xvf_add_filter - Add a mac filter to the filter list
 * @adapter: board private structure
 * @macaddr: the MAC address
 *
 * Returns ptr to the filter object or NULL when no memory available.
 **/
static struct ne6xvf_mac_filter *ne6xvf_add_filter(struct ne6xvf_adapter *adapter,
						   const u8 *macaddr)
{
	struct ne6xvf_mac_filter *f;

	if (!macaddr)
		return NULL;

	f = ne6xvf_find_filter(adapter, macaddr);
	if (!f) {
		f = kzalloc(sizeof(*f), GFP_ATOMIC);
		if (!f)
			return f;

		ether_addr_copy(f->macaddr, macaddr);

		list_add_tail(&f->list, &adapter->mac_filter_list);
		f->add = true;
		f->add_handled = false;
		f->is_new_mac = true;
		f->is_primary = false;
		adapter->aq_required |= NE6XVF_FLAG_AQ_ADD_MAC_FILTER;
	} else {
		f->remove = false;
	}

	return f;
}

/**
 * ne6xvf_down - Shutdown the connection processing
 * @adapter: board private structure
 *
 * Expects to be called while holding the __NE6XVF_IN_CRITICAL_TASK bit lock.
 **/
void ne6xvf_down(struct ne6xvf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct ne6xvf_vlan_filter *vlf;
	struct ne6xvf_mac_filter *f;

	if (adapter->state <= __NE6XVF_DOWN_PENDING)
		return;

	netif_carrier_off(netdev);
	netif_tx_disable(netdev);
	adapter->link_up = false;
	set_bit(NE6X_ADPT_DOWN, adapter->comm.state);
	ne6xvf_irq_disable(adapter);
	ne6xvf_napi_disable_all(adapter);

	spin_lock_bh(&adapter->mac_vlan_list_lock);

	/* clear the sync flag on all filters */
	__dev_uc_unsync(adapter->netdev, NULL);
	__dev_mc_unsync(adapter->netdev, NULL);

	/* remove all MAC filters */
	list_for_each_entry(f, &adapter->mac_filter_list, list)
		f->remove = true;

	/* remove all VLAN filters */
	list_for_each_entry(vlf, &adapter->vlan_filter_list, list)
		vlf->remove = true;

	spin_unlock_bh(&adapter->mac_vlan_list_lock);

	if (!(adapter->flags & NE6XVF_FLAG_PF_COMMS_FAILED) &&
	    adapter->state != __NE6XVF_RESETTING) {
		dev_info(&adapter->pdev->dev, "%s: state->%s\n", __func__,
			 ne6xvf_state_str(adapter->state));
		/* cancel any current operation */
		adapter->current_op = VIRTCHNL_OP_UNKNOWN;

		/* Schedule operations to close down the HW. Don't wait
		 * here for this to complete. The watchdog is still running
		 * and it will take care of this.
		 */
		adapter->aq_required |= NE6XVF_FLAG_AQ_DEL_MAC_FILTER;

		/* In case the queue configure or enable operations are still
		 * pending from when the interface was opened, make sure
		 * they're canceled here.
		 */
		adapter->aq_required &= ~NE6XVF_FLAG_AQ_ENABLE_QUEUES;
		adapter->aq_required &= ~NE6XVF_FLAG_AQ_CONFIGURE_QUEUES;
	}

	mod_delayed_work(ne6xvf_wq, &adapter->watchdog_task, 0);
}

static void ne6xvf_get_port_link_status(struct ne6xvf_adapter *adapter)
{
	adapter->aq_required |= NE6XVF_FLAG_AQ_GET_PORT_LINK_STATUS;
	mod_delayed_work(ne6xvf_wq, &adapter->watchdog_task, 0);
}

static void ne6xvf_set_vport_state(struct ne6xvf_adapter *adapter, int tx_state, int rx_state)
{
	if (rx_state)
		adapter->hw_feature &= ~NE6X_F_RX_DISABLE;
	else
		adapter->hw_feature |= NE6X_F_RX_DISABLE;

	if (tx_state)
		adapter->hw_feature &= ~NE6X_F_TX_DISABLE;
	else
		adapter->hw_feature |= NE6X_F_TX_DISABLE;

	adapter->aq_required |= NE6XVF_FLAG_AQ_CONFIGURE_HW_OFFLOAD;
	mod_delayed_work(ne6xvf_wq, &adapter->watchdog_task, 0);
}

/**
 * ne6xvf_open - Called when a network interface is made active
 * @netdev: network interface device structure
 *
 * Returns 0 on success, negative value on failure
 *
 * The open entry point is called when a network interface is made
 * active by the system (IFF_UP).  At this point all resources needed
 * for transmit and receive operations are allocated, the interrupt
 * handler is registered with the OS, the watchdog is started,
 * and the stack is notified that the interface is ready.
 **/
int ne6xvf_open(struct net_device *netdev)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	int err;

	netdev_info(netdev, "open !!!\n");

	while (test_and_set_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section))
		usleep_range(500, 1000);

	if (adapter->flags & NE6XVF_FLAG_PF_COMMS_FAILED) {
		dev_err(&adapter->pdev->dev, "Unable to open device due to PF driver failure.\n");
		err = -EIO;
		goto unlock;
	}

	if (adapter->state == __NE6XVF_RUNNING && !test_bit(NE6X_ADPT_DOWN, adapter->comm.state)) {
		dev_dbg(&adapter->pdev->dev, "VF is already open.\n");
		err = 0;
		goto unlock;
	}

	if (adapter->state != __NE6XVF_DOWN) {
		err = -EBUSY;
		goto unlock;
	}
	err = ne6xvf_setup_all_tg_resources(adapter);
	if (err)
		goto err_setup_tg;

	err = ne6xvf_setup_all_cq_resources(adapter);
	if (err)
		goto err_setup_cq;

	/* allocate transmit descriptors */
	err = ne6xvf_setup_all_tx_resources(adapter);
	if (err)
		goto err_setup_tx;

	/* allocate receive descriptors */
	err = ne6xvf_setup_all_rx_resources(adapter);
	if (err)
		goto err_setup_rx;

	/* clear any pending interrupts, may auto mask */
	err = ne6xvf_request_traffic_irqs(adapter, netdev->name);
	if (err)
		goto err_req_irq;

	ne6xvf_configure(adapter);

	ne6xvf_up_complete(adapter);

	ne6xvf_irq_enable(adapter, true);

	ne6xvf_get_port_link_status(adapter);

	ne6xvf_set_vport_state(adapter, true, true);
	clear_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section);

	return 0;

err_req_irq:
	ne6xvf_down(adapter);
	ne6xvf_free_traffic_irqs(adapter);
err_setup_rx:
	ne6xvf_free_all_rx_resources(adapter);
err_setup_tx:
	ne6xvf_free_all_tx_resources(adapter);
err_setup_cq:
	ne6xvf_free_all_cq_resources(adapter);
err_setup_tg:
	ne6xvf_free_all_tg_resources(adapter);

unlock:
	clear_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section);

	return err;
}

/**
 * ne6xvf_close - Disables a network interface
 * @netdev: network interface device structure
 *
 * Returns 0, this is not allowed to fail
 *
 * The close entry point is called when an interface is de-activated
 * by the OS.  The hardware is still under the drivers control, but
 * needs to be disabled. All IRQs except vector 0 (reserved for admin queue)
 * are freed, along with all transmit and receive resources.
 **/
int ne6xvf_close(struct net_device *netdev)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	struct ne6xvf_hw *hw = &adapter->hw;
	int status;
	int i;

	netdev_info(netdev, "close !!!\n");

	while (test_and_set_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section))
		usleep_range(500, 1000);

	if (adapter->state <= __NE6XVF_DOWN_PENDING) {
		clear_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section);
		return 0;
	}

	ne6xvf_set_vport_state(adapter, false, false);
	ne6xvf_down(adapter);

	for (i = 0; i < adapter->num_msix_vectors; i++)
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_VP_RELOAD), 0x1);

	usleep_range(100, 120);

	ne6xvf_change_state(adapter, __NE6XVF_DOWN_PENDING);
	ne6xvf_free_traffic_irqs(adapter);

	ne6xvf_free_all_tg_resources(adapter);
	ne6xvf_free_all_cq_resources(adapter);
	ne6xvf_free_all_tx_resources(adapter);
	ne6xvf_free_all_rx_resources(adapter);
	if (adapter->state == __NE6XVF_DOWN_PENDING)
		ne6xvf_change_state(adapter, __NE6XVF_DOWN);

	clear_bit(__NE6XVF_IN_CRITICAL_TASK, &adapter->crit_section);

	/* If we're closing the interface as part of driver removal then don't
	 * wait. The VF resources will be reinitialized when the hardware is
	 * reset.
	 */
	if (ne6xvf_is_remove_in_progress(adapter))
		return 0;

	/* We explicitly don't free resources here because the hardware is
	 * still active and can DMA into memory. Resources are cleared in
	 * ne6xvf_virtchnl_completion() after we get confirmation from the PF
	 * driver that the rings have been stopped.
	 *
	 * Also, we wait for state to transition to __NE6XVF_DOWN before
	 * returning. State change occurs in ne6xvf_virtchnl_completion() after
	 * VF resources are released (which occurs after PF driver processes and
	 * responds to admin queue commands).
	 */
	status = wait_event_timeout(adapter->down_waitqueue, adapter->state == __NE6XVF_DOWN,
				    msecs_to_jiffies(500));
	if (!status)
		netdev_dbg(netdev, "Device resources not yet released\n");

	return 0;
}

/**
 * ne6xvf_addr_sync - Callback for dev_(mc|uc)_sync to add address
 * @netdev: the netdevice
 * @addr: address to add
 *
 * Called by __dev_(mc|uc)_sync when an address needs to be added. We call
 * __dev_(uc|mc)_sync from .set_rx_mode and guarantee to hold the hash lock.
 */
static int ne6xvf_addr_sync(struct net_device *netdev, const u8 *addr)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	if (ether_addr_equal(addr, netdev->dev_addr))
		return 0;

	if (ne6xvf_add_filter(adapter, addr))
		return 0;
	else
		return -ENOMEM;
}

/**
 * ne6xvf_addr_unsync - Callback for dev_(mc|uc)_sync to remove address
 * @netdev: the netdevice
 * @addr: address to add
 *
 * Called by __dev_(mc|uc)_sync when an address needs to be removed. We call
 * __dev_(uc|mc)_sync from .set_rx_mode and guarantee to hold the hash lock.
 */
static int ne6xvf_addr_unsync(struct net_device *netdev, const u8 *addr)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	struct ne6xvf_mac_filter *f;

	/* Under some circumstances, we might receive a request to delete
	 * our own device address from our uc list. Because we store the
	 * device address in the VSI's MAC/VLAN filter list, we need to ignore
	 * such requests and not delete our device address from this list.
	 */
	if (ether_addr_equal(addr, netdev->dev_addr))
		return 0;

	f = ne6xvf_find_filter(adapter, addr);
	if (f) {
		f->remove = true;
		adapter->aq_required |= NE6XVF_FLAG_AQ_DEL_MAC_FILTER;
	}

	return 0;
}

/**
 * ne6xvf_promiscuous_mode_changed - check if promiscuous mode bits changed
 * @adapter: device specific adapter
 */
bool ne6xvf_promiscuous_mode_changed(struct ne6xvf_adapter *adapter)
{
	return (adapter->current_netdev_promisc_flags ^ adapter->netdev->flags) &
	       (IFF_PROMISC | IFF_ALLMULTI);
}

/**
 * ne6xvf_set_rx_mode - NDO callback to set the netdev filters
 * @netdev: network interface device structure
 **/
static void ne6xvf_set_rx_mode(struct net_device *netdev)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	spin_lock_bh(&adapter->mac_vlan_list_lock);

	__dev_uc_sync(netdev, ne6xvf_addr_sync, ne6xvf_addr_unsync);
	__dev_mc_sync(netdev, ne6xvf_addr_sync, ne6xvf_addr_unsync);

	spin_unlock_bh(&adapter->mac_vlan_list_lock);

	if (!adapter->trusted) {
		adapter->hw_feature &= ~NE6X_F_PROMISC;
		adapter->hw_feature &= ~NE6X_F_RX_ALLMULTI;
		adapter->flags &= ~NE6XVF_FLAG_PROMISC_ON;
		adapter->flags &= ~NE6XVF_FLAG_ALLMULTI_ON;
		return;
	}

	if (netdev->flags & IFF_PROMISC) {
		adapter->flags |= NE6XVF_FLAG_PROMISC_ON;
		adapter->flags |= NE6XVF_FLAG_ALLMULTI_ON;
	} else if (netdev->flags & IFF_ALLMULTI) {
		adapter->flags &= ~NE6XVF_FLAG_PROMISC_ON;
		adapter->flags |= NE6XVF_FLAG_ALLMULTI_ON;
	} else {
		adapter->flags &= ~NE6XVF_FLAG_PROMISC_ON;
		adapter->flags &= ~NE6XVF_FLAG_ALLMULTI_ON;
	}

	adapter->aq_required |= NE6XVF_FLAG_AQ_REQUEST_PROMISC;
}

/**
 * ne6xvf_get_stats - Get System Network Statistics
 * @netdev: network interface device structure
 *
 * Returns the address of the device statistics structure.
 * The statistics are actually updated from the watchdog task.
 **/
static struct net_device_stats *ne6xvf_get_stats(struct net_device *netdev)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	if (adapter->netdev)
		return &adapter->netdev->stats;
	else
		return &adapter->net_stats;
}

static void ne6xvf_sync_features(struct net_device *netdev)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	if (netdev->features & NETIF_F_GSO_UDP_TUNNEL_CSUM)
		adapter->hw_feature |= NE6X_F_TX_UDP_TNL_SEG;
	else
		adapter->hw_feature &= ~NE6X_F_TX_UDP_TNL_SEG;

	if (netdev->features & NETIF_F_HW_VLAN_CTAG_RX)
		adapter->hw_feature |= NE6X_F_RX_VLAN_STRIP;
	else
		adapter->hw_feature &= ~NE6X_F_RX_VLAN_STRIP;

	if (netdev->features & NETIF_F_HW_VLAN_CTAG_TX)
		adapter->hw_feature |= NE6X_F_TX_VLAN;
	else
		adapter->hw_feature &= ~NE6X_F_TX_VLAN;

	if (netdev->features & NETIF_F_HW_VLAN_STAG_RX)
		adapter->hw_feature |= NE6X_F_RX_QINQ_STRIP;
	else
		adapter->hw_feature &= ~NE6X_F_RX_QINQ_STRIP;

	if (netdev->features & NETIF_F_HW_VLAN_STAG_TX)
		adapter->hw_feature |= NE6X_F_TX_QINQ;
	else
		adapter->hw_feature &= ~NE6X_F_TX_QINQ;

	if (netdev->features & (NETIF_F_HW_VLAN_CTAG_FILTER | NETIF_F_HW_VLAN_STAG_FILTER))
		adapter->hw_feature |= NE6X_F_RX_VLAN_FILTER;
	else
		adapter->hw_feature &= ~NE6X_F_RX_VLAN_FILTER;

	if (netdev->features & NETIF_F_RXCSUM)
		adapter->hw_feature |= NE6X_OFFLOAD_RXCSUM;

	if (netdev->features & NETIF_F_LRO)
		adapter->hw_feature |= NE6X_OFFLOAD_LRO;

	if (netdev->features & (NETIF_F_TSO | NETIF_F_TSO6))
		adapter->hw_feature |= NE6X_OFFLOAD_TSO;

	if (netdev->features & NETIF_F_IP_CSUM)
		adapter->hw_feature |= NE6X_OFFLOAD_TXCSUM;

	if (netdev->features & NETIF_F_RXHASH)
		adapter->hw_feature |= NE6X_OFFLOAD_RSS;

	if (netdev->features & NETIF_F_HW_L2FW_DOFFLOAD)
		adapter->hw_feature |= NE6X_OFFLOAD_L2;

	if (netdev->features & NETIF_F_RXHASH)
		adapter->hw_feature |= NE6X_OFFLOAD_RSS;

	if (netdev->features & NETIF_F_SCTP_CRC)
		adapter->hw_feature |= NE6X_OFFLOAD_SCTP_CSUM;
	else
		adapter->hw_feature &= ~NE6X_OFFLOAD_SCTP_CSUM;

	dev_info(&adapter->pdev->dev, "%s: adapter->hw_feature = 0x%08x\n", __func__,
		 adapter->hw_feature);

	adapter->aq_required |= NE6XVF_FLAG_AQ_CONFIGURE_HW_OFFLOAD;
}

#define NETIF_VLAN_OFFLOAD_FEATURES (NETIF_F_HW_VLAN_CTAG_RX | \
				     NETIF_F_HW_VLAN_CTAG_TX | \
				     NETIF_F_HW_VLAN_STAG_RX | \
				     NETIF_F_HW_VLAN_STAG_TX)

#define NETIF_VLAN_FILTERING_FEATURES (NETIF_F_HW_VLAN_CTAG_FILTER | NETIF_F_HW_VLAN_STAG_FILTER)

#define NETIF_UDP_TNL_FEATURES (NETIF_F_GSO_UDP_TUNNEL | NETIF_F_GSO_UDP_TUNNEL_CSUM)

/**
 * nce_set_features - set the netdev feature flags
 * @netdev: ptr to the netdev being adjusted
 * @features: the feature set that the stack is suggesting
 * Note: expects to be called while under rtnl_lock()
 **/
static int ne6xvf_set_features(struct net_device *netdev, netdev_features_t features)
{
	netdev_features_t changed = features ^ netdev->features;
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	if (changed & (NETIF_F_GSO_UDP_TUNNEL | NETIF_F_GSO_UDP_TUNNEL_CSUM)) {
		if (features & NETIF_F_GSO_UDP_TUNNEL_CSUM)
			adapter->hw_feature |= NE6X_F_TX_UDP_TNL_SEG;
		else
			adapter->hw_feature &= ~NE6X_F_TX_UDP_TNL_SEG;
	}

	if (changed & NETIF_VLAN_OFFLOAD_FEATURES || changed & NETIF_VLAN_FILTERING_FEATURES) {
		/* keep cases separate because one ethertype for offloads can be
		 * disabled at the same time as another is disabled, so check for an
		 * enabled ethertype first, then check for disabled. Default to
		 * ETH_P_8021Q so an ethertype is specified if disabling insertion and
		 * stripping.
		 */
		if (features & NETIF_F_HW_VLAN_CTAG_RX)
			adapter->hw_feature |= NE6X_F_RX_VLAN_STRIP;
		else
			adapter->hw_feature &= ~NE6X_F_RX_VLAN_STRIP;

		if (features & NETIF_F_HW_VLAN_CTAG_TX)
			adapter->hw_feature |= NE6X_F_TX_VLAN;
		else
			adapter->hw_feature &= ~NE6X_F_TX_VLAN;

		if (features & NETIF_F_HW_VLAN_STAG_RX)
			adapter->hw_feature |= NE6X_F_RX_QINQ_STRIP;
		else
			adapter->hw_feature &= ~NE6X_F_RX_QINQ_STRIP;

		if (features & NETIF_F_HW_VLAN_STAG_TX)
			adapter->hw_feature |= NE6X_F_TX_QINQ;
		else
			adapter->hw_feature &= ~NE6X_F_TX_QINQ;

		if (features & (NETIF_F_HW_VLAN_CTAG_FILTER | NETIF_F_HW_VLAN_STAG_FILTER))
			adapter->hw_feature |= NE6X_F_RX_VLAN_FILTER;
		else
			adapter->hw_feature &= ~NE6X_F_RX_VLAN_FILTER;
	}

	if (changed & (NETIF_F_RXCSUM | NETIF_F_LRO)) {
		if (features & NETIF_F_RXCSUM)
			adapter->hw_feature |= NE6X_OFFLOAD_RXCSUM;
		else
			adapter->hw_feature &= ~NE6X_OFFLOAD_RXCSUM;

		/* update hardware LRO capability accordingly */
		if (features & NETIF_F_LRO)
			adapter->hw_feature |= NE6X_OFFLOAD_LRO;
		else
			adapter->hw_feature &= ~NE6X_OFFLOAD_LRO;
	}

	if (changed & (NETIF_F_TSO6 | NETIF_F_TSO)) {
		if (features & (NETIF_F_TSO | NETIF_F_TSO6))
			adapter->hw_feature |= NE6X_OFFLOAD_TSO;
		else
			adapter->hw_feature &= ~NE6X_OFFLOAD_TSO;
	}

	if (changed & NETIF_F_GSO_UDP) {
		if (features & NETIF_F_GSO_UDP)
			adapter->hw_feature |= NE6X_OFFLOAD_UFO;
		else
			adapter->hw_feature &= ~NE6X_OFFLOAD_UFO;
	}

	if (changed & NETIF_F_IP_CSUM) {
		if (features & NETIF_F_IP_CSUM)
			adapter->hw_feature |= NE6X_OFFLOAD_TXCSUM;
		else
			adapter->hw_feature &= ~NE6X_OFFLOAD_TXCSUM;
	}

	if (changed & NETIF_F_RXHASH) {
		if (features & NETIF_F_RXHASH)
			adapter->hw_feature |= NE6X_OFFLOAD_RSS;
		else
			adapter->hw_feature &= ~NE6X_OFFLOAD_RSS;
	}

	if (changed & NETIF_F_HW_L2FW_DOFFLOAD) {
		if (features & NETIF_F_HW_L2FW_DOFFLOAD)
			adapter->hw_feature |= NE6X_OFFLOAD_L2;
		else
			adapter->hw_feature &= ~NE6X_OFFLOAD_L2;
	}

	if (changed & NETIF_F_SCTP_CRC) {
		if (features & NETIF_F_SCTP_CRC)
			adapter->hw_feature |= NE6X_OFFLOAD_SCTP_CSUM;
		else
			adapter->hw_feature &= ~NE6X_OFFLOAD_SCTP_CSUM;
	}

	dev_info(&adapter->pdev->dev, "%s: adapter->hw_feature = 0x%08x\n", __func__,
		 adapter->hw_feature);

	adapter->aq_required |= NE6XVF_FLAG_AQ_CONFIGURE_HW_OFFLOAD;
	mod_delayed_work(ne6xvf_wq, &adapter->watchdog_task, 0);

	return 0;
}

/**
 * nce_fix_features - fix the netdev feature flags
 * @netdev: ptr to the netdev being adjusted
 * @features: the feature set that the stack is suggesting
 * Note: expects to be called while under rtnl_lock()
 **/
static netdev_features_t ne6xvf_fix_features(struct net_device *netdev, netdev_features_t features)
{
	if (features & NETIF_F_HW_VLAN_CTAG_RX)
		features &= ~NETIF_F_HW_VLAN_STAG_RX;

	if (features & NETIF_F_HW_VLAN_STAG_RX)
		features &= ~NETIF_F_HW_VLAN_CTAG_RX;

	if (features & NETIF_F_HW_VLAN_CTAG_TX)
		features &= ~NETIF_F_HW_VLAN_STAG_TX;

	if (features & NETIF_F_HW_VLAN_STAG_TX)
		features &= ~NETIF_F_HW_VLAN_CTAG_TX;

	if (features & NETIF_VLAN_FILTERING_FEATURES)
		features |= NETIF_VLAN_FILTERING_FEATURES;

	return features;
}

/**
 * ne6xvf_replace_primary_mac - Replace current primary address
 * @adapter: board private structure
 * @new_mac: new MAC address to be applied
 *
 * Replace current dev_addr and send request to PF for removal of previous
 * primary MAC address filter and addition of new primary MAC filter.
 * Return 0 for success, -ENOMEM for failure.
 *
 * Do not call this with mac_vlan_list_lock!
 **/
int ne6xvf_replace_primary_mac(struct ne6xvf_adapter *adapter, const u8 *new_mac)
{
	memcpy(adapter->hw.mac.addr, new_mac, 6);
	adapter->aq_required |= NE6XVF_FLAG_AQ_SET_VF_MAC;

	/* schedule the watchdog task to immediately process the request */
	queue_work(ne6xvf_wq, &adapter->watchdog_task.work);
	return 0;
}

/**
 * ne6xvf_set_mac - NDO callback to set port mac address
 * @netdev: network interface device structure
 * @p: pointer to an address structure
 *
 * Returns 0 on success, negative on failure
 **/
static int ne6xvf_set_mac(struct net_device *netdev, void *p)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	struct sockaddr *addr = p;
	int ret;

	netdev_info(netdev, "set mac address %pM\n", addr->sa_data);
	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	if (is_multicast_ether_addr(addr->sa_data)) {
		netdev_err(netdev, "Invalid Ethernet address %pM\n", addr->sa_data);
		return -EINVAL;
	}

	if (ether_addr_equal(netdev->dev_addr, addr->sa_data)) {
		netdev_info(netdev, "already using mac address %pM\n", addr->sa_data);
		return 0;
	}

	ret = ne6xvf_replace_primary_mac(adapter, addr->sa_data);

	if (ret)
		return ret;

	ret = wait_event_interruptible_timeout(adapter->vc_waitqueue,
					       ether_addr_equal(netdev->dev_addr, addr->sa_data),
					       msecs_to_jiffies(2500));

	/* If ret < 0 then it means wait was interrupted.
	 * If ret == 0 then it means we got a timeout.
	 * else it means we got response for set MAC from PF,
	 * check if netdev MAC was updated to requested MAC,
	 * if yes then set MAC succeeded otherwise it failed return -EACCES
	 */
	netdev_info(netdev, "%s,%pM %pM\n", __func__, addr->sa_data, netdev->dev_addr);
	if (!ether_addr_equal(netdev->dev_addr, addr->sa_data))
		return -EACCES;

	return 0;
}

/**
 * ne6xvf_do_ioctl - Handle network device specific ioctls
 * @netdev: network interface device structure
 * @ifr: interface request data
 * @cmd: ioctl command
 *
 * Callback to handle the networking device specific ioctls. Used to handle
 * the SIOCGHWTSTAMP and SIOCSHWTSTAMP ioctl requests that configure Tx and Rx
 * timstamping support.
 */
static int ne6xvf_do_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	return 0;
}

/**
 * ne6xvf_change_mtu - Change the Maximum Transfer Unit
 * @netdev: network interface device structure
 * @new_mtu: new value for maximum frame size
 *
 * Returns 0 on success, negative on failure
 **/
static int ne6xvf_change_mtu(struct net_device *netdev, int new_mtu)
{
	int max_frame = new_mtu;

	if (new_mtu < NE6X_MIN_MTU_SIZE) {
		netdev_err(netdev, "mtu < MIN MTU size");
		return -EINVAL;
	}

	max_frame = new_mtu + ETH_HLEN + ETH_FCS_LEN;
	if (max_frame > NE6X_MAX_RXBUFFER) {
		netdev_err(netdev, "mtu > MAX MTU size");
		return -EINVAL;
	}

	netdev_info(netdev, "changing MTU from %d to %d\n", netdev->mtu, new_mtu);
	netdev->mtu = new_mtu;

	return 0;
}

/**
 * ne6xvf_find_vlan - Search filter list for specific vlan filter
 * @vsi: board private structure
 * @vlan: vlan tag
 *
 * Returns ptr to the filter object or NULL. Must be called while holding the
 * mac_vlan_list_lock.
 **/
static struct ne6xvf_vlan_filter *ne6xvf_find_vlan(struct ne6xvf_adapter *adapter,
						   struct ne6x_vf_vlan vlan)
{
	struct ne6xvf_vlan_filter *f;

	list_for_each_entry(f, &adapter->vlan_filter_list, list) {
		if (f->vlan.vid == vlan.vid && f->vlan.tpid == vlan.tpid)
			return f;
	}

	return NULL;
}

/**
 * ne6xvf_add_vlan - Add a vlan filter to the list
 * @adapter: board private structure
 * @vlan: VLAN tag
 *
 * Returns ptr to the filter object or NULL when no memory available.
 **/
struct ne6xvf_vlan_filter *ne6xvf_add_vlan_list(struct ne6xvf_adapter *adapter,
						struct ne6x_vf_vlan vlan)
{
	struct ne6xvf_vlan_filter *f = NULL;

	spin_lock_bh(&adapter->mac_vlan_list_lock);

	f = ne6xvf_find_vlan(adapter, vlan);
	if (!f) {
		f = kzalloc(sizeof(*f), GFP_ATOMIC);
		if (!f)
			goto clearout;

		f->vlan = vlan;

		list_add_tail(&f->list, &adapter->vlan_filter_list);
		f->add = true;
	}

clearout:
	spin_unlock_bh(&adapter->mac_vlan_list_lock);
	return f;
}

/**
 * ne6xvf_del_vlan - Remove a vlan filter from the list
 * @adapter: board private structure
 * @vlan: VLAN tag
 **/
void ne6xvf_del_vlan_list(struct ne6xvf_adapter *adapter, struct ne6x_vf_vlan vlan)
{
	struct ne6xvf_vlan_filter *f;

	spin_lock_bh(&adapter->mac_vlan_list_lock);

	f = ne6xvf_find_vlan(adapter, vlan);
	if (f) {
		list_del(&f->list);
		kfree(f);
	}

	spin_unlock_bh(&adapter->mac_vlan_list_lock);
}

/**
 * ne6xvf_add_vlan - Add a vlan filter to the list
 * @adapter: board private structure
 * @vlan: VLAN tag
 *
 * Returns ptr to the filter object or NULL when no memory available.
 **/
static struct ne6xvf_vlan_filter *ne6xvf_add_vlan(struct ne6xvf_adapter *adapter,
						  struct ne6x_vf_vlan vlan)
{
	struct ne6xvf_vlan_filter *f = NULL;

	spin_lock_bh(&adapter->mac_vlan_list_lock);

	f = ne6xvf_find_vlan(adapter, vlan);
	if (!f) {
		f = kzalloc(sizeof(*f), GFP_ATOMIC);
		if (!f)
			goto clearout;

		f->vlan = vlan;

		list_add_tail(&f->list, &adapter->vlan_filter_list);
		f->add = true;
		adapter->aq_required |= NE6XVF_FLAG_AQ_ADD_VLAN_FILTER;
	}

clearout:
	spin_unlock_bh(&adapter->mac_vlan_list_lock);
	return f;
}

/**
 * ne6xvf_del_vlan - Remove a vlan filter from the list
 * @adapter: board private structure
 * @vlan: VLAN tag
 **/
static void ne6xvf_del_vlan(struct ne6xvf_adapter *adapter, struct ne6x_vf_vlan vlan)
{
	struct ne6xvf_vlan_filter *f;

	spin_lock_bh(&adapter->mac_vlan_list_lock);

	f = ne6xvf_find_vlan(adapter, vlan);
	if (f) {
		f->remove = true;
		adapter->aq_required |= NE6XVF_FLAG_AQ_DEL_VLAN_FILTER;
	}

	spin_unlock_bh(&adapter->mac_vlan_list_lock);
}

static int ne6xvf_vlan_rx_add_vid(struct net_device *netdev, __always_unused __be16 proto, u16 vid)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	struct ne6x_vf_vlan vlan;

	netdev_info(netdev, "%s:%d:   proto:%04x vid:%d\n", __func__, __LINE__,
		    be16_to_cpu(proto), vid);
	vlan = NE6X_VF_VLAN(vid, be16_to_cpu(proto));

	if (!vid)
		return 0;

	if (!ne6xvf_add_vlan(adapter, vlan))
		return -ENOMEM;

	mod_delayed_work(ne6xvf_wq, &adapter->watchdog_task, 0);

	return 0;
}

static int ne6xvf_vlan_rx_kill_vid(struct net_device *netdev, __always_unused __be16 proto, u16 vid)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	struct ne6x_vf_vlan vlan;

	netdev_info(netdev, "%s:%d:   proto:%04x vid:%d\n", __func__, __LINE__,
		    be16_to_cpu(proto), vid);
	vlan = NE6X_VF_VLAN(vid, be16_to_cpu(proto));

	ne6xvf_del_vlan(adapter, vlan);
	mod_delayed_work(ne6xvf_wq, &adapter->watchdog_task, 0);

	return 0;
}

/**
 *__ne6xvf_setup_tc - configure multiple traffic classes
 * @netdev: network interface device structure
 * @type_data: tc offload data
 *
 * This function processes the config information provided by the
 * user to configure traffic classes/queue channels and packages the
 * information to request the PF to setup traffic classes.
 *
 * Returns 0 on success.
 **/
static int __ne6xvf_setup_tc(struct net_device *netdev, void *type_data)
{
	return 0;
}

/**
 * ne6xvf_setup_tc - configure multiple traffic classes
 * @dev: network interface device structure
 * @type: type of offload
 * @type_data: tc offload data
 *
 * This function is the callback to ndo_setup_tc in the
 * netdev_ops.
 *
 * Returns 0 on success
 **/
static int ne6xvf_setup_tc(struct net_device *dev, enum tc_setup_type type, void *type_data)
{
	return __ne6xvf_setup_tc(dev, type_data);
}

/**
 * ne6xvf_features_check - Validate encapsulated packet conforms to limits
 * @skb: skb buff
 * @dev: This physical port's netdev
 * @features: Offload features that the stack believes apply
 **/
static netdev_features_t ne6xvf_features_check(struct sk_buff *skb,
					       struct net_device *dev,
					       netdev_features_t features)
{
	size_t len;

	/* No point in doing any of this if neither checksum nor GSO are
	 * being requested for this frame.  We can rule out both by just
	 * checking for CHECKSUM_PARTIAL
	 */
	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return features;

	/* We cannot support GSO if the MSS is going to be less than
	 * 64 bytes.  If it is then we need to drop support for GSO.
	 */
	if (skb_is_gso(skb) && (skb_shinfo(skb)->gso_size < 64))
		features &= ~NETIF_F_GSO_MASK;

	/* MACLEN can support at most 63 words */
	len = skb_network_header(skb) - skb->data;
	if (len & ~(63 * 2))
		goto out_err;

	/* IPLEN and EIPLEN can support at most 127 dwords */
	len = skb_transport_header(skb) - skb_network_header(skb);
	if (len & ~(127 * 4))
		goto out_err;

	/* No need to validate L4LEN as TCP is the only protocol with a
	 * a flexible value and we support all possible values supported
	 * by TCP, which is at most 15 dwords
	 */

	return features;

out_err:
	return features & ~(NETIF_F_CSUM_MASK | NETIF_F_GSO_MASK);
}

/**
 * ne6xvf_fwd_add_macvlan - Configure MACVLAN interface
 * @netdev: Main net device to configure
 * @vdev: MACVLAN subordinate device
 */
static void *ne6xvf_fwd_add_macvlan(struct net_device *netdev, struct net_device *vdev)
{
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);
	struct ne6x_macvlan *mv = NULL;
	u8 mac[ETH_ALEN];

	ether_addr_copy(mac, vdev->dev_addr);
	mv = devm_kzalloc(&adapter->pdev->dev, sizeof(*mv), GFP_KERNEL);
	if (!mv)
		return NULL;

	spin_lock_bh(&adapter->mac_vlan_list_lock);
	ne6xvf_addr_sync(netdev, mac);
	spin_unlock_bh(&adapter->mac_vlan_list_lock);
	INIT_LIST_HEAD(&mv->list);
	mv->vdev = vdev;
	ether_addr_copy(mv->mac, mac);
	list_add(&mv->list, &adapter->macvlan_list);
	netdev_info(netdev, "MACVLAN offloads for %s are on\n", vdev->name);

	return mv;
}

/**
 * ne6xvf_fwd_del_macvlan - Delete MACVLAN interface resources
 * @netdev: Main net device
 * @accel_priv: MACVLAN sub ordinate device
 */
static void ne6xvf_fwd_del_macvlan(struct net_device *netdev, void *accel_priv)
{
	struct ne6x_macvlan *mv = (struct ne6x_macvlan *)accel_priv;
	struct ne6xvf_adapter *adapter = netdev_priv(netdev);

	if (!accel_priv)
		return;

	spin_lock_bh(&adapter->mac_vlan_list_lock);
	ne6xvf_addr_unsync(netdev, mv->mac);
	spin_unlock_bh(&adapter->mac_vlan_list_lock);
	list_del(&mv->list);
	devm_kfree(&adapter->pdev->dev, mv);

	netdev_info(netdev, "MACVLAN offloads for %s are off\n", mv->vdev->name);
}

static const struct net_device_ops ne6xvf_netdev_ops = {
	.ndo_open                      = ne6xvf_open,
	.ndo_stop                      = ne6xvf_close,
	.ndo_start_xmit                = ne6xvf_lan_xmit_frame,
	.ndo_get_stats                 = ne6xvf_get_stats,
	.ndo_set_rx_mode               = ne6xvf_set_rx_mode,
	.ndo_validate_addr             = eth_validate_addr,
	.ndo_set_mac_address           = ne6xvf_set_mac,
	.ndo_do_ioctl                  = ne6xvf_do_ioctl,
	.ndo_change_mtu                = ne6xvf_change_mtu,
	.ndo_tx_timeout                = ne6xvf_tx_timeout,

	.ndo_vlan_rx_add_vid           = ne6xvf_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid          = ne6xvf_vlan_rx_kill_vid,

	.ndo_vlan_rx_add_vid           = ne6xvf_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid          = ne6xvf_vlan_rx_kill_vid,

	.ndo_setup_tc                  = ne6xvf_setup_tc,
	.ndo_features_check            = ne6xvf_features_check,

	.ndo_dfwd_add_station          = ne6xvf_fwd_add_macvlan,
	.ndo_dfwd_del_station          = ne6xvf_fwd_del_macvlan,

	.ndo_fix_features              = ne6xvf_fix_features,
	.ndo_set_features              = ne6xvf_set_features,
};

static int ne6xvf_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct ne6xvf_adapter *adapter = NULL;
	struct ne6xvf_hw *hw = NULL;
	struct net_device *netdev;
	char name[IFNAMSIZ] = {0};
	int err;

	err = pci_enable_device(pdev);
	if (err)
		return err;

	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(48));
	if (err) {
		err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
		if (err) {
			dev_err(&pdev->dev, "DMA configuration failed: 0x%x\n", err);
			goto err_dma;
		}
	}

	err = pci_request_regions(pdev, ne6xvf_driver_name);
	if (err) {
		dev_err(pci_dev_to_dev(pdev), "pci_request_regions failed 0x%x\n", err);
		goto err_pci_reg;
	}

	pci_set_master(pdev);

	sprintf(name, "enp%ds%df%d", pdev->bus->number, PCI_SLOT(pdev->devfn),
		PCI_FUNC(pdev->devfn));

	netdev = alloc_netdev_mq(sizeof(struct ne6xvf_adapter), name, NET_NAME_USER, ether_setup,
				 NE6XVF_MAX_REQ_QUEUES);
	if (!netdev) {
		err = -ENOMEM;
		goto err_alloc_etherdev;
	}

	SET_NETDEV_DEV(netdev, &pdev->dev);

	pci_set_drvdata(pdev, netdev);
	adapter = netdev_priv(netdev);

	adapter->netdev = netdev;
	adapter->pdev = pdev;

	hw = &adapter->hw;
	hw->back = adapter;

	ne6xvf_change_state(adapter, __NE6XVF_STARTUP);

	pci_save_state(pdev);

	hw->hw_addr0 = ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
	hw->hw_addr2 = ioremap(pci_resource_start(pdev, 2), pci_resource_len(pdev, 2));

	if (!hw->hw_addr0 || !hw->hw_addr2) {
		err = -EIO;
		goto err_ioremap;
	}

	hw->vendor_id = pdev->vendor;
	hw->device_id = pdev->device;
	pci_read_config_byte(pdev, PCI_REVISION_ID, &hw->revision_id);
	hw->subsystem_vendor_id = pdev->subsystem_vendor;
	hw->subsystem_device_id = pdev->subsystem_device;
	hw->bus.device = PCI_SLOT(pdev->devfn);
	hw->bus.func = PCI_FUNC(pdev->devfn);
	hw->bus.bus_id = pdev->bus->number;

	ne6xvf_init_spinlock(&hw->mbx.mbx_spinlock);
	spin_lock_init(&adapter->mac_vlan_list_lock);

	INIT_LIST_HEAD(&adapter->mac_filter_list);
	INIT_LIST_HEAD(&adapter->vlan_filter_list);
	INIT_LIST_HEAD(&adapter->macvlan_list);

	INIT_WORK(&adapter->sdk_task, ne6xvf_sdk_task);
	INIT_DELAYED_WORK(&adapter->watchdog_task, ne6xvf_watchdog_task);

	init_waitqueue_head(&adapter->down_waitqueue);
	init_waitqueue_head(&adapter->vc_waitqueue);

	ne6xvf_startup(adapter);
	ne6xvf_init_get_resources(adapter);
	adapter->aq_required = 0;
	ne6xvf_init_process_extended_caps(adapter);
	ne6xvf_init_config_adapter(adapter);

	queue_delayed_work(ne6xvf_wq, &adapter->watchdog_task,
			   msecs_to_jiffies(5 * (pdev->devfn & 0x07)));
#ifdef CONFIG_DEBUG_FS
	ne6xvf_dbg_pf_init(adapter);
#endif
	hw->debug_mask = 0xffffffff;
	return 0;
err_ioremap:
	free_netdev(netdev);
err_alloc_etherdev:
	pci_release_regions(pdev);
err_pci_reg:
err_dma:
	pci_disable_device(pdev);
	return err;
}

/**
 * ne6xvf_irq_enable_queues - Enable interrupt for specified queues
 * @adapter: board private structure
 * @mask: bitmap of queues to enable
 **/
void ne6xvf_irq_enable_queues(struct ne6xvf_adapter *adapter, u32 mask)
{
	struct ne6xvf_hw *hw = &adapter->hw;
	int i;

	for (i = 0; i < adapter->num_msix_vectors; i++)
		wr64(hw, NE6XVF_REG_ADDR(i, NE6X_VP_INT_MASK), ~(1ULL << NE6X_VP_CQ_INTSHIFT));
}

/**
 * ne6xvf_irq_enable - Enable default interrupt generation settings
 * @adapter: board private structure
 * @flush: boolean value whether to run rd32()
 **/
void ne6xvf_irq_enable(struct ne6xvf_adapter *adapter, bool flush)
{
	ne6xvf_irq_enable_queues(adapter, ~0);
}

void ne6xvf_free_all_tg_resources(struct ne6xvf_adapter *adapter)
{
	int i;

	if (!adapter->tg_rings)
		return;

	for (i = 0; i < adapter->num_active_queues; i++)
		if (adapter->tg_rings[i].desc) {
			struct ne6x_ring *tg_ring = &adapter->tg_rings[i];
			/* Zero out the descriptor ring */
			memset(tg_ring->desc, 0, tg_ring->size);
			tg_ring->next_to_use = 0;
			tg_ring->next_to_clean = 0;

			if (!tg_ring->netdev)
				return;

			dma_free_coherent(tg_ring->dev, tg_ring->size, tg_ring->desc, tg_ring->dma);
			tg_ring->desc = NULL;
		}
}

void ne6xvf_free_all_cq_resources(struct ne6xvf_adapter *adapter)
{
	int i;

	if (!adapter->cq_rings)
		return;

	for (i = 0; i < adapter->num_active_queues; i++)
		if (adapter->cq_rings[i].desc) {
			struct ne6x_ring *cq_ring = &adapter->cq_rings[i];
			/* Zero out the descriptor ring */
			memset(cq_ring->desc, 0, cq_ring->size);
			cq_ring->next_to_use = 0;
			cq_ring->next_to_clean = 0;

			if (!cq_ring->netdev)
				return;

			dma_free_coherent(cq_ring->dev, cq_ring->size, cq_ring->desc, cq_ring->dma);
			cq_ring->desc = NULL;
		}
}

void ne6xvf_free_all_tx_resources(struct ne6xvf_adapter *adapter)
{
	unsigned long bi_size;
	int i, idx;

	if (!adapter->tx_rings)
		return;

	for (i = 0; i < adapter->num_active_queues; i++)
		if (adapter->tx_rings[i].desc) {
			struct ne6x_ring *tx_ring = &adapter->tx_rings[i];

			/* ring already cleared, nothing to do */
			if (tx_ring->tx_buf) {
				/* Free all the Tx ring sk_buffs */
				for (idx = 0; idx < tx_ring->count; idx++)
					ne6xvf_unmap_and_free_tx_resource(tx_ring,
									  &tx_ring->tx_buf[idx]);

				bi_size = sizeof(struct ne6x_tx_buf) * tx_ring->count;
				memset(tx_ring->tx_buf, 0, bi_size);
				/* Zero out the descriptor ring */
				memset(tx_ring->desc, 0, tx_ring->size);
				tx_ring->next_to_use = 0;
				tx_ring->next_to_clean = 0;
				tx_ring->cq_last_expect = 0;

				if (tx_ring->netdev)
					/* cleanup Tx queue statistics */
					netdev_tx_reset_queue(txring_txq(tx_ring));
			}

			kfree(tx_ring->tx_buf);
			tx_ring->tx_buf = NULL;
			dma_free_coherent(tx_ring->dev, tx_ring->size, tx_ring->desc, tx_ring->dma);
			tx_ring->desc = NULL;
			kfree(tx_ring->sgl);
		}
}

void ne6xvf_free_all_rx_resources(struct ne6xvf_adapter *adapter)
{
	unsigned long bi_size;
	int i, idx;

	if (!adapter->rx_rings)
		return;

	for (i = 0; i < adapter->num_active_queues; i++)
		if (adapter->rx_rings[i].desc) {
			struct ne6x_ring *rx_ring = &adapter->rx_rings[i];
			/* ring already cleared, nothing to do */
			if (rx_ring->rx_buf) {
				if (rx_ring->skb) {
					dev_kfree_skb(rx_ring->skb);
					rx_ring->skb = NULL;
				}

				/* Free all the Rx ring sk_buffs */
				for (idx = 0; idx < rx_ring->count; idx++) {
					struct ne6x_rx_buf *rx_bi = &rx_ring->rx_buf[idx];

					if (!rx_bi->page)
						continue;

					/* Invalidate cache lines that may have been written to by
					 * device so that we avoid corrupting memory.
					 */
					dma_sync_single_range_for_cpu(rx_ring->dev, rx_bi->dma,
								      rx_bi->page_offset,
								      rx_ring->rx_buf_len,
								      DMA_FROM_DEVICE);

					/* free resources associated with mapping */
					dma_unmap_page_attrs(rx_ring->dev, rx_bi->dma,
							     ne6x_rx_pg_size(rx_ring),
							     DMA_FROM_DEVICE, NE6X_RX_DMA_ATTR);

					__page_frag_cache_drain(rx_bi->page, rx_bi->pagecnt_bias);

					rx_bi->page = NULL;
					rx_bi->page_offset = 0;
				}

				bi_size = sizeof(struct ne6x_rx_buf) * rx_ring->count;
				memset(rx_ring->rx_buf, 0, bi_size);

				/* Zero out the descriptor ring */
				memset(rx_ring->desc, 0, rx_ring->size);

				rx_ring->next_to_alloc = 0;
				rx_ring->next_to_clean = 0;
				rx_ring->next_to_use = 0;
				rx_ring->cq_last_expect = 0;
			}

			kfree(rx_ring->rx_buf);
			rx_ring->rx_buf = NULL;

			if (rx_ring->desc) {
				dma_free_coherent(rx_ring->dev, rx_ring->size, rx_ring->desc,
						  rx_ring->dma);
				rx_ring->desc = NULL;
			}
		}
}

void ne6xvf_reset_interrupt_capability(struct ne6xvf_adapter *adapter)
{
	if (!adapter->msix_entries)
		return;

	pci_disable_msix(adapter->pdev);
	kfree(adapter->msix_entries);
	adapter->msix_entries = NULL;
}

static void ne6xvf_remove(struct pci_dev *pdev)
{
	struct ne6xvf_adapter *adapter = ne6xvf_pdev_to_adapter(pdev);
	struct net_device *netdev = adapter->netdev;
	struct ne6xvf_vlan_filter *vlf, *vlftmp;
	struct ne6xvf_hw *hw = &adapter->hw;
	struct ne6xvf_mac_filter *f, *ftmp;
	struct ne6x_macvlan *mv, *mv_tmp;
	int i;

#ifdef CONFIG_DEBUG_FS
	ne6xvf_dbg_pf_exit(adapter);
#endif

	set_bit(__NE6XVF_IN_REMOVE_TASK, &adapter->crit_section);
	cancel_work_sync(&adapter->sdk_task);
	cancel_delayed_work_sync(&adapter->watchdog_task);

	if (adapter->netdev_registered) {
		/* This will call ne6xvf_close if the device was open previously.
		 * The Admin Queue and watchdog tasks have already been shut
		 * down at this point so the driver will rely on
		 * ne6xvf_request_reset below to disable the queues and handle
		 * any other Admin Queue-based cleanup normally done as part of
		 * ne6xvf_close.
		 */
		unregister_netdev(netdev);
		adapter->netdev_registered = false;
	}

	dev_info(&adapter->pdev->dev, "Removing device\n");

	/* Shut down all the garbage mashers on the detention level */
	ne6xvf_change_state(adapter, __NE6XVF_REMOVE);
	adapter->flags &= ~NE6XVF_FLAG_REINIT_ITR_NEEDED;

	ne6xvf_request_reset(adapter);

	for (i = 0; i < adapter->num_active_queues; i++)
		NE6XVF_WRITE_REG(hw, NE6XVF_REG_ADDR(i, NE6X_VP_RELOAD), 0x1);

	ne6xvf_free_all_tg_resources(adapter);
	ne6xvf_free_all_cq_resources(adapter);
	ne6xvf_free_all_tx_resources(adapter);
	ne6xvf_free_all_rx_resources(adapter);

	if (adapter->last_state == __NE6XVF_RESETTING ||
	    (adapter->last_state == __NE6XVF_RUNNING && !(netdev->flags & IFF_UP)))
		ne6xvf_free_traffic_irqs(adapter);

	ne6xvf_reset_interrupt_capability(adapter);
	ne6xvf_free_q_vectors(adapter);

	ne6xvf_destroy_spinlock(&hw->mbx.mbx_spinlock);

	spin_lock_bh(&adapter->mac_vlan_list_lock);
	list_for_each_entry_safe(f, ftmp, &adapter->mac_filter_list, list) {
		list_del(&f->list);
		kfree(f);
	}

	/* release vsi vlan list resource */
	list_for_each_entry_safe(vlf, vlftmp, &adapter->vlan_filter_list, list) {
		list_del(&vlf->list);
		kfree(vlf);
	}
	spin_unlock_bh(&adapter->mac_vlan_list_lock);

	list_for_each_entry_safe(mv, mv_tmp, &adapter->macvlan_list, list)
		ne6xvf_fwd_del_macvlan(netdev, mv);

	iounmap(hw->hw_addr0);
	iounmap(hw->hw_addr2);
	pci_release_regions(pdev);

	ne6xvf_free_queues(adapter);
	kfree(adapter->vf_res);
	adapter->vf_res = NULL;

	free_netdev(netdev);

	pci_disable_device(pdev);
}

static struct pci_driver ne6xvf_driver = {
	.name = ne6xvf_driver_name,
	.id_table = ne6xvf_pci_tbl,
	.probe = ne6xvf_probe,
	.remove = ne6xvf_remove,
};

static int __init ne6xvf_init_module(void)
{
	int ret;

	pr_info("navf: %s - version %s\n", ne6xvf_driver_string, ne6xvf_driver_version);

	pr_info("%s\n", ne6xvf_copyright);

	ne6xvf_wq = create_singlethread_workqueue(ne6xvf_driver_name);
	if (!ne6xvf_wq) {
		pr_err("%s: Failed to create workqueue\n", ne6xvf_driver_name);
		return -ENOMEM;
	}

#ifdef CONFIG_DEBUG_FS
	ne6xvf_dbg_init();
#endif
	ret = pci_register_driver(&ne6xvf_driver);

	return ret;
}

module_init(ne6xvf_init_module);

/**
 * ne6xvf_exit_module - Driver Exit Cleanup Routine
 *
 * ne6xvf_exit_module is called just before the driver is removed
 * from memory.
 **/
static void __exit ne6xvf_exit_module(void)
{
	pci_unregister_driver(&ne6xvf_driver);
	destroy_workqueue(ne6xvf_wq);
#ifdef CONFIG_DEBUG_FS
	ne6xvf_dbg_exit();
#endif
}

module_exit(ne6xvf_exit_module);
