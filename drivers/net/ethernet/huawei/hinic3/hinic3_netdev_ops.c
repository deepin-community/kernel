// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

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

#include "ossl_knl.h"
#if defined(HAVE_NDO_UDP_TUNNEL_ADD) || defined(HAVE_UDP_TUNNEL_NIC_INFO)
#include <net/udp_tunnel.h>
#endif /* HAVE_NDO_UDP_TUNNEL_ADD || HAVE_UDP_TUNNEL_NIC_INFO */
#ifdef HAVE_XDP_SUPPORT
#include <linux/bpf.h>
#endif
#include "hinic3_hw.h"
#include "hinic3_crm.h"
#include "hinic3_nic_io.h"
#include "hinic3_nic_dev.h"
#include "hinic3_srv_nic.h"
#include "hinic3_tx.h"
#include "hinic3_rx.h"
#include "hinic3_dcb.h"
#include "hinic3_nic_prof.h"

#include "nic_npu_cmd.h"

#include "vram_common.h"

#define HINIC3_DEFAULT_RX_CSUM_OFFLOAD	0xFFF

#define HINIC3_LRO_DEFAULT_COAL_PKT_SIZE		32
#define HINIC3_LRO_DEFAULT_TIME_LIMIT			16
#define HINIC3_WAIT_FLUSH_QP_RESOURCE_TIMEOUT		100
static void hinic3_nic_set_rx_mode(struct net_device *netdev)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	if (netdev_uc_count(netdev) != nic_dev->netdev_uc_cnt ||
	    netdev_mc_count(netdev) != nic_dev->netdev_mc_cnt) {
		set_bit(HINIC3_UPDATE_MAC_FILTER, &nic_dev->flags);
		nic_dev->netdev_uc_cnt = netdev_uc_count(netdev);
		nic_dev->netdev_mc_cnt = netdev_mc_count(netdev);
	}

	queue_work(nic_dev->workq, &nic_dev->rx_mode_work);
}

static void hinic3_free_irq_vram(struct hinic3_nic_dev *nic_dev,
				 struct hinic3_dyna_txrxq_params *in_q_params)
{
	u32 size;
	int is_use_vram = get_use_vram_flag();
	struct hinic3_dyna_txrxq_params q_params = nic_dev->q_params;

	if (!q_params.irq_cfg)
		return;

	size = sizeof(struct hinic3_irq) * (q_params.num_qps);

	if (is_use_vram != 0) {
		hi_vram_kfree((void *)q_params.irq_cfg, q_params.irq_cfg_vram_name, size);
		q_params.irq_cfg = NULL;
	} else {
		kfree(in_q_params->irq_cfg);
		in_q_params->irq_cfg = NULL;
	}
}

static int hinic3_alloc_irq_vram(struct hinic3_nic_dev *nic_dev,
				 struct hinic3_dyna_txrxq_params *q_params,
				 bool is_up_eth)
{
	u32 size;
	int is_use_vram = get_use_vram_flag();
	u16 func_id;

	size = sizeof(struct hinic3_irq) * q_params->num_qps;

	if (is_use_vram != 0) {
		func_id = hinic3_global_func_id(nic_dev->hwdev);
		snprintf(q_params->irq_cfg_vram_name, VRAM_NAME_MAX_LEN,
			 "%s%hu", VRAM_NIC_IRQ_VRAM, func_id);
		q_params->irq_cfg = (struct hinic3_irq *)hi_vram_kalloc(q_params->irq_cfg_vram_name,
									size);
		if (!q_params->irq_cfg) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "NIC irq vram alloc failed.\n");
			return -ENOMEM;
		}
		/* in order to clear napi stored in vram, irq need to init when eth up */
		if (is_up_eth)
			memset(q_params->irq_cfg, 0, size);
	} else {
		q_params->irq_cfg = kzalloc(size, GFP_KERNEL);
		if (!q_params->irq_cfg)
			return -ENOMEM;
	}

	return 0;
}

static int hinic3_alloc_txrxq_resources(struct hinic3_nic_dev *nic_dev,
			struct hinic3_dyna_txrxq_params *q_params,
			bool is_up_eth)
{
	u32 size;
	int err;

	size = sizeof(*q_params->txqs_res) * q_params->num_qps;
	q_params->txqs_res = kzalloc(size, GFP_KERNEL);
	if (!q_params->txqs_res) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to alloc txqs resources array\n");
		return -ENOMEM;
	}

	size = sizeof(*q_params->rxqs_res) * q_params->num_qps;
	q_params->rxqs_res = kzalloc(size, GFP_KERNEL);
	if (!q_params->rxqs_res) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to alloc rxqs resource array\n");
		err = -ENOMEM;
		goto alloc_rxqs_res_arr_err;
	}

	err = hinic3_alloc_irq_vram(nic_dev, q_params, is_up_eth);
	if  (err != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Failed to alloc irq resource array\n");
		goto alloc_irq_cfg_err;
	}

	err = hinic3_alloc_txqs_res(nic_dev, q_params->num_qps,
				    q_params->sq_depth, q_params->txqs_res);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to alloc txqs resource\n");
		goto alloc_txqs_res_err;
	}

	err = hinic3_alloc_rxqs_res(nic_dev, q_params->num_qps,
				    q_params->rq_depth, q_params->rxqs_res);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to alloc rxqs resource\n");
		goto alloc_rxqs_res_err;
	}

	return 0;

alloc_rxqs_res_err:
	hinic3_free_txqs_res(nic_dev, q_params->num_qps, q_params->sq_depth,
			     q_params->txqs_res);

alloc_txqs_res_err:
	hinic3_free_irq_vram(nic_dev, q_params);

alloc_irq_cfg_err:
	kfree(q_params->rxqs_res);
	q_params->rxqs_res = NULL;

alloc_rxqs_res_arr_err:
	kfree(q_params->txqs_res);
	q_params->txqs_res = NULL;

	return err;
}

static void hinic3_free_txrxq_resources(struct hinic3_nic_dev *nic_dev,
			struct hinic3_dyna_txrxq_params *q_params)
{
	int is_in_kexec = vram_get_kexec_flag();

	hinic3_free_rxqs_res(nic_dev, q_params->num_qps, q_params->rq_depth,
			     q_params->rxqs_res);
	hinic3_free_txqs_res(nic_dev, q_params->num_qps, q_params->sq_depth,
			     q_params->txqs_res);

	if (is_in_kexec == 0)
		hinic3_free_irq_vram(nic_dev, q_params);

	kfree(q_params->rxqs_res);
	q_params->rxqs_res = NULL;

	kfree(q_params->txqs_res);
	q_params->txqs_res = NULL;
}

static int hinic3_configure_txrxqs(struct hinic3_nic_dev *nic_dev,
				   struct hinic3_dyna_txrxq_params *q_params)
{
	int err;

	err = hinic3_configure_txqs(nic_dev, q_params->num_qps,
				    q_params->sq_depth, q_params->txqs_res);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to configure txqs\n");
		return err;
	}

	err = hinic3_configure_rxqs(nic_dev, q_params->num_qps,
				    q_params->rq_depth, q_params->rxqs_res);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to configure rxqs\n");
		return err;
	}

	return 0;
}

static void config_dcb_qps_map(struct hinic3_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	struct hinic3_dcb *dcb = nic_dev->dcb;
	u8 num_cos;

	if (!test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags)) {
		hinic3_update_tx_db_cos(nic_dev, 0);
		return;
	}

	num_cos = hinic3_get_dev_user_cos_num(nic_dev);
	hinic3_update_qp_cos_cfg(nic_dev, num_cos);
	/* For now, we don't support to change num_cos */
	if (num_cos > dcb->cos_config_num_max ||
	    nic_dev->q_params.num_qps < num_cos) {
		nicif_err(nic_dev, drv, netdev, "Invalid num_cos: %u or num_qps: %u, disable DCB\n",
			  num_cos, nic_dev->q_params.num_qps);
		nic_dev->q_params.num_cos = 0;
		clear_bit(HINIC3_DCB_ENABLE, &nic_dev->flags);
		clear_bit(HINIC3_DCB_ENABLE, &nic_dev->nic_vram->flags);
		/* if we can't enable rss or get enough num_qps,
		 * need to sync default configure to hw
		 */
		hinic3_configure_dcb(netdev);
	}

	hinic3_update_tx_db_cos(nic_dev, 1);
}

static int hinic3_configure(struct hinic3_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	int err;
	int is_in_kexec = vram_get_kexec_flag();

	if (is_in_kexec == 0) {
		err = hinic3_set_port_mtu(nic_dev->hwdev, (u16)netdev->mtu);
		if (err != 0) {
			nicif_err(nic_dev, drv, netdev, "Failed to set mtu\n");
			return err;
		}
	}

	config_dcb_qps_map(nic_dev);

	/* rx rss init */
	err = hinic3_rx_configure(netdev,
				  test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags)
				  ? 1 : 0);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Failed to configure rx\n");
		return err;
	}

	return 0;
}

static void hinic3_remove_configure(struct hinic3_nic_dev *nic_dev)
{
	hinic3_rx_remove_configure(nic_dev->netdev);
}

/* try to modify the number of irq to the target number,
 * and return the actual number of irq.
 */
