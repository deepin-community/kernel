// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt
#include <net/dsfield.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/netlink.h>
#include <linux/debugfs.h>
#include <linux/ip.h>

#include "sss_kernel.h"
#ifdef HAVE_XDP_SUPPORT
#include <linux/bpf.h>
#endif
#include "sss_hw.h"
#include "sss_nic_io.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_tx.h"
#include "sss_nic_rx.h"
#include "sss_nic_dcb.h"
#include "sss_nic_netdev_ops_api.h"
#include "sss_nic_cfg.h"
#include "sss_nic_vf_cfg.h"
#include "sss_nic_mag_cfg.h"

#define SSSNIC_MAX_VLAN_ID	4094
#define SSSNIC_MAX_QOS_NUM	7

#define SSSNIC_TX_RATE_TABLE_FULL	12

static int sss_nic_ndo_open(struct net_device *netdev)
{
	int ret;
	struct sss_nic_qp_info qp_info = {0};
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_INTF_UP)) {
		nicif_info(nic_dev, drv, netdev, "Netdev already open\n");
		return 0;
	}

	ret = sss_nic_io_resource_init(nic_dev->nic_io);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to init nic io resource\n");
		return ret;
	}

	ret = sss_nic_dev_resource_init(nic_dev);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to init qp resource\n");
		goto init_dev_res_err;
	}

	ret = sss_nic_qp_resource_init(nic_dev, &qp_info, &nic_dev->qp_res);
	if (ret != 0)
		goto alloc_qp_res_err;

	ret = sss_nic_open_dev(nic_dev, &qp_info, &nic_dev->qp_res);
	if (ret != 0)
		goto open_chan_err;

	ret = sss_nic_vport_up(nic_dev);
	if (ret != 0)
		goto vport_err;

	SSSNIC_SET_NIC_DEV_FLAG(nic_dev, SSSNIC_INTF_UP);
	nicif_info(nic_dev, drv, nic_dev->netdev, "Netdev is up\n");

	return 0;

vport_err:
	sss_nic_close_dev(nic_dev, &qp_info);

open_chan_err:
	sss_nic_qp_resource_deinit(nic_dev, &qp_info, &nic_dev->qp_res);

alloc_qp_res_err:
	sss_nic_dev_resource_deinit(nic_dev);

init_dev_res_err:
	sss_nic_io_resource_deinit(nic_dev->nic_io);

	return ret;
}

static int sss_nic_ndo_stop(struct net_device *netdev)
{
	struct sss_nic_qp_info qp_info = {0};
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (!SSSNIC_TEST_CLEAR_NIC_DEV_FLAG(nic_dev, SSSNIC_INTF_UP)) {
		nicif_info(nic_dev, drv, netdev, "Netdev already close\n");
		return 0;
	}

	if (SSSNIC_TEST_CLEAR_NIC_DEV_FLAG(nic_dev, SSSNIC_CHANGE_RES_INVALID))
		goto out;

	sss_nic_vport_down(nic_dev);
	sss_nic_close_dev(nic_dev, &qp_info);
	sss_nic_qp_resource_deinit(nic_dev, &qp_info, &nic_dev->qp_res);

out:
	sss_nic_io_resource_deinit(nic_dev->nic_io);
	sss_nic_dev_resource_deinit(nic_dev);

	nicif_info(nic_dev, drv, nic_dev->netdev, "Netdev is down\n");

	return 0;
}

#if defined(HAVE_NDO_SELECT_QUEUE_SB_DEV_ONLY)
static u16 sss_nic_ndo_select_queue(struct net_device *netdev, struct sk_buff *skb,
				    struct net_device *sb_dev)
#elif defined(HAVE_NDO_SELECT_QUEUE_ACCEL_FALLBACK)
#if defined(HAVE_NDO_SELECT_QUEUE_SB_DEV)
static u16 sss_nic_ndo_select_queue(struct net_device *netdev, struct sk_buff *skb,
				    struct net_device *sb_dev,
				    select_queue_fallback_t fallback)
#else
static u16 sss_nic_ndo_select_queue(struct net_device *netdev, struct sk_buff *skb,
				    __always_unused void *accel,
				    select_queue_fallback_t fallback)
