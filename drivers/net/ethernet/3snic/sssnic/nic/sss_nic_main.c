// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

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

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_nic_cfg.h"
#include "sss_nic_vf_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_rss_cfg.h"
#include "sss_nic_io.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_tx.h"
#include "sss_nic_tx_init.h"
#include "sss_nic_rx.h"
#include "sss_nic_rx_init.h"
#include "sss_nic_rx_reset.h"
#include "sss_nic_rss.h"
#include "sss_nic_dcb.h"
#include "sss_nic_ethtool.h"
#include "sss_nic_filter.h"
#include "sss_nic_netdev_ops.h"
#include "sss_nic_netdev_ops_api.h"
#include "sss_nic_ntuple.h"
#include "sss_nic_event.h"
#include "sss_tool_nic_func.h"

#define DEFAULT_POLL_BUDGET	64
static u32 poll_budget = DEFAULT_POLL_BUDGET;
module_param(poll_budget, uint, 0444);
MODULE_PARM_DESC(poll_budget, "Number packets for NAPI budget (default=64)");

#define SSSNIC_DEAULT_TXRX_MSIX_PENDING_LIMIT		2
#define SSSNIC_DEAULT_TXRX_MSIX_COALESC_TIMER_CFG	25
#define SSSNIC_DEAULT_TXRX_MSIX_RESEND_TIMER_CFG	7

static u8 msix_pending_limit = SSSNIC_DEAULT_TXRX_MSIX_PENDING_LIMIT;
module_param(msix_pending_limit, byte, 0444);
MODULE_PARM_DESC(msix_pending_limit, "QP MSI-X Interrupt coalescing parameter pending_limit (default=2)");

static u8 msix_coalesc_timer =
	SSSNIC_DEAULT_TXRX_MSIX_COALESC_TIMER_CFG;
module_param(msix_coalesc_timer, byte, 0444);
MODULE_PARM_DESC(msix_coalesc_timer, "QP MSI-X Interrupt coalescing parameter coalesc_timer_cfg (default=25)");

#define DEFAULT_RX_BUFF_LEN	2
u16 rx_buff_size = DEFAULT_RX_BUFF_LEN;
module_param(rx_buff_size, ushort, 0444);
MODULE_PARM_DESC(rx_buff_size, "Set rx_buff size, buffer len must be 2^n. 2 - 16, default is 2KB");

static u32 rx_poll_wqe = 256;
module_param(rx_poll_wqe, uint, 0444);
MODULE_PARM_DESC(rx_poll_wqe, "Number wqe for rx poll (default=256)");

static u8 link_follow_status = SSSNIC_LINK_FOLLOW_STATUS_MAX;
module_param(link_follow_status, byte, 0444);
MODULE_PARM_DESC(link_follow_status, "Set link follow status port status (0=default,1=follow,2=separate,3=unset");

#define SSSNIC_DEV_WQ_NAME		"sssnic_dev_wq"

#define DEFAULT_MSG_ENABLE		(NETIF_MSG_DRV | NETIF_MSG_LINK)

#define QID_MASKED(qid, nic_dev)	((qid) & ((nic_dev)->qp_num - 1))
#define WATCHDOG_TIMEOUT	5

#define SSSNIC_SQ_DEPTH			1024
#define SSSNIC_RQ_DEPTH			1024

enum sss_nic_rx_buff_len {
	RX_BUFF_VALID_2KB		= 2,
	RX_BUFF_VALID_4KB		= 4,
	RX_BUFF_VALID_8KB		= 8,
	RX_BUFF_VALID_16KB		= 16,
};

#define CONVERT_UNIT			1024
#define RX_BUFF_TO_BYTES(size) ((u16)((size) * CONVERT_UNIT))
#define RX_BUFF_NUM_PER_PAGE 2
#define RX_BUFF_TO_DMA_SIZE(rx_buff_len) (RX_BUFF_NUM_PER_PAGE * (rx_buff_len))
#define DMA_SIZE_TO_PAGE_NUM(buff_size) ((buff_size) / PAGE_SIZE)
#define PAGE_NUM_TO_ORDER(page_num) ((page_num) > 0 ? ilog2(page_num) : 0)
#define BUFF_SIZE_TO_PAGE_ORDER(buff_size) PAGE_NUM_TO_ORDER(DMA_SIZE_TO_PAGE_NUM(buff_size))

#define POLL_BUDGET_IS_VALID(budget) ((budget) <= SSSNIC_MAX_RX_QUEUE_DEPTH)

#define SSSNIC_NETDEV_DEFAULT_FEATURE (NETIF_F_SG | NETIF_F_HIGHDMA)

#define SSSNIC_LP_PKT_LEN		60

#ifdef HAVE_MULTI_VLAN_OFFLOAD_EN

#define SSSNIC_MAX_VLAN_DEPTH_OFFLOAD_SUPPORT	1
#define SSSNIC_VLAN_CLEAR_OFFLOAD	(~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | \
					NETIF_F_SCTP_CRC | NETIF_F_RXCSUM | \
					NETIF_F_ALL_TSO))

#define SSSNIC_DRV_DESC "Intelligent Network Interface Card Driver"

