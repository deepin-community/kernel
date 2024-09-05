#include <net/rtnetlink.h>

#include "sxevf.h"
#include "sxevf_netdev.h"
#include "sxevf_monitor.h"
#include "sxevf_ethtool.h"
#include "sxevf_msg.h"

#define SXEVF_CHECK_LINK_TIMER_PERIOD   (HZ / 10)
#define SXEVF_NORMAL_TIMER_PERIOD       (HZ * 2)

#define SXEVF_CHECK_LINK_CYCLE_CNT      (5)
#define SXEVF_CHECK_LINK_DELAY_TIME     (100)

void sxevf_task_timer_trigger(struct sxevf_adapter *adapter)
{
	set_bit(SXEVF_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state);
	LOG_DEBUG_BDF("link check requester, state=%lx, monitor_state=%lx, is_up=%d\n",
			adapter->state, adapter->monitor_ctxt.state, adapter->link.is_up);

	mod_timer(&adapter->monitor_ctxt.timer, jiffies);

	return;
}

void sxevf_monitor_work_schedule(struct sxevf_adapter *adapter)
{
	struct workqueue_struct  *wq = sxevf_wq_get();

	if (!test_bit(SXEVF_DOWN, &adapter->state) &&
		!test_bit(SXEVF_REMOVING, &adapter->state) &&
		!test_and_set_bit(SXEVF_MONITOR_WORK_SCHED,
				&adapter->monitor_ctxt.state)) {

		queue_work(wq, &adapter->monitor_ctxt.work);
	}

	return;
}

static void sxevf_timer_cb(struct timer_list *timer)
{
	struct sxevf_monitor_context *monitor = container_of(timer, struct sxevf_monitor_context,
					timer);
	struct sxevf_adapter *adapter = container_of(monitor, struct sxevf_adapter,
					monitor_ctxt);
	unsigned long period ;

	if (test_bit(SXEVF_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state)) {
		period = SXEVF_CHECK_LINK_TIMER_PERIOD;
	} else {
		period = SXEVF_NORMAL_TIMER_PERIOD;
	}

	mod_timer(&adapter->monitor_ctxt.timer, period + jiffies);

	sxevf_monitor_work_schedule(adapter);

	return;
}

static void sxevf_monitor_work_complete(struct sxevf_adapter *adapter)
{
	BUG_ON(!test_bit(SXEVF_MONITOR_WORK_SCHED, &adapter->monitor_ctxt.state));

	smp_mb__before_atomic();
	clear_bit(SXEVF_MONITOR_WORK_SCHED, &adapter->monitor_ctxt.state);

	return;
}

STATIC s32 sxevf_ctrl_msg_check(struct sxevf_adapter *adapter)
{
	struct sxevf_hw *hw = &adapter->hw;
	struct sxevf_ctrl_msg ctrl_msg;
	s32 ret;

	spin_lock_bh(&adapter->mbx_lock);
	ret = sxevf_ctrl_msg_rcv_and_clear(hw, (u32 *)&ctrl_msg,
				SXEVF_MSG_NUM(sizeof(struct sxevf_ctrl_msg)));
	spin_unlock_bh(&adapter->mbx_lock);
	if (ret) {
		LOG_ERROR_BDF("ctrl msg rcv fail due to lock fail.(err:%d)\n", ret);
		goto l_end;
	}

	if (ctrl_msg.msg_type & SXEVF_PF_CTRL_MSG_REINIT) {
		adapter->link.need_reinit = true;
		clear_bit(SXEVF_NETDEV_DOWN, &adapter->monitor_ctxt.state);
		LOG_WARN_BDF("rcv ctrl msg:0x%x need reinit vf.\n",
			     ctrl_msg.msg_type);
	} else if (ctrl_msg.msg_type & SXEVF_PF_CTRL_MSG_NETDEV_DOWN) {
		adapter->link.is_up = false;

		set_bit(SXEVF_NETDEV_DOWN, &adapter->monitor_ctxt.state);
		LOG_WARN_BDF("rcv ctrl msg:0x%x need link down.\n", ctrl_msg.msg_type);
	} else if (ctrl_msg.msg_type & SXEVF_PF_CTRL_MSG_LINK_UPDATE) {
		adapter->link.is_up = true;
		LOG_WARN_BDF("rcv ctrl msg:0x%x physical link up.\n", ctrl_msg.msg_type);
	}

l_end:
	return ret;
}