static u16 hinic3_qp_irq_change(struct hinic3_nic_dev *nic_dev,
				u16 dst_num_qp_irq)
{
	struct irq_info *qps_irq_info = nic_dev->qps_irq_info;
	u16 resp_irq_num, irq_num_gap, i;
	u16 idx;
	int err;

	if (dst_num_qp_irq > nic_dev->num_qp_irq) {
		irq_num_gap = dst_num_qp_irq - nic_dev->num_qp_irq;
		err = hinic3_alloc_irqs(nic_dev->hwdev, SERVICE_T_NIC,
					irq_num_gap,
					&qps_irq_info[nic_dev->num_qp_irq],
					&resp_irq_num);
		if (err) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Failed to alloc irqs\n");
			return nic_dev->num_qp_irq;
		}

		nic_dev->num_qp_irq += resp_irq_num;
	} else if (dst_num_qp_irq < nic_dev->num_qp_irq) {
		irq_num_gap = nic_dev->num_qp_irq - dst_num_qp_irq;
		for (i = 0; i < irq_num_gap; i++) {
			idx = (nic_dev->num_qp_irq - i) - 1;
			hinic3_free_irq(nic_dev->hwdev, SERVICE_T_NIC,
					qps_irq_info[idx].irq_id);
			qps_irq_info[idx].irq_id = 0;
			qps_irq_info[idx].msix_entry_idx = 0;
		}
		nic_dev->num_qp_irq = dst_num_qp_irq;
	}

	return nic_dev->num_qp_irq;
}

static void config_dcb_num_qps(struct hinic3_nic_dev *nic_dev,
			       const struct hinic3_dyna_txrxq_params *q_params,
			       u16 max_qps)
{
	struct hinic3_dcb *dcb = nic_dev->dcb;
	u8 num_cos = q_params->num_cos;
	u8 user_cos_num = hinic3_get_dev_user_cos_num(nic_dev);

	if (!num_cos || num_cos > dcb->cos_config_num_max || num_cos > max_qps)
		return; /* will disable DCB in config_dcb_qps_map() */

	hinic3_update_qp_cos_cfg(nic_dev, user_cos_num);
}

static void hinic3_config_num_qps(struct hinic3_nic_dev *nic_dev,
				  struct hinic3_dyna_txrxq_params *q_params)
{
	u16 alloc_num_irq, cur_num_irq;
	u16 dst_num_irq;

	if (!test_bit(HINIC3_RSS_ENABLE, &nic_dev->flags))
		q_params->num_qps = 1;

	config_dcb_num_qps(nic_dev, q_params, q_params->num_qps);

	if (nic_dev->num_qp_irq >= q_params->num_qps)
		goto out;

	cur_num_irq = nic_dev->num_qp_irq;

	alloc_num_irq = hinic3_qp_irq_change(nic_dev, q_params->num_qps);
	if (alloc_num_irq < q_params->num_qps) {
		q_params->num_qps = alloc_num_irq;
		config_dcb_num_qps(nic_dev, q_params, q_params->num_qps);
		nicif_warn(nic_dev, drv, nic_dev->netdev,
			   "Can not get enough irqs, adjust num_qps to %u\n",
			   q_params->num_qps);

		/* The current irq may be in use, we must keep it */
		dst_num_irq = (u16)max_t(u16, cur_num_irq, q_params->num_qps);
		hinic3_qp_irq_change(nic_dev, dst_num_irq);
	}

out:
	nicif_info(nic_dev, drv, nic_dev->netdev, "Finally num_qps: %u\n",
		   q_params->num_qps);
}

/* determin num_qps from rss_tmpl_id/irq_num/dcb_en */
static int hinic3_setup_num_qps(struct hinic3_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	u32 irq_size;

	nic_dev->num_qp_irq = 0;

	irq_size = sizeof(*nic_dev->qps_irq_info) * nic_dev->max_qps;
	if (!irq_size) {
		nicif_err(nic_dev, drv, netdev, "Cannot allocate zero size entries\n");
		return -EINVAL;
	}
	nic_dev->qps_irq_info = kzalloc(irq_size, GFP_KERNEL);
	if (!nic_dev->qps_irq_info)
		return -ENOMEM;

	hinic3_config_num_qps(nic_dev, &nic_dev->q_params);

	return 0;
}

static void hinic3_destroy_num_qps(struct hinic3_nic_dev *nic_dev)
{
	u16 i;

	for (i = 0; i < nic_dev->num_qp_irq; i++)
		hinic3_free_irq(nic_dev->hwdev, SERVICE_T_NIC,
				nic_dev->qps_irq_info[i].irq_id);

	kfree(nic_dev->qps_irq_info);
}

int hinic3_maybe_set_port_state(struct hinic3_nic_dev *nic_dev, bool enable)
{
	return hinic3_set_port_enable(nic_dev->hwdev, enable,
				      HINIC3_CHANNEL_NIC);
}

static void hinic3_print_link_message(struct hinic3_nic_dev *nic_dev,
				      u8 link_status)
{
	if (nic_dev->link_status == link_status)
		return;

	nic_dev->link_status = link_status;

	nicif_info(nic_dev, link, nic_dev->netdev, "Link is %s\n",
		   (link_status ? "up" : "down"));
}

static int hinic3_alloc_channel_resources(struct hinic3_nic_dev *nic_dev,
			struct hinic3_dyna_qp_params *qp_params,
			struct hinic3_dyna_txrxq_params *trxq_params, bool is_up_eth)
{
	int err;

	qp_params->num_qps = trxq_params->num_qps;
	qp_params->sq_depth = trxq_params->sq_depth;
	qp_params->rq_depth = trxq_params->rq_depth;

	err = hinic3_alloc_qps(nic_dev->hwdev, nic_dev->qps_irq_info,
			       qp_params);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Failed to alloc qps\n");
		return err;
	}

	err = hinic3_alloc_txrxq_resources(nic_dev, trxq_params, is_up_eth);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Failed to alloc txrxq resources\n");
		hinic3_free_qps(nic_dev->hwdev, qp_params);
		return err;
	}

	return 0;
}

static void hinic3_free_channel_resources(struct hinic3_nic_dev *nic_dev,
			struct hinic3_dyna_qp_params *qp_params,
			struct hinic3_dyna_txrxq_params *trxq_params)
{
	mutex_lock(&nic_dev->nic_mutex);
	hinic3_free_txrxq_resources(nic_dev, trxq_params);
	hinic3_free_qps(nic_dev->hwdev, qp_params);
	mutex_unlock(&nic_dev->nic_mutex);
}

static int hinic3_open_channel(struct hinic3_nic_dev *nic_dev,
			       struct hinic3_dyna_qp_params *qp_params,
			       struct hinic3_dyna_txrxq_params *trxq_params)
{
	int err;

	err = hinic3_init_qps(nic_dev->hwdev, qp_params);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Failed to init qps\n");
		return err;
	}

	err = hinic3_configure_txrxqs(nic_dev, trxq_params);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Failed to configure txrxqs\n");
		goto cfg_txrxqs_err;
	}

	err = hinic3_qps_irq_init(nic_dev);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Failed to init txrxq irq\n");
		goto init_qp_irq_err;
	}

	err = hinic3_configure(nic_dev);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Failed to init txrxq irq\n");
		goto configure_err;
	}

	return 0;

configure_err:
	hinic3_qps_irq_deinit(nic_dev);

init_qp_irq_err:
cfg_txrxqs_err:
	hinic3_deinit_qps(nic_dev->hwdev, qp_params);

	return err;
}

static void hinic3_close_channel(struct hinic3_nic_dev *nic_dev,
				 struct hinic3_dyna_qp_params *qp_params)
{
	hinic3_remove_configure(nic_dev);
	hinic3_qps_irq_deinit(nic_dev);
	hinic3_deinit_qps(nic_dev->hwdev, qp_params);
}

int hinic3_vport_up(struct hinic3_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	u8 link_status = 0;
	u16 glb_func_id;
	int err;

	glb_func_id = hinic3_global_func_id(nic_dev->hwdev);
	err = hinic3_set_vport_enable(nic_dev->hwdev, glb_func_id, true,
				      HINIC3_CHANNEL_NIC);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Failed to enable vport\n");
		goto vport_enable_err;
	}

	err = hinic3_maybe_set_port_state(nic_dev, true);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Failed to enable port\n");
		goto port_enable_err;
	}

	netif_set_real_num_tx_queues(netdev, nic_dev->q_params.num_qps);
	netif_set_real_num_rx_queues(netdev, nic_dev->q_params.num_qps);
	netif_tx_wake_all_queues(netdev);

	if (test_bit(HINIC3_FORCE_LINK_UP, &nic_dev->flags)) {
		link_status = true;
		netif_carrier_on(netdev);
	} else {
		err = hinic3_get_link_state(nic_dev->hwdev, &link_status);
		if (!err && link_status)
			netif_carrier_on(netdev);
	}

	queue_delayed_work(nic_dev->workq, &nic_dev->moderation_task,
			   HINIC3_MODERATONE_DELAY);
	if (test_bit(HINIC3_RXQ_RECOVERY, &nic_dev->flags))
		queue_delayed_work(nic_dev->workq,
				   &nic_dev->rxq_check_work, HZ);

	hinic3_print_link_message(nic_dev, link_status);

	if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev))
		hinic3_notify_all_vfs_link_changed(nic_dev->hwdev, link_status);

	return 0;

port_enable_err:
	hinic3_set_vport_enable(nic_dev->hwdev, glb_func_id, false,
				HINIC3_CHANNEL_NIC);

vport_enable_err:
	hinic3_flush_qps_res(nic_dev->hwdev);
	/* After set vport disable 100ms, no packets will be send to host */
	msleep(100);

	return err;
}

