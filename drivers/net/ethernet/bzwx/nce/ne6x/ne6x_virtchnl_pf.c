// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include "ne6x.h"
#include "ne6x_reg.h"
#include "ne6x_portmap.h"
#include "ne6x_dev.h"
#include "ne6x_txrx.h"
#include "ne6x_interrupt.h"

#ifdef CONFIG_PCI_IOV

void ne6x_clear_vf_status(struct ne6x_vf *vf)
{
	struct ne6x_flowctrl flowctrl;

	flowctrl.rx_pause = 0;
	flowctrl.tx_pause = 0;
	ne6x_dev_set_flowctrl(vf->adpt, &flowctrl);
	ne6x_dev_set_vf_bw(vf->adpt, 0);
}

void ne6x_mbx_deinit_snapshot(struct ne6x_hw *hw)
{
	struct ne6x_mbx_snapshot *snap = &hw->mbx_snapshot;

	/* Free VF counter array and reset vf counter length */
	kfree(snap->mbx_vf.vf_cntr);
	snap->mbx_vf.vfcntr_len = 0;
}

int ne6x_mbx_init_snapshot(struct ne6x_hw *hw, u16 vf_count)
{
	struct ne6x_mbx_snapshot *snap = &hw->mbx_snapshot;

	/* Ensure that the number of VFs allocated is non-zero and
	 * is not greater than the number of supported VFs defined in
	 * the functional capabilities of the PF.
	 */
	if (!vf_count || vf_count > NE6X_MAX_VP_NUM)
		return 1;

	snap->mbx_vf.vf_cntr = kcalloc(vf_count, sizeof(*snap->mbx_vf.vf_cntr), GFP_KERNEL);
	if (!snap->mbx_vf.vf_cntr)
		return 1;

	/* Setting the VF counter length to the number of allocated
	 * VFs for given PF's functional capabilities.
	 */
	snap->mbx_vf.vfcntr_len = vf_count;
	snap->state = NE6X_MAL_VF_DETECT_STATE_NEW_SNAPSHOT;
	memset(hw->ne6x_mbx_ready_to_send, true, 64);

	return 0;
}

int ne6x_status_to_errno(int err)
{
	if (err)
		return -EINVAL;

	return 0;
}

void ne6x_set_vf_state_qs_dis(struct ne6x_vf *vf)
{
	/* Clear Rx/Tx enabled queues flag */
	if (test_bit(NE6X_VF_STATE_QS_ENA, vf->vf_states))
		clear_bit(NE6X_VF_STATE_QS_ENA, vf->vf_states);
}

static void ne6x_dis_vf_qs(struct ne6x_vf *vf)
{
	ne6x_set_vf_state_qs_dis(vf);
}

bool ne6x_is_reset_in_progress(unsigned long *state)
{
	return test_bit(NE6X_PF_RESET_REQUESTED, state) ||
	       test_bit(NE6X_RESET_INTR_RECEIVED, state) ||
	       test_bit(NE6X_CORE_RESET_REQUESTED, state) ||
	       test_bit(NE6X_GLOBAL_RESET_REQUESTED, state);
}

void ne6x_adpt_close_vf(struct ne6x_adapter *adpt, u16 vf_id)
{
	if (!test_and_set_bit(NE6X_ADPT_DOWN, adpt->comm.state))
		clear_bit(NE6X_ADPT_DOWN, adpt->comm.state);
}

static int ne6x_adpt_clear_vf(struct ne6x_adapter *adpt)
{
	struct mac_addr_head *mc_head = &adpt->mc_mac_addr;
	struct mac_addr_head *uc_head = &adpt->uc_mac_addr;
	struct mac_addr_node *temp_node, *addr_node;
	struct ne6x_vlan_filter *vlf, *vlftmp;
	struct ne6x_pf *pf;

	if (!adpt)
		return 0;

	if (!adpt->back)
		goto free_adpt;

	pf = adpt->back;

	mutex_lock(&pf->switch_mutex);
	if (!pf->adpt[adpt->idx]) {
		dev_err(&pf->pdev->dev, "pf->adpt[%d] is NULL, just free adpt[%d](type %d)\n",
			adpt->idx, adpt->idx, adpt->type);
		goto unlock_adpt;
	}

	if (pf->adpt[adpt->idx] != adpt) {
		dev_err(&pf->pdev->dev, "pf->adpt[%d](type %d) != adpt[%d](type %d): no free!\n",
			pf->adpt[adpt->idx]->idx, pf->adpt[adpt->idx]->type, adpt->idx, adpt->type);
		goto unlock_adpt;
	}

	pf->adpt[adpt->idx] = NULL;
	if (adpt->idx < pf->next_adpt)
		pf->next_adpt = adpt->idx;

	kfree(adpt->tx_rings);
	adpt->tx_rings = NULL;

	kfree(adpt->q_vectors);
	adpt->q_vectors = NULL;

	kfree(adpt->port_info);
	adpt->port_info = NULL;

	/* release adpt multicast addr list resource */
	mutex_lock(&mc_head->mutex);
	list_for_each_entry_safe(addr_node, temp_node, &mc_head->list, list) {
		list_del(&addr_node->list);
		kfree(addr_node);
	}
	mutex_unlock(&mc_head->mutex);

	/* release adpt unicast addr list resource */
	mutex_lock(&uc_head->mutex);
	list_for_each_entry_safe(addr_node, temp_node, &uc_head->list, list) {
		list_del(&addr_node->list);
		kfree(addr_node);
	}
	mutex_unlock(&uc_head->mutex);

	spin_lock_bh(&adpt->mac_vlan_list_lock);
	/* release adpt vlan list resource */
	list_for_each_entry_safe(vlf, vlftmp, &adpt->vlan_filter_list, list) {
		list_del(&vlf->list);
		kfree(vlf);
	}
	spin_unlock_bh(&adpt->mac_vlan_list_lock);

unlock_adpt:
	mutex_unlock(&pf->switch_mutex);
free_adpt:
	kfree(adpt);

	return 0;
}

int ne6x_adpt_release_vf(struct ne6x_adapter *adpt, u16 vf_id)
{
	struct ne6x_pf *pf;

	if (!adpt->back)
		return -ENODEV;

	pf = adpt->back;

	if (adpt->netdev && !ne6x_is_reset_in_progress(pf->state) &&
	    (test_bit(NE6X_ADPT_NETDEV_REGISTERED, adpt->comm.state))) {
		unregister_netdev(adpt->netdev);
		clear_bit(NE6X_ADPT_NETDEV_REGISTERED, adpt->comm.state);
	}

	ne6x_adpt_close_vf(adpt, vf_id);

	if (!ne6x_is_reset_in_progress(pf->state))
		ne6x_adpt_clear_vf(adpt);

	return 0;
}

struct ne6x_adapter *ne6x_get_vf_adpt(struct ne6x_vf *vf)
{
	return vf->pf->adpt[vf->lan_adpt_idx];
}

static void ne6x_vf_invalidate_adpt(struct ne6x_vf *vf)
{
	vf->lan_adpt_idx = NE6X_NO_ADPT;
}

static void ne6x_vf_adpt_release(struct ne6x_vf *vf)
{
	ne6x_adpt_clear_mac_vlan(ne6x_get_vf_adpt(vf));
	ne6x_dev_del_broadcast_leaf(ne6x_get_vf_adpt(vf));
	ne6x_dev_set_features(vf->adpt, 0);
	ne6x_dev_del_vf_qinq(vf, 0, 0);
	ne6x_adpt_release_vf(ne6x_get_vf_adpt(vf), vf->vf_id);
	ne6x_vf_invalidate_adpt(vf);
}

static void ne6x_free_vf_res(struct ne6x_vf *vf)
{
	/* First, disable VF's configuration API to prevent OS from
	 * accessing the VF's adapter after it's freed or invalidated.
	 */
	clear_bit(NE6X_VF_STATE_INIT, vf->vf_states);

	/* free adapter and disconnect it from the parent uplink */
	if (vf->lan_adpt_idx != NE6X_NO_ADPT) {
		if (vf->tx_rate) {
			ne6x_dev_set_vf_bw(ne6x_get_vf_adpt(vf), 0);
			vf->tx_rate = 0;
		}

		ne6x_vf_adpt_release(vf);
	}
}

static int ne6x_sriov_free_msix_res(struct ne6x_pf *pf)
{
	struct ne6x_lump_tracking *res;

	if (!pf)
		return -EINVAL;

	res = pf->irq_pile;
	if (!res)
		return -EINVAL;

	wr64_bar4(&pf->hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DREQ_INT_MASK), 0xffffffffffffffff);
	wr64_bar4(&pf->hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DACK_INT_MASK), 0xffffffffffffffff);
	wr64_bar4(&pf->hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DREQ_INT), 0xffffffffffffffff);
	wr64_bar4(&pf->hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DACK_INT), 0xffffffffffffffff);

	return 0;
}

void ne6x_free_vfs(struct ne6x_pf *pf)
{
	struct device *dev = ne6x_pf_to_dev(pf);
	unsigned int tmp, i;
	u64 reg;

	if (!pf->vf)
		return;

	while (test_and_set_bit(NE6X_VF_DIS, pf->state))
		usleep_range(1000, 2000);

	/* Disable IOV before freeing resources. This lets any VF drivers
	 * running in the host get themselves cleaned up before we yank
	 * the carpet out from underneath their feet.
	 */
	if (!pci_vfs_assigned(pf->pdev))
		pci_disable_sriov(pf->pdev);
	else
		dev_warn(dev, "VFs are assigned - not disabling SR-IOV\n");

	/* Avoid wait time by stopping all VFs at the same time */
	ne6x_for_each_vf(pf, i) {
		if (test_bit(NE6X_VF_STATE_QS_ENA, pf->vf[i].vf_states))
			ne6x_dis_vf_qs(&pf->vf[i]);
	}

	tmp = pf->num_alloc_vfs;
	pf->num_qps_per_vf = 0;
	pf->num_alloc_vfs = 0;

	for (i = 0; i < tmp; i++) {
		if (test_bit(NE6X_VF_STATE_INIT, pf->vf[i].vf_states)) {
			set_bit(NE6X_VF_STATE_DIS, pf->vf[i].vf_states);
			ne6x_free_vf_res(&pf->vf[i]);
		}
	}

	if (ne6x_sriov_free_msix_res(pf))
		dev_err(dev, "Failed to free MSIX resources used by SR-IOV\n");

	ne6x_dev_clear_vport(pf);
	kfree(pf->vf);
	pf->vf = NULL;

	reg = rd64_bar4(&pf->hw, 0x05300);
	reg &= ~0xfc000;
	reg |= 0x8000;
	wr64_bar4(&pf->hw, 0x05300, reg);

	clear_bit(NE6X_VF_DIS, pf->state);
}

static int ne6x_alloc_vfs(struct ne6x_pf *pf, int num_vfs)
{
	struct ne6x_vf *vfs;

	vfs = kcalloc(num_vfs, sizeof(*vfs), GFP_KERNEL);
	if (!vfs)
		return -ENOMEM;

	pf->vf = vfs;
	pf->num_alloc_vfs = num_vfs;

	return 0;
}

static int ne6x_sriov_set_msix_res(struct ne6x_pf *pf, u16 num_msix_needed)
{
	int sriov_base_vector;

	sriov_base_vector = NE6X_MAX_MSIX_NUM - num_msix_needed;

	/* make sure we only grab irq_tracker entries from the list end and
	 * that we have enough available MSIX vectors
	 */
	if (sriov_base_vector < 0)
		return -EINVAL;

	return 0;
}

static int ne6x_set_per_vf_res(struct ne6x_pf *pf)
{
	struct device *dev = ne6x_pf_to_dev(pf);
	u16 queue;

	if (!pf->num_alloc_vfs)
		return -EINVAL;

	queue = NE6X_MAX_VP_NUM / pf->num_alloc_vfs;

	if (ne6x_sriov_set_msix_res(pf, queue * pf->num_alloc_vfs)) {
		dev_err(dev, "Unable to set MSI-X resources for %d VFs\n", pf->num_alloc_vfs);
		return -EINVAL;
	}

	/* only allow equal Tx/Rx queue count (i.e. queue pairs) */
	pf->num_qps_per_vf = queue;
	dev_info(dev, "Enabling %d VFs with %d vectors and %d queues per VF\n", pf->num_alloc_vfs,
		 pf->num_qps_per_vf, pf->num_qps_per_vf);

	return 0;
}

