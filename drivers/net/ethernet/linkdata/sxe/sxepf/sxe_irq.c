
#include <linux/numa.h>
#include <linux/kernel.h>
#include <linux/rcupdate.h>
#include <linux/ethtool.h>
#include <linux/moduleparam.h>

#include "sxe.h"
#ifdef HAVE_NO_OVERFLOW_H
#include <sxe_compat_overflow.h>
#else
#include <linux/overflow.h>
#endif
#include "sxe_irq.h"
#include "sxe_pci.h"
#include "sxe_regs.h"
#include "sxe_rx_proc.h"
#include "sxe_tx_proc.h"
#include "sxe_log.h"
#include "sxe_sriov.h"
#include "sxe_monitor.h"
#include "sxe_netdev.h"
#include "sxe_xdp.h"
#include "sxe_host_hdc.h"

#ifdef SXE_SFP_DEBUG
static unsigned int sw_sfp_los_delay_ms = SXE_SW_SFP_LOS_DELAY_MS;
static unsigned int hw_spp_proc_delay_us = SXE_SPP_PROC_DELAY_US;
#ifndef SXE_TEST
module_param(sw_sfp_los_delay_ms, uint, 0);
MODULE_PARM_DESC(sw_sfp_los_delay_ms, "LOS_N(sdp 1) interrupt software filtering time - default is 200");

module_param(hw_spp_proc_delay_us, uint, 0);
MODULE_PARM_DESC(hw_spp_proc_delay_us, "SDP interrupt filtering time - default is 7");
#endif 
#endif

#ifdef NETIF_NAPI_ADD_API_NEED_3_PARAMS
static inline void
netif_napi_add_compat(struct net_device *dev, struct napi_struct *napi,
			int (*poll)(struct napi_struct *, int), int weight)
{
	netif_napi_add(dev, napi, poll);
}

#define netif_napi_add(dev, napi, poll, weight) netif_napi_add_compat(dev, napi, poll, weight)
#endif

static void sxe_enable_irq(struct sxe_adapter *adapter, bool is_ring,
				  bool is_flush)
{
	struct sxe_hw *hw = &adapter->hw;

	u32 value = (SXE_EIMS_ENABLE_MASK & ~SXE_EIMS_RTX_QUEUE);

	if (test_bit(SXE_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state)) {
		value &= ~SXE_EIMS_LSC;
	}

	value |= SXE_EIMS_GPI_SPP1;
	value |= SXE_EIMS_GPI_SPP2;
	value |= SXE_EIMS_MAILBOX;
	value |= SXE_EIMS_ECC;
	value |= SXE_EIMS_HDC;

	if ((adapter->cap & SXE_FNAV_SAMPLE_ENABLE) &&
	   !test_bit(SXE_FNAV_REQUIRES_REINIT, &adapter->monitor_ctxt.state)) {
		value |= SXE_EIMS_FLOW_NAV;
	}

	hw->irq.ops->specific_irq_enable(hw, value);

	if (is_ring) {
		hw->irq.ops->ring_irq_enable(hw, ~0);
	}

	if (is_flush) {
		hw->setup.ops->regs_flush(hw);
	}

	return;
}

static void sxe_irq_num_init(struct sxe_adapter *adapter)
{
	u16 total;
	u16 ring_irq;
	u16 cpu_cnt = num_online_cpus();

	ring_irq = max(adapter->rx_ring_ctxt.num, adapter->tx_ring_ctxt.num);
	ring_irq = max(ring_irq, adapter->xdp_ring_ctxt.num);

	ring_irq = min_t(u16, ring_irq, cpu_cnt);

	total = ring_irq + SXE_EVENT_IRQ_NUM;

	total = min_t(u16, total, adapter->irq_ctxt.max_irq_num);

	adapter->irq_ctxt.total_irq_num = total;

	adapter->irq_ctxt.ring_irq_num = total - SXE_EVENT_IRQ_NUM;

	LOG_INFO("msi-x interrupt rx_ring_num:%u tx_ring_num:%u "
		 "xdp_ring_num:%u cpu cnt:%u max_irq_num:%u "
		 "total_irq_num:%u ring_irq_num:%u event_irq_num:%u \n",
		adapter->rx_ring_ctxt.num,
		adapter->tx_ring_ctxt.num,
		adapter->xdp_ring_ctxt.num,
		cpu_cnt,
		adapter->irq_ctxt.max_irq_num,
		adapter->irq_ctxt.total_irq_num,
		adapter->irq_ctxt.ring_irq_num,
		SXE_EVENT_IRQ_NUM);

	return;
}

static void sxe_irq_num_reinit(struct sxe_adapter *adapter)
{
	adapter->irq_ctxt.total_irq_num = SXE_RING_IRQ_MIN_NUM;
	adapter->irq_ctxt.ring_irq_num = SXE_RING_IRQ_MIN_NUM;

	LOG_INFO("non-msix interrupt rxr_num:%u txr_num:%u "
		"xdp_num:%u max_irq_num:%u total_irq_num:%u "
		"ring_irq_num:%u\n",
		adapter->rx_ring_ctxt.num,
		adapter->tx_ring_ctxt.num,
		adapter->xdp_ring_ctxt.num,
		adapter->irq_ctxt.max_irq_num,
		adapter->irq_ctxt.total_irq_num,
		adapter->irq_ctxt.ring_irq_num);

	return;
}

int sxe_msi_irq_init(struct sxe_adapter *adapter)
{
	int ret;

	ret = pci_enable_msi(adapter->pdev);
	if (ret) {
		adapter->cap &= ~SXE_MSI_ENABLED;
		LOG_DEV_ERR("enable msi interrupt fail. cap:0x%x.(err:%d)\n",
			  adapter->cap, ret);
	} else {
		adapter->cap |= SXE_MSI_ENABLED;
		LOG_INFO("enable msi irq done ret:%d. cap:0x%x\n",
			  ret, adapter->cap);
	}

	return ret;
}

s32 sxe_config_space_irq_num_get(struct sxe_adapter *adapter)
{
	int ret = 0;
	u16 msix_num;
	struct sxe_hw *hw = &adapter->hw;

	msix_num = sxe_read_pci_cfg_word(adapter->pdev,
				hw, SXE_PCIE_MSIX_CAPS_OFFSET);
	if (msix_num == SXE_READ_CFG_WORD_FAILED) {
		ret = -EIO;
		LOG_ERROR_BDF("msi-x caps read fail due to adapter removed.(err:%d)\n",
			   ret);
		goto l_out;
	}

	msix_num &= SXE_PCIE_MSIX_ENTRY_MASK;

	msix_num++;

	msix_num = (msix_num > SXE_MSIX_IRQ_MAX_NUM) ?
		   SXE_MSIX_IRQ_MAX_NUM : msix_num;

	adapter->irq_ctxt.max_irq_num = msix_num;

l_out:
	return ret;
}