static int sss_nic_netdev_event_handler(struct notifier_block *notifier,
					unsigned long event, void *ptr);
typedef void (*sss_nic_port_module_event_handler_t)(struct sss_nic_dev *nic_dev, void *event_data);

static DEFINE_MUTEX(g_netdev_notifier_mutex);
static int g_netdev_notifier_ref_cnt;

typedef void (*sss_nic_event_handler_t)(struct sss_nic_dev *nic_dev, struct sss_event_info *event);

static struct notifier_block g_netdev_notifier = {
	.notifier_call = sss_nic_netdev_event_handler,
};

static void sss_nic_register_notifier(struct sss_nic_dev *nic_dev)
{
	int ret;

	mutex_lock(&g_netdev_notifier_mutex);
	g_netdev_notifier_ref_cnt++;
	if (g_netdev_notifier_ref_cnt == 1) {
		ret = register_netdevice_notifier(&g_netdev_notifier);
		if (ret != 0) {
			nic_info(nic_dev->dev_hdl,
				 "Fail to register netdevice notifier, ret: %d\n", ret);
			g_netdev_notifier_ref_cnt--;
		}
	}
	mutex_unlock(&g_netdev_notifier_mutex);
}

static void sss_nic_unregister_notifier(struct sss_nic_dev *nic_dev)
{
	mutex_lock(&g_netdev_notifier_mutex);
	if (g_netdev_notifier_ref_cnt == 1)
		unregister_netdevice_notifier(&g_netdev_notifier);

	if (g_netdev_notifier_ref_cnt > 0)
		g_netdev_notifier_ref_cnt--;
	mutex_unlock(&g_netdev_notifier_mutex);
}

#if IS_ENABLED(CONFIG_VLAN_8021Q)
static u16 sss_nic_get_vlan_depth(struct net_device *dev)
{
	u16 vlan_depth = 0;
	struct net_device *vlan_dev = dev;

	do {
		vlan_depth++;
		vlan_dev = vlan_dev_priv(vlan_dev)->real_dev;
	} while (is_vlan_dev(vlan_dev));

	return vlan_depth;
}

static void sss_nic_clear_netdev_vlan_offload(struct net_device *dev, u16 vlan_depth)
{
	if (vlan_depth == SSSNIC_MAX_VLAN_DEPTH_OFFLOAD_SUPPORT) {
		dev->vlan_features &= SSSNIC_VLAN_CLEAR_OFFLOAD;
	} else if (vlan_depth > SSSNIC_MAX_VLAN_DEPTH_OFFLOAD_SUPPORT) {
#ifdef HAVE_NDO_SET_FEATURES
		dev->hw_features &= SSSNIC_VLAN_CLEAR_OFFLOAD;
#endif
		dev->features &= SSSNIC_VLAN_CLEAR_OFFLOAD;
	}
}
#endif

static int sss_nic_netdev_event_handler(struct notifier_block *notifier,
					unsigned long event, void *ptr)
{
#if IS_ENABLED(CONFIG_VLAN_8021Q)
	u16 vlan_depth;
#endif
	struct net_device *real_dev = NULL;
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);

	if (!is_vlan_dev(dev))
		return NOTIFY_DONE;

	if (event != NETDEV_REGISTER)
		return NOTIFY_DONE;

	dev_hold(dev);

	real_dev = vlan_dev_real_dev(dev);
	if (!sss_nic_is_netdev_ops_match(real_dev))
		goto out;

#if IS_ENABLED(CONFIG_VLAN_8021Q)
	vlan_depth = sss_nic_get_vlan_depth(dev);
	sss_nic_clear_netdev_vlan_offload(dev, vlan_depth);
#endif
out:
	dev_put(dev);

	return NOTIFY_DONE;
}
#endif

static netdev_features_t sss_nic_default_cso_feature(struct sss_nic_dev *nic_dev)
{
	netdev_features_t feature = 0;

	if (SSSNIC_SUPPORT_CSUM(nic_dev->nic_io))
		feature |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM;
	if (SSSNIC_SUPPORT_SCTP_CRC(nic_dev->nic_io))
		feature |= NETIF_F_SCTP_CRC;

	return feature;
}

static netdev_features_t sss_nic_default_gso_feature(struct sss_nic_dev *nic_dev)
{
	netdev_features_t feature = 0;

	if (SSSNIC_SUPPORT_TSO(nic_dev->nic_io))
		feature |= NETIF_F_TSO | NETIF_F_TSO6;
#ifdef HAVE_ENCAPSULATION_TSO
	if (SSSNIC_SUPPORT_VXLAN_OFFLOAD(nic_dev->nic_io))
		feature |= NETIF_F_GSO_UDP_TUNNEL | NETIF_F_GSO_UDP_TUNNEL_CSUM;
#endif /* HAVE_ENCAPSULATION_TSO */

	return feature;
}

