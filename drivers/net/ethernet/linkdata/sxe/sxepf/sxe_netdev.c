
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/socket.h>
#include <linux/if_macvlan.h>
#include <net/xfrm.h>
#ifdef SXE_PHY_CONFIGURE
#include <linux/mdio.h>
#endif

#include "sxe_netdev.h"
#include "sxe_rx_proc.h"
#include "sxe_hw.h"
#include "sxe_tx_proc.h"
#include "sxe_log.h"
#include "sxe_irq.h"
#include "sxe_pci.h"
#include "sxe_sriov.h"
#include "sxe_ethtool.h"
#include "sxe_filter.h"
#include "sxe_netdev.h"
#include "sxe_ptp.h"
#include "sxe_monitor.h"
#include "sxe_ipsec.h"
#include "sxe_dcb.h"
#include "sxe_xdp.h"
#include "sxe_debug.h"
#include "sxe_host_hdc.h"

#define SXE_HW_REINIT_SRIOV_DELAY	(2000)

#define SXE_UC_ADDR_DEL_WAIT_MIN	(10000)
#define SXE_UC_ADDR_DEL_WAIT_MAX	(20000)

#ifdef HAVE_MACVLAN_OFFLOAD_SUPPORT
void sxe_macvlan_configure(struct sxe_adapter *adapter);
STATIC void sxe_macvlan_offload_reset(struct sxe_adapter *adapter);

#ifdef HAVE_NO_MACVLAN_DEST_FILTER
static inline bool macvlan_supports_dest_filter(struct net_device *dev)
{
	struct macvlan_dev *macvlan = netdev_priv(dev);

	return macvlan->mode == MACVLAN_MODE_PRIVATE ||
	       macvlan->mode == MACVLAN_MODE_VEPA ||
	       macvlan->mode == MACVLAN_MODE_BRIDGE;
}
#endif

#endif

void sxe_reset(struct sxe_adapter *adapter)
{
	s32 ret;
	struct sxe_hw *hw = &adapter->hw;

	if (sxe_is_hw_fault(hw)) {
		goto l_end;
	}

	while (test_and_set_bit(SXE_IN_SFP_INIT, &adapter->state)) {
		usleep_range(SXE_SFP_INIT_WAIT_ITR_MIN, SXE_SFP_INIT_WAIT_ITR_MAX);
	}

	clear_bit(SXE_SFP_NEED_RESET, &adapter->monitor_ctxt.state);
	clear_bit(SXE_LINK_NEED_CONFIG, &adapter->monitor_ctxt.state);


	ret = sxe_hw_reset(adapter);
	if (ret < 0) {
		LOG_ERROR_BDF("hw init failed, ret=%d\n", ret);
	} else {
		sxe_hw_start(hw);
	}

	clear_bit(SXE_IN_SFP_INIT, &adapter->state);

	__dev_uc_unsync(adapter->netdev, NULL);
	sxe_mac_filter_reset(adapter);

	sxe_mac_addr_set(adapter);

	if (test_bit(SXE_PTP_RUNNING, &adapter->state)) {
		sxe_ptp_reset(adapter);
	}

l_end:
	return;
}

u32 sxe_sw_mtu_get(struct sxe_adapter *adapter)
{
	u32 max_frame;

	max_frame = adapter->netdev->mtu + SXE_ETH_DEAD_LOAD;

	if (max_frame < (ETH_DATA_LEN + SXE_ETH_DEAD_LOAD)) {
		max_frame = (ETH_DATA_LEN + SXE_ETH_DEAD_LOAD);
	}

	LOG_INFO_BDF("pf netdev mtu:%u result:%u\n",
		      adapter->netdev->mtu,
		      max_frame);

	return max_frame;
}

s32 sxe_link_config(struct sxe_adapter *adapter)
{
	s32 ret;
	bool autoneg;
	u32 speed = adapter->phy_ctxt.autoneg_advertised;

	if (!speed) {
		adapter->phy_ctxt.ops->get_link_capabilities(adapter,
					&speed, &autoneg);
	}

	if (adapter->phy_ctxt.ops->link_configure != NULL) {
		ret = adapter->phy_ctxt.ops->link_configure(adapter, speed);
		if (ret) {
			LOG_ERROR_BDF("set link speed failed, ret=%d\n", ret);
			goto l_end;
		}
	}

	ret = adapter->phy_ctxt.ops->reset(adapter);
	if (ret) {
		LOG_ERROR_BDF("phy reset failed, ret=%d\n", ret);
		goto l_end;
	}

	LOG_INFO_BDF("speed config seccess, speed=%x\n", speed);

l_end:
	return ret;
}

static void sxe_txrx_enable(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;

	hw->dbu.ops->rx_cap_switch_on(hw);

	return;
}

static inline void sxe_vt2_configure(struct sxe_adapter *adapter)
{
#ifdef HAVE_MACVLAN_OFFLOAD_SUPPORT
	sxe_macvlan_configure(adapter);
#endif
}

int sxe_open(struct net_device *netdev)
{
	int ret;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;

	if (test_bit(SXE_TESTING, &adapter->state)) {
		ret = -EBUSY;
		goto l_end;
	}

	netif_carrier_off(netdev);

	ret = sxe_host_to_fw_time_sync(adapter);
	if (ret) {
		LOG_ERROR_BDF("fw time sync failed, fw status err, ret=%d\n", ret);
		goto l_end;
	}

#ifdef SXE_DCB_CONFIGURE
	sxe_dcb_configure(adapter);
#endif

	sxe_vt1_configure(adapter);

#ifdef SXE_TPH_CONFIGURE
	if (adapter->cap & SXE_TPH_CAPABLE) {
		sxe_tph_setup(adapter);
	}
#endif

	ret = sxe_tx_configure(adapter);
	if (ret) {
		LOG_ERROR_BDF("tx config failed, ret=%d\n", ret);
		goto l_reset;
	}

	ret = sxe_rx_configure(adapter);
	if (ret) {
		LOG_ERROR_BDF("rx config failed, reset and wait for next insmode\n");
		goto l_free_tx;
	}

	sxe_vt2_configure(adapter);

#ifdef SXE_IPSEC_CONFIGURE
	sxe_ipsec_table_restore(adapter);
#endif

	ret = sxe_irq_configure(adapter);
	if (ret) {
		LOG_ERROR_BDF("irq config failed, ret=%d\n", ret);
		goto l_free_rx;
	}

	sxe_txrx_enable(adapter);

	sxe_task_timer_trigger(adapter);

	hw->setup.ops->pf_rst_done_set(hw);

#ifdef HAVE_NDO_SET_VF_LINK_STATE
	sxe_vf_enable_and_reinit_notify_vf_all(adapter);
#endif

	LOG_INFO_BDF("open success\n");
	return 0;

l_free_rx:
	sxe_rx_release(adapter);
l_free_tx:
	sxe_tx_release(adapter);
l_reset:
	sxe_reset(adapter);
l_end:
	return ret;
}

static void sxe_netif_disable(struct net_device *netdev)
{
	netif_tx_stop_all_queues(netdev);

	netif_carrier_off(netdev);
	netif_tx_disable(netdev);

	return;
}

static void sxe_hw_disable(struct sxe_adapter *adapter)
{
	sxe_hw_rx_disable(adapter);

	sxe_hw_irq_disable(adapter);

	sxe_hw_tx_disable(adapter);

	return;
}

static void sxe_txrx_ring_clean(struct sxe_adapter *adapter)
{
	u32 i;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		sxe_tx_ring_buffer_clean(adapter->tx_ring_ctxt.ring[i]);
	}
	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		sxe_tx_ring_buffer_clean(adapter->xdp_ring_ctxt.ring[i]);
	}
	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		sxe_rx_ring_buffer_clean(adapter->rx_ring_ctxt.ring[i]);
	}

	return ;
}

STATIC void sxe_resource_release(struct sxe_adapter *adapter)
{
	sxe_irq_release(adapter);

	sxe_rx_release(adapter);

	sxe_tx_release(adapter);

	return;
}

static void sxe_txrx_disable(struct sxe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	sxe_netif_disable(netdev);

	sxe_hw_disable(adapter);

	sxe_napi_disable(adapter);

	return ;
}

