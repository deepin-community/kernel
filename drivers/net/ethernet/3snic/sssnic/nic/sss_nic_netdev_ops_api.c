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
#include "sss_nic_tx_init.h"
#include "sss_nic_rx_init.h"
#include "sss_nic_rx.h"
#include "sss_nic_dcb.h"
#include "sss_nic_netdev_ops_api.h"
#include "sss_nic_irq.h"
#include "sss_nic_cfg.h"
#include "sss_nic_vf_cfg.h"
#include "sss_nic_mag_cfg.h"

#define IPV4_VERSION		4
#define IPV6_VERSION		6

#define SSSNIC_LRO_DEF_COAL_PKT_SIZE			32
#define SSSNIC_LRO_DEF_TIME_LIMIT				16
#define SSSNIC_WAIT_FLUSH_QP_RES_TIMEOUT		100

#define SSSNIC_IPV6_ADDR_SIZE			4
#define SSSNIC_PKT_INFO_SIZE			9
#define SSSNIC_BIT_PER_TUPLE			32

#define SSSNIC_RSS_VAL(val, type)		\
	(((type) == SSSNIC_RSS_ENGINE_TOEP) ? ntohl(val) : (val))

/* Low 16 bits are sport, High 16 bits are dport */
#define SSSNIC_RSS_VAL_BY_L4_PORT(l4_hdr) \
			(((u32)ntohs(*((u16 *)(l4_hdr) + 1U)) << 16) | ntohs(*(u16 *)(l4_hdr)))

#define SSSNIC_GET_SQ_ID_BY_RSS_INDIR(nic_dev, sq_id) \
			((u16)(nic_dev)->rss_indir_tbl[(sq_id) & 0xFF])

#define SSSNIC_GET_DSCP_PRI_OFFSET		2

#define SSSNIC_FEATURE_OP_STR(op)		((op) ? "Enable" : "Disable")

#define SSSNIC_VLAN_TCI_TO_COS_ID(skb)	\
		((skb)->vlan_tci >> VLAN_PRIO_SHIFT)

#define SSSNIC_IPV4_DSF_TO_COS_ID(skb)	\
		(ipv4_get_dsfield(ip_hdr(skb)) >> SSSNIC_GET_DSCP_PRI_OFFSET)

#define SSSNIC_IPV6_DSF_TO_COS_ID(skb)	\
		(ipv6_get_dsfield(ipv6_hdr(skb)) >> SSSNIC_GET_DSCP_PRI_OFFSET)

static int sss_nic_alloc_qp_mgmt_info(struct sss_nic_dev *nic_dev,
				      struct sss_nic_qp_resource *qp_res)
{
	u16 qp_num = qp_res->qp_num;
	u32 len;

	len = sizeof(*qp_res->irq_cfg) * qp_num;
	qp_res->irq_cfg = kzalloc(len, GFP_KERNEL);
	if (!qp_res->irq_cfg) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to alloc irq config\n");
		return -ENOMEM;
	}

	len = sizeof(*qp_res->rq_res_group) * qp_num;
	qp_res->rq_res_group = kzalloc(len, GFP_KERNEL);
	if (!qp_res->rq_res_group) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to alloc rq res info\n");
		goto alloc_rq_res_err;
	}

	len = sizeof(*qp_res->sq_res_group) * qp_num;
	qp_res->sq_res_group = kzalloc(len, GFP_KERNEL);
	if (!qp_res->sq_res_group) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to alloc sq res info\n");
		goto alloc_sq_res_err;
	}

	return 0;

alloc_sq_res_err:
	kfree(qp_res->rq_res_group);
	qp_res->rq_res_group = NULL;

alloc_rq_res_err:
	kfree(qp_res->irq_cfg);
	qp_res->irq_cfg = NULL;

	return -ENOMEM;
}

static void sss_nic_free_qp_mgmt_info(struct sss_nic_qp_resource *qp_res)
{
	kfree(qp_res->irq_cfg);
	kfree(qp_res->rq_res_group);
	kfree(qp_res->sq_res_group);
	qp_res->irq_cfg = NULL;
	qp_res->sq_res_group = NULL;
	qp_res->rq_res_group = NULL;
}

static int sss_nic_alloc_qp_resource(struct sss_nic_dev *nic_dev,
				     struct sss_nic_qp_resource *qp_res)
{
	int ret;

	ret = sss_nic_alloc_qp_mgmt_info(nic_dev, qp_res);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to alloc qp mgmt info\n");
		return ret;
	}

	ret = sss_nic_alloc_rq_res_group(nic_dev, qp_res);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to alloc rq resource\n");
		goto alloc_rq_res_err;
	}

	ret = sss_nic_alloc_sq_resource(nic_dev, qp_res);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to alloc sq resource\n");
		goto alloc_sq_res_err;
	}

	return 0;

alloc_sq_res_err:
	sss_nic_free_rq_res_group(nic_dev, qp_res);

alloc_rq_res_err:
	sss_nic_free_qp_mgmt_info(qp_res);

	return ret;
}