void sxe_disable_dcb(struct sxe_adapter *adapter)
{
	if (sxe_dcb_tc_get(adapter) > 1) {
		LOG_DEV_WARN("number of DCB TCs exceeds number of available queues."
			"disabling DCB support.\n");
		netdev_reset_tc(adapter->netdev);
		adapter->cap &= ~SXE_DCB_ENABLE;
		adapter->dcb_ctxt.cee_temp_cfg.pfc_mode_enable = false;
		adapter->dcb_ctxt.cee_cfg.pfc_mode_enable = false;
	}

	sxe_dcb_tc_set(adapter, 0);
	adapter->dcb_ctxt.cee_cfg.num_tcs.pg_tcs = 1;
	adapter->dcb_ctxt.cee_cfg.num_tcs.pfc_tcs = 1;

	LOG_INFO_BDF("dcb disabled  cap:0x%x.\n", adapter->cap);

	return;
}

static int sxe_disable_sriov(struct sxe_adapter *adapter)
{
	LOG_DEV_WARN("disabling SR-IOV support.\n");

	sxe_vf_resource_release(adapter);
	sxe_vf_disable(adapter);

	pci_disable_sriov(adapter->pdev);

	return 0;
}

void sxe_disable_rss(struct sxe_adapter *adapter)
{
	LOG_DEV_WARN("disabling RSS support.\n");

	adapter->ring_f.rss_limit = 1;
	adapter->cap &= ~SXE_RSS_ENABLE;

	LOG_INFO_BDF("rss disabled rss_limit:%u cap:0x%x.\n",
		adapter->ring_f.rss_limit, adapter->cap);

	return;
}

static bool sxe_is_irq_bind_cpu(struct sxe_adapter *adapter)
{
	if (!(adapter->cap  & SXE_DCB_ENABLE) &&
	    !(adapter->cap & SXE_SRIOV_ENABLE) &&
	    (adapter->cap & SXE_RSS_ENABLE) &&
	    (adapter->cap & SXE_FNAV_SAMPLE_ENABLE)) {
		LOG_INFO("cap:0x%x need alloc memory in cpu node.\n",
			adapter->cap);
		return true;
	} else {
		LOG_INFO("cap:0x%x no need alloc memory in cpu node.\n",
			adapter->cap);
		return false;
	}
}

static s32 sxe_msix_irq_init(struct sxe_adapter *adapter)
{
	u16 i;
	s32 ret;
	u16 total = adapter->irq_ctxt.total_irq_num;

	adapter->irq_ctxt.msix_entries = kcalloc(total,
						sizeof(struct msix_entry),
						GFP_KERNEL);
	if (adapter->irq_ctxt.msix_entries == NULL) {
		ret = -ENOMEM;
		LOG_ERROR_BDF("msi-x irq entry num:%u per size:%lu kcalloc fail."
			  "(err:%d)\n",
			   total, sizeof(struct msix_entry), ret);
		goto l_out;
	}

	for (i = 0; i < total; i++) {
		adapter->irq_ctxt.msix_entries[i].entry = i;
	}

	ret = pci_enable_msix_range(adapter->pdev,
				    adapter->irq_ctxt.msix_entries,
				    SXE_MSIX_IRQ_MIN_NUM, total);
	if (ret < 0) { 
		adapter->cap &= ~SXE_MSIX_ENABLED;
		SXE_KFREE(adapter->irq_ctxt.msix_entries);

		LOG_DEV_ERR("min:%u max:%u pci enable msi-x failed.(err:%d)\n",
			 SXE_MSIX_IRQ_MIN_NUM, total, ret);
	} else {
		adapter->cap |= SXE_MSIX_ENABLED;
		if (ret != total) {
			adapter->irq_ctxt.total_irq_num = ret;
			adapter->irq_ctxt.ring_irq_num = ret -
					SXE_EVENT_IRQ_NUM;
		}
		LOG_WARN_BDF("enable %d pci msix entry.min:%u max:%u"
			 " total irq num:%u ring irq num:%u cap:0x%x\n",
			  ret, SXE_MSIX_IRQ_MIN_NUM, total,
			  adapter->irq_ctxt.total_irq_num,
			  adapter->irq_ctxt.ring_irq_num,
			  adapter->cap);

		ret = 0;
	}

l_out:
	return ret;
}

static inline void sxe_irq_non_msix_configure(struct sxe_adapter *adapter)
{
	sxe_disable_dcb(adapter);
	sxe_disable_sriov(adapter);
	sxe_disable_rss(adapter);
	sxe_ring_num_set(adapter);
	sxe_irq_num_reinit(adapter);

	return;
}

static void sxe_irq_mode_init(struct sxe_adapter *adapter)
{
	s32 ret;

	if (sxe_is_irq_intx_mode()) {
		adapter->cap &= ~SXE_MSI_ENABLED;
		sxe_irq_non_msix_configure(adapter);
		LOG_INFO("the intx ctrl param enable\n");
		goto l_end;
	}

	if (sxe_is_irq_msi_mode()) {
		LOG_INFO("the msi ctrl param enable\n");
		goto l_msi;
	}

	ret = sxe_msix_irq_init(adapter);
	if (!ret) {
		goto l_end;

	} else {
		LOG_WARN_BDF("msix-x irq init fail (err:%d), try msi irq.\n", ret);
	}

l_msi:
	sxe_irq_non_msix_configure(adapter);

	ret = sxe_msi_irq_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("msi irq init fail.(err:%d) "
				"use legacy(intx) irq cap:0x%x\n",
				ret, adapter->cap);
	}

l_end:
	return;
}

static void sxe_irq_data_free(struct sxe_adapter *adapter,
					    u16 irq_idx)
{
	u16 idx;
	struct sxe_irq_data *irq_data = adapter->irq_ctxt.irq_data[irq_idx];
	struct sxe_ring *ring;

	sxe_for_each_ring(ring, irq_data->tx.list) {
		adapter->tx_ring_ctxt.ring[ring->idx] = NULL;
	}

	if (irq_data->tx.xdp_ring) {
		idx = irq_data->tx.xdp_ring->idx;
		adapter->xdp_ring_ctxt.ring[idx] = NULL;
	}

	sxe_for_each_ring(ring, irq_data->rx.list) {
		adapter->rx_ring_ctxt.ring[ring->idx] = NULL;
	}

	adapter->irq_ctxt.irq_data[irq_idx] = NULL;
	netif_napi_del(&irq_data->napi);

#ifdef HAVE_XDP_SUPPORT
	if (static_key_enabled(&sxe_xdp_tx_lock_key)) {
		static_branch_dec(&sxe_xdp_tx_lock_key);
	}
#endif

	kfree_rcu(irq_data, rcu);

	return;
}

static void sxe_all_irq_data_free(struct sxe_adapter *adapter)
{
	u16 irq_idx = adapter->irq_ctxt.ring_irq_num;

	while (irq_idx--) {
		sxe_irq_data_free(adapter, irq_idx);
	}

	return;
}

