
#include <linux/etherdevice.h>
#include <linux/pci.h>
#include <linux/socket.h>

#include "sxevf_netdev.h"
#include "sxevf_hw.h"
#include "sxe_log.h"
#include "sxevf_pci.h"
#include "sxevf.h"
#include "sxevf_irq.h"
#include "sxevf_msg.h"
#include "sxevf_monitor.h"
#include "sxevf_tx_proc.h"
#include "sxevf_rx_proc.h"
#include "sxevf_hw.h"
#include "sxevf_ethtool.h"
#include "sxevf_debug.h"
#include "sxevf_ipsec.h"
#include "sxevf_xdp.h"

#define SXEVF_MAX_MAC_HDR_LEN		(127)
#define SXEVF_MAX_NETWORK_HDR_LEN	(511)

#define SXEVF_HW_DISABLE_SLEEP_TIME_MIN 10000
#define SXEVF_HW_DISABLE_SLEEP_TIME_MAX 20000

void sxevf_reset(struct sxevf_adapter *adapter)
{
	struct sxevf_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;

	if (sxevf_dev_reset(hw)) {
		LOG_DEV_ERR("reset still fail\n");
	} else {
		sxevf_start_adapter(adapter);
		sxevf_mbx_api_version_init(adapter);
		LOG_DEBUG_BDF("hw reset finished\n");
	}

	if (is_valid_ether_addr(adapter->mac_filter_ctxt.cur_uc_addr)) {
#ifndef HAVE_ETH_HW_ADDR_SET_API
		ether_addr_copy(netdev->dev_addr,
			adapter->mac_filter_ctxt.cur_uc_addr);
#else
		eth_hw_addr_set(netdev, adapter->mac_filter_ctxt.cur_uc_addr);
#endif
		ether_addr_copy(netdev->perm_addr,
			adapter->mac_filter_ctxt.cur_uc_addr);
	}

	adapter->link.check_timeout = jiffies;

	return;
}

static void sxevf_dev_mac_addr_set(struct sxevf_adapter *adapter)
{
	struct sxevf_hw *hw = &adapter->hw;

	spin_lock_bh(&adapter->mbx_lock);

	if (is_valid_ether_addr(adapter->mac_filter_ctxt.cur_uc_addr)) {
		sxevf_uc_addr_set(hw, adapter->mac_filter_ctxt.cur_uc_addr);
	} else {
		sxevf_uc_addr_set(hw, adapter->mac_filter_ctxt.def_uc_addr);
	}

	spin_unlock_bh(&adapter->mbx_lock);

	return;
}

STATIC s32 sxevf_mac_addr_set(struct net_device *netdev, void *p)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	struct sxevf_hw *hw = &adapter->hw;
	struct sockaddr *addr = p;
	s32 ret;

	if (!is_valid_ether_addr(addr->sa_data)) {
		ret = -EADDRNOTAVAIL;
		LOG_ERROR_BDF("invalid mac addr:%pM.(err:%d)\n", addr->sa_data, ret);
		goto l_out;
	}

	spin_lock_bh(&adapter->mbx_lock);
	ret = sxevf_uc_addr_set(hw, addr->sa_data);
	spin_unlock_bh(&adapter->mbx_lock);

	if (ret) {
		LOG_ERROR_BDF("add vf mac addr:%pM to pf filter fail.(ret:%d)\n",
			   addr->sa_data, ret);
		goto l_out;
	}

	ether_addr_copy(adapter->mac_filter_ctxt.cur_uc_addr, addr->sa_data);
	ether_addr_copy(adapter->mac_filter_ctxt.def_uc_addr, addr->sa_data);
#ifndef HAVE_ETH_HW_ADDR_SET_API
	ether_addr_copy(netdev->dev_addr, addr->sa_data);
#else
	eth_hw_addr_set(netdev, addr->sa_data);
#endif

	LOG_INFO_BDF("change vf cur and default mac addr to %pM done.\n",
		addr->sa_data);

l_out:
	return ret;
}

