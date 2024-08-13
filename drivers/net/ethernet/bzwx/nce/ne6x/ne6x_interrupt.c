// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include "ne6x.h"
#include "ne6x_interrupt.h"

static int ne6x_init_msix(struct ne6x_pf *pf, int budget)
{
	int actual_vector;
	ssize_t size;

	actual_vector = pci_enable_msix_range(pf->pdev, pf->msix_entries, NE6X_MIN_MSIX, budget);
	dev_info(&pf->pdev->dev, "%s actual_vector = %d\n", __func__, actual_vector);
	if (actual_vector <= 0) {
		kfree(pf->msix_entries);
		pf->msix_entries = NULL;
		pci_disable_msix(pf->pdev);
		dev_err(&pf->pdev->dev, "error msix enable failed\n");
		return -ENODEV;
	}

	size = struct_size(pf->irq_pile, list, actual_vector);
	pf->irq_pile = kzalloc(size, GFP_KERNEL);
	if (!pf->irq_pile) {
		dev_err(&pf->pdev->dev, "error allocating irq_pile memory\n");
		kfree(pf->msix_entries);
		pf->msix_entries = NULL;
		pci_disable_msix(pf->pdev);
		return -ENOMEM;
	}
	pf->irq_pile->num_entries = actual_vector;

	return 0;
}

static int ne6x_init_intx(struct ne6x_pf *pf)
{
	int actual_vector;
	ssize_t size;

	dev_info(&pf->pdev->dev, "try enable intx\n");
	actual_vector = 0x1;

	size = struct_size(pf->irq_pile, list, actual_vector);
	pf->irq_pile = kzalloc(size, GFP_KERNEL);
	if (!pf->irq_pile) {
		dev_err(&pf->pdev->dev, "error intx allocating irq_pile memory\n");
		return -ENOMEM;
	}
	pf->irq_pile->num_entries = actual_vector;

	test_and_set_bit(NE6X_PF_INTX, pf->state);

	return 0;
}

int ne6x_init_interrupt_scheme(struct ne6x_pf *pf)
{
	union ne6x_ciu_time_out_cfg ciu_time_out_cdg;
	union ne6x_all_rq_cfg all_rq_cfg;
	union ne6x_all_sq_cfg all_sq_cfg;
	union ne6x_all_cq_cfg all_cq_cfg;
	union ne6x_merge_cfg merge_cfg;
	struct ne6x_hw *hw = &pf->hw;
	u64 __iomem *reg;
	int err;
	int i;

	pf->msix_entries = kcalloc(NE6X_MAX_MSIX_NUM, sizeof(struct msix_entry), GFP_KERNEL);
	if (!pf->msix_entries)
		return -ENOMEM;

	for (i = 0; i < NE6X_MAX_MSIX_NUM; i++)
		pf->msix_entries[i].entry = i;

	test_and_set_bit(NE6X_PF_MSIX, pf->state);

	if (ne6x_init_msix(pf, NE6X_MAX_MSIX_NUM)) {
		clear_bit(NE6X_PF_MSIX, pf->state);
		err = ne6x_init_intx(pf);
		if (err) {
			dev_err(&pf->pdev->dev, "error intx enable failed\n");
			return err;
		}
	}

	if (pf->irq_pile->num_entries >= NE6X_MAX_MSIX_NUM) {
		err = ne6x_init_link_irq(pf);
		if (err) {
			dev_err(&pf->pdev->dev, "init int irq failed\n");
			return err;
		}
	}

	/* We only initialize int once, so as not to overwrite user settings */
	if (test_and_set_bit(NE6X_INT_INIT_DOWN, pf->state))
		return 0;

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

	return 0;
}

static int ne6x_adpt_alloc_q_vector(struct ne6x_adapter *adpt, int v_idx)
{
	struct ne6x_q_vector *q_vector;

	/* allocate q_vector */
	q_vector = kzalloc(sizeof(*q_vector), GFP_KERNEL);
	if (!q_vector)
		return -ENOMEM;

	q_vector->adpt = adpt;
	q_vector->v_idx = v_idx;

	cpumask_copy(&q_vector->affinity_mask, cpu_possible_mask);

	if (adpt->netdev)
		netif_napi_add(adpt->netdev, &q_vector->napi, ne6x_napi_poll);

	/* tie q_vector and adpt together */
	adpt->q_vectors[v_idx] = q_vector;
	return 0;
}