static int sxe_irq_data_alloc(struct sxe_adapter *adapter,
				  u16 total_count, u16 irq_idx, bool is_bind)
{
	int ret = 0;
	s32 node = dev_to_node(&adapter->pdev->dev);
	s32 cpu = -1;
	struct sxe_irq_data *irq_data;

	if (is_bind) {
		cpu = cpumask_local_spread(irq_idx, node);
		node = cpu_to_node(cpu);
	}

	irq_data = kzalloc_node(struct_size(irq_data, ring, total_count),
				GFP_KERNEL, node);
	if (irq_data == NULL) {
		LOG_ERROR_BDF("alloc interrupt data and ring resource in node:%d "
			  "failed, try remote. size: %zu irq_idx:%u "
			  "ring count:%u.(err:%d)\n",
			  node, struct_size(irq_data, ring, total_count),
			  irq_idx, total_count, ret);

		irq_data = kzalloc(struct_size(irq_data, ring, total_count),
				  GFP_KERNEL);
		if (irq_data == NULL) {
			ret = -ENOMEM;
			LOG_ERROR_BDF("alloc interrupt data and ring resource "
				  "failed again. size: %zu irq_idx:%u "
				  "ring count:%u.(err:%d)\n",
				  struct_size(irq_data, ring, total_count),
				  irq_idx, total_count, ret);
			goto l_out;
		}
	}

	if (cpu != -1) {
		cpumask_set_cpu(cpu, &irq_data->affinity_mask);
	}

	irq_data->numa_node = node;

#ifdef SXE_TPH_CONFIGURE
	irq_data->cpu = -1;
#endif

	netif_napi_add(adapter->netdev, &irq_data->napi, sxe_poll,
		       SXE_NAPI_WEIGHT);

	adapter->irq_ctxt.irq_data[irq_idx] = irq_data;
	irq_data->adapter = adapter;
	irq_data->irq_idx = irq_idx;

	irq_data->tx.work_limit = SXE_TX_WORK_LIMIT;

	LOG_INFO_BDF("irq_idx:%u ring_cnt:%u is_bind:%d bind to cpu:%d node:%d. "
		  "tx work_limit:%d \n",
		  irq_idx, total_count,
		  is_bind, cpu, node,
		  irq_data->tx.work_limit);

l_out:
	return ret;
}

void sxe_napi_disable(struct sxe_adapter *adapter)
{
	u32 i;

	for (i = 0; i < adapter->irq_ctxt.ring_irq_num; i++) {
		napi_disable(&adapter->irq_ctxt.irq_data[i]->napi);
	}

	return ;
}

static void sxe_napi_enable_all(struct sxe_adapter * adapter)
{
	u16 irq_idx;

	for (irq_idx = 0; irq_idx < adapter->irq_ctxt.ring_irq_num; irq_idx++) {
		napi_enable(&(adapter->irq_ctxt.irq_data[irq_idx]->napi));
	}

	return;
}

static void sxe_irq_interval_init(struct sxe_irq_context  *irq_ctxt,
				u16 irq_idx, u16 txr_cnt, u16 rxr_cnt)
{
	struct sxe_irq_data *irq_data = irq_ctxt->irq_data[irq_idx];

	if (txr_cnt && !rxr_cnt) {
		if (irq_ctxt->tx_irq_interval == 1) {
			irq_data->irq_interval = SXE_IRQ_ITR_12K;
		} else {
			irq_data->irq_interval = irq_ctxt->tx_irq_interval;
		}
	} else {
		if (irq_ctxt->rx_irq_interval == 1) {
			irq_data->irq_interval = SXE_IRQ_ITR_20K;
		} else {
			irq_data->irq_interval = irq_ctxt->rx_irq_interval;
		}
	}

	irq_data->tx.irq_rate.irq_interval = SXE_IRQ_ITR_MAX
						     | SXE_IRQ_ITR_LATENCY;
	irq_data->rx.irq_rate.irq_interval = SXE_IRQ_ITR_MAX
						     | SXE_IRQ_ITR_LATENCY;

	LOG_INFO("irq_idx:%u irq level interval:%u "
		     "list level rx irq interval:%u tx irq interval:%u\n",
		     irq_idx, irq_data->irq_interval,
		     irq_data->rx.irq_rate.irq_interval,
		     irq_data->tx.irq_rate.irq_interval);
	return;
}

#ifdef HAVE_AF_XDP_ZERO_COPY
static void sxe_set_ring_idx(struct sxe_adapter *adapter)
{
	u16 i;

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		if (adapter->rx_ring_ctxt.ring[i]) {
			adapter->rx_ring_ctxt.ring[i]->ring_idx = i;
		}
	}

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		if (adapter->tx_ring_ctxt.ring[i]) {
			adapter->tx_ring_ctxt.ring[i]->ring_idx = i;
		}
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		if (adapter->xdp_ring_ctxt.ring[i]) {
			adapter->xdp_ring_ctxt.ring[i]->ring_idx = i;
		}
	}

	return;
}
#endif

static s32 sxe_irq_ring_bind(struct sxe_adapter *adapter)
{
	s32 ret = 0;
	u16 rxr_idx = 0;
	u16 txr_idx = 0;
	u16 xdp_idx = 0;
	u16 irq_idx = 0;
	u16 irq_num = adapter->irq_ctxt.ring_irq_num;
	u16 rxr_remain = adapter->rx_ring_ctxt.num;
	u16 txr_remain = adapter->tx_ring_ctxt.num;
	u16 xdp_remain = adapter->xdp_ring_ctxt.num;
	u16 total_ring = rxr_remain + txr_remain + xdp_remain;
	bool is_bind = sxe_is_irq_bind_cpu(adapter);

	if (irq_num >= total_ring) {
		for (; rxr_remain > 0; irq_idx++) {
			ret = sxe_irq_data_alloc(adapter, 1, irq_idx, is_bind);
			if (ret) {
				LOG_ERROR_BDF("irq_num:%u rxr_remain:%u "
					  "txr_remain:%u xdp_remain:%u "
					  "irq_idx:%u alloc rx irq "
					  "resource priority fail.(err:%d)\n",
					  irq_num, rxr_remain,
					  txr_remain, xdp_remain,
					  irq_idx, ret);
				goto l_error;
			}

			sxe_irq_interval_init(&adapter->irq_ctxt, irq_idx, 0, 1);

			sxe_rx_ring_init(adapter, 0, 1, rxr_idx, irq_idx);

			rxr_remain--;
			rxr_idx++;
		}
		LOG_INFO("alloc rx irq resource priority done.irq_idx:%u "
			  "rxr_idx:%u txr_remain:%u rxr_remain:%u xdp_remain:%u"
			  " ring_irq_num:%u total_ring:%u \n",
			  irq_idx, rxr_idx, txr_remain, rxr_remain,
			  xdp_remain, irq_num, total_ring);
	}

	for (; irq_idx < irq_num; irq_idx++) {
		u16 txr_cnt = DIV_ROUND_UP(txr_remain, irq_num - irq_idx);

		u16 xdp_cnt = DIV_ROUND_UP(xdp_remain, irq_num - irq_idx);

		u16 rxr_cnt = DIV_ROUND_UP(rxr_remain, irq_num - irq_idx);

		total_ring = txr_cnt + xdp_cnt + rxr_cnt;

		ret = sxe_irq_data_alloc(adapter, total_ring, irq_idx, is_bind);
		if (ret) {
			LOG_ERROR_BDF("irq_num:%u rxr_remain:%u txr_remain:%u "
				  "xdp_remain:%u rxr_cnt:%u txr_cnt:%u "
				  " xdp_cnt:%u ird_idx:%u alloc irq resource "
				  " fail.(err:%d)\n",
				  irq_num, rxr_remain, txr_remain,
				  xdp_remain, rxr_cnt, txr_cnt,
				  xdp_cnt, irq_idx, ret);
			goto l_error;
		}

		sxe_irq_interval_init(&adapter->irq_ctxt, irq_idx, txr_cnt,
				     rxr_cnt);

		sxe_tx_ring_init(adapter, 0, txr_cnt, txr_idx, irq_idx);

		sxe_xdp_ring_init(adapter, txr_cnt, xdp_cnt, xdp_idx, irq_idx);

		sxe_rx_ring_init(adapter, txr_cnt + xdp_cnt, rxr_cnt,
				 rxr_idx, irq_idx);

		txr_remain -= txr_cnt;
		xdp_remain -= xdp_cnt;
		rxr_remain -= rxr_cnt;

		txr_idx++;
		xdp_idx += xdp_cnt;
		rxr_idx++;
	}

#ifdef HAVE_AF_XDP_ZERO_COPY
	sxe_set_ring_idx(adapter);
#endif

	return ret;

l_error:
	adapter->irq_ctxt.ring_irq_num = 0;
	adapter->tx_ring_ctxt.num = 0;
	adapter->rx_ring_ctxt.num = 0;
	adapter->xdp_ring_ctxt.num = 0;

	while(irq_idx--) {
		sxe_irq_data_free(adapter, irq_idx);
	}

	return ret;

}