void sxevf_set_rx_mode(struct net_device *netdev)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	struct sxevf_hw *hw = &adapter->hw;
	enum sxevf_cast_mode mode;

	if (netdev->flags & IFF_PROMISC) {
		mode = SXEVF_CAST_MODE_PROMISC;
	} else if (netdev->flags & IFF_ALLMULTI) {
		mode = SXEVF_CAST_MODE_ALLMULTI;
	} else if (netdev->flags & (IFF_BROADCAST | IFF_MULTICAST)) {
		mode = SXEVF_CAST_MODE_MULTI;
	} else {
		mode = SXEVF_CAST_MODE_NONE;
	}

	spin_lock_bh(&adapter->mbx_lock);

	sxevf_cast_mode_set(hw, mode);
	sxevf_mc_addr_sync(hw, netdev);
	sxevf_uc_addr_sync(hw, netdev);

	spin_unlock_bh(&adapter->mbx_lock);

	return;
}

void sxevf_sw_mtu_set(struct sxevf_adapter *adapter, u32 new_mtu)
{
	LOG_INFO_BDF("set sw mtu from %u to %u vf netdev mtu:%u\n",
		      adapter->sw_mtu,
		      new_mtu,
		      adapter->netdev->mtu);

	adapter->sw_mtu = new_mtu;

	return;
}

u32 sxevf_sw_mtu_get(struct sxevf_adapter *adapter)
{
	u32 max_frame;

	if (adapter->sw_mtu == 0) {
		max_frame = adapter->netdev->mtu + SXEVF_ETH_DEAD_LOAD;
	} else {
		max_frame = adapter->sw_mtu;
	}

	LOG_DEBUG_BDF("sw mtu:%u vf netdev mtu:%u result:%u\n",
		      adapter->sw_mtu,
		      adapter->netdev->mtu,
		      max_frame);

	return max_frame;
}

STATIC s32 sxevf_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	u32 frame = new_mtu;
	s32 ret;

	if (adapter->xdp_prog) {
		LOG_DEV_WARN("%s %s xdp progarm can't change mtu.\n",
				netdev_name(adapter->netdev),
				dev_name(&adapter->pdev->dev));
		ret = -EPERM;
		goto l_end;
	}

	ret = sxevf_rx_max_frame_configure(adapter, frame);
	if (ret) {
		ret = -EINVAL;
		LOG_ERROR_BDF("set max frame:%u fail.(err:%d)\n", frame, ret);
		goto l_end;
	}

	LOG_DEV_DEBUG("%s %s change mtu from %u to %u.\n",
			netdev_name(adapter->netdev),
			dev_name(&adapter->pdev->dev),
			netdev->mtu, new_mtu);

	netdev->mtu = new_mtu;
	if (netif_running(netdev)) {
		LOG_INFO_BDF("change mtu to:%u, next to reinit.\n", new_mtu);
		sxevf_hw_reinit(adapter);
	}

l_end:
	return ret;
}

static void sxevf_dcb_configure(struct sxevf_adapter *adapter)
{
	s32 ret;
	u16 rx_ring;
	u16 tx_ring;
	u8 tc_num, default_tc, max_tx_num;
	struct sxevf_hw *hw = &adapter->hw;

	ret = sxevf_ring_info_get(adapter, &tc_num, &default_tc, &max_tx_num);
	if (ret) {
		LOG_ERROR_BDF("get pf ring cfg info fail, use default_tc ring num."
			  "(err:%d)\n", ret);
		goto l_end;
	}

	if (tc_num > 1) {
		tx_ring = 1;
		rx_ring = tc_num;
		adapter->tx_ring_ctxt.ring[0]->reg_idx = default_tc;

		if ((rx_ring != adapter->rx_ring_ctxt.num) ||
			(tx_ring != adapter->tx_ring_ctxt.num)) {
			hw->mbx.interval = 0;
			set_bit(SXEVF_RING_REASSIGN_REQUESTED, &adapter->monitor_ctxt.state);
		}
	}

l_end:
	return;
}

static s32 sxevf_get_link_enable(struct sxevf_adapter *adapter)
{
	s32 ret = 0;
	struct sxevf_hw *hw = &adapter->hw;
	struct sxevf_link_enable_msg msg = {};

	bool enable = adapter->link.link_enable;
	msg.msg_type = SXEVF_LINK_ENABLE_GET;
	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&msg,
				    SXEVF_MSG_NUM(sizeof(msg)));
	if(!ret && msg.msg_type == (SXEVF_LINK_ENABLE_GET | SXEVF_MSGTYPE_ACK)) {
		adapter->link.link_enable = msg.link_enable;
		if (enable && enable != adapter->link.link_enable)
		LOG_MSG_INFO(drv, "VF is administratively disabled\n");
	}
	LOG_INFO_BDF("vf link enable: %d\n", adapter->link.link_enable);

	return ret;
}