static void sxevf_physical_link_check(struct sxevf_adapter *adapter)
{
	u32 link_reg, i;
	u32 msg;
	struct sxevf_hw *hw = &adapter->hw;

	spin_lock_bh(&adapter->mbx_lock);
	sxevf_ctrl_msg_rcv(hw, &msg, 1);
	spin_unlock_bh(&adapter->mbx_lock);

	link_reg = hw->setup.ops->link_state_get(hw);
	if (!(link_reg & SXE_VFLINKS_UP)) {
		adapter->link.is_up = false;
		goto l_end;
	}

	for (i = 0; i < SXEVF_CHECK_LINK_CYCLE_CNT; i++) {
		udelay(SXEVF_CHECK_LINK_DELAY_TIME);
		link_reg = hw->setup.ops->link_state_get(hw);
		if (!(link_reg & SXE_VFLINKS_UP)) {
			adapter->link.is_up = false;
			goto l_end;
		}
	}

	switch (link_reg & SXE_VFLINKS_SPEED) {
	case SXE_VFLINKS_SPEED_10G:
		adapter->link.speed = SXEVF_LINK_SPEED_10GB_FULL;
		break;
	case SXE_VFLINKS_SPEED_1G:
		adapter->link.speed = SXEVF_LINK_SPEED_1GB_FULL;
		break;
	case SXE_VFLINKS_SPEED_100:
		adapter->link.speed = SXEVF_LINK_SPEED_100_FULL;
		break;
	}

	adapter->link.is_up = true;

l_end:
	LOG_INFO_BDF("link up status:%d.\n", adapter->link.is_up);
	return;
}

STATIC void sxevf_link_up_handle(struct sxevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	if (netif_carrier_ok(netdev)) {
		goto l_end;
	}

	LOG_DEV_INFO("NIC %s %s link state, down to up, speed=%s\n",
		netdev_name(adapter->netdev),
		dev_name(&adapter->pdev->dev),
		(adapter->link.speed == SXEVF_LINK_SPEED_10GB_FULL) ?
		"10 Gbps" :
		(adapter->link.speed == SXEVF_LINK_SPEED_1GB_FULL) ?
		"1 Gbps" :
		(adapter->link.speed == SXEVF_LINK_SPEED_100_FULL) ?
		"100 Mbps" :
		"unknown speed");

	netif_carrier_on(netdev);

l_end:
	return;
}

static void sxevf_link_down_handle(struct sxevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	adapter->link.speed = 0;

	if (netif_carrier_ok(netdev)) {
		LOG_DEV_INFO("NIC %s %s link state, up to down\n",
			     netdev_name(adapter->netdev),
			     dev_name(&adapter->pdev->dev));
		netif_carrier_off(netdev);
	}

	return;
}

static void sxevf_detect_link_work(struct sxevf_adapter *adapter)
{
	struct sxevf_hw *hw = &adapter->hw;
	s32 ret;

	if (test_bit(SXEVF_DOWN, &adapter->state) ||
	    test_bit(SXEVF_RESETTING, &adapter->state)) {
		goto l_end;
	}

	if (!sxevf_pf_rst_check(hw) || !hw->mbx.retry ||
	    (test_bit(SXEVF_NETDEV_DOWN, &adapter->monitor_ctxt.state))) {
		LOG_WARN_BDF("checked pf reset not done or someone timeout:%d "
			"monitor state:0x%lx.\n",
			hw->mbx.retry, adapter->monitor_ctxt.state);
		set_bit(SXEVF_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state);
	}

	if (!test_bit(SXEVF_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state)) {
		goto l_end;
	}

	sxevf_physical_link_check(adapter);

	if (adapter->link.is_up) {
		ret = sxevf_ctrl_msg_check(adapter);
		if (ret) {
			LOG_ERROR_BDF("ctrl msg rcv fail, try to next workqueue.\n");
			goto l_end;
		}

		if (adapter->link.need_reinit || !hw->mbx.retry) {
			adapter->link.need_reinit = false;
			adapter->link.is_up = false;
			set_bit(SXEVF_RESET_REQUESTED, &adapter->monitor_ctxt.state);
		}
	}

	if (adapter->link.is_up) {
		clear_bit(SXEVF_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state);
		if(adapter->link.link_enable){
			sxevf_link_up_handle(adapter);
		}
	} else {
		sxevf_link_down_handle(adapter);
	}

l_end:
	return;
}

static void sxevf_reset_work(struct sxevf_adapter *adapter)
{
	if (!test_and_clear_bit(SXEVF_RESET_REQUESTED, &adapter->monitor_ctxt.state)) {
		goto l_end;
	}

	rtnl_lock();
	if (test_bit(SXEVF_DOWN, &adapter->state) ||
	    test_bit(SXEVF_REMOVING, &adapter->state) ||
	    test_bit(SXEVF_RESETTING, &adapter->state)) {
		rtnl_unlock();
		goto l_end;
	}

	LOG_ERROR_BDF("reset adapter\n");
	adapter->stats.sw.tx_timeout_count++;

	sxevf_hw_reinit(adapter);
	rtnl_unlock();

l_end:
	return;
}