void sxe_down(struct sxe_adapter *adapter)
{
	LOG_INFO_BDF("down start\n");
	carrier_lock(adapter);
	if (test_and_set_bit(SXE_DOWN, &adapter->state)) {
		carrier_unlock(adapter);
		goto l_end;
	}

	sxe_txrx_disable(adapter);
	carrier_unlock(adapter);

	clear_bit(SXE_RESET_REQUESTED, &adapter->monitor_ctxt.state);
	clear_bit(SXE_LINK_CHECK_REQUESTED, &adapter->monitor_ctxt.state);
	clear_bit(SXE_FNAV_REQUIRES_REINIT, &adapter->monitor_ctxt.state);

	del_timer_sync(&adapter->monitor_ctxt.timer);
	sxe_vf_down(adapter);

	if (!pci_channel_offline(adapter->pdev)) {
		sxe_reset(adapter);
	}

	if (adapter->phy_ctxt.ops->sfp_tx_laser_disable) {
		adapter->phy_ctxt.ops->sfp_tx_laser_disable(adapter);
	}

	sxe_txrx_ring_clean(adapter);
l_end:
	LOG_INFO_BDF("down finish\n");
	return;
}

static void sxe_fuc_resource_release(struct sxe_adapter *adapter)
{
	sxe_ptp_suspend(adapter);

	return ;
}

void sxe_terminate(struct sxe_adapter *adapter)
{
	sxe_fuc_resource_release(adapter);

	sxe_down(adapter);

	sxe_resource_release(adapter);

	return ;
}

void sxe_up(struct sxe_adapter *adapter)
{
#ifdef SXE_DCB_CONFIGURE
	sxe_dcb_configure(adapter);
#endif

	sxe_vt1_configure(adapter);

#ifdef SXE_IPSEC_CONFIGURE
	sxe_ipsec_table_restore(adapter);
#endif

#ifdef SXE_TPH_CONFIGURE
	if (adapter->cap & SXE_TPH_CAPABLE) {
		sxe_tph_setup(adapter);
	}
#endif

	sxe_hw_tx_configure(adapter);

	sxe_hw_rx_configure(adapter);

	sxe_vt2_configure(adapter);

	sxe_hw_irq_configure(adapter);

	sxe_txrx_enable(adapter);

	sxe_task_timer_trigger(adapter);

	adapter->hw.setup.ops->pf_rst_done_set(&adapter->hw);

#ifdef HAVE_NDO_SET_VF_LINK_STATE
	sxe_vf_enable_and_reinit_notify_vf_all(adapter);
#endif

	LOG_INFO_BDF("up finish\n");
	return ;
}

void sxe_hw_reinit(struct sxe_adapter *adapter)
{
	WARN_ON(in_interrupt());

	netif_trans_update(adapter->netdev);

	while (test_and_set_bit(SXE_RESETTING, &adapter->state)) {
		usleep_range(1000, 2000);
	}

	sxe_down(adapter);

	if (adapter->cap & SXE_SRIOV_ENABLE) {
		msleep(SXE_HW_REINIT_SRIOV_DELAY);
	}

	sxe_up(adapter);
	clear_bit(SXE_RESETTING, &adapter->state);

	LOG_INFO_BDF("reinit finish\n");
	return;
}

void sxe_do_reset(struct net_device *netdev)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev)) {
		sxe_hw_reinit(adapter);
	} else {
		sxe_reset(adapter);
	}

	return;
}

int sxe_close(struct net_device *netdev)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	sxe_ptp_stop(adapter);

	sxe_terminate(adapter);

	sxe_fnav_rules_clean(adapter);

	LOG_INFO_BDF("close finish\n");
	return 0;
}

static void sxe_vlan_strip_enable(struct sxe_adapter *adapter)
{
	u32 i, j;
	struct sxe_hw *hw = &adapter->hw;

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		struct sxe_ring *ring = adapter->rx_ring_ctxt.ring[i];

		if(!netif_is_sxe(ring->netdev)) {
			continue;
		}

		j = ring->reg_idx;
		hw->dma.ops->vlan_tag_strip_switch(hw, j, true);
	}

	return;
}

static void sxe_vlan_strip_disable(struct sxe_adapter *adapter)
{
	u32 i, j;
	struct sxe_hw *hw = &adapter->hw;

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		struct sxe_ring *ring = adapter->rx_ring_ctxt.ring[i];

		j = ring->reg_idx;
		hw->dma.ops->vlan_tag_strip_switch(hw, j, false);
	}

	return;
}

static void sxe_refill_vfta(struct sxe_adapter *adapter, u32 vfta_offset)
{
	u32 i, vid, word, bits;
	struct sxe_hw *hw = &adapter->hw;
	u32 vfta[VFTA_BLOCK_SIZE] = { 0 };
	u32 vid_start = vfta_offset * VF_BLOCK_BITS;
	u32 vid_end = vid_start + (VFTA_BLOCK_SIZE * VF_BLOCK_BITS);

	for (i = SXE_VLVF_ENTRIES; --i;) {
		u32 vlvf = hw->filter.vlan.ops->pool_filter_read(hw, i);

		vid = vlvf & VLAN_VID_MASK;

		if (vid < vid_start || vid >= vid_end) {
			continue;
		}

		if (vlvf) {
			vfta[(vid - vid_start) / VF_BLOCK_BITS] |= BIT(vid % VF_BLOCK_BITS);

			if (test_bit(vid, adapter->vlan_ctxt.active_vlans))
				continue;
		}

		word = i * 2 + PF_POOL_INDEX(0) / VF_BLOCK_BITS;   

		bits = ~BIT(PF_POOL_INDEX(0) % VF_BLOCK_BITS);     
		bits &= hw->filter.vlan.ops->pool_filter_bitmap_read(hw, word);
		hw->filter.vlan.ops->pool_filter_bitmap_write(hw, word, bits);
	}

	for (i = VFTA_BLOCK_SIZE; i--;) {
		vid = (vfta_offset + i) * VF_BLOCK_BITS;
		word = vid / BITS_PER_LONG;
		bits = vid % BITS_PER_LONG;

		vfta[i] |= adapter->vlan_ctxt.active_vlans[word] >> bits;
		hw->filter.vlan.ops->filter_array_write(hw, vfta_offset + i, vfta[i]);
	}

	return;
}

static void sxe_vlan_promisc_disable(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;

	hw->filter.vlan.ops->filter_switch(hw, true);
	return;
}

static void sxe_vf_vlan_promisc_disable(struct sxe_adapter *adapter)
{
	u32 i;

	if (!(adapter->cap & SXE_VLAN_PROMISC)) {
		goto l_end;
	}

	adapter->cap &= ~SXE_VLAN_PROMISC;

	for (i = 0; i < adapter->vlan_ctxt.vlan_table_size; i += VFTA_BLOCK_SIZE) {
		sxe_refill_vfta(adapter, i);
	}

l_end:
	return ;
}

static void sxe_vlan_promisc_enable(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;

	hw->filter.vlan.ops->filter_switch(hw, false);
	return;
}

static void sxe_vf_vlan_promisc_enable(struct sxe_adapter *adapter)
{
	u32 i;
	struct sxe_hw *hw = &adapter->hw;

	hw->filter.vlan.ops->filter_switch(hw, true);

	if (adapter->cap & SXE_VLAN_PROMISC) {
		goto l_end;
	}

	adapter->cap |= SXE_VLAN_PROMISC;

	for (i = SXE_VLVF_ENTRIES; --i;) {
		u32 reg_offset = i * 2 + PF_POOL_INDEX(0) / VF_BLOCK_BITS;
		u32 vlvfb = hw->filter.vlan.ops->pool_filter_bitmap_read(hw, reg_offset);

		vlvfb |= BIT(PF_POOL_INDEX(0) % VF_BLOCK_BITS);
		hw->filter.vlan.ops->pool_filter_bitmap_write(hw, reg_offset, vlvfb);
	}

	for (i = adapter->vlan_ctxt.vlan_table_size; i--;) {
		hw->filter.vlan.ops->filter_array_write(hw, i, ~0U);
	}

l_end:
	return;
}