static int hinic3_flush_rq_and_check(struct hinic3_nic_dev *nic_dev, u16 glb_func_id)
{
	struct hinic3_flush_rq *rq_flush_msg = NULL;
	struct hinic3_cmd_buf *cmd_buf = NULL;
	int out_buf_len = sizeof(struct hinic3_flush_rq);
	u16 rq_id;
	u64 out_param = 0;
	int ret;

	cmd_buf = hinic3_alloc_cmd_buf(nic_dev->hwdev);
	if (!cmd_buf) {
		nic_err(&nic_dev->pdev->dev, "Failed to allocate cmd buf\n");
		return -ENOMEM;
	}

	cmd_buf->size = sizeof(struct hinic3_flush_rq);
	rq_flush_msg = (struct hinic3_flush_rq *)cmd_buf->buf;
	rq_flush_msg->dw.bs.func_id = glb_func_id;
	for (rq_id = 0; rq_id < nic_dev->q_params.num_qps; rq_id++) {
		rq_flush_msg->dw.bs.rq_id = rq_id;
		hinic3_cpu_to_be32(rq_flush_msg, out_buf_len);
		ret = hinic3_cmdq_direct_resp(nic_dev->hwdev, HINIC3_MOD_L2NIC,
					      HINIC3_UCODE_CHK_RQ_STOP,
					      cmd_buf, &out_param, 0,
					      HINIC3_CHANNEL_NIC);
		if (ret != 0 || out_param != 0) {
			nic_err(&nic_dev->pdev->dev, "Failed to flush rq, ret:%d, func:%u, rq:%u\n",
				ret, glb_func_id, rq_id);
			goto err;
		}
		hinic3_be32_to_cpu(rq_flush_msg, out_buf_len);
	}

	nic_info(&nic_dev->pdev->dev, "Func:%u rq_num:%u flush rq success\n",
		 glb_func_id, nic_dev->q_params.num_qps);
	hinic3_free_cmd_buf(nic_dev->hwdev, cmd_buf);
	return 0;
err:
	hinic3_free_cmd_buf(nic_dev->hwdev, cmd_buf);
	return -1;
}

void hinic3_vport_down(struct hinic3_nic_dev *nic_dev)
{
	u16 glb_func_id;
	int is_in_kexec = vram_get_kexec_flag();

	netif_carrier_off(nic_dev->netdev);
	netif_tx_disable(nic_dev->netdev);

	cancel_delayed_work_sync(&nic_dev->rxq_check_work);

	cancel_delayed_work_sync(&nic_dev->moderation_task);

	if (hinic3_get_chip_present_flag(nic_dev->hwdev)) {
		if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev))
			hinic3_notify_all_vfs_link_changed(nic_dev->hwdev, 0);

		if (is_in_kexec != 0)
			nicif_info(nic_dev, drv, nic_dev->netdev, "Skip changing mag status!\n");
		else
			hinic3_maybe_set_port_state(nic_dev, false);

		glb_func_id = hinic3_global_func_id(nic_dev->hwdev);
		hinic3_set_vport_enable(nic_dev->hwdev, glb_func_id, false,
					HINIC3_CHANNEL_NIC);

		hinic3_flush_txqs(nic_dev->netdev);

		if (is_in_kexec == 0)
			msleep(HINIC3_WAIT_FLUSH_QP_RESOURCE_TIMEOUT);
		else
			(void)hinic3_flush_rq_and_check(nic_dev, glb_func_id);
		hinic3_flush_qps_res(nic_dev->hwdev);
	}
}

int hinic3_change_channel_settings(struct hinic3_nic_dev *nic_dev,
				   struct hinic3_dyna_txrxq_params *trxq_params,
				   hinic3_reopen_handler reopen_handler,
				   const void *priv_data)
{
	struct hinic3_dyna_qp_params new_qp_params = {0};
	struct hinic3_dyna_qp_params cur_qp_params = {0};
	int err;
	bool is_free_resources = false;

	hinic3_config_num_qps(nic_dev, trxq_params);

	err = hinic3_alloc_channel_resources(nic_dev, &new_qp_params,
					     trxq_params, false);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to alloc channel resources\n");
		return err;
	}

	if (!test_and_set_bit(HINIC3_CHANGE_RES_INVALID, &nic_dev->flags)) {
		hinic3_vport_down(nic_dev);
		hinic3_close_channel(nic_dev, &cur_qp_params);
		hinic3_free_channel_resources(nic_dev, &cur_qp_params,
					      &nic_dev->q_params);
		is_free_resources = true;
	}

	if (nic_dev->num_qp_irq > trxq_params->num_qps)
		hinic3_qp_irq_change(nic_dev, trxq_params->num_qps);

	if (is_free_resources) {
		err = hinic3_alloc_irq_vram(nic_dev, trxq_params, false);
		if (err != 0) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Change chl alloc irq failed\n");
			goto alloc_irq_err;
		}
	}
	nic_dev->q_params = *trxq_params;

	if (reopen_handler)
		reopen_handler(nic_dev, priv_data);

	err = hinic3_open_channel(nic_dev, &new_qp_params, trxq_params);
	if (err)
		goto open_channel_err;

	err = hinic3_vport_up(nic_dev);
	if (err)
		goto vport_up_err;

	clear_bit(HINIC3_CHANGE_RES_INVALID, &nic_dev->flags);
	nicif_info(nic_dev, drv, nic_dev->netdev, "Change channel settings success\n");

	return 0;

vport_up_err:
	hinic3_close_channel(nic_dev, &new_qp_params);
alloc_irq_err:
open_channel_err:
	hinic3_free_channel_resources(nic_dev, &new_qp_params, trxq_params);

	return err;
}

int hinic3_open(struct net_device *netdev)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct hinic3_dyna_qp_params qp_params = {0};
	int err;

	if (test_bit(HINIC3_INTF_UP, &nic_dev->flags)) {
		nicif_info(nic_dev, drv, netdev, "Netdev already open, do nothing\n");
		return 0;
	}

	err = hinic3_init_nicio_res(nic_dev->hwdev);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Failed to init nicio resources\n");
		return err;
	}

	err = hinic3_setup_num_qps(nic_dev);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Failed to setup num_qps\n");
		goto setup_qps_err;
	}

	err = hinic3_alloc_channel_resources(nic_dev, &qp_params,
					     &nic_dev->q_params, true);
	if (err)
		goto alloc_channel_res_err;

	err = hinic3_open_channel(nic_dev, &qp_params, &nic_dev->q_params);
	if (err)
		goto open_channel_err;

	err = hinic3_vport_up(nic_dev);
	if (err)
		goto vport_up_err;

	err = hinic3_set_master_dev_state(nic_dev, true);
	if (err)
		goto set_master_dev_err;

	set_bit(HINIC3_INTF_UP, &nic_dev->flags);
	nicif_info(nic_dev, drv, nic_dev->netdev, "Netdev is up\n");

	return 0;

set_master_dev_err:
	hinic3_vport_down(nic_dev);

vport_up_err:
	hinic3_close_channel(nic_dev, &qp_params);

open_channel_err:
	hinic3_free_channel_resources(nic_dev, &qp_params, &nic_dev->q_params);

alloc_channel_res_err:
	hinic3_destroy_num_qps(nic_dev);

setup_qps_err:
	hinic3_deinit_nicio_res(nic_dev->hwdev);

	return err;
}

static void hinic3_delete_napi(struct hinic3_nic_dev *nic_dev)
{
	u16 q_id;
	int is_in_kexec = vram_get_kexec_flag();
	struct hinic3_irq *irq_cfg = NULL;

	if (is_in_kexec == 0 || !nic_dev->q_params.irq_cfg)
		return;

	for (q_id = 0; q_id < nic_dev->q_params.num_qps; q_id++) {
		irq_cfg = &nic_dev->q_params.irq_cfg[q_id];
		qp_del_napi(irq_cfg);
	}

	hinic3_free_irq_vram(nic_dev, &nic_dev->q_params);
}

int hinic3_close(struct net_device *netdev)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct hinic3_dyna_qp_params qp_params = {0};

	if (!test_and_clear_bit(HINIC3_INTF_UP, &nic_dev->flags)) {
		/* delete napi in os hotreplace rollback */
		hinic3_delete_napi(nic_dev);
		nicif_info(nic_dev, drv, netdev, "Netdev already close, do nothing\n");
		return 0;
	}

	if (test_and_clear_bit(HINIC3_CHANGE_RES_INVALID, &nic_dev->flags))
		goto out;

	hinic3_set_master_dev_state(nic_dev, false);

	hinic3_vport_down(nic_dev);
	hinic3_close_channel(nic_dev, &qp_params);
	hinic3_free_channel_resources(nic_dev, &qp_params, &nic_dev->q_params);

out:
	hinic3_deinit_nicio_res(nic_dev->hwdev);
	hinic3_destroy_num_qps(nic_dev);

	nicif_info(nic_dev, drv, nic_dev->netdev, "Netdev is down\n");

	return 0;
}

#define IPV6_ADDR_LEN  4
#define PKT_INFO_LEN   9
#define BITS_PER_TUPLE 32
static u32 calc_xor_rss(u8 *rss_tunple, u32 len)
{
	u32 hash_value;
	u32 i;

	hash_value = rss_tunple[0];
	for (i = 1; i < len; i++)
		hash_value = hash_value ^ rss_tunple[i];

	return hash_value;
}

