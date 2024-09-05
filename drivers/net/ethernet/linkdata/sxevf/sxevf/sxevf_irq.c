
#include <linux/numa.h>
#include <linux/kernel.h>
#include <linux/rcupdate.h>
#include <linux/ethtool.h>

#include "sxevf.h"
#ifdef HAVE_NO_OVERFLOW_H
#include <sxe_compat_overflow.h>
#else
#include <linux/overflow.h>
#endif
#include "sxevf_irq.h"
#include "sxe_log.h"
#include "sxevf_monitor.h"
#include "sxevf_rx_proc.h"
#include "sxevf_tx_proc.h"
#include "sxevf_netdev.h"

#ifdef NETIF_NAPI_ADD_API_NEED_3_PARAMS
static inline void
netif_napi_add_compat(struct net_device *dev, struct napi_struct *napi,
			int (*poll)(struct napi_struct *, int), int weight)
{
	netif_napi_add(dev, napi, poll);
}

#define netif_napi_add(dev, napi, poll, weight) netif_napi_add_compat(dev, napi, poll, weight)
#endif

s32 sxevf_irq_coalesce_get(struct net_device * netdev,
				struct ethtool_coalesce *user)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	u16 rx_itr = adapter->irq_ctxt.rx_irq_interval;
	u16 tx_itr = adapter->irq_ctxt.tx_irq_interval;
	struct sxevf_irq_data *irq_data = adapter->irq_ctxt.irq_data[0];
	bool is_mixed;
	s32 ret = 0;


	if (irq_data->tx.list.cnt && irq_data->rx.list.cnt) {
		is_mixed = true;
	} else {
		is_mixed = false;
	}

	if (rx_itr == SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE) {
		user->rx_coalesce_usecs = SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE;
	} else {
		user->rx_coalesce_usecs = rx_itr >> SXEVF_EITR_ITR_SHIFT;
	}

	if (is_mixed) {
		LOG_INFO_BDF("interrupt 0 has both rx and tx ring, "
			 "just report rx itr:%u.\n",
			 user->rx_coalesce_usecs);
		goto l_out;
	}

	if (tx_itr == SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE) {
		user->tx_coalesce_usecs = SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE;
	} else {
		user->tx_coalesce_usecs = tx_itr >> SXEVF_EITR_ITR_SHIFT;
	}

	LOG_INFO_BDF("rx irq interval:%u tx irq interval:%u.\n",
	         rx_itr, tx_itr);

l_out:
	return ret;
}

s32 sxevf_irq_coalesce_set(struct net_device *netdev, struct ethtool_coalesce *user)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	struct sxevf_hw *hw = &adapter->hw;
	struct sxevf_irq_data *irq_data = adapter->irq_ctxt.irq_data[0];
	u16 tx_itr, tx_itr_old;
	u16 rx_itr;
	u8 i;
	bool is_mixed;
	bool need_rst = false;
	u32 itr_max = SXEVF_EITR_ITR_MAX;
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

	if(is_mixed) {
		if (user->tx_coalesce_usecs) {
			ret = -EINVAL;
			LOG_ERROR_BDF("irq both has rx and rx ring, rx_coalesce_usecs:%u"
				"tx_coalesce_usecs:%u invalid.(err:%d)\n",
				user->rx_coalesce_usecs,
				user->tx_coalesce_usecs,
				ret);
			goto l_out;
		}
		tx_itr_old = adapter->irq_ctxt.rx_irq_interval;
	} else {
		tx_itr_old = adapter->irq_ctxt.rx_irq_interval;
	}

	if (user->rx_coalesce_usecs > SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE) {
		adapter->irq_ctxt.rx_irq_interval = user->rx_coalesce_usecs <<
			SXEVF_EITR_ITR_SHIFT;
	} else {
		adapter->irq_ctxt.rx_irq_interval = user->rx_coalesce_usecs;
	}

	if (adapter->irq_ctxt.rx_irq_interval == SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE) {
		rx_itr = SXEVF_IRQ_INTERVAL_20K;
	} else {
		rx_itr = adapter->irq_ctxt.rx_irq_interval;
	}

	if (user->tx_coalesce_usecs > SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE) {
		adapter->irq_ctxt.tx_irq_interval = user->tx_coalesce_usecs <<
			SXEVF_EITR_ITR_SHIFT;
	} else {
		adapter->irq_ctxt.tx_irq_interval = user->tx_coalesce_usecs;
	}

	if (adapter->irq_ctxt.tx_irq_interval == SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE) {
		tx_itr = SXEVF_IRQ_INTERVAL_12K;
	} else {
		tx_itr = adapter->irq_ctxt.tx_irq_interval;
	}

	if (is_mixed) {
		adapter->irq_ctxt.tx_irq_interval = adapter->irq_ctxt.rx_irq_interval;
	}

	if (!!adapter->irq_ctxt.tx_irq_interval != !!tx_itr_old) {
		need_rst = true;
	}

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
		if (netif_running(netdev)) {
			sxevf_hw_reinit(adapter);
		} else {
			sxevf_reset(adapter);
		}
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