static void sxe_pci_irq_disable(struct sxe_adapter *adapter)
{
	if (adapter->cap & SXE_MSIX_ENABLED) {
		adapter->cap &= ~SXE_MSIX_ENABLED;
		pci_disable_msix(adapter->pdev);
		SXE_KFREE(adapter->irq_ctxt.msix_entries);
	} else if (adapter->cap & SXE_MSI_ENABLED) {
		adapter->cap &= ~SXE_MSI_ENABLED;
		pci_disable_msi(adapter->pdev);
	}

	return;
}

void sxe_hw_irq_disable(struct sxe_adapter *adapter)
{
	u16 i;
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_irq_context *irq = &adapter->irq_ctxt;

	hw->irq.ops->all_irq_disable(hw);

	if (adapter->cap & SXE_MSIX_ENABLED) {
		for (i = 0; i < adapter->irq_ctxt.ring_irq_num; i++) {
			synchronize_irq(irq->msix_entries[i].vector);
		}
		synchronize_irq(irq->msix_entries[i].vector);
	} else {
		synchronize_irq(adapter->pdev->irq);
	}

	return;
}

void sxe_irq_release(struct sxe_adapter *adapter)
{
	u16 irq_idx;
	struct sxe_irq_context *irq_ctxt = &adapter->irq_ctxt;

	if (!irq_ctxt->ring_irq_num) {
		goto l_out;
	}

	if (!(adapter->cap & SXE_MSIX_ENABLED)) {
		free_irq(adapter->pdev->irq, adapter);
		goto l_out;
	}

	if (!irq_ctxt->msix_entries) {
		goto l_out;
	}

	for (irq_idx = 0; irq_idx < irq_ctxt->ring_irq_num; irq_idx++) {
		struct sxe_irq_data *irq_data = irq_ctxt->irq_data[irq_idx];
		struct msix_entry *entry = &irq_ctxt->msix_entries[irq_idx];

		if (!irq_data->rx.list.next &&
		    !irq_data->tx.list.next &&
		    !irq_data->tx.xdp_ring) {
			continue;
		}

		irq_set_affinity_hint(entry->vector, NULL);

		free_irq(entry->vector, irq_data);
	}

	free_irq(irq_ctxt->msix_entries[irq_idx].vector, adapter);

l_out:
	LOG_INFO_BDF("adapter cap:0x%x ring_irq_num:%u irq unregister done.",
		      adapter->cap, irq_ctxt->ring_irq_num);
	return;
}

s32 sxe_irq_ctxt_init(struct sxe_adapter *adapter)
{
	s32 ret;

	sxe_irq_num_init(adapter);

	sxe_irq_mode_init(adapter);

	ret = sxe_irq_ring_bind(adapter);
	if (ret) {
		LOG_DEV_ERR("interrupt and ring bind fail.(err:%d)\n", ret);
		goto l_disable_irq;
	}

	return ret;

l_disable_irq:
	sxe_pci_irq_disable(adapter);
	return ret;

}

void sxe_irq_ctxt_exit(struct sxe_adapter *adapter)
{

	sxe_all_irq_data_free(adapter);

	sxe_pci_irq_disable(adapter);

	adapter->irq_ctxt.ring_irq_num = 0;
	adapter->tx_ring_ctxt.num = 0;
	adapter->rx_ring_ctxt.num = 0;
	adapter->xdp_ring_ctxt.num = 0;

	return;
}

static bool sxe_set_irq_name(struct sxe_irq_data *irq_data,
				     char *dev_name,
				     u16 *rx_idx, u16 *tx_idx)
{
	if (irq_data->tx.list.next &&
	    irq_data->rx.list.next) {
		snprintf(irq_data->name, sizeof(irq_data->name),
			"%s-TxRx-%u", dev_name, (*rx_idx)++);
		(*tx_idx)++;
	} else if (irq_data->rx.list.next) {
		snprintf(irq_data->name, sizeof(irq_data->name),
			"%s-Rx-%u", dev_name, (*rx_idx)++);
	} else if (irq_data->tx.list.next ||
		   irq_data->tx.xdp_ring) {
		snprintf(irq_data->name, sizeof(irq_data->name),
			"%s-Tx-%u", dev_name, (*tx_idx)++);
	} else {
		LOG_INFO("%u irq has no ring bind.\n", irq_data->irq_idx);
		return false;
	}

	return true;
}

STATIC irqreturn_t sxe_msix_ring_irq_handler(int irq, void *data)
{
	struct sxe_irq_data *irq_data = data;

	if (irq_data->tx.list.next ||
	    irq_data->rx.list.next ||
	    irq_data->tx.xdp_ring) {
		napi_schedule_irqoff(&irq_data->napi);
	}

	return IRQ_HANDLED ;

}

void sxe_lsc_irq_handler(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;

	adapter->stats.sw.link_state_change_cnt++;
	set_bit(SXE_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state);
	LOG_DEBUG("lsc irq: trigger link_check subtask\n");

	adapter->link.check_timeout = jiffies;
	if (!test_bit(SXE_DOWN, &adapter->state)) {
		hw->irq.ops->specific_irq_disable(hw, SXE_EIMC_LSC);
		sxe_monitor_work_schedule(adapter);
		LOG_DEBUG_BDF("lsc: monitor schedule\n");
	}

	return;
}

void sxe_mailbox_irq_handler(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	unsigned long flags;
	u8 vf_idx;

	LOG_DEBUG_BDF("rcv mailbox irq.num_vfs:%u.\n", adapter->vt_ctxt.num_vfs);

	spin_lock_irqsave(&adapter->vt_ctxt.vfs_lock, flags);
	for (vf_idx = 0; vf_idx < adapter->vt_ctxt.num_vfs; vf_idx++) {

		if (hw->mbx.ops->rst_check(hw, vf_idx)) {
			LOG_INFO("vf_idx:%d flr triggered.\n", vf_idx);
			sxe_vf_hw_rst(adapter, vf_idx);
		}

		if (hw->mbx.ops->req_check(hw, vf_idx)) {
			sxe_vf_req_task_handle(adapter, vf_idx);
		}

		if (hw->mbx.ops->ack_check(hw, vf_idx)) {
			sxe_vf_ack_task_handle(adapter, vf_idx);
		}
	}
	spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);

	return;
}