static void ne6x_vc_clear_allowlist(struct ne6x_vf *vf)
{
	bitmap_zero(vf->opcodes_allowlist, VIRTCHNL_OP_MAX);
}

/* default opcodes to communicate with VF */
static const u32 default_allowlist_opcodes[] = {
	VIRTCHNL_OP_GET_VF_RESOURCES,
	VIRTCHNL_OP_VERSION,
	VIRTCHNL_OP_RESET_VF,
};

static void ne6x_vc_allowlist_opcodes(struct ne6x_vf *vf, const u32 *opcodes, size_t size)
{
	unsigned int i;

	for (i = 0; i < size; i++)
		set_bit(opcodes[i], vf->opcodes_allowlist);
}

void ne6x_vc_set_default_allowlist(struct ne6x_vf *vf)
{
	ne6x_vc_clear_allowlist(vf);
	ne6x_vc_allowlist_opcodes(vf, default_allowlist_opcodes,
				  ARRAY_SIZE(default_allowlist_opcodes));
}

static void ne6x_set_dflt_settings_vfs(struct ne6x_pf *pf)
{
	int i;

	ne6x_for_each_vf(pf, i) {
		struct ne6x_vf *vf = &pf->vf[i];

		vf->pf = pf;
		vf->vf_id = i;
		vf->base_queue = (NE6X_MAX_VP_NUM / pf->num_alloc_vfs) * i;
		vf->num_vf_qs = pf->num_qps_per_vf;
		vf->tx_rate = 0;
		test_and_clear_bit(NE6X_VF_CONFIG_FLAG_TRUSTED, vf->vf_config_flag);
		ne6x_vc_set_default_allowlist(vf);
	}
}

void ne6x_send_init_mbx_mesg(struct ne6x_pf *pf)
{
	struct ne6x_hw *hw = &pf->hw;
	u64 reg_cfg;
	int i;

	wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DREQ_INT_MASK), 0xffffffffffffffff);
	wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DACK_INT_MASK), 0xffffffffffffffff);
	wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DREQ_INT), 0xffffffffffffffff);
	wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DACK_INT), 0xffffffffffffffff);

	ne6x_for_each_vf(pf, i) {
		struct ne6x_vf *vf = &pf->vf[i];

		wr64_bar4(hw, NE6X_PF_MAILBOX_ADDR(vf->base_queue), 0x0);
		reg_cfg = rd64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DREQ_INT_MASK));
		reg_cfg &= ~(1ULL << vf->base_queue);
		wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DREQ_INT_MASK), reg_cfg);
		wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DACK_INT_MASK), reg_cfg);
	}
}

static struct ne6x_port_info *ne6x_vf_get_port_info(struct ne6x_vf *vf)
{
	struct ne6x_adapter *adpt = ne6x_get_vf_adpt(vf);

	return adpt->port_info;
}

static struct ne6x_adapter *ne6x_adpt_alloc(struct ne6x_pf *pf, u16 vf_id, u16 num_vfs)
{
	struct device *dev = ne6x_pf_to_dev(pf);
	struct ne6x_adapter *adpt = NULL;
	int pf_adpt_idx;

	/* Need to protect the allocation of the adapters at the PF level */
	mutex_lock(&pf->switch_mutex);

	/* If we have already allocated our maximum number of adapters,
	 * pf->next_adpt will be NE6X_NO_ADPT. If not, pf->next_adpt index
	 * is available to be populated
	 */
	if (pf->next_adpt == NE6X_NO_ADPT) {
		dev_dbg(dev, "out of adapter slots!\n");
		goto unlock_pf;
	}

	adpt = kzalloc(sizeof(*adpt), GFP_KERNEL);
	adpt->back = pf;
	adpt->type = NE6X_ADPT_VF;
	set_bit(NE6X_ADPT_DOWN, adpt->comm.state);

	adpt->num_queue = pf->vf[vf_id].num_vf_qs;
	adpt->num_q_vectors = pf->vf[vf_id].num_vf_qs;
	/* vf_id 0 -- 63: vport: 0 -- 64: pf: 64 -- 68 */
	adpt->idx = pf->vf[vf_id].vf_id + pf->num_alloc_adpt;
	adpt->vport = pf->vf[vf_id].vf_id;
	adpt->port_info = kzalloc(sizeof(*adpt->port_info), GFP_KERNEL);
	if (!adpt->port_info)
		goto err_rings;

	/* vf attach pf alloc */
	pf_adpt_idx = pf->vf[vf_id].base_queue / (NE6X_MAX_VP_NUM / pf->hw.pf_port);
	adpt->port_info->lport = pf->adpt[pf_adpt_idx]->port_info->lport;
	adpt->port_info->hw_port_id = pf->adpt[pf_adpt_idx]->port_info->hw_port_id;
	adpt->port_info->hw = &pf->hw;
	adpt->port_info->hw_trunk_id = pf->adpt[pf_adpt_idx]->port_info->hw_trunk_id;
	adpt->port_info->hw_queue_base = pf->vf[vf_id].base_queue;
	adpt->port_info->hw_max_queue = pf->vf[vf_id].num_vf_qs;
	adpt->base_queue = pf->vf[vf_id].base_queue;

	/* init multicast MAC addr list head node */
	INIT_LIST_HEAD(&adpt->mc_mac_addr.list);
	mutex_init(&adpt->mc_mac_addr.mutex);

	/* init unicast MAC addr list head node */
	INIT_LIST_HEAD(&adpt->uc_mac_addr.list);
	mutex_init(&adpt->uc_mac_addr.mutex);

	/* init vlan list head node */
	spin_lock_init(&adpt->mac_vlan_list_lock);
	INIT_LIST_HEAD(&adpt->vlan_filter_list);

	pf->adpt[adpt->idx] = adpt;

	goto unlock_pf;

err_rings:
	kfree(adpt);
	adpt = NULL;
unlock_pf:
	mutex_unlock(&pf->switch_mutex);
	return adpt;
}

struct ne6x_adapter *ne6x_adpt_setup_vf(struct ne6x_pf *pf, u16 vf_id, u16 num_vfs)
{
	struct device *dev = ne6x_pf_to_dev(pf);
	struct ne6x_adapter *adpt;

	adpt = ne6x_adpt_alloc(pf, vf_id, num_vfs);
	if (!adpt) {
		dev_err(dev, "could not allocate adapter\n");
		return NULL;
	}

	return adpt;
}

static struct ne6x_adapter *ne6x_vf_adpt_setup(struct ne6x_vf *vf, u16 num_vfs)
{
	struct ne6x_pf *pf = vf->pf;
	struct ne6x_adapter *adpt;

	adpt = ne6x_adpt_setup_vf(pf, vf->vf_id, num_vfs);
	if (!adpt) {
		dev_err(ne6x_pf_to_dev(pf), "Failed to create VF adapter\n");
		ne6x_vf_invalidate_adpt(vf);
		return NULL;
	}

	vf->lan_adpt_idx = adpt->idx;
	vf->adpt = adpt;

	return adpt;
}

static int ne6x_init_vf_adpt_res(struct ne6x_vf *vf, u16 num_vfs)
{
	struct ne6x_pf *pf = vf->pf;
	u8 broadcast[ETH_ALEN];
	struct ne6x_adapter *adpt;
	struct device *dev;

	dev = ne6x_pf_to_dev(pf);
	adpt = ne6x_vf_adpt_setup(vf, num_vfs);
	if (!adpt)
		return -ENOMEM;

	vf->tx_rate = 0;
	ne6x_dev_set_vf_bw(adpt, vf->tx_rate);
	eth_broadcast_addr(broadcast);

	return 0;
}

static int ne6x_start_vfs(struct ne6x_pf *pf, u16 num_vfs)
{
	int retval, i;

	ne6x_for_each_vf(pf, i) {
		struct ne6x_vf *vf = &pf->vf[i];

		retval = ne6x_init_vf_adpt_res(vf, num_vfs);
		if (retval) {
			dev_err(ne6x_pf_to_dev(pf), "Failed to initialize adapter resources for VF %d, error %d\n",
				vf->vf_id, retval);
			goto teardown;
		}

		set_bit(NE6X_VF_STATE_INIT, vf->vf_states);
	}

	ne6x_linkscan_schedule(pf);

	return 0;

teardown:
	for (i = i - 1; i >= 0; i--) {
		struct ne6x_vf *vf = &pf->vf[i];

		ne6x_vf_adpt_release(vf);
	}

	return retval;
}

static int ne6x_delete_pf_trunk(struct ne6x_pf *pf)
{
	return 0;
}

static int ne6x_recycle_vp_resources(struct ne6x_pf *pf)
{
	struct ne6x_adapter *adpt;
	int rst, i;
	u64 reg;

	rst = ne6x_delete_pf_trunk(pf);
	if (rst)
		return rst;

	ne6x_disable_link_irq(pf);
	ne6x_free_link_irq(pf);
	for (i = 0; i < pf->num_alloc_adpt; i++) {
		adpt = pf->adpt[i];
		if (test_bit(NE6X_ADPT_OPEN, adpt->comm.state))
			ne6x_adpt_close(adpt);
	}

	reg = rd64_bar4(&pf->hw, 0x05300);
	reg &= ~0xfc000;
	reg |= 0x7c000;
	wr64_bar4(&pf->hw, 0x05300, reg);

	return 0;
}