#endif
#elif defined(HAVE_NDO_SELECT_QUEUE_ACCEL)
static u16 sss_nic_ndo_select_queue(struct net_device *netdev, struct sk_buff *skb,
				    __always_unused void *accel)
#else
static u16 sss_nic_ndo_select_queue(struct net_device *netdev, struct sk_buff *skb)
#endif /* end of HAVE_NDO_SELECT_QUEUE_ACCEL_FALLBACK */
{
	u8 cos;
	u8 qp_num;
	u16 sq_num;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_SAME_RXTX))
		return sss_nic_select_queue_by_hash_func(netdev, skb, netdev->real_num_tx_queues);

	sq_num =
#if defined(HAVE_NDO_SELECT_QUEUE_SB_DEV_ONLY)
		netdev_pick_tx(netdev, skb, NULL);
#elif defined(HAVE_NDO_SELECT_QUEUE_ACCEL_FALLBACK)
#ifdef HAVE_NDO_SELECT_QUEUE_SB_DEV
		fallback(netdev, skb, sb_dev);
#else
		fallback(netdev, skb);
#endif
#else
		skb_tx_hash(netdev, skb);
#endif

	if (likely(!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE)))
		return sq_num;

	cos = sss_nic_get_cos(nic_dev, skb);

	qp_num = (nic_dev->hw_dcb_cfg.cos_qp_num[cos] != 0) ?
		  sq_num % nic_dev->hw_dcb_cfg.cos_qp_num[cos] : 0;
	sq_num = nic_dev->hw_dcb_cfg.cos_qp_offset[cos] + qp_num;

	return sq_num;
}

#ifdef HAVE_NDO_GET_STATS64
#ifdef HAVE_VOID_NDO_GET_STATS64
static void sss_nic_ndo_get_stats64(struct net_device *netdev,
				    struct rtnl_link_stats64 *stats)
#else
static struct rtnl_link_stats64 *sss_nic_ndo_get_stats64(struct net_device *netdev,
							 struct rtnl_link_stats64 *stats)
#endif

#else /* !HAVE_NDO_GET_STATS64 */
static struct net_device_stats *sss_nic_ndo_get_stats(struct net_device *netdev)
#endif
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
#ifndef HAVE_NDO_GET_STATS64
#ifdef HAVE_NETDEV_STATS_IN_NETDEV
	struct net_device_stats *stats = &netdev->stats;
#else
	struct net_device_stats *stats = &nic_dev->net_stats;
#endif /* HAVE_NETDEV_STATS_IN_NETDEV */
#endif /* HAVE_NDO_GET_STATS64 */

	sss_nic_get_tx_stats(nic_dev, stats);
	sss_nic_get_rx_stats(nic_dev, stats);

#ifndef HAVE_VOID_NDO_GET_STATS64
	return stats;
#endif
}

#ifdef HAVE_TX_TIMEOUT_TXQUEUE
static void sss_nic_ndo_tx_timeout(struct net_device *netdev,
				   unsigned int __maybe_unused queue)
#else
static void sss_nic_ndo_tx_timeout(struct net_device *netdev)
#endif
{
	struct sss_nic_io_queue *sq = NULL;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	u32 sw_pi;
	u32 hw_ci;
	u8 qid;

	SSSNIC_STATS_TX_TIMEOUT_INC(nic_dev);
	nicif_err(nic_dev, drv, netdev, "Tx timeout\n");

	for (qid = 0; qid < nic_dev->qp_res.qp_num; qid++) {
		if (!netif_xmit_stopped(netdev_get_tx_queue(netdev, qid)))
			continue;

		sq = nic_dev->sq_desc_group[qid].sq;
		sw_pi = sss_nic_get_sq_local_pi(sq);
		hw_ci = sss_nic_get_sq_hw_ci(sq);
		nicif_info(nic_dev, drv, netdev,
			   "Sq%u: sw_pi: %u, hw_ci: %u, sw_ci: %u, napi state: 0x%lx.\n",
			qid, sw_pi, hw_ci, sss_nic_get_sq_local_ci(sq),
			nic_dev->qp_res.irq_cfg[qid].napi.state);

		if (sw_pi != hw_ci) {
			SSSNIC_SET_NIC_EVENT_FLAG(nic_dev, SSSNIC_EVENT_TX_TIMEOUT);
			return;
		}
	}
}