int sxevf_open(struct net_device *netdev)
{
	int ret;
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	struct sxevf_hw *hw = &adapter->hw;

	if (test_bit(SXEVF_TESTING, &adapter->state)) {
		ret = -EBUSY;
		goto l_end;
	}

	if (!adapter->irq_ctxt.ring_irq_num) {
		LOG_ERROR_BDF("ring irq num zero.sxevf open fail.\n");
		ret = -ENOMEM;
		goto l_end;
	}

	if (test_bit(SXEVF_HW_STOP, &hw->state)) {
		sxevf_reset(adapter);

		if (test_bit(SXEVF_HW_STOP, &hw->state)) {
			ret = -SXEVF_ERR_RESET_FAILED;
			LOG_DEV_ERR("open process reset vf still fail.(err:%d)\n", ret);
			goto l_reset;
		}
	}

	netif_carrier_off(netdev);

	ret = sxevf_tx_configure(adapter);
	if (ret) {
		LOG_ERROR_BDF("tx config failed, ret=%d\n", ret);
		goto l_reset;
	}

	ret = sxevf_rx_configure(adapter);
	if (ret) {
		LOG_ERROR_BDF("rx config failed, reset and wait for next insmod\n");
		goto l_free_tx;
	}

#ifdef SXE_IPSEC_CONFIGURE
	sxevf_ipsec_restore(adapter);
#endif

	sxevf_get_link_enable(adapter);

	ret = sxevf_irq_configure(adapter);
	if (ret) {
		LOG_ERROR_BDF("irq config failed, ret=%d\n", ret);
		goto l_irq_err;

	}

	sxevf_dcb_configure(adapter);

	sxevf_save_reset_stats(adapter);
	sxevf_last_counter_stats_init(adapter);

	netif_tx_start_all_queues(netdev);

	sxevf_task_timer_trigger(adapter);

	sxevf_dev_mac_addr_set(adapter);

	LOG_INFO_BDF("vf open success\n");

	return 0;

l_irq_err:
	sxevf_rx_release(adapter);
l_free_tx:
	sxevf_tx_release(adapter);
	sxevf_irq_ctxt_exit(adapter);
l_reset:
	sxevf_reset(adapter);
l_end:
	return ret;
}

static void sxevf_netif_stop(struct net_device *netdev)
{
	netif_tx_stop_all_queues(netdev);

	netif_carrier_off(netdev);
	netif_tx_disable(netdev);

	return;
}

static void sxevf_hw_disable(struct sxevf_adapter *adapter)
{
	sxevf_hw_rx_disable(adapter);

	usleep_range(SXEVF_HW_DISABLE_SLEEP_TIME_MIN,
		SXEVF_HW_DISABLE_SLEEP_TIME_MAX);

	sxevf_hw_irq_disable(adapter);

	sxevf_hw_tx_disable(adapter);

	return;
}

static void sxevf_txrx_stop(struct sxevf_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	sxevf_netif_stop(netdev);

	sxevf_hw_disable(adapter);

	sxevf_napi_disable(adapter);

	return ;
}

static void sxevf_txrx_ring_clean(struct sxevf_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		sxevf_tx_ring_buffer_clean(adapter->tx_ring_ctxt.ring[i]);
	}
	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		sxevf_tx_ring_buffer_clean(adapter->xdp_ring_ctxt.ring[i]);
	}
	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		sxevf_rx_ring_buffer_clean(adapter->rx_ring_ctxt.ring[i]);
	}

	return ;
}

void sxevf_down(struct sxevf_adapter *adapter)
{
	if (test_and_set_bit(SXEVF_DOWN, &adapter->state)) {
		goto l_end;
	}

	sxevf_txrx_stop(adapter);

	del_timer_sync(&adapter->monitor_ctxt.timer);

	if (!pci_channel_offline(adapter->pdev)) {
		sxevf_reset(adapter);
	}

	sxevf_txrx_ring_clean(adapter);

	LOG_INFO_BDF("sxevf down done\n");

l_end:
	return;
}