static void sss_nic_free_qp_resource(struct sss_nic_dev *nic_dev,
				     struct sss_nic_qp_resource *qp_res)
{
	sss_nic_free_rq_res_group(nic_dev, qp_res);
	sss_nic_free_sq_resource(nic_dev, qp_res);
	sss_nic_free_qp_mgmt_info(qp_res);
}

static int sss_nic_init_qp_wq(struct sss_nic_dev *nic_dev,
			      struct sss_nic_qp_resource *qp_res)
{
	int ret;

	sss_nic_init_all_sq(nic_dev, qp_res);

	ret = sss_nic_init_rq_desc_group(nic_dev, qp_res);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to configure rq\n");
		return ret;
	}

	return 0;
}

static void sss_nic_config_dcb_qp_map(struct sss_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	u8 cos_num;
	u16 qp_num = nic_dev->qp_res.qp_num;

	if (!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE)) {
		sss_nic_update_sq_cos(nic_dev, 0);
		return;
	}

	cos_num = sss_nic_get_user_cos_num(nic_dev);
	sss_nic_update_qp_cos_map(nic_dev, cos_num);
	/* For now, we don't support to change cos_num */
	if (cos_num > nic_dev->max_cos_num || cos_num > qp_num) {
		nicif_err(nic_dev, drv, netdev,
			  "Invalid cos_num: %u, qp_num: %u or RSS is disable, disable DCB\n",
			  cos_num, qp_num);
		nic_dev->qp_res.cos_num = 0;
		SSSNIC_CLEAR_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE);
		/* if we can't enable rss or get enough qp_num,
		 * need to sync default configure to hw
		 */
		sss_nic_update_dcb_cfg(nic_dev);
	}

	sss_nic_update_sq_cos(nic_dev, 1);
}

static int sss_nic_update_dev_cfg(struct sss_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	int ret;

	ret = sss_nic_set_dev_mtu(nic_dev, (u16)netdev->mtu);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to set mtu\n");
		return ret;
	}

	sss_nic_config_dcb_qp_map(nic_dev);

	ret = sss_nic_update_rx_rss(nic_dev);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to update rx rss\n");
		return ret;
	}

	return 0;
}

static u16 sss_nic_realloc_qp_irq(struct sss_nic_dev *nic_dev,
				  u16 new_qp_irq_num)
{
	struct sss_irq_desc *qps_irq_info = nic_dev->irq_desc_group;
	u16 act_irq_num;
	u16 extra_irq_num;
	u16 id;
	u16 i;

	if (new_qp_irq_num > nic_dev->irq_desc_num) {
		extra_irq_num = new_qp_irq_num - nic_dev->irq_desc_num;
		act_irq_num = sss_alloc_irq(nic_dev->hwdev, SSS_SERVICE_TYPE_NIC,
					    &qps_irq_info[nic_dev->irq_desc_num],
					    extra_irq_num);
		if (act_irq_num == 0) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to alloc irq\n");
			return nic_dev->irq_desc_num;
		}

		nic_dev->irq_desc_num += act_irq_num;
	} else if (new_qp_irq_num < nic_dev->irq_desc_num) {
		extra_irq_num = nic_dev->irq_desc_num - new_qp_irq_num;
		for (i = 0; i < extra_irq_num; i++) {
			id = (nic_dev->irq_desc_num - i) - 1;
			sss_free_irq(nic_dev->hwdev, SSS_SERVICE_TYPE_NIC,
				     qps_irq_info[id].irq_id);
			qps_irq_info[id].irq_id = 0;
			qps_irq_info[id].msix_id = 0;
		}
		nic_dev->irq_desc_num = new_qp_irq_num;
	}

	return nic_dev->irq_desc_num;
}

static void sss_nic_update_dcb_cos_map(struct sss_nic_dev *nic_dev,
				       const struct sss_nic_qp_resource *qp_res)
{
	u8 cos_num = qp_res->cos_num;
	u16 max_qp = qp_res->qp_num;
	u8 user_cos_num = sss_nic_get_user_cos_num(nic_dev);

	if (cos_num == 0 || cos_num > nic_dev->max_cos_num || cos_num > max_qp)
		return; /* will disable DCB */

	sss_nic_update_qp_cos_map(nic_dev, user_cos_num);
}

static void sss_nic_update_qp_info(struct sss_nic_dev *nic_dev,
				   struct sss_nic_qp_resource *qp_res)
{
	u16 alloc_irq_num;
	u16 dst_irq_num;
	u16 cur_irq_num;
	struct net_device *netdev = nic_dev->netdev;