static u32 calc_toep_rss(const u32 *rss_tunple, u32 len, const u32 *rss_key)
{
	u32 rss = 0;
	u32 i, j;

	for (i = 1; i <= len; i++) {
		for (j = 0; j < BITS_PER_TUPLE; j++)
			if (rss_tunple[i - 1] & ((u32)1 <<
			    (u32)((BITS_PER_TUPLE - 1) - j)))
				rss ^= (rss_key[i - 1] << j) |
					(u32)((u64)rss_key[i] >>
					(BITS_PER_TUPLE - j));
	}

	return rss;
}

#define RSS_VAL(val, type)		\
	(((type) == HINIC3_RSS_HASH_ENGINE_TYPE_TOEP) ? ntohl(val) : (val))

static u8 parse_ipv6_info(struct sk_buff *skb, u32 *rss_tunple,
			  u8 hash_engine, u32 *len)
{
	struct ipv6hdr *ipv6hdr = ipv6_hdr(skb);
	u32 *saddr = (u32 *)&ipv6hdr->saddr;
	u32 *daddr = (u32 *)&ipv6hdr->daddr;
	u8 i;

	for (i = 0; i < IPV6_ADDR_LEN; i++) {
		rss_tunple[i] = RSS_VAL(daddr[i], hash_engine);
		/* The offset of the sport relative to the dport is 4 */
		rss_tunple[(u32)(i + IPV6_ADDR_LEN)] =
			RSS_VAL(saddr[i], hash_engine);
	}
	*len = IPV6_ADDR_LEN + IPV6_ADDR_LEN;

	if (skb_network_header(skb) + sizeof(*ipv6hdr) ==
	    skb_transport_header(skb))
		return ipv6hdr->nexthdr;
	return 0;
}

static u16 select_queue_by_hash_func(struct net_device *dev,
				     struct sk_buff *skb,
				     unsigned int num_tx_queues)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(dev);
	struct nic_rss_type rss_type = nic_dev->rss_type;
	struct iphdr *iphdr = NULL;
	u32 rss_tunple[PKT_INFO_LEN] = {0};
	u32 len = 0;
	u32 hash = 0;
	u8 hash_engine = nic_dev->rss_hash_engine;
	u8 l4_proto;
	unsigned char *l4_hdr = NULL;

	if (skb_rx_queue_recorded(skb)) {
		hash = skb_get_rx_queue(skb);
		if (unlikely(hash >= num_tx_queues))
			hash %= num_tx_queues;

		return (u16)hash;
	}

	iphdr = ip_hdr(skb);
	if (iphdr->version == IPV4_VERSION) {
		rss_tunple[len++] = RSS_VAL(iphdr->daddr, hash_engine);
		rss_tunple[len++] = RSS_VAL(iphdr->saddr, hash_engine);
		l4_proto = iphdr->protocol;
	} else if (iphdr->version == IPV6_VERSION) {
		l4_proto = parse_ipv6_info(skb, (u32 *)rss_tunple,
					   hash_engine, &len);
	} else {
		return (u16)hash;
	}

	if ((iphdr->version == IPV4_VERSION &&
	     ((l4_proto == IPPROTO_UDP && rss_type.udp_ipv4) ||
	     (l4_proto == IPPROTO_TCP && rss_type.tcp_ipv4))) ||
	    (iphdr->version == IPV6_VERSION &&
	     ((l4_proto == IPPROTO_UDP && rss_type.udp_ipv6) ||
	     (l4_proto == IPPROTO_TCP && rss_type.tcp_ipv6)))) {
		l4_hdr = skb_transport_header(skb);
		/* High 16 bits are dport, low 16 bits are sport. */
		rss_tunple[len++] = ((u32)ntohs(*((u16 *)l4_hdr + 1U)) << 16) |
			ntohs(*(u16 *)l4_hdr);
	} /* rss_type.ipv4 and rss_type.ipv6 default on. */

	if (hash_engine == HINIC3_RSS_HASH_ENGINE_TYPE_TOEP)
		hash = calc_toep_rss((u32 *)rss_tunple, len,
				     nic_dev->rss_hkey_be);
	else
		hash = calc_xor_rss((u8 *)rss_tunple, len * (u32)sizeof(u32));

	return (u16)nic_dev->rss_indir[hash & 0xFF];
}

#define GET_DSCP_PRI_OFFSET 2
static u8 hinic3_get_dscp_up(struct hinic3_nic_dev *nic_dev,
			     struct sk_buff *skb)
{
	struct hinic3_dcb *dcb = nic_dev->dcb;
	int dscp_cp;

	if (skb->protocol == htons(ETH_P_IP))
		dscp_cp = ipv4_get_dsfield(ip_hdr(skb)) >> GET_DSCP_PRI_OFFSET;
	else if (skb->protocol == htons(ETH_P_IPV6))
		dscp_cp = ipv6_get_dsfield(ipv6_hdr(skb)) >>
					GET_DSCP_PRI_OFFSET;
	else
		return dcb->hw_dcb_cfg.default_cos;
	return dcb->hw_dcb_cfg.dscp2cos[dscp_cp];
}

#if defined(HAVE_NDO_SELECT_QUEUE_SB_DEV_ONLY)
static u16 hinic3_select_queue(struct net_device *netdev, struct sk_buff *skb,
			       struct net_device *sb_dev)
#elif defined(HAVE_NDO_SELECT_QUEUE_ACCEL_FALLBACK)
#if defined(HAVE_NDO_SELECT_QUEUE_SB_DEV)
static u16 hinic3_select_queue(struct net_device *netdev, struct sk_buff *skb,
			       struct net_device *sb_dev,
			       select_queue_fallback_t fallback)
#else
static u16 hinic3_select_queue(struct net_device *netdev, struct sk_buff *skb,
			       __always_unused void *accel,
			       select_queue_fallback_t fallback)
#endif

#elif defined(HAVE_NDO_SELECT_QUEUE_ACCEL)
static u16 hinic3_select_queue(struct net_device *netdev, struct sk_buff *skb,
			       __always_unused void *accel)

#else
static u16 hinic3_select_queue(struct net_device *netdev, struct sk_buff *skb)
#endif /* end of HAVE_NDO_SELECT_QUEUE_ACCEL_FALLBACK */
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct hinic3_dcb *dcb = nic_dev->dcb;
	u16 txq;
	u8 cos, qp_num;

	if (test_bit(HINIC3_SAME_RXTX, &nic_dev->flags))
		return select_queue_by_hash_func(netdev, skb,
						 netdev->real_num_tx_queues);

	txq =
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

	if (test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags)) {
		if (dcb->hw_dcb_cfg.trust == HINIC3_DCB_PCP) {
			if (skb->vlan_tci)
				cos = dcb->hw_dcb_cfg.pcp2cos[skb->vlan_tci >>
						VLAN_PRIO_SHIFT];
			else
				cos = dcb->hw_dcb_cfg.default_cos;
		} else {
			cos = hinic3_get_dscp_up(nic_dev, skb);
		}

		qp_num = dcb->hw_dcb_cfg.cos_qp_num[cos] ?
			txq % dcb->hw_dcb_cfg.cos_qp_num[cos] : 0;
		txq = dcb->hw_dcb_cfg.cos_qp_offset[cos] + qp_num;
	}

	return txq;
}

#ifdef HAVE_NDO_GET_STATS64
#ifdef HAVE_VOID_NDO_GET_STATS64
static void hinic3_get_stats64(struct net_device *netdev,
			       struct rtnl_link_stats64 *stats)
#else
static struct rtnl_link_stats64
	*hinic3_get_stats64(struct net_device *netdev,
			    struct rtnl_link_stats64 *stats)
#endif

#else /* !HAVE_NDO_GET_STATS64 */
static struct net_device_stats *hinic3_get_stats(struct net_device *netdev)
#endif
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
#ifndef HAVE_NDO_GET_STATS64
#ifdef HAVE_NETDEV_STATS_IN_NETDEV
	struct net_device_stats *stats = &netdev->stats;
#else
	struct net_device_stats *stats = &nic_dev->net_stats;
#endif /* HAVE_NETDEV_STATS_IN_NETDEV */
#endif /* HAVE_NDO_GET_STATS64 */
	struct hinic3_txq_stats *txq_stats = NULL;
	struct hinic3_rxq_stats *rxq_stats = NULL;
	struct hinic3_txq *txq = NULL;
	struct hinic3_rxq *rxq = NULL;
	u64 bytes, packets, dropped, errors;
	unsigned int start;
	int i;

	bytes = 0;
	packets = 0;
	dropped = 0;
	for (i = 0; i < nic_dev->max_qps; i++) {
		if (!nic_dev->txqs)
			break;

		txq = &nic_dev->txqs[i];
		txq_stats = &txq->txq_stats;
		do {
			start = u64_stats_fetch_begin(&txq_stats->syncp);
			bytes += txq_stats->bytes;
			packets += txq_stats->packets;
			dropped += txq_stats->dropped;
		} while (u64_stats_fetch_retry(&txq_stats->syncp, start));
	}
	stats->tx_packets = packets;
	stats->tx_bytes   = bytes;
	stats->tx_dropped = dropped;

	bytes = 0;
	packets = 0;
	errors = 0;
	dropped = 0;
	for (i = 0; i < nic_dev->max_qps; i++) {
		if (!nic_dev->rxqs)
			break;

		rxq = &nic_dev->rxqs[i];
		rxq_stats = &rxq->rxq_stats;
		do {
			start = u64_stats_fetch_begin(&rxq_stats->syncp);
			bytes += rxq_stats->bytes;
			packets += rxq_stats->packets;
			errors += rxq_stats->csum_errors +
				rxq_stats->other_errors;
			dropped += rxq_stats->dropped;
		} while (u64_stats_fetch_retry(&rxq_stats->syncp, start));
	}
	stats->rx_packets = packets;
	stats->rx_bytes   = bytes;
	stats->rx_errors  = errors;
	stats->rx_dropped = dropped + nic_dev->vport_stats.rx_discard_vport;

