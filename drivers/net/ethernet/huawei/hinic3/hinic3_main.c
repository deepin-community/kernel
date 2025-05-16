// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/if_vlan.h>
#include <linux/ethtool.h>
#include <linux/dcbnl.h>
#include <linux/tcp.h>
#include <linux/ip.h>
#include <linux/debugfs.h>

#include "ossl_knl.h"
#if defined(HAVE_NDO_UDP_TUNNEL_ADD) || defined(HAVE_UDP_TUNNEL_NIC_INFO)
#include <net/udp_tunnel.h>
#endif /* HAVE_NDO_UDP_TUNNEL_ADD || HAVE_UDP_TUNNEL_NIC_INFO */
#include "hinic3_hw.h"
#include "hinic3_crm.h"
#include "hinic3_mt.h"
#include "hinic3_nic_cfg.h"
#include "hinic3_srv_nic.h"
#include "hinic3_nic_io.h"
#include "hinic3_nic_dev.h"
#include "hinic3_tx.h"
#include "hinic3_rx.h"
#include "hinic3_lld.h"
#include "hinic3_srv_nic.h"
#include "hinic3_rss.h"
#include "hinic3_dcb.h"
#include "hinic3_nic_prof.h"
#include "hinic3_profile.h"
#include "hinic3_bond.h"

#define DEFAULT_POLL_WEIGHT	64
static unsigned int poll_weight = DEFAULT_POLL_WEIGHT;
module_param(poll_weight, uint, 0444);
MODULE_PARM_DESC(poll_weight, "Number packets for NAPI budget (default=64)");

#define HINIC3_DEAULT_TXRX_MSIX_PENDING_LIMIT		2
#define HINIC3_DEAULT_TXRX_MSIX_COALESC_TIMER_CFG	25
#define HINIC3_DEAULT_TXRX_MSIX_RESEND_TIMER_CFG	7

static unsigned char qp_pending_limit = HINIC3_DEAULT_TXRX_MSIX_PENDING_LIMIT;
module_param(qp_pending_limit, byte, 0444);
MODULE_PARM_DESC(qp_pending_limit, "QP MSI-X Interrupt coalescing parameter pending_limit (default=2)");

static unsigned char qp_coalesc_timer_cfg =
		HINIC3_DEAULT_TXRX_MSIX_COALESC_TIMER_CFG;
module_param(qp_coalesc_timer_cfg, byte, 0444);
MODULE_PARM_DESC(qp_coalesc_timer_cfg, "QP MSI-X Interrupt coalescing parameter coalesc_timer_cfg (default=32)");

#define DEFAULT_RX_BUFF_LEN	2
u16 hinic3_rx_buff = DEFAULT_RX_BUFF_LEN;
module_param(hinic3_rx_buff, ushort, 0444);
MODULE_PARM_DESC(hinic3_rx_buff, "Set hinic3_rx_buff size, buffer len must be 2^n. 2 - 16, default is 2KB");

static unsigned int lro_replenish_thld = 256;
module_param(lro_replenish_thld, uint, 0444);
MODULE_PARM_DESC(lro_replenish_thld, "Number wqe for lro replenish buffer (default=256)");

static unsigned char set_link_status_follow = HINIC3_LINK_FOLLOW_STATUS_MAX;
module_param(set_link_status_follow, byte, 0444);
MODULE_PARM_DESC(set_link_status_follow, "Set link status follow port status (0=default,1=follow,2=separate,3=unset");

static bool page_pool_enabled = true;
module_param(page_pool_enabled, bool, 0444);
MODULE_PARM_DESC(page_pool_enabled, "enable/disable page_pool feature for rxq page management (default enable)");

#define HINIC3_NIC_DEV_WQ_NAME		"hinic3_nic_dev_wq"

#define DEFAULT_MSG_ENABLE		(NETIF_MSG_DRV | NETIF_MSG_LINK)

#define QID_MASKED(q_id, nic_dev)	((q_id) & ((nic_dev)->num_qps - 1))
#define WATCHDOG_TIMEOUT	5

#define HINIC3_SQ_DEPTH			1024
#define HINIC3_RQ_DEPTH			1024

#define LRO_ENABLE 1

enum hinic3_rx_buff_len {
	RX_BUFF_VALID_2KB		= 2,
	RX_BUFF_VALID_4KB		= 4,
	RX_BUFF_VALID_8KB		= 8,
	RX_BUFF_VALID_16KB		= 16,
};

#define NIC_MAX_PF_NUM			32

#define CONVERT_UNIT			1024

#define BIFUR_RESOURCE_PF_SSID		0x5a1

#ifdef HAVE_MULTI_VLAN_OFFLOAD_EN
static int hinic3_netdev_event(struct notifier_block *notifier,
			       unsigned long event, void *ptr);

/* used for netdev notifier register/unregister */
static DEFINE_MUTEX(hinic3_netdev_notifiers_mutex);
static int hinic3_netdev_notifiers_ref_cnt;
static struct notifier_block hinic3_netdev_notifier = {
	.notifier_call = hinic3_netdev_event,
};

#ifdef HAVE_UDP_TUNNEL_NIC_INFO
static const struct udp_tunnel_nic_info hinic3_udp_tunnels = {
	.set_port	= hinic3_udp_tunnel_set_port,
	.unset_port	= hinic3_udp_tunnel_unset_port,
	.flags		= UDP_TUNNEL_NIC_INFO_MAY_SLEEP,
	.tables		= {
	{ .n_entries = 1, .tunnel_types = UDP_TUNNEL_TYPE_VXLAN, },
	},
};
#endif /* HAVE_UDP_TUNNEL_NIC_INFO */

static void hinic3_register_notifier(struct hinic3_nic_dev *nic_dev)
{
	int err;

	mutex_lock(&hinic3_netdev_notifiers_mutex);
	hinic3_netdev_notifiers_ref_cnt++;
	if (hinic3_netdev_notifiers_ref_cnt == 1) {
		err = register_netdevice_notifier(&hinic3_netdev_notifier);
		if (err) {
			nic_info(&nic_dev->pdev->dev, "Register netdevice notifier failed, err: %d\n",
				 err);
			hinic3_netdev_notifiers_ref_cnt--;
		}
	}
	mutex_unlock(&hinic3_netdev_notifiers_mutex);
}

static void hinic3_unregister_notifier(struct hinic3_nic_dev *nic_dev)
{
	mutex_lock(&hinic3_netdev_notifiers_mutex);
	if (hinic3_netdev_notifiers_ref_cnt == 1)
		unregister_netdevice_notifier(&hinic3_netdev_notifier);

	if (hinic3_netdev_notifiers_ref_cnt)
		hinic3_netdev_notifiers_ref_cnt--;
	mutex_unlock(&hinic3_netdev_notifiers_mutex);
}

#define HINIC3_MAX_VLAN_DEPTH_OFFLOAD_SUPPORT	1
#define HINIC3_VLAN_CLEAR_OFFLOAD	(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | \
					 NETIF_F_SCTP_CRC | NETIF_F_RXCSUM | \
					 NETIF_F_ALL_TSO)