static void ne6x_free_q_vector(struct ne6x_adapter *adpt, int v_idx)
{
	struct ne6x_q_vector *q_vector = adpt->q_vectors[v_idx];
	struct ne6x_ring *ring;
	struct device *dev;

	dev = ne6x_pf_to_dev(adpt->back);

	if (!q_vector) {
		dev_dbg(dev, "Queue vector at index %d not found\n", v_idx);
		return;
	}

	/* disassociate q_vector from rings */
	ne6x_for_each_ring(ring, q_vector->tx) ring->q_vector = NULL;

	ne6x_for_each_ring(ring, q_vector->rx) ring->q_vector = NULL;

	ne6x_for_each_ring(ring, q_vector->cq) ring->q_vector = NULL;

	/* only adapter w/ an associated netdev is set up w/ NAPI */
	if (adpt->netdev)
		netif_napi_del(&q_vector->napi);

	adpt->q_vectors[v_idx] = NULL;
	kfree(q_vector);
}

static int ne6x_adpt_alloc_q_vectors(struct ne6x_adapter *adpt)
{
	int v_idx, num_q_vectors, err;

	/* if not MSIX, give the one vector only to the LAN adapter */
	num_q_vectors = adpt->num_q_vectors;

	for (v_idx = 0; v_idx < num_q_vectors; v_idx++) {
		err = ne6x_adpt_alloc_q_vector(adpt, v_idx);
		if (err)
			goto err_out;
	}

	return 0;

err_out:
	while (v_idx--)
		ne6x_free_q_vector(adpt, v_idx);

	return err;
}

void ne6x_adpt_free_q_vectors(struct ne6x_adapter *adpt)
{
	int v_idx;

	for (v_idx = 0; v_idx < adpt->num_q_vectors; v_idx++)
		ne6x_free_q_vector(adpt, v_idx);
}

int ne6x_adpt_setup_vectors(struct ne6x_adapter *adpt)
{
	struct ne6x_pf *pf = adpt->back;
	int ret = -ENOENT;

	if (adpt->q_vectors[0]) {
		dev_info(&pf->pdev->dev, "adapter %d has existing q_vectors\n", adpt->idx);
		return -EEXIST;
	}

	if (adpt->base_vector) {
		dev_info(&pf->pdev->dev, "adapter %d has non-zero base vector %d\n", adpt->idx,
			 adpt->base_vector);
		return -EEXIST;
	}

	ret = ne6x_adpt_alloc_q_vectors(adpt);
	if (ret) {
		dev_info(&pf->pdev->dev, "failed to allocate %d q_vector for adapter %d, ret=%d\n",
			 adpt->num_q_vectors, adpt->idx, ret);
		adpt->num_q_vectors = 0;
		goto vector_setup_out;
	}

	if (adpt->num_q_vectors)
		adpt->base_vector = adpt->port_info->hw_queue_base;

	if (adpt->base_vector < 0) {
		dev_info(&pf->pdev->dev, "failed to get tracking for %d vectors for adapter %d, err=%d\n",
			 adpt->num_q_vectors, adpt->idx, adpt->base_vector);
		ne6x_adpt_free_q_vectors(adpt);
		ret = -ENOENT;
		goto vector_setup_out;
	}

vector_setup_out:
	return ret;
}

static void ne6x_irq_affinity_notify(struct irq_affinity_notify *notify, const cpumask_t *mask)
{
	struct ne6x_q_vector *q_vector =
		container_of(notify, struct ne6x_q_vector, affinity_notify);

	cpumask_copy(&q_vector->affinity_mask, mask);
}

static void ne6x_irq_affinity_release(struct kref *ref) {}