static int sss_nic_ndo_change_mtu(struct net_device *netdev, int new_mtu)
{
	int ret = 0;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

#ifdef HAVE_XDP_SUPPORT
	u32 xdp_max_mtu;

	if (SSSNIC_IS_XDP_ENABLE(nic_dev)) {
		xdp_max_mtu = SSSNIC_XDP_MAX_MTU(nic_dev);
		if (new_mtu > xdp_max_mtu) {
			nicif_err(nic_dev, drv, netdev,
				  "Fail to change mtu to %d, max mtu is %d\n",
				  new_mtu, xdp_max_mtu);
			return -EINVAL;
		}
	}
#endif

	ret = sss_nic_set_dev_mtu(nic_dev, (u16)new_mtu);
	if (ret) {
		nicif_err(nic_dev, drv, netdev, "Fail to change mtu to %d\n",
			  new_mtu);
		return ret;
	}

	nicif_info(nic_dev, drv, nic_dev->netdev, "Success to change mtu from %u to %d\n",
		   netdev->mtu, new_mtu);

	netdev->mtu = new_mtu;

	return 0;
}

static int sss_nic_ndo_set_mac_address(struct net_device *netdev, void *mac_addr)
{
	int ret = 0;
	struct sockaddr *set_addr = mac_addr;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (!is_valid_ether_addr(set_addr->sa_data))
		return -EADDRNOTAVAIL;

	if (ether_addr_equal(netdev->dev_addr, set_addr->sa_data)) {
		nicif_info(nic_dev, drv, netdev,
			   "Already using mac addr: %pM\n", set_addr->sa_data);
		return 0;
	}

	ret = sss_nic_update_mac(nic_dev, set_addr->sa_data);
	if (ret)
		return ret;

	ether_addr_copy((u8 *)(netdev->dev_addr), set_addr->sa_data);

	nicif_info(nic_dev, drv, netdev,
		   "Success to set new mac addr: %pM\n", set_addr->sa_data);

	return 0;
}

int sss_nic_ndo_vlan_rx_add_vid(struct net_device *netdev,
				__always_unused __be16 proto, u16 vlan_id)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	int ret;

	if (vlan_id == 0)
		return 0;

	ret = sss_nic_config_vlan(nic_dev, SSSNIC_MBX_OPCODE_ADD, vlan_id);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to add vlan %u\n", vlan_id);
		return ret;
	}

	SSSNIC_SET_VLAN_BITMAP(nic_dev, vlan_id);
	nicif_info(nic_dev, drv, netdev, "Success to add vlan %u\n", vlan_id);

	return 0;
}

int sss_nic_ndo_vlan_rx_kill_vid(struct net_device *netdev,
				 __always_unused __be16 proto, u16 vlan_id)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	int ret;

	if (vlan_id == 0)
		return 0;

	ret = sss_nic_config_vlan(nic_dev, SSSNIC_MBX_OPCODE_DEL, vlan_id);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to delete vlan\n");
		return ret;
	}

	SSSNIC_CLEAR_VLAN_BITMAP(nic_dev, vlan_id);
	nicif_info(nic_dev, drv, netdev, "Success to delete vlan %u\n", vlan_id);

	return 0;
}

static netdev_features_t sss_nic_ndo_fix_features(struct net_device *netdev,
						  netdev_features_t features)
{
	netdev_features_t netdev_feature = features;

	/* If Rx checksum is disabled, then LRO should also be disabled */
	if ((netdev_feature & NETIF_F_RXCSUM) == 0)
		netdev_feature &= ~NETIF_F_LRO;

	return netdev_feature;
}

static int sss_nic_ndo_set_features(struct net_device *netdev, netdev_features_t features)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	return sss_nic_set_feature(nic_dev, nic_dev->netdev->features, features);
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void sss_nic_ndo_poll_controller(struct net_device *netdev)
{
	u16 i;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	for (i = 0; i < nic_dev->qp_res.qp_num; i++)
		napi_schedule(&nic_dev->qp_res.irq_cfg[i].napi);
}
#endif