static netdev_features_t sss_nic_default_vlan_feature(struct sss_nic_dev *nic_dev)
{
	netdev_features_t feature = 0;

	if (SSSNIC_SUPPORT_RXVLAN_FILTER(nic_dev->nic_io)) {
#if defined(NETIF_F_HW_VLAN_CTAG_FILTER)
		feature |= NETIF_F_HW_VLAN_CTAG_FILTER;
#elif defined(NETIF_F_HW_VLAN_FILTER)
		feature |= NETIF_F_HW_VLAN_FILTER;
#endif
	}

	if (SSSNIC_SUPPORT_VLAN_OFFLOAD(nic_dev->nic_io)) {
#if defined(NETIF_F_HW_VLAN_CTAG_TX)
		feature |= NETIF_F_HW_VLAN_CTAG_TX;
#elif defined(NETIF_F_HW_VLAN_TX)
		feature |= NETIF_F_HW_VLAN_TX;
#endif

#if defined(NETIF_F_HW_VLAN_CTAG_RX)
		feature |= NETIF_F_HW_VLAN_CTAG_RX;
#elif defined(NETIF_F_HW_VLAN_RX)
		feature |= NETIF_F_HW_VLAN_RX;
#endif
	}

	return feature;
}

static netdev_features_t sss_nic_default_lro_feature(struct sss_nic_dev *nic_dev)
{
	netdev_features_t feature = 0;

	if (SSSNIC_SUPPORT_LRO(nic_dev->nic_io))
		feature = NETIF_F_LRO;

	return feature;
}

static void sss_nic_init_netdev_hw_feature(struct sss_nic_dev *nic_dev,
					   netdev_features_t lro_feature)
{
	struct net_device *netdev = nic_dev->netdev;
	netdev_features_t hw_features = 0;

	hw_features = netdev->hw_features;

	hw_features |= netdev->features | lro_feature;

	netdev->hw_features = hw_features;
}

static void sss_nic_init_netdev_hw_enc_feature(struct sss_nic_dev *nic_dev,
					       netdev_features_t cso_feature,
					       netdev_features_t gso_feature)
{
	struct net_device *netdev = nic_dev->netdev;

#ifdef HAVE_ENCAPSULATION_CSUM
	netdev->hw_enc_features |= SSSNIC_NETDEV_DEFAULT_FEATURE;
	if (SSSNIC_SUPPORT_VXLAN_OFFLOAD(nic_dev->nic_io)) {
		netdev->hw_enc_features |= cso_feature;
#ifdef HAVE_ENCAPSULATION_TSO
		netdev->hw_enc_features |= gso_feature | NETIF_F_TSO_ECN;
#endif /* HAVE_ENCAPSULATION_TSO */
	}
#endif /* HAVE_ENCAPSULATION_CSUM */
}

static void sss_nic_init_netdev_feature(struct sss_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	netdev_features_t cso_feature = 0;
	netdev_features_t gso_feature = 0;
	netdev_features_t vlan_feature = 0;
	netdev_features_t lro_feature = 0;

	cso_feature = sss_nic_default_cso_feature(nic_dev);
	gso_feature = sss_nic_default_gso_feature(nic_dev);
	vlan_feature = sss_nic_default_vlan_feature(nic_dev);
	lro_feature = sss_nic_default_lro_feature(nic_dev);

	netdev->features |= SSSNIC_NETDEV_DEFAULT_FEATURE |
			    cso_feature | gso_feature | vlan_feature;
	netdev->vlan_features |= SSSNIC_NETDEV_DEFAULT_FEATURE |
				 cso_feature | gso_feature;

	sss_nic_init_netdev_hw_feature(nic_dev, lro_feature);
	sss_nic_init_netdev_hw_enc_feature(nic_dev, cso_feature, gso_feature);

#ifdef IFF_UNICAST_FLT
	netdev->priv_flags |= IFF_UNICAST_FLT;
#endif
}

static void sss_nic_init_intr_coal_param(struct sss_nic_intr_coal_info *intr_coal, u16 max_qp)
{
	u16 i;

	for (i = 0; i < max_qp; i++) {
		intr_coal[i].pkt_rate_low = SSSNIC_RX_RATE_LOW;
		intr_coal[i].pkt_rate_high = SSSNIC_RX_RATE_HIGH;
		intr_coal[i].rx_usecs_low = SSSNIC_RX_COAL_TIME_LOW;
		intr_coal[i].rx_usecs_high = SSSNIC_RX_COAL_TIME_HIGH;
		intr_coal[i].rx_pending_limt_low = SSSNIC_RX_PENDING_LIMIT_LOW;
		intr_coal[i].rx_pending_limt_high = SSSNIC_RX_PENDING_LIMIT_HIGH;
		intr_coal[i].pending_limt = msix_pending_limit;
		intr_coal[i].coalesce_timer = msix_coalesc_timer;
		intr_coal[i].resend_timer = SSSNIC_DEAULT_TXRX_MSIX_RESEND_TIMER_CFG;
	}
}

static int sss_nic_init_intr_coalesce(struct sss_nic_dev *nic_dev)
{
	u64 coalesce_size;

	coalesce_size = sizeof(*nic_dev->coal_info) * nic_dev->max_qp_num;
	nic_dev->coal_info = kzalloc(coalesce_size, GFP_KERNEL);
	if (!nic_dev->coal_info)
		return -ENOMEM;

	sss_nic_init_intr_coal_param(nic_dev->coal_info, nic_dev->max_qp_num);

	if (test_bit(SSSNIC_INTR_ADAPT, &nic_dev->flags))
		nic_dev->use_adaptive_rx_coalesce = 1;
	else
		nic_dev->use_adaptive_rx_coalesce = 0;

	return 0;
}