static void sxe_set_vlan_mode(struct net_device *netdev,
					netdev_features_t features)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	LOG_DEBUG_BDF("netdev[%p]'s vlan strip %s\n", netdev,
		(features & NETIF_F_HW_VLAN_CTAG_RX) ? "enabled" : "disabled");
	if (features & NETIF_F_HW_VLAN_CTAG_RX) {
		sxe_vlan_strip_enable(adapter);
	} else {
		sxe_vlan_strip_disable(adapter);
	}

	LOG_DEBUG_BDF("netdev[%p]'s pf vlan promisc %s\n", netdev,
		(features & NETIF_F_HW_VLAN_CTAG_FILTER) ? "disabled" : "enabled");
	if (features & NETIF_F_HW_VLAN_CTAG_FILTER) {
		sxe_vlan_promisc_disable(adapter);

		if (adapter->cap & SXE_MACVLAN_ENABLE) {
			LOG_DEBUG_BDF("netdev[%p]'s vf vlan promisc disabled\n", netdev);
			sxe_vf_vlan_promisc_disable(adapter);
		}
	} else {
		sxe_vlan_promisc_enable(adapter);

		if (adapter->cap & SXE_MACVLAN_ENABLE) {
			LOG_DEBUG_BDF("netdev[%p]'s vf vlan promisc enabled\n", netdev);
			sxe_vf_vlan_promisc_enable(adapter);
		}
	}

	return;
}

s32 sxe_vlan_rx_add_vid(struct net_device *netdev,
				 __be16 proto, u16 vid)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;

	LOG_INFO_BDF("netdev[%p] add vlan: proto[%u], vid[%u]\n", netdev, proto, vid);
	if (!vid || !(adapter->cap & SXE_VLAN_PROMISC)) {
		hw->filter.vlan.ops->filter_configure(hw, vid, PF_POOL_INDEX(0), true, !!vid);
	}

	set_bit(vid, adapter->vlan_ctxt.active_vlans);

	return 0;
}

static s32 sxe_vlan_rx_kill_vid(struct net_device *netdev,
				  __be16 proto, u16 vid)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;

	LOG_INFO_BDF("kill vlan: proto[%u], vid[%u]\n", proto, vid);
	if (vid && !(adapter->cap & SXE_VLAN_PROMISC)) {
		hw->filter.vlan.ops->filter_configure(hw, vid, PF_POOL_INDEX(0), false, true);
	}

	clear_bit(vid, adapter->vlan_ctxt.active_vlans);

	return 0;
}

void __sxe_set_rx_mode(struct net_device *netdev, bool lock)
{
	u32 flt_ctrl;
	s32 count;
	u32 vmolr = SXE_VMOLR_BAM | SXE_VMOLR_AUPE;
	netdev_features_t features = netdev->features;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;
	unsigned long flags;

	flt_ctrl = hw->filter.mac.ops->rx_mode_get(hw);
	LOG_DEBUG_BDF("read flt_ctrl=0x%x\n", flt_ctrl);

	flt_ctrl &= ~SXE_FCTRL_SBP; 
	flt_ctrl |= SXE_FCTRL_BAM;

	flt_ctrl &= ~(SXE_FCTRL_UPE | SXE_FCTRL_MPE);
	if (netdev->flags & IFF_PROMISC) {
		flt_ctrl |= (SXE_FCTRL_UPE | SXE_FCTRL_MPE);
		vmolr |= SXE_VMOLR_MPE;
		features &= ~NETIF_F_HW_VLAN_CTAG_FILTER;
		LOG_INFO_BDF("both unicast promisc and multicast promisc enabled."
			"flags:0x%x flt_ctrl:0x%x vmolr:0x%x features:0x%llx\n",
			netdev->flags, flt_ctrl, vmolr, features);
	} else {
		if (netdev->flags & IFF_ALLMULTI) {
			flt_ctrl |= SXE_FCTRL_MPE;
			vmolr |= SXE_VMOLR_MPE;
			LOG_INFO_BDF("unicast promisc enabled."
			"flags:0x%x flt_ctrl:0x%x vmolr:0x%x features:0x%llx\n",
			netdev->flags, flt_ctrl, vmolr, features);
		}
	}

	if (features & NETIF_F_RXALL) {
		flt_ctrl |= (SXE_FCTRL_SBP |	
			  SXE_FCTRL_BAM );	

	}

	if (__dev_uc_sync(netdev, sxe_uc_sync, sxe_uc_unsync)) {
		flt_ctrl |= SXE_FCTRL_UPE;
		vmolr |= SXE_VMOLR_ROPE;
		LOG_ERROR_BDF("uc addr sync fail, enable unicast promisc."
			"flags:0x%x flt_ctrl:0x%x vmolr:0x%x features:0x%llx\n",
			netdev->flags, flt_ctrl, vmolr, features);
	}

	if (lock) {
		spin_lock_irqsave(&adapter->vt_ctxt.vfs_lock, flags);
		count = sxe_mc_addr_add(netdev);
		spin_unlock_irqrestore(&adapter->vt_ctxt.vfs_lock, flags);
	} else {
		count = sxe_mc_addr_add(netdev);
	}

	if (count < 0) {
		flt_ctrl |= SXE_FCTRL_MPE;
		vmolr |= SXE_VMOLR_MPE;
		LOG_ERROR_BDF("mc addr add fail count:%d, enable multicast promisc."
			"flags:0x%x flt_ctrl:0x%x vmolr:0x%x features:0x%llx\n",
			count, netdev->flags, flt_ctrl, vmolr, features);
	} else if (count) {
		vmolr |= SXE_VMOLR_ROMPE;
	}

	vmolr |= hw->filter.mac.ops->pool_rx_mode_get(hw, PF_POOL_INDEX(0)) &
		 (~(SXE_VMOLR_MPE | SXE_VMOLR_ROMPE | SXE_VMOLR_ROPE));

	hw->filter.mac.ops->pool_rx_mode_set(hw, vmolr, PF_POOL_INDEX(0));

	LOG_DEBUG_BDF("write flt_ctrl=0x%x\n", flt_ctrl);
	hw->filter.mac.ops->rx_mode_set(hw, flt_ctrl);

	sxe_set_vlan_mode(netdev, features);

	return;
}

void sxe_set_rx_mode(struct net_device *netdev)
{
	__sxe_set_rx_mode(netdev, true);
}

static int sxe_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	s32 ret = 0;

#ifdef HAVE_XDP_SUPPORT
	u32 new_frame_size;
	if (adapter->xdp_prog) {
		new_frame_size = new_mtu + SXE_ETH_DEAD_LOAD;
		if (new_frame_size > sxe_max_xdp_frame_size(adapter)) {
			LOG_MSG_WARN(probe, "requested mtu size is not"
				"supported with xdp\n");
			ret = -EINVAL;
			goto l_ret;
		}
	}
#endif

	LOG_MSG_INFO(probe, "changing MTU from %d to %d\n", netdev->mtu, new_mtu);

	netdev->mtu = new_mtu;

	if (netif_running(netdev)) {
		sxe_hw_reinit(adapter);
	}

#ifdef HAVE_XDP_SUPPORT
l_ret:
#endif
	return ret;
}