STATIC void sxe_fnav_irq_handler(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	u32 reinit_count = 0;
	u32 i;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		struct sxe_ring *ring = adapter->tx_ring_ctxt.ring[i];
		if (test_and_clear_bit(SXE_TX_FNAV_INIT_DONE,
				       &ring->state)) {
			reinit_count++;
		}
	}

	LOG_INFO_BDF("adapter[%p] fnav reinit, count=%u\n",adapter, reinit_count);

	if (reinit_count) {
		hw->irq.ops->specific_irq_disable(hw, SXE_EIMC_FLOW_NAV);
		set_bit(SXE_FNAV_REQUIRES_REINIT, &adapter->monitor_ctxt.state);
		sxe_monitor_work_schedule(adapter);
	}

	return;
}

static void sxe_sfp_irq_handler(struct sxe_adapter *adapter, u32 eicr)
{
	struct sxe_hw *hw = &adapter->hw;

	if (!sxe_is_sfp(adapter)) {
		return;
	}

	if (eicr & SXE_EICR_GPI_SPP2) {
		hw->irq.ops->pending_irq_write_clear(hw, SXE_EICR_GPI_SPP2);
		if (!test_bit(SXE_DOWN, &adapter->state)) {
			set_bit(SXE_SFP_NEED_RESET, &adapter->monitor_ctxt.state);
			adapter->link.sfp_reset_timeout = 0;
			LOG_DEV_WARN("sfp is inserted into slot, "
				"trigger sfp_reset subtask\n");
			sxe_monitor_work_schedule(adapter);
		}
	}

	if (eicr & SXE_EICR_GPI_SPP1) {
		hw->irq.ops->pending_irq_write_clear(hw, SXE_EICR_GPI_SPP1);
		if (!test_bit(SXE_DOWN, &adapter->state) &&
			!test_bit(SXE_SFP_MULTI_SPEED_SETTING, &adapter->state)) {
			if (time_after(jiffies, adapter->link.last_lkcfg_time +
#ifdef SXE_SFP_DEBUG
				(HZ * sw_sfp_los_delay_ms) / SXE_HZ_TRANSTO_MS)
#else
				(HZ * SXE_SW_SFP_LOS_DELAY_MS) / SXE_HZ_TRANSTO_MS)
#endif
				) {
				adapter->link.last_lkcfg_time = jiffies;
				set_bit(SXE_LINK_NEED_CONFIG, &adapter->monitor_ctxt.state);
				LOG_MSG_INFO(hw, "sfp optical signal level below standard, "
					"trigger link_config subtask\n");
				sxe_monitor_work_schedule(adapter);
			}
		}
	}
}

static void sxe_event_irq_common_handler(struct sxe_adapter *adapter, u32 eicr)
{
	if (eicr & SXE_EICR_HDC) {
		sxe_hdc_irq_handler(adapter);
	}

	if (eicr & SXE_EICR_LSC) {
		sxe_lsc_irq_handler(adapter);
	}

	if (eicr & SXE_EICR_MAILBOX)
		sxe_mailbox_irq_handler(adapter);

	if (eicr & SXE_EICR_ECC) {
		LOG_MSG_WARN(link, "ecc interrupt triggered eicr:0x%x.\n",
			 eicr);
	}

	if (eicr & SXE_EICR_FLOW_NAV) {
		sxe_fnav_irq_handler(adapter);
	}

	if ((eicr & SXE_EICR_GPI_SPP1) || (eicr & SXE_EICR_GPI_SPP2)) {
		sxe_sfp_irq_handler(adapter, eicr);
	}

	return;
}

STATIC irqreturn_t sxe_msix_event_irq_handler(int irq, void *data)
{
	struct sxe_adapter *adapter = data;
	struct sxe_hw *hw = &adapter->hw;
	unsigned long flags;
	u32 eicr;

	spin_lock_irqsave(&adapter->irq_ctxt.event_irq_lock, flags);

	hw->irq.ops->specific_irq_disable(hw, 0xFFFF0000);
	mb();

	eicr = hw->irq.ops->irq_cause_get(hw);

	eicr &= 0xFFFF0000;

	hw->irq.ops->pending_irq_write_clear(hw, eicr);

	spin_unlock_irqrestore(&adapter->irq_ctxt.event_irq_lock, flags);

	sxe_event_irq_common_handler(adapter, eicr);

	if (!test_bit(SXE_DOWN, &adapter->state)) {
		sxe_enable_irq(adapter, false, false);
	}

	LOG_INFO("rcv event irq:%d eicr:0x%x.\n", irq, eicr);

	return IRQ_HANDLED;
}

STATIC irqreturn_t sxe_non_msix_irq_handler(int irq, void *data)
{
	struct sxe_adapter *adapter = data;
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_irq_data *irq_data = adapter->irq_ctxt.irq_data[0];
	u32 eicr;

	hw->irq.ops->specific_irq_disable(hw, SXE_IRQ_CLEAR_MASK);

	eicr = hw->irq.ops->pending_irq_read_clear(hw);
	if (!eicr) {
		if (!test_bit(SXE_DOWN, &adapter->state)) {
			sxe_enable_irq(adapter, true, true);
		}
		return IRQ_NONE;
	}

	sxe_event_irq_common_handler(adapter, eicr);

	napi_schedule_irqoff(&irq_data->napi);

	if (!test_bit(SXE_DOWN, &adapter->state)) {
		sxe_enable_irq(adapter, false, false);
	}

	return IRQ_HANDLED;
}

static int sxe_msix_request_irqs(struct sxe_adapter *adapter)
{
	int ret;
	u16 rx_idx = 0;
	u16 tx_idx = 0;
	u16 irq_idx;
	struct sxe_irq_data *irq_data;
	struct msix_entry   *entry;
	struct net_device *netdev = adapter->netdev;
	struct sxe_irq_context *irq_ctxt = &adapter->irq_ctxt;

	for (irq_idx = 0; irq_idx < irq_ctxt->ring_irq_num; irq_idx++) {
		irq_data = irq_ctxt->irq_data[irq_idx];
		entry = &irq_ctxt->msix_entries[irq_idx];

		if (!(sxe_set_irq_name(irq_data, netdev->name,
				       &rx_idx, &tx_idx))) {
			continue;
		}

		ret = request_irq(entry->vector, &sxe_msix_ring_irq_handler, 0,
				  irq_data->name, irq_data);
		if (ret) {
			LOG_MSG_ERR(probe, "irq_idx:%u rx_idx:%u tx_idx:%u irq_num:%u "
				  "vector:%u msi-x ring interrupt "
				  "reuqest fail.(err:%d)\n",
				  irq_idx, rx_idx, tx_idx,
				  irq_ctxt->ring_irq_num,
				  entry->vector, ret);
			goto l_free_irq;
		}

		if (adapter->cap & SXE_FNAV_SAMPLE_ENABLE) {
			irq_set_affinity_hint(entry->vector,
				&irq_data->affinity_mask);
		}
	}

	ret = request_irq(irq_ctxt->msix_entries[irq_idx].vector,
			 sxe_msix_event_irq_handler, 0, netdev->name, adapter);
	if (ret) {
		LOG_MSG_ERR(probe, "irq_idx:%u vector:%u msi-x event interrupt "
			  "reuqest fail.(err:%d)\n",
			  irq_idx,
			  irq_ctxt->msix_entries[irq_idx].vector, ret);
		goto l_free_irq;
	}

	return ret;

l_free_irq:
	while (irq_idx) {
		irq_idx--;
		irq_set_affinity_hint(adapter->irq_ctxt.msix_entries[irq_idx].vector, NULL);
		free_irq(irq_ctxt->msix_entries[irq_idx].vector,
			irq_ctxt->irq_data[irq_idx]);
	}


	adapter->cap &= ~SXE_MSIX_ENABLED;
	pci_disable_msix(adapter->pdev);

	SXE_KFREE(adapter->irq_ctxt.msix_entries);

	return ret;
}