	if (!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_RSS_ENABLE))
		qp_res->qp_num = 1;

	sss_nic_update_dcb_cos_map(nic_dev, qp_res);

	if (nic_dev->irq_desc_num >= qp_res->qp_num)
		goto out;

	cur_irq_num = nic_dev->irq_desc_num;

	alloc_irq_num = sss_nic_realloc_qp_irq(nic_dev, qp_res->qp_num);
	if (alloc_irq_num < qp_res->qp_num) {
		qp_res->qp_num = alloc_irq_num;
		sss_nic_update_dcb_cos_map(nic_dev, qp_res);
		nicif_warn(nic_dev, drv, netdev,
			   "Fail to alloc enough irq, qp_num: %u\n",
			   qp_res->qp_num);

		dst_irq_num = (u16)max_t(u16, cur_irq_num, qp_res->qp_num);
		sss_nic_realloc_qp_irq(nic_dev, dst_irq_num);
	}

out:
	nicif_info(nic_dev, drv, netdev, "Finally qp_num: %u\n",
		   qp_res->qp_num);
}

static int sss_nic_init_qp_irq(struct sss_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	u32 irq_info_len = sizeof(*nic_dev->irq_desc_group) * nic_dev->max_qp_num;

	nic_dev->irq_desc_num = 0;

	if (irq_info_len == 0) {
		nicif_err(nic_dev, drv, netdev, "Invalid irq_info_len\n");
		return -EINVAL;
	}

	nic_dev->irq_desc_group = kzalloc(irq_info_len, GFP_KERNEL);
	if (!nic_dev->irq_desc_group)
		return -ENOMEM;

	if (!test_bit(SSSNIC_RSS_ENABLE, &nic_dev->flags))
		nic_dev->qp_res.qp_num = 1;

	if (nic_dev->irq_desc_num >= nic_dev->qp_res.qp_num) {
		nicif_info(nic_dev, drv, netdev, "Finally qp_num: %u\n",
			   nic_dev->qp_res.qp_num);
		return 0;
	}

	nic_dev->irq_desc_num = sss_alloc_irq(nic_dev->hwdev, SSS_SERVICE_TYPE_NIC,
					      nic_dev->irq_desc_group, nic_dev->qp_res.qp_num);
	if (nic_dev->irq_desc_num == 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to alloc qp irq\n");
		kfree(nic_dev->irq_desc_group);
		nic_dev->irq_desc_group = NULL;
		return -ENOMEM;
	}

	if (nic_dev->irq_desc_num < nic_dev->qp_res.qp_num) {
		nic_dev->qp_res.qp_num = nic_dev->irq_desc_num;
		nicif_warn(nic_dev, drv, netdev,
			   "Fail to alloc enough irq, now qp_num: %u\n",
			   nic_dev->qp_res.qp_num);
	}

	return 0;
}

static void sss_nic_deinit_qp_irq(struct sss_nic_dev *nic_dev)
{
	u16 id;

	for (id = 0; id < nic_dev->irq_desc_num; id++)
		sss_free_irq(nic_dev->hwdev, SSS_SERVICE_TYPE_NIC,
			     nic_dev->irq_desc_group[id].irq_id);

	kfree(nic_dev->irq_desc_group);
	nic_dev->irq_desc_group = NULL;
}

int sss_nic_dev_resource_init(struct sss_nic_dev *nic_dev)
{
	int ret;
	struct net_device *netdev = nic_dev->netdev;

	ret = sss_nic_init_qp_irq(nic_dev);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to init irq info\n");
		return ret;
	}

	sss_nic_update_dcb_cos_map(nic_dev, &nic_dev->qp_res);

	return 0;
}

void sss_nic_dev_resource_deinit(struct sss_nic_dev *nic_dev)
{
	sss_nic_deinit_qp_irq(nic_dev);
}

static int sss_nic_set_port_state(struct sss_nic_dev *nic_dev, bool state)
{
	int ret;

	down(&nic_dev->port_sem);

	ret = sss_nic_set_hw_port_state(nic_dev, state, SSS_CHANNEL_NIC);

	up(&nic_dev->port_sem);

	return ret;
}

static void sss_nic_update_link_state(struct sss_nic_dev *nic_dev,
				      u8 link_state)
{
	struct net_device *netdev = nic_dev->netdev;

	if (nic_dev->link_status == link_state)
		return;

	nic_dev->link_status = link_state;

	nicif_info(nic_dev, link, netdev, "Link is %s\n",
		   (link_state ? "up" : "down"));
}

int sss_nic_qp_resource_init(struct sss_nic_dev *nic_dev,
			     struct sss_nic_qp_info *qp_info,
			     struct sss_nic_qp_resource *qp_res)
{
	int ret;
	struct net_device *netdev = nic_dev->netdev;

	qp_info->sq_depth = qp_res->sq_depth;
	qp_info->rq_depth = qp_res->rq_depth;
	qp_info->qp_num = qp_res->qp_num;

	ret = sss_nic_alloc_qp(nic_dev->nic_io, nic_dev->irq_desc_group, qp_info);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to alloc qp\n");
		return ret;
	}

	ret = sss_nic_alloc_qp_resource(nic_dev, qp_res);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to alloc qp resource\n");
		sss_nic_free_qp(nic_dev->nic_io, qp_info);
		return ret;
	}

	return 0;
}