static int sxe_set_features(struct net_device *netdev,
			      netdev_features_t features)
{
	bool need_reset = false;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	netdev_features_t changed = netdev->features ^ features;

	if (!(features & NETIF_F_LRO)) {
		if (adapter->cap & SXE_LRO_ENABLE) {
			need_reset = true;
		}
		adapter->cap &= ~SXE_LRO_ENABLE;
		LOG_DEBUG_BDF("lro disabled and need_reset is %s\n",
				need_reset ? "true" : "false");
	} else if ((adapter->cap & SXE_LRO_CAPABLE) &&
		   !(adapter->cap & SXE_LRO_ENABLE)) {
		if (adapter->irq_ctxt.rx_irq_interval == 1 ||
		    adapter->irq_ctxt.rx_irq_interval > SXE_MIN_LRO_ITR) {
			adapter->cap |= SXE_LRO_ENABLE;
			need_reset = true;
			LOG_DEBUG_BDF("lro enabled and need reset,"
				" rx_irq_throttle=%u\n",
				adapter->irq_ctxt.rx_irq_interval);
		} else if ((changed ^ features) & NETIF_F_LRO) {
			LOG_MSG_WARN(probe, "irq interval set too low, lro can not process"
			       "disabling LRO\n");
		}
	}

	if ((features & NETIF_F_NTUPLE) || (features & NETIF_F_HW_TC)) {
		if (!(adapter->cap & SXE_FNAV_SPECIFIC_ENABLE)) {
			need_reset = true;
		}

		adapter->cap &= ~SXE_FNAV_SAMPLE_ENABLE;
		adapter->cap |= SXE_FNAV_SPECIFIC_ENABLE;
		LOG_DEBUG_BDF("switch to specific mode and need_reset is %s\n",
				need_reset ? "true" : "false");
	} else {
		if (adapter->cap & SXE_FNAV_SPECIFIC_ENABLE) {
			need_reset = true;
		}

		adapter->cap &= ~SXE_FNAV_SPECIFIC_ENABLE;
		LOG_DEBUG_BDF("switch off specific mode and need_reset is %s\n",
				need_reset ? "true" : "false");

		if ((adapter->cap & SXE_SRIOV_ENABLE) ||
			(sxe_dcb_tc_get(adapter) > 1) ||
			(adapter->ring_f.rss_limit <= 1)) {
			LOG_DEBUG_BDF("can not switch to sample mode. vt_mode=%s,"
				" tcs=%u, rss_limit=%u\n",
				(adapter->cap & SXE_SRIOV_ENABLE) ? "on" : "off",
				sxe_dcb_tc_get(adapter), adapter->ring_f.rss_limit);
		} else {
			adapter->cap |= SXE_FNAV_SAMPLE_ENABLE;
			LOG_DEBUG_BDF("switch to sample mode and need_reset is %s\n",
				need_reset ? "true" : "false");
		}
	}

	if (changed & NETIF_F_RXALL) {
		need_reset = true;
	}

	netdev->features = features;

#ifdef HAVE_MACVLAN_OFFLOAD_SUPPORT
	if ((changed & NETIF_F_HW_L2FW_DOFFLOAD) &&
			adapter->pool_f.pf_num_used > 1) {
		sxe_macvlan_offload_reset(adapter);
	} else if (need_reset) {
#else
	if (need_reset) {
#endif
		sxe_do_reset(netdev);
	} else if (changed & NETIF_F_HW_VLAN_CTAG_FILTER) {
		__sxe_set_rx_mode(netdev, true);
	}

	return 1;
}

static netdev_features_t sxe_fix_features(struct net_device *netdev,
					    netdev_features_t features)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	if (!(features & NETIF_F_RXCSUM)) {
		LOG_DEBUG_BDF("netif rxcsum off, and lro need off too");
		features &= ~NETIF_F_LRO;
	}

	if (!(adapter->cap & SXE_LRO_CAPABLE)) {
		LOG_DEBUG_BDF("sxe capacity not support lro, turn lro off");
		features &= ~NETIF_F_LRO;
	}

	if (adapter->xdp_prog && (features & NETIF_F_LRO)) {
		LOG_DEV_ERR("lro is not supported with xdp\n");
		features &= ~NETIF_F_LRO;
	}

	return features;
}

static netdev_features_t sxe_features_check(struct sk_buff *skb,
		struct net_device *dev, netdev_features_t features)
{
	unsigned int network_hdr_len, mac_hdr_len;
	netdev_features_t changed_features = features;

	mac_hdr_len = skb_network_header(skb) - skb->data;
	if (unlikely(mac_hdr_len > SXE_MAX_MAC_HDR_LEN)) {
		LOG_DEBUG("mac_hdr_len=%u > %u\n",
			 mac_hdr_len, SXE_MAX_MAC_HDR_LEN);
		SKB_DUMP(skb);
		changed_features = (features & ~(NETIF_F_HW_CSUM |
			    NETIF_F_SCTP_CRC |
			    NETIF_F_HW_VLAN_CTAG_TX |
			    NETIF_F_TSO |
			    NETIF_F_TSO6));
		goto l_ret;
	}

	network_hdr_len = skb_checksum_start(skb) - skb_network_header(skb);
	if (unlikely(network_hdr_len >  SXE_MAX_NETWORK_HDR_LEN)) {
		LOG_DEBUG("network_hdr_len=%u > %u\n",
			 network_hdr_len, SXE_MAX_NETWORK_HDR_LEN);
		SKB_DUMP(skb);
		changed_features = (features & ~(NETIF_F_HW_CSUM |
			    NETIF_F_SCTP_CRC |
			    NETIF_F_TSO |
			    NETIF_F_TSO6));
		goto l_ret;
	}

	if (skb->encapsulation && !(features & NETIF_F_TSO_MANGLEID)) {
#ifdef SXE_IPSEC_CONFIGURE
		if (!secpath_exists(skb))
#endif
		changed_features = features & ~NETIF_F_TSO;
	}

l_ret:
	return changed_features;
}

static void sxe_rx_stats_update(struct sxe_adapter *adapter)
{
	u32 i;
	struct sxe_ring *rx_ring;
	struct net_device *netdev = adapter->netdev;
	struct sxe_sw_stats  *sw_stats = &adapter->stats.sw;
	u64 alloc_rx_page = 0, lro_count = 0, lro_flush = 0;
	u64 alloc_rx_page_failed = 0, alloc_rx_buff_failed = 0;
	u64 non_eop_descs = 0, bytes = 0, packets = 0, hw_csum_rx_error = 0;

	if (adapter->cap & SXE_LRO_ENABLE) {
		for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
			rx_ring = READ_ONCE(adapter->rx_ring_ctxt.ring[i]);
			if (!rx_ring) {
				continue;
			}

			lro_count += rx_ring->rx_stats.lro_count;
			lro_flush += rx_ring->rx_stats.lro_flush;
		}
		sw_stats->lro_total_count = lro_count;
		sw_stats->lro_total_flush = lro_flush;
	}

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		rx_ring = READ_ONCE(adapter->rx_ring_ctxt.ring[i]);
		if (!rx_ring) {
			continue;
		}

		non_eop_descs        += rx_ring->rx_stats.non_eop_descs;
		alloc_rx_page        += rx_ring->rx_stats.alloc_rx_page;
		alloc_rx_page_failed += rx_ring->rx_stats.alloc_rx_page_failed;
		alloc_rx_buff_failed += rx_ring->rx_stats.alloc_rx_buff_failed;
		hw_csum_rx_error     += rx_ring->rx_stats.csum_err;
		bytes                += rx_ring->stats.bytes;
		packets              += rx_ring->stats.packets;
	}

	sw_stats->non_eop_descs        = non_eop_descs;
	sw_stats->alloc_rx_page        = alloc_rx_page;
	sw_stats->alloc_rx_page_failed = alloc_rx_page_failed;
	sw_stats->alloc_rx_buff_failed = alloc_rx_buff_failed;
	sw_stats->hw_csum_rx_error     = hw_csum_rx_error;
	netdev->stats.rx_bytes         = bytes;
	netdev->stats.rx_packets       = packets;

	return;
}

static void sxe_tx_stats_update(struct sxe_adapter *adapter)
{
	u32 i;
	struct sxe_ring *tx_ring;
	struct sxe_ring *xdp_ring;
	struct net_device *netdev = adapter->netdev;
	struct sxe_sw_stats  *sw_stats = &adapter->stats.sw;
	u64 bytes = 0, packets = 0, restart_queue = 0, tx_busy = 0;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		tx_ring = adapter->tx_ring_ctxt.ring[i];
		if (!tx_ring) {
			continue;
		}

		restart_queue += tx_ring->tx_stats.restart_queue;
		tx_busy       += tx_ring->tx_stats.tx_busy;
		bytes         += tx_ring->stats.bytes;
		packets       += tx_ring->stats.packets;
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		xdp_ring = adapter->xdp_ring_ctxt.ring[i];
		if (!xdp_ring) {
			continue;
		}

		restart_queue += xdp_ring->tx_stats.restart_queue;
		tx_busy       += xdp_ring->tx_stats.tx_busy;
		bytes         += xdp_ring->stats.bytes;
		packets       += xdp_ring->stats.packets;
	}
	sw_stats->restart_queue  = restart_queue;
	sw_stats->tx_busy        = tx_busy;
	netdev->stats.tx_bytes   = bytes;
	netdev->stats.tx_packets = packets;

	return;
}

static void sxe_hw_stats_update(struct sxe_adapter *adapter)
{
	u32 i;
	u64 total_mpc = 0;
	struct sxe_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	struct sxe_mac_stats *hw_stats = &adapter->stats.hw;

	hw->stat.ops->stats_get(hw, hw_stats);

	netdev->stats.multicast = hw_stats->mprc;

	netdev->stats.rx_errors        = hw_stats->crcerrs + hw_stats->rlec;
	netdev->stats.rx_dropped       = 0;
	netdev->stats.rx_length_errors = hw_stats->rlec;
	netdev->stats.rx_crc_errors    = hw_stats->crcerrs;

	for (i = 0; i < 8; i++) {
		total_mpc += hw_stats->dburxdrofpcnt[i];
	}
	netdev->stats.rx_missed_errors = total_mpc;

	return;
}