static void sxevf_check_hang_work(struct sxevf_adapter *adapter)
{
	u32 i;
	u64 eics = 0;
	struct sxevf_irq_data *irq_priv;
	struct sxevf_hw   *hw = &adapter->hw;
	struct sxevf_ring **tx_ring = adapter->tx_ring_ctxt.ring;
	struct sxevf_ring **xdp_ring = adapter->xdp_ring_ctxt.ring;

	if (test_bit(SXEVF_DOWN, &adapter->state) ||
		test_bit(SXEVF_RESETTING, &adapter->state)) {

		goto l_end;
	}

	if (netif_carrier_ok(adapter->netdev)) {
		for (i = 0; i < adapter->tx_ring_ctxt.num; i++)
			SXEVF_TX_HANG_PROC_ACTIVE(tx_ring[i]);
		for (i = 0; i < adapter->xdp_ring_ctxt.num; i++)
			SXEVF_TX_HANG_PROC_ACTIVE(xdp_ring[i]);
	}

	for (i = 0; i < adapter->irq_ctxt.ring_irq_num; i++) {
		irq_priv = adapter->irq_ctxt.irq_data[i];
		if (irq_priv->tx.list.next ||
			irq_priv->rx.list.next)
			eics |= BIT(i);
	}

	hw->irq.ops->ring_irq_trigger(hw, eics);

	LOG_INFO_BDF("set check hang flag ok eics:0x%llx\n", eics);

l_end:
	return;
}

static void sxevf_stats_update_work(struct sxevf_adapter *adapter)
{
	if (test_bit(SXEVF_DOWN, &adapter->state) ||
	    test_bit(SXEVF_RESETTING, &adapter->state)) {
		return;
	}

	sxevf_update_stats(adapter);

	return;
}

static s32 sxevf_hw_fault_handle_task(struct sxevf_adapter *adapter)
{
	s32 ret = 0;

	if (sxevf_is_hw_fault(&adapter->hw)) {
		if (!test_bit(SXEVF_DOWN, &adapter->state)) {
			rtnl_lock();
			sxevf_down(adapter);
			rtnl_unlock();
		}

		LOG_ERROR_BDF("sxe nic fault\n");
		ret = -EFAULT;
	}

	return ret;
}

STATIC void sxevf_ring_reassign_work(struct sxevf_adapter *adapter)
{
	struct net_device *dev = adapter->netdev;

	if (!test_and_clear_bit(SXEVF_RING_REASSIGN_REQUESTED,
				&adapter->monitor_ctxt.state)) {
		goto l_end;
	}

	if (test_bit(SXEVF_DOWN, &adapter->state) ||
	    test_bit(SXEVF_RESETTING, &adapter->state)) {
		goto l_end;
	}

	rtnl_lock();

	if (netif_running(dev)) {
		sxevf_close(dev);
	}

	sxevf_ring_irq_exit(adapter);
	sxevf_ring_irq_init(adapter);

	if (netif_running(dev)) {
		sxevf_open(dev);
	}

	rtnl_unlock();

l_end:
	return;
}

STATIC void sxevf_work_cb(struct work_struct *work)
{
	struct sxevf_monitor_context *monitor = container_of(work, struct sxevf_monitor_context,
					work);
	struct sxevf_adapter *adapter = container_of(monitor, struct sxevf_adapter,
					monitor_ctxt);

	if (sxevf_hw_fault_handle_task(adapter)) {
		goto l_end;
	}

	sxevf_ring_reassign_work(adapter);
	sxevf_reset_work(adapter);
	sxevf_detect_link_work(adapter);
	sxevf_stats_update_work(adapter);
	sxevf_check_hang_work(adapter);

l_end:
	sxevf_monitor_work_complete(adapter);

	return;
}

static void sxevf_hw_fault_task_trigger(void *priv)
{
	struct sxevf_adapter *adapter = (struct sxevf_adapter *)priv;

	if (test_bit(SXEVF_MONITOR_WORK_INITED,
			&adapter->monitor_ctxt.state)) {
		sxevf_monitor_work_schedule(adapter);
		LOG_ERROR_BDF("sxe vf nic fault, submit monitor task and "
			"perform the down operation\n");
	}

	return;
}

void sxevf_monitor_init(struct sxevf_adapter *adapter)
{
	struct sxevf_hw *hw = &adapter->hw;

	timer_setup(&adapter->monitor_ctxt.timer, sxevf_timer_cb, 0);

	INIT_WORK(&adapter->monitor_ctxt.work, sxevf_work_cb);

	set_bit(SXEVF_MONITOR_WORK_INITED, &adapter->monitor_ctxt.state);
	clear_bit(SXEVF_MONITOR_WORK_SCHED, &adapter->monitor_ctxt.state);

	sxevf_hw_fault_handle_init(hw, sxevf_hw_fault_task_trigger, adapter);

	return;
}