static void sss_nic_ndo_set_rx_mode(struct net_device *netdev)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (netdev_uc_count(netdev) != nic_dev->netdev_uc_cnt ||
	    netdev_mc_count(netdev) != nic_dev->netdev_mc_cnt) {
		nic_dev->netdev_uc_cnt = netdev_uc_count(netdev);
		nic_dev->netdev_mc_cnt = netdev_mc_count(netdev);
		SSSNIC_SET_NIC_DEV_FLAG(nic_dev, SSSNIC_UPDATE_MAC_FILTER);
	}

	queue_work(nic_dev->workq, &nic_dev->rx_mode_work);
}

static int sss_nic_ndo_set_vf_mac(struct net_device *netdev, int vf_id, u8 *mac)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	struct sss_nic_io *nic_io = nic_dev->nic_io;
	struct sss_nic_vf_info *vf_info = NULL;

	if (vf_id >= pci_num_vf(nic_dev->pdev) ||
	    is_multicast_ether_addr(mac))
		return -EINVAL;

	vf_info = &nic_io->vf_info_group[vf_id];
	ether_addr_copy(vf_info->user_mac, mac);

	if (is_zero_ether_addr(mac))
		nic_info(nic_dev->dev_hdl,
			 "Success to delete mac on vf %d\n", vf_id);
	else
		nic_info(nic_dev->dev_hdl,
			 "Success to set mac %pM on vf %d\n", mac, vf_id);

	return 0;
}

#ifdef IFLA_VF_MAX
#ifdef IFLA_VF_VLAN_INFO_MAX
static int sss_nic_ndo_set_vf_vlan(struct net_device *netdev, int vf_id, u16 vlan_id,
				   u8 qos, __be16 vlan_proto)
#else
static int sss_nic_ndo_set_vf_vlan(struct net_device *netdev, int vf_id, u16 vlan_id,
				   u8 qos)
#endif
{
	u16 pre_vlanprio;
	u16 cur_vlanprio;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (qos > SSSNIC_MAX_QOS_NUM || vlan_id > SSSNIC_MAX_VLAN_ID ||
	    vf_id >= pci_num_vf(nic_dev->pdev))
		return -EINVAL;
#ifdef IFLA_VF_VLAN_INFO_MAX
	if (vlan_proto != htons(ETH_P_8021Q))
		return -EPROTONOSUPPORT;
#endif
	pre_vlanprio = SSSNIC_GET_VLAN_PRIO(vlan_id, qos);
	cur_vlanprio =
		sss_nic_vf_info_vlan_prio(nic_dev->nic_io, SSSNIC_OS_VF_ID_TO_HW(vf_id));
	if (pre_vlanprio == cur_vlanprio)
		return 0;

	return sss_nic_set_hw_vf_vlan(nic_dev, cur_vlanprio, vf_id, vlan_id, qos);
}
#endif

#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
static int sss_nic_ndo_set_vf_spoofchk(struct net_device *netdev, int vf_id,
				       bool set_spoofchk)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	bool cur_spoofchk;
	u16 id = SSSNIC_OS_VF_ID_TO_HW(vf_id);
	int ret;

	if (vf_id >= pci_num_vf(nic_dev->pdev))
		return -EINVAL;

	cur_spoofchk = SSSNIC_GET_VF_SPOOFCHK(nic_dev->nic_io, vf_id);
	if (set_spoofchk == cur_spoofchk)
		return 0;

	ret = sss_nic_set_vf_spoofchk(nic_dev->nic_io, id, set_spoofchk);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev,
			  "Fail to %s spoofchk control for VF %d\n",
			  set_spoofchk ? "enable" : "disable", vf_id);
		return ret;
	}

	nicif_info(nic_dev, drv, netdev,
		   "Success to %s spoofchk control for VF %d\n",
		   set_spoofchk ? "enable" : "disable", vf_id);
	return 0;
}
#endif