int ne6x_adpt_request_irq_msix(struct ne6x_adapter *adpt, char *basename)
{
	int q_vectors = adpt->num_q_vectors;
	struct ne6x_pf *pf = adpt->back;
	int base = adpt->base_vector;
	int rx_int_idx = 0;
	int tx_int_idx = 0;
	int vector, err;
	int irq_num;
	int cpu;

	for (vector = 0; vector < q_vectors; vector++) {
		struct ne6x_q_vector *q_vector = adpt->q_vectors[vector];

		irq_num = pf->msix_entries[base + vector].vector;

		if (q_vector->tx.ring && q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name) - 1, "%s-%s-%d", basename,
				 "TxRx", rx_int_idx++);
			tx_int_idx++;
		} else if (q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name) - 1, "%s-%s-%d", basename,
				 "rx", rx_int_idx++);
		} else if (q_vector->tx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name) - 1, "%s-%s-%d", basename,
				 "tx", tx_int_idx++);
		} else {
			/* skip this unused q_vector */
			continue;
		}

		err = request_irq(irq_num, adpt->irq_handler, 0, q_vector->name, q_vector);
		if (err) {
			dev_info(&pf->pdev->dev, "MSIX request_irq failed, error: %d\n", err);
			goto free_queue_irqs;
		}

		/* register for affinity change notifications */
		q_vector->affinity_notify.notify = ne6x_irq_affinity_notify;
		q_vector->affinity_notify.release = ne6x_irq_affinity_release;
		irq_set_affinity_notifier(irq_num, &q_vector->affinity_notify);

		/* Spread affinity hints out across online CPUs.
		 *
		 * get_cpu_mask returns a static constant mask with
		 * a permanent lifetime so it's ok to pass to
		 * irq_set_affinity_hint without making a copy.
		 */
		cpu = cpumask_local_spread(q_vector->v_idx, -1);
		irq_set_affinity_hint(irq_num, get_cpu_mask(cpu));
	}

	adpt->irqs_ready = true;
	return 0;

free_queue_irqs:
	while (vector) {
		vector--;
		irq_num = pf->msix_entries[base + vector].vector;
		irq_set_affinity_notifier(irq_num, NULL);
		irq_set_affinity_hint(irq_num, NULL);
		free_irq(irq_num, &adpt->q_vectors[vector]);
	}

	return err;
}

static irqreturn_t ne6x_intr(int irq, void *data)
{
	struct ne6x_q_vector *q_vector = data;
	struct ne6x_adapter	*adpt = q_vector->adpt;
	struct ne6x_hw       *hw = &adpt->back->hw;
	u64 reg_val;

	reg_val = rd64(hw, NE6X_VPINT_DYN_CTLN(0, NE6X_VP_INT));
	if (!(reg_val & 0x10000))
		return IRQ_NONE;

	napi_schedule(&q_vector->napi);
	return IRQ_HANDLED;
}

int ne6x_adpt_request_irq_intx(struct ne6x_adapter *adpt, char *basename)
{
	struct ne6x_q_vector *q_vector = adpt->q_vectors[0];
	struct net_device *netdev = adpt->netdev;
	struct ne6x_pf *pf = adpt->back;
	u32 irq = pf->pdev->irq;
	int err;

	snprintf(q_vector->name, sizeof(q_vector->name) - 1, "%s-%s-INTx", basename, "TxRx");

	err = request_irq(irq, &ne6x_intr, IRQF_SHARED, netdev->name, q_vector);
	if (err) {
		dev_info(&pf->pdev->dev, "INTx request_irq failed, error: %d\n", err);
		return err;
	}

	return 0;
}

int ne6x_adpt_request_irq(struct ne6x_adapter *adpt, char *basename)
{
	struct ne6x_pf *pf = adpt->back;
	int err;

	if (test_bit(NE6X_PF_MSIX, pf->state))
		err = ne6x_adpt_request_irq_msix(adpt, basename);
	else
		err = ne6x_adpt_request_irq_intx(adpt, basename);

	if (err)
		dev_info(&pf->pdev->dev, "request_irq failed, Error %d\n", err);

	return err;
}

