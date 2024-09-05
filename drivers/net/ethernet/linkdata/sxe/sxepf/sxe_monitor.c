
#include <linux/moduleparam.h>

#include "sxe_hw.h"
#include "sxe_monitor.h"
#include "sxe_ptp.h"
#include "sxe_dcb.h"
#include "sxe_netdev.h"
#include "sxe_tx_proc.h"
#include "sxe_rx_proc.h"
#include "sxe_sriov.h"
#include "sxe_errno.h"

extern struct workqueue_struct *sxe_fnav_workqueue;

#define SXE_LINK_CHECK_WAIT_TIME (4 * HZ)
#define SXE_SFP_RESET_WAIT_TIME  (2 * HZ)

#define SXE_CHECK_LINK_TIMER_PERIOD   (HZ / 10)
#define SXE_NORMAL_TIMER_PERIOD       (HZ * 2)

#ifdef SXE_SFP_DEBUG
static unsigned int sw_sfp_multi_gb_ms = SXE_SW_SFP_MULTI_GB_MS;
#ifndef SXE_TEST
module_param(sw_sfp_multi_gb_ms, uint, 0);
MODULE_PARM_DESC(sw_sfp_multi_gb_ms, "Mask LOS_N interrupt(SDP1) time after active rate switching - default is 4000");
#endif 
#endif

void sxe_task_timer_trigger(struct sxe_adapter *adapter)
{
	set_bit(SXE_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state);
	LOG_DEBUG_BDF("trigger link_check subtask, state=%lx, monitor_state=%lx, is_up=%d\n",
		adapter->state, adapter->monitor_ctxt.state, adapter->link.is_up);

	adapter->link.check_timeout = jiffies;

	mod_timer(&adapter->monitor_ctxt.timer, jiffies);

	return;
}

void sxe_sfp_reset_task_submit(struct sxe_adapter *adapter)
{
	set_bit(SXE_SFP_NEED_RESET, &adapter->monitor_ctxt.state);
	LOG_INFO("trigger sfp_reset subtask\n");
	adapter->link.sfp_reset_timeout = 0;
	adapter->link.last_lkcfg_time = 0;
	adapter->link.sfp_multispeed_time = 0;

	return;
}

void sxe_monitor_work_schedule(struct sxe_adapter *adapter)
{
	struct workqueue_struct  *wq = sxe_workqueue_get();

	if (!test_bit(SXE_DOWN, &adapter->state) &&
		!test_bit(SXE_REMOVING, &adapter->state) &&
		!test_and_set_bit(SXE_MONITOR_WORK_SCHED,
				&adapter->monitor_ctxt.state)) {
		queue_work(wq, &adapter->monitor_ctxt.work);
	}

	return;
}

static void sxe_timer_cb(struct timer_list *timer)
{
	struct sxe_monitor_context *monitor = container_of(timer,
					struct sxe_monitor_context, timer);
	struct sxe_adapter *adapter = container_of(monitor, struct sxe_adapter,
					monitor_ctxt);
	unsigned long period ;

	if (test_bit(SXE_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state) ||
		test_bit(SXE_SFP_MULTI_SPEED_SETTING, &adapter->state)) {
		period = SXE_CHECK_LINK_TIMER_PERIOD;
	} else {
		period = SXE_NORMAL_TIMER_PERIOD;
	}

	mod_timer(&adapter->monitor_ctxt.timer, period + jiffies);

	sxe_monitor_work_schedule(adapter);

	return;
}

static void sxe_monitor_work_complete(struct sxe_adapter *adapter)
{
	BUG_ON(!test_bit(SXE_MONITOR_WORK_SCHED, &adapter->monitor_ctxt.state));

	smp_mb__before_atomic();
	clear_bit(SXE_MONITOR_WORK_SCHED, &adapter->monitor_ctxt.state);

	return;
}