#ifdef HAVE_NDO_SET_VF_TRUST
static int sss_nic_ndo_set_vf_trust(struct net_device *netdev, int vf_id, bool new_trust)
{
	bool old_trust;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (vf_id >= pci_num_vf(nic_dev->pdev) || vf_id > nic_dev->nic_io->max_vf_num) {
		nicif_err(nic_dev, drv, netdev, "Invalid vf id, VF: %d pci_num_vf: %d max_vfs: %d\n",
			  vf_id, pci_num_vf(nic_dev->pdev), nic_dev->nic_io->max_vf_num);
		return -EINVAL;
	}

	old_trust = !!nic_dev->nic_io->vf_info_group[vf_id].trust;
	/* Same old and new, no need to set, return success directly */
	if (new_trust == old_trust)
		return 0;

	nic_dev->nic_io->vf_info_group[vf_id].trust = !!new_trust;

	nicif_info(nic_dev, drv, netdev, "Success to set VF %d trust %d to %d\n",
		   vf_id, old_trust, new_trust);

	return 0;
}
#endif

static int sss_nic_ndo_get_vf_config(struct net_device *netdev,
				     int vf_id, struct ifla_vf_info *ifla_vf)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (vf_id >= pci_num_vf(nic_dev->pdev))
		return -EINVAL;

	sss_nic_get_vf_attribute(nic_dev->nic_io, (u16)SSSNIC_OS_VF_ID_TO_HW(vf_id), ifla_vf);

	return 0;
}

int sss_nic_ndo_set_vf_link_state(struct net_device *netdev, int vf_id, int link)
{
	int ret;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (vf_id >= pci_num_vf(nic_dev->pdev)) {
		nicif_err(nic_dev, drv, netdev,
			  "Invalid VF Id %d, pci_num_vf %d\n", vf_id, pci_num_vf(nic_dev->pdev));
		return -EINVAL;
	}

	ret = sss_nic_set_vf_link_state(nic_dev->nic_io, (u16)SSSNIC_OS_VF_ID_TO_HW(vf_id), link);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to set VF %d link state %d\n", vf_id, link);
		return ret;
	}

	nicif_info(nic_dev, drv, netdev, "Success to set VF %d link state %d\n",
		   vf_id, link);

	return 0;
}

static int sss_nic_check_vf_bw_param(const struct sss_nic_dev *nic_dev,
				     int vf_id, int min_rate, int max_rate)
{
	if (!SSSNIC_SUPPORT_RATE_LIMIT(nic_dev->nic_io)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupport to set vf rate limit.\n");
		return -EOPNOTSUPP;
	}

	if (vf_id >= pci_num_vf(nic_dev->pdev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Invalid VF number %d\n",
			  pci_num_vf(nic_dev->pdev));
		return -EINVAL;
	}

	if (max_rate < min_rate) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Invalid rate, maximum rate %d minimum rate %d\n",
			  max_rate, min_rate);
		return -EINVAL;
	}

	if (max_rate < 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Invalid maximum rate %d\n", max_rate);
		return -EINVAL;
	}

	return 0;
}

#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
static int sss_nic_ndo_set_vf_rate(struct net_device *netdev,
				   int vf_id, int min_tx_rate, int max_tx_rate)
#else
static int sss_nic_ndo_set_vf_tx_rate(struct net_device *netdev, int vf_id,
				      int max_tx_rate)
#endif /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */
{
#ifndef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	int min_tx_rate = 0;
#endif
	u8 link_status;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	struct sss_nic_port_info port_info = {0};
	u32 speeds[] = {0, SPEED_10, SPEED_100, SPEED_1000, SPEED_10000,
			SPEED_25000, SPEED_40000, SPEED_50000, SPEED_100000,
			SPEED_200000
		       };
	int ret;

	ret = sss_nic_check_vf_bw_param(nic_dev, vf_id, min_tx_rate, max_tx_rate);
	if (ret != 0)
		return ret;

	ret = sss_nic_get_hw_link_state(nic_dev, &link_status);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev,
			  "Fail to get link status when set vf tx rate.\n");
		return -EIO;
	}

	if (link_status == 0) {
		nicif_err(nic_dev, drv, netdev,
			  "Fail to set vf tx rate. the link state is down.\n");
		return -EINVAL;
	}

	ret = sss_nic_get_hw_port_info(nic_dev, &port_info,
				       SSS_CHANNEL_NIC);
	if (ret != 0 || port_info.speed >= SSSNIC_PORT_SPEED_UNKNOWN)
		return -EIO;

	if (max_tx_rate > speeds[port_info.speed]) {
		nicif_err(nic_dev, drv, netdev, "Invalid max_tx_rate, it must be in [0 - %u]\n",
			  speeds[port_info.speed]);
		return -EINVAL;
	}

	ret = sss_nic_set_vf_tx_rate_limit(nic_dev->nic_io, (u16)SSSNIC_OS_VF_ID_TO_HW(vf_id),
					   (u32)min_tx_rate, (u32)max_tx_rate);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev,
			  "Fail to set VF %d max rate %d min rate %d%s\n",
			  vf_id, max_tx_rate, min_tx_rate,
			  ret == SSSNIC_TX_RATE_TABLE_FULL ?
			  ", tx rate profile is full" : "");
		return -EIO;
	}

	nicif_info(nic_dev, drv, netdev,
		   "Success to set VF %d tx rate [%u-%u]\n",
		   vf_id, min_tx_rate, max_tx_rate);

	return 0;
}