static void sss_nic_deinit_intr_coalesce(struct sss_nic_dev *nic_dev)
{
	kfree(nic_dev->coal_info);
	nic_dev->coal_info = NULL;
}

static int sss_nic_alloc_lb_test_buf(struct sss_nic_dev *nic_dev)
{
	u8 *loop_test_rx_buf = NULL;

	loop_test_rx_buf = vmalloc(SSSNIC_LP_PKT_CNT * SSSNIC_LP_PKT_LEN);
	if (!loop_test_rx_buf)
		return -ENOMEM;

	nic_dev->loop_test_rx_buf = loop_test_rx_buf;
	nic_dev->loop_pkt_len = SSSNIC_LP_PKT_LEN;

	return 0;
}

static void sss_nic_free_lb_test_buf(struct sss_nic_dev *nic_dev)
{
	vfree(nic_dev->loop_test_rx_buf);
	nic_dev->loop_test_rx_buf = NULL;
}

static void sss_nic_dev_deinit(struct sss_nic_dev *nic_dev)
{
	sss_nic_free_lb_test_buf(nic_dev);

	sss_nic_deinit_intr_coalesce(nic_dev);

	sss_nic_free_rq_desc_group(nic_dev);

	sss_nic_free_sq_desc_group(nic_dev);

	sss_nic_clean_mac_list_filter(nic_dev);

	sss_nic_del_mac(nic_dev, nic_dev->netdev->dev_addr, 0,
			sss_get_global_func_id(nic_dev->hwdev), SSS_CHANNEL_NIC);

	sss_nic_free_rss_key(nic_dev);
	if (test_bit(SSSNIC_DCB_ENABLE, &nic_dev->flags))
		sss_nic_set_hw_dcb_state(nic_dev,
					 SSSNIC_MBX_OPCODE_SET_DCB_STATE, SSSNIC_DCB_STATE_DISABLE);
}

static int sss_nic_init_mac_addr(struct sss_nic_dev *nic_dev)
{
	int ret;
	struct net_device *netdev = nic_dev->netdev;

	ret = sss_nic_get_default_mac(nic_dev, (u8 *)(netdev->dev_addr));
	if (ret != 0) {
		nic_err(nic_dev->dev_hdl, "Fail to get MAC address\n");
		return ret;
	}

	if (!is_valid_ether_addr(netdev->dev_addr)) {
		nic_info(nic_dev->dev_hdl,
			 "Invalid default mac address %pM\n", netdev->dev_addr);
		if (!SSSNIC_FUNC_IS_VF(nic_dev->hwdev)) {
			nic_err(nic_dev->dev_hdl, "Invalid default MAC address\n");
			return -EIO;
		}

		eth_hw_addr_random(netdev);
		nic_info(nic_dev->dev_hdl,
			 "Use random mac address %pM\n", netdev->dev_addr);
	}

	ret = sss_nic_set_mac(nic_dev, netdev->dev_addr, 0,
			      sss_get_global_func_id(nic_dev->hwdev), SSS_CHANNEL_NIC);
	if (ret != 0 && ret != SSSNIC_PF_SET_VF_ALREADY) {
		/* If it is a VF device, it is possible that the MAC address has been set by PF,
		 * and this situation is legal.
		 */
		nic_err(nic_dev->dev_hdl, "Fail to set default MAC\n");
		return ret;
	}

	return 0;
}

static void sss_nic_set_mtu_range(struct net_device *netdev)
{
	/* MTU range: 384 - 9600 */
#ifdef HAVE_NETDEVICE_MIN_MAX_MTU
	netdev->min_mtu = SSSNIC_MIN_MTU_SIZE;
	netdev->max_mtu = SSSNIC_MAX_JUMBO_FRAME_SIZE;
#endif

#ifdef HAVE_NETDEVICE_EXTENDED_MIN_MAX_MTU
	netdev->extended->min_mtu = SSSNIC_MIN_MTU_SIZE;
	netdev->extended->max_mtu = SSSNIC_MAX_JUMBO_FRAME_SIZE;
#endif
}