#ifndef HAVE_VOID_NDO_GET_STATS64
	return stats;
#endif
}

#ifdef HAVE_NDO_TX_TIMEOUT_TXQ
static void hinic3_tx_timeout(struct net_device *netdev, unsigned int txqueue)
#else
static void hinic3_tx_timeout(struct net_device *netdev)
#endif
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct hinic3_io_queue *sq = NULL;
	bool hw_err = false;
	u32 sw_pi, hw_ci;
	u8 q_id;

	HINIC3_NIC_STATS_INC(nic_dev, netdev_tx_timeout);
	nicif_err(nic_dev, drv, netdev, "Tx timeout\n");

	for (q_id = 0; q_id < nic_dev->q_params.num_qps; q_id++) {
		if (!netif_xmit_stopped(netdev_get_tx_queue(netdev, q_id)))
			continue;

		sq = nic_dev->txqs[q_id].sq;
		sw_pi = hinic3_get_sq_local_pi(sq);
		hw_ci = hinic3_get_sq_hw_ci(sq);
		nicif_info(nic_dev, drv, netdev,
			   "txq%u: sw_pi: %u, hw_ci: %u, sw_ci: %u, napi->state: 0x%lx.\n",
			   q_id, sw_pi, hw_ci, hinic3_get_sq_local_ci(sq),
			   nic_dev->q_params.irq_cfg[q_id].napi.state);

		if (sw_pi != hw_ci)
			hw_err = true;
	}

	if (hw_err)
		set_bit(EVENT_WORK_TX_TIMEOUT, &nic_dev->event_flag);
}

static int hinic3_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	u32 mtu = (u32)new_mtu;
	int err = 0;
	int is_in_kexec = vram_get_kexec_flag();

#ifdef HAVE_XDP_SUPPORT
	u32 xdp_max_mtu;
#endif

	if (is_in_kexec != 0) {
		nicif_info(nic_dev, drv, netdev, "Hotreplace skip change mtu\n");
		return err;
	}

#ifdef HAVE_XDP_SUPPORT
	if (hinic3_is_xdp_enable(nic_dev)) {
		xdp_max_mtu = hinic3_xdp_max_mtu(nic_dev);
		if (mtu > xdp_max_mtu) {
			nicif_err(nic_dev, drv, netdev,
				  "Max MTU for xdp usage is %d\n", xdp_max_mtu);
			return -EINVAL;
		}
	}
#endif

	err = hinic3_config_port_mtu(nic_dev, mtu);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Failed to change port mtu to %d\n",
			  new_mtu);
	} else {
		nicif_info(nic_dev, drv, nic_dev->netdev, "Change mtu from %u to %d\n",
			   netdev->mtu, new_mtu);
		netdev->mtu = mtu;
		nic_dev->nic_vram->vram_mtu = mtu;
	}

	return err;
}

static int hinic3_set_mac_addr(struct net_device *netdev, void *addr)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct sockaddr *saddr = addr;
	int err;

	if (!is_valid_ether_addr(saddr->sa_data))
		return -EADDRNOTAVAIL;

	if (ether_addr_equal(netdev->dev_addr, saddr->sa_data)) {
		nicif_info(nic_dev, drv, netdev,
			   "Already using mac address %pM\n",
			   saddr->sa_data);
		return 0;
	}

	err = hinic3_config_port_mac(nic_dev, saddr);
	if (err)
		return err;

	eth_hw_addr_set(netdev, saddr->sa_data);

	nicif_info(nic_dev, drv, netdev, "Set new mac address %pM\n",
		   saddr->sa_data);

	return 0;
}

#if defined(HAVE_NDO_UDP_TUNNEL_ADD) || defined(HAVE_UDP_TUNNEL_NIC_INFO)
static int hinic3_udp_tunnel_port_config(struct net_device *netdev,
					 struct udp_tunnel_info *ti,
					 u8 action)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	u16 func_id = hinic3_global_func_id(nic_dev->hwdev);
	u16 dst_port;
	int ret = 0;

	switch (ti->type) {
	case UDP_TUNNEL_TYPE_VXLAN:
		dst_port = ntohs(ti->port);
		ret = hinic3_vlxan_port_config(nic_dev->hwdev,
					       func_id, dst_port, action);
		if (ret != 0) {
			nicif_warn(nic_dev, drv, netdev,
				   "Failed to set vxlan port %u to device\n",
				   dst_port);
		break;
		}
		nicif_info(nic_dev, link, netdev, "Vxlan dst port set to %u\n",
			   action == HINIC3_CMD_OP_ADD ?
			   dst_port : ntohs(VXLAN_OFFLOAD_PORT_LE));
		break;
	default:
		nicif_err(nic_dev, drv, netdev, "Failed to add port, only vxlan dst port is supported\n");
		ret = -EINVAL;
	}
	return ret;
}
#endif /* HAVE_NDO_UDP_TUNNEL_ADD || HAVE_UDP_TUNNEL_NIC_INFO */
#ifdef HAVE_NDO_UDP_TUNNEL_ADD
static void hinic3_udp_tunnel_add(struct net_device *netdev,
				  struct udp_tunnel_info *ti)
{
	if (ti->sa_family != AF_INET && ti->sa_family != AF_INET6)
		return;

	hinic3_udp_tunnel_port_config(netdev, ti, HINIC3_CMD_OP_ADD);
}

static void hinic3_udp_tunnel_del(struct net_device *netdev,
				  struct udp_tunnel_info *ti)
{
	if (ti->sa_family != AF_INET && ti->sa_family != AF_INET6)
		return;

	hinic3_udp_tunnel_port_config(netdev, ti, HINIC3_CMD_OP_DEL);
}
#endif /* HAVE_NDO_UDP_TUNNEL_ADD */

#ifdef HAVE_UDP_TUNNEL_NIC_INFO
int hinic3_udp_tunnel_set_port(struct net_device *netdev,
			       __always_unused unsigned int table,
			       __always_unused unsigned int entry,
			       struct udp_tunnel_info *ti)
{
	return hinic3_udp_tunnel_port_config(netdev, ti, HINIC3_CMD_OP_ADD);
}

int hinic3_udp_tunnel_unset_port(struct net_device *netdev,
				 __always_unused unsigned int table,
				 __always_unused unsigned int entry,
				 struct udp_tunnel_info *ti)
{
	return hinic3_udp_tunnel_port_config(netdev, ti, HINIC3_CMD_OP_DEL);
}
#endif /* HAVE_UDP_TUNNEL_NIC_INFO */

static int
hinic3_vlan_rx_add_vid(struct net_device *netdev,
		       __always_unused __be16 proto,
		       u16 vid)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	unsigned long *vlan_bitmap = nic_dev->vlan_bitmap;
	u16 func_id;
	u32 col, line;
	int err = 0;

	/* VLAN 0 donot be added, which is the same as VLAN 0 deleted. */
	if (vid == 0)
		goto end;

	col = VID_COL(nic_dev, vid);
	line = VID_LINE(nic_dev, vid);

	func_id = hinic3_global_func_id(nic_dev->hwdev);

	err = hinic3_add_vlan(nic_dev->hwdev, vid, func_id);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Failed to add vlan %u\n", vid);
		goto end;
	}

	set_bit(col, &vlan_bitmap[line]);

	nicif_info(nic_dev, drv, netdev, "Add vlan %u\n", vid);

end:
	return err;
}

static int
hinic3_vlan_rx_kill_vid(struct net_device *netdev,
			__always_unused __be16 proto,
			u16 vid)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	unsigned long *vlan_bitmap = nic_dev->vlan_bitmap;
	u16 func_id;
	int col, line;
	int err = 0;

	col  = VID_COL(nic_dev, vid);
	line = (int)VID_LINE(nic_dev, vid);

	/* In the broadcast scenario, ucode finds the corresponding function
	 * based on VLAN 0 of vlan table. If we delete VLAN 0, the VLAN function
	 * is affected.
	 */
	if (vid == 0)
		goto end;

	func_id = hinic3_global_func_id(nic_dev->hwdev);
	err = hinic3_del_vlan(nic_dev->hwdev, vid, func_id);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Failed to delete vlan\n");
		goto end;
	}

	clear_bit(col, &vlan_bitmap[line]);

	nicif_info(nic_dev, drv, netdev, "Remove vlan %u\n", vid);

end:
	return err;
}