void sxe_stats_update(struct sxe_adapter *adapter)
{
	if (test_bit(SXE_DOWN, &adapter->state) ||
	    test_bit(SXE_RESETTING, &adapter->state)) {
		goto l_end;
	}

	sxe_rx_stats_update(adapter);

	sxe_tx_stats_update(adapter);

	sxe_hw_stats_update(adapter);

l_end:
	return;
}

static void sxe_ring_stats64_get(struct rtnl_link_stats64 *stats,
				   struct sxe_ring *ring,
				   bool is_rx)
{
	u32 start;
	u64 bytes, packets;

	if (ring) {
		do {
			start = u64_stats_fetch_begin_irq(&ring->syncp);
			packets = ring->stats.packets;
			bytes   = ring->stats.bytes;
		} while (u64_stats_fetch_retry_irq(&ring->syncp, start));

		if (true == is_rx) {
			stats->rx_packets += packets;
			stats->rx_bytes   += bytes;
		} else {
			stats->tx_packets += packets;
			stats->tx_bytes   += bytes;
		}
	}

	return;
}

#ifdef NO_VOID_NDO_GET_STATS64
static struct rtnl_link_stats64 *
sxe_get_stats64(struct net_device *netdev, struct rtnl_link_stats64 *stats)
#else
static void sxe_get_stats64(struct net_device *netdev,
			      struct rtnl_link_stats64 *stats)
#endif
{
	u32 i;
	struct sxe_ring *ring;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	rcu_read_lock();
	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		ring = READ_ONCE(adapter->rx_ring_ctxt.ring[i]);
		sxe_ring_stats64_get(stats, ring, true);
	}

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		ring = READ_ONCE(adapter->tx_ring_ctxt.ring[i]);
		sxe_ring_stats64_get(stats, ring, false);
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		ring = READ_ONCE(adapter->xdp_ring_ctxt.ring[i]);
		sxe_ring_stats64_get(stats, ring, false);
	}
	rcu_read_unlock();

	stats->multicast	= netdev->stats.multicast;
	stats->rx_errors	= netdev->stats.rx_errors;
	stats->rx_crc_errors	= netdev->stats.rx_crc_errors;
	stats->rx_length_errors	= netdev->stats.rx_length_errors;
	stats->rx_missed_errors	= netdev->stats.rx_missed_errors;

#ifdef NO_VOID_NDO_GET_STATS64
	return stats;
#endif
}

static int sxe_ioctl(struct net_device *netdev, struct ifreq *req, int cmd)
{
	int ret;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	switch (cmd) {
	case SIOCSHWTSTAMP:
		ret = sxe_ptp_hw_tstamp_config_set(adapter, req);
		break;
	case SIOCGHWTSTAMP:
		ret = sxe_ptp_hw_tstamp_config_get(adapter, req);
		break;

	default:
#ifdef SXE_PHY_CONFIGURE
		ret = mdio_mii_ioctl(&adapter->phy_ctxt.mdio, if_mii(req), cmd);
#else
		ret = -EOPNOTSUPP;
#endif
	}

	return ret;
}

#ifdef HAVE_MACVLAN_OFFLOAD_SUPPORT

#ifdef NEED_SET_MACVLAN_MODE
static void sxe_macvlan_set_rx_mode (struct net_device *dev, unsigned int pool,
				      struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	u32 vmolr;

	vmolr = hw->filter.mac.ops->pool_rx_mode_get(hw, SXE_VMOLR(pool));
	vmolr |= (SXE_VMOLR_ROMPE | SXE_VMOLR_BAM | SXE_VMOLR_AUPE);

	vmolr &= ~SXE_VMOLR_MPE;

	if (dev->flags & IFF_ALLMULTI) {
		vmolr |= SXE_VMOLR_MPE;
	} else {
		vmolr |= SXE_VMOLR_ROMPE;
	}

	hw->filter.mac.ops->pool_rx_mode_set(hw, vmolr, pool);
}
#endif 

static int sxe_macvlan_ring_configure(struct sxe_adapter *adapter,
			     struct sxe_macvlan *accel)
{
	s32 ret;
	u32 i, baseq;
	struct net_device *vdev = accel->netdev;
#ifndef HAVE_NO_SB_BIND_CHANNEL
	u16 rss_i = sxe_rss_num_get(adapter);
	int num_tc = netdev_get_num_tc(adapter->netdev);
#endif

	baseq = accel->pool * adapter->ring_f.ring_per_pool;
	LOG_DEV_DEBUG("pool %i:%i queues %i:%i\n",
		   accel->pool, adapter->pool_f.pf_num_used,
		   baseq, baseq + adapter->ring_f.ring_per_pool);

	accel->rx_ring_offset = baseq;
	accel->tx_ring_offset = baseq;

#ifndef HAVE_NO_SB_BIND_CHANNEL
	for (i = 0; i < num_tc; i++) {
		netdev_bind_sb_channel_queue(adapter->netdev, vdev,
					     i, rss_i, baseq + (rss_i * i));
	}
#endif

	for (i = 0; i < adapter->ring_f.ring_per_pool; i++) {
		adapter->rx_ring_ctxt.ring[baseq + i]->netdev = vdev;
	}

	wmb();

	ret = sxe_uc_addr_add(&adapter->hw,
				adapter->mac_filter_ctxt.uc_addr_table,
				vdev->dev_addr, PF_POOL_INDEX(accel->pool));
	if (ret >= 0) {
#ifdef NEED_SET_MACVLAN_MODE
		sxe_macvlan_set_rx_mode(vdev, accel->pool, adapter);
#endif
		goto l_end;
	}

#ifndef HAVE_NO_MACVLAN_RELEASE
	macvlan_release_l2fw_offload(vdev);
#endif

	for (i = 0; i < adapter->ring_f.ring_per_pool; i++) {
		adapter->rx_ring_ctxt.ring[baseq + i]->netdev = NULL;
	}

	LOG_DEV_ERR("l2fw offload disabled due to L2 filter error\n");

#ifndef HAVE_NO_SB_BIND_CHANNEL
	netdev_unbind_sb_channel(adapter->netdev, vdev);
	netdev_set_sb_channel(vdev, 0);
#endif

	clear_bit(accel->pool, adapter->vt_ctxt.pf_pool_bitmap);
	kfree(accel);

l_end:
	return ret;
}