static int sss_nic_dev_init(struct sss_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	int ret = 0;

	/* get nic cap from hw */
	sss_get_nic_capability(nic_dev->hwdev, &nic_dev->nic_svc_cap);

	ret = sss_nic_dcb_init(nic_dev);
	if (ret != 0) {
		nic_err(nic_dev->dev_hdl, "Fail to init dcb\n");
		return -EFAULT;
	}

	sss_nic_try_to_enable_rss(nic_dev);

	ret = sss_nic_init_mac_addr(nic_dev);
	if (ret != 0) {
		nic_err(nic_dev->dev_hdl, "Fail to init mac address\n");
		goto init_mac_addr_err;
	}

	sss_nic_set_mtu_range(netdev);

	ret = sss_nic_alloc_sq_desc_group(nic_dev);
	if (ret != 0) {
		nic_err(nic_dev->dev_hdl, "Fail to init sq\n");
		goto init_sq_err;
	}

	ret = sss_nic_alloc_rq_desc_group(nic_dev);
	if (ret != 0) {
		nic_err(nic_dev->dev_hdl, "Fail to init rq\n");
		goto init_rq_err;
	}

	ret = sss_nic_init_intr_coalesce(nic_dev);
	if (ret != 0) {
		nic_err(nic_dev->dev_hdl, "Fail to init interrupt and coalesce\n");
		goto init_intr_coalesce_err;
	}

	ret = sss_nic_alloc_lb_test_buf(nic_dev);
	if (ret) {
		nic_err(nic_dev->dev_hdl, "Fail to alloc loopback test buf\n");
		goto alloc_lb_test_buf_err;
	}

	return 0;

alloc_lb_test_buf_err:
	sss_nic_deinit_intr_coalesce(nic_dev);

init_intr_coalesce_err:
	sss_nic_free_rq_desc_group(nic_dev);

init_rq_err:
	sss_nic_free_sq_desc_group(nic_dev);

init_sq_err:
	sss_nic_del_mac(nic_dev, netdev->dev_addr, 0,
			sss_get_global_func_id(nic_dev->hwdev), SSS_CHANNEL_NIC);

init_mac_addr_err:
	sss_nic_free_rss_key(nic_dev);

	return ret;
}

static void sss_nic_init_netdev_ops(struct sss_nic_dev *nic_dev)
{
	sss_nic_set_netdev_ops(nic_dev);

	sss_nic_set_ethtool_ops(nic_dev);

	nic_dev->netdev->watchdog_timeo = WATCHDOG_TIMEOUT * HZ;
}

static void sss_nic_validate_parameters(struct pci_dev *pdev)
{
	u16 i;
	u16 valid_rx_buff_len_list[] = {
		RX_BUFF_VALID_2KB, RX_BUFF_VALID_4KB,
		RX_BUFF_VALID_8KB, RX_BUFF_VALID_16KB
	};

	if (!POLL_BUDGET_IS_VALID(poll_budget))
		poll_budget = DEFAULT_POLL_BUDGET;

	for (i = 0; i < ARRAY_LEN(valid_rx_buff_len_list); i++) {
		if (rx_buff_size == valid_rx_buff_len_list[i])
			return;
	}

	rx_buff_size = DEFAULT_RX_BUFF_LEN;
}

static void sss_nic_periodic_work_handler(struct work_struct *work)
{
	struct delayed_work *delay_work = to_delayed_work(work);
	struct sss_nic_dev *nic_dev = container_of(delay_work, struct sss_nic_dev, routine_work);

	if (SSSNIC_TEST_CLEAR_NIC_EVENT_FLAG(nic_dev, SSSNIC_EVENT_TX_TIMEOUT))
		sss_fault_event_report(nic_dev->hwdev, SSS_FAULT_SRC_TX_TIMEOUT,
				       SSS_FAULT_LEVEL_SERIOUS_FLR);

	queue_delayed_work(nic_dev->workq, &nic_dev->routine_work, HZ);
}

static void sss_nic_dev_resource_destroy(struct sss_nic_dev *nic_dev)
{
	destroy_workqueue(nic_dev->workq);
	kfree(nic_dev->vlan_bitmap);
}

static int sss_nic_dev_params_init(struct net_device *netdev,
				   struct sss_hal_dev *uld_dev)
{
	struct pci_dev *pdev = uld_dev->pdev;
	struct sss_nic_dev *nic_dev;

	nic_dev = (struct sss_nic_dev *)netdev_priv(netdev);
	nic_dev->hwdev = uld_dev->hwdev;
	nic_dev->netdev = netdev;
	nic_dev->pdev = pdev;
	nic_dev->dev_hdl = &pdev->dev;
	nic_dev->uld_dev = uld_dev;
	nic_dev->rx_buff_len = RX_BUFF_TO_BYTES(rx_buff_size);
	nic_dev->rx_dma_buff_size = RX_BUFF_TO_DMA_SIZE(nic_dev->rx_buff_len);
	nic_dev->page_order = BUFF_SIZE_TO_PAGE_ORDER(nic_dev->rx_dma_buff_size);
	nic_dev->poll_budget = (int)poll_budget;
	nic_dev->rx_poll_wqe = rx_poll_wqe;
	nic_dev->msg_enable = DEFAULT_MSG_ENABLE;
	nic_dev->qp_res.sq_depth = SSSNIC_SQ_DEPTH;
	nic_dev->qp_res.rq_depth = SSSNIC_RQ_DEPTH;
	nic_dev->max_qp_num = sss_get_max_sq_num(nic_dev->hwdev);
	SET_NETDEV_DEV(netdev, &pdev->dev);

	mutex_init(&nic_dev->qp_mutex);
	sema_init(&nic_dev->port_sem, 1);

	nic_dev->vlan_bitmap = kzalloc(SSSNIC_VLAN_BITMAP_SIZE(nic_dev), GFP_KERNEL);
	if (!nic_dev->vlan_bitmap)
		return -ENOMEM;