STATIC int sxe_request_irq(struct sxe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int ret;

	if (adapter->cap & SXE_MSIX_ENABLED) {
		LOG_INFO("request msi-x interrupt.\n");
		ret = sxe_msix_request_irqs(adapter);
	} else if (adapter->cap & SXE_MSI_ENABLED) {
		LOG_INFO("request msi interrupt.\n");
		ret = request_irq(adapter->pdev->irq, sxe_non_msix_irq_handler,
				  0, netdev->name, adapter);
	} else {
		LOG_INFO("request legacy interrupt.\n");
		ret = request_irq(adapter->pdev->irq, sxe_non_msix_irq_handler,
				  IRQF_SHARED, netdev->name, adapter);
	}

	return ret;
}

static void sxe_configure_msix_hw(struct sxe_adapter *adapter)
{
	u16 irq_idx;
	u32 value;
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_ring *ring;
	struct sxe_irq_context *irq_ctxt = &adapter->irq_ctxt;

	if (adapter->vt_ctxt.num_vfs > 32) {
		u32 sel_value = BIT(adapter->vt_ctxt.num_vfs - 32) - 1;
		hw->irq.ops->set_eitrsel(hw, sel_value);
	}

	for (irq_idx = 0; irq_idx < irq_ctxt->ring_irq_num; irq_idx++) {
		struct sxe_irq_data *irq_data = irq_ctxt->irq_data[irq_idx];

		sxe_for_each_ring(ring, irq_data->rx.list) {

			hw->irq.ops->ring_irq_map(hw, false, ring->reg_idx,
						  irq_idx);
		}

		sxe_for_each_ring(ring, irq_data->tx.list) {
			hw->irq.ops->ring_irq_map(hw, true, ring->reg_idx,
						  irq_idx);
		}

		if (irq_data->tx.xdp_ring) {
			hw->irq.ops->ring_irq_map(hw, true,
						irq_data->tx.xdp_ring->reg_idx,
						irq_idx);
		}

		hw->irq.ops->ring_irq_interval_set(hw, irq_idx, irq_data->irq_interval);
	}

	hw->irq.ops->event_irq_map(hw, 1, irq_idx);

	hw->irq.ops->event_irq_interval_set(hw, irq_idx, 1950);


	value = SXE_EIMS_ENABLE_MASK;
	value &= ~(SXE_EIMS_OTHER | SXE_EIMS_MAILBOX | SXE_EIMS_LSC);
	hw->irq.ops->event_irq_auto_clear_set(hw, value);

	return;
}

static void sxe_configure_non_msix_hw(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_irq_data *irq_data = adapter->irq_ctxt.irq_data[0];

	hw->irq.ops->ring_irq_interval_set(hw, 0, irq_data->irq_interval);

	hw->irq.ops->ring_irq_map(hw, false, 0, 0);
	hw->irq.ops->ring_irq_map(hw, true, 0, 0);

	LOG_MSG_INFO(hw, "non msix interrupt ivar setup done.\n");

	return;
}

static void sxe_irq_general_configure(struct sxe_adapter *adapter)
{
	u32 gpie = 0;
	struct sxe_hw *hw = &adapter->hw;
	u32 pool_mask = sxe_pool_mask_get(adapter);

	if (adapter->cap & SXE_MSIX_ENABLED) {
		gpie = SXE_GPIE_MSIX_MODE | SXE_GPIE_PBA_SUPPORT |
			SXE_GPIE_OCD | SXE_GPIE_EIAME;

		hw->irq.ops->ring_irq_auto_disable(hw, true);
	} else {
		hw->irq.ops->ring_irq_auto_disable(hw, false);
	}

	if (adapter->cap & SXE_SRIOV_ENABLE) {
		gpie &= ~SXE_GPIE_VTMODE_MASK;
		switch (pool_mask) {
		case SXE_8Q_PER_POOL_MASK:
			gpie |= SXE_GPIE_VTMODE_16;
			break;
		case SXE_4Q_PER_POOL_MASK:
			gpie |= SXE_GPIE_VTMODE_32;
			break;
		default:
			gpie |= SXE_GPIE_VTMODE_64;
			break;
		}
	}

	gpie |= SXE_GPIE_SPP1_EN | SXE_GPIE_SPP2_EN;

	hw->irq.ops->irq_general_reg_set(hw, gpie);

	return;
}

void sxe_hw_irq_configure(struct sxe_adapter *adapter)
{
	s32 ret = 0;
	struct sxe_hw *hw = &adapter->hw;

	hw->irq.ops->spp_configure(hw,
#ifdef SXE_SFP_DEBUG
	hw_spp_proc_delay_us);
#else
	SXE_SPP_PROC_DELAY_US);
#endif

	sxe_irq_general_configure(adapter);

	if (adapter->cap & SXE_MSIX_ENABLED) {
		sxe_configure_msix_hw(adapter);
	} else {
		sxe_configure_non_msix_hw(adapter);
	}

	if (adapter->phy_ctxt.ops->sfp_tx_laser_enable) {
		adapter->phy_ctxt.ops->sfp_tx_laser_enable(adapter);
	}

	smp_mb__before_atomic();
	clear_bit(SXE_DOWN, &adapter->state);

	sxe_napi_enable_all(adapter);

	if (sxe_is_sfp(adapter)) {
		sxe_sfp_reset_task_submit(adapter);
	} else {
		ret = sxe_link_config(adapter);
		if(ret) {
			LOG_MSG_ERR(probe, "sxe_link_config failed %d\n", ret);
		}
	}

	hw->irq.ops->pending_irq_read_clear(hw);

	sxe_enable_irq(adapter, true, true);

	return;
}

int sxe_irq_configure(struct sxe_adapter *adapter)
{
	int ret;

	ret = sxe_request_irq(adapter);
	if (ret) {
		LOG_MSG_ERR(probe, "interrupt mode:0x%x request irq failed, (err:%d)\n",
			   adapter->cap, ret);
		goto l_out;
	}

	sxe_hw_irq_configure(adapter);

l_out:
	return ret;
}