static void sxevf_irq_num_init(struct sxevf_adapter *adapter)
{
	u16 ring_irq;
	u16 cpu_cnt = num_online_cpus();

	ring_irq = max(adapter->rx_ring_ctxt.num, adapter->tx_ring_ctxt.num);

	ring_irq = min_t(u16, ring_irq, cpu_cnt);

	adapter->irq_ctxt.total_irq_num = ring_irq + SXEVF_NON_QUEUE_IRQ_NUM;

	adapter->irq_ctxt.ring_irq_num = ring_irq;

	LOG_INFO_BDF("msi-x interrupt rxr_num:%u txr_num:%u "
		 "xdp_num:%u cpu cnt:%u "
		 "total_irq_num:%u ring_irq_num:%u\n",
		adapter->rx_ring_ctxt.num,
		adapter->tx_ring_ctxt.num,
		adapter->xdp_ring_ctxt.num,
		cpu_cnt,
		adapter->irq_ctxt.total_irq_num,
		adapter->irq_ctxt.ring_irq_num);

	return;
}

static s32 sxevf_msix_irq_init(struct sxevf_adapter *adapter)
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
				    SXEVF_MIN_MSIX_IRQ_NUM, total);
	if (ret < 0) { 
		SXEVF_KFREE(adapter->irq_ctxt.msix_entries);

		LOG_DEV_ERR("min:%u max:%u pci enable msi-x failed.(err:%d)\n",
			 SXEVF_MIN_MSIX_IRQ_NUM, total, ret);
	} else {
		if (ret != total) {
			adapter->irq_ctxt.total_irq_num = ret;
			adapter->irq_ctxt.ring_irq_num = ret -
					SXEVF_NON_QUEUE_IRQ_NUM;
		}
		LOG_INFO_BDF("enable pci msi-x success.result:%d maxCnt:%u"
			 " total irq num:%u ring irq num:%u\n",
			  ret, total,
			  adapter->irq_ctxt.total_irq_num,
			  adapter->irq_ctxt.ring_irq_num);

		ret = 0;
	}

l_out:
	return ret;
}

static void sxevf_irq_data_free(struct sxevf_adapter *adapter,
					    u16 irq_idx)
{
	u16 idx;
	struct sxevf_irq_data *irq_data = adapter->irq_ctxt.irq_data[irq_idx];
	struct sxevf_ring *ring;

	sxevf_for_each_ring(ring, irq_data->tx.list) {
		adapter->tx_ring_ctxt.ring[ring->idx] = NULL;
	}

	if (irq_data->tx.xdp_ring) {
		idx = irq_data->tx.xdp_ring->idx;
		adapter->xdp_ring_ctxt.ring[idx] = NULL;
	}

	sxevf_for_each_ring(ring, irq_data->rx.list) {
		adapter->rx_ring_ctxt.ring[ring->idx] = NULL;
	}

	adapter->irq_ctxt.irq_data[irq_idx] = NULL;

	netif_napi_del(&irq_data->napi);
	kfree_rcu(irq_data, rcu);

	return ;
}

static void sxevf_all_irq_data_free(struct sxevf_adapter *adapter)
{
	u16 irq_idx = adapter->irq_ctxt.ring_irq_num;

	while (irq_idx--) {
		sxevf_irq_data_free(adapter, irq_idx);
	}

	return;
}