	nic_dev->workq = create_singlethread_workqueue(SSSNIC_DEV_WQ_NAME);
	if (!nic_dev->workq) {
		nic_err(&pdev->dev, "Fail to initialize nic workqueue\n");
		kfree(nic_dev->vlan_bitmap);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&nic_dev->tcam_info.tcam_node_info.tcam_node_list);
	INIT_LIST_HEAD(&nic_dev->tcam_info.tcam_list);
	INIT_LIST_HEAD(&nic_dev->rx_rule.rule_list);

	INIT_LIST_HEAD(&nic_dev->mc_filter_list);
	INIT_LIST_HEAD(&nic_dev->uc_filter_list);

	INIT_DELAYED_WORK(&nic_dev->routine_work, sss_nic_periodic_work_handler);
	INIT_DELAYED_WORK(&nic_dev->rq_watchdog_work, sss_nic_rq_watchdog_handler);
	INIT_WORK(&nic_dev->rx_mode_work, sss_nic_set_rx_mode_work);

	SSSNIC_SET_NIC_DEV_FLAG(nic_dev, SSSNIC_INTR_ADAPT);

	return 0;
}

static void sss_nic_set_default_link_follow(struct sss_nic_dev *nic_dev)
{
	int ret;

	if (SSSNIC_FUNC_IS_VF(nic_dev->hwdev))
		return;

	if (link_follow_status >= SSSNIC_LINK_FOLLOW_STATUS_MAX)
		return;

	ret = sss_nic_set_link_follow_state(nic_dev, link_follow_status);
	if (ret == SSS_MGMT_CMD_UNSUPPORTED)
		nic_warn(nic_dev->dev_hdl,
			 "Firmware doesn't support to set link status follow port status\n");
}

static int sss_nic_set_default_feature_to_hw(struct sss_nic_dev *nic_dev)
{
	int ret;

	sss_nic_set_default_link_follow(nic_dev);

	ret = sss_nic_set_feature_to_hw(nic_dev->nic_io);
	if (ret != 0) {
		nic_err(nic_dev->dev_hdl, "Fail to set nic feature\n");
		return ret;
	}

	/* enable all features in netdev->features */
	ret = sss_nic_enable_netdev_feature(nic_dev);
	if (ret != 0) {
		sss_nic_update_nic_feature(nic_dev, 0);
		sss_nic_set_feature_to_hw(nic_dev->nic_io);
		nic_err(nic_dev->dev_hdl, "Fail to set netdev feature\n");
		return ret;
	}

	if (SSSNIC_SUPPORT_RXQ_RECOVERY(nic_dev->nic_io))
		SSSNIC_SET_NIC_DEV_FLAG(nic_dev, SSSNIC_RXQ_RECOVERY);

	return 0;
}

static struct net_device *sss_nic_alloc_netdev(void *hwdev)
{
	u16 max_qps = sss_get_max_sq_num(hwdev);

	return alloc_etherdev_mq(sizeof(struct sss_nic_dev), max_qps);
}

static void sss_nic_free_netdev(struct sss_nic_dev *nic_dev)
{
	kfree(nic_dev->vlan_bitmap);
	free_netdev(nic_dev->netdev);
}

static int sss_nic_reset_function(void *hwdev)
{
	u16 glb_func_id = sss_get_global_func_id(hwdev);

	return sss_chip_reset_function(hwdev, glb_func_id, SSS_NIC_RESET, SSS_CHANNEL_NIC);
}

static int sss_nic_init_netdev(struct sss_nic_dev *nic_dev)
{
	int ret;

	sss_nic_init_netdev_ops(nic_dev);

	sss_nic_init_netdev_feature(nic_dev);

	ret = sss_nic_set_default_feature_to_hw(nic_dev);
	if (ret != 0)
		return ret;

	return 0;
}

static void sss_nic_deinit_netdev(struct sss_nic_dev *nic_dev)
{
	sss_nic_update_nic_feature(nic_dev, 0);
	sss_nic_set_feature_to_hw(nic_dev->nic_io);
}

static int sss_nic_register_netdev(struct sss_nic_dev *nic_dev)
{
	int ret;
	struct net_device *netdev = nic_dev->netdev;

#ifdef HAVE_MULTI_VLAN_OFFLOAD_EN
	sss_nic_register_notifier(nic_dev);
#endif

	ret = register_netdev(netdev);
	if (ret != 0) {
#ifdef HAVE_MULTI_VLAN_OFFLOAD_EN
		sss_nic_unregister_notifier(nic_dev);
#endif
		nic_err(nic_dev->dev_hdl, "Fail to register netdev\n");
		return -ENOMEM;
	}

	queue_delayed_work(nic_dev->workq, &nic_dev->routine_work, HZ);

	netif_carrier_off(netdev);

	return 0;
}

static void sss_nic_unregister_netdev(struct sss_nic_dev *nic_dev)
{
	unregister_netdev(nic_dev->netdev);

#ifdef HAVE_MULTI_VLAN_OFFLOAD_EN
	sss_nic_unregister_notifier(nic_dev);
#endif
	cancel_delayed_work_sync(&nic_dev->routine_work);
	cancel_delayed_work_sync(&nic_dev->rq_watchdog_work);
	cancel_work_sync(&nic_dev->rx_mode_work);
	destroy_workqueue(nic_dev->workq);
}