void sss_nic_qp_resource_deinit(struct sss_nic_dev *nic_dev,
				struct sss_nic_qp_info *qp_info,
				struct sss_nic_qp_resource *qp_res)
{
	mutex_lock(&nic_dev->qp_mutex);
	sss_nic_free_qp_resource(nic_dev, qp_res);
	sss_nic_free_qp(nic_dev->nic_io, qp_info);
	mutex_unlock(&nic_dev->qp_mutex);
}

int sss_nic_open_dev(struct sss_nic_dev *nic_dev,
		     struct sss_nic_qp_info *qp_info,
		     struct sss_nic_qp_resource *qp_res)
{
	int ret;
	struct net_device *netdev = nic_dev->netdev;

	ret = sss_nic_init_qp_info(nic_dev->nic_io, qp_info);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to init qp info\n");
		return ret;
	}

	ret = sss_nic_init_qp_wq(nic_dev, qp_res);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to init qp wq\n");
		goto cfg_qp_err;
	}

	ret = sss_nic_request_qp_irq(nic_dev);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to request qp irq\n");
		goto init_qp_irq_err;
	}

	ret = sss_nic_update_dev_cfg(nic_dev);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to update configure\n");
		goto cfg_err;
	}

	return 0;

cfg_err:
	sss_nic_release_qp_irq(nic_dev);

init_qp_irq_err:
cfg_qp_err:
	sss_nic_deinit_qp_info(nic_dev->nic_io, qp_info);

	return ret;
}

void sss_nic_close_dev(struct sss_nic_dev *nic_dev,
		       struct sss_nic_qp_info *qp_info)
{
	sss_nic_reset_rx_rss(nic_dev->netdev);
	sss_nic_release_qp_irq(nic_dev);
	sss_nic_deinit_qp_info(nic_dev->nic_io, qp_info);
}

int sss_nic_vport_up(struct sss_nic_dev *nic_dev)
{
	u16 func_id;
	u8 link_state = 0;
	int ret;
	struct net_device *netdev = nic_dev->netdev;

	func_id = sss_get_global_func_id(nic_dev->hwdev);
	ret = sss_nic_set_hw_vport_state(nic_dev, func_id, true, SSS_CHANNEL_NIC);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to set vport enable\n");
		goto set_vport_state_err;
	}

	ret = sss_nic_set_port_state(nic_dev, true);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to set port enable\n");
		goto set_port_state_err;
	}

	netif_set_real_num_rx_queues(netdev, nic_dev->qp_res.qp_num);
	netif_set_real_num_tx_queues(netdev, nic_dev->qp_res.qp_num);
	netif_tx_wake_all_queues(netdev);

	if (!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_FORCE_LINK_UP)) {
		ret = sss_nic_get_hw_link_state(nic_dev, &link_state);
		if (ret == 0 && link_state != 0)
			netif_carrier_on(netdev);
	} else {
		link_state = true;
		netif_carrier_on(netdev);
	}

	queue_delayed_work(nic_dev->workq, &nic_dev->moderation_task,
			   SSSNIC_MODERATONE_DELAY);
	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_RXQ_RECOVERY))
		queue_delayed_work(nic_dev->workq, &nic_dev->rq_watchdog_work, HZ);

	sss_nic_update_link_state(nic_dev, link_state);

	if (!SSSNIC_FUNC_IS_VF(nic_dev->hwdev))
		sss_nic_notify_all_vf_link_state(nic_dev->nic_io, link_state);

	return 0;

set_port_state_err:
	sss_nic_set_hw_vport_state(nic_dev, func_id, false, SSS_CHANNEL_NIC);

set_vport_state_err:
	sss_nic_clear_hw_qp_resource(nic_dev);
	/*No packets will be send to host when after set vport disable 100ms*/
	msleep(SSSNIC_WAIT_FLUSH_QP_RES_TIMEOUT);

	return ret;
}

void sss_nic_vport_down(struct sss_nic_dev *nic_dev)
{
	u16 func_id;

	netif_carrier_off(nic_dev->netdev);
	netif_tx_disable(nic_dev->netdev);

	cancel_delayed_work_sync(&nic_dev->rq_watchdog_work);
	cancel_delayed_work_sync(&nic_dev->moderation_task);

	if (sss_get_dev_present_flag(nic_dev->hwdev) == 0)
		return;

	if (SSSNIC_FUNC_IS_VF(nic_dev->hwdev) == 0)
		sss_nic_notify_all_vf_link_state(nic_dev->nic_io, 0);

	sss_nic_set_port_state(nic_dev, false);

	func_id = sss_get_global_func_id(nic_dev->hwdev);
	sss_nic_set_hw_vport_state(nic_dev, func_id, false, SSS_CHANNEL_NIC);

	sss_nic_flush_all_sq(nic_dev);
	msleep(SSSNIC_WAIT_FLUSH_QP_RES_TIMEOUT);
	sss_nic_clear_hw_qp_resource(nic_dev);
}