static bool sxe_lro_status_update(struct sxe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	u16 itr = adapter->irq_ctxt.rx_irq_interval;
	bool ret = false;

	if (!(adapter->cap & SXE_LRO_CAPABLE) ||
	    !(netdev->features & NETIF_F_LRO)) {
	     LOG_INFO("lro disable status, no need judge itr interval.\n");
	     goto l_out;
	}

	if ((itr == 1) || (itr > SXE_IRQ_LRO_ITR_MIN)) {
		if (!(adapter->cap & SXE_LRO_ENABLE)) {
			adapter->cap |= SXE_LRO_ENABLE;
			ret = true;
			LOG_MSG_INFO(probe, "user itr:%u large than lro delay:%u, "
				 "enable lro.\n", itr, SXE_IRQ_LRO_ITR_MIN);
		}
	} else if (adapter->cap & SXE_LRO_ENABLE) {
		adapter->cap &= ~SXE_LRO_ENABLE;
		ret = true;
		LOG_MSG_INFO(probe, "user itr:%u less than lro delay:%u, disable lro.\n",
			itr, SXE_IRQ_LRO_ITR_MIN);
	}

l_out:
	return ret;
}

s32 sxe_irq_coalesce_get(struct net_device * netdev,
				struct ethtool_coalesce *user)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	u16 rx_itr = adapter->irq_ctxt.rx_irq_interval;
	u16 tx_itr = adapter->irq_ctxt.tx_irq_interval;
	struct sxe_irq_data *irq_data = adapter->irq_ctxt.irq_data[0];
	bool is_mixed;
	s32 ret = 0;


	if (irq_data->tx.list.cnt && irq_data->rx.list.cnt) {
		is_mixed = true;
	} else {
		is_mixed = false;
	}

	if (rx_itr == SXE_IRQ_ITR_CONSTANT_MODE_VALUE) {
		user->rx_coalesce_usecs = SXE_IRQ_ITR_CONSTANT_MODE_VALUE;
	} else {
		user->rx_coalesce_usecs = rx_itr >> SXE_EITR_ITR_SHIFT;
	}

	if (is_mixed) {
		LOG_INFO("interrupt 0 has both rx and tx ring, "
			 "just report rx itr:%u.\n",
			 user->rx_coalesce_usecs);
		goto l_out;
	}

	if (tx_itr == SXE_IRQ_ITR_CONSTANT_MODE_VALUE) {
		user->tx_coalesce_usecs = SXE_IRQ_ITR_CONSTANT_MODE_VALUE;
	} else {
		user->tx_coalesce_usecs = tx_itr >> SXE_EITR_ITR_SHIFT;
	}

	LOG_INFO("rx irq interval:%u tx irq interval:%u.\n",
	         rx_itr, tx_itr);

l_out:
	return ret;
}

s32 sxe_irq_coalesce_set(struct net_device *netdev, struct ethtool_coalesce *user)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_irq_data *irq_data = adapter->irq_ctxt.irq_data[0];
	u16 tx_itr;
	u16 rx_itr;
	u16 tx_itr_old;
	u8 i;
	bool is_mixed;
	bool need_rst = false;
	u32 itr_max = SXE_EITR_ITR_MAX;
	s32 ret = 0;

	if ((user->rx_coalesce_usecs > itr_max) ||
	    (user->tx_coalesce_usecs > itr_max)) {
		ret = -EINVAL;
		LOG_ERROR_BDF("user param invalid, rx_coalesce_usecs:%u"
			  "tx_coalesce_usecs:%u max:%u.(err:%d)\n",
			  user->rx_coalesce_usecs,
			  user->tx_coalesce_usecs,
			  itr_max, ret);
		goto l_out;
	}

	if (irq_data->tx.list.cnt && irq_data->rx.list.cnt) {
		is_mixed = true;
	} else {
		is_mixed = false;
	}

	if (is_mixed) {
		if (user->tx_coalesce_usecs) {
			ret = -EINVAL;
			LOG_ERROR_BDF("irq_idx:0 bind tx ring cnt:%u rx ring cnt:%u"
				  "tx_coalesce_usecs:%u rx_coalesce_usecs:%u."
				  "(err:%d)\n ",
				  irq_data->tx.list.cnt,
				  irq_data->rx.list.cnt,
				  user->tx_coalesce_usecs,
				  user->rx_coalesce_usecs,
				  ret);
			goto l_out;
		}
		tx_itr_old = adapter->irq_ctxt.rx_irq_interval;
	} else {
		tx_itr_old = adapter->irq_ctxt.tx_irq_interval;
	}

	if (user->rx_coalesce_usecs == SXE_IRQ_ITR_CONSTANT_MODE_VALUE) {
		adapter->irq_ctxt.rx_irq_interval = SXE_IRQ_ITR_CONSTANT_MODE_VALUE;
		rx_itr = SXE_IRQ_ITR_20K;
	} else {
		adapter->irq_ctxt.rx_irq_interval = user->rx_coalesce_usecs << SXE_EITR_ITR_SHIFT;
		rx_itr = adapter->irq_ctxt.rx_irq_interval;
	}

	if (user->tx_coalesce_usecs == SXE_IRQ_ITR_CONSTANT_MODE_VALUE) {
		adapter->irq_ctxt.tx_irq_interval = SXE_IRQ_ITR_CONSTANT_MODE_VALUE;
		tx_itr = SXE_IRQ_ITR_12K;
	} else {
		adapter->irq_ctxt.tx_irq_interval = user->tx_coalesce_usecs << SXE_EITR_ITR_SHIFT;
		tx_itr = adapter->irq_ctxt.tx_irq_interval;
	}

	if (is_mixed) {
		adapter->irq_ctxt.tx_irq_interval = adapter->irq_ctxt.rx_irq_interval;
	}

	if (!!adapter->irq_ctxt.tx_irq_interval != !!tx_itr_old) {
		need_rst = true;
	}

	need_rst |= sxe_lro_status_update(adapter);

	for (i = 0; i < adapter->irq_ctxt.ring_irq_num; i++) {
		irq_data = adapter->irq_ctxt.irq_data[i];
		if (irq_data->tx.list.cnt && !irq_data->rx.list.cnt) {
			irq_data->irq_interval = tx_itr;
		} else {
			irq_data->irq_interval = rx_itr;
		}

		hw->irq.ops->ring_irq_interval_set(hw, i, irq_data->irq_interval);
	}

	if (need_rst) {
		sxe_do_reset(netdev);
	}

	LOG_INFO_BDF("user tx_coalesce_usecs:%u rx_coalesce_usecs:%u "
		     "adapter tx_irq_interval:%u rx_irq_interval:%u "
		     "tx_itr:%u rx_itr:%u need_rst:%u is_misxed:%u.\n",
		     user->tx_coalesce_usecs,
		     user->rx_coalesce_usecs,
		     adapter->irq_ctxt.tx_irq_interval,
		     adapter->irq_ctxt.rx_irq_interval,
		     tx_itr,
		     rx_itr,
		     need_rst,
		     is_mixed);

l_out:
	return ret;
}

static void sxe_packet_size_cal(u32 *size)
{
	if ((*size) <= 60) {
		(*size) = 5120;
	} else if ((*size) <= 316) {
		(*size) *= 40;
		(*size) += 2820;
	} else if ((*size) <= 1084) {
		(*size) *= 15;
		(*size) += 11452;
	} else if ((*size) < 1968) {
		(*size) *= 5;
		(*size) += 22420;
	} else {
		(*size) = 32256;
	}

	return;
}