static int sss_nic_probe(struct sss_hal_dev *hal_dev, void **uld_dev,
			 char *uld_dev_name)
{
	struct pci_dev *pdev = hal_dev->pdev;
	void *hwdev = hal_dev->hwdev;
	struct sss_nic_dev *nic_dev = NULL;
	struct net_device *netdev = NULL;
	int ret;

	if (!sss_support_nic(hwdev)) {
		nic_info(&pdev->dev, "Hw don't support nic\n");
		return 0;
	}

	nic_info(&pdev->dev, "NIC probe begin\n");

	sss_nic_validate_parameters(pdev);

	ret = sss_nic_reset_function(hwdev);
	if (ret != 0) {
		nic_err(&pdev->dev, "Fail to reset function\n");
		goto err_out;
	}

	netdev = sss_nic_alloc_netdev(hwdev);
	if (!netdev) {
		nic_err(&pdev->dev, "Fail to allocate net device\n");
		ret = -ENOMEM;
		goto err_out;
	}

	ret = sss_nic_dev_params_init(netdev, hal_dev);
	if (ret != 0) {
		nic_err(&pdev->dev, "Fail to init nic_dev params\n");
		goto nic_dev_params_init_err;
	}

	nic_dev = (struct sss_nic_dev *)netdev_priv(netdev);

	ret = sss_nic_io_init(nic_dev);
	if (ret != 0) {
		nic_err(&pdev->dev, "Fail to init nic io\n");
		goto nic_io_init_err;
	}

	ret = sss_nic_dev_init(nic_dev);
	if (ret != 0) {
		nic_err(&pdev->dev, "Fail to init nic dev\n");
		goto nic_dev_init_err;
	}

	ret = sss_nic_init_netdev(nic_dev);
	if (ret != 0) {
		nic_err(&pdev->dev, "Fail to init net device\n");
		goto init_netdev_err;
	}

	ret = sss_nic_register_netdev(nic_dev);
	if (ret != 0) {
		nic_err(&pdev->dev, "Fail to register net device\n");
		goto register_netdev_err;
	}

	*uld_dev = nic_dev;
	nic_info(&pdev->dev, "Success to probe NIC\n");

	return 0;

register_netdev_err:
	sss_nic_deinit_netdev(nic_dev);

init_netdev_err:
	sss_nic_dev_deinit(nic_dev);

nic_dev_init_err:
	sss_nic_io_deinit(nic_dev);

nic_io_init_err:
	sss_nic_dev_resource_destroy(nic_dev);

nic_dev_params_init_err:
	free_netdev(netdev);

err_out:
	nic_err(&pdev->dev, "Fail to run NIC probe\n");

	return ret;
}

static void sss_nic_remove(struct sss_hal_dev *hal_dev, void *adapter)
{
	struct sss_nic_dev *nic_dev = adapter;

	if (!nic_dev || !sss_support_nic(hal_dev->hwdev))
		return;

	nic_info(&hal_dev->pdev->dev, "NIC remove begin\n");

	sss_nic_unregister_netdev(nic_dev);

	sss_nic_flush_tcam(nic_dev);

	sss_nic_deinit_netdev(nic_dev);

	sss_nic_dev_deinit(nic_dev);

	sss_nic_io_deinit(nic_dev);

	sss_nic_free_netdev(nic_dev);

	nic_info(&hal_dev->pdev->dev, "Success to remove NIC\n");
}

static void sss_nic_sriov_state_change(struct sss_nic_dev *nic_dev,
				       struct sss_event_info *event)
{
	struct sss_sriov_state_info *info = (void *)event->event_data;

	if (!info->enable)
		sss_nic_clear_all_vf_info(nic_dev->nic_io);
}

void sss_nic_port_module_cable_plug(struct sss_nic_dev *nic_dev, void *event_data)
{
	nicif_info(nic_dev, link, nic_dev->netdev,
		   "Port module event: Cable plugged\n");
}

void sss_nic_port_module_cable_unplug(struct sss_nic_dev *nic_dev, void *event_data)
{
	nicif_info(nic_dev, link, nic_dev->netdev,
		   "Port module event: Cable unplugged\n");
}

void sss_nic_port_module_link_err(struct sss_nic_dev *nic_dev, void *event_data)
{
	struct sss_nic_port_module_event *port_event = event_data;
	enum link_err_type err_type = port_event->err_type;

	nicif_info(nic_dev, link, nic_dev->netdev,
		   "Fail to link, err_type: 0x%x\n", err_type);
}

static void sss_nic_port_module_event_handler(struct sss_nic_dev *nic_dev,
					      struct sss_event_info *event)
{
	struct sss_nic_port_module_event *port_event = (void *)event->event_data;
	enum port_module_event_type type = port_event->type;

	sss_nic_port_module_event_handler_t handler[SSSNIC_PORT_MODULE_MAX_EVENT] = {
		sss_nic_port_module_cable_plug,
		sss_nic_port_module_cable_unplug,
		sss_nic_port_module_link_err,
	};