void sxevf_up(struct sxevf_adapter *adapter)
{
	sxevf_dcb_configure(adapter);

#ifdef SXE_IPSEC_CONFIGURE
	sxevf_ipsec_restore(adapter);
#endif

	sxevf_hw_tx_configure(adapter);

	sxevf_hw_rx_configure(adapter);

	sxevf_dev_mac_addr_set(adapter);

	sxevf_get_link_enable(adapter);

	sxevf_hw_irq_configure(adapter);

	sxevf_save_reset_stats(adapter);
	sxevf_last_counter_stats_init(adapter);

	netif_tx_start_all_queues(adapter->netdev);

	sxevf_task_timer_trigger(adapter);

	LOG_INFO_BDF("up finish\n");
	return ;
}

void sxevf_hw_reinit(struct sxevf_adapter *adapter)
{
	WARN_ON(in_interrupt());

	while (test_and_set_bit(SXEVF_RESETTING, &adapter->state)) {
		msleep(1);
	}

	sxevf_down(adapter);
	pci_set_master(adapter->pdev);
	sxevf_up(adapter);
	clear_bit(SXEVF_RESETTING, &adapter->state);

	LOG_INFO_BDF("reinit finish\n");
	return;
}

static void sxevf_resource_release(struct sxevf_adapter *adapter)
{
	sxevf_irq_release(adapter);

	sxevf_rx_release(adapter);

	sxevf_tx_release(adapter);

	return;
}

void sxevf_terminate(struct sxevf_adapter *adapter)
{
	sxevf_down(adapter);

	sxevf_resource_release(adapter);
}

int sxevf_close(struct net_device *netdev)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	if (netif_device_present(netdev)) {
		sxevf_terminate(adapter);
	}

	LOG_INFO_BDF("close finish\n");
	return 0;
}

int sxevf_vlan_rx_add_vid(struct net_device *netdev,
				   __be16 proto, u16 vid)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	struct sxevf_hw *hw = &adapter->hw;
	int ret;

	LOG_DEBUG_BDF("add vlan[%u] in vfta\n", vid);

	spin_lock_bh(&adapter->mbx_lock);
	ret = sxe_vf_filter_array_vid_update(hw, vid, true);
	spin_unlock_bh(&adapter->mbx_lock);

	if (ret == -SXEVF_ERR_MSG_HANDLE_ERR) {
		LOG_ERROR_BDF("pf vf mailbox msg handle error\n");
		ret = -EACCES;
		goto l_ret;
	}

	if (ret != -SXEVF_ERR_MSG_HANDLE_ERR && ret != 0) {
		LOG_ERROR_BDF("pf response error ret = %d\n", ret);
		ret = -EIO;
		goto l_ret;
	}

	set_bit(vid, adapter->active_vlans);

l_ret:
	return ret;
}

int sxevf_vlan_rx_kill_vid(struct net_device *netdev,
				    __be16 proto, u16 vid)
{
	struct sxevf_adapter *adapter = netdev_priv(netdev);
	struct sxevf_hw *hw = &adapter->hw;
	int ret;

	spin_lock_bh(&adapter->mbx_lock);

	LOG_DEBUG_BDF("delete vlan[%u] in vfta\n", vid);
	ret = sxe_vf_filter_array_vid_update(hw, vid, false);

	if (ret == -SXEVF_ERR_MSG_HANDLE_ERR) {
		LOG_ERROR_BDF("pf vf mailbox msg handle error\n");
		ret = -EACCES;
	} else if (ret != -SXEVF_ERR_MSG_HANDLE_ERR && ret != 0) {
		LOG_ERROR_BDF("pf response error ret = %d\n", ret);
		ret = -EIO;
	}

	spin_unlock_bh(&adapter->mbx_lock);

	clear_bit(vid, adapter->active_vlans);

	return ret;
}

static netdev_features_t sxevf_features_check(struct sk_buff *skb,
						struct net_device *dev,
						netdev_features_t features)
{
	u32 network_hdr_len, mac_hdr_len;
	struct sxevf_adapter *adapter = netdev_priv(dev);

	mac_hdr_len = skb_network_header(skb) - skb->data;
	if (unlikely(mac_hdr_len > SXEVF_MAX_MAC_HDR_LEN)) {
		LOG_DEBUG_BDF("mac_hdr_len=%u > %u\n",
				mac_hdr_len, SXEVF_MAX_MAC_HDR_LEN);
		SKB_DUMP(skb);
		features &= ~(NETIF_F_HW_CSUM |
			    NETIF_F_SCTP_CRC |
			    NETIF_F_HW_VLAN_CTAG_TX |
			    NETIF_F_TSO |
			    NETIF_F_TSO6);
		goto l_ret;
	}

	network_hdr_len = skb_checksum_start(skb) - skb_network_header(skb);
	if (unlikely(network_hdr_len >  SXEVF_MAX_NETWORK_HDR_LEN)) {
		LOG_DEBUG_BDF("network_hdr_len=%u > %u\n",
				network_hdr_len, SXEVF_MAX_NETWORK_HDR_LEN);
		SKB_DUMP(skb);
		features &= ~(NETIF_F_HW_CSUM |
			    NETIF_F_SCTP_CRC |
			    NETIF_F_TSO |
			    NETIF_F_TSO6);
		goto l_ret;
	}

	if (skb->encapsulation && !(features & NETIF_F_TSO_MANGLEID)) {
		features &= ~NETIF_F_TSO;
	}

l_ret:
	return features;
}