static int hinic3_netdev_event(struct notifier_block *notifier,
			       unsigned long event, void *ptr)
{
	struct net_device *ndev = netdev_notifier_info_to_dev(ptr);
	struct net_device *real_dev = NULL;
	struct net_device *ret = NULL;
	u16 vlan_depth;

	if (!is_vlan_dev(ndev))
		return NOTIFY_DONE;

	dev_hold(ndev);

	switch (event) {
	case NETDEV_REGISTER:
		real_dev = vlan_dev_real_dev(ndev);
		if (!hinic3_is_netdev_ops_match(real_dev))
			goto out;

		vlan_depth = 1;
		ret = vlan_dev_priv(ndev)->real_dev;
		while (is_vlan_dev(ret)) {
			ret = vlan_dev_priv(ret)->real_dev;
			vlan_depth++;
		}

		if (vlan_depth == HINIC3_MAX_VLAN_DEPTH_OFFLOAD_SUPPORT) {
			ndev->vlan_features &= (~HINIC3_VLAN_CLEAR_OFFLOAD);
		} else if (vlan_depth > HINIC3_MAX_VLAN_DEPTH_OFFLOAD_SUPPORT) {
#ifdef HAVE_NDO_SET_FEATURES
#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
			set_netdev_hw_features(ndev,
					       get_netdev_hw_features(ndev) &
					       (~HINIC3_VLAN_CLEAR_OFFLOAD));
#else
			ndev->hw_features &= (~HINIC3_VLAN_CLEAR_OFFLOAD);
#endif
#endif
			ndev->features &= (~HINIC3_VLAN_CLEAR_OFFLOAD);
		}

		break;

	default:
		break;
	};

out:
	dev_put(ndev);

	return NOTIFY_DONE;
}
#endif

void hinic3_link_status_change(struct hinic3_nic_dev *nic_dev, bool status)
{
	struct net_device *netdev = nic_dev->netdev;

	if (!HINIC3_CHANNEL_RES_VALID(nic_dev) ||
	    test_bit(HINIC3_LP_TEST, &nic_dev->flags) ||
	    test_bit(HINIC3_FORCE_LINK_UP, &nic_dev->flags))
		return;

	if (status) {
		if (netif_carrier_ok(netdev))
			return;

		nic_dev->link_status = status;
		netif_carrier_on(netdev);
		nicif_info(nic_dev, link, netdev, "Link is up\n");
	} else {
		if (!netif_carrier_ok(netdev))
			return;

		nic_dev->link_status = status;
		netif_carrier_off(netdev);
		nicif_info(nic_dev, link, netdev, "Link is down\n");
	}
}

static void netdev_feature_init(struct net_device *netdev)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	netdev_features_t dft_fts = 0;
	netdev_features_t cso_fts = 0;
	netdev_features_t vlan_fts = 0;
	netdev_features_t tso_fts = 0;
	netdev_features_t hw_features = 0;

	dft_fts |= NETIF_F_SG | NETIF_F_HIGHDMA;

	if (HINIC3_SUPPORT_CSUM(nic_dev->hwdev))
		cso_fts |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM;
	if (HINIC3_SUPPORT_SCTP_CRC(nic_dev->hwdev))
		cso_fts |= NETIF_F_SCTP_CRC;

	if (HINIC3_SUPPORT_TSO(nic_dev->hwdev))
		tso_fts |= NETIF_F_TSO | NETIF_F_TSO6;

	if (HINIC3_SUPPORT_VLAN_OFFLOAD(nic_dev->hwdev)) {
#if defined(NETIF_F_HW_VLAN_CTAG_TX)
		vlan_fts |= NETIF_F_HW_VLAN_CTAG_TX;
#elif defined(NETIF_F_HW_VLAN_TX)
		vlan_fts |= NETIF_F_HW_VLAN_TX;
#endif

#if defined(NETIF_F_HW_VLAN_CTAG_RX)
		vlan_fts |= NETIF_F_HW_VLAN_CTAG_RX;
#elif defined(NETIF_F_HW_VLAN_RX)
		vlan_fts |= NETIF_F_HW_VLAN_RX;
#endif
	}

	if (HINIC3_SUPPORT_RXVLAN_FILTER(nic_dev->hwdev)) {
#if defined(NETIF_F_HW_VLAN_CTAG_FILTER)
		vlan_fts |= NETIF_F_HW_VLAN_CTAG_FILTER;
#elif defined(NETIF_F_HW_VLAN_FILTER)
		vlan_fts |= NETIF_F_HW_VLAN_FILTER;
#endif
	}

#ifdef HAVE_ENCAPSULATION_TSO
	if (HINIC3_SUPPORT_VXLAN_OFFLOAD(nic_dev->hwdev))
		tso_fts |= NETIF_F_GSO_UDP_TUNNEL | NETIF_F_GSO_UDP_TUNNEL_CSUM;
#endif /* HAVE_ENCAPSULATION_TSO */

	/* LRO is disable in default, only set hw features */
	if (HINIC3_SUPPORT_LRO(nic_dev->hwdev))
		hw_features |= NETIF_F_LRO;

	netdev->features |= dft_fts | cso_fts | tso_fts | vlan_fts;
	netdev->vlan_features |= dft_fts | cso_fts | tso_fts;

	if (nic_dev->nic_cap.lro_enable == LRO_ENABLE) {
		netdev->features |= NETIF_F_LRO;
		netdev->vlan_features |= NETIF_F_LRO;
	}

#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
	hw_features |= get_netdev_hw_features(netdev);
#else
	hw_features |= netdev->hw_features;
#endif

	hw_features |= netdev->features;

#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
	set_netdev_hw_features(netdev, hw_features);
#else
	netdev->hw_features = hw_features;
#endif

#ifdef IFF_UNICAST_FLT
	netdev->priv_flags |= IFF_UNICAST_FLT;
#endif

#ifdef HAVE_ENCAPSULATION_CSUM
	netdev->hw_enc_features |= dft_fts;
	if (HINIC3_SUPPORT_VXLAN_OFFLOAD(nic_dev->hwdev)) {
		netdev->hw_enc_features |= cso_fts;
#ifdef HAVE_ENCAPSULATION_TSO
		netdev->hw_enc_features |= tso_fts | NETIF_F_TSO_ECN;
#endif /* HAVE_ENCAPSULATION_TSO */
	}
#endif /* HAVE_ENCAPSULATION_CSUM */
}

static void init_intr_coal_param(struct hinic3_nic_dev *nic_dev)
{
	struct hinic3_intr_coal_info *info = NULL;
	u16 i;

	for (i = 0; i < nic_dev->max_qps; i++) {
		info = &nic_dev->intr_coalesce[i];

		info->pending_limt = qp_pending_limit;
		info->coalesce_timer_cfg = qp_coalesc_timer_cfg;

		info->resend_timer_cfg =
			HINIC3_DEAULT_TXRX_MSIX_RESEND_TIMER_CFG;

		info->pkt_rate_high = HINIC3_RX_RATE_HIGH;
		info->rx_usecs_high = HINIC3_RX_COAL_TIME_HIGH;
		info->rx_pending_limt_high = HINIC3_RX_PENDING_LIMIT_HIGH;

		info->pkt_rate_low = HINIC3_RX_RATE_LOW;
		info->rx_usecs_low = HINIC3_RX_COAL_TIME_LOW;
		info->rx_pending_limt_low = HINIC3_RX_PENDING_LIMIT_LOW;
	}
}