int sss_nic_update_channel_setting(struct sss_nic_dev *nic_dev,
				   struct sss_nic_qp_resource *qp_res,
				   sss_nic_reopen_handler_t reopen_hdl,
				   const void *priv_data)
{
	struct net_device *netdev = nic_dev->netdev;
	struct sss_nic_qp_info cur_qp_info = {0};
	struct sss_nic_qp_info new_qp_info = {0};
	int ret;

	sss_nic_update_qp_info(nic_dev, qp_res);

	ret = sss_nic_qp_resource_init(nic_dev, &new_qp_info, qp_res);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev,
			  "Fail to alloc channel resource\n");
		return ret;
	}

	if (!SSSNIC_TEST_SET_NIC_DEV_FLAG(nic_dev, SSSNIC_CHANGE_RES_INVALID)) {
		sss_nic_vport_down(nic_dev);
		sss_nic_close_dev(nic_dev, &cur_qp_info);
		sss_nic_qp_resource_deinit(nic_dev, &cur_qp_info,
					   &nic_dev->qp_res);
	}

	if (nic_dev->irq_desc_num > qp_res->qp_num)
		sss_nic_realloc_qp_irq(nic_dev, qp_res->qp_num);
	nic_dev->qp_res = *qp_res;

	if (reopen_hdl)
		reopen_hdl(nic_dev, priv_data);

	ret = sss_nic_open_dev(nic_dev, &new_qp_info, qp_res);
	if (ret != 0)
		goto open_channel_err;

	ret = sss_nic_vport_up(nic_dev);
	if (ret != 0)
		goto up_vport_err;

	clear_bit(SSSNIC_CHANGE_RES_INVALID, &nic_dev->flags);
	nicif_info(nic_dev, drv, netdev, "Success to update channel settings\n");

	return 0;

up_vport_err:
	sss_nic_close_dev(nic_dev, &new_qp_info);

open_channel_err:
	sss_nic_qp_resource_deinit(nic_dev, &new_qp_info, qp_res);

	return ret;
}

static u32 sss_nic_calc_xor_rss(u8 *rss_tunple, u32 size)
{
	u32 count;
	u32 hash_value;

	hash_value = rss_tunple[0];
	for (count = 1; count < size; count++)
		hash_value = hash_value ^ rss_tunple[count];

	return hash_value;
}

static u32 sss_nic_calc_toep_rss(const u32 *rss_tunple, u32 size, const u32 *rss_key)
{
	u32 i;
	u32 j;
	u32 rss = 0;
	u32 tunple;

	for (i = 0; i < size; i++) {
		for (j = 0; j < SSSNIC_BIT_PER_TUPLE; j++) {
			tunple = rss_tunple[i] &
				 ((u32)1 << (u32)((SSSNIC_BIT_PER_TUPLE - 1) - j));
			if (tunple != 0)
				rss ^= (rss_key[i] << j) |
				       ((u32)((u64)rss_key[i + 1] >> (SSSNIC_BIT_PER_TUPLE - j)));
		}
	}

	return rss;
}

static u8 sss_nic_parse_ipv6_info(struct sk_buff *skb, u8 hash_engine,
				  u32 *rss_tunple, u32 *size)
{
	struct ipv6hdr *ipv6hdr = ipv6_hdr(skb);
	u32 *daddr = (u32 *)&ipv6hdr->daddr;
	u32 *saddr = (u32 *)&ipv6hdr->saddr;
	u32 offset;
	u8 i;

	for (i = 0; i < SSSNIC_IPV6_ADDR_SIZE; i++) {
		rss_tunple[i] = SSSNIC_RSS_VAL(daddr[i], hash_engine);
		/* The offset of the sport relative to the dport is 4 */
		offset = (u32)(i + SSSNIC_IPV6_ADDR_SIZE);
		rss_tunple[offset] = SSSNIC_RSS_VAL(saddr[i], hash_engine);
	}
	*size = SSSNIC_IPV6_ADDR_SIZE << 1;

	return (skb_network_header(skb) + sizeof(*ipv6hdr) ==
	    skb_transport_header(skb)) ? ipv6hdr->nexthdr : 0;
}