static int ne6x_adpt_resetup(struct ne6x_pf *pf, bool recovery)
{
	int vid, pooling, i, actual_vector = 1, size;
	struct device *dev = ne6x_pf_to_dev(pf);
	union ne6x_ciu_time_out_cfg ciu_time_out_cdg;
	union ne6x_all_rq_cfg all_rq_cfg;
	union ne6x_all_sq_cfg all_sq_cfg;
	union ne6x_all_cq_cfg all_cq_cfg;
	union ne6x_merge_cfg merge_cfg;
	struct ne6x_hw *hw = &pf->hw;
	int qp_remaining, q_vectors;
	struct ne6x_adapter *adpt = NULL;
	u64 __iomem *reg;

	pooling = test_bit(NE6X_LINK_POOLING, pf->state);
	if (pooling)
		clear_bit(NE6X_LINK_POOLING, pf->state);

	if (test_bit(NE6X_PF_MSIX, pf->state)) {
		pci_disable_msix(pf->pdev);
		actual_vector = pci_enable_msix_range(pf->pdev, pf->msix_entries, NE6X_MIN_MSIX,
						      NE6X_MAX_MSIX_NUM);
		if (actual_vector < NE6X_MAX_MSIX_NUM) {
			clear_bit(NE6X_PF_MSIX, pf->state);
			pci_disable_msix(pf->pdev);
			dev_err(dev, "%s-%d: error msix enable failed\n", __func__, __LINE__);
		}

		pf->irq_pile->num_entries = actual_vector;
	} else {
		if (!pf->irq_pile) {
			size = sizeof(struct ne6x_lump_tracking) + (sizeof(u16) * actual_vector);
			pf->irq_pile = kzalloc(size, GFP_KERNEL);
			if (!pf->irq_pile) {
				dev_err(dev, "error intx allocating irq_pile memory\n");
				return -ENOMEM;
			}

			pf->irq_pile->num_entries = actual_vector;
		}

		test_and_set_bit(NE6X_PF_INTX, pf->state);
	}

	reg = (void __iomem *)hw->hw_addr4 + NE6X_PFINT_DYN_CTLN(7, NE6X_ALL_RQ_CFG);
	all_rq_cfg.val = readq(reg);
	all_rq_cfg.reg.csr_allrq_pull_merge_cfg = 0x10;
	writeq(all_rq_cfg.val, reg);
	reg = (void __iomem *)hw->hw_addr4 + NE6X_PFINT_DYN_CTLN(7, NE6X_ALL_SQ_CFG);
	all_sq_cfg.val = readq(reg);
	all_sq_cfg.reg.csr_allsq_pull_merge_cfg = 0x10;
	writeq(all_sq_cfg.val, reg);
	reg = (void __iomem *)hw->hw_addr4 + NE6X_PFINT_DYN_CTLN(7, NE6X_ALL_CQ_CFG);
	all_cq_cfg.val = readq(reg);
	all_cq_cfg.reg.csr_allcq_merge_size = 0x1;
	all_cq_cfg.reg.csr_allcq_wt_rr_cnt = 0x7F;
	all_cq_cfg.reg.csr_allcq_wt_rr_flag = 0x1;
	writeq(all_cq_cfg.val, reg);
	reg = (void __iomem *)hw->hw_addr4 + NE6X_PFINT_DYN_CTLN(7, NE6X_MERGE_CFG);
	merge_cfg.val = readq(reg);
	merge_cfg.reg.csr_merge_clk_cnt = 800;
	writeq(merge_cfg.val, reg);
	reg = (void __iomem *)hw->hw_addr4 + NE6X_PFINT_DYN_CTLN(7, NE6X_CIU_TIME_OUT_CFG);
	ciu_time_out_cdg.val = readq(reg);
	ciu_time_out_cdg.reg.csr_int_timer_out_cnt = 0xfff;
	writeq(ciu_time_out_cdg.val, reg);

	ne6x_for_each_pf(pf, vid) {
		adpt = pf->adpt[vid];
		if (recovery) {
			adpt->port_info->hw_queue_base = adpt->port_info->hw_queue_base_old;
			adpt->base_queue = adpt->port_info->hw_queue_base;
			adpt->port_info->hw_queue_base = pf->hw.expect_vp * vid;
			adpt->base_queue = adpt->port_info->hw_queue_base;
			adpt->base_vector = adpt->base_queue;
			adpt->port_info->hw_max_queue = pf->hw.max_queue;
			adpt->port_info->queue = adpt->port_info->hw_max_queue;
			adpt->num_q_vectors = adpt->port_info->queue;
			adpt->num_queue = adpt->num_q_vectors;
		} else {
			adpt->port_info->hw_queue_base_old = adpt->port_info->hw_queue_base;
			adpt->port_info->hw_queue_base = NE6X_PF_VP1_NUM + vid;
			adpt->base_queue = adpt->port_info->hw_queue_base;
			adpt->base_vector = adpt->base_queue;
			adpt->port_info->hw_max_queue = 1u;
			adpt->port_info->queue = 1u;
			adpt->num_q_vectors = adpt->port_info->queue;
			adpt->num_queue = adpt->num_q_vectors;
		}

		for (i = 0; i < adpt->num_queue; i++) {
			adpt->rx_rings[i]->reg_idx = adpt->base_queue + i;
			adpt->cq_rings[i]->reg_idx = adpt->rx_rings[i]->reg_idx;
			adpt->tx_rings[i]->reg_idx = adpt->cq_rings[i]->reg_idx;
		}

		qp_remaining = adpt->num_queue;
		q_vectors = adpt->num_q_vectors;
		for (i = 0; i < adpt->num_q_vectors; i++) {
			adpt->q_vectors[i]->num_ringpairs =
				DIV_ROUND_UP(qp_remaining, q_vectors - i);
			adpt->q_vectors[i]->reg_idx =
				adpt->q_vectors[i]->v_idx + adpt->base_vector;
			qp_remaining--;
		}

		ne6x_adpt_reset_stats(adpt);
		ne6x_dev_set_vport(adpt);
		for (i = 0; i < adpt->rss_info.ind_table_size; i++)
			adpt->rss_info.ind_table[i] =
				ethtool_rxfh_indir_default(i, adpt->num_queue);

		ne6x_dev_set_rss(adpt, &adpt->rss_info);
		ne6x_dev_set_port2pi(adpt);
		rtnl_lock();

		if (test_bit(NE6X_ADPT_OPEN, adpt->comm.state))
			ne6x_adpt_open(adpt);

		rtnl_unlock();
	}

	ne6x_init_link_irq(pf);
	ne6x_enable_link_irq(pf);

	if (pooling) {
		set_bit(NE6X_LINK_POOLING, pf->state);
		ne6x_linkscan_schedule(pf);
	}

	return 0;
}

static int ne6x_ena_vfs(struct ne6x_pf *pf, u16 num_vfs)
{
	struct device *dev = ne6x_pf_to_dev(pf);
	int ret;

	ret = ne6x_recycle_vp_resources(pf);
	if (ret)
		goto err_pci_disable_sriov;

	ret = ne6x_adpt_resetup(pf, false);
	if (ret)
		goto err_pci_disable_sriov;

	ne6x_clr_vf_bw_for_max_vpnum(pf);
	ret = ne6x_alloc_vfs(pf, num_vfs);
	if (ret)
		goto err_pci_disable_sriov;

	if (ne6x_set_per_vf_res(pf)) {
		dev_err(dev, "Not enough resources for %d VFs, try with fewer number of VFs\n",
			num_vfs);
		ret = -ENOSPC;
		goto err_unroll_sriov;
	}

	ne6x_set_dflt_settings_vfs(pf);
	if (ne6x_start_vfs(pf, num_vfs)) {
		dev_err(dev, "Failed to start VF(s)\n");
		ret = -EAGAIN;
		goto err_unroll_sriov;
	}

	ne6x_init_mailbox_irq(pf);
	ne6x_send_init_mbx_mesg(pf);
	clear_bit(NE6X_VF_DIS, pf->state);

	return 0;

err_unroll_sriov:
	kfree(pf->vf);
	pf->vf = NULL;
	pf->num_alloc_vfs = 0;
err_pci_disable_sriov:
	pci_disable_sriov(pf->pdev);

	return ret;
}

static int ne6x_pci_sriov_ena(struct ne6x_pf *pf, int num_vfs)
{
	int pre_existing_vfs = pci_num_vf(pf->pdev);
	struct device *dev = ne6x_pf_to_dev(pf);
	int err;

	if (pre_existing_vfs && pre_existing_vfs != num_vfs)
		ne6x_free_vfs(pf);
	else if (pre_existing_vfs && pre_existing_vfs == num_vfs)
		return 0;

	if (num_vfs > NE6X_MAX_VP_NUM) {
		dev_err(dev, "Can't enable %d VFs, max VFs supported is %d\n", num_vfs,
			NE6X_MAX_VP_NUM);
		return -EOPNOTSUPP;
	}

	err = ne6x_ena_vfs(pf, num_vfs);
	if (err) {
		dev_err(dev, "Failed to enable SR-IOV: %d\n", err);
		return err;
	}

	if (num_vfs)
		test_and_set_bit(NE6X_FLAG_SRIOV_ENA, pf->state);

	return 0;
}

int ne6x_sriov_configure(struct pci_dev *pdev, int num_vfs)
{
	struct ne6x_pf *pf = pci_get_drvdata(pdev);
	struct ne6x_adapter *adpt = NULL;
	struct ne6x_vf *vf  = NULL;
	pbmp_t port_bitmap;
	int err = 0, vf_id;
	int timeout = 50;
	int status;

	if (!(num_vfs == 0 || num_vfs == 2 || num_vfs == 4 || num_vfs == 8 ||
	      num_vfs == 16 || num_vfs == 32 || num_vfs == 64))
		return -EINVAL;

	if (pf->irq_pile->num_entries < NE6X_MAX_MSIX_NUM) {
		dev_err(ne6x_pf_to_dev(pf), "ne6x irq number < %d!\n", NE6X_MAX_MSIX_NUM);
		return -EPERM;
	}

	while (test_and_set_bit(NE6X_CONFIG_BUSY, pf->state)) {
		timeout--;
		if (!timeout) {
			dev_warn(ne6x_pf_to_dev(pf), "ne6x config busy, timeout!\n");
			return -EBUSY;
		}
		usleep_range(1000, 2000);
	}

	if (!num_vfs) {
		set_bit(NE6X_TIMEOUT_RECOVERY_PENDING, pf->state);
		if (!pci_vfs_assigned(pdev)) {
			ne6x_free_vfs(pf);
			ne6x_disable_mailbox_irq(pf);
			ne6x_free_mailbox_irq(pf);
			ne6x_mbx_deinit_snapshot(&pf->hw);
			if (test_bit(NE6X_FLAG_SRIOV_ENA, pf->state))
				clear_bit(NE6X_FLAG_SRIOV_ENA, pf->state);

			if (!test_bit(NE6X_REMOVE, pf->state)) {
				ne6x_recycle_vp_resources(pf);
				err = ne6x_adpt_resetup(pf, true);
			}

			clear_bit(NE6X_TIMEOUT_RECOVERY_PENDING, pf->state);
			clear_bit(NE6X_CONFIG_BUSY, pf->state);
			if (err)
				goto err_recovery;

			return 0;
		}

		clear_bit(NE6X_TIMEOUT_RECOVERY_PENDING, pf->state);
		clear_bit(NE6X_CONFIG_BUSY, pf->state);
		return -EBUSY;
	}

	status = ne6x_mbx_init_snapshot(&pf->hw, num_vfs);
	if (status)
		return ne6x_status_to_errno(status);

	err = ne6x_pci_sriov_ena(pf, num_vfs);
	if (err) {
		ne6x_mbx_deinit_snapshot(&pf->hw);
		clear_bit(NE6X_CONFIG_BUSY, pf->state);
		return err;
	}

	PBMP_CLEAR(port_bitmap);

	/* config vport, default vlan */
	ne6x_for_each_vf(pf, vf_id) {
		vf = &pf->vf[vf_id];
		adpt = vf->adpt;

		/* config default vlan */
		PBMP_PORT_ADD(port_bitmap, adpt->vport);
		ne6x_dev_set_vport(adpt);
		adpt->hw_feature = ne6x_dev_get_features(adpt);
	}

	err = pci_enable_sriov(pf->pdev, num_vfs);
	if (err)
		goto err_hanler;

	clear_bit(NE6X_CONFIG_BUSY, pf->state);

	return num_vfs;

err_hanler:
	ne6x_dev_clear_vport(pf);
	/* config vport, default vlan */
	ne6x_for_each_pf(pf, vf_id) {
		adpt = pf->adpt[vf_id];
		adpt->port_info->hw_queue_base = adpt->port_info->hw_queue_base_old;
		ne6x_dev_set_vport(adpt);
	}

	if (!pci_vfs_assigned(pdev)) {
		ne6x_mbx_deinit_snapshot(&pf->hw);
		ne6x_free_vfs(pf);
		pf->num_alloc_vfs = 0;
		if (test_bit(NE6X_FLAG_SRIOV_ENA, pf->state))
			clear_bit(NE6X_FLAG_SRIOV_ENA, pf->state);
	}

err_recovery:
	clear_bit(NE6X_CONFIG_BUSY, pf->state);
	return err;
}

static int ne6x_validate_vf_id(struct ne6x_pf *pf, u16 vf_id)
{
	/* vf_id range is only valid for 0-255, and should always be unsigned */
	if (vf_id >= pf->num_alloc_vfs)
		return -EINVAL;

	return 0;
}

static int ne6x_validate_outer_vf_id(struct ne6x_pf *pf, u16 out_vf_id)
{
	if (out_vf_id >= (pf->num_alloc_vfs / pf->num_alloc_adpt))
		return -EINVAL;

	return 0;
}

int ne6x_sdk_send_msg_to_vf(struct ne6x_hw *hw, u16 vfid, u32 v_opcode,
			    u32 v_retval, u8 *msg, u16 msglen)
{
	union u_ne6x_mbx_snap_buffer_data usnap;
	struct ne6x_pf *pf = hw->back;
	struct ne6x_vf *vf = &pf->vf[vfid];
	int timeout = 2000;
	int i;

	usnap.snap.state = v_retval;
	usnap.snap.len = msglen;
	usnap.snap.type = v_opcode;

	for (i = 0; i < msglen && i < 6; i++)
		usnap.snap.data[i] = msg[i];

	while (!(pf->hw.ne6x_mbx_ready_to_send[vfid])) {
		usleep_range(100, 200);
		timeout--;
		if (!timeout)
			break;
	}

	wr64_bar4(hw, NE6X_PF_MAILBOX_ADDR(vf->base_queue), usnap.val);
	wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_INT_REQ), (1ULL << vf->base_queue));
	pf->hw.mbx_snapshot.state = NE6X_MAL_VF_DETECT_STATE_TRAVERSE;
	pf->hw.ne6x_mbx_ready_to_send[vfid] = false;

	return 0;
}