static int hinic3_init_intr_coalesce(struct hinic3_nic_dev *nic_dev)
{
	u64 size;

	if (qp_pending_limit != HINIC3_DEAULT_TXRX_MSIX_PENDING_LIMIT ||
	    qp_coalesc_timer_cfg != HINIC3_DEAULT_TXRX_MSIX_COALESC_TIMER_CFG)
		nic_dev->intr_coal_set_flag = 1;
	else
		nic_dev->intr_coal_set_flag = 0;

	size = sizeof(*nic_dev->intr_coalesce) * nic_dev->max_qps;
	if (!size) {
		nic_err(&nic_dev->pdev->dev, "Cannot allocate zero size intr coalesce\n");
		return -EINVAL;
	}
	nic_dev->intr_coalesce = kzalloc(size, GFP_KERNEL);
	if (!nic_dev->intr_coalesce)
		return -ENOMEM;

	init_intr_coal_param(nic_dev);

	if (test_bit(HINIC3_INTR_ADAPT, &nic_dev->flags))
		nic_dev->adaptive_rx_coal = 1;
	else
		nic_dev->adaptive_rx_coal = 0;

	return 0;
}

static void hinic3_free_intr_coalesce(struct hinic3_nic_dev *nic_dev)
{
	kfree(nic_dev->intr_coalesce);
	nic_dev->intr_coalesce = NULL;
}

static int hinic3_alloc_txrxqs(struct hinic3_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	int err;

	err = hinic3_alloc_txqs(netdev);
	if (err) {
		nic_err(&nic_dev->pdev->dev, "Failed to alloc txqs\n");
		return err;
	}

	err = hinic3_alloc_rxqs(netdev);
	if (err) {
		nic_err(&nic_dev->pdev->dev, "Failed to alloc rxqs\n");
		goto alloc_rxqs_err;
	}

	err = hinic3_init_intr_coalesce(nic_dev);
	if (err) {
		nic_err(&nic_dev->pdev->dev, "Failed to init_intr_coalesce\n");
		goto init_intr_err;
	}

	return 0;

init_intr_err:
	hinic3_free_rxqs(netdev);

alloc_rxqs_err:
	hinic3_free_txqs(netdev);

	return err;
}

static void hinic3_free_txrxqs(struct hinic3_nic_dev *nic_dev)
{
	hinic3_free_intr_coalesce(nic_dev);
	hinic3_free_rxqs(nic_dev->netdev);
	hinic3_free_txqs(nic_dev->netdev);
}

static void hinic3_tx_rx_ops_init(struct hinic3_nic_dev *nic_dev)
{
	if (HINIC3_SUPPORT_TX_COMPACT_WQE_OL(nic_dev->hwdev)) {
		nic_dev->tx_rx_ops.tx_set_wqebb_cnt =
				hinic3_tx_set_compact_offload_wqebb_cnt;
		nic_dev->tx_rx_ops.tx_set_wqe_task =
				hinic3_tx_set_compact_offload_wqe_task;
	} else {
		nic_dev->tx_rx_ops.tx_set_wqebb_cnt = hinic3_tx_set_wqebb_cnt;
		nic_dev->tx_rx_ops.tx_set_wqe_task = hinic3_tx_set_wqe_task;
	}

	if (HINIC3_SUPPORT_RX_COMPACT_CQE(nic_dev->hwdev))
		nic_dev->tx_rx_ops.rx_get_cqe_info =
				hinic3_rx_get_compact_cqe_info;
	else
		nic_dev->tx_rx_ops.rx_get_cqe_info = hinic3_rx_get_cqe_info;
}

static void hinic3_sw_deinit(struct hinic3_nic_dev *nic_dev)
{
	hinic3_free_txrxqs(nic_dev);

	hinic3_clean_mac_list_filter(nic_dev);

	hinic3_del_mac(nic_dev->hwdev, nic_dev->netdev->dev_addr, 0,
		       hinic3_global_func_id(nic_dev->hwdev),
		       HINIC3_CHANNEL_NIC);

	hinic3_clear_rss_config(nic_dev);

	hinic3_dcb_deinit(nic_dev);
}

static void hinic3_netdev_mtu_init(struct net_device *netdev)
{
	/* MTU range: 384 - 9600 */
#ifdef HAVE_NETDEVICE_MIN_MAX_MTU
	netdev->min_mtu = HINIC3_MIN_MTU_SIZE;
	netdev->max_mtu = HINIC3_MAX_JUMBO_FRAME_SIZE;
#endif

#ifdef HAVE_NETDEVICE_EXTENDED_MIN_MAX_MTU
	netdev->extended->min_mtu = HINIC3_MIN_MTU_SIZE;
	netdev->extended->max_mtu = HINIC3_MAX_JUMBO_FRAME_SIZE;
#endif
}

static int hinic3_set_default_mac(struct hinic3_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	u8 mac_addr[ETH_ALEN];
	int err = 0;

	err = hinic3_get_default_mac(nic_dev->hwdev, mac_addr);
	if (err) {
		nic_err(&nic_dev->pdev->dev, "Failed to get MAC address\n");
		return err;
	}

	eth_hw_addr_set(netdev, mac_addr);

	if (!is_valid_ether_addr(netdev->dev_addr)) {
		if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev)) {
			nic_err(&nic_dev->pdev->dev, "Invalid MAC address %pM\n",
			netdev->dev_addr);
			return -EIO;
	}

		nic_info(&nic_dev->pdev->dev,
			 "Invalid MAC address %pM, using random\n",
			 netdev->dev_addr);
		eth_hw_addr_random(netdev);
	}

	err = hinic3_set_mac(nic_dev->hwdev, netdev->dev_addr, 0,
			     hinic3_global_func_id(nic_dev->hwdev),
			     HINIC3_CHANNEL_NIC);
	/* When this is VF driver, we must consider that PF has already set VF
	 * MAC, and we can't consider this condition is error status during
	 * driver probe procedure.
	 */
	if (err && err != HINIC3_PF_SET_VF_ALREADY)
		nic_err(&nic_dev->pdev->dev, "Failed to set default MAC\n");

	if (err == HINIC3_PF_SET_VF_ALREADY)
		return 0;

	return err;
}

static void hinic3_outband_cfg_init(struct hinic3_nic_dev *nic_dev)
{
	u16 outband_default_vid = 0;
	int err = 0;

	if (!nic_dev->nic_cap.outband_vlan_cfg_en)
		return;

	err = hinic3_get_outband_vlan_cfg(nic_dev->hwdev, &outband_default_vid);
	if (err) {
		nic_err(&nic_dev->pdev->dev,
			"Failed to get_outband_cfg, err: %d\n", err);
		return;
	}

	nic_dev->outband_cfg.outband_default_vid = outband_default_vid;
}