static void sxe_reset_work(struct sxe_adapter *adapter)
{
	if (!test_and_clear_bit(SXE_RESET_REQUESTED, &adapter->monitor_ctxt.state)) {
		goto l_end;
	}

	rtnl_lock();
	if (test_bit(SXE_DOWN, &adapter->state) ||
	    test_bit(SXE_REMOVING, &adapter->state) ||
	    test_bit(SXE_RESETTING, &adapter->state)) {
		goto l_unlock;
	}

	LOG_DEV_ERR("reset adapter\n");
	adapter->stats.sw.reset_work_trigger_cnt++;

	sxe_hw_reinit(adapter);

l_unlock:
	rtnl_unlock();
l_end:
	return;
}

static void sxe_stats_update_work(struct sxe_adapter *adapter)
{
	if (test_bit(SXE_DOWN, &adapter->state) ||
	    test_bit(SXE_REMOVING, &adapter->state) ||
	    test_bit(SXE_RESETTING, &adapter->state)) {
		goto l_end;
	}

	stats_lock(adapter);
	sxe_stats_update(adapter);
	stats_unlock(adapter);

l_end:
	return;
}

static void sxe_check_hang_work(struct sxe_adapter *adapter)
{
	u32 i;
	u64 eics = 0;
	struct sxe_irq_data *irq_priv;
	struct sxe_hw   *hw = &adapter->hw;
	struct sxe_ring **tx_ring = adapter->tx_ring_ctxt.ring;
	struct sxe_ring **xdp_ring = adapter->xdp_ring_ctxt.ring;

	if (test_bit(SXE_DOWN, &adapter->state) ||
	    test_bit(SXE_REMOVING, &adapter->state) ||
	    test_bit(SXE_RESETTING, &adapter->state)) {
		goto l_end;
	}

	if (netif_carrier_ok(adapter->netdev)) {
		for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
			SXE_TX_HANG_CHECK_ACTIVE(tx_ring[i]);
		}
		for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
			SXE_TX_HANG_CHECK_ACTIVE(xdp_ring[i]);
		}
	}

	if (!(adapter->cap & SXE_MSIX_ENABLED)) {
		hw->irq.ops->event_irq_trigger(hw);
	} else {
		for (i = 0; i < adapter->irq_ctxt.ring_irq_num; i++) {
			irq_priv = adapter->irq_ctxt.irq_data[i];
			if (irq_priv->tx.list.next ||
				irq_priv->rx.list.next) {
				eics |= BIT_ULL(i);
			}
		}

		hw->irq.ops->ring_irq_trigger(hw, eics);
	}

l_end:
	return;
}

static void sxe_fc_configure(struct sxe_adapter *adapter)
{
	bool pfc_en = adapter->dcb_ctxt.cee_cfg.pfc_mode_enable;

#ifdef SXE_DCB_CONFIGURE
	if (adapter->dcb_ctxt.ieee_pfc) {
		pfc_en |= !!(adapter->dcb_ctxt.ieee_pfc->pfc_en);
	}
#endif

	if (!((adapter->cap & SXE_DCB_ENABLE) && pfc_en)) {
		LOG_DEBUG_BDF("lfc configure\n");
		sxe_fc_enable(adapter);
		sxe_rx_drop_mode_set(adapter);
	}
#ifdef SXE_DCB_CONFIGURE
	else {
		LOG_DEBUG_BDF("pfc configure\n");
		sxe_dcb_pfc_configure(adapter);
	}
#endif

	return;
}

static void sxe_vmac_configure(struct sxe_adapter *adapter)
{
	sxe_fc_configure(adapter);

	sxe_ptp_configure(adapter);

	return;
}

static void sxe_link_update(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	unsigned long flags;

	if (!test_bit(SXE_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state)) {
		goto l_end;
	}

	sxe_link_info_get(adapter, &adapter->link.speed,
					&adapter->link.is_up);

	LOG_DEBUG_BDF("link update, speed=%x, is_up=%d\n",
			adapter->link.speed, adapter->link.is_up);

	if (adapter->link.is_up) {
		sxe_vmac_configure(adapter);
	}

	if (adapter->link.is_up || \
		time_after(jiffies, (adapter->link.check_timeout +
				 SXE_LINK_CHECK_WAIT_TIME))) {
		clear_bit(SXE_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state);

		spin_lock_irqsave(&adapter->irq_ctxt.event_irq_lock, flags);
		hw->irq.ops->specific_irq_enable(hw, SXE_EIMS_LSC);
		spin_unlock_irqrestore(&adapter->irq_ctxt.event_irq_lock, flags);
		LOG_DEBUG_BDF("clear link check requester, is_up=%d\n",
					adapter->link.is_up);
	}

l_end:
	return;
}