static int ne6x_vc_send_msg_to_vf(struct ne6x_vf *vf, u32 v_opcode,
				  enum virtchnl_status_code v_retval,
				  u8 *msg, u16 msglen)
{
	struct device *dev;
	struct ne6x_pf *pf;
	int aq_ret;

	if (!vf)
		return -EINVAL;

	pf = vf->pf;
	dev = ne6x_pf_to_dev(pf);

	if (ne6x_validate_vf_id(pf, vf->vf_id)) {
		dev_err(dev, "vf id[%d] is invalid\n", vf->vf_id);
		return -EINVAL;
	}

	/* single place to detect unsuccessful return values */
	if (v_retval)
		dev_info(dev, "VF %d failed opcode %s, retval: %s\n", vf->vf_id,
			 ne6x_opcode_str(v_opcode), ne6x_mbox_status_str(v_retval));

	aq_ret = ne6x_sdk_send_msg_to_vf(&pf->hw, vf->vf_id, v_opcode, v_retval, msg, msglen);
	if (aq_ret) {
		dev_info(dev, "Unable to send the message to VF %d aq_err %d\n", vf->vf_id, aq_ret);
		return -EIO;
	}

	return 0;
}

static int ne6x_check_vf_init(struct ne6x_pf *pf, struct ne6x_vf *vf)
{
	if (!test_bit(NE6X_VF_STATE_INIT, vf->vf_states)) {
		dev_err(ne6x_pf_to_dev(pf), "VF ID: %u in reset. Try again.\n", vf->vf_id);
		return -EBUSY;
	}

	return 0;
}

static int ne6x_vc_add_def_mac_addr(struct ne6x_vf *vf, struct ne6x_adapter *adpt,
				    struct virtchnl_ether_addr *vc_ether_addr)
{
	struct device *dev = ne6x_pf_to_dev(vf->pf);
	u8 *mac_addr = vc_ether_addr->addr;

	if (!is_unicast_ether_addr(mac_addr)) {
		dev_err(dev, "VF attempting to override administratively set MAC address, bring down and up the VF interface to resume normal operation\n");
		return -EPERM;
	}

	if (ether_addr_equal(mac_addr, vf->dev_lan_addr.addr)) {
		dev_err(dev, "vf already use the same addr\n");
		return -EPERM;
	}

	ether_addr_copy(vf->dev_lan_addr.addr, mac_addr);
	ne6x_adpt_add_mac(adpt, mac_addr, true);

	return 0;
}

static int ne6x_vc_del_def_mac_addr(struct ne6x_vf *vf, struct ne6x_adapter *adpt, u8 *mac)
{
	return ne6x_adpt_del_mac(adpt, mac, true);
}

static int ne6x_vc_get_vf_res_msg(struct ne6x_vf *vf, u8 *msg)
{
	enum virtchnl_status_code v_ret = VIRTCHNL_STATUS_SUCCESS;
	union u_ne6x_mbx_snap_buffer_data *vfres = NULL;
	union u_ne6x_mbx_snap_buffer_data *rsvsnap;
	struct virtchnl_ether_addr vc_ether_addr;
	struct ne6x_pf *pf = vf->pf;
	struct ne6x_adapter *pf_adpt;
	int len, ret;

	if (ne6x_check_vf_init(pf, vf)) {
		v_ret = VIRTCHNL_STATUS_ERR_PARAM;
		goto err;
	}

	rsvsnap = (union u_ne6x_mbx_snap_buffer_data *)msg;
	vc_ether_addr.addr[0] = rsvsnap->snap.data[0];
	vc_ether_addr.addr[1] = rsvsnap->snap.data[1];
	vc_ether_addr.addr[2] = rsvsnap->snap.data[2];
	vc_ether_addr.addr[3] = rsvsnap->snap.data[3];
	vc_ether_addr.addr[4] = rsvsnap->snap.data[4];
	vc_ether_addr.addr[5] = rsvsnap->snap.data[5];

	pf_adpt = vf->adpt;

	ne6x_vc_add_def_mac_addr(vf, pf_adpt, &vc_ether_addr);

	len = sizeof(union u_ne6x_mbx_snap_buffer_data);
	vfres = kzalloc(len, GFP_KERNEL);
	if (!vfres) {
		v_ret = VIRTCHNL_STATUS_ERR_NO_MEMORY;
		len = 0;
		goto err;
	}

	vfres->snap.type = VIRTCHNL_OP_GET_VF_RESOURCES;
	vfres->snap.data[0] = vf->vf_id;                             /* vport */
	vfres->snap.data[1] = pf_adpt->port_info->lport;              /* lport */
	vfres->snap.data[2] = pf_adpt->port_info->hw_port_id;         /* pport */
	vfres->snap.data[3] = pf_adpt->port_info->hw_queue_base;      /* base_queue */
	vfres->snap.data[4] = pf->num_qps_per_vf;                    /* num_qps_per_vf */
	vfres->snap.data[5] = pf->num_alloc_vfs / pf->num_alloc_adpt; /* num vfs of per hw_port */
	vfres->snap.len = 6;
	vf->ready = 0;
	vf->adpt->port_info->phy.link_info.link_info = 0;
	vf->ready_to_link_notify = 0;
	set_bit(NE6X_VF_STATE_ACTIVE, vf->vf_states);

err:
	/* send the response back to the VF */
	vfres->snap.state = v_ret;
	ret = ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_GET_VF_RESOURCES,
				     vfres->snap.state,
				     (u8 *)vfres->snap.data,
				     vfres->snap.len);

	return ret;
}

static int ne6x_vc_add_mac_addr(struct ne6x_vf *vf, struct ne6x_adapter *adpt,
				struct virtchnl_ether_addr *vc_ether_addr)
{
	u8 *mac_addr = vc_ether_addr->addr;
	int ret = 0;

	if (likely(is_multicast_ether_addr(mac_addr))) {
		if (is_broadcast_ether_addr(mac_addr))
			return 0;

		ne6x_adpt_add_mac(adpt, mac_addr, false);
	} else {
		ne6x_adpt_add_mac(adpt, mac_addr, true);
	}

	return ret;
}

static int ne6x_vc_del_mac_addr(struct ne6x_vf *vf, struct ne6x_adapter *adpt,
				struct virtchnl_ether_addr *vc_ether_addr)
{
	u8 *mac_addr = vc_ether_addr->addr;
	int ret = 0;

	if (likely(is_multicast_ether_addr(mac_addr))) {
		if (is_broadcast_ether_addr(mac_addr))
			return 0;

		ne6x_adpt_del_mac(adpt, mac_addr, false);
	} else {
		ne6x_adpt_del_mac(adpt, mac_addr, true);
	}

	return ret;
}

static int ne6x_vc_handle_mac_addr_msg(struct ne6x_vf *vf, u8 *msg, bool set)
{
	int (*ne6x_vc_cfg_mac)(struct ne6x_vf *vf, struct ne6x_adapter *adpt,
			       struct virtchnl_ether_addr *virtchnl_ether_addr);
	enum virtchnl_status_code v_ret = VIRTCHNL_STATUS_SUCCESS;
	union u_ne6x_mbx_snap_buffer_data *usnap;
	struct virtchnl_ether_addr eth_addr;
	enum virtchnl_ops vc_op;
	struct ne6x_adapter *adpt;
	u8 *mac_addr;
	int result;

	if (set) {
		vc_op = VIRTCHNL_OP_ADD_ETH_ADDR;
		ne6x_vc_cfg_mac = ne6x_vc_add_mac_addr;
	} else {
		vc_op = VIRTCHNL_OP_DEL_ETH_ADDR;
		ne6x_vc_cfg_mac = ne6x_vc_del_mac_addr;
	}

	adpt = ne6x_get_vf_adpt(vf);
	if (!adpt) {
		v_ret = VIRTCHNL_STATUS_ERR_PARAM;
		goto handle_mac_exit;
	}

	usnap = (union u_ne6x_mbx_snap_buffer_data *)msg;
	mac_addr = usnap->snap.data;

	if (is_broadcast_ether_addr(mac_addr) || is_zero_ether_addr(mac_addr))
		goto handle_mac_exit;

	if (ether_addr_equal(vf->dev_lan_addr.addr, mac_addr))
		goto handle_mac_exit;

	ether_addr_copy(eth_addr.addr, mac_addr);
	result = ne6x_vc_cfg_mac(vf, adpt, &eth_addr);
	if (result == -EEXIST || result == -ENOENT) {
		goto handle_mac_exit;
	} else if (result) {
		v_ret = VIRTCHNL_STATUS_ERR_ADMIN_QUEUE_ERROR;
		goto handle_mac_exit;
	}

handle_mac_exit:
	/* send the response to the VF */
	return ne6x_vc_send_msg_to_vf(vf, vc_op, v_ret, NULL, 0);
}

static int ne6x_vc_add_mac_addr_msg(struct ne6x_vf *vf, u8 *msg)
{
	return ne6x_vc_handle_mac_addr_msg(vf, msg, true);
}

static int ne6x_vc_del_mac_addr_msg(struct ne6x_vf *vf, u8 *msg)
{
	return ne6x_vc_handle_mac_addr_msg(vf, msg, false);
}

static int ne6x_vf_set_adpt_promisc(struct ne6x_vf *vf, struct ne6x_adapter *adpt,
				    u8 promisc_m)
{
	int status = 0;

	dev_info(ne6x_pf_to_dev(adpt->back), "%s: adpt->vport = %d enable promiscuous <%s>\n",
		 __func__, adpt->vport,
		 (promisc_m & NE6X_UCAST_PROMISC_BITS) ? "unicast" : "multicast");

	if (promisc_m & NE6X_UCAST_PROMISC_BITS)
		status = ne6x_dev_set_uc_promiscuous_enable(adpt, true);

	if (promisc_m & NE6X_MCAST_PROMISC_BITS)
		status = ne6x_dev_set_mc_promiscuous_enable(adpt, true);

	if (status) {
		dev_err(ne6x_pf_to_dev(adpt->back), "disable Tx/Rx filter promiscuous mode off VF-%u mac: %d, trunk: 0x%x, failed, error: %d\n",
			vf->vf_id, 0, adpt->port_info->hw_trunk_id, status);
		return status;
	}

	return 0;
}

static int ne6x_vf_clear_adpt_promisc(struct ne6x_vf *vf, struct ne6x_adapter *adpt, u8 promisc_m)
{
	int status = 0;

	dev_info(ne6x_pf_to_dev(adpt->back), "%s: adpt->vport = %d clear promiscuous <%s>\n",
		 __func__, adpt->vport,
		 (promisc_m & NE6X_UCAST_PROMISC_BITS) ? "unicast" : "multicast");

	if (promisc_m & NE6X_UCAST_PROMISC_BITS)
		status = ne6x_dev_set_uc_promiscuous_enable(adpt, false);

	if (promisc_m & NE6X_MCAST_PROMISC_BITS)
		status = ne6x_dev_set_mc_promiscuous_enable(adpt, false);

	if (status) {
		dev_err(ne6x_pf_to_dev(adpt->back), "disable Tx/Rx filter promiscuous mode on VF-%u failed, error: %d\n",
			vf->vf_id, status);
		return status;
	}

	return 0;
}