u16 sss_nic_select_queue_by_hash_func(struct net_device *dev, struct sk_buff *skb,
				      unsigned int max_sq_num)
{
	struct iphdr *iphdr = NULL;
	unsigned char *l4_hdr = NULL;
	struct sss_nic_dev *nic_dev = netdev_priv(dev);
	struct sss_nic_rss_type rss_type = nic_dev->rss_type;
	u8 l4_proto;
	u32 sq_id = 0;
	u32 cnt = 0;
	u8 hash_engine = nic_dev->rss_hash_engine;
	u32 rss_tunple[SSSNIC_PKT_INFO_SIZE] = {0};
	bool convert_flag;

	if (skb_rx_queue_recorded(skb)) {
		sq_id = skb_get_rx_queue(skb);
		if (unlikely(sq_id >= max_sq_num))
			sq_id %= max_sq_num;

		return (u16)sq_id;
	}

	iphdr = ip_hdr(skb);

	if (iphdr->version != IPV4_VERSION && iphdr->version != IPV6_VERSION)
		return (u16)sq_id;

	if (iphdr->version == IPV4_VERSION) {
		rss_tunple[cnt++] = SSSNIC_RSS_VAL(iphdr->daddr, hash_engine);
		rss_tunple[cnt++] = SSSNIC_RSS_VAL(iphdr->saddr, hash_engine);
		l4_proto = iphdr->protocol;
		convert_flag = ((l4_proto == IPPROTO_UDP) && rss_type.udp_ipv4) ||
			       ((l4_proto == IPPROTO_TCP) && rss_type.tcp_ipv4);
	} else {
		l4_proto = sss_nic_parse_ipv6_info(skb, hash_engine, (u32 *)rss_tunple, &cnt);
		convert_flag = ((l4_proto == IPPROTO_UDP) && rss_type.udp_ipv6) ||
			       ((l4_proto == IPPROTO_TCP) && rss_type.tcp_ipv6);
	}

	if (convert_flag) {
		l4_hdr = skb_transport_header(skb);
		rss_tunple[cnt++] = SSSNIC_RSS_VAL_BY_L4_PORT(l4_hdr);
	}

	if (hash_engine == SSSNIC_RSS_ENGINE_TOEP)
		sq_id = sss_nic_calc_toep_rss((u32 *)rss_tunple, cnt, nic_dev->rss_key_big);
	else
		sq_id = sss_nic_calc_xor_rss((u8 *)rss_tunple, cnt * (u32)sizeof(cnt));

	return SSSNIC_GET_SQ_ID_BY_RSS_INDIR(nic_dev, sq_id);
}

static inline u8 sss_nic_get_cos_by_dscp(struct sss_nic_dev *nic_dev, struct sk_buff *skb)
{
	int dscp_cp;

	dscp_cp = (skb->protocol == htons(ETH_P_IP)) ? SSSNIC_IPV4_DSF_TO_COS_ID(skb) :
		  (skb->protocol == htons(ETH_P_IPV6) ? SSSNIC_IPV6_DSF_TO_COS_ID(skb) :
		  nic_dev->hw_dcb_cfg.default_cos);
	return nic_dev->hw_dcb_cfg.dscp2cos[dscp_cp];
}

static inline u8 sss_nic_get_cos_by_pcp(struct sss_nic_dev *nic_dev,
					struct sk_buff *skb)
{
	return skb->vlan_tci ?
	       nic_dev->hw_dcb_cfg.pcp2cos[SSSNIC_VLAN_TCI_TO_COS_ID(skb)] :
	       nic_dev->hw_dcb_cfg.default_cos;
}

u8 sss_nic_get_cos(struct sss_nic_dev *nic_dev, struct sk_buff *skb)
{
	if (nic_dev->hw_dcb_cfg.trust == DCB_PCP)
		return sss_nic_get_cos_by_pcp(nic_dev, skb);

	return sss_nic_get_cos_by_dscp(nic_dev, skb);
}

#ifdef NEED_VLAN_RESTORE
static int sss_nic_restore_vlan(struct sss_nic_dev *nic_dev)
{
	int ret = 0;
#if defined(CONFIG_VLAN_8021Q) || defined(CONFIG_VLAN_8021Q_MODULE)
#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	u16 i;
	struct net_device *netdev = nic_dev->netdev;
	struct net_device *vlandev = NULL;

	rcu_read_lock();
	for (i = 0; i < VLAN_N_VID; i++) {
#ifdef HAVE_VLAN_FIND_DEV_DEEP_RCU
		vlandev = __vlan_find_dev_deep_rcu(netdev, htons(ETH_P_8021Q), i);
#else
		vlandev = __vlan_find_dev_deep(netdev, htons(ETH_P_8021Q), i);
#endif

		if (!vlandev && SSSNIC_TEST_VLAN_BIT(nic_dev, i) != 0) {
			ret = netdev->netdev_ops->ndo_vlan_rx_kill_vid(netdev,
								       htons(ETH_P_8021Q), i);
			if (ret != 0) {
				sss_nic_err(nic_dev, drv,
					    "Fail to delete vlan %u, ret: %d\n", i, ret);
				break;
			}
		} else if (vlandev && SSSNIC_TEST_VLAN_BIT(nic_dev, i) == 0) {
			ret = netdev->netdev_ops->ndo_vlan_rx_add_vid(netdev,
								      htons(ETH_P_8021Q), i);
			if (ret != 0) {
				sss_nic_err(nic_dev, drv,
					    "Fail to restore vlan %u, ret: %d\n", i, ret);
				break;
			}
		}
	}
	rcu_read_unlock();
#endif
#endif
	return ret;
}
#endif