static void sxe_link_up_handle(struct sxe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	const char *speed_str;

	if (netif_carrier_ok(netdev) &&
		!test_bit(SXE_LINK_SPEED_CHANGE, &(adapter->monitor_ctxt.state))) {
		goto l_end;
	}

	clear_bit(SXE_LINK_SPEED_CHANGE, &(adapter->monitor_ctxt.state));

	switch (adapter->link.speed) {
		case SXE_LINK_SPEED_10GB_FULL:
			speed_str  = "10 Gbps";
			break;
		case SXE_LINK_SPEED_1GB_FULL:
			speed_str  = "1 Gbps";
			break;
		case SXE_LINK_SPEED_100_FULL:
			speed_str  = "100 Mbps";
			break;
		case SXE_LINK_SPEED_10_FULL:
			speed_str  = "10 Mbps";
			break;
		default:
			speed_str  = "unknow speed";
			break;
		}
	LOG_MSG_WARN(drv, "nic link is up, speed: %s\n", speed_str);

	netif_carrier_on(netdev);

	sxe_vf_rate_update(adapter);

	netif_tx_wake_all_queues(adapter->netdev);

	sxe_link_update_notify_vf_all(adapter);
l_end:
	return;
}

static void sxe_link_down_handle(struct sxe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	adapter->link.is_up = false;
	adapter->link.speed = 0;

	if (netif_carrier_ok(netdev)) {
		LOG_MSG_WARN(drv, "nic link is down\n");
		netif_carrier_off(netdev);
		sxe_link_update_notify_vf_all(adapter);
	}

	if (sxe_tx_ring_pending(adapter) || sxe_vf_tx_pending(adapter)) {
		LOG_MSG_WARN(drv, "initiating reset to clear Tx work after link loss\n");
		set_bit(SXE_RESET_REQUESTED, &adapter->monitor_ctxt.state);
	}

	return;
}

static void sxe_detect_link_work(struct sxe_adapter *adapter)
{
	carrier_lock(adapter);
	if (test_bit(SXE_DOWN, &adapter->state) ||
	    test_bit(SXE_REMOVING, &adapter->state) ||
	    test_bit(SXE_RESETTING, &adapter->state)) {
		carrier_unlock(adapter);
		goto l_end;
	}

	sxe_link_update(adapter);

	if (adapter->link.is_up) {
		sxe_link_up_handle(adapter);
	} else {
		sxe_link_down_handle(adapter);
	}
	carrier_unlock(adapter);

	sxe_bad_vf_flr(adapter);

	sxe_spoof_packets_check(adapter);

l_end:
	return;
}

static s32 sxe_fnav_all_sample_rules_del(struct sxe_adapter *adapter)
{
	struct sxe_fnav_sample_filter *filter;
	struct sxe_hw *hw = &adapter->hw;
	struct hlist_node *tmp;
	int bkt;

	if (!adapter->fnav_ctxt.sample_rules_cnt) {
		return 0;
	}

	flush_workqueue(sxe_fnav_workqueue);

	spin_lock(&adapter->fnav_ctxt.sample_lock);
	hash_for_each_safe(adapter->fnav_ctxt.sample_list, bkt, tmp, filter, hlist) {
		hw->dbu.ops->fnav_single_sample_rule_del(hw, filter->hash);
		hash_del(&filter->hlist);
		kfree(filter);
	}

	adapter->fnav_ctxt.sample_rules_cnt = 0;
	spin_unlock(&adapter->fnav_ctxt.sample_lock);

	hw->dbu.ops->fnav_sample_stats_reinit(hw);

	return 0;
}