static int ne6x_vc_cfg_promiscuous_mode_msg(struct ne6x_vf *vf, u8 *msg)
{
	union u_ne6x_mbx_snap_buffer_data *usnap = (union u_ne6x_mbx_snap_buffer_data *)msg;
	struct virtchnl_promisc_info *info = (struct virtchnl_promisc_info *)usnap->snap.data;
	enum virtchnl_status_code v_ret = VIRTCHNL_STATUS_SUCCESS;
	bool alluni = false, allmulti = false;
	int ucast_err = 0, mcast_err = 0;
	struct ne6x_pf *pf = vf->pf;
	u8 mcast_m, ucast_m;
	struct ne6x_adapter *adpt;
	struct device *dev;

	if (!test_bit(NE6X_VF_STATE_ACTIVE, vf->vf_states)) {
		v_ret = VIRTCHNL_STATUS_ERR_PARAM;
		goto error_param;
	}

	adpt = ne6x_get_vf_adpt(vf);
	if (!adpt) {
		v_ret = VIRTCHNL_STATUS_ERR_PARAM;
		goto error_param;
	}

	dev = ne6x_pf_to_dev(pf);

	if (info->flags & FLAG_VF_UNICAST_PROMISC)
		alluni = true;

	if (info->flags & FLAG_VF_MULTICAST_PROMISC)
		allmulti = true;

	mcast_m = NE6X_MCAST_PROMISC_BITS;
	ucast_m = NE6X_UCAST_PROMISC_BITS;

	if (alluni)
		ucast_err = ne6x_vf_set_adpt_promisc(vf, adpt, ucast_m);
	else
		ucast_err = ne6x_vf_clear_adpt_promisc(vf, adpt, ucast_m);

	if (allmulti)
		mcast_err = ne6x_vf_set_adpt_promisc(vf, adpt, mcast_m);
	else
		mcast_err = ne6x_vf_clear_adpt_promisc(vf, adpt, mcast_m);

	if (!mcast_err) {
		if (allmulti && !test_and_set_bit(NE6X_VF_STATE_MC_PROMISC, vf->vf_states))
			dev_info(dev, "VF %u successfully set multicast promiscuous mode\n",
				 vf->vf_id);
		else if (!allmulti && test_and_clear_bit(NE6X_VF_STATE_MC_PROMISC, vf->vf_states))
			dev_info(dev, "VF %u successfully unset multicast promiscuous mode\n",
				 vf->vf_id);
	}

	if (!ucast_err) {
		if (alluni && !test_and_set_bit(NE6X_VF_STATE_UC_PROMISC, vf->vf_states))
			dev_info(dev, "VF %u successfully set unicast promiscuous mode\n",
				 vf->vf_id);
		else if (!alluni && test_and_clear_bit(NE6X_VF_STATE_UC_PROMISC, vf->vf_states))
			dev_info(dev, "VF %u successfully unset unicast promiscuous mode\n",
				 vf->vf_id);
	}

error_param:
	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_CONFIG_PROMISCUOUS_MODE, v_ret, NULL, 0);
}

static bool ne6x_is_vf_link_up(struct ne6x_vf *vf)
{
	struct ne6x_port_info *pi = ne6x_vf_get_port_info(vf);
	struct ne6x_pf *pf = vf->pf;

	if (ne6x_check_vf_init(pf, vf))
		return false;

	if (vf->link_forced)
		return vf->link_up;
	else
		return pi->phy.link_info.link_info & NE6X_AQ_LINK_UP;
}

u32 ne6x_conv_link_speed_to_virtchnl(bool adv_link_support, u16 link_speed)
{
	u32 speed;

	switch (link_speed) {
	case NE6X_LINK_SPEED_10GB:
		speed = NE6X_LINK_SPEED_10GB;
		break;
	case NE6X_LINK_SPEED_25GB:
		speed = NE6X_LINK_SPEED_25GB;
		break;
	case NE6X_LINK_SPEED_40GB:
		speed = NE6X_LINK_SPEED_40GB;
		break;
	case NE6X_LINK_SPEED_100GB:
		speed = NE6X_LINK_SPEED_100GB;
		break;
	default:
		speed = NE6X_LINK_SPEED_UNKNOWN;
		break;
	}

	return speed;
}

static void ne6x_set_pfe_link(struct ne6x_vf *vf, struct virtchnl_pf_event *pfe,
			      int ne6x_link_speed, bool link_up)
{
	pfe->link_status = link_up;
	/* Speed in Mbps */
	if (link_up && vf->link_forced)
		ne6x_link_speed = NE6X_LINK_SPEED_25GB;

	pfe->link_speed = ne6x_conv_link_speed_to_virtchnl(true, ne6x_link_speed);
}

void ne6x_vc_notify_vf_link_state(struct ne6x_vf *vf)
{
	struct virtchnl_pf_event pfe = {0};
	struct ne6x_hw *hw = &vf->pf->hw;
	struct ne6x_port_info *pi;
	u8 data[6] = {0};

	pi = ne6x_vf_get_port_info(vf);
	pfe.event = VIRTCHNL_EVENT_LINK_CHANGE;

	if (ne6x_is_vf_link_up(vf))
		ne6x_set_pfe_link(vf, &pfe, pi->phy.link_info.link_speed, true);
	else
		ne6x_set_pfe_link(vf, &pfe, NE6X_LINK_SPEED_UNKNOWN, false);

	data[0] = pfe.event;
	data[1] = (pfe.link_speed >> 24) & 0xff;
	data[2] = (pfe.link_speed >> 16) & 0xff;
	data[3] = (pfe.link_speed >> 8) & 0xff;
	data[4] = (pfe.link_speed >> 0) & 0xff;
	data[5] = pfe.link_status;

	ne6x_sdk_send_msg_to_vf(hw, vf->vf_id, VIRTCHNL_OP_EVENT,
				VIRTCHNL_STATUS_SUCCESS,
				(u8 *)data, 6);
}

void ne6x_vc_notify_link_state(struct ne6x_vf *vf)
{
	if (vf->ready_to_link_notify)
		ne6x_vc_notify_vf_link_state(vf);
}

static void ne6x_vc_notify_vf_reset(struct ne6x_vf *vf)
{
	struct virtchnl_pf_event pfe;
	struct ne6x_pf *pf;
	u8 data[6] = {0};

	if (!vf)
		return;

	pf = vf->pf;
	if (ne6x_validate_vf_id(pf, vf->vf_id))
		return;

	/* Bail out if VF is in disabled state, neither initialized, nor active
	 * state - otherwise proceed with notifications
	 */
	if ((!test_bit(NE6X_VF_STATE_INIT, vf->vf_states) &&
	     !test_bit(NE6X_VF_STATE_ACTIVE, vf->vf_states)) ||
	     test_bit(NE6X_VF_STATE_DIS, vf->vf_states))
		return;

	pfe.event = VIRTCHNL_EVENT_RESET_IMPENDING;
	data[0] = pfe.event;
	ne6x_sdk_send_msg_to_vf(&pf->hw, vf->vf_id, VIRTCHNL_OP_EVENT,
				VIRTCHNL_STATUS_SUCCESS,
				(u8 *)data, 1);
}

static void ne6x_vc_notify_vf_trust_change(struct ne6x_vf *vf)
{
	struct virtchnl_vf_config vfconfig = {0};
	struct ne6x_hw *hw = &vf->pf->hw;
	struct ne6x_pf *pf = vf->pf;
	struct device *dev;
	u8 data[6] = {0};

	dev = ne6x_pf_to_dev(pf);
	vfconfig.type = VIRTCHNL_VF_CONFIG_TRUST;
	if (test_bit(NE6X_VF_CONFIG_FLAG_TRUSTED, vf->vf_config_flag))
		vfconfig.data[0] = 1;
	else
		vfconfig.data[0] = 0;

	data[0] = vfconfig.type;
	data[1] = vfconfig.data[0];
	dev_info(dev, "vfconfig_type = %d,data = %d\n", data[0], data[1]);
	ne6x_sdk_send_msg_to_vf(hw, vf->vf_id, VIRTCHNL_OP_VF_CONFIG,
				VIRTCHNL_STATUS_SUCCESS,
				(u8 *)data, 2);
}

bool ne6x_reset_vf(struct ne6x_vf *vf, bool is_vflr)
{
	struct ne6x_adapter *adpt;

	adpt = ne6x_get_vf_adpt(vf);

	if (test_bit(NE6X_VF_STATE_QS_ENA, vf->vf_states))
		ne6x_dis_vf_qs(vf);

	if (test_bit(NE6X_VF_STATE_ACTIVE, vf->vf_states)) {
		clear_bit(NE6X_VF_STATE_ACTIVE, vf->vf_states);
		adpt->port_info->phy.link_info.link_info = 0x0;
		if (is_vflr)
			vf->rx_tx_state = false;
	}

	if (test_bit(NE6X_VF_STATE_UC_PROMISC, vf->vf_states))
		clear_bit(NE6X_VF_STATE_UC_PROMISC, vf->vf_states);

	if (test_bit(NE6X_VF_STATE_MC_PROMISC, vf->vf_states))
		clear_bit(NE6X_VF_STATE_MC_PROMISC, vf->vf_states);

	return 0;
}

static void ne6x_vc_reset_vf(struct ne6x_vf *vf, bool update_tx_rx)
{
	ne6x_vc_notify_vf_reset(vf);
	ne6x_reset_vf(vf, update_tx_rx);
}

static int ne6x_vc_request_qs_msg(struct ne6x_vf *vf, u8 *msg)
{
	union u_ne6x_mbx_snap_buffer_data *usnap = (union u_ne6x_mbx_snap_buffer_data *)msg;
	u16 req_queues = (usnap->snap.data[1] << 8) | usnap->snap.data[0];
	enum virtchnl_status_code v_ret = VIRTCHNL_STATUS_SUCCESS;
	u16 max_avail_vf_qps, max_allowed_vf_qps;
	u8 req_reset = usnap->snap.data[2];
	bool need_update_rx_tx = false;
	struct ne6x_pf *pf = vf->pf;
	u16 tx_rx_queue_left;
	u16 num_queue_pairs;
	struct device *dev;
	u16 cur_queues;

	ne6x_clear_vf_status(vf);
	dev = ne6x_pf_to_dev(pf);

	if (!test_bit(NE6X_VF_STATE_ACTIVE, vf->vf_states)) {
		v_ret = VIRTCHNL_STATUS_ERR_PARAM;
		goto error_param;
	}

	max_allowed_vf_qps = pf->num_qps_per_vf;
	cur_queues = vf->num_vf_qs;
	tx_rx_queue_left = cur_queues;
	max_avail_vf_qps = tx_rx_queue_left + cur_queues;

	if (!req_queues) {
		dev_err(dev, "VF %d tried to request 0 queues. Ignoring.\n", vf->vf_id);
	} else if (req_queues > max_allowed_vf_qps) {
		dev_err(dev, "VF %d tried to request more than %d queues.\n", vf->vf_id,
			max_allowed_vf_qps);
		num_queue_pairs = max_allowed_vf_qps;
	} else if (req_queues > cur_queues && req_queues - cur_queues > tx_rx_queue_left) {
		dev_warn(dev, "VF %d requested %u more queues, but only %u left.\n", vf->vf_id,
			 req_queues - cur_queues, tx_rx_queue_left);
		num_queue_pairs = min_t(u16, max_avail_vf_qps, max_allowed_vf_qps);
	} else {
		if (req_queues != vf->num_req_qs) {
			vf->num_req_qs = req_queues;
			need_update_rx_tx = true;
		}
		if (req_reset) {
			ne6x_vc_reset_vf(vf, need_update_rx_tx);
		} else {
			vf->ready = false;
			if (need_update_rx_tx)
				vf->rx_tx_state = false;

			vf->adpt->port_info->phy.link_info.link_info = 0x0;
			return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_REQUEST_QUEUES,
						     VIRTCHNL_STATUS_SUCCESS, NULL, 0);
		}

		return 0;
	}

error_param:
	/* send the response to the VF */
	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_REQUEST_QUEUES, v_ret, (u8 *)&num_queue_pairs,
				     2);
}

static int ne6x_vc_config_mtu_msg(struct ne6x_vf *vf, u8 *msg)
{
	enum virtchnl_status_code v_ret = VIRTCHNL_STATUS_SUCCESS;
	union u_ne6x_mbx_snap_buffer_data *rsvsnap;
	struct ne6x_adapter *adpt = vf->adpt;
	struct ne6x_pf *pf = vf->pf;
	struct device *dev;
	u16 *mtu;

	rsvsnap = (union u_ne6x_mbx_snap_buffer_data *)msg;
	mtu = (u16 *)(rsvsnap->snap.data);

	dev = ne6x_pf_to_dev(pf);
	dev_info(dev, "%s: mtu = %d\n", __func__, *mtu);
	ne6x_dev_set_mtu(adpt, *mtu);

	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_CONFIG_MTU, v_ret, NULL, 0);
}