void ne6x_adpt_configure_msix(struct ne6x_adapter *adpt)
{
	union ne6x_vp_int_mask int_mask;
	struct ne6x_pf *pf = adpt->back;
	struct ne6x_hw *hw = &pf->hw;
	union ne6x_int_cfg int_cfg;
	u32 qp, nextqp;
	int i, q;

	/* The interrupt indexing is offset by 1 in the PFINT_ITRn
	 * and PFINT_LNKLSTn registers, e.g.:
	 *   PFINT_ITRn[0..n-1] gets msix-1..msix-n  (qpair interrupts)
	 */
	qp = adpt->base_queue;

	/* SRIOV mode VF Config OR SRIOV disabled PF Config */
	if (qp < NE6X_PF_VP0_NUM) {
		for (i = 0; i < adpt->num_q_vectors; i++) {
			struct ne6x_q_vector *q_vector = adpt->q_vectors[i];

			for (q = 0; q < q_vector->num_ringpairs; q++) {
				nextqp = qp + i + q;

				int_cfg.val = rd64(hw, NE6X_VPINT_DYN_CTLN(nextqp, NE6X_INT_CFG));
				int_cfg.reg.csr_sq_hdle_half_int_cnt_vp = 0x0;
				int_cfg.reg.csr_rq_hdle_half_int_cnt_vp = 0x0;
				int_cfg.reg.csr_cq_hdle_half_int_cnt_vp = 0xffff;
				wr64(hw, NE6X_VPINT_DYN_CTLN(nextqp, NE6X_INT_CFG), int_cfg.val);

				int_mask.val = rd64(hw,
						    NE6X_VPINT_DYN_CTLN(nextqp, NE6X_VP_INT_MASK));
				int_mask.reg.csr_ciu_mask_vp = NE6X_MAX_U64;
				wr64(hw, NE6X_VPINT_DYN_CTLN(nextqp, NE6X_VP_INT_MASK),
				     int_mask.val);
			}
		}
	} else {
		/* SRIOV mode PF Config */
		for (i = 0; i < adpt->num_q_vectors; i++) {
			struct ne6x_q_vector *q_vector = adpt->q_vectors[i];

			for (q = 0; q < q_vector->num_ringpairs; q++) {
				nextqp = qp - NE6X_PF_VP0_NUM + i + q;

				int_cfg.val = rd64_bar4(hw,
							NE6X_PFINT_DYN_CTLN(nextqp, NE6X_INT_CFG));
				int_cfg.reg.csr_sq_hdle_half_int_cnt_vp = 0x0;
				int_cfg.reg.csr_rq_hdle_half_int_cnt_vp = 0x0;
				int_cfg.reg.csr_cq_hdle_half_int_cnt_vp = 0xffff;
				wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(nextqp, NE6X_INT_CFG),
					  int_cfg.val);

				int_mask.val =
					rd64_bar4(hw,
						  NE6X_PFINT_DYN_CTLN(nextqp,
								      NE6X_VP_INT_MASK));
				int_mask.reg.csr_ciu_mask_vp = NE6X_MAX_U64;
				wr64_bar4(hw,
					  NE6X_PFINT_DYN_CTLN(nextqp, NE6X_VP_INT_MASK),
					  int_mask.val);
			}
		}
	}
}

static inline void ne6x_irq_dynamic_enable(struct ne6x_adapter *adpt, int vector)
{
	union ne6x_vp_int_mask int_mask;
	struct ne6x_pf *pf = adpt->back;
	struct ne6x_hw *hw = &pf->hw;

	if (vector < NE6X_PF_VP0_NUM) {
		int_mask.val = rd64(hw, NE6X_VPINT_DYN_CTLN(vector, NE6X_VP_INT_MASK));
		int_mask.reg.csr_ciu_mask_vp &= ~(1ULL << NE6X_VP_CQ_INTSHIFT);
		wr64(hw, NE6X_VPINT_DYN_CTLN(vector, NE6X_VP_INT_MASK), int_mask.val);
	} else {
		int_mask.val = rd64_bar4(hw,
					 NE6X_PFINT_DYN_CTLN(vector - NE6X_PF_VP0_NUM,
							     NE6X_VP_INT_MASK));
		int_mask.reg.csr_ciu_mask_vp &= ~(1ULL << NE6X_VP_CQ_INTSHIFT);
		wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(vector - NE6X_PF_VP0_NUM,
						  NE6X_VP_INT_MASK),
			  int_mask.val);
	}
}

int ne6x_adpt_enable_irq(struct ne6x_adapter *adpt)
{
	int i;

	for (i = 0; i < adpt->num_q_vectors; i++)
		ne6x_irq_dynamic_enable(adpt, adpt->base_vector + i);

	return 0;
}