static int hinic3_sw_init(struct hinic3_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	u64 nic_features;
	int err = 0;

	nic_features = hinic3_get_feature_cap(nic_dev->hwdev);
	/* You can update the features supported by the driver according to the
	 * scenario here
	 */
	nic_features &= NIC_DRV_DEFAULT_FEATURE;
	hinic3_update_nic_feature(nic_dev->hwdev, nic_features);

	err = hinic3_dcb_init(nic_dev);
	if (err) {
		nic_err(&nic_dev->pdev->dev, "Failed to init dcb\n");
		return -EFAULT;
	}

	nic_dev->q_params.sq_depth = HINIC3_SQ_DEPTH;
	nic_dev->q_params.rq_depth = HINIC3_RQ_DEPTH;

	hinic3_try_to_enable_rss(nic_dev);

	err = hinic3_set_default_mac(nic_dev);
	if (err)
		goto set_mac_err;

	hinic3_netdev_mtu_init(netdev);

	err = hinic3_alloc_txrxqs(nic_dev);
	if (err) {
		nic_err(&nic_dev->pdev->dev, "Failed to alloc qps\n");
		goto alloc_qps_err;
	}

	hinic3_outband_cfg_init(nic_dev);

	hinic3_tx_rx_ops_init(nic_dev);

	return 0;

alloc_qps_err:
	hinic3_del_mac(nic_dev->hwdev, netdev->dev_addr, 0,
		       hinic3_global_func_id(nic_dev->hwdev),
		       HINIC3_CHANNEL_NIC);

set_mac_err:
	hinic3_clear_rss_config(nic_dev);

	return err;
}

static void hinic3_assign_netdev_ops(struct hinic3_nic_dev *adapter)
{
	hinic3_set_netdev_ops(adapter);
	if (!HINIC3_FUNC_IS_VF(adapter->hwdev))
		hinic3_set_ethtool_ops(adapter->netdev);
	else
		hinic3vf_set_ethtool_ops(adapter->netdev);

	adapter->netdev->watchdog_timeo = WATCHDOG_TIMEOUT * HZ;
}

static int hinic3_validate_parameters(struct hinic3_lld_dev *lld_dev)
{
	struct pci_dev *pdev = lld_dev->pdev;

	/* If weight exceeds the queue depth, the queue resources will be
	 * exhausted, and increasing it has no effect.
	 */
	if (!poll_weight || poll_weight > HINIC3_MAX_RX_QUEUE_DEPTH) {
		nic_warn(&pdev->dev, "Module Parameter poll_weight is out of range: [1, %d], resetting to %d\n",
			 HINIC3_MAX_RX_QUEUE_DEPTH, DEFAULT_POLL_WEIGHT);
		poll_weight = DEFAULT_POLL_WEIGHT;
	}

	/* check hinic3_rx_buff value, default hinic3_rx_buff is 2KB.
	 * Valid hinic3_rx_buff include 2KB/4KB/8KB/16KB.
	 */
	if (hinic3_rx_buff != RX_BUFF_VALID_2KB &&
	    hinic3_rx_buff != RX_BUFF_VALID_4KB &&
	    hinic3_rx_buff != RX_BUFF_VALID_8KB &&
	    hinic3_rx_buff != RX_BUFF_VALID_16KB) {
		nic_warn(&pdev->dev, "Module Parameter hinic3_rx_buff value %u is out of range, must be 2^n. Valid range is 2 - 16, resetting to %dKB",
			 hinic3_rx_buff, DEFAULT_RX_BUFF_LEN);
			 hinic3_rx_buff = DEFAULT_RX_BUFF_LEN;
	}

	return 0;
}

static void decide_intr_cfg(struct hinic3_nic_dev *nic_dev)
{
	set_bit(HINIC3_INTR_ADAPT, &nic_dev->flags);
}

static void adaptive_configuration_init(struct hinic3_nic_dev *nic_dev)
{
	decide_intr_cfg(nic_dev);
}

static int set_interrupt_moder(struct hinic3_nic_dev *nic_dev, u16 q_id,
			       u8 coalesc_timer_cfg, u8 pending_limt)
{
	struct interrupt_info info;
	int err;

	memset(&info, 0, sizeof(info));

	if (coalesc_timer_cfg == nic_dev->rxqs[q_id].last_coalesc_timer_cfg &&
	    pending_limt == nic_dev->rxqs[q_id].last_pending_limt)
		return 0;

	/* netdev not running or qp not in using,
	 * don't need to set coalesce to hw
	 */
	if (!HINIC3_CHANNEL_RES_VALID(nic_dev) ||
	    q_id >= nic_dev->q_params.num_qps)
		return 0;

	info.lli_set = 0;
	info.interrupt_coalesc_set = 1;
	info.coalesc_timer_cfg = coalesc_timer_cfg;
	info.pending_limt = pending_limt;
	info.msix_index = nic_dev->q_params.irq_cfg[q_id].msix_entry_idx;
	info.resend_timer_cfg =
		nic_dev->intr_coalesce[q_id].resend_timer_cfg;

	err = hinic3_set_interrupt_cfg(nic_dev->hwdev, info,
				       HINIC3_CHANNEL_NIC);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to modify moderation for Queue: %u\n", q_id);
	} else {
		nic_dev->rxqs[q_id].last_coalesc_timer_cfg = coalesc_timer_cfg;
		nic_dev->rxqs[q_id].last_pending_limt = pending_limt;
	}

	return err;
}

static void calc_coal_para(struct hinic3_nic_dev *nic_dev,
			   struct hinic3_intr_coal_info *q_coal, u64 rx_rate,
			   u8 *coalesc_timer_cfg, u8 *pending_limt)
{
	if (rx_rate < q_coal->pkt_rate_low) {
		*coalesc_timer_cfg = q_coal->rx_usecs_low;
		*pending_limt = q_coal->rx_pending_limt_low;
	} else if (rx_rate > q_coal->pkt_rate_high) {
		*coalesc_timer_cfg = q_coal->rx_usecs_high;
		*pending_limt = q_coal->rx_pending_limt_high;
	} else {
		*coalesc_timer_cfg =
			(u8)((rx_rate - q_coal->pkt_rate_low) *
			     (q_coal->rx_usecs_high - q_coal->rx_usecs_low) /
			     (q_coal->pkt_rate_high - q_coal->pkt_rate_low) +
			     q_coal->rx_usecs_low);

		*pending_limt =
			(u8)((rx_rate - q_coal->pkt_rate_low) *
			     (q_coal->rx_pending_limt_high -
			     q_coal->rx_pending_limt_low) /
			     (q_coal->pkt_rate_high - q_coal->pkt_rate_low) +
			     q_coal->rx_pending_limt_low);
	}
}

static void update_queue_coal(struct hinic3_nic_dev *nic_dev, u16 qid,
			      u64 rx_rate, u64 avg_pkt_size, u64 tx_rate)
{
	struct hinic3_intr_coal_info *q_coal = NULL;
	u8 coalesc_timer_cfg, pending_limt;

	q_coal = &nic_dev->intr_coalesce[qid];

	if (rx_rate > HINIC3_RX_RATE_THRESH &&
	    avg_pkt_size > HINIC3_AVG_PKT_SMALL) {
		calc_coal_para(nic_dev, q_coal, rx_rate,
			       &coalesc_timer_cfg, &pending_limt);
	} else {
		coalesc_timer_cfg = HINIC3_LOWEST_LATENCY;
		pending_limt = q_coal->rx_pending_limt_low;
	}

	set_interrupt_moder(nic_dev, qid, coalesc_timer_cfg, pending_limt);
}