#ifndef NO_NEED_POOL_DEFRAG
#ifdef HAVE_NETDEV_NESTED_PRIV
STATIC int sxe_macvlan_pool_reassign(struct net_device *vdev,
				struct netdev_nested_priv *priv)
{
	struct sxe_adapter *adapter = (struct sxe_adapter *)priv->data;
#else
STATIC int sxe_macvlan_pool_reassign(struct net_device *vdev, void *data)
{
	struct sxe_adapter *adapter = data;
#endif
	u32 pool;
	struct sxe_macvlan *accel;

	if (!netif_is_macvlan(vdev)) {
		goto l_end;
	}

	accel = macvlan_accel_priv(vdev);
	if (!accel) {
		goto l_end;
	}

	pool = find_first_zero_bit(adapter->vt_ctxt.pf_pool_bitmap,
		adapter->pool_f.pf_num_used);
	LOG_INFO_BDF("free pool=%u, pf pool used=%u\n",
		pool, adapter->pool_f.pf_num_used);
	if (pool < adapter->pool_f.pf_num_used) {
		set_bit(pool, adapter->vt_ctxt.pf_pool_bitmap);
		accel->pool = pool;
		goto l_end;
	}

	LOG_DEV_ERR("l2fw offload disabled due to lack of queue resources\n");
	macvlan_release_l2fw_offload(vdev);
	netdev_unbind_sb_channel(adapter->netdev, vdev);
	netdev_set_sb_channel(vdev, 0);

	kfree(accel);

l_end:
	return 0;
}

void sxe_macvlan_pools_defrag(struct net_device *dev)
{
	struct sxe_adapter *adapter = netdev_priv(dev);

#ifdef HAVE_NETDEV_NESTED_PRIV
	struct netdev_nested_priv priv = {
		.data = (void *)adapter,
	};
	bitmap_clear(adapter->vt_ctxt.pf_pool_bitmap, 1, 63);
	netdev_walk_all_upper_dev_rcu(dev, sxe_macvlan_pool_reassign, &priv);
#else
	bitmap_clear(adapter->vt_ctxt.pf_pool_bitmap, 1, 63);
	netdev_walk_all_upper_dev_rcu(dev, sxe_macvlan_pool_reassign, adapter);
#endif
	return;
}
#endif 

static s32 sxe_macvlan_pools_assign(struct sxe_adapter *adapter)
{
	s32 ret;
	u32 pool;
	u16 assigned_pools, total_pools;
	u8  tcs = sxe_dcb_tc_get(adapter) ? : 1;
	u16 *pf_pools = &adapter->pool_f.pf_num_used;

	pool = find_first_zero_bit(adapter->vt_ctxt.pf_pool_bitmap, *pf_pools);
	if (pool == adapter->pool_f.pf_num_used) {
		total_pools = adapter->vt_ctxt.num_vfs + *pf_pools;

		if (((adapter->cap & SXE_DCB_ENABLE) &&
			*pf_pools >= (SXE_TXRX_RING_NUM_MAX / tcs)) ||
			*pf_pools > SXE_MAX_MACVLANS) {
			LOG_ERROR_BDF("macvlan pool exceed the limit, cap=%x, pf_pool_num=%u\n",
				adapter->cap, *pf_pools);
			ret = -EBUSY;
			goto l_end;
		}

		if (total_pools >= SXE_POOLS_NUM_MAX) {
			LOG_ERROR_BDF("pool num exceed the limit, total_pool_num=%u\n",
				total_pools);
			ret = -EBUSY;
			goto l_end;
		}

		adapter->cap |= SXE_MACVLAN_ENABLE |
				  SXE_SRIOV_ENABLE;

		if (total_pools < SXE_32_POOL && *pf_pools < SXE_16_POOL) {
			assigned_pools = min_t(u16,
					SXE_32_POOL - total_pools,
					SXE_16_POOL - *pf_pools);
			LOG_INFO_BDF("reserved %u pool to macvlan, 4 ring\n", assigned_pools);
		} else if (*pf_pools < SXE_32_POOL) {
			assigned_pools = min_t(u16,
					SXE_POOLS_NUM_MAX - total_pools,
					SXE_32_POOL - *pf_pools);
			LOG_INFO_BDF("reserved %u pool to macvlan, 2 ring\n", assigned_pools);
		} else {
			assigned_pools = SXE_POOLS_NUM_MAX - total_pools;
			LOG_INFO_BDF("reserved %u pool to macvlan, 1 ring\n", assigned_pools);
		}

		if (!assigned_pools) {
			LOG_ERROR_BDF("no remaining pool\n");
			ret = -EBUSY;
			goto l_end;
		}

		adapter->pool_f.pf_num_limit += assigned_pools;

		ret = sxe_ring_reassign(adapter, sxe_dcb_tc_get(adapter));
		if (ret) {
			LOG_ERROR_BDF("ring reassign failed, ret=%d\n", ret);
			goto l_end;
		}

		if (pool >= *pf_pools) {
			ret = -ENOMEM;
			goto l_end;
		}
	}

	return 0;

l_end:
	return ret;
}

STATIC void sxe_macvlan_offload_reset(struct sxe_adapter *adapter)
{
	s32 ret;
	u32 rss = min_t(u32, SXE_RSS_RING_NUM_MAX, num_online_cpus());

	if (!adapter->pool_f.vf_num_used) {
		LOG_DEBUG_BDF("dont enable vf , adpater->cap=%x\n", adapter->cap);
		adapter->cap &= ~(SXE_MACVLAN_ENABLE | SXE_SRIOV_ENABLE);
	}

	LOG_WARN_BDF("macvlan off: go back to rss mode\n");
	adapter->ring_f.rss_limit    = rss;
	adapter->pool_f.pf_num_limit = 1;
	ret = sxe_ring_reassign(adapter, sxe_dcb_tc_get(adapter));
	if (ret) {
		LOG_ERROR_BDF("ring reassign failed, ret=%d\n", ret);
	}

	return;
}

static void *sxe_dfwd_add(struct net_device *pdev, struct net_device *vdev)
{
	s32 ret;
	u32 pool;

	struct sxe_macvlan *accel;
	struct sxe_adapter *adapter = netdev_priv(pdev);

	if (adapter->xdp_prog) {
		LOG_MSG_WARN(probe, "l2fw offload is not supported with xdp\n");
		ret = -EINVAL;
		goto l_err;
	}

	LOG_DEBUG_BDF("macvlan offload start\n");

	if (!macvlan_supports_dest_filter(vdev)) {
		LOG_ERROR_BDF("macvlan mode err\n");
		ret = -EMEDIUMTYPE;
		goto l_err;
	}

#ifndef HAVE_NO_SB_BIND_CHANNEL
	if (netif_is_multiqueue(vdev)) {
		LOG_ERROR_BDF("macvlan is multiqueue\n");
		ret = -ERANGE;
		goto l_err;
	}
#endif

	ret = sxe_macvlan_pools_assign(adapter);
	if (ret < 0) {
		goto l_err;
	}

	accel = kzalloc(sizeof(*accel), GFP_KERNEL);
	if (!accel) {
		LOG_ERROR_BDF("kzalloc failed\n");
		ret = -ENOMEM;
		goto l_err;
	}

	pool = find_first_zero_bit(adapter->vt_ctxt.pf_pool_bitmap,
				adapter->pool_f.pf_num_used);
	set_bit(pool, adapter->vt_ctxt.pf_pool_bitmap);
#ifndef HAVE_NO_SB_BIND_CHANNEL
	netdev_set_sb_channel(vdev, pool);
#endif
	accel->pool = pool;
	accel->netdev = vdev;

	if (!netif_running(pdev)) {
		goto l_end;
	}

	ret = sxe_macvlan_ring_configure(adapter, accel);
	if (ret < 0) {
		goto  l_err;
	}

	LOG_INFO_BDF("macvlan offload success, pool=%d, ring_idx=%u \n",
				pool, accel->tx_ring_offset);

l_end:
	return accel;

l_err:
	return ERR_PTR(ret);
}

static void sxe_dfwd_del(struct net_device *pdev, void *priv)
{
	u32 i;
	struct sxe_ring *ring;
	struct sxe_irq_data *irq_priv;
	struct sxe_macvlan *accel = priv;
	u32 rxbase = accel->rx_ring_offset;
	struct sxe_adapter *adapter = netdev_priv(pdev);

	sxe_uc_addr_del(&adapter->hw, adapter->mac_filter_ctxt.uc_addr_table,
			accel->netdev->dev_addr, PF_POOL_INDEX(accel->pool));

	usleep_range(SXE_UC_ADDR_DEL_WAIT_MIN, SXE_UC_ADDR_DEL_WAIT_MAX);

	for (i = 0; i < adapter->ring_f.ring_per_pool; i++) {
		ring = adapter->rx_ring_ctxt.ring[rxbase + i];
		irq_priv = ring->irq_data;

		if (netif_running(adapter->netdev)) {
			napi_synchronize(&irq_priv->napi);
		}
		ring->netdev = NULL;
	}

#ifndef HAVE_NO_SB_BIND_CHANNEL
	netdev_unbind_sb_channel(pdev, accel->netdev);
	netdev_set_sb_channel(accel->netdev, 0);
#endif

	LOG_INFO_BDF("macvlan del success, pool=%d, ring_idx=%u \n",
			accel->pool, accel->tx_ring_offset);

	clear_bit(accel->pool, adapter->vt_ctxt.pf_pool_bitmap);
	kfree(accel);

	return;
}

#ifndef HAVE_NO_WALK_UPPER_DEV
#ifdef HAVE_NETDEV_NESTED_PRIV
STATIC int sxe_macvlan_up(struct net_device *vdev,
			struct netdev_nested_priv *priv)
{
	struct sxe_adapter *adapter = (struct sxe_adapter *)priv->data;
#else
STATIC int sxe_macvlan_up(struct net_device *vdev, void *data)
{
	struct sxe_adapter *adapter = data;
#endif
	struct sxe_macvlan *accel;

	if (!netif_is_macvlan(vdev))
	{
		goto l_end;
	}

	accel = macvlan_accel_priv(vdev);
	if (!accel){
		goto l_end;
	}

	sxe_macvlan_ring_configure(adapter, accel);

l_end:
	return 0;
}
#endif 

void sxe_macvlan_configure(struct sxe_adapter *adapter)
{
#ifdef HAVE_NO_WALK_UPPER_DEV
		struct net_device *upper;
		struct list_head *iter;
		int err;
		netdev_for_each_all_upper_dev_rcu(adapter->netdev, upper, iter) {
			if (netif_is_macvlan(upper)) {
				struct macvlan_dev *macvlan = netdev_priv(upper);
				struct sxe_macvlan *accel = macvlan->fwd_priv;
	
				if (macvlan->fwd_priv) {
					err = sxe_macvlan_ring_configure(adapter, accel);
					if (err)
						continue;
				}
			}
		}
#else
#ifdef HAVE_NETDEV_NESTED_PRIV
	struct netdev_nested_priv priv = {
		.data = (void *)adapter,
	};

	netdev_walk_all_upper_dev_rcu(adapter->netdev,
				      sxe_macvlan_up, &priv);
#else
	netdev_walk_all_upper_dev_rcu(adapter->netdev,
				      sxe_macvlan_up, adapter);
#endif
#endif 

	return;
}
#endif 

s32 sxe_ring_reassign(struct sxe_adapter *adapter, u8 tc)
{
	s32 ret;
	struct net_device *dev = adapter->netdev;

#ifdef SXE_DCB_CONFIGURE
	ret = sxe_dcb_tc_validate(adapter, tc);
	if (ret) {
		goto l_end;
	}
#endif

	if (netif_running(dev)) {
		sxe_close(dev);
	} else {
		sxe_reset(adapter);
	}

	set_bit(SXE_DOWN, &adapter->state);

	sxe_ring_irq_exit(adapter);

#ifdef SXE_DCB_CONFIGURE
	ret = sxe_dcb_tc_setup(adapter, tc);
	if (ret) {
		LOG_ERROR_BDF("dcb tc setup failed, tc=%u\n", tc);
		goto l_end;
	}
#endif

	ret = sxe_ring_irq_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("interrupt ring assign scheme init failed, err=%d\n", ret);
		goto l_end;
	}

#ifdef HAVE_MACVLAN_OFFLOAD_SUPPORT
#ifndef NO_NEED_POOL_DEFRAG
	sxe_macvlan_pools_defrag(dev);
#endif
#endif