void ne6x_adpt_disable_irq(struct ne6x_adapter *adpt)
{
	struct ne6x_pf *pf = adpt->back;
	struct ne6x_hw *hw = &pf->hw;
	int base = adpt->base_vector;
	int i;

	/* disable each interrupt */
	if (base < NE6X_PF_VP0_NUM) {
		for (i = adpt->base_vector; i < (adpt->num_q_vectors + adpt->base_vector); i++) {
			wr64(hw, NE6X_VPINT_DYN_CTLN(i, NE6X_VP_INT), NE6X_MAX_U64);
			wr64(hw, NE6X_VPINT_DYN_CTLN(i, NE6X_VP_INT_MASK), NE6X_MAX_U64);
		}
	} else {
		for (i = adpt->base_vector; i < (adpt->num_q_vectors + adpt->base_vector); i++) {
			wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(i - NE6X_PF_VP0_NUM, NE6X_VP_INT),
				  NE6X_MAX_U64);
			wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(i - NE6X_PF_VP0_NUM, NE6X_VP_INT_MASK),
				  NE6X_MAX_U64);
		}
	}

	if (test_bit(NE6X_PF_MSIX, pf->state)) {
		for (i = 0; i < adpt->num_q_vectors; i++)
			synchronize_irq(pf->msix_entries[i + base].vector);
	} else {
		synchronize_irq(pf->pdev->irq);
	}
}

void ne6x_adpt_free_irq(struct ne6x_adapter *adpt)
{
	struct ne6x_pf *pf = adpt->back;
	int base = adpt->base_vector;
	int i;

	if (!adpt->q_vectors)
		return;

	if (!adpt->irqs_ready)
		return;

	adpt->irqs_ready = false;
	for (i = 0; i < adpt->num_q_vectors; i++) {
		int irq_num;
		u16 vector;

		vector = i + base;
		irq_num = pf->msix_entries[vector].vector;

		/* free only the irqs that were actually requested */
		if (!adpt->q_vectors[i] || !adpt->q_vectors[i]->num_ringpairs)
			continue;

		/* clear the affinity notifier in the IRQ descriptor */
		irq_set_affinity_notifier(irq_num, NULL);

		/* remove our suggested affinity mask for this IRQ */
		irq_set_affinity_hint(irq_num, NULL);

		synchronize_irq(irq_num);
		free_irq(irq_num, adpt->q_vectors[i]);
	}
}

static void ne6x_reset_interrupt_capability(struct ne6x_pf *pf)
{
	/* If we're in Legacy mode, the interrupt was cleaned in adpt_close */
	if (pf->msix_entries) {
		pci_disable_msix(pf->pdev);
		kfree(pf->msix_entries);
		pf->msix_entries = NULL;
	}

	kfree(pf->irq_pile);
	pf->irq_pile = NULL;
}

int ne6x_init_link_irq(struct ne6x_pf *pf)
{
	int irq_num;
	int err;

	snprintf(pf->link_intname, sizeof(pf->link_intname) - 1, "%s-%s-%d",
		 dev_driver_string(&pf->pdev->dev), "link", pf->hw.bus.bus_num);
	irq_num = pf->msix_entries[NE6X_NIC_INT_VP].vector;
	err = request_irq(irq_num, ne6x_linkint_irq_handler, 0, pf->link_intname, pf);
	if (!err)
		pf->link_int_irq_ready = true;

	return 0;
}

int ne6x_enable_link_irq(struct ne6x_pf *pf)
{
	u64 int_mask = 0xffffffffffffffff;
	u64 temp = 1;
	int i = 0;

	if (!pf->link_int_irq_ready)
		return 0;

	for (i = 0; i < pf->hw.pf_port; i++)
		int_mask &= ~(temp << (i + NE6X_NIC_INT_START_BIT));

	wr64_bar4(&pf->hw, NE6X_PFINT_DYN_CTLN(NE6X_NIC_INT_VP - NE6X_PF_VP0_NUM, NE6X_VP_INT_MASK),
		  int_mask);

	return 0;
}

int ne6x_disable_link_irq(struct ne6x_pf *pf)
{
	u64 int_mask = 0xffffffffffffffff;
	u64 int_val;

	wr64_bar4(&pf->hw, NE6X_PFINT_DYN_CTLN(NE6X_NIC_INT_VP - NE6X_PF_VP0_NUM, NE6X_VP_INT_MASK),
		  int_mask);
	int_val = rd64_bar4(&pf->hw,
			    NE6X_PFINT_DYN_CTLN(NE6X_NIC_INT_VP - NE6X_PF_VP0_NUM, NE6X_VP_INT));
	wr64_bar4(&pf->hw, NE6X_PFINT_DYN_CTLN(NE6X_NIC_INT_VP - NE6X_PF_VP0_NUM, NE6X_VP_INT),
		  int_val);

	return 0;
}