void hinic3_auto_moderation_work(struct work_struct *work)
{
	struct delayed_work *delay = to_delayed_work(work);
	struct hinic3_nic_dev *nic_dev = container_of(delay,
						      struct hinic3_nic_dev,
						      moderation_task);
	unsigned long period = (unsigned long)(jiffies -
		nic_dev->last_moder_jiffies);
	u64 rx_packets, rx_bytes, rx_pkt_diff, rx_rate, avg_pkt_size;
	u64 tx_packets, tx_bytes, tx_pkt_diff, tx_rate;
	u16 qid;

	if (!test_bit(HINIC3_INTF_UP, &nic_dev->flags))
		return;

	queue_delayed_work(nic_dev->workq, &nic_dev->moderation_task,
			   HINIC3_MODERATONE_DELAY);

	if (!nic_dev->adaptive_rx_coal || !period)
		return;

	for (qid = 0; qid < nic_dev->q_params.num_qps; qid++) {
		rx_packets = nic_dev->rxqs[qid].rxq_stats.packets;
		rx_bytes = nic_dev->rxqs[qid].rxq_stats.bytes;
		tx_packets = nic_dev->txqs[qid].txq_stats.packets;
		tx_bytes = nic_dev->txqs[qid].txq_stats.bytes;

		rx_pkt_diff =
			rx_packets - nic_dev->rxqs[qid].last_moder_packets;
		avg_pkt_size = rx_pkt_diff ?
			((unsigned long)(rx_bytes -
			 nic_dev->rxqs[qid].last_moder_bytes)) /
			 rx_pkt_diff : 0;

		rx_rate = rx_pkt_diff * HZ / period;
		tx_pkt_diff =
			tx_packets - nic_dev->txqs[qid].last_moder_packets;
		tx_rate = tx_pkt_diff * HZ / period;

		update_queue_coal(nic_dev, qid, rx_rate, avg_pkt_size,
				  tx_rate);

		nic_dev->rxqs[qid].last_moder_packets = rx_packets;
		nic_dev->rxqs[qid].last_moder_bytes = rx_bytes;
		nic_dev->txqs[qid].last_moder_packets = tx_packets;
		nic_dev->txqs[qid].last_moder_bytes = tx_bytes;
	}

	nic_dev->last_moder_jiffies = jiffies;
}

static void hinic3_periodic_work_handler(struct work_struct *work)
{
	struct delayed_work *delay = to_delayed_work(work);
	struct hinic3_nic_dev *nic_dev =
		container_of(delay, struct hinic3_nic_dev, periodic_work);

	if (test_and_clear_bit(EVENT_WORK_TX_TIMEOUT, &nic_dev->event_flag))
		hinic3_fault_event_report(nic_dev->hwdev,
					  HINIC3_FAULT_SRC_TX_TIMEOUT,
					  FAULT_LEVEL_SERIOUS_FLR);

	queue_delayed_work(nic_dev->workq, &nic_dev->periodic_work, HZ);
}

static void hinic3_vport_stats_work_handler(struct work_struct *work)
{
	int err;
	struct hinic3_vport_stats vport_stats = {0};
	struct delayed_work *delay = to_delayed_work(work);
	struct hinic3_nic_dev *nic_dev = container_of(delay,
						      struct hinic3_nic_dev,
						      vport_stats_work);
	err = hinic3_get_vport_stats(nic_dev->hwdev,
				     hinic3_global_func_id(nic_dev->hwdev),
				     &vport_stats);
	if (err)
		nic_err(&nic_dev->pdev->dev, "Failed to get dropped stats from fw\n");
	else
		nic_dev->vport_stats.rx_discard_vport =
					vport_stats.rx_discard_vport;
	queue_delayed_work(nic_dev->workq, &nic_dev->vport_stats_work, HZ);
}

static void free_nic_dev_vram(struct hinic3_nic_dev *nic_dev)
{
	int is_use_vram = get_use_vram_flag();

	if (is_use_vram != 0)
		hi_vram_kfree((void *)nic_dev->nic_vram, nic_dev->nic_vram_name,
			      sizeof(struct hinic3_vram));
	else
		kfree(nic_dev->nic_vram);
	nic_dev->nic_vram = NULL;
}

static void free_nic_dev(struct hinic3_nic_dev *nic_dev)
{
	hinic3_deinit_nic_prof_adapter(nic_dev);
	destroy_workqueue(nic_dev->workq);
	kfree(nic_dev->vlan_bitmap);
	nic_dev->vlan_bitmap = NULL;
	free_nic_dev_vram(nic_dev);
}

static int setup_nic_dev(struct net_device *netdev,
			 struct hinic3_lld_dev *lld_dev)
{
	struct pci_dev *pdev = lld_dev->pdev;
	struct hinic3_nic_dev *nic_dev;
	char *netdev_name_fmt;
	u32 page_num;
	u16 func_id;
	int ret;
	int is_in_kexec = vram_get_kexec_flag();
	int is_use_vram = get_use_vram_flag();

	nic_dev = (struct hinic3_nic_dev *)netdev_priv(netdev);
	nic_dev->netdev = netdev;
	SET_NETDEV_DEV(netdev, &pdev->dev);
	nic_dev->lld_dev = lld_dev;
	nic_dev->hwdev = lld_dev->hwdev;
	nic_dev->pdev = pdev;
	nic_dev->poll_weight = (int)poll_weight;
	nic_dev->msg_enable = DEFAULT_MSG_ENABLE;
	nic_dev->lro_replenish_thld = lro_replenish_thld;
	nic_dev->rx_buff_len = (u16)(hinic3_rx_buff * CONVERT_UNIT);
	nic_dev->dma_rx_buff_size = RX_BUFF_NUM_PER_PAGE * nic_dev->rx_buff_len;
	page_num = nic_dev->dma_rx_buff_size / PAGE_SIZE;
	nic_dev->page_order = page_num > 0 ? ilog2(page_num) : 0;
	nic_dev->page_pool_enabled = page_pool_enabled;
	nic_dev->outband_cfg.outband_default_vid = 0;

	// value other than 0 indicates hot replace
	if (is_use_vram != 0) {
		func_id = hinic3_global_func_id(nic_dev->hwdev);
		snprintf(nic_dev->nic_vram_name, VRAM_NAME_MAX_LEN,
			 "%s%hu", VRAM_NIC_VRAM, func_id);

		nic_dev->nic_vram = (struct hinic3_vram *)hi_vram_kalloc(nic_dev->nic_vram_name,
								sizeof(struct hinic3_vram));
		if (!nic_dev->nic_vram) {
			nic_err(&pdev->dev, "Failed to allocate nic vram\n");
			return -ENOMEM;
		}

		if (is_in_kexec == 0)
			nic_dev->nic_vram->vram_mtu = netdev->mtu;
		else
			netdev->mtu = nic_dev->nic_vram->vram_mtu;
	} else {
		nic_dev->nic_vram = kzalloc(sizeof(*nic_dev->nic_vram),
					    GFP_KERNEL);
		if (!nic_dev->nic_vram)
			return -ENOMEM;

		nic_dev->nic_vram->vram_mtu = netdev->mtu;
	}

	mutex_init(&nic_dev->nic_mutex);

	nic_dev->vlan_bitmap = kzalloc(VLAN_BITMAP_SIZE(nic_dev), GFP_KERNEL);
	if (!nic_dev->vlan_bitmap) {
		nic_err(&pdev->dev, "Failed to allocate vlan bitmap\n");
		ret = -ENOMEM;
		goto vlan_bitmap_error;
	}