static int sss_nic_set_lro_feature(struct sss_nic_dev *nic_dev, netdev_features_t old_feature,
				   netdev_features_t new_feature, netdev_features_t *fail_feature)
{
	int ret;
	bool change = !!((new_feature ^ old_feature) & NETIF_F_LRO);
	bool en = !!(new_feature & NETIF_F_LRO);

	if (!change)
		return 0;

#ifdef HAVE_XDP_SUPPORT
	if (en && SSSNIC_IS_XDP_ENABLE(nic_dev)) {
		*fail_feature |= NETIF_F_LRO;
		sss_nic_err(nic_dev, drv, "Fail to enable LRO when xdp is enable\n");
		return -EINVAL;
	}
#endif
	ret = sss_nic_set_rx_lro_state(nic_dev, en,
				       SSSNIC_LRO_DEF_TIME_LIMIT, SSSNIC_LRO_DEF_COAL_PKT_SIZE);
	if (ret != 0) {
		*fail_feature |= NETIF_F_LRO;
		sss_nic_err(nic_dev, drv, "Fail to set lro %s\n", SSSNIC_FEATURE_OP_STR(en));
		return ret;
	}

	sss_nic_info(nic_dev, drv, "Success to set lro %s\n", SSSNIC_FEATURE_OP_STR(en));

	return 0;
}

static int sss_nic_set_rx_cvlan_feature(struct sss_nic_dev *nic_dev, netdev_features_t old_feature,
					netdev_features_t new_feature,
					netdev_features_t *fail_feature)
{
	int ret;
#ifdef NETIF_F_HW_VLAN_CTAG_RX
	netdev_features_t vlan_feature = NETIF_F_HW_VLAN_CTAG_RX;
#else
	netdev_features_t vlan_feature = NETIF_F_HW_VLAN_RX;
#endif
	bool change = !!((old_feature ^ new_feature) & vlan_feature);
	bool en = !!(new_feature & vlan_feature);

	if (!change)
		return 0;

	ret = sss_nic_set_rx_vlan_offload(nic_dev, en);
	if (ret != 0) {
		*fail_feature |= vlan_feature;
		sss_nic_err(nic_dev, drv, "Fail to set %s rx vlan offload\n",
			    SSSNIC_FEATURE_OP_STR(en));
		return ret;
	}

	sss_nic_info(nic_dev, drv, "Success to set rx vlan offload %s\n",
		     SSSNIC_FEATURE_OP_STR(en));

	return 0;
}

static int sss_nic_set_vlan_filter_feature(struct sss_nic_dev *nic_dev,
					   netdev_features_t old_feature,
					   netdev_features_t new_feature,
					   netdev_features_t *fail_feature)
{
	int ret = 0;
#if defined(NETIF_F_HW_VLAN_CTAG_FILTER)
	netdev_features_t filter_feature = NETIF_F_HW_VLAN_CTAG_FILTER;
#elif defined(NETIF_F_HW_VLAN_FILTER)
	netdev_features_t filter_feature = NETIF_F_HW_VLAN_FILTER;
#endif
	bool change = !!((new_feature ^ old_feature) & filter_feature);
	bool en = !!(new_feature & filter_feature);

	if (!change)
		return 0;

#ifdef NEED_VLAN_RESTORE
	if (en) {
		ret = sss_nic_restore_vlan(nic_dev);
		if (ret != 0) {
			*fail_feature |= filter_feature;
			sss_nic_err(nic_dev, drv,
				    "Fail to set rx vlan filter %s\n", SSSNIC_FEATURE_OP_STR(en));
			return ret;
		}
	}
#endif
	ret = sss_nic_set_vlan_fliter(nic_dev, en);
	if (ret != 0) {
		*fail_feature |= filter_feature;
		sss_nic_err(nic_dev, drv,
			    "Fail to set rx vlan filter %s\n", SSSNIC_FEATURE_OP_STR(en));
		return ret;
	}

	sss_nic_info(nic_dev, drv, "Success to set rx vlan filter %s\n", SSSNIC_FEATURE_OP_STR(en));

	return 0;
}

int sss_nic_set_feature(struct sss_nic_dev *nic_dev, netdev_features_t old_feature,
			netdev_features_t new_feature)
{
	u32 ret = 0;
	netdev_features_t fail_feature = 0;

	ret |= (u32)sss_nic_set_lro_feature(nic_dev, old_feature, new_feature, &fail_feature);
	ret |= (u32)sss_nic_set_rx_cvlan_feature(nic_dev, old_feature, new_feature, &fail_feature);
	ret |= (u32)sss_nic_set_vlan_filter_feature(nic_dev, old_feature,
						    new_feature, &fail_feature);
	if (ret != 0) {
		nic_dev->netdev->features = new_feature ^ fail_feature;
		return -EIO;
	}

	return 0;
}

int sss_nic_enable_netdev_feature(struct sss_nic_dev *nic_dev)
{
	/* enable all feature in netdev->features */
	return sss_nic_set_feature(nic_dev, ~nic_dev->netdev->features, nic_dev->netdev->features);
}