#ifdef NEED_VLAN_RESTORE
static int hinic3_vlan_restore(struct net_device *netdev)
{
	int err = 0;
#if defined(CONFIG_VLAN_8021Q) || defined(CONFIG_VLAN_8021Q_MODULE)
	struct net_device *vlandev = NULL;
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	unsigned long *vlan_bitmap = nic_dev->vlan_bitmap;
	u32 col, line;
	u16 i;

	if (!netdev->netdev_ops->ndo_vlan_rx_add_vid)
		return -EFAULT;
	rcu_read_lock();
	for (i = 0; i < VLAN_N_VID; i++) {
#ifdef HAVE_VLAN_FIND_DEV_DEEP_RCU
		vlandev =
			__vlan_find_dev_deep_rcu(netdev, htons(ETH_P_8021Q), i);
#else
		vlandev = __vlan_find_dev_deep(netdev, htons(ETH_P_8021Q), i);
#endif
		col = VID_COL(nic_dev, i);
		line = VID_LINE(nic_dev, i);
		if (!vlandev && (vlan_bitmap[line] & (1UL << col)) != 0) {
			err = netdev->netdev_ops->ndo_vlan_rx_kill_vid(netdev,
				htons(ETH_P_8021Q), i);
			if (err) {
				hinic3_err(nic_dev, drv, "delete vlan %u failed, err code %d\n",
					   i, err);
				break;
			}
		} else if (vlandev && (vlan_bitmap[line] & (1UL << col)) == 0) {
			err = netdev->netdev_ops->ndo_vlan_rx_add_vid(netdev,
				htons(ETH_P_8021Q), i);
			if (err) {
				hinic3_err(nic_dev, drv, "restore vlan %u failed, err code %d\n",
					   i, err);
				break;
			}
		}
	}
	rcu_read_unlock();
#endif

	return err;
}
#endif

#define SET_FEATURES_OP_STR(op)		((op) ? "Enable" : "Disable")

static int set_feature_rx_csum(struct hinic3_nic_dev *nic_dev,
			       netdev_features_t wanted_features,
			       netdev_features_t features,
			       netdev_features_t *failed_features)
{
	netdev_features_t changed = wanted_features ^ features;

	if (changed & NETIF_F_RXCSUM)
		hinic3_info(nic_dev, drv, "%s rx csum success\n",
			    SET_FEATURES_OP_STR(wanted_features &
						NETIF_F_RXCSUM));

	return 0;
}

static int set_feature_tso(struct hinic3_nic_dev *nic_dev,
			   netdev_features_t wanted_features,
			   netdev_features_t features,
			   netdev_features_t *failed_features)
{
	netdev_features_t changed = wanted_features ^ features;

	if (changed & NETIF_F_TSO)
		hinic3_info(nic_dev, drv, "%s tso success\n",
			    SET_FEATURES_OP_STR(wanted_features & NETIF_F_TSO));

	return 0;
}

#ifdef NETIF_F_UFO
static int set_feature_ufo(struct hinic3_nic_dev *nic_dev,
			   netdev_features_t wanted_features,
			   netdev_features_t features,
			   netdev_features_t *failed_features)
{
	netdev_features_t changed = wanted_features ^ features;

	if (changed & NETIF_F_UFO)
		hinic3_info(nic_dev, drv, "%s ufo success\n",
			    SET_FEATURES_OP_STR(wanted_features & NETIF_F_UFO));

	return 0;
}
#endif

static int set_feature_lro(struct hinic3_nic_dev *nic_dev,
			   netdev_features_t wanted_features,
			   netdev_features_t features,
			   netdev_features_t *failed_features)
{
	netdev_features_t changed = wanted_features ^ features;
	bool en = !!(wanted_features & NETIF_F_LRO);
	int err;

	if (!(changed & NETIF_F_LRO))
		return 0;

#ifdef HAVE_XDP_SUPPORT
	if (en && hinic3_is_xdp_enable(nic_dev)) {
		hinic3_err(nic_dev, drv, "Can not enable LRO when xdp is enable\n");
		*failed_features |= NETIF_F_LRO;
		return -EINVAL;
	}
#endif

	err = hinic3_set_rx_lro_state(nic_dev->hwdev, en,
				      HINIC3_LRO_DEFAULT_TIME_LIMIT,
				      HINIC3_LRO_DEFAULT_COAL_PKT_SIZE);
	if (err) {
		hinic3_err(nic_dev, drv, "%s lro failed\n",
			   SET_FEATURES_OP_STR(en));
		*failed_features |= NETIF_F_LRO;
	} else {
		hinic3_info(nic_dev, drv, "%s lro success\n",
			    SET_FEATURES_OP_STR(en));
	}

	return err;
}

static int set_feature_rx_cvlan(struct hinic3_nic_dev *nic_dev,
				netdev_features_t wanted_features,
				netdev_features_t features,
				netdev_features_t *failed_features)
{
	netdev_features_t changed = wanted_features ^ features;
#ifdef NETIF_F_HW_VLAN_CTAG_RX
	netdev_features_t vlan_feature = NETIF_F_HW_VLAN_CTAG_RX;
#else
	netdev_features_t vlan_feature = NETIF_F_HW_VLAN_RX;
#endif
	bool en = !!(wanted_features & vlan_feature);
	int err;

	if (!(changed & vlan_feature))
		return 0;

	err = hinic3_set_rx_vlan_offload(nic_dev->hwdev, en);
	if (err) {
		hinic3_err(nic_dev, drv, "%s rxvlan failed\n",
			   SET_FEATURES_OP_STR(en));
		*failed_features |= vlan_feature;
	} else {
		hinic3_info(nic_dev, drv, "%s rxvlan success\n",
			    SET_FEATURES_OP_STR(en));
	}

	return err;
}

static int set_feature_vlan_filter(struct hinic3_nic_dev *nic_dev,
				   netdev_features_t wanted_features,
				   netdev_features_t features,
				   netdev_features_t *failed_features)
{
	netdev_features_t changed = wanted_features ^ features;
#if defined(NETIF_F_HW_VLAN_CTAG_FILTER)
	netdev_features_t vlan_filter_feature = NETIF_F_HW_VLAN_CTAG_FILTER;
#elif defined(NETIF_F_HW_VLAN_FILTER)
	netdev_features_t vlan_filter_feature = NETIF_F_HW_VLAN_FILTER;
#endif
	bool en = !!(wanted_features & vlan_filter_feature);
	int err = 0;

	if (!(changed & vlan_filter_feature))
		return 0;

#ifdef NEED_VLAN_RESTORE
	if (en) {
		err = hinic3_vlan_restore(nic_dev->netdev);
		if (err) {
			hinic3_err(nic_dev, drv, "vlan restore failed\n");
			*failed_features |= vlan_filter_feature;
			return err;
		}
	}
#endif

	err = hinic3_set_vlan_fliter(nic_dev->hwdev, en);
	if (err) {
		hinic3_err(nic_dev, drv, "%s rx vlan filter failed\n",
			   SET_FEATURES_OP_STR(en));
		*failed_features |= vlan_filter_feature;
	} else {
		hinic3_info(nic_dev, drv, "%s rx vlan filter success\n",
			    SET_FEATURES_OP_STR(en));
	}

	return err;
}

static int set_features(struct hinic3_nic_dev *nic_dev,
			netdev_features_t pre_features,
			netdev_features_t features)
{
	netdev_features_t failed_features = 0;
	u32 err = 0;

	err |= (u32)set_feature_rx_csum(nic_dev, features, pre_features,
					&failed_features);
	err |= (u32)set_feature_tso(nic_dev, features, pre_features,
				    &failed_features);
	err |= (u32)set_feature_lro(nic_dev, features, pre_features,
				    &failed_features);
#ifdef NETIF_F_UFO
	err |= (u32)set_feature_ufo(nic_dev, features, pre_features,
				    &failed_features);
#endif
	err |= (u32)set_feature_rx_cvlan(nic_dev, features, pre_features,
					 &failed_features);
	err |= (u32)set_feature_vlan_filter(nic_dev, features, pre_features,
					    &failed_features);
	if (err) {
		nic_dev->netdev->features = features ^ failed_features;
		return -EIO;
	}

	return 0;
}

#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
static int hinic3_set_features(struct net_device *netdev, u32 features)
#else
static int hinic3_set_features(struct net_device *netdev,
			       netdev_features_t features)
#endif
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	return set_features(nic_dev, nic_dev->netdev->features,
			    features);
}

int hinic3_set_hw_features(struct hinic3_nic_dev *nic_dev)
{
	/* enable all hw features in netdev->features */
	return set_features(nic_dev, ~nic_dev->netdev->features,
			    nic_dev->netdev->features);
}

#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
static u32 hinic3_fix_features(struct net_device *netdev, u32 features)
#else
static netdev_features_t hinic3_fix_features(struct net_device *netdev,
					     netdev_features_t features)
#endif
{
	netdev_features_t features_tmp = features;

	/* If Rx checksum is disabled, then LRO should also be disabled */
	if (!(features_tmp & NETIF_F_RXCSUM))
		features_tmp &= ~NETIF_F_LRO;

	return features_tmp;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void hinic3_netpoll(struct net_device *netdev)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	u16 i;

	for (i = 0; i < nic_dev->q_params.num_qps; i++)
		napi_schedule(&nic_dev->q_params.irq_cfg[i].napi);
}
#endif /* CONFIG_NET_POLL_CONTROLLER */

static int hinic3_ndo_set_vf_mac(struct net_device *netdev, int vf, u8 *mac)
{
	struct hinic3_nic_dev *adapter = netdev_priv(netdev);
	int err;

	if (is_multicast_ether_addr(mac) ||
	    vf >= pci_num_vf(adapter->pdev))
		return -EINVAL;

	err = hinic3_set_vf_mac(adapter->hwdev, OS_VF_ID_TO_HW(vf), mac);
	if (err)
		return err;

	if (!is_zero_ether_addr(mac))
		nic_info(&adapter->pdev->dev, "Setting MAC %pM on VF %d\n",
			 mac, vf);
	else
		nic_info(&adapter->pdev->dev, "Deleting MAC on VF %d\n", vf);

	nic_info(&adapter->pdev->dev, "Please reload the VF driver to make this change effective.");

	return 0;
}