#ifdef NEED_BOOTTIME_SECONDS
static inline time64_t ktime_get_boottime_seconds(void)
{
	return ktime_divns(ktime_get_boottime(), NSEC_PER_SEC);
}
#endif

STATIC void sxe_fnav_sample_reinit_work(struct sxe_adapter *adapter)
{
	u32 i;
	struct sxe_hw *hw = &adapter->hw;
	unsigned long flags;

	if (adapter->fnav_ctxt.fdir_overflow_time &&
	    (ktime_get_boottime_seconds() - adapter->fnav_ctxt.fdir_overflow_time > 1)) {
		adapter->fnav_ctxt.fdir_overflow_time = 0;
		adapter->stats.sw.fnav_overflow++;

		if (sxe_fnav_all_sample_rules_del(adapter) == 0) {
			for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
				set_bit(SXE_TX_FNAV_INIT_DONE,
					&(adapter->tx_ring_ctxt.ring[i]->state));
			}

			for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
				set_bit(SXE_TX_FNAV_INIT_DONE,
					&adapter->xdp_ring_ctxt.ring[i]->state);
			}

			hw->irq.ops->pending_irq_write_clear(hw, SXE_EICR_FLOW_NAV);

			spin_lock_irqsave(&adapter->irq_ctxt.event_irq_lock, flags);
			hw->irq.ops->specific_irq_enable(hw, SXE_EIMS_FLOW_NAV);
			spin_unlock_irqrestore(&adapter->irq_ctxt.event_irq_lock, flags);
			adapter->fnav_ctxt.is_sample_table_overflowed = false;
		} else {
			LOG_MSG_ERR(probe, "failed to finish FNAV re-initialization, "
			      "ignored adding FNAV APP_TR filters\n");
		}

		goto l_ret;
	 }

	if (!test_bit(SXE_FNAV_REQUIRES_REINIT, &adapter->monitor_ctxt.state)) {
		LOG_INFO_BDF("fnav not requires reinit\n");
		goto l_ret;
	}

	clear_bit(SXE_FNAV_REQUIRES_REINIT, &adapter->monitor_ctxt.state);

	if (test_bit(SXE_DOWN, &adapter->state)) {
		LOG_INFO_BDF("sxe state is down no need fnav reinit\n");
		goto l_ret;
	}

	if (!(adapter->cap & SXE_FNAV_SAMPLE_ENABLE)) {
		LOG_INFO_BDF("only sample fnav mode need reinit\n");
		goto l_ret;
	}

	adapter->fnav_ctxt.fdir_overflow_time = ktime_get_boottime_seconds();
	adapter->fnav_ctxt.is_sample_table_overflowed = true;

l_ret:
	LOG_INFO_BDF("fnav reinit finish, and overflow=%llu\n",
			adapter->stats.sw.fnav_overflow);
	return;
}

STATIC void sxe_ptp_timer_check(struct sxe_adapter *adapter)
{
	if (test_bit(SXE_PTP_RUNNING, &adapter->state)) {
		sxe_ptp_overflow_check(adapter);
		if (adapter->cap & SXE_RX_HWTSTAMP_IN_REGISTER) {
			sxe_ptp_rx_hang(adapter);
		}

		sxe_ptp_tx_hang(adapter);
	}

	return;
}

static s32 sxe_hw_fault_handle_task(struct sxe_adapter *adapter)
{
	s32 ret = 0;

	if (sxe_is_hw_fault(&adapter->hw)) {
		if (!test_bit(SXE_DOWN, &adapter->state)) {
			rtnl_lock();
			sxe_down(adapter);
			rtnl_unlock();
		}

		LOG_ERROR_BDF("sxe nic fault\n");
		ret = -EFAULT;
	}

	return ret;
}