#ifdef HAVE_XDP_SUPPORT
#ifdef HAVE_NDO_BPF_NETDEV_BPF
static int sss_nic_ndo_bpf(struct net_device *netdev, struct netdev_bpf *xdp)
#else
static int sss_nic_ndo_xdp(struct net_device *netdev, struct netdev_xdp *xdp)
#endif
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

#ifdef HAVE_XDP_QUERY_PROG
	if (xdp->command ==  XDP_QUERY_PROG) {
		xdp->prog_id = nic_dev->xdp_prog ? nic_dev->xdp_prog->aux->id : 0;
		return 0;
	}
#endif
	if (xdp->command == XDP_SETUP_PROG)
		return sss_nic_setup_xdp(nic_dev, xdp);

	return -EINVAL;
}
#endif

static const struct net_device_ops g_nic_netdev_ops = {
	.ndo_open = sss_nic_ndo_open,
	.ndo_stop = sss_nic_ndo_stop,
	.ndo_start_xmit = sss_nic_ndo_start_xmit,

#ifdef HAVE_NDO_GET_STATS64
	.ndo_get_stats64 =  sss_nic_ndo_get_stats64,
#else
	.ndo_get_stats = sss_nic_ndo_get_stats,
#endif /* HAVE_NDO_GET_STATS64 */

	.ndo_tx_timeout = sss_nic_ndo_tx_timeout,
	.ndo_select_queue = sss_nic_ndo_select_queue,
#ifdef HAVE_RHEL7_NETDEV_OPS_EXT_NDO_CHANGE_MTU
	.extended.ndo_change_mtu = sss_nic_ndo_change_mtu,
#else
	.ndo_change_mtu = sss_nic_ndo_change_mtu,
#endif
	.ndo_set_mac_address = sss_nic_ndo_set_mac_address,
	.ndo_validate_addr = eth_validate_addr,

#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	.ndo_vlan_rx_add_vid = sss_nic_ndo_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = sss_nic_ndo_vlan_rx_kill_vid,
#endif

#ifdef HAVE_RHEL7_NET_DEVICE_OPS_EXT
	/* RHEL7 requires this to be defined to enable extended ops.  RHEL7
	 * uses the function get_ndo_ext to retrieve offsets for extended
	 * fields from with the net_device_ops struct and ndo_size is checked
	 * to determine whether or not the offset is valid.
	 */
	.ndo_size		= sizeof(const struct net_device_ops),
#endif

#ifdef IFLA_VF_MAX
	.ndo_set_vf_mac		= sss_nic_ndo_set_vf_mac,
#ifdef HAVE_RHEL7_NETDEV_OPS_EXT_NDO_SET_VF_VLAN
	.extended.ndo_set_vf_vlan = sss_nic_ndo_set_vf_vlan,
#else
	.ndo_set_vf_vlan	= sss_nic_ndo_set_vf_vlan,
#endif
#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	.ndo_set_vf_rate	= sss_nic_ndo_set_vf_rate,
#else
	.ndo_set_vf_tx_rate	= sss_nic_ndo_set_vf_tx_rate,
#endif /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */
#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
	.ndo_set_vf_spoofchk	= sss_nic_ndo_set_vf_spoofchk,
#endif

#ifdef HAVE_NDO_SET_VF_TRUST
#ifdef HAVE_RHEL7_NET_DEVICE_OPS_EXT
	.extended.ndo_set_vf_trust = sss_nic_ndo_set_vf_trust,
#else
	.ndo_set_vf_trust	= sss_nic_ndo_set_vf_trust,
#endif /* HAVE_RHEL7_NET_DEVICE_OPS_EXT */
#endif /* HAVE_NDO_SET_VF_TRUST */

	.ndo_get_vf_config	= sss_nic_ndo_get_vf_config,
#endif

#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller = sss_nic_ndo_poll_controller,
#endif /* CONFIG_NET_POLL_CONTROLLER */

	.ndo_set_rx_mode = sss_nic_ndo_set_rx_mode,

#ifdef HAVE_XDP_SUPPORT
#ifdef HAVE_NDO_BPF_NETDEV_BPF
	.ndo_bpf = sss_nic_ndo_bpf,
#else
	.ndo_xdp = sss_nic_ndo_xdp,
#endif
#endif

#ifdef HAVE_NDO_SET_VF_LINK_STATE
	.ndo_set_vf_link_state	= sss_nic_ndo_set_vf_link_state,
#endif

#ifdef HAVE_NDO_SET_FEATURES
	.ndo_fix_features = sss_nic_ndo_fix_features,
	.ndo_set_features = sss_nic_ndo_set_features,
#endif /* HAVE_NDO_SET_FEATURES */
};