#ifdef IFLA_VF_MAX
int sss_nic_set_hw_vf_vlan(struct sss_nic_dev *nic_dev,
			   u16 cur_vlanprio, int vf_id, u16 vlan_id, u8 qos)
{
	int ret = 0;
	u16 old_vlan = cur_vlanprio & VLAN_VID_MASK;

	if (vlan_id == 0 && qos == 0) {
		ret = sss_nic_destroy_vf_vlan(nic_dev->nic_io, SSSNIC_OS_VF_ID_TO_HW(vf_id));
	} else {
		if (cur_vlanprio != 0) {
			ret = sss_nic_destroy_vf_vlan(nic_dev->nic_io,
						      SSSNIC_OS_VF_ID_TO_HW(vf_id));
			if (ret != 0)
				return ret;
		}
		ret = sss_nic_create_vf_vlan(nic_dev->nic_io, SSSNIC_OS_VF_ID_TO_HW(vf_id),
					     vlan_id, qos);
	}

	ret = sss_nic_update_mac_vlan(nic_dev, old_vlan, vlan_id, SSSNIC_OS_VF_ID_TO_HW(vf_id));
	return ret;
}
#endif

#ifdef HAVE_XDP_SUPPORT
static void sss_nic_put_prog(struct sss_nic_dev *nic_dev, struct bpf_prog *prog)
{
	int i;
	struct bpf_prog *pre_prog = NULL;

	pre_prog = xchg(&nic_dev->xdp_prog, prog);
	for (i = 0; i < nic_dev->max_qp_num; i++)
		xchg(&nic_dev->rq_desc_group[i].xdp_prog, nic_dev->xdp_prog);

	if (pre_prog)
		bpf_prog_put(pre_prog);
}

#ifdef HAVE_NDO_BPF_NETDEV_BPF
int sss_nic_setup_xdp(struct sss_nic_dev *nic_dev, struct netdev_bpf *xdp)
#else
int sss_nic_setup_xdp(struct sss_nic_dev *nic_dev, struct netdev_xdp *xdp)
#endif
{
	struct net_device *netdev = nic_dev->netdev;
	struct netlink_ext_ack *extack = xdp->extack;
	int xdp_max_mtu = SSSNIC_XDP_MAX_MTU(nic_dev);

	if (netdev->mtu > xdp_max_mtu) {
		NL_SET_ERR_MSG_MOD(extack, "Invalid mtu for loading xdp program");
		nicif_err(nic_dev, drv, netdev,
			  "Fail to setup xdp, netdev mtu %d is larger than xdp allowed mtu %d\n",
			  netdev->mtu, xdp_max_mtu);

		return -EINVAL;
	}

	if ((netdev->features & NETIF_F_LRO) != 0) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Fail to setup xdp when LRO is on\n");
		nicif_err(nic_dev, drv, netdev,
			  "Fail to setup xdp when LRO is on\n");

		return -EINVAL;
	}

	sss_nic_put_prog(nic_dev, xdp->prog);

	return 0;
}

void sss_nic_get_tx_stats(struct sss_nic_dev *nic_dev,
			  struct rtnl_link_stats64 *stats)
{
	struct sss_nic_sq_desc *sq_desc = NULL;
	struct sss_nic_sq_stats *sq_stats = NULL;
	unsigned int start;
	int qid;

	stats->tx_bytes = 0;
	stats->tx_packets = 0;
	stats->tx_dropped = 0;

	if (!nic_dev->sq_desc_group)
		return;

	for (qid = 0; qid < nic_dev->max_qp_num; qid++) {
		sq_desc = &nic_dev->sq_desc_group[qid];
		sq_stats = &sq_desc->stats;
		do {
			start = u64_stats_fetch_begin(&sq_stats->stats_sync);
			stats->tx_dropped += sq_stats->tx_dropped;
			stats->tx_packets += sq_stats->tx_packets;
			stats->tx_bytes += sq_stats->tx_bytes;
		} while (u64_stats_fetch_retry(&sq_stats->stats_sync, start));
	}
}

void sss_nic_get_rx_stats(struct sss_nic_dev *nic_dev,
			  struct rtnl_link_stats64 *stats)
{
	struct sss_nic_rq_desc *rq_desc = NULL;
	struct sss_nic_rq_stats *rq_stats = NULL;
	unsigned int start;
	int qid;

	stats->rx_errors = 0;
	stats->rx_dropped = 0;
	stats->rx_packets = 0;
	stats->rx_bytes = 0;

	if (!nic_dev->rq_desc_group)
		return;

	for (qid = 0; qid < nic_dev->max_qp_num; qid++) {
		rq_desc = &nic_dev->rq_desc_group[qid];
		rq_stats = &rq_desc->stats;
		do {
			start = u64_stats_fetch_begin(&rq_stats->stats_sync);
			stats->rx_dropped += rq_stats->rx_dropped;
			stats->rx_errors += rq_stats->csum_errors +
					    rq_stats->other_errors;
			stats->rx_packets += rq_stats->rx_packets;
			stats->rx_bytes += rq_stats->rx_bytes;
		} while (u64_stats_fetch_retry(&rq_stats->stats_sync, start));
	}
}
#endif