#ifdef IFLA_VF_MAX
static int set_hw_vf_vlan(void *hwdev, u16 cur_vlanprio, int vf,
			  u16 vlan, u8 qos)
{
	int err = 0;
	u16 old_vlan = cur_vlanprio & VLAN_VID_MASK;

	if (vlan || qos) {
		if (cur_vlanprio) {
			err = hinic3_kill_vf_vlan(hwdev, OS_VF_ID_TO_HW(vf));
			if (err)
				return err;
		}
		err = hinic3_add_vf_vlan(hwdev, OS_VF_ID_TO_HW(vf), vlan, qos);
	} else {
		err = hinic3_kill_vf_vlan(hwdev, OS_VF_ID_TO_HW(vf));
	}

	err = hinic3_update_mac_vlan(hwdev, old_vlan, vlan, OS_VF_ID_TO_HW(vf));
	return err;
}

#define HINIC3_MAX_VLAN_ID	4094
#define HINIC3_MAX_QOS_NUM	7

#ifdef IFLA_VF_VLAN_INFO_MAX
static int hinic3_ndo_set_vf_vlan(struct net_device *netdev, int vf, u16 vlan,
				  u8 qos, __be16 vlan_proto)
#else
static int hinic3_ndo_set_vf_vlan(struct net_device *netdev, int vf, u16 vlan,
				  u8 qos)
#endif
{
	struct hinic3_nic_dev *adapter = netdev_priv(netdev);
	u16 vlanprio, cur_vlanprio;

	if (vf >= pci_num_vf(adapter->pdev) ||
	    vlan > HINIC3_MAX_VLAN_ID || qos > HINIC3_MAX_QOS_NUM)
		return -EINVAL;
#ifdef IFLA_VF_VLAN_INFO_MAX
	if (vlan_proto != htons(ETH_P_8021Q))
		return -EPROTONOSUPPORT;
#endif
	vlanprio = vlan | (qos << HINIC3_VLAN_PRIORITY_SHIFT);
	cur_vlanprio = hinic3_vf_info_vlanprio(adapter->hwdev,
					       OS_VF_ID_TO_HW(vf));
	/* duplicate request, so just return success */
	if (vlanprio == cur_vlanprio)
		return 0;

	return set_hw_vf_vlan(adapter->hwdev, cur_vlanprio, vf, vlan, qos);
}
#endif

#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
static int hinic3_ndo_set_vf_spoofchk(struct net_device *netdev, int vf,
				      bool setting)
{
	struct hinic3_nic_dev *adapter = netdev_priv(netdev);
	int err = 0;
	bool cur_spoofchk = false;

	if (vf >= pci_num_vf(adapter->pdev))
		return -EINVAL;

	cur_spoofchk = hinic3_vf_info_spoofchk(adapter->hwdev,
					       OS_VF_ID_TO_HW(vf));
	/* same request, so just return success */
	if ((setting && cur_spoofchk) || (!setting && !cur_spoofchk))
		return 0;

	err = hinic3_set_vf_spoofchk(adapter->hwdev,
				     (u16)OS_VF_ID_TO_HW(vf), setting);
	if (!err)
		nicif_info(adapter, drv, netdev, "Set VF %d spoofchk %s\n",
			   vf, setting ? "on" : "off");

	return err;
}
#endif

#ifdef HAVE_NDO_SET_VF_TRUST
static int hinic3_ndo_set_vf_trust(struct net_device *netdev, int vf,
				   bool setting)
{
	struct hinic3_nic_dev *adapter = netdev_priv(netdev);
	int err;
	bool cur_trust;

	if (vf >= pci_num_vf(adapter->pdev))
		return -EINVAL;

	cur_trust = hinic3_get_vf_trust(adapter->hwdev,
					OS_VF_ID_TO_HW(vf));
	/* same request, so just return success */
	if ((setting && cur_trust) || (!setting && !cur_trust))
		return 0;

	err = hinic3_set_vf_trust(adapter->hwdev,
				  (u16)OS_VF_ID_TO_HW(vf), setting);
	if (!err)
		nicif_info(adapter, drv, netdev, "Set VF %d trusted %s successfully\n",
			   vf, setting ? "on" : "off");
	else
		nicif_err(adapter, drv, netdev, "Failed set VF %d trusted %s\n",
			  vf, setting ? "on" : "off");

	return err;
}
#endif

static int hinic3_ndo_get_vf_config(struct net_device *netdev,
				    int vf, struct ifla_vf_info *ivi)
{
	struct hinic3_nic_dev *adapter = netdev_priv(netdev);

	if (vf >= pci_num_vf(adapter->pdev))
		return -EINVAL;

	hinic3_get_vf_config(adapter->hwdev, (u16)OS_VF_ID_TO_HW(vf), ivi);

	return 0;
}

/**
 * hinic3_ndo_set_vf_link_state
 * @netdev: network interface device structure
 * @vf_id: VF identifier
 * @link: required link state
 *
 * Set the link state of a specified VF, regardless of physical link state
 **/
int hinic3_ndo_set_vf_link_state(struct net_device *netdev, int vf_id, int link)
{
	static const char * const vf_link[] = {"auto", "enable", "disable"};
	struct hinic3_nic_dev *adapter = netdev_priv(netdev);
	int err;

	/* validate the request */
	if (vf_id >= pci_num_vf(adapter->pdev)) {
		nicif_err(adapter, drv, netdev,
			  "Invalid VF Identifier %d\n", vf_id);
		return -EINVAL;
	}

	err = hinic3_set_vf_link_state(adapter->hwdev,
				       (u16)OS_VF_ID_TO_HW(vf_id), link);
	if (!err)
		nicif_info(adapter, drv, netdev, "Set VF %d link state: %s\n",
			   vf_id, vf_link[link]);

	return err;
}

static int is_set_vf_bw_param_valid(const struct hinic3_nic_dev *adapter,
				    int vf, int min_tx_rate, int max_tx_rate)
{
	if (!HINIC3_SUPPORT_RATE_LIMIT(adapter->hwdev)) {
		nicif_err(adapter, drv, adapter->netdev, "Current function doesn't support to set vf rate limit\n");
		return -EOPNOTSUPP;
	}

	/* verify VF is active */
	if (vf >= pci_num_vf(adapter->pdev)) {
		nicif_err(adapter, drv, adapter->netdev, "VF number must be less than %d\n",
			  pci_num_vf(adapter->pdev));
		return -EINVAL;
	}

	if (max_tx_rate < min_tx_rate) {
		nicif_err(adapter, drv, adapter->netdev, "Invalid rate, max rate %d must greater than min rate %d\n",
			  max_tx_rate, min_tx_rate);
		return -EINVAL;
	}

	return 0;
}

#define HINIC3_TX_RATE_TABLE_FULL	12

#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
static int hinic3_ndo_set_vf_bw(struct net_device *netdev,
				int vf, int min_tx_rate, int max_tx_rate)
#else
static int hinic3_ndo_set_vf_bw(struct net_device *netdev, int vf,
				int max_tx_rate)
#endif /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */
{
	struct hinic3_nic_dev *adapter = netdev_priv(netdev);
	struct nic_port_info port_info = {0};
#ifndef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	int min_tx_rate = 0;
#endif
	u8 link_status = 0;
	u32 speeds[] = {0, SPEED_10, SPEED_100, SPEED_1000, SPEED_10000,
			SPEED_25000, SPEED_40000, SPEED_50000, SPEED_100000,
			SPEED_200000};
	int err = 0;

	err = is_set_vf_bw_param_valid(adapter, vf, min_tx_rate, max_tx_rate);
	if (err)
		return err;

	err = hinic3_get_link_state(adapter->hwdev, &link_status);
	if (err) {
		nicif_err(adapter, drv, netdev,
			  "Get link status failed when set vf tx rate\n");
		return -EIO;
	}

	if (!link_status) {
		nicif_err(adapter, drv, netdev,
			  "Link status must be up when set vf tx rate\n");
		return -EINVAL;
	}

	err = hinic3_get_port_info(adapter->hwdev, &port_info,
				   HINIC3_CHANNEL_NIC);
	if (err || port_info.speed >= PORT_SPEED_UNKNOWN)
		return -EIO;

	/* rate limit cannot be less than 0 and greater than link speed */
	if (max_tx_rate < 0 || max_tx_rate > speeds[port_info.speed]) {
		nicif_err(adapter, drv, netdev, "Set vf max tx rate must be in [0 - %u]\n",
			  speeds[port_info.speed]);
		return -EINVAL;
	}

	err = hinic3_set_vf_tx_rate(adapter->hwdev, (u16)OS_VF_ID_TO_HW(vf),
				    (u32)max_tx_rate, (u32)min_tx_rate);
	if (err) {
		nicif_err(adapter, drv, netdev,
			  "Unable to set VF %d max rate %d min rate %d%s\n",
			  vf, max_tx_rate, min_tx_rate,
			  err == HINIC3_TX_RATE_TABLE_FULL ?
			  ", tx rate profile is full" : "");
		return -EIO;
	}

#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	nicif_info(adapter, drv, netdev,
		   "Set VF %d max tx rate %d min tx rate %d successfully\n",
		   vf, max_tx_rate, min_tx_rate);