	nic_dev->workq = create_singlethread_workqueue(HINIC3_NIC_DEV_WQ_NAME);
	if (!nic_dev->workq) {
		nic_err(&pdev->dev, "Failed to initialize nic workqueue\n");
		ret = -ENOMEM;
		goto create_workq_error;
	}

	INIT_DELAYED_WORK(&nic_dev->periodic_work,
			  hinic3_periodic_work_handler);
	INIT_DELAYED_WORK(&nic_dev->rxq_check_work,
			  hinic3_rxq_check_work_handler);
	if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev))
		INIT_DELAYED_WORK(&nic_dev->vport_stats_work,
				  hinic3_vport_stats_work_handler);

	INIT_LIST_HEAD(&nic_dev->uc_filter_list);
	INIT_LIST_HEAD(&nic_dev->mc_filter_list);
	INIT_WORK(&nic_dev->rx_mode_work, hinic3_set_rx_mode_work);

	INIT_LIST_HEAD(&nic_dev->rx_flow_rule.rules);
	INIT_LIST_HEAD(&nic_dev->tcam.tcam_list);
	INIT_LIST_HEAD(&nic_dev->tcam.tcam_dynamic_info.tcam_dynamic_list);

	hinic3_init_nic_prof_adapter(nic_dev);

	netdev_name_fmt = hinic3_get_dft_netdev_name_fmt(nic_dev);
	if (netdev_name_fmt) {
		ret = strscpy(netdev->name, netdev_name_fmt, IFNAMSIZ);
		if (ret < 0)
			goto get_netdev_name_error;
	}

	return 0;

get_netdev_name_error:
	hinic3_deinit_nic_prof_adapter(nic_dev);
	destroy_workqueue(nic_dev->workq);
create_workq_error:
	kfree(nic_dev->vlan_bitmap);
	nic_dev->vlan_bitmap = NULL;
vlan_bitmap_error:
	free_nic_dev_vram(nic_dev);
	return ret;
}

static int hinic3_set_default_hw_feature(struct hinic3_nic_dev *nic_dev)
{
	int err;

	if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev)) {
		hinic3_dcb_reset_hw_config(nic_dev);

		if (set_link_status_follow < HINIC3_LINK_FOLLOW_STATUS_MAX) {
			err = hinic3_set_link_status_follow(nic_dev->hwdev,
							    set_link_status_follow);
			if (err == HINIC3_MGMT_CMD_UNSUPPORTED)
				nic_warn(&nic_dev->pdev->dev,
					 "Current version of firmware doesn't support to set link status follow port status\n");
		}
	}

	err = hinic3_set_nic_feature_to_hw(nic_dev->hwdev);
	if (err) {
		nic_err(&nic_dev->pdev->dev, "Failed to set nic features\n");
		return err;
	}

	/* enable all hw features in netdev->features */
	err = hinic3_set_hw_features(nic_dev);
	if (err) {
		hinic3_update_nic_feature(nic_dev->hwdev, 0);
		hinic3_set_nic_feature_to_hw(nic_dev->hwdev);
		return err;
	}

	if (HINIC3_SUPPORT_RXQ_RECOVERY(nic_dev->hwdev))
		set_bit(HINIC3_RXQ_RECOVERY, &nic_dev->flags);

	return 0;
}

static void hinic3_bond_init(struct hinic3_nic_dev *nic_dev)
{
	u32 bond_id = HINIC3_INVALID_BOND_ID;
	int err = hinic3_create_bond(nic_dev->hwdev, &bond_id);

	if (err != 0)
		goto bond_init_failed;

	/* bond id does not change, means this pf is not bond active pf, no log is generated */
	if (bond_id == HINIC3_INVALID_BOND_ID)
		return;

	err = hinic3_open_close_bond(nic_dev->hwdev, true);
	if (err != 0) {
		hinic3_delete_bond(nic_dev->hwdev);
		goto bond_init_failed;
	}

	nic_info(&nic_dev->pdev->dev, "Bond %d init success\n", bond_id);
	return;

bond_init_failed:
	nic_err(&nic_dev->pdev->dev, "Bond init failed\n");
}

static int nic_probe(struct hinic3_lld_dev *lld_dev, void **uld_dev,
		     char *uld_dev_name)
{
	struct pci_dev *pdev = lld_dev->pdev;
	struct hinic3_nic_dev *nic_dev = NULL;
	struct net_device *netdev = NULL;
	u16 max_qps, glb_func_id;
	int err;

	if (!hinic3_support_nic(lld_dev->hwdev, NULL)) {
		nic_info(&pdev->dev, "Hw don't support nic\n");
		return 0;
	}

	nic_info(&pdev->dev, "NIC service probe begin\n");

	err = hinic3_validate_parameters(lld_dev);
	if (err) {
		err = -EINVAL;
		goto err_out;
	}

	glb_func_id = hinic3_global_func_id(lld_dev->hwdev);
	err = hinic3_func_reset(lld_dev->hwdev, glb_func_id, HINIC3_NIC_RES,
				HINIC3_CHANNEL_NIC);
	if (err) {
		nic_err(&pdev->dev, "Failed to reset function\n");
		goto err_out;
	}

	err = hinic3_get_dev_cap(lld_dev->hwdev);
	if (err != 0) {
		nic_err(&pdev->dev, "Failed to get dev cap\n");
		goto err_out;
	}

	max_qps = hinic3_func_max_nic_qnum(lld_dev->hwdev);
	netdev = alloc_etherdev_mq(sizeof(*nic_dev), max_qps);
	if (!netdev) {
		nic_err(&pdev->dev, "Failed to allocate ETH device\n");
		err = -ENOMEM;
		goto err_out;
	}

	nic_dev = (struct hinic3_nic_dev *)netdev_priv(netdev);
	err = setup_nic_dev(netdev, lld_dev);
	if (err)
		goto setup_dev_err;

	adaptive_configuration_init(nic_dev);

	/* get nic cap from hw */
	hinic3_support_nic(lld_dev->hwdev, &nic_dev->nic_cap);

	err = hinic3_init_nic_hwdev(nic_dev->hwdev, pdev, &pdev->dev,
				    nic_dev->rx_buff_len);
	if (err) {
		nic_err(&pdev->dev, "Failed to init nic hwdev\n");
		goto init_nic_hwdev_err;
	}

	err = hinic3_sw_init(nic_dev);
	if (err)
		goto sw_init_err;

	hinic3_assign_netdev_ops(nic_dev);
	netdev_feature_init(netdev);

#ifdef HAVE_UDP_TUNNEL_NIC_INFO
	netdev->udp_tunnel_nic_info = &hinic3_udp_tunnels;
#endif /* HAVE_UDP_TUNNEL_NIC_INFO */

	err = hinic3_set_default_hw_feature(nic_dev);
	if (err)
		goto set_features_err;

	if (hinic3_get_bond_create_mode(lld_dev->hwdev) != 0)
		hinic3_bond_init(nic_dev);

#ifdef HAVE_MULTI_VLAN_OFFLOAD_EN
	hinic3_register_notifier(nic_dev);
#endif

	if (pdev->subsystem_device != BIFUR_RESOURCE_PF_SSID) {
		err = register_netdev(netdev);
		if (err) {
			nic_err(&pdev->dev, "Failed to register netdev\n");
			err = -ENOMEM;
			goto netdev_err;
		}
	}

	queue_delayed_work(nic_dev->workq, &nic_dev->periodic_work, HZ);
	if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev))
		queue_delayed_work(nic_dev->workq,
				   &nic_dev->vport_stats_work, HZ);
	netif_carrier_off(netdev);

	*uld_dev = nic_dev;
	nicif_info(nic_dev, probe, netdev, "Register netdev succeed\n");
	nic_info(&pdev->dev, "NIC service probed\n");

	return 0;