	if (netif_running(dev)) {
		ret = sxe_open(dev);
		LOG_INFO_BDF("open done, err=%d\n", ret);
	}

l_end:
	return ret;
}

static int sxe_set_mac_address(struct net_device *netdev, void *p)
{
	s32 ret = 0;
	struct sockaddr *sock_addr = p;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	if (!is_valid_ether_addr(sock_addr->sa_data)) {
		ret = -SXE_ERR_INVALID_PARAM;
		LOG_ERROR_BDF("invalid mac addr:%pM.(err:%d)\n",
			   sock_addr->sa_data, ret);
		goto l_end;
	}

#ifndef HAVE_ETH_HW_ADDR_SET_API
	memcpy(netdev->dev_addr, sock_addr->sa_data, netdev->addr_len);
#else
	eth_hw_addr_set(netdev, sock_addr->sa_data);
#endif
	memcpy(adapter->mac_filter_ctxt.cur_mac_addr,
				sock_addr->sa_data, netdev->addr_len);

	sxe_mac_addr_set(adapter);

l_end:
	return ret;
}

STATIC u16 sxe_available_uc_num_get(struct sxe_adapter *adapter, u16 pool)
{
	struct sxe_uc_addr_table *uc_table =
			&adapter->mac_filter_ctxt.uc_addr_table[0];
	u16 i;
	u16 count = 0;

	for (i = 1; i < SXE_UC_ENTRY_NUM_MAX; i++, uc_table++) {
		if (test_bit(SXE_UC_ADDR_ENTRY_USED, &uc_table->state)) {
			if (uc_table->pool != pool) {
				continue;
			}
		}

		count++;
	}

	LOG_DEBUG_BDF("get uc num = %u\n", count);
	return count;
}


static int sxe_fdb_add(struct ndmsg *ndm, struct nlattr *tb[],
				struct net_device *dev,
				const unsigned char *addr, u16 vid,
#ifdef HAVE_NDO_FDB_ADD_EXTACK
				u16 flags,
				struct netlink_ext_ack *extack)
#else
				u16 flags)
#endif
{
	int ret;

	if (is_unicast_ether_addr(addr) || is_link_local_ether_addr(addr)) {
		struct sxe_adapter *adapter = netdev_priv(dev);
		u16 pool = PF_POOL_INDEX(0);
		u16 available_num = sxe_available_uc_num_get(adapter, pool);

		if (netdev_uc_count(dev) >= available_num) {
			LOG_ERROR_BDF("netdev_uc_count=%u >= available_num=%u\n",
					netdev_uc_count(dev), available_num);
			ret = -ENOMEM;
			goto l_ret;
		}
	}

	ret = ndo_dflt_fdb_add(ndm, tb, dev, addr, vid, flags);

l_ret:
	return ret;
}

static s32 sxe_bridge_mode_configure(struct sxe_adapter *adapter,
					__u16 mode)
{
	struct sxe_hw *hw = &adapter->hw;
	s32 ret = 0;

	switch (mode) {
	case BRIDGE_MODE_VEPA:
		hw->dma.ops->vt_pool_loopback_switch(hw, false);

		break;
	case BRIDGE_MODE_VEB:
		hw->dma.ops->vt_pool_loopback_switch(hw, true);


		break;
	default:
		ret = -EINVAL;
		LOG_ERROR_BDF("config hw[%p] bridge mode[%u], num_vfs[%u] failed\n",
			hw, mode, adapter->vt_ctxt.num_vfs);
		goto l_ret;
	}

	adapter->bridge_mode = mode;

	LOG_MSG_INFO(drv, "enabling bridge mode: %s\n",
		mode == BRIDGE_MODE_VEPA ? "VEPA" : "VEB");

l_ret:
	return ret;
}

static int sxe_bridge_setlink(struct net_device *dev,
					struct nlmsghdr *nlh,
#ifdef HAVE_NDO_BRIDGE_SETLINK_EXTACK
					u16 flags,
					struct netlink_ext_ack *extack)
#else
					u16 flags)
#endif
{
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct nlattr *attr, *br_spec;
	int rem;
	s32 ret = 0;

	if (!(adapter->cap & SXE_SRIOV_ENABLE)) {
		LOG_ERROR_BDF("not in sriov mode,exit\n");
		ret = -EOPNOTSUPP;
		goto l_ret;
	}

	br_spec = nlmsg_find_attr(nlh, sizeof(struct ifinfomsg), IFLA_AF_SPEC);
	if (!br_spec) {
		LOG_ERROR_BDF("can not find proper attr\n");
		ret = -EINVAL;
		goto l_ret;
	}

	nla_for_each_nested(attr, br_spec, rem) {
		int status;
		u16 mode;

		if (nla_type(attr) != IFLA_BRIDGE_MODE) {
			continue;
		}

		if (nla_len(attr) < sizeof(mode)) {
			LOG_ERROR_BDF("attr size[%u] < sizeof(mode)=%zu\n",
					nla_len(attr), sizeof(mode));
			ret = -EINVAL;
			goto l_ret;
		}

		mode = nla_get_u16(attr);
		status = sxe_bridge_mode_configure(adapter, mode);
		if (status) {
			LOG_ERROR_BDF("mode[0x%x] config failed:status=%d\n",
					mode, status);
			ret = status;
			goto l_ret;
		}

		break;
	}

l_ret:
	return ret;
}

static int sxe_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq,
				    struct net_device *dev,
				    u32 filter_mask, int nlflags)
{
	struct sxe_adapter *adapter = netdev_priv(dev);
	s32 ret = 0;

	if (!(adapter->cap & SXE_SRIOV_ENABLE)) {
		LOG_ERROR_BDF("not in sriov mode,exit\n");
		goto l_ret;
	}

	LOG_DEBUG_BDF("get link:pid[%u], seq[%u], bridge_mode[0x%x], dev[%p], "
		"filter_mask[0x%x], nlflags[0x%x]\n", pid, seq,
		adapter->bridge_mode, dev, filter_mask, nlflags);

	ret = ndo_dflt_bridge_getlink(skb, pid, seq, dev,
				       adapter->bridge_mode, 0, 0, nlflags,
				       filter_mask, NULL);

l_ret:
	return ret;
}

u32 sxe_mbps_link_speed_get(u32 speed)
{
	u32 mbps;

	switch(speed) {
	case SXE_LINK_SPEED_10GB_FULL:
		mbps = SXE_LINK_SPEED_MBPS_10G;
		break;
	case SXE_LINK_SPEED_1GB_FULL:
		mbps = SXE_LINK_SPEED_MBPS_1G;
		break;
	case SXE_LINK_SPEED_100_FULL:
		mbps = SXE_LINK_SPEED_MBPS_100;
		break;
	case SXE_LINK_SPEED_10_FULL:
		mbps = SXE_LINK_SPEED_MBPS_10;
		break;
	default:
		mbps = 0;
		break;
	}

	LOG_INFO("link speed:0x%x mbps speed:0x%x.\n", speed, mbps);

	return mbps;
}