struct virtchnl_vlan_info {
	u16 vlan_id;
	s16 flags;
};

static int ne6x_vc_config_vlan_msg(struct ne6x_vf *vf, u8 *msg)
{
	enum virtchnl_status_code v_ret = VIRTCHNL_STATUS_SUCCESS;
	union u_ne6x_mbx_snap_buffer_data *rsvsnap;
	struct virtchnl_vlan_info *dpdk_vlan;
	struct ne6x_adapter *adpt = vf->adpt;
	struct ne6x_pf *pf = vf->pf;
	struct device *dev;
	struct ne6x_vlan vlan;
	int ret;

	dev = ne6x_pf_to_dev(pf);
	rsvsnap = (union u_ne6x_mbx_snap_buffer_data *)msg;
	dpdk_vlan = (struct virtchnl_vlan_info *)rsvsnap->snap.data;
	if (dpdk_vlan->flags) {
		dev_info(dev, "%s: flags = %d vlan id = %d\n", __func__, dpdk_vlan->flags,
			 dpdk_vlan->vlan_id);

		vlan = NE6X_VLAN(ETH_P_8021Q, dpdk_vlan->vlan_id, 0);
		ret = ne6x_adpt_add_vlan(adpt, vlan);
		if (!ret) {
			dev_info(dev, "%s: add vlan id success\n", __func__);
			set_bit(NE6X_ADPT_VLAN_FLTR_CHANGED, adpt->comm.state);
		} else {
			dev_info(dev, "%s: add vlan id failed\n", __func__);
			v_ret = VIRTCHNL_STATUS_ERR_PARAM;
		}
	} else {
		dev_info(dev, "%s: flags = %d vlan id = %d\n", __func__, dpdk_vlan->flags,
			 dpdk_vlan->vlan_id);

		vlan = NE6X_VLAN(ETH_P_8021Q, dpdk_vlan->vlan_id, 0);
		ret = ne6x_adpt_del_vlan(adpt, vlan);
		if (ret) {
			dev_info(dev, "%s: del vlan id failed\n", __func__);
			v_ret = VIRTCHNL_STATUS_ERR_PARAM;
		} else {
			dev_info(dev, "%s: del vlan id success\n", __func__);
			set_bit(NE6X_ADPT_VLAN_FLTR_CHANGED, adpt->comm.state);
		}
	}

	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_CONFIG_VLAN, v_ret, NULL, 0);
}

#define ETH_VLAN_STRIP_MASK        0x0001
#define ETH_VLAN_FILTER_MASK       0x0002
#define ETH_QINQ_STRIP_MASK        0x0008
#define DEV_RX_OFFLOAD_VLAN_STRIP  0x00000001
#define DEV_RX_OFFLOAD_QINQ_STRIP  0x00000020
#define DEV_RX_OFFLOAD_VLAN_FILTER 0x00000200

struct virtchnl_vlan_offload_info {
	u16 mask;
	u16 feature;
};

static int ne6x_vc_config_vlan_offload_msg(struct ne6x_vf *vf, u8 *msg)
{
	enum virtchnl_status_code v_ret = VIRTCHNL_STATUS_SUCCESS;
	struct virtchnl_vlan_offload_info *offload;
	union u_ne6x_mbx_snap_buffer_data *rsvsnap;
	struct ne6x_adapter *adpt = vf->adpt;
	struct ne6x_pf *pf = vf->pf;
	struct device *dev;

	dev = ne6x_pf_to_dev(pf);
	adpt->hw_feature = ne6x_dev_get_features(adpt);
	rsvsnap = (union u_ne6x_mbx_snap_buffer_data *)msg;
	offload = (struct virtchnl_vlan_offload_info *)rsvsnap->snap.data;

	if (offload->mask & ETH_VLAN_FILTER_MASK) {
		dev_info(dev, "%s: ETH_VLAN_FILTER_MASK\n", __func__);
		if (offload->feature & DEV_RX_OFFLOAD_VLAN_FILTER) {
			dev_info(dev, "%s: ETH_VLAN_FILTER ON\n", __func__);
			adpt->hw_feature |= (NE6X_F_RX_VLAN_FILTER);
		} else {
			dev_info(dev, "%s: ETH_VLAN_FILTER OFF\n", __func__);
			adpt->hw_feature &= ~(NE6X_F_RX_VLAN_FILTER);
		}
	}

	if (offload->mask & ETH_VLAN_STRIP_MASK) {
		dev_info(dev, "%s: ETH_VLAN_STRIP_MASK\n", __func__);
		if (offload->feature & DEV_RX_OFFLOAD_VLAN_STRIP) {
			dev_info(dev, "%s: ETH_VLAN_STRIP ON\n", __func__);
			adpt->hw_feature |= NE6X_F_RX_VLAN_STRIP;
		} else {
			dev_info(dev, "%s: ETH_VLAN_STRIP OFF\n", __func__);
			adpt->hw_feature &= ~NE6X_F_RX_VLAN_STRIP;
		}
	}

	if (offload->mask & ETH_QINQ_STRIP_MASK) {
		dev_info(dev, "%s: ETH_QINQ_STRIP_MASK\n", __func__);
		if (offload->feature & DEV_RX_OFFLOAD_QINQ_STRIP) {
			dev_info(dev, "%s: ETH_QINQ_STRIP ON\n", __func__);
			adpt->hw_feature |= NE6X_F_RX_QINQ_STRIP;
		} else {
			dev_info(dev, "%s: ETH_QINQ_STRIP OFF\n", __func__);
			adpt->hw_feature &= ~NE6X_F_RX_QINQ_STRIP;
		}
	}

	ne6x_dev_set_features(adpt, adpt->hw_feature);

	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_CONFIG_VLAN_OFFLOAD, v_ret, NULL, 0);
}

struct virtchnl_flow_ctrl_info {
	u16 mode;
	u16 high_water;
};

enum rte_eth_fc_mode {
	RTE_FC_NONE = 0, /**< Disable flow control. */
	RTE_FC_RX_PAUSE, /**< RX pause frame, enable flowctrl on TX side. */
	RTE_FC_TX_PAUSE, /**< TX pause frame, enable flowctrl on RX side. */
	RTE_FC_FULL      /**< Enable flow control on both side. */
};

static int ne6x_vc_config_flow_ctrl_msg(struct ne6x_vf *vf, u8 *msg)
{
	enum virtchnl_status_code v_ret = VIRTCHNL_STATUS_SUCCESS;
	union u_ne6x_mbx_snap_buffer_data *rsvsnap;
	struct virtchnl_flow_ctrl_info *flow;
	struct ne6x_adapter *adpt = vf->adpt;
	struct ne6x_flowctrl flowctrl;
	struct ne6x_pf *pf = vf->pf;
	struct device *dev;
	int ret;

	dev = ne6x_pf_to_dev(pf);
	rsvsnap = (union u_ne6x_mbx_snap_buffer_data *)msg;
	flow = (struct virtchnl_flow_ctrl_info *)rsvsnap->snap.data;
	if (flow->mode == RTE_FC_FULL) {
		flowctrl.rx_pause = 1;
		flowctrl.tx_pause = 1;
	} else if (flow->mode == RTE_FC_RX_PAUSE) {
		flowctrl.rx_pause = 1;
	} else if (flow->mode == RTE_FC_TX_PAUSE) {
		flowctrl.tx_pause = 1;
	} else {
		flowctrl.rx_pause = 0;
		flowctrl.tx_pause = 0;
	}

	dev_info(dev, "%s: mode = %d high water = %d\n", __func__, flow->mode, flow->high_water);
	ret = ne6x_dev_set_flowctrl(adpt, &flowctrl);
	if (ret) {
		dev_info(dev, "%s: set flow ctrl failed\n", __func__);
		v_ret = VIRTCHNL_STATUS_ERR_NOT_SUPPORTED;
	}

	ret = ne6x_dev_set_vf_bw(adpt, flow->high_water);
	if (ret)
		v_ret = VIRTCHNL_STATUS_ERR_PARAM;

	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_CONFIG_FLOW_CTRL, v_ret, NULL, 0);
}

static int ne6x_vc_config_rss_msg(struct ne6x_vf *vf, u8 *msg)
{
	enum virtchnl_status_code v_ret = VIRTCHNL_STATUS_SUCCESS;
	union u_ne6x_mbx_snap_buffer_data *rsvsnap;
	struct ne6x_adapter *adpt = vf->adpt;
	u8 *data = (u8 *)&adpt->rss_info;
	int i;

	rsvsnap = (union u_ne6x_mbx_snap_buffer_data *)msg;

	for (i = 0; i < rsvsnap->snap.len; i++) {
		data[adpt->rss_size] = rsvsnap->snap.data[i];
		adpt->rss_size++;
	}

	if (adpt->rss_size >= sizeof(struct ne6x_rss_info)) {
		adpt->rss_size = 0;
		ne6x_dev_set_rss(adpt, &adpt->rss_info);
	}

	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_CONFIG_RSS, v_ret, NULL, 0);
}

static int ne6x_vc_changed_rss_msg(struct ne6x_vf *vf, u8 *msg)
{
	union u_ne6x_mbx_snap_buffer_data *rsvsnap;
	struct ne6x_adapter *adpt = vf->adpt;
	int i, ret;

	rsvsnap = (union u_ne6x_mbx_snap_buffer_data *)msg;
	memcpy(&adpt->num_queue, rsvsnap->snap.data, sizeof(adpt->num_queue));

	if (adpt->rss_info.ind_table_size > NE6X_RSS_MAX_IND_TABLE_SIZE)
		adpt->rss_info.ind_table_size = NE6X_RSS_MAX_IND_TABLE_SIZE;

	for (i = 0; i < adpt->rss_info.ind_table_size; i++)
		adpt->rss_info.ind_table[i] = ethtool_rxfh_indir_default(i, adpt->num_queue);

	ret = ne6x_dev_set_rss(adpt, &adpt->rss_info);
	ret |= ne6x_dev_add_unicast_for_fastmode(adpt, vf->dev_lan_addr.addr);
	ret |= ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_CHANGED_RSS,
				      VIRTCHNL_STATUS_SUCCESS, NULL, 0);

	return ret;
}

static int ne6x_vc_add_vlan_msg(struct ne6x_vf *vf, u8 *msg)
{
	struct ne6x_vlan vlan;
	u16 vlan_tpid = 0;
	u16 vlan_id = 0;

	vlan_id = *((u16 *)msg);
	vlan_tpid = *((u16 *)(msg + 2));
	dev_info(&vf->pf->pdev->dev, "%s:vlan tpid:%04x,vlan id:%04x\n",
		 __func__, vlan_tpid, vlan_id);

	vlan = NE6X_VLAN(vlan_tpid, vlan_id, 0);

	dev_info(&vf->pf->pdev->dev, "%s:vfp_vid %04x\n", __func__, vf->vfp_vid);

	ne6x_adpt_add_vlan(vf->adpt, vlan);

	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_ADD_VLAN, VIRTCHNL_STATUS_SUCCESS, NULL, 0);
}

static int ne6x_vc_del_vlan_msg(struct ne6x_vf *vf, u8 *msg)
{
	struct ne6x_vlan vlan;
	u16 vlan_tpid = 0;
	u16 vlan_id = 0;

	vlan_id = *((u16 *)msg);
	vlan_tpid = *((u16 *)(msg + 2));

	dev_info(&vf->pf->pdev->dev, "%s:vlan tpid:%04x,vlan id:%04x\n", __func__, vlan_tpid,
		 vlan_id);
	vlan = NE6X_VLAN(vlan_tpid, vlan_id, 0);

	ne6x_adpt_del_vlan(vf->adpt, vlan);

	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_DEL_VLAN, VIRTCHNL_STATUS_SUCCESS, NULL, 0);
}