netdev_err:
#ifdef HAVE_MULTI_VLAN_OFFLOAD_EN
	hinic3_unregister_notifier(nic_dev);
#endif
	hinic3_update_nic_feature(nic_dev->hwdev, 0);
	hinic3_set_nic_feature_to_hw(nic_dev->hwdev);

set_features_err:
	hinic3_sw_deinit(nic_dev);

sw_init_err:
	hinic3_free_nic_hwdev(nic_dev->hwdev);

init_nic_hwdev_err:
	free_nic_dev(nic_dev);
setup_dev_err:
	free_netdev(netdev);

err_out:
	nic_err(&pdev->dev, "NIC service probe failed\n");

	return err;
}

static void hinic3_bond_deinit(struct hinic3_nic_dev *nic_dev)
{
	int ret = 0;

	ret = hinic3_open_close_bond(nic_dev->hwdev, false);
	if (ret != 0)
		goto bond_deinit_failed;

	ret = hinic3_delete_bond(nic_dev->hwdev);
	if (ret != 0)
		goto bond_deinit_failed;

	nic_info(&nic_dev->pdev->dev, "Bond deinit success\n");
	return;

bond_deinit_failed:
	nic_err(&nic_dev->pdev->dev, "Bond deinit failed\n");
}

static void nic_remove(struct hinic3_lld_dev *lld_dev, void *adapter)
{
	struct hinic3_nic_dev *nic_dev = adapter;
	struct net_device *netdev = NULL;

	if (!nic_dev || !hinic3_support_nic(lld_dev->hwdev, NULL))
		return;

	nic_info(&lld_dev->pdev->dev, "NIC service remove begin\n");

	netdev = nic_dev->netdev;

	if (lld_dev->pdev->subsystem_device != BIFUR_RESOURCE_PF_SSID)
		unregister_netdev(netdev);

#ifdef HAVE_MULTI_VLAN_OFFLOAD_EN
	hinic3_unregister_notifier(nic_dev);
#endif

	if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev))
		cancel_delayed_work_sync(&nic_dev->vport_stats_work);
	cancel_delayed_work_sync(&nic_dev->periodic_work);
	cancel_delayed_work_sync(&nic_dev->rxq_check_work);
	cancel_work_sync(&nic_dev->rx_mode_work);
	destroy_workqueue(nic_dev->workq);

	hinic3_flush_rx_flow_rule(nic_dev);

	if (hinic3_get_bond_create_mode(lld_dev->hwdev) != 0)
		hinic3_bond_deinit(nic_dev);

	hinic3_update_nic_feature(nic_dev->hwdev, 0);
	hinic3_set_nic_feature_to_hw(nic_dev->hwdev);

	hinic3_sw_deinit(nic_dev);

	hinic3_free_nic_hwdev(nic_dev->hwdev);

	hinic3_deinit_nic_prof_adapter(nic_dev);
	kfree(nic_dev->vlan_bitmap);
	nic_dev->vlan_bitmap = NULL;

	free_netdev(netdev);

	nic_info(&lld_dev->pdev->dev, "NIC service removed\n");
}

static void sriov_state_change(struct hinic3_nic_dev *nic_dev,
			       const struct hinic3_sriov_state_info *info)
{
	if (!info->enable)
		hinic3_clear_vfs_info(nic_dev->hwdev);
}

static void hinic3_port_module_event_handler(struct hinic3_nic_dev *nic_dev,
					     struct hinic3_event_info *event)
{
	const char *g_hinic3_module_link_err[LINK_ERR_NUM] = {
						"Unrecognized module" };
	struct hinic3_port_module_event *module_event =
						(void *)event->event_data;
	enum port_module_event_type type = module_event->type;
	enum link_err_type err_type = module_event->err_type;

	switch (type) {
	case HINIC3_PORT_MODULE_CABLE_PLUGGED:
	case HINIC3_PORT_MODULE_CABLE_UNPLUGGED:
		nicif_info(nic_dev, link, nic_dev->netdev,
			   "Port module event: Cable %s\n",
			   type == HINIC3_PORT_MODULE_CABLE_PLUGGED ?
			   "plugged" : "unplugged");
		break;
	case HINIC3_PORT_MODULE_LINK_ERR:
		if (err_type >= LINK_ERR_NUM) {
			nicif_info(nic_dev, link, nic_dev->netdev,
				   "Link failed, Unknown error type: 0x%x\n",
				   err_type);
		} else {
			nicif_info(nic_dev, link, nic_dev->netdev,
				   "Link failed, error type: 0x%x: %s\n",
				   err_type,
				   g_hinic3_module_link_err[err_type]);
		}
		break;
	default:
		nicif_err(nic_dev, link, nic_dev->netdev,
			  "Unknown port module type %d\n", type);
		break;
	}
}

bool hinic3_need_proc_link_event(struct hinic3_lld_dev *lld_dev)
{
	int ret = 0;
	u16 func_id;
	u8 roce_enable = false;
	bool is_slave_func = false;
	struct hinic3_hw_bond_infos hw_bond_infos = {0};

	if (!lld_dev)
		return false;

	/* 非slave设备需要处理link down事件 */
	ret = hinic3_is_slave_func(lld_dev->hwdev, &is_slave_func);
	if (ret != 0) {
		nic_err(&lld_dev->pdev->dev, "NIC get info, lld_dev is null\n");
		return true;
	}

	if (!is_slave_func)
		return true;

	/* 未使能了vroce功能，需处理link down事件 */
	func_id = hinic3_global_func_id(lld_dev->hwdev);
	ret = hinic3_get_func_vroce_enable(lld_dev->hwdev, func_id, &roce_enable);
	if (ret != 0)
		return true;

	if (!roce_enable)
		return true;

	/* 未创建bond,需要处理link down事件 */
	hw_bond_infos.bond_id = HINIC_OVS_BOND_DEFAULT_ID;

	ret = hinic3_get_hw_bond_infos(lld_dev->hwdev, &hw_bond_infos, HINIC3_CHANNEL_COMM);
	if (ret != 0) {
		pr_err("[ROCE, ERR] Get chipf bond info failed (%d)\n", ret);
		return true;
	}

	if (!hw_bond_infos.valid)
		return true;

	return false;
}

bool hinic3_need_proc_bond_event(struct hinic3_lld_dev *lld_dev)
{
	return !hinic3_need_proc_link_event(lld_dev);
}

static void hinic_porc_bond_state_change(struct hinic3_lld_dev *lld_dev, void *adapter,
					 struct hinic3_event_info *event)
{
	struct hinic3_nic_dev *nic_dev = adapter;