void sxevf_update_stats(struct sxevf_adapter *adapter)
{
	u32 i;
	struct sxevf_hw *hw = &adapter->hw;
	u64 alloc_rx_page_failed = 0, alloc_rx_buff_failed = 0;
	u64 alloc_rx_page = 0, hw_csum_rx_error = 0;
	struct sxevf_ring **rx_ring  = adapter->rx_ring_ctxt.ring;

	if (test_bit(SXEVF_DOWN, &adapter->state) ||
	    test_bit(SXEVF_RESETTING, &adapter->state)) {
	    LOG_WARN_BDF("adapter state:0x%lx.\n",
	    	        adapter->state);
		goto l_end;
	}

	hw->stat.ops->packet_stats_get(hw, &adapter->stats.hw);

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		alloc_rx_page        += rx_ring[i]->rx_stats.alloc_rx_page;
		alloc_rx_page_failed += rx_ring[i]->rx_stats.alloc_rx_page_failed;
		alloc_rx_buff_failed += rx_ring[i]->rx_stats.alloc_rx_buff_failed;
		hw_csum_rx_error     += rx_ring[i]->rx_stats.csum_err;
	}

	adapter->stats.sw.alloc_rx_page        = alloc_rx_page;
	adapter->stats.sw.alloc_rx_page_failed = alloc_rx_page_failed;
	adapter->stats.sw.alloc_rx_buff_failed = alloc_rx_buff_failed;
	adapter->stats.sw.hw_csum_rx_error     = hw_csum_rx_error;

l_end:
	return;
}

static void sxevf_ring_stats64_get(struct rtnl_link_stats64 *stats,
				   struct sxevf_ring *ring,
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
sxevf_get_stats64(struct net_device *netdev, struct rtnl_link_stats64 *stats)
#else
static void sxevf_get_stats64(struct net_device *netdev,
			      struct rtnl_link_stats64 *stats)
#endif
{
	u32 i;
	struct sxevf_ring *ring;
	struct sxevf_adapter *adapter = netdev_priv(netdev);

	sxevf_update_stats(adapter);

	stats->multicast = adapter->stats.hw.vfmprc - adapter->stats.hw.base_vfmprc;
	stats->multicast += adapter->stats.hw.saved_reset_vfmprc;

	rcu_read_lock();

	for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
		ring = adapter->rx_ring_ctxt.ring[i];
		sxevf_ring_stats64_get(stats, ring, true);
	}

	for (i = 0; i < adapter->tx_ring_ctxt.num; i++) {
		ring = adapter->tx_ring_ctxt.ring[i];
		sxevf_ring_stats64_get(stats, ring, false);
	}

	for (i = 0; i < adapter->xdp_ring_ctxt.num; i++) {
		ring = adapter->xdp_ring_ctxt.ring[i];
		sxevf_ring_stats64_get(stats, ring, false);
	}

	rcu_read_unlock();

#ifdef NO_VOID_NDO_GET_STATS64
	return stats;
#endif
}

static const struct net_device_ops sxevf_netdev_ops = {
	.ndo_open		= sxevf_open,
	.ndo_stop		= sxevf_close,
	.ndo_set_mac_address	= sxevf_mac_addr_set,
	.ndo_start_xmit		= sxevf_xmit,
	.ndo_set_rx_mode	= sxevf_set_rx_mode,
	.ndo_validate_addr	= eth_validate_addr,
#ifdef HAVE_NET_DEVICE_EXTENDED
	.ndo_size		= sizeof(struct net_device_ops),
	.extended.ndo_change_mtu	= sxevf_change_mtu,
#else
	.ndo_change_mtu			= sxevf_change_mtu,
#endif
	.ndo_vlan_rx_add_vid	= sxevf_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid	= sxevf_vlan_rx_kill_vid,
	.ndo_tx_timeout		= sxevf_tx_timeout,
	.ndo_features_check	= sxevf_features_check,

	.ndo_get_stats64	= sxevf_get_stats64,  
#ifdef HAVE_XDP_SUPPORT
	.ndo_bpf		= sxevf_xdp,
#endif
};