static int ne6x_vc_config_offload_msg(struct ne6x_vf *vf, u8 *msg)
{
	union u_ne6x_mbx_snap_buffer_data *rsvsnap = (union u_ne6x_mbx_snap_buffer_data *)msg;
	struct ne6x_adapter *adpt = vf->adpt;

	adpt->hw_feature = rsvsnap->snap.data[3];
	adpt->hw_feature = adpt->hw_feature << 8;
	adpt->hw_feature |= rsvsnap->snap.data[2];
	adpt->hw_feature = adpt->hw_feature << 8;
	adpt->hw_feature |= rsvsnap->snap.data[1];
	adpt->hw_feature = adpt->hw_feature << 8;
	adpt->hw_feature |= rsvsnap->snap.data[0];

	if (vf->tx_rate)
		adpt->hw_feature |= NE6X_F_TX_QOSBANDWIDTH;
	else
		adpt->hw_feature &= ~NE6X_F_TX_QOSBANDWIDTH;

	ne6x_dev_set_features(adpt, adpt->hw_feature);

	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_CONFIG_OFFLOAD, VIRTCHNL_STATUS_SUCCESS, NULL,
				     0);
}

static int ne6x_vc_request_feature_msg(struct ne6x_vf *vf, u8 *msg)
{
	struct ne6x_adapter *adpt = vf->adpt;

	adpt->hw_feature = ne6x_dev_get_features(adpt);
	dev_info(&vf->pf->pdev->dev, "%s: vf->vf_id =%d vport = %d lport = %d pport = %d hw_queue_base = %d hw_feature = %08X\n",
		 __func__, vf->vf_id, adpt->vport, adpt->port_info->lport,
		 adpt->port_info->hw_port_id, adpt->port_info->hw_queue_base, adpt->hw_feature);

	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_GET_VF_FEATURE, VIRTCHNL_STATUS_SUCCESS,
				     (u8 *)&adpt->hw_feature, sizeof(u32));
}

static int ne6x_vc_reset_vf_msg(struct ne6x_vf *vf, u8 *msg)
{
	union u_ne6x_mbx_snap_buffer_data *rsvsnap;
	struct virtchnl_ether_addr vc_ether_addr;

	vf->ready = false;
	vf->rx_tx_state = 0;
	vf->adpt->port_info->phy.link_info.link_info = false;

	rsvsnap = (union u_ne6x_mbx_snap_buffer_data *)msg;
	vc_ether_addr.addr[0] = rsvsnap->snap.data[0];
	vc_ether_addr.addr[1] = rsvsnap->snap.data[1];
	vc_ether_addr.addr[2] = rsvsnap->snap.data[2];
	vc_ether_addr.addr[3] = rsvsnap->snap.data[3];
	vc_ether_addr.addr[4] = rsvsnap->snap.data[4];
	vc_ether_addr.addr[5] = rsvsnap->snap.data[5];

	ne6x_dev_set_features(vf->adpt, 0);
	ne6x_dev_del_vf_qinq(vf, 0, 0);

	vf->port_vlan_info = NE6X_VLAN(0, 0, 0);
	vf->link_forced = false;
	vf->trusted = false;
	vf->tx_rate = 0;
	clear_bit(NE6X_VF_CONFIG_FLAG_TRUSTED, vf->vf_config_flag);
	ne6x_dev_del_broadcast_leaf(ne6x_get_vf_adpt(vf));
	ne6x_adpt_clear_mac_vlan(ne6x_get_vf_adpt(vf));

	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_RESET_VF, VIRTCHNL_STATUS_SUCCESS, NULL, 0);
}

static int ne6x_get_logic_vf_id(struct net_device *netdev, int vf_id)
{
	struct ne6x_netdev_priv *np = netdev_priv(netdev);
	struct ne6x_adapter *adpt = np->adpt;
	struct ne6x_pf *pf = adpt->back;

	return (adpt->idx * (pf->num_alloc_vfs / pf->num_alloc_adpt) + vf_id);
}

int ne6x_set_vf_trust(struct net_device *netdev, int vf_id, bool trusted)
{
	struct ne6x_pf *pf = ne6x_netdev_to_pf(netdev);
	struct ne6x_vf *vf;
	int logic_vf_id;
	int ret = 0;

	ret = ne6x_validate_outer_vf_id(pf, vf_id);
	if (ret)
		return ret;

	logic_vf_id = ne6x_get_logic_vf_id(netdev, vf_id);

	if (logic_vf_id >= pf->num_alloc_vfs)
		return -EINVAL;

	vf = ne6x_get_vf_by_id(pf, logic_vf_id);

	netdev_info(netdev, "set vf-%d trust %s\n", vf_id, trusted ? "on" : "off");

	if (!vf) {
		netdev_err(netdev, "vf is NULL\n");
		return -EINVAL;
	}

	/* Check if already ready ?*/
	if (!vf->ready) {
		netdev_err(netdev, "vf is not ready\n");
		return (-1);
	}

	/* Check if already trusted */
	if (trusted == vf->trusted)
		return 0;

	vf->trusted = trusted;

	if (vf->trusted) {
		set_bit(NE6X_VF_CONFIG_FLAG_TRUSTED, vf->vf_config_flag);
	} else {
		clear_bit(NE6X_VF_CONFIG_FLAG_TRUSTED, vf->vf_config_flag);
		ne6x_vf_clear_adpt_promisc(vf, ne6x_get_vf_adpt(vf),
					   NE6X_UCAST_PROMISC_BITS |
					   NE6X_MCAST_PROMISC_BITS);
	}

	ne6x_vc_notify_vf_trust_change(vf);
	dev_info(ne6x_pf_to_dev(pf), "VF %u is now %strusted\n",
		 logic_vf_id, trusted ? "" : "un");

	return 0;
}

int ne6x_set_vf_link_state(struct net_device *netdev, int vf_id, int link_state)
{
	struct ne6x_pf *pf = ne6x_netdev_to_pf(netdev);
	int ret = 0, logic_vf_id;
	struct ne6x_vf *vf;

	ret = ne6x_validate_outer_vf_id(pf, vf_id);
	if (ret)
		return ret;

	logic_vf_id = ne6x_get_logic_vf_id(netdev, vf_id);

	vf = ne6x_get_vf_by_id(pf, logic_vf_id);
	if (!vf)
		return -EINVAL;

	netdev_info(netdev, "set vf-%d link state %s\n", vf_id,
		    link_state == IFLA_VF_LINK_STATE_ENABLE
			    ? "enable"
			    : (link_state == IFLA_VF_LINK_STATE_DISABLE ? "disable" : "auto"));

	/* Check if already ready ?*/
	if (!vf->ready)
		return (-1);

	if (!vf->trusted)
		return (-1);

	switch (link_state) {
	case IFLA_VF_LINK_STATE_AUTO:
		vf->link_forced = false;
		break;
	case IFLA_VF_LINK_STATE_ENABLE:
		vf->link_forced = true;
		vf->link_up = true;
		break;
	case IFLA_VF_LINK_STATE_DISABLE:
		vf->link_forced = true;
		vf->link_up = false;
		break;
	default:
		ret = -EINVAL;
		goto out_put_vf;
	}

	ne6x_vc_notify_vf_link_state(vf);

out_put_vf:
	return ret;
}

static int ne6x_vc_modify_vf_mac(struct ne6x_vf *vf, u8 *msg)
{
	enum virtchnl_status_code v_ret = VIRTCHNL_STATUS_SUCCESS;
	union u_ne6x_mbx_snap_buffer_data *rsvsnap;
	struct virtchnl_ether_addr vc_ether_addr;
	struct ne6x_pf *pf = vf->pf;
	struct ne6x_adapter *pf_adpt;

	if (ne6x_check_vf_init(pf, vf)) {
		v_ret = VIRTCHNL_STATUS_ERR_PARAM;
		goto err;
	}

	rsvsnap = (union u_ne6x_mbx_snap_buffer_data *)msg;
	vc_ether_addr.addr[0] = rsvsnap->snap.data[0];
	vc_ether_addr.addr[1] = rsvsnap->snap.data[1];
	vc_ether_addr.addr[2] = rsvsnap->snap.data[2];
	vc_ether_addr.addr[3] = rsvsnap->snap.data[3];
	vc_ether_addr.addr[4] = rsvsnap->snap.data[4];
	vc_ether_addr.addr[5] = rsvsnap->snap.data[5];

	pf_adpt = vf->adpt;
	if (!pf->adpt)
		dev_info(ne6x_pf_to_dev(pf), "adpt is null vf %d\n", vf->vf_id);

	/* set zero addr mean clear mac */
	if (is_zero_ether_addr(vc_ether_addr.addr))
		return ne6x_vc_del_def_mac_addr(vf, pf_adpt, vf->dev_lan_addr.addr);

	if (is_valid_ether_addr(vf->dev_lan_addr.addr)) {
		ne6x_vc_del_def_mac_addr(vf, pf_adpt, vf->dev_lan_addr.addr);
		memset(vf->dev_lan_addr.addr, 0, 6);
	}

	ne6x_vc_add_def_mac_addr(vf, pf_adpt, &vc_ether_addr);

err:
	/* send the response back to the VF */
	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_SET_VF_ADDR, v_ret, vc_ether_addr.addr, 6);
}

static int ne6x_vc_set_fast_mode(struct ne6x_vf *vf, u8 *msg)
{
	enum virtchnl_status_code v_ret = VIRTCHNL_STATUS_SUCCESS;
	union u_ne6x_mbx_snap_buffer_data *rsvsnap;
	struct ne6x_pf *pf = vf->pf;

	rsvsnap = (union u_ne6x_mbx_snap_buffer_data *)msg;

	if (rsvsnap->snap.data[0]) {
		vf->adpt->num_queue = rsvsnap->snap.data[1];
		v_ret = ne6x_dev_set_fast_mode(pf, true, vf->adpt->num_queue);
	} else {
		v_ret = ne6x_dev_set_fast_mode(pf, false, 0);
	}

	/* send the response back to the VF */
	return ne6x_vc_send_msg_to_vf(vf, VIRTCHNL_OP_SET_FAST_MDOE, v_ret, NULL, 0);
}