static s32 sxevf_irq_data_alloc(struct sxevf_adapter *adapter,
				  u16 total_count, u16 irq_idx)
{
	s32 ret = 0;
	struct sxevf_irq_data *irq_data;

	irq_data = kzalloc(struct_size(irq_data, ring, total_count),
			  GFP_KERNEL);
	if (irq_data == NULL) {
		ret = -ENOMEM;
		LOG_ERROR_BDF("alloc interrupt data and ring resource "
			  "failed. size: %ld irq_idx:%u "
			  "ring count:%u.(err:%d)\n",
			  struct_size(irq_data, ring, total_count),
			  irq_idx, total_count, ret);
		goto l_out;
	}

	netif_napi_add(adapter->netdev, &irq_data->napi, sxevf_poll,
		       SXEVF_NAPI_WEIGHT);

	adapter->irq_ctxt.irq_data[irq_idx] = irq_data;
	irq_data->adapter = adapter;
	irq_data->irq_idx = irq_idx;

l_out:
	return ret;
}

static void sxevf_irq_interval_init(struct sxevf_irq_context  *irq_ctxt,
				u16 irq_idx, u16 txr_cnt, u16 rxr_cnt)
{
	struct sxevf_irq_data *irq_data = irq_ctxt->irq_data[irq_idx];

	if (txr_cnt && !rxr_cnt) {
		if (irq_ctxt->tx_irq_interval == SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE) {
			irq_data->irq_interval = SXEVF_IRQ_INTERVAL_12K;
		} else {
			irq_data->irq_interval = irq_ctxt->tx_irq_interval;
		}
	} else {
		if (irq_ctxt->rx_irq_interval == SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE) {
			irq_data->irq_interval = SXEVF_IRQ_INTERVAL_20K;
		} else {
			irq_data->irq_interval = irq_ctxt->rx_irq_interval;
		}
	}

	irq_data->tx.irq_rate.irq_interval = SXEVF_LOWEST_LATENCY;
	irq_data->rx.irq_rate.irq_interval = SXEVF_LOWEST_LATENCY;

	LOG_INFO("irq_idx:%u irq level interval:%u "
		     "list level rx irq interval:%u tx irq interval:%u\n",
		     irq_idx, irq_data->irq_interval,
		     irq_data->rx.irq_rate.irq_interval,
		     irq_data->tx.irq_rate.irq_interval);

	return;
}

static s32 sxevf_irq_ring_bind(struct sxevf_adapter *adapter)
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

	if (irq_num >= total_ring) {
		for (; rxr_remain > 0; irq_idx++, irq_num--) {
			u16 rxr_cnt = DIV_ROUND_UP(txr_remain, irq_num);

			ret = sxevf_irq_data_alloc(adapter, rxr_cnt, irq_idx);
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

			sxevf_irq_interval_init(&adapter->irq_ctxt, irq_idx, 0, 1);

			sxevf_rx_ring_init(adapter, 0, 1, rxr_idx,
					   irq_idx, rxr_idx);

			rxr_remain -= rxr_cnt;
			rxr_idx += rxr_cnt;
		}
		LOG_INFO_BDF("alloc rx irq resource priority done.irq_idx:%u "
			  "rxr_idx:%u txr_remain:%u rxr_remain:%u xdp_remain:%u"
			  " ring_irq_num:%u total_ring:%u \n",
			  irq_idx, rxr_idx, txr_remain, rxr_remain,
			  xdp_remain, irq_num, total_ring);
	}

	for (; irq_num; irq_idx++, irq_num--) {
		u16 txr_cnt = DIV_ROUND_UP(txr_remain, irq_num);

		u16 xdp_cnt = DIV_ROUND_UP(xdp_remain, irq_num);

		u16 rxr_cnt = DIV_ROUND_UP(rxr_remain, irq_num);
		u16 tx_reg_idx = txr_idx + xdp_idx;
		u16 xdp_reg_idx = txr_cnt ? (tx_reg_idx + 1) : tx_reg_idx;

		total_ring = txr_cnt + xdp_cnt + rxr_cnt;

		LOG_DEBUG_BDF("irq_num:%u irq_idx:%u txr_cnt:%u xdp_cnt:%u"
			  " rxr_cnt:%u base txr_idx:%u xdp_idx:%u"
			  " rxr_idx:%u \n",
			  irq_num, irq_idx, txr_cnt, xdp_cnt, rxr_cnt,
			  txr_idx, xdp_idx, rxr_idx);

		ret = sxevf_irq_data_alloc(adapter, total_ring, irq_idx);
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

		sxevf_irq_interval_init(&adapter->irq_ctxt, irq_idx, txr_cnt,
				     rxr_cnt);


		sxevf_tx_ring_init(adapter, 0, txr_cnt, txr_idx,
				   irq_idx, tx_reg_idx);

		sxevf_xdp_ring_init(adapter, txr_cnt, xdp_cnt,
				    xdp_idx, irq_idx, xdp_reg_idx);

		sxevf_rx_ring_init(adapter, txr_cnt + xdp_cnt, rxr_cnt,
				   rxr_idx, irq_idx, rxr_idx);

		txr_remain -= txr_cnt;
		xdp_remain -= xdp_cnt;
		rxr_remain -= rxr_cnt;

		txr_idx += txr_cnt;
		xdp_idx += xdp_cnt;
		rxr_idx += rxr_cnt;
	}

	return ret;