STATIC void sxe_sfp_reset_work(struct sxe_adapter *adapter)
{
	s32 ret;
	struct sxe_monitor_context *monitor = &adapter->monitor_ctxt;

	if (!test_bit(SXE_SFP_NEED_RESET, &monitor->state)) {
		goto l_end;
	}

	if (adapter->link.sfp_reset_timeout &&
	    time_after(adapter->link.sfp_reset_timeout, jiffies)) {
		goto l_end;
	}

	if (test_and_set_bit(SXE_IN_SFP_INIT, &adapter->state)) {
		goto l_end;
	}

	adapter->link.sfp_reset_timeout = jiffies + SXE_SFP_RESET_WAIT_TIME - 1;

	ret = adapter->phy_ctxt.ops->identify(adapter);
	if (ret) {
		LOG_WARN_BDF("monitor identify sfp failed\n");
		goto sfp_out;
	}

	if (!test_bit(SXE_SFP_NEED_RESET, &monitor->state)) {
		goto sfp_out;
	}

	clear_bit(SXE_SFP_NEED_RESET, &monitor->state);

	set_bit(SXE_LINK_NEED_CONFIG, &monitor->state);
	LOG_MSG_INFO(probe, "SFP+ reset done, trigger link_config subtask\n");

sfp_out:
	clear_bit(SXE_IN_SFP_INIT, &adapter->state);

	if ((ret == SXE_ERR_SFF_NOT_SUPPORTED) && \
		(adapter->netdev->reg_state == NETREG_REGISTERED)) {
		LOG_DEV_ERR("failed to initialize because an unsupported "
			  "SFP+ module type was detected.\n");
		LOG_DEV_ERR("reload the driver after installing a "
			  "supported module.\n");
		unregister_netdev(adapter->netdev);
	}

l_end:
	return;
}

STATIC void sxe_sfp_link_config_work(struct sxe_adapter *adapter)
{
	s32 ret;
	u32 speed;
	bool autoneg;
	struct sxe_monitor_context *monitor = &adapter->monitor_ctxt;

	if(time_after(jiffies, adapter->link.sfp_multispeed_time +
#ifdef SXE_SFP_DEBUG
		(HZ * sw_sfp_multi_gb_ms) / SXE_HZ_TRANSTO_MS)) {
#else
		(HZ * SXE_SW_SFP_MULTI_GB_MS) / SXE_HZ_TRANSTO_MS)) {
#endif
		clear_bit(SXE_SFP_MULTI_SPEED_SETTING, &adapter->state);
	}

	if (test_and_set_bit(SXE_IN_SFP_INIT, &adapter->state)) {
		goto l_sfp_end;
	}

	if (!test_bit(SXE_LINK_NEED_CONFIG, &monitor->state)) {
		goto l_sfp_uninit;
	}

	adapter->phy_ctxt.ops->get_link_capabilities(adapter, &speed, &autoneg);

	ret = sxe_link_configure(adapter, speed);
	if (ret) {
		LOG_DEV_ERR("link config err, ret=%d, try...\n", ret);
		goto l_sfp_uninit;
	}

	clear_bit(SXE_LINK_NEED_CONFIG, &monitor->state);

	set_bit(SXE_LINK_CHECK_REQUESTED, &monitor->state);
	LOG_DEBUG("link_config subtask done, trigger link_check subtask\n");
	adapter->link.check_timeout = jiffies;

l_sfp_uninit:
	clear_bit(SXE_IN_SFP_INIT, &adapter->state);

l_sfp_end:
	return;
}

static void sxe_fc_tx_xoff_check(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	u32 i, xoff, dbu_to_mac_stats;

	if ((hw->fc.current_mode != SXE_FC_FULL) &&
	    (hw->fc.current_mode != SXE_FC_RX_PAUSE)) {
		goto l_end;
	}

	xoff = hw->dbu.ops->tx_dbu_fc_status_get(hw);
	dbu_to_mac_stats = hw->stat.ops->tx_dbu_to_mac_stats(hw);
	xoff &= SXE_TFCS_PB0_MASK;

	if (!xoff && !dbu_to_mac_stats) {
		goto l_end;
	}

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		clear_bit(SXE_HANG_CHECK_ARMED, &adapter->tx_ring_ctxt.ring[i]->state);
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		clear_bit(SXE_HANG_CHECK_ARMED, &adapter->xdp_ring_ctxt.ring[i]->state);
	}