static int sxe_tx_maxrate_set(struct net_device *netdev,
				int queue_index, u32 maxrate)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;
	u32 bcnrc_val = sxe_mbps_link_speed_get(adapter->link.speed);

	if (!maxrate) {
		goto l_end;
	}

	bcnrc_val <<= SXE_RTTBCNRC_RF_INT_SHIFT;
	bcnrc_val /= maxrate;

	bcnrc_val &= SXE_RTTBCNRC_RF_INT_MASK | SXE_RTTBCNRC_RF_DEC_MASK;

	bcnrc_val |= SXE_RTTBCNRC_RS_ENA;

	hw->dma.ops->dcb_tx_ring_rate_factor_set(hw, queue_index, bcnrc_val);

l_end:
	return 0;
}

static const struct net_device_ops sxe_netdev_ops = {
	.ndo_open		= sxe_open,
	.ndo_stop		= sxe_close,
	.ndo_start_xmit		= sxe_xmit,
	.ndo_set_rx_mode	= sxe_set_rx_mode,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_mac_address	= sxe_set_mac_address,
#ifdef HAVE_NET_DEVICE_EXTENDED
	.ndo_size		= sizeof(struct net_device_ops),
	.extended.ndo_change_mtu	= sxe_change_mtu,
#else
	.ndo_change_mtu			= sxe_change_mtu,
#endif
	.ndo_tx_timeout         = sxe_tx_timeout,

#ifdef HAVE_NET_DEVICE_EXTENDED
	.extended.ndo_set_tx_maxrate	= sxe_tx_maxrate_set,
#else
	.ndo_set_tx_maxrate		= sxe_tx_maxrate_set,
#endif
	.ndo_vlan_rx_add_vid	= sxe_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid	= sxe_vlan_rx_kill_vid,
	.ndo_set_vf_rate	= sxe_set_vf_rate,
#ifdef HAVE_NET_DEVICE_EXTENDED
	.extended.ndo_set_vf_vlan	= sxe_set_vf_vlan,
#else
	.ndo_set_vf_vlan		= sxe_set_vf_vlan,
#endif
	.ndo_set_vf_mac         = sxe_set_vf_mac,
	.ndo_set_vf_spoofchk    = sxe_set_vf_spoofchk,
	.ndo_set_vf_rss_query_en = sxe_set_vf_rss_query_en,
#ifdef HAVE_NET_DEVICE_EXTENDED
	.extended.ndo_set_vf_trust	= sxe_set_vf_trust,
#else
	.ndo_set_vf_trust		= sxe_set_vf_trust,
#endif

	.ndo_get_vf_config	= sxe_get_vf_config,

#ifdef HAVE_NDO_SET_VF_LINK_STATE
	.ndo_set_vf_link_state	= sxe_set_vf_link_state,
#endif

	.ndo_set_features	= sxe_set_features,
	.ndo_fix_features	= sxe_fix_features,
	.ndo_features_check	= sxe_features_check,

	.ndo_get_stats64	= sxe_get_stats64,  
#ifdef HAVE_NDO_ETH_IOCTL
	.ndo_eth_ioctl		= sxe_ioctl,
#else
	.ndo_do_ioctl		= sxe_ioctl,
#endif

#ifdef HAVE_MACVLAN_OFFLOAD_SUPPORT
	.ndo_dfwd_add_station	= sxe_dfwd_add,
	.ndo_dfwd_del_station	= sxe_dfwd_del,
#endif
	.ndo_fdb_add		= sxe_fdb_add,
	.ndo_bridge_setlink	= sxe_bridge_setlink,
	.ndo_bridge_getlink	= sxe_bridge_getlink,

#ifdef HAVE_XDP_SUPPORT
	.ndo_bpf		= sxe_xdp,
	.ndo_xdp_xmit		= sxe_xdp_xmit,
#ifdef HAVE_AF_XDP_ZERO_COPY
#ifdef HAVE_NDO_XSK_WAKEUP
	.ndo_xsk_wakeup		= sxe_xsk_wakeup,
#else
	.ndo_xsk_async_xmit	= sxe_xsk_async_xmit,
#endif
#endif
#endif
};

static void sxe_netdev_ops_init(struct net_device *netdev)
{
	netdev->netdev_ops = &sxe_netdev_ops;
	return;
}

bool netif_is_sxe(struct net_device *dev)
{
	return dev && (dev->netdev_ops == &sxe_netdev_ops);
}

STATIC void sxe_netdev_feature_init(struct net_device *netdev)
{
	struct sxe_adapter *adapter;

	netdev->features = NETIF_F_SG |      
			   NETIF_F_RXCSUM |  
			   NETIF_F_HW_CSUM | 
			   NETIF_F_SCTP_CRC |  
			   NETIF_F_RXHASH;   

	netdev->gso_partial_features = SXE_GSO_PARTIAL_FEATURES;
	netdev->features |= NETIF_F_TSO |      
			NETIF_F_TSO6 |
			NETIF_F_GSO_PARTIAL|
			SXE_GSO_PARTIAL_FEATURES;

#ifdef SXE_IPSEC_CONFIGURE
	netdev->features |= NETIF_F_HW_ESP |
				NETIF_F_HW_ESP_TX_CSUM |
				NETIF_F_GSO_ESP;
#endif

	netdev->hw_features |= netdev->features |
			       NETIF_F_HW_VLAN_CTAG_FILTER | 
			       NETIF_F_HW_VLAN_CTAG_TX |     
#ifdef HAVE_MACVLAN_OFFLOAD_SUPPORT
			       NETIF_F_HW_L2FW_DOFFLOAD |    
#endif
			       NETIF_F_NTUPLE |              
			       NETIF_F_LRO    |              
			       NETIF_F_RXALL;

	if (dma_get_mask(netdev->dev.parent) == DMA_BIT_MASK(SXE_DMA_BIT_WIDTH_64)) {
		netdev->features |= NETIF_F_HIGHDMA;
	}

	netdev->vlan_features |= netdev->features | NETIF_F_TSO_MANGLEID;

	netdev->mpls_features |= NETIF_F_SG  |
				NETIF_F_TSO  |
				NETIF_F_TSO6 |
				NETIF_F_HW_CSUM |
				SXE_GSO_PARTIAL_FEATURES; 

	netdev->features |= NETIF_F_HW_VLAN_CTAG_FILTER |
			    NETIF_F_HW_VLAN_CTAG_RX |     
			    NETIF_F_HW_VLAN_CTAG_TX;

	adapter = netdev_priv(netdev);
	adapter->cap |= SXE_LRO_CAPABLE;

	return;
}

static void sxe_netdev_name_init(struct net_device *netdev,
					struct pci_dev *pdev)
{
	strlcpy(netdev->name, pci_name(pdev), sizeof(netdev->name));

	return;
}

#ifndef NO_NETDEVICE_MIN_MAX_MTU
static void sxe_netdev_mtu_init(struct net_device *netdev)
{
#ifdef HAVE_NET_DEVICE_EXTENDED
	netdev->extended->min_mtu = ETH_MIN_MTU;
	netdev->extended->max_mtu = SXE_MAX_JUMBO_FRAME_SIZE - SXE_ETH_DEAD_LOAD;
#else
	netdev->min_mtu = ETH_MIN_MTU;
	netdev->max_mtu = SXE_MAX_JUMBO_FRAME_SIZE - SXE_ETH_DEAD_LOAD;
#endif
	return;
}
#endif

static void sxe_netdev_priv_flags_init(struct net_device *netdev)
{
	netdev->priv_flags |= IFF_UNICAST_FLT;
	netdev->priv_flags |= IFF_SUPP_NOFCS;

	return;
}

void sxe_netdev_init(struct net_device *netdev, struct pci_dev *pdev)
{
	SET_NETDEV_DEV(netdev, &pdev->dev);

	sxe_netdev_ops_init(netdev);

	sxe_netdev_name_init(netdev, pdev);

	sxe_netdev_feature_init(netdev);

	sxe_netdev_priv_flags_init(netdev);

#ifndef NO_NETDEVICE_MIN_MAX_MTU
	sxe_netdev_mtu_init(netdev);
#endif

	sxe_ethtool_ops_set(netdev);
	return;
}