l_error:
	adapter->irq_ctxt.ring_irq_num = 0;
	adapter->tx_ring_ctxt.num = 0;
	adapter->rx_ring_ctxt.num = 0;
	adapter->xdp_ring_ctxt.num = 0;

	while(irq_idx--) {
		sxevf_irq_data_free(adapter, irq_idx);
	}

	return ret;

}

static void sxevf_pci_irq_disable(struct sxevf_adapter *adapter)
{
	pci_disable_msix(adapter->pdev);
	SXEVF_KFREE(adapter->irq_ctxt.msix_entries);

	return;
}

void sxevf_hw_irq_disable(struct sxevf_adapter *adapter)
{
	u16 i;
	struct sxevf_hw *hw = &adapter->hw;
	struct sxevf_irq_context *irq = &adapter->irq_ctxt;

	hw->irq.ops->irq_disable(hw);

	for (i = 0; i < adapter->irq_ctxt.ring_irq_num; i++) {
		synchronize_irq(irq->msix_entries[i].vector);
	}

	synchronize_irq(irq->msix_entries[i].vector);

	return;
}

void sxevf_irq_release(struct sxevf_adapter *adapter)
{
	u16 irq_idx;
	struct sxevf_irq_context *irq_ctxt = &adapter->irq_ctxt;

	if (!irq_ctxt->msix_entries) {
		goto l_out;
	}

	for (irq_idx = 0; irq_idx < irq_ctxt->ring_irq_num; irq_idx++) {
		struct sxevf_irq_data *irq_data = irq_ctxt->irq_data[irq_idx];
		struct msix_entry *entry = &irq_ctxt->msix_entries[irq_idx];

		if (!irq_data->rx.list.next &&
		    !irq_data->tx.list.next &&
		    !irq_data->tx.xdp_ring) {
			continue;
		}

		free_irq(entry->vector, irq_data);
	}

	free_irq(irq_ctxt->msix_entries[irq_idx].vector, adapter);

l_out:
	return;
}

s32 sxevf_irq_ctxt_init(struct sxevf_adapter *adapter)
{
	s32 ret;

	adapter->irq_ctxt.rx_irq_interval = SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE;
	adapter->irq_ctxt.tx_irq_interval = SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE;

	sxevf_irq_num_init(adapter);

	ret = sxevf_msix_irq_init(adapter);
	if (ret) {
		LOG_DEV_ERR("msix irq init fail.(err:%d)\n",
			   ret);
		goto l_out;
	}

	ret = sxevf_irq_ring_bind(adapter);
	if (ret) {
		LOG_DEV_ERR("interrupt and ring bind fail.(err:%d)\n", ret);
		goto l_disable_irq;
	}

	LOG_INFO_BDF("adapter rx_irq_interval:%u tx_irq_interval:%u.\n",
		    adapter->irq_ctxt.rx_irq_interval,
		    adapter->irq_ctxt.tx_irq_interval);

l_out:
	return ret;

l_disable_irq:
	sxevf_pci_irq_disable(adapter);
	return ret;

}