static u32 sxe_itr_rate_cal(u32 size, u32 link_speed)
{
	u32 ret = 0;

	switch (link_speed) {
	case SXE_LINK_SPEED_10_FULL:
	case SXE_LINK_SPEED_1GB_FULL:
		size = (size > 8064) ? 8064 : size;
		ret += DIV_ROUND_UP(size, SXE_IRQ_ITR_INC_MIN * 64) *
				    SXE_IRQ_ITR_INC_MIN;
		break;

	default: 
		ret += DIV_ROUND_UP(size, SXE_IRQ_ITR_INC_MIN * 256) *
				    SXE_IRQ_ITR_INC_MIN;
		break;
	}

	return ret;
}

static void sxe_irq_interval_update(struct sxe_irq_data *irq_data,
					struct sxe_irq_rate *rate)
{
	u32 itr;
	u32 size;
	u32 packets = rate->total_packets;
	u32 bytes = rate->total_bytes;
	u16 old_itr = irq_data->irq_interval;
	u8  old_itr_tmp = rate->irq_interval;
	unsigned long cur = jiffies;

	if (time_after(cur, rate->next_update)) {
		itr = SXE_IRQ_ITR_MIN | SXE_IRQ_ITR_LATENCY;
		goto update;
	}

	if (!packets) {
		itr = (old_itr >> 2) + SXE_IRQ_ITR_INC_MIN;
		itr = (itr > SXE_IRQ_ITR_MAX) ? SXE_IRQ_ITR_MAX : itr;
		itr += (old_itr_tmp & SXE_IRQ_ITR_LATENCY);
		goto update;
	} else if ((packets < SXE_IRQ_ITR_PKT_4) && (bytes < SXE_IRQ_ITR_BYTES_9000)) {
		itr = SXE_IRQ_ITR_LATENCY;
		goto adjust;
	} else {
		if (packets < SXE_IRQ_ITR_PKT_48) {
			itr = (old_itr >> 2) + SXE_IRQ_ITR_INC_MIN;
			itr = (itr > SXE_IRQ_ITR_MAX) ? SXE_IRQ_ITR_MAX : itr;
		} else if (packets < SXE_IRQ_ITR_PKT_96) {
			itr = old_itr >> 2;
		} else if (packets < SXE_IRQ_ITR_PKT_256) {
			itr = old_itr >> 3;
			itr = (itr < SXE_IRQ_ITR_MIN) ? SXE_IRQ_ITR_MIN : itr;
		} else {
			itr = SXE_IRQ_ITR_BULK;
			goto adjust;
		}

		goto update;
	}

adjust:
	size = bytes / packets;
	sxe_packet_size_cal(&size);

	if (itr & SXE_IRQ_ITR_LATENCY) {
		size >>= 1;
	}

	itr += sxe_itr_rate_cal(size, irq_data->adapter->link.speed);

update:
	rate->irq_interval = itr;
	rate->next_update = cur + 1;
	rate->total_bytes = 0;
	rate->total_packets = 0;

	return;
}

static void sxe_irq_rate_adjust(struct sxe_irq_data *irq_data)
{
	u32 irq_data_itr;
	struct sxe_irq_rate *tx_rate = &irq_data->tx.irq_rate;
	struct sxe_irq_rate *rx_rate = &irq_data->rx.irq_rate;
	struct sxe_adapter *adapter = irq_data->adapter;
	struct sxe_hw *hw = &adapter->hw;

	if (irq_data->tx.list.cnt) {
		sxe_irq_interval_update(irq_data, tx_rate);
	}

	if (irq_data->rx.list.cnt) {
		sxe_irq_interval_update(irq_data, rx_rate);
	}

	irq_data_itr = min(tx_rate->irq_interval, rx_rate->irq_interval);
	irq_data_itr &= ~SXE_IRQ_ITR_LATENCY;

	irq_data_itr <<= SXE_EITR_ITR_SHIFT;

	if (irq_data_itr != irq_data->irq_interval) {
		irq_data->irq_interval = irq_data_itr;
		hw->irq.ops->ring_irq_interval_set(hw, irq_data->irq_idx,
						irq_data->irq_interval);
	}

	return;
}

int sxe_poll(struct napi_struct *napi, int weight)
{
	struct sxe_ring *ring;
	bool clean_complete = true;
	struct sxe_irq_data *irq_data = container_of(napi,
				        struct sxe_irq_data, napi);
	struct sxe_adapter *adapter = irq_data->adapter;
	struct sxe_hw *hw = &adapter->hw;
	s32 per_ring_budget;
	u32 cleaned;
	s32 total_cleaned = 0;

#ifdef SXE_TPH_CONFIGURE
	if (adapter->cap & SXE_TPH_ENABLE){
		sxe_tph_update(irq_data);
	}
#endif

	sxe_for_each_ring(ring, irq_data->tx.list) {
		clean_complete =
			  sxe_tx_ring_irq_clean(irq_data, ring, weight);

		LOG_DEBUG_BDF("tx ring[%u] clean_complete:%s\n", ring->idx,
			clean_complete ? "true" : "false");
	}
#ifdef HAVE_AF_XDP_ZERO_COPY
	ring = irq_data->tx.xdp_ring;
	if (ring) {
		if (ring->xsk_pool) {
			LOG_DEBUG_BDF("ring[%u] has xsk_umem, clean xdp tx irq\n",
				ring->idx);
			clean_complete = sxe_xdp_tx_ring_irq_clean(irq_data, ring, weight);
		}
	}
#endif
	if (weight <= 0) {
		LOG_DEBUG_BDF("weight:%d\n", weight);
		return weight;
	}

	per_ring_budget = max(weight / irq_data->rx.list.cnt, 1);
	LOG_DEBUG_BDF("rings in irq=%u, per_ring_budget=%d\n",
			irq_data->rx.list.cnt, per_ring_budget);

	sxe_for_each_ring(ring, irq_data->rx.list) {
#ifdef HAVE_AF_XDP_ZERO_COPY
		cleaned = ring->xsk_pool ?
				sxe_zc_rx_ring_irq_clean(irq_data, ring,
						per_ring_budget) :
				sxe_rx_ring_irq_clean(irq_data, ring,
						 per_ring_budget);
#else
		cleaned = sxe_rx_ring_irq_clean(irq_data, ring, per_ring_budget);
#endif
		total_cleaned += cleaned;
		if (cleaned >= per_ring_budget) {
			clean_complete = false;
		}
		LOG_DEBUG_BDF("ring[%u] %s cleaned = %u, total_cleaned = %u\n",
			ring->idx,
#ifdef HAVE_AF_XDP_ZERO_COPY
			ring->xsk_pool ? "xdp" : "",
#else
			"",
#endif
			cleaned, total_cleaned);
	}

	if (false == clean_complete) {
		LOG_DEBUG_BDF("not cleaned, rescheduling\n");
		return weight;
	}

	if (likely(napi_complete_done(napi, total_cleaned))) {
		if (adapter->irq_ctxt.rx_irq_interval == SXE_IRQ_ITR_CONSTANT_MODE_VALUE) {
			sxe_irq_rate_adjust(irq_data);
		}

		if (!test_bit(SXE_DOWN, &adapter->state)) {
			hw->irq.ops->ring_irq_enable(hw,
				BIT_ULL(irq_data->irq_idx));
		}
	}

	return min(total_cleaned, (weight - 1));
}