	if (type >= SSSNIC_PORT_MODULE_MAX_EVENT) {
		nicif_err(nic_dev, link, nic_dev->netdev,
			  "Unknown port module type %d\n", type);
		return;
	}

	if (handler[type])
		handler[type](nic_dev, event->event_data);
}

static void sss_nic_link_down(struct sss_nic_dev *nic_dev, struct sss_event_info *event)
{
	struct net_device *netdev = nic_dev->netdev;

	if (!SSS_CHANNEL_RES_VALID(nic_dev) ||
	    test_bit(SSSNIC_LP_TEST, &nic_dev->flags) ||
	    test_bit(SSSNIC_FORCE_LINK_UP, &nic_dev->flags))
		return;

	if (!netif_carrier_ok(netdev))
		return;

	netif_carrier_off(netdev);
	nic_dev->link_status = false;
	nicif_info(nic_dev, link, netdev, "Link is down\n");
}

static void sss_nic_link_up(struct sss_nic_dev *nic_dev, struct sss_event_info *event)
{
	struct net_device *netdev = nic_dev->netdev;

	if (!SSS_CHANNEL_RES_VALID(nic_dev) ||
	    test_bit(SSSNIC_LP_TEST, &nic_dev->flags) ||
	    test_bit(SSSNIC_FORCE_LINK_UP, &nic_dev->flags))
		return;

	if (netif_carrier_ok(netdev))
		return;

	netif_carrier_on(netdev);
	nic_dev->link_status = true;

	nicif_info(nic_dev, link, netdev, "Link is up\n");
}

static void sss_nic_comm_fail_envet_handler(struct sss_nic_dev *nic_dev,
					    struct sss_event_info *event)
{
	struct sss_fault_event *fault = (void *)event->event_data;

	if (fault->fault_level == SSS_FAULT_LEVEL_SERIOUS_FLR &&
	    fault->info.chip.func_id == sss_get_global_func_id(nic_dev->hwdev))
		sss_nic_link_down(nic_dev, event);
}

static void sss_nic_event_handler(struct sss_nic_dev *nic_dev, struct sss_event_info *event)
{
	sss_nic_event_handler_t handler[SSSNIC_EVENT_MAX] = {
		sss_nic_link_down,
		sss_nic_link_up,
		sss_nic_port_module_event_handler,
		NULL,
	};

	if (event->type >= SSSNIC_EVENT_MAX)
		return;

	if (handler[event->type])
		handler[event->type](nic_dev, event);
}

static void sss_nic_comm_event_handler(struct sss_nic_dev *nic_dev,
				       struct sss_event_info *event)
{
	sss_nic_event_handler_t handler[SSS_EVENT_MAX] = {
		sss_nic_link_down,
		sss_nic_link_down,
		sss_nic_comm_fail_envet_handler,
		sss_nic_sriov_state_change,
		NULL,
		sss_nic_link_down,
	};

	if (event->type >= SSS_EVENT_MAX)
		return;

	if (handler[event->type])
		handler[event->type](nic_dev, event);
}

static void sss_nic_event(struct sss_hal_dev *uld_dev, void *adapter,
			  struct sss_event_info *event)
{
	struct sss_nic_dev *nic_dev = adapter;

	if (!nic_dev || !event || !sss_support_nic(uld_dev->hwdev))
		return;

	if (event->service == SSS_EVENT_SRV_NIC) {
		sss_nic_event_handler(nic_dev, event);
		return;
	}

	if (event->service == SSS_EVENT_SRV_COMM) {
		sss_nic_comm_event_handler(nic_dev, event);
		return;
	}
}

struct sss_uld_info g_nic_uld_info = {
	.probe = sss_nic_probe,
	.remove = sss_nic_remove,
	.suspend = NULL,
	.resume = NULL,
	.event = sss_nic_event,
	.ioctl = sss_tool_ioctl,
};

struct sss_uld_info *get_nic_uld_info(void)
{
	return &g_nic_uld_info;
}

static __init int sss_nic_init(void)
{
	int ret;

	pr_info("%s - version %s\n", SSSNIC_DRV_DESC,
		SSSNIC_DRV_VERSION);

	ret = sss_init_pci();
	if (ret) {
		pr_err("SDK init failed.\n");
		return ret;
	}

	ret = sss_register_uld(SSS_SERVICE_TYPE_NIC, &g_nic_uld_info);
	if (ret != 0) {
		pr_err("Fail to register sss_nic uld\n");
		sss_exit_pci();
		return ret;
	}

	return 0;
}

static __exit void sss_nic_exit(void)
{
	sss_unregister_uld(SSS_SERVICE_TYPE_NIC);
	sss_exit_pci();
}

#ifndef _LLT_TEST_
module_init(sss_nic_init);
module_exit(sss_nic_exit);
#endif

MODULE_AUTHOR("steven.song@3snic.com");
MODULE_DESCRIPTION("3SNIC Network Interface Card Driver");
MODULE_VERSION(SSSNIC_DRV_VERSION);
MODULE_LICENSE("GPL");