void ne6x_free_link_irq(struct ne6x_pf *pf)
{
	if (pf->link_int_irq_ready) {
		synchronize_irq(pf->msix_entries[NE6X_NIC_INT_VP].vector);
		free_irq(pf->msix_entries[NE6X_NIC_INT_VP].vector, pf);
	}

	pf->link_int_irq_ready = false;
}

irqreturn_t ne6x_msix_clean_vf_mbx(int irq, void *data)
{
	struct ne6x_pf *pf = data;
	struct ne6x_hw *hw = &pf->hw;
	bool have_cmd = false;
	struct ne6x_vf *vf;
	u64 int_val = 0;
	u64 val;
	int i;

	val = rd64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DREQ_INT));
	ne6x_for_each_vf(pf, i) {
		vf = &pf->vf[i];
		if (val & (1ULL << vf->base_queue)) {
			test_and_set_bit(NE6X_MAILBOXQ_EVENT_PENDING, pf->state);
			pf->hw.mbx_snapshot.state = NE6X_MAL_VF_DETECT_STATE_DETECT;
			pf->hw.mbx_snapshot.mbx_vf.vf_cntr[i] = true;
			have_cmd = true;
			int_val |= (1ULL << vf->base_queue);
		}
	}

	if (have_cmd) {
		ne6x_service_event_schedule(pf);
		wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DREQ_INT), int_val);
	}

	val = rd64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DACK_INT));
	ne6x_for_each_vf(pf, i) {
		vf = &pf->vf[i];
		if (val & (1ULL << vf->base_queue)) {
			wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DACK_INT),
				  (1ULL << vf->base_queue));
			pf->hw.mbx_snapshot.state = NE6X_MAL_VF_DETECT_STATE_NEW_SNAPSHOT;
			pf->hw.ne6x_mbx_ready_to_send[i] = true;
		}
	}

	return IRQ_HANDLED;
}

int ne6x_init_mailbox_irq(struct ne6x_pf *pf)
{
	int irq_num;
	int err;

	snprintf(pf->mailbox_intname, sizeof(pf->mailbox_intname) - 1, "%s-%s-%d",
		 dev_driver_string(&pf->pdev->dev), "mailbox", pf->hw.bus.bus_num);
	irq_num = pf->msix_entries[NE6X_MAILBOX_VP_NUM].vector;
	err = request_irq(irq_num, ne6x_msix_clean_vf_mbx, 0, pf->mailbox_intname, pf);
	if (!err)
		pf->mailbox_int_irq_ready = true;

	dev_info(&pf->pdev->dev, "reg mailbox irq id= %d,name = %s\n", irq_num,
		 pf->mailbox_intname);

	return err;
}

int ne6x_disable_mailbox_irq(struct ne6x_pf *pf)
{
	struct ne6x_hw *hw = &pf->hw;

	wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DREQ_INT_MASK), 0xffffffffffffffff);
	wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DACK_INT_MASK), 0xffffffffffffffff);
	wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DREQ_INT), 0xffffffffffffffff);
	wr64_bar4(hw, NE6X_PF_CON_ADDR(NE6X_PF_DB_DACK_INT), 0xffffffffffffffff);

	return 0;
}

void ne6x_free_mailbox_irq(struct ne6x_pf *pf)
{
	if (pf->mailbox_int_irq_ready) {
		synchronize_irq(pf->msix_entries[NE6X_MAILBOX_VP_NUM].vector);
		free_irq(pf->msix_entries[NE6X_MAILBOX_VP_NUM].vector, pf);
	}

	pf->mailbox_int_irq_ready = false;
}

void ne6x_clear_interrupt_scheme(struct ne6x_pf *pf)
{
	int i;

	for (i = 0; i < pf->num_alloc_adpt; i++) {
		if (pf->adpt[i])
			ne6x_adpt_free_q_vectors(pf->adpt[i]);
	}

	ne6x_disable_link_irq(pf);
	ne6x_free_link_irq(pf);
	ne6x_reset_interrupt_capability(pf);
}