void sxevf_irq_ctxt_exit(struct sxevf_adapter *adapter)
{
	sxevf_all_irq_data_free(adapter);

	sxevf_pci_irq_disable(adapter);

	adapter->irq_ctxt.ring_irq_num = 0;
	adapter->tx_ring_ctxt.num = 0;
	adapter->rx_ring_ctxt.num = 0;
	adapter->xdp_ring_ctxt.num = 0;

	return;
}

static bool sxevf_set_irq_name(struct sxevf_irq_data *irq_data,
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

STATIC irqreturn_t sxevf_ring_irq_handler(int irq, void *data)
{
	struct sxevf_irq_data *irq_data = data;

	if (irq_data->tx.list.next ||
	    irq_data->rx.list.next ||
	    irq_data->tx.xdp_ring) {
		napi_schedule_irqoff(&irq_data->napi);
	}

	return IRQ_HANDLED ;
}

STATIC irqreturn_t sxevf_event_irq_handler(int irq, void *data)
{
	struct sxevf_adapter *adapter = data;
	struct sxevf_hw *hw = &adapter->hw;

	set_bit(SXEVF_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state);

	sxevf_monitor_work_schedule(adapter);

	hw->irq.ops->specific_irq_enable(hw, adapter->irq_ctxt.mailbox_irq);

	LOG_INFO_BDF("rcv event irq:%d\n", irq);

	return IRQ_HANDLED;
}

static s32 sxevf_msix_request_irqs(struct sxevf_adapter *adapter)
{
	s32 ret;
	u16 rx_idx = 0;
	u16 tx_idx = 0;
	u16 irq_idx;
	struct sxevf_irq_data *irq_data;
	struct msix_entry   *entry;
	struct net_device *netdev = adapter->netdev;
	struct sxevf_irq_context *irq_ctxt = &adapter->irq_ctxt;

	if (!irq_ctxt->ring_irq_num) {
		ret = -SXEVF_ERR_IRQ_NUM_INVALID;
		LOG_ERROR_BDF("irq_num:%d request irq fail, invalid retry open"
			  "need reload ko.(err:%d)\n",
			  irq_ctxt->ring_irq_num, ret);
		goto l_out;
	}

	for (irq_idx = 0; irq_idx < irq_ctxt->ring_irq_num; irq_idx++) {
		irq_data = irq_ctxt->irq_data[irq_idx];
		entry = &irq_ctxt->msix_entries[irq_idx];

		if (!(sxevf_set_irq_name(irq_data, netdev->name,
				       &rx_idx, &tx_idx))) {
			continue;
		}

		ret = request_irq(entry->vector, &sxevf_ring_irq_handler, 0,
				  irq_data->name, irq_data);
		if (ret) {
			LOG_DEV_ERR("irq_idx:%u rx_idx:%u tx_idx:%u irq_num:%u "
				  "vector:%u msi-x ring interrupt "
				  "reuqest fail.(err:%d)\n",
				  irq_idx, rx_idx, tx_idx,
				  irq_ctxt->ring_irq_num,
				  entry->vector, ret);
			goto l_free_irq;
		}

	}

	ret = request_irq(irq_ctxt->msix_entries[irq_idx].vector,
			 sxevf_event_irq_handler, 0, netdev->name, adapter);
	if (ret) {
		LOG_DEV_ERR("irq_idx:%u vector:%u msi-x other interrupt "
			  "reuqest fail.(err:%d)\n",
			  irq_idx,
			  irq_ctxt->msix_entries[irq_idx].vector, ret);
		goto l_free_irq;
	}

l_out:
	return ret;

l_free_irq:
	while (irq_idx) {
		irq_idx--;
		free_irq(irq_ctxt->msix_entries[irq_idx].vector,
			irq_ctxt->irq_data[irq_idx]);
	}

	return ret;
}

void sxevf_configure_msix_hw(struct sxevf_adapter *adapter)
{
	u16 irq_idx;
	struct sxevf_hw *hw = &adapter->hw;
	struct sxevf_ring *ring;
	struct sxevf_irq_context *irq_ctxt = &adapter->irq_ctxt;
	irq_ctxt->irq_mask = 0;

	for (irq_idx = 0; irq_idx < irq_ctxt->ring_irq_num; irq_idx++) {
		struct sxevf_irq_data *irq_data = irq_ctxt->irq_data[irq_idx];

		sxevf_for_each_ring(ring, irq_data->rx.list) {
			hw->irq.ops->ring_irq_map(hw, false, ring->reg_idx,
						  irq_idx);
		}

		sxevf_for_each_ring(ring, irq_data->tx.list) {
			hw->irq.ops->ring_irq_map(hw, true, ring->reg_idx,
						  irq_idx);
		}

		if (irq_data->tx.xdp_ring) {
			hw->irq.ops->ring_irq_map(hw, true,
						irq_data->tx.xdp_ring->reg_idx,
						irq_idx);
		}

		hw->irq.ops->ring_irq_interval_set(hw, irq_idx, irq_data->irq_interval);
		irq_ctxt->irq_mask |= BIT(irq_idx);

	}

	irq_ctxt->mailbox_irq = BIT(irq_idx);
	irq_ctxt->irq_mask |= BIT(irq_idx);

	hw->irq.ops->event_irq_map(hw, irq_idx);

	return;
}

static void sxevf_napi_enable_all(struct sxevf_adapter * adapter)
{
	u16 irq_idx;

	for (irq_idx = 0; irq_idx < adapter->irq_ctxt.ring_irq_num; irq_idx++) {
		napi_enable(&(adapter->irq_ctxt.irq_data[irq_idx]->napi));
	}

	return;
}

void sxevf_napi_disable(struct sxevf_adapter *adapter)
{
	u16 irq_idx;

	for (irq_idx = 0; irq_idx < adapter->irq_ctxt.ring_irq_num; irq_idx++) {
		napi_disable(&adapter->irq_ctxt.irq_data[irq_idx]->napi);
	}

	return;
}

void sxevf_hw_irq_configure(struct sxevf_adapter *adapter)
{
	struct sxevf_hw *hw = &adapter->hw;

	sxevf_configure_msix_hw(adapter);

	smp_mb__before_atomic();
	clear_bit(SXEVF_DOWN, &adapter->state);

	sxevf_napi_enable_all(adapter);

	hw->irq.ops->pending_irq_clear(hw);

	hw->irq.ops->irq_enable(hw, adapter->irq_ctxt.irq_mask);

	return;
}

s32 sxevf_irq_configure(struct sxevf_adapter *adapter)
{
	s32 ret;

	ret = sxevf_msix_request_irqs(adapter);
	if (ret) {
		LOG_DEV_ERR("irq_num:%d msi-x request irq failed, (err:%d)\n",
			    adapter->irq_ctxt.ring_irq_num, ret);
		goto l_out;
	}

	sxevf_hw_irq_configure(adapter);

l_out:
	return ret;
}

static void sxevf_irq_interval_update(struct sxevf_irq_data *irq_data,
					struct sxevf_irq_rate *rate)
{
	u32 bytes = rate->total_bytes;
	u32 packets = rate->total_packets;
	u16 old_irq_itr = irq_data->irq_interval >> SXEVF_EITR_ITR_SHIFT;
	u64 bytes_rate;
	u16 itr = rate->irq_interval;

	if ((packets == 0) || (old_irq_itr == 0)) {
		goto l_end;
	}

	bytes_rate = bytes / old_irq_itr;
	switch (itr) {
	case SXEVF_LOWEST_LATENCY:
		if (bytes_rate > SXEVF_LOW_LATENCY_BYTE_RATE_MIN) {
			itr = SXEVF_LOW_LATENCY;
		}
		break;
	case SXEVF_LOW_LATENCY:
		if (bytes_rate > SXEVF_BULK_LATENCY_BYTE_RATE_MIN) {
			itr = SXEVF_BULK_LATENCY;
		} else if (bytes_rate <= SXEVF_LOW_LATENCY_BYTE_RATE_MIN) {
			itr = SXEVF_LOWEST_LATENCY;
		}
		break;
	case SXEVF_BULK_LATENCY:
		if (bytes_rate <=  SXEVF_BULK_LATENCY_BYTE_RATE_MIN) {
			itr = SXEVF_LOW_LATENCY;
		}
		break;
	}

	rate->total_bytes = 0;
	rate->total_packets = 0;

	rate->irq_interval = itr;

l_end:
	return;

}

static void sxevf_irq_rate_adjust(struct sxevf_irq_data *irq_data)
{
	u16 curr_itr;
	u16 new_itr = irq_data->irq_interval;
	struct sxevf_irq_rate *tx_rate = &irq_data->tx.irq_rate;
	struct sxevf_irq_rate *rx_rate = &irq_data->rx.irq_rate;
	struct sxevf_adapter *adapter = irq_data->adapter;
	struct sxevf_hw *hw = &adapter->hw;

	if (irq_data->tx.list.cnt) {
		sxevf_irq_interval_update(irq_data, tx_rate);
	}

	if (irq_data->rx.list.cnt) {
		sxevf_irq_interval_update(irq_data, rx_rate);
	}

	curr_itr = max(tx_rate->irq_interval, rx_rate->irq_interval);

	switch (curr_itr) {
	case SXEVF_LOWEST_LATENCY:
		new_itr = SXEVF_IRQ_INTERVAL_100K;
		break;
	case SXEVF_LOW_LATENCY:
		new_itr = SXEVF_IRQ_INTERVAL_20K;
		break;
	case SXEVF_BULK_LATENCY:
		new_itr = SXEVF_IRQ_INTERVAL_12K;
		break;
	}

	if (new_itr != irq_data->irq_interval) {
		new_itr = (10 * new_itr * irq_data->irq_interval) /
			  ((9 * new_itr) + irq_data->irq_interval);

		irq_data->irq_interval = new_itr;

		hw->irq.ops->ring_irq_interval_set(hw, irq_data->irq_idx,
						   irq_data->irq_interval);
	}

	return;
}

s32 sxevf_poll(struct napi_struct *napi, int weight)
{
	struct sxevf_irq_data *irq_data = container_of(napi,
				        struct sxevf_irq_data, napi);
	struct sxevf_adapter *adapter = irq_data->adapter;
	struct sxevf_hw *hw = &adapter->hw;
	struct sxevf_ring *ring;
	s32 per_ring_budget;
	s32 total_cleaned = 0;
	bool clean_complete = true;
	u32 cleaned = 0; 

	sxevf_for_each_ring(ring, irq_data->tx.list) {
		if (!sxevf_tx_ring_irq_clean(irq_data, ring, weight)) {
			clean_complete = false;
		}
	}

	ring = irq_data->tx.xdp_ring;
	if (ring) {
		if (!sxevf_xdp_ring_irq_clean(irq_data, ring, weight)) {
			clean_complete = false;
		}
	}

	if (weight <= 0) {
		return weight;
	}


	per_ring_budget = max(weight / irq_data->rx.list.cnt, 1);
	LOG_DEBUG_BDF("weight:%d rings in irq=%u, per_ring_budget=%d\n",
		   weight, irq_data->rx.list.cnt, per_ring_budget);

	sxevf_for_each_ring(ring, irq_data->rx.list) {
		cleaned = sxevf_rx_ring_irq_clean(irq_data, ring,
						   per_ring_budget);
		total_cleaned += cleaned;
		if (cleaned >= per_ring_budget) {
			clean_complete = false;
		}
	}

	if (!clean_complete) {
		LOG_WARN_BDF("weight:%d cleand:%u total_cleaned:%d "
			" per_ring_budget:%d not complete\n",
			weight, cleaned, total_cleaned, per_ring_budget);
		return weight;
	}

	if (likely(napi_complete_done(napi, total_cleaned))) {
		LOG_INFO_BDF("weight:%d cleand:%u total_cleaned:%d per_ring_budget:%d "
			" complete done\n",
			weight, cleaned, total_cleaned, per_ring_budget);
		if (adapter->irq_ctxt.rx_irq_interval == SXEVF_IRQ_ITR_CONSTANT_MODE_VALUE) {
			sxevf_irq_rate_adjust(irq_data);
		}

		if (!test_bit(SXEVF_DOWN, &adapter->state)) {
			hw->irq.ops->specific_irq_enable(hw,
				BIT_ULL(irq_data->irq_idx));
		}
	}

	return min(total_cleaned, weight - 1);
}