#else
	nicif_info(adapter, drv, netdev,
		   "Set VF %d tx rate %d successfully\n",
		   vf, max_tx_rate);
#endif

	return 0;
}

#ifdef HAVE_XDP_SUPPORT
bool hinic3_is_xdp_enable(struct hinic3_nic_dev *nic_dev)
{
	return !!nic_dev->xdp_prog;
}

int hinic3_xdp_max_mtu(struct hinic3_nic_dev *nic_dev)
{
	return nic_dev->rx_buff_len - (ETH_HLEN + ETH_FCS_LEN + VLAN_HLEN);
}

static int hinic3_xdp_setup(struct hinic3_nic_dev *nic_dev,
			    struct bpf_prog *prog,
			    struct netlink_ext_ack *extack)
{
	struct bpf_prog *old_prog = NULL;
	int max_mtu = hinic3_xdp_max_mtu(nic_dev);
	int q_id;

	if (nic_dev->netdev->mtu > max_mtu) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to setup xdp program, the current MTU %d is larger than max allowed MTU %d\n",
			  nic_dev->netdev->mtu, max_mtu);
		NL_SET_ERR_MSG_MOD(extack,
				   "MTU too large for loading xdp program");
		return -EINVAL;
	}

	if (nic_dev->netdev->features & NETIF_F_LRO) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to setup xdp program while LRO is on\n");
		NL_SET_ERR_MSG_MOD(extack,
				   "Failed to setup xdp program while LRO is on\n");
		return -EINVAL;
	}

	old_prog = xchg(&nic_dev->xdp_prog, prog);
	for (q_id = 0; q_id < nic_dev->max_qps; q_id++)
		xchg(&nic_dev->rxqs[q_id].xdp_prog, nic_dev->xdp_prog);

	if (old_prog)
		bpf_prog_put(old_prog);

	return 0;
}

#ifdef HAVE_NDO_BPF_NETDEV_BPF
static int hinic3_xdp(struct net_device *netdev, struct netdev_bpf *xdp)
#else
static int hinic3_xdp(struct net_device *netdev, struct netdev_xdp *xdp)
#endif
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	switch (xdp->command) {
	case XDP_SETUP_PROG:
		return hinic3_xdp_setup(nic_dev, xdp->prog, xdp->extack);
#ifdef HAVE_XDP_QUERY_PROG
	case XDP_QUERY_PROG:
		xdp->prog_id = nic_dev->xdp_prog ?
			nic_dev->xdp_prog->aux->id : 0;
		return 0;
#endif
	default:
		return -EINVAL;
	}
}
#endif

static const struct net_device_ops hinic3_netdev_ops = {
	.ndo_open = hinic3_open,
	.ndo_stop = hinic3_close,
	.ndo_start_xmit = hinic3_xmit_frame,

#ifdef HAVE_NDO_GET_STATS64
	.ndo_get_stats64 =  hinic3_get_stats64,
#else
	.ndo_get_stats = hinic3_get_stats,
#endif /* HAVE_NDO_GET_STATS64 */

	.ndo_tx_timeout = hinic3_tx_timeout,
	.ndo_select_queue = hinic3_select_queue,
#ifdef HAVE_RHEL7_NETDEV_OPS_EXT_NDO_CHANGE_MTU
	.extended.ndo_change_mtu = hinic3_change_mtu,
#else
	.ndo_change_mtu = hinic3_change_mtu,
#endif
	.ndo_set_mac_address = hinic3_set_mac_addr,
	.ndo_validate_addr = eth_validate_addr,

#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	.ndo_vlan_rx_add_vid = hinic3_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = hinic3_vlan_rx_kill_vid,
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
	.ndo_set_vf_mac		= hinic3_ndo_set_vf_mac,
#ifdef HAVE_RHEL7_NETDEV_OPS_EXT_NDO_SET_VF_VLAN
	.extended.ndo_set_vf_vlan = hinic3_ndo_set_vf_vlan,
#else
	.ndo_set_vf_vlan	= hinic3_ndo_set_vf_vlan,
#endif
#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	.ndo_set_vf_rate	= hinic3_ndo_set_vf_bw,
#else
	.ndo_set_vf_tx_rate	= hinic3_ndo_set_vf_bw,
#endif /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */
#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
	.ndo_set_vf_spoofchk	= hinic3_ndo_set_vf_spoofchk,
#endif

#ifdef HAVE_NDO_SET_VF_TRUST
#ifdef HAVE_RHEL7_NET_DEVICE_OPS_EXT
	.extended.ndo_set_vf_trust = hinic3_ndo_set_vf_trust,
#else
	.ndo_set_vf_trust	= hinic3_ndo_set_vf_trust,
#endif /* HAVE_RHEL7_NET_DEVICE_OPS_EXT */
#endif /* HAVE_NDO_SET_VF_TRUST */

	.ndo_get_vf_config	= hinic3_ndo_get_vf_config,
#endif

#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller = hinic3_netpoll,
#endif /* CONFIG_NET_POLL_CONTROLLER */

	.ndo_set_rx_mode = hinic3_nic_set_rx_mode,

#ifdef HAVE_XDP_SUPPORT
#ifdef HAVE_NDO_BPF_NETDEV_BPF
	.ndo_bpf = hinic3_xdp,
#else
	.ndo_xdp = hinic3_xdp,
#endif
#endif
#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
};

/* RHEL6 keeps these operations in a separate structure */
static const struct net_device_ops_ext hinic3_netdev_ops_ext = {
	.size = sizeof(struct net_device_ops_ext),
#endif /* HAVE_RHEL6_NET_DEVICE_OPS_EXT */

#ifdef HAVE_NDO_SET_VF_LINK_STATE
	.ndo_set_vf_link_state	= hinic3_ndo_set_vf_link_state,
#endif

#ifdef HAVE_NDO_SET_FEATURES
	.ndo_fix_features = hinic3_fix_features,
	.ndo_set_features = hinic3_set_features,
#endif /* HAVE_NDO_SET_FEATURES */
};

static const struct net_device_ops hinic3vf_netdev_ops = {
	.ndo_open = hinic3_open,
	.ndo_stop = hinic3_close,
	.ndo_start_xmit = hinic3_xmit_frame,

#ifdef HAVE_NDO_GET_STATS64
	.ndo_get_stats64 =  hinic3_get_stats64,
#else
	.ndo_get_stats = hinic3_get_stats,
#endif /* HAVE_NDO_GET_STATS64 */

	.ndo_tx_timeout = hinic3_tx_timeout,
	.ndo_select_queue = hinic3_select_queue,

#ifdef HAVE_RHEL7_NET_DEVICE_OPS_EXT
	/* RHEL7 requires this to be defined to enable extended ops.  RHEL7
	 * uses the function get_ndo_ext to retrieve offsets for extended
	 * fields from with the net_device_ops struct and ndo_size is checked
	 * to determine whether or not the offset is valid.
	 */
	 .ndo_size = sizeof(const struct net_device_ops),
#endif

#ifdef HAVE_RHEL7_NETDEV_OPS_EXT_NDO_CHANGE_MTU
	.extended.ndo_change_mtu = hinic3_change_mtu,
#else
	.ndo_change_mtu = hinic3_change_mtu,
#endif
	.ndo_set_mac_address = hinic3_set_mac_addr,
	.ndo_validate_addr = eth_validate_addr,

#if defined(NETIF_F_HW_VLAN_TX) || defined(NETIF_F_HW_VLAN_CTAG_TX)
	.ndo_vlan_rx_add_vid = hinic3_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = hinic3_vlan_rx_kill_vid,
#endif

#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller = hinic3_netpoll,
#endif /* CONFIG_NET_POLL_CONTROLLER */

	.ndo_set_rx_mode = hinic3_nic_set_rx_mode,

#ifdef HAVE_XDP_SUPPORT
#ifdef HAVE_NDO_BPF_NETDEV_BPF
		.ndo_bpf = hinic3_xdp,
#else
		.ndo_xdp = hinic3_xdp,
#endif
#endif
#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
};

/* RHEL6 keeps these operations in a separate structure */
static const struct net_device_ops_ext hinic3vf_netdev_ops_ext = {
	.size = sizeof(struct net_device_ops_ext),
#endif /* HAVE_RHEL6_NET_DEVICE_OPS_EXT */

#ifdef HAVE_NDO_SET_FEATURES
	.ndo_fix_features = hinic3_fix_features,
	.ndo_set_features = hinic3_set_features,
#endif /* HAVE_NDO_SET_FEATURES */
};

void hinic3_set_netdev_ops(struct hinic3_nic_dev *nic_dev)
{
	if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev)) {
		nic_dev->netdev->netdev_ops = &hinic3_netdev_ops;
#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
		set_netdev_ops_ext(nic_dev->netdev, &hinic3_netdev_ops_ext);
#endif /* HAVE_RHEL6_NET_DEVICE_OPS_EXT */
	} else {
		nic_dev->netdev->netdev_ops = &hinic3vf_netdev_ops;
#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
		set_netdev_ops_ext(nic_dev->netdev, &hinic3vf_netdev_ops_ext);
#endif /* HAVE_RHEL6_NET_DEVICE_OPS_EXT */
	}
}

bool hinic3_is_netdev_ops_match(const struct net_device *netdev)
{
	return netdev->netdev_ops == &hinic3_netdev_ops ||
		netdev->netdev_ops == &hinic3vf_netdev_ops;
}