	if (!nic_dev || !event || !hinic3_support_nic(lld_dev->hwdev, NULL))
		return;

	switch (HINIC3_SRV_EVENT_TYPE(event->service, event->type)) {
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_NIC, EVENT_NIC_BOND_DOWN):
		if (!hinic3_need_proc_bond_event(lld_dev)) {
			nic_info(&lld_dev->pdev->dev, "NIC don't need proc bond event\n");
			return;
		}
		nic_info(&lld_dev->pdev->dev, "NIC proc bond down\n");
		hinic3_link_status_change(nic_dev, false);
		break;
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_NIC, EVENT_NIC_BOND_UP):
		if (!hinic3_need_proc_bond_event(lld_dev)) {
			nic_info(&lld_dev->pdev->dev, "NIC don't need proc bond event\n");
			return;
		}
		nic_info(&lld_dev->pdev->dev, "NIC proc bond up\n");
		hinic3_link_status_change(nic_dev, true);
		break;
	default:
		break;
	}
}

static void hinic3_outband_cfg_event_handler(struct hinic3_nic_dev *nic_dev,
					     struct hinic3_outband_cfg_info *info)
{
	int err = 0;

	if (!nic_dev || !info || !hinic3_support_nic(nic_dev->hwdev, NULL)) {
		pr_err("Outband cfg event invalid param\n");
		return;
	}

	if (hinic3_func_type(nic_dev->hwdev) != TYPE_VF &&
	    info->func_id >= NIC_MAX_PF_NUM) {
		err = hinic3_notify_vf_outband_cfg(nic_dev->hwdev,
						   info->func_id,
						   info->outband_default_vid);
		if (err) {
			nic_err(&nic_dev->pdev->dev,
				"Outband cfg event notify vf err: %d, func_id: 0x%x, vid: 0x%x\n",
				err, info->func_id, info->outband_default_vid);
			return;
		}
	}

		nic_info(&nic_dev->pdev->dev, "Change outband default vid from %u to %u\n",
			 nic_dev->outband_cfg.outband_default_vid,
			 info->outband_default_vid);

	nic_dev->outband_cfg.outband_default_vid = info->outband_default_vid;
}

static void nic_event(struct hinic3_lld_dev *lld_dev, void *adapter,
		      struct hinic3_event_info *event)
{
	struct hinic3_nic_dev *nic_dev = adapter;
	struct hinic3_fault_event *fault = NULL;

	if (!nic_dev || !event || !hinic3_support_nic(lld_dev->hwdev, NULL))
		return;

	switch (HINIC3_SRV_EVENT_TYPE(event->service, event->type)) {
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_NIC, EVENT_NIC_LINK_DOWN):
		if (!hinic3_need_proc_link_event(lld_dev)) {
			nic_info(&lld_dev->pdev->dev, "NIC don't need proc link event\n");
			return;
		}
		hinic3_link_status_change(nic_dev, false);
		break;
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_NIC, EVENT_NIC_LINK_UP):
		hinic3_link_status_change(nic_dev, true);
		break;
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_NIC, EVENT_NIC_BOND_DOWN):
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_NIC, EVENT_NIC_BOND_UP):
		hinic_porc_bond_state_change(lld_dev, adapter, event);
		break;
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_NIC, EVENT_NIC_PORT_MODULE_EVENT):
		hinic3_port_module_event_handler(nic_dev, event);
		break;
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_NIC, EVENT_NIC_OUTBAND_CFG):
		hinic3_outband_cfg_event_handler(nic_dev,
						 (void *)event->event_data);
		break;
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_COMM,
					EVENT_COMM_SRIOV_STATE_CHANGE):
		sriov_state_change(nic_dev, (void *)event->event_data);
		break;
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_COMM, EVENT_COMM_FAULT):
		fault = (void *)event->event_data;
		if (fault->fault_level == FAULT_LEVEL_SERIOUS_FLR &&
		    fault->event.chip.func_id ==
		    hinic3_global_func_id(lld_dev->hwdev))
			hinic3_link_status_change(nic_dev, false);
		break;
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_COMM, EVENT_COMM_PCIE_LINK_DOWN):
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_COMM, EVENT_COMM_HEART_LOST):
	case HINIC3_SRV_EVENT_TYPE(EVENT_SRV_COMM, EVENT_COMM_MGMT_WATCHDOG):
		hinic3_link_status_change(nic_dev, false);
		break;
	default:
		break;
	}
}

struct net_device *hinic3_get_netdev_by_lld(struct hinic3_lld_dev *lld_dev)
{
	struct hinic3_nic_dev *nic_dev = NULL;

	if (!lld_dev || !hinic3_support_nic(lld_dev->hwdev, NULL))
		return NULL;

	nic_dev = hinic3_get_uld_dev_unsafe(lld_dev, SERVICE_T_NIC);
	if (!nic_dev) {
		nic_err(&lld_dev->pdev->dev,
			"There's no net device attached on the pci device");
		return NULL;
	}

	return nic_dev->netdev;
}
EXPORT_SYMBOL(hinic3_get_netdev_by_lld);

struct hinic3_lld_dev *hinic3_get_lld_dev_by_netdev(struct net_device *netdev)
{
	struct hinic3_nic_dev *nic_dev = NULL;

	if (!netdev || !hinic3_is_netdev_ops_match(netdev))
		return NULL;

	nic_dev = netdev_priv(netdev);
	if (!nic_dev)
		return NULL;

	return nic_dev->lld_dev;
}
EXPORT_SYMBOL(hinic3_get_lld_dev_by_netdev);

struct hinic3_uld_info g_nic3_uld_info = {
	.probe = nic_probe,
	.remove = nic_remove,
	.suspend = NULL,
	.resume = NULL,
	.event = nic_event,
	.ioctl = hinic3_ioctl,
};

struct hinic3_uld_info *hinic3_get_nic_uld_info(void)
{
	return &g_nic3_uld_info;
}

#define HINIC3_NIC_DRV_DESC "Intelligent Network Interface Card Driver"

static __init int hinic3_nic_lld_init(void)
{
	int err;

	pr_info("%s - version %s\n", HINIC3_NIC_DRV_DESC,
		HINIC3_NIC_DRV_VERSION);

	err = hinic3_lld_init();
	if (err) {
		pr_err("SDK init failed.\n");
		return err;
	}

	err = hinic3_register_uld(SERVICE_T_NIC, &g_nic3_uld_info);
	if (err) {
		pr_err("Register hinic3 uld failed\n");
		hinic3_lld_exit();
		return err;
	}

	err = hinic3_module_pre_init();
	if (err) {
		pr_err("Init custom failed\n");
		hinic3_unregister_uld(SERVICE_T_NIC);
		hinic3_lld_exit();
		return err;
	}

	return 0;
}

static __exit void hinic3_nic_lld_exit(void)
{
	hinic3_unregister_uld(SERVICE_T_NIC);

	hinic3_module_post_exit();

	hinic3_lld_exit();
}

module_init(hinic3_nic_lld_init);
module_exit(hinic3_nic_lld_exit);

MODULE_AUTHOR("Huawei Technologies CO., Ltd");
MODULE_DESCRIPTION(HINIC3_NIC_DRV_DESC);
MODULE_VERSION(HINIC3_NIC_DRV_VERSION);
MODULE_LICENSE("GPL");