static const struct net_device_ops g_nicvf_netdev_ops = {
	.ndo_open = sss_nic_ndo_open,
	.ndo_stop = sss_nic_ndo_stop,
	.ndo_start_xmit = sss_nic_ndo_start_xmit,

#ifdef HAVE_NDO_GET_STATS64
	.ndo_get_stats64 =  sss_nic_ndo_get_stats64,
#else
	.ndo_get_stats = sss_nic_ndo_get_stats,
#endif /* HAVE_NDO_GET_STATS64 */

	.ndo_tx_timeout = sss_nic_ndo_tx_timeout,
	.ndo_select_queue = sss_nic_ndo_select_queue,

#ifdef HAVE_RHEL7_NET_DEVICE_OPS_EXT
	/* RHEL7 requires this to be defined to enable extended ops.  RHEL7
	 * uses the function get_ndo_ext to retrieve offsets for extended
	 * fields from with the net_device_ops struct and ndo_size is checked
	 * to determine whether or not the offset is valid.
	 */
	.ndo_size = sizeof(const struct net_device_ops),
#endif

#ifdef HAVE_RHEL7_NETDEV_OPS_EXT_NDO_CHANGE_MTU
	.extended.ndo_change_mtu = sss_nic_ndo_change_mtu,
#else
	.ndo_change_mtu = sss_nic_ndo_change_mtu,
#endif
	.ndo_set_mac_address = sss_nic_ndo_set_mac_address,
	.ndo_validate_addr = eth_validate_addr,

#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	.ndo_vlan_rx_add_vid = sss_nic_ndo_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = sss_nic_ndo_vlan_rx_kill_vid,
#endif

#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller = sss_nic_ndo_poll_controller,
#endif /* CONFIG_NET_POLL_CONTROLLER */

	.ndo_set_rx_mode = sss_nic_ndo_set_rx_mode,

#ifdef HAVE_XDP_SUPPORT
#ifdef HAVE_NDO_BPF_NETDEV_BPF
	.ndo_bpf = sss_nic_ndo_bpf,
#else
	.ndo_xdp = sss_nic_ndo_xdp,
#endif
#endif

#ifdef HAVE_NDO_SET_FEATURES
	.ndo_fix_features = sss_nic_ndo_fix_features,
	.ndo_set_features = sss_nic_ndo_set_features,
#endif /* HAVE_NDO_SET_FEATURES */
};

void sss_nic_set_netdev_ops(struct sss_nic_dev *nic_dev)
{
	if (!SSSNIC_FUNC_IS_VF(nic_dev->hwdev))
		nic_dev->netdev->netdev_ops = &g_nic_netdev_ops;
	else
		nic_dev->netdev->netdev_ops = &g_nicvf_netdev_ops;
}

bool sss_nic_is_netdev_ops_match(const struct net_device *netdev)
{
	return netdev->netdev_ops == &g_nic_netdev_ops ||
	       netdev->netdev_ops == &g_nicvf_netdev_ops;
}