static void sxevf_netdev_ops_init(struct net_device *netdev)
{
	netdev->netdev_ops = &sxevf_netdev_ops;
	return;
}

STATIC void sxevf_netdev_feature_init(struct net_device *netdev)
{
	netdev->features = NETIF_F_SG    |  
			NETIF_F_SCTP_CRC |  
			NETIF_F_RXCSUM   |  
			NETIF_F_HW_CSUM;    

	netdev->gso_partial_features = SXEVF_GSO_PARTIAL_FEATURES;
	netdev->features |= NETIF_F_TSO   |      
			    NETIF_F_TSO6  |
			    NETIF_F_GSO_PARTIAL|
			    SXEVF_GSO_PARTIAL_FEATURES;

#ifdef SXE_IPSEC_CONFIGURE
	netdev->features |= SXEVF_ESP_FEATURES;
	netdev->hw_enc_features |= SXEVF_ESP_FEATURES;
#endif

	netdev->hw_features |= netdev->features;

	if (dma_get_mask(netdev->dev.parent) == DMA_BIT_MASK(SXEVF_DMA_BIT_WIDTH_64)) {
		netdev->features |= NETIF_F_HIGHDMA;
	}

	netdev->vlan_features |= netdev->features | NETIF_F_TSO_MANGLEID;

	netdev->mpls_features |= NETIF_F_SG  |
				NETIF_F_TSO  |
				NETIF_F_TSO6 |
				NETIF_F_HW_CSUM;
	netdev->mpls_features |= SXEVF_GSO_PARTIAL_FEATURES;

	netdev->hw_enc_features |= netdev->vlan_features;

	netdev->features |= NETIF_F_HW_VLAN_CTAG_FILTER |
			    NETIF_F_HW_VLAN_CTAG_RX |
			    NETIF_F_HW_VLAN_CTAG_TX;

	return;
}

static void sxevf_netdev_name_init(struct net_device *netdev,
					struct pci_dev *pdev)
{
	strlcpy(netdev->name, pci_name(pdev), sizeof(netdev->name));

	return;
}

#ifndef NO_NETDEVICE_MIN_MAX_MTU
static void sxevf_netdev_mtu_init(struct sxevf_adapter *adapter)
{
	u32 max_mtu;
	struct net_device *netdev = adapter->netdev;

	if (adapter->hw.board_type == SXE_BOARD_VF_HV) {
		max_mtu = ETH_DATA_LEN;
	} else {
		max_mtu = SXEVF_JUMBO_FRAME_SIZE_MAX - SXEVF_ETH_DEAD_LOAD;
	}
#ifdef HAVE_NET_DEVICE_EXTENDED
	netdev->extended->min_mtu = ETH_MIN_MTU;
	netdev->extended->max_mtu = max_mtu;

	LOG_INFO("max_mtu:%u min_mtu:%u.\n",
		netdev->extended->max_mtu, netdev->extended->min_mtu);
#else
	netdev->min_mtu = ETH_MIN_MTU;
	netdev->max_mtu = max_mtu;

	LOG_INFO("max_mtu:%u min_mtu:%u.\n", netdev->max_mtu, netdev->min_mtu);
#endif
	return;
}
#endif 

static void sxevf_netdev_priv_flags_init(struct net_device *netdev)
{
	netdev->priv_flags |= IFF_UNICAST_FLT;

	return;
}

void sxevf_netdev_init(struct sxevf_adapter *adapter, struct pci_dev *pdev)
{
	struct net_device *netdev = adapter->netdev;

	SET_NETDEV_DEV(netdev, &pdev->dev);

	sxevf_netdev_ops_init(netdev);

	sxevf_netdev_name_init(netdev, pdev);

	sxevf_netdev_feature_init(netdev);

	sxevf_netdev_priv_flags_init(netdev);

#ifndef NO_NETDEVICE_MIN_MAX_MTU
	sxevf_netdev_mtu_init(adapter);
#endif

	sxevf_ethtool_ops_set(netdev);

	return ;
}