l_end:
	return;
}

static void sxe_pfc_tx_xoff_check(struct sxe_adapter *adapter)
{
	u8 tc;
	struct sxe_hw *hw = &adapter->hw;
	u32 i, data, xoff[SXE_PKG_BUF_NUM_MAX], dbu_to_mac_stats;

	data = hw->dbu.ops->tx_dbu_fc_status_get(hw);
	dbu_to_mac_stats = hw->stat.ops->tx_dbu_to_mac_stats(hw);

	for (i = 0; i < SXE_PKG_BUF_NUM_MAX; i++) {
		xoff[i] = SXE_TFCS_PB_MASK; 
		xoff[i] &= data & (SXE_TFCS_PB0_MASK << i);
	}

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		tc = adapter->tx_ring_ctxt.ring[i]->tc_idx;

		if (!xoff[tc] && !dbu_to_mac_stats) {
			continue;
		} else {
			clear_bit(SXE_HANG_CHECK_ARMED, &adapter->tx_ring_ctxt.ring[i]->state);
		}

	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		tc = adapter->xdp_ring_ctxt.ring[i]->tc_idx;

		if (!xoff[tc] && !dbu_to_mac_stats) {
			continue;
		} else {
			clear_bit(SXE_HANG_CHECK_ARMED, &adapter->xdp_ring_ctxt.ring[i]->state);
		}
	}

	return;
}

static void sxe_tx_xoff_check_work(struct sxe_adapter *adapter)
{
	bool pfc_en = adapter->dcb_ctxt.cee_cfg.pfc_mode_enable;

#ifdef SXE_DCB_CONFIGURE
	if (adapter->dcb_ctxt.ieee_pfc) {
		pfc_en |= !!(adapter->dcb_ctxt.ieee_pfc->pfc_en);
	}
#endif

	if (!(adapter->cap & SXE_DCB_ENABLE) || !pfc_en) {
		sxe_fc_tx_xoff_check(adapter);
	} else {
		sxe_pfc_tx_xoff_check(adapter);
	}

	return;
}

void sxe_work_cb(struct work_struct *work)
{
	struct sxe_monitor_context *monitor = container_of(work,
					struct sxe_monitor_context, work);
	struct sxe_adapter *adapter = container_of(monitor, struct sxe_adapter,
					monitor_ctxt);

	if (sxe_hw_fault_handle_task(adapter)) {
		goto l_end;
	}

	sxe_reset_work(adapter);
	sxe_sfp_reset_work(adapter);
	sxe_sfp_link_config_work(adapter);

	sxe_detect_link_work(adapter);

	sxe_stats_update_work(adapter);
	sxe_tx_xoff_check_work(adapter);

	sxe_fnav_sample_reinit_work(adapter);

	sxe_check_hang_work(adapter);

	sxe_ptp_timer_check(adapter);

l_end:
	sxe_monitor_work_complete(adapter);
	return;
}

static void sxe_hw_fault_task_trigger(void *priv)
{
	struct sxe_adapter *adapter = (struct sxe_adapter *)priv;

	if (test_bit(SXE_MONITOR_WORK_INITED,
			&adapter->monitor_ctxt.state)) {
		sxe_monitor_work_schedule(adapter);
		LOG_ERROR_BDF("sxe nic fault, submit monitor task and "
			"perform the down operation\n");
	}

	return;
}

void sxe_monitor_init(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;

	timer_setup(&adapter->monitor_ctxt.timer, sxe_timer_cb, 0);

	INIT_WORK(&adapter->monitor_ctxt.work, sxe_work_cb);
	set_bit(SXE_MONITOR_WORK_INITED, &adapter->monitor_ctxt.state);
	clear_bit(SXE_MONITOR_WORK_SCHED, &adapter->monitor_ctxt.state);

	sxe_hw_fault_handle_init(hw, sxe_hw_fault_task_trigger, adapter);

	mutex_init(&adapter->link.carrier_mutex);
	mutex_init(&adapter->stats.stats_mutex);

	return;
}