void ne6x_vc_process_vf_msg(struct ne6x_pf *pf)
{
	union u_ne6x_mbx_snap_buffer_data usnap;
	struct ne6x_hw *hw = &pf->hw;
	struct ne6x_vf *vf = NULL;
	struct ne6x_vlan vlan;
	struct device *dev;
	int err = 0;
	int i;

	dev = ne6x_pf_to_dev(pf);
	ne6x_for_each_vf(pf, i) {
		if (pf->hw.mbx_snapshot.mbx_vf.vf_cntr[i]) {
			vf = &pf->vf[i];
			usnap.val = rd64_bar4(hw, NE6X_VF_MAILBOX_ADDR(vf->base_queue));
			WARN(usnap.snap.len > 6, ">>>>>>>>>>>>>>>>>>recv VF mailbox error!!!<<<<<<<<<<<<<<<<<<<");
			switch (usnap.snap.type) {
			case VIRTCHNL_OP_GET_VF_RESOURCES:
				err = ne6x_vc_get_vf_res_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_CONFIG_TX_QUEUE:
			case VIRTCHNL_OP_CONFIG_RX_QUEUE:
				err = ne6x_vc_send_msg_to_vf(vf, usnap.snap.type,
							     VIRTCHNL_STATUS_SUCCESS,
							     NULL, 0);
				break;
			case VIRTCHNL_OP_ENABLE_QUEUES:
				err = ne6x_vc_send_msg_to_vf(vf, usnap.snap.type,
							     VIRTCHNL_STATUS_SUCCESS,
							     NULL, 0);
				vf->ready = 1;
				break;
			case VIRTCHNL_OP_ADD_ETH_ADDR:
				err = ne6x_vc_add_mac_addr_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_DEL_ETH_ADDR:
				err = ne6x_vc_del_mac_addr_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_ADD_VLAN:
				err = ne6x_vc_add_vlan_msg(vf, (u8 *)&usnap.snap.data);
				break;
			case VIRTCHNL_OP_DEL_VLAN:
				err = ne6x_vc_del_vlan_msg(vf, (u8 *)&usnap.snap.data);
				break;
			case VIRTCHNL_OP_CONFIG_PROMISCUOUS_MODE:
				ne6x_vc_cfg_promiscuous_mode_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_EVENT:
				err = ne6x_vc_send_msg_to_vf(vf, usnap.snap.type,
							     VIRTCHNL_STATUS_SUCCESS,
							     NULL, 0);
				break;
			case VIRTCHNL_OP_REQUEST_QUEUES:
				err = ne6x_vc_request_qs_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_CONFIG_RSS:
				err = ne6x_vc_config_rss_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_CONFIG_VLAN:
				err = ne6x_vc_config_vlan_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_CONFIG_VLAN_OFFLOAD:
				err = ne6x_vc_config_vlan_offload_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_CONFIG_MTU:
				err = ne6x_vc_config_mtu_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_CONFIG_FLOW_CTRL:
				err = ne6x_vc_config_flow_ctrl_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_CHANGED_RSS:
				err = ne6x_vc_changed_rss_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_CONFIG_OFFLOAD:
				err = ne6x_vc_config_offload_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_GET_VF_FEATURE:
				err = ne6x_vc_request_feature_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_RESET_VF:
				err = ne6x_vc_reset_vf_msg(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_GET_PORT_STATUS:
				ne6x_dev_add_broadcast_leaf(ne6x_get_vf_adpt(vf));
				vlan = NE6X_VLAN(ETH_P_8021Q, 0xfff, 0);
				ne6x_adpt_add_vlan(ne6x_get_vf_adpt(vf), vlan);
				ne6x_vc_notify_vf_link_state(vf);

				if (!vf->ready_to_link_notify)
					vf->ready_to_link_notify = 1;

				ne6x_linkscan_schedule(pf);
				break;
			case VIRTCHNL_OP_SET_VF_ADDR:
				err = ne6x_vc_modify_vf_mac(vf, (u8 *)&usnap);
				break;
			case VIRTCHNL_OP_SET_FAST_MDOE:
				err = ne6x_vc_set_fast_mode(vf, (u8 *)&usnap);
				break;
			/* VIRTCHNL_OP_VERSION not used */
			default:
				dev_err(dev, "Unsupported opcode %s from VF %d\n",
					ne6x_opcode_str(usnap.snap.type), i);
				err = ne6x_vc_send_msg_to_vf(vf, usnap.snap.type,
							     VIRTCHNL_STATUS_ERR_NOT_SUPPORTED,
							     NULL, 0);
				break;
			}
			pf->hw.mbx_snapshot.mbx_vf.vf_cntr[i] = false;
		}
		if (err)
			/* Helper function cares less about error return values here
			 * as it is busy with pending work.
			 */
			dev_info(dev, "PF failed to honor VF %d, opcode %d, error %d\n", i,
				 usnap.snap.type, err);
	}

	if (test_bit(NE6X_MAILBOXQ_EVENT_PENDING, pf->state))
		clear_bit(NE6X_MAILBOXQ_EVENT_PENDING, pf->state);
}

int ne6x_get_vf_config(struct net_device *netdev, int vf_id,
		       struct ifla_vf_info *ivi)
{
	struct ne6x_netdev_priv *np = netdev_priv(netdev);
	struct ne6x_adapter *adpt = np->adpt;
	struct ne6x_pf *pf = adpt->back;
	struct ne6x_vf *vf;
	int logic_vfid = 0;
	int ret = 0;

	/* validate the request */
	ret = ne6x_validate_outer_vf_id(pf, vf_id);
	if (ret)
		goto error_param;

	logic_vfid = ne6x_get_logic_vf_id(netdev, vf_id);
	vf = &pf->vf[logic_vfid];
	/* first adpt is always the LAN adpt */
	adpt = pf->adpt[vf->lan_adpt_idx];
	if (!adpt) {
		ret = -ENOENT;
		goto error_param;
	}

	ivi->vf = vf_id;

	ether_addr_copy(ivi->mac, vf->dev_lan_addr.addr);

	ivi->vlan = vf->port_vlan_info.vid;
	ivi->qos = vf->port_vlan_info.prio;
	if (vf->port_vlan_info.vid)
		ivi->vlan_proto = cpu_to_be16(vf->port_vlan_info.tpid);

	if (!vf->link_forced)
		ivi->linkstate = IFLA_VF_LINK_STATE_AUTO;
	else if (vf->link_up)
		ivi->linkstate = IFLA_VF_LINK_STATE_ENABLE;
	else
		ivi->linkstate = IFLA_VF_LINK_STATE_DISABLE;

	ivi->max_tx_rate = vf->tx_rate;
	ivi->min_tx_rate = 0;
	if (test_bit(NE6X_VF_CONFIG_FLAG_TRUSTED, vf->vf_config_flag))
		ivi->trusted = 1;
	else
		ivi->trusted = 0;

error_param:
	return ret;
}

static void ne6x_calc_token_for_bw(int max_tx_rate, int *time_inv, int *tocken)
{
	if (max_tx_rate <= 100) {
		*time_inv = 3910;
		*tocken = max_tx_rate;
	} else if (max_tx_rate <= 1000) {
		*time_inv = 790;
		*tocken = max_tx_rate / 5;
	} else if (max_tx_rate < 5000) {
		*time_inv = 395;
		*tocken = max_tx_rate / 10;
	} else if (max_tx_rate < 10000) {
		*time_inv = 118;
		*tocken = max_tx_rate / 33;
	} else {
		*time_inv = 39;
		*tocken = max_tx_rate / 100;
	}
}

int ne6x_set_vf_bw_for_max_vpnum(struct ne6x_pf *pf, int vf_id, int max_tx_rate)
{
	union ne6x_sq_meter_cfg0 sq_meter_cfg0;
	union ne6x_sq_meter_cfg1 sq_meter_cfg1;
	union ne6x_sq_meter_cfg2 sq_meter_cfg2;
	union ne6x_sq_meter_cfg3 sq_meter_cfg3;
	struct ne6x_hw *hw = &pf->hw;
	int time_inv = 0;
	int tocken = 0;

	sq_meter_cfg3.val = rd64(hw, NE6X_VPINT_DYN_CTLN(vf_id, NE6X_SQ_METER_CFG3));
	sq_meter_cfg3.reg.csr_meter_pause_threshold_vp = 1;
	wr64(hw, NE6X_VPINT_DYN_CTLN(vf_id, NE6X_SQ_METER_CFG3), sq_meter_cfg3.val);
	sq_meter_cfg2.val = rd64(hw, NE6X_VPINT_DYN_CTLN(vf_id, NE6X_SQ_METER_CFG2));
	sq_meter_cfg2.reg.csr_meter_resume_threshold_vp = 1;
	wr64(hw, NE6X_VPINT_DYN_CTLN(vf_id, NE6X_SQ_METER_CFG2), sq_meter_cfg2.val);

	sq_meter_cfg1.val = rd64(hw, NE6X_VPINT_DYN_CTLN(vf_id, NE6X_SQ_METER_CFG1));
	sq_meter_cfg1.reg.csr_meter_refresh_count_vp = max_tx_rate;

	if (max_tx_rate) {
		ne6x_calc_token_for_bw(max_tx_rate, &time_inv, &tocken);
		sq_meter_cfg1.reg.csr_meter_refresh_count_vp = tocken;
		sq_meter_cfg1.reg.csr_meter_refresh_interval_vp = time_inv;
	} else {
		sq_meter_cfg1.reg.csr_meter_refresh_count_vp = 0x1;
		sq_meter_cfg1.reg.csr_meter_refresh_interval_vp = 0x1;
	}

	wr64(hw, NE6X_VPINT_DYN_CTLN(vf_id, NE6X_SQ_METER_CFG1), sq_meter_cfg1.val);
	sq_meter_cfg0.val = rd64(hw, NE6X_VPINT_DYN_CTLN(vf_id, NE6X_SQ_METER_CFG0));
	sq_meter_cfg0.reg.csr_meter_pkt_token_num_vp = 0x1;
	sq_meter_cfg0.reg.csr_meter_ipg_len_vp = 0x0;
	sq_meter_cfg0.reg.csr_meter_refresh_en_vp = 0x1;
	sq_meter_cfg0.reg.csr_meter_packet_mode_vp = 0x0;

	if (max_tx_rate) {
		sq_meter_cfg0.reg.csr_meter_rate_limit_en_vp = 0x1;
		sq_meter_cfg0.reg.csr_meter_refresh_en_vp = 0x1;
	} else {
		sq_meter_cfg0.reg.csr_meter_rate_limit_en_vp = 0x0;
		sq_meter_cfg0.reg.csr_meter_refresh_en_vp = 0x0;
	}

	wr64(hw, NE6X_VPINT_DYN_CTLN(vf_id, NE6X_SQ_METER_CFG0), sq_meter_cfg0.val);

	return 0;
}

void ne6x_clr_vf_bw_for_max_vpnum(struct ne6x_pf *pf)
{
	int index;

	for (index = 0; index < NE6X_MAX_VP_NUM; index++)
		ne6x_set_vf_bw_for_max_vpnum(pf, index, 0);
}

int ne6x_ndo_set_vf_bw(struct net_device *netdev, int vf_id, int min_tx_rate, int max_tx_rate)
{
	struct ne6x_netdev_priv *np = netdev_priv(netdev);
	struct ne6x_pf *pf = np->adpt->back;
	struct ne6x_adapter *adpt;
	struct ne6x_vf *vf;
	int logic_vfid;
	int ret;

	/* validate the request */
	ret = ne6x_validate_outer_vf_id(pf, vf_id);
	if (ret)
		goto error;

	logic_vfid = ne6x_get_logic_vf_id(netdev, vf_id);
	vf = &pf->vf[logic_vfid];
	adpt = ne6x_get_vf_adpt(vf);
	if (!adpt) {
		ret = -EINVAL;
		goto error;
	}

	ret = ne6x_validata_tx_rate(adpt, logic_vfid, min_tx_rate, max_tx_rate);
	if (ret) {
		ret = -EINVAL;
		goto error;
	}

	if (!test_bit(NE6X_VF_STATE_INIT, vf->vf_states)) {
		dev_err(&pf->pdev->dev, "VF %d still in reset. Try again.\n", logic_vfid);
		ret = -EAGAIN;
		goto error;
	}

	if (pf->num_alloc_vfs == 64)
		ret = ne6x_set_vf_bw_for_max_vpnum(pf, logic_vfid, max_tx_rate);
	else
		ret = ne6x_dev_set_vf_bw(adpt, max_tx_rate);

	if (ret)
		goto error;

	vf->tx_rate = max_tx_rate;

	return 0;
error:
	return ret;
}

int ne6x_set_vf_mac(struct net_device *netdev, int vf_id, u8 *mac)
{
	struct ne6x_netdev_priv *np = netdev_priv(netdev);
	union u_ne6x_mbx_snap_buffer_data usnap;
	struct ne6x_adapter *adpt = np->adpt;
	struct ne6x_pf *pf = adpt->back;
	struct ne6x_vf *vf;
	int logic_vfid;
	int ret;

	/* validate the request */
	ret = ne6x_validate_outer_vf_id(pf, vf_id);
	if (ret)
		goto error_param;

	logic_vfid = ne6x_get_logic_vf_id(netdev, vf_id);
	vf = &pf->vf[logic_vfid];

	adpt = ne6x_get_vf_adpt(vf);
	if (!is_valid_ether_addr(mac)) {
		dev_err(&pf->pdev->dev, "Invalid Ethernet address %pM for VF %d\n", mac, vf_id);
		ret = -EINVAL;
		goto error_param;
	}

	if (is_multicast_ether_addr(mac)) {
		dev_err(&pf->pdev->dev, "Invalid Ethernet address %pM for VF %d\n", mac, vf_id);
		ret = -EINVAL;
		goto error_param;
	}

	if (ether_addr_equal(vf->dev_lan_addr.addr, mac)) {
		dev_err(&pf->pdev->dev, "already use the same Ethernet address %pM for VF %d\n",
			mac, vf_id);
		goto error_param;
	}

	/*simluate a msg from vf*/
	usnap.snap.type = VIRTCHNL_OP_SET_VF_ADDR;
	usnap.snap.state = VIRTCHNL_STATUS_SUCCESS;
	usnap.snap.len = 6;
	memcpy(usnap.snap.data, mac, usnap.snap.len);
	ret = ne6x_vc_modify_vf_mac(vf, (u8 *)&usnap);

error_param:
	return ret;
}

#endif
