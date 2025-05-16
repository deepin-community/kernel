// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/if_vlan.h>
#include <linux/ethtool.h>

#include "ossl_knl.h"
#include "hinic3_hw.h"
#include "hinic3_crm.h"
#include "hinic3_nic_dev.h"
#include "hinic3_tx.h"
#include "hinic3_rx.h"
#include "hinic3_rss.h"

#define COALESCE_ALL_QUEUE		0xFFFF
#define COALESCE_PENDING_LIMIT_UNIT	8
#define	COALESCE_TIMER_CFG_UNIT		5
#define COALESCE_MAX_PENDING_LIMIT	(255 * COALESCE_PENDING_LIMIT_UNIT)
#define COALESCE_MAX_TIMER_CFG		(255 * COALESCE_TIMER_CFG_UNIT)
#define HINIC3_WAIT_PKTS_TO_RX_BUFFER	200
#define HINIC3_WAIT_CLEAR_LP_TEST	100

#ifndef SET_ETHTOOL_OPS
#define SET_ETHTOOL_OPS(netdev, ops) \
	((netdev)->ethtool_ops = (ops))
#endif

static void hinic3_get_drvinfo(struct net_device *netdev,
			       struct ethtool_drvinfo *info)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct pci_dev *pdev = nic_dev->pdev;
	u8 mgmt_ver[HINIC3_MGMT_VERSION_MAX_LEN] = {0};
	int err;

	strscpy(info->driver, HINIC3_NIC_DRV_NAME, sizeof(info->driver));
	strscpy(info->version, HINIC3_NIC_DRV_VERSION, sizeof(info->version));
	strscpy(info->bus_info, pci_name(pdev), sizeof(info->bus_info));

	err = hinic3_get_mgmt_version(nic_dev->hwdev, mgmt_ver,
				      HINIC3_MGMT_VERSION_MAX_LEN,
				      HINIC3_CHANNEL_NIC);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Failed to get fw version\n");
		return;
	}

	snprintf(info->fw_version, sizeof(info->fw_version),
		 "%s", mgmt_ver);
}

static u32 hinic3_get_msglevel(struct net_device *netdev)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	return nic_dev->msg_enable;
}

static void hinic3_set_msglevel(struct net_device *netdev, u32 data)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	nic_dev->msg_enable = data;

	nicif_info(nic_dev, drv, netdev, "Set message level: 0x%x\n", data);
}

static int hinic3_nway_reset(struct net_device *netdev)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct nic_port_info port_info = {0};
	int err;

	while (test_and_set_bit(HINIC3_AUTONEG_RESET, &nic_dev->flags))
		msleep(100); /* sleep 100 ms, waiting for another autoneg restart progress done */

	err = hinic3_get_port_info(nic_dev->hwdev, &port_info,
				   HINIC3_CHANNEL_NIC);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Get port info failed\n");
		err = -EFAULT;
		goto reset_err;
	}

	if (port_info.autoneg_state != PORT_CFG_AN_ON) {
		nicif_err(nic_dev, drv, netdev, "Autonegotiation is not on, don't support to restart it\n");
		err = -EOPNOTSUPP;
		goto reset_err;
	}

	err = hinic3_set_autoneg(nic_dev->hwdev, false);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Set autonegotiation off failed\n");
		err = -EFAULT;
		goto reset_err;
	}

	msleep(200); /* sleep 200 ms, waiting for status polling finished */

	err = hinic3_set_autoneg(nic_dev->hwdev, true);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Set autonegotiation on failed\n");
		err = -EFAULT;
		goto reset_err;
	}

	msleep(200); /* sleep 200 ms, waiting for status polling finished */
	nicif_info(nic_dev, drv, netdev, "Restart autonegotiation successfully\n");

reset_err:
	clear_bit(HINIC3_AUTONEG_RESET, &nic_dev->flags);
	return err;
}

static void hinic3_get_ringparam(struct net_device *netdev,
				 struct ethtool_ringparam *ring,
				 struct kernel_ethtool_ringparam *kernel_ring,
				 struct netlink_ext_ack *extack)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	ring->rx_max_pending = HINIC3_MAX_RX_QUEUE_DEPTH;
	ring->tx_max_pending = HINIC3_MAX_TX_QUEUE_DEPTH;
	ring->rx_pending = nic_dev->rxqs[0].q_depth;
	ring->tx_pending = nic_dev->txqs[0].q_depth;
}

static void hinic3_update_qp_depth(struct hinic3_nic_dev *nic_dev,
				   u32 sq_depth, u32 rq_depth)
{
	u16 i;

	nic_dev->q_params.sq_depth = sq_depth;
	nic_dev->q_params.rq_depth = rq_depth;
	for (i = 0; i < nic_dev->max_qps; i++) {
		nic_dev->txqs[i].q_depth = sq_depth;
		nic_dev->txqs[i].q_mask = sq_depth - 1;
		nic_dev->rxqs[i].q_depth = rq_depth;
		nic_dev->rxqs[i].q_mask = rq_depth - 1;
	}
}

static int check_ringparam_valid(struct net_device *netdev,
				 const struct ethtool_ringparam *ring)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	if (ring->rx_jumbo_pending || ring->rx_mini_pending) {
		nicif_err(nic_dev, drv, netdev,
			  "Unsupported rx_jumbo_pending/rx_mini_pending\n");
		return -EINVAL;
	}

	if (ring->tx_pending > HINIC3_MAX_TX_QUEUE_DEPTH ||
	    ring->tx_pending < HINIC3_MIN_QUEUE_DEPTH ||
	    ring->rx_pending > HINIC3_MAX_RX_QUEUE_DEPTH ||
	    ring->rx_pending < HINIC3_MIN_QUEUE_DEPTH) {
		nicif_err(nic_dev, drv, netdev,
			  "Queue depth out of rang tx[%d-%d] rx[%d-%d]\n",
			  HINIC3_MIN_QUEUE_DEPTH, HINIC3_MAX_TX_QUEUE_DEPTH,
			  HINIC3_MIN_QUEUE_DEPTH, HINIC3_MAX_RX_QUEUE_DEPTH);
		return -EINVAL;
	}

	return 0;
}

static int hinic3_set_ringparam(struct net_device *netdev,
				struct ethtool_ringparam *ring,
				struct kernel_ethtool_ringparam *kernel_ring,
				struct netlink_ext_ack *extack)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct hinic3_dyna_txrxq_params q_params = {0};
	u32 new_sq_depth, new_rq_depth;
	int err;

	err = check_ringparam_valid(netdev, ring);
	if (err)
		return err;

	new_sq_depth = (u32)(1U << (u16)ilog2(ring->tx_pending));
	new_rq_depth = (u32)(1U << (u16)ilog2(ring->rx_pending));
	if (new_sq_depth == nic_dev->q_params.sq_depth &&
	    new_rq_depth == nic_dev->q_params.rq_depth)
		return 0; /* nothing to do */

	nicif_info(nic_dev, drv, netdev,
		   "Change Tx/Rx ring depth from %u/%u to %u/%u\n",
		   nic_dev->q_params.sq_depth, nic_dev->q_params.rq_depth,
		   new_sq_depth, new_rq_depth);

	if (!netif_running(netdev)) {
		hinic3_update_qp_depth(nic_dev, new_sq_depth, new_rq_depth);
	} else {
		q_params = nic_dev->q_params;
		q_params.sq_depth = new_sq_depth;
		q_params.rq_depth = new_rq_depth;
		q_params.txqs_res = NULL;
		q_params.rxqs_res = NULL;
		q_params.irq_cfg = NULL;

		nicif_info(nic_dev, drv, netdev, "Restarting channel\n");
		err = hinic3_change_channel_settings(nic_dev, &q_params,
						     NULL, NULL);
		if (err) {
			nicif_err(nic_dev, drv, netdev, "Failed to change channel settings\n");
			return -EFAULT;
		}
	}

	return 0;
}

static int get_coalesce(struct net_device *netdev,
			struct ethtool_coalesce *coal, u16 queue)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct hinic3_intr_coal_info *interrupt_info = NULL;

	if (queue == COALESCE_ALL_QUEUE) {
		/* get tx/rx irq0 as default parameters */
		interrupt_info = &nic_dev->intr_coalesce[0];
	} else {
		if (queue >= nic_dev->q_params.num_qps) {
			nicif_err(nic_dev, drv, netdev,
				  "Invalid queue_id: %u\n", queue);
			return -EINVAL;
		}
		interrupt_info = &nic_dev->intr_coalesce[queue];
	}

	/* coalescs_timer is in unit of 5us */
	coal->rx_coalesce_usecs = interrupt_info->coalesce_timer_cfg *
			COALESCE_TIMER_CFG_UNIT;
	/* coalescs_frams is in unit of 8 */
	coal->rx_max_coalesced_frames = interrupt_info->pending_limt *
			COALESCE_PENDING_LIMIT_UNIT;

	/* tx/rx use the same interrupt */
	coal->tx_coalesce_usecs = coal->rx_coalesce_usecs;
	coal->tx_max_coalesced_frames = coal->rx_max_coalesced_frames;
	coal->use_adaptive_rx_coalesce = nic_dev->adaptive_rx_coal;

	coal->pkt_rate_high = (u32)interrupt_info->pkt_rate_high;
	coal->rx_coalesce_usecs_high = interrupt_info->rx_usecs_high *
				       COALESCE_TIMER_CFG_UNIT;
	coal->rx_max_coalesced_frames_high =
					interrupt_info->rx_pending_limt_high *
					COALESCE_PENDING_LIMIT_UNIT;

	coal->pkt_rate_low = (u32)interrupt_info->pkt_rate_low;
	coal->rx_coalesce_usecs_low = interrupt_info->rx_usecs_low *
				      COALESCE_TIMER_CFG_UNIT;
	coal->rx_max_coalesced_frames_low =
					interrupt_info->rx_pending_limt_low *
					COALESCE_PENDING_LIMIT_UNIT;

	return 0;
}

static int set_queue_coalesce(struct hinic3_nic_dev *nic_dev, u16 q_id,
			      struct hinic3_intr_coal_info *coal)
{
	struct hinic3_intr_coal_info *intr_coal = NULL;
	struct interrupt_info info = {0};
	struct net_device *netdev = nic_dev->netdev;
	int err;

	intr_coal = &nic_dev->intr_coalesce[q_id];
	if (intr_coal->coalesce_timer_cfg != coal->coalesce_timer_cfg ||
	    intr_coal->pending_limt != coal->pending_limt)
		intr_coal->user_set_intr_coal_flag = 1;

	intr_coal->coalesce_timer_cfg = coal->coalesce_timer_cfg;
	intr_coal->pending_limt = coal->pending_limt;
	intr_coal->pkt_rate_low = coal->pkt_rate_low;
	intr_coal->rx_usecs_low = coal->rx_usecs_low;
	intr_coal->rx_pending_limt_low = coal->rx_pending_limt_low;
	intr_coal->pkt_rate_high = coal->pkt_rate_high;
	intr_coal->rx_usecs_high = coal->rx_usecs_high;
	intr_coal->rx_pending_limt_high = coal->rx_pending_limt_high;

	/* netdev not running or qp not in using,
	 * don't need to set coalesce to hw
	 */
	if (!test_bit(HINIC3_INTF_UP, &nic_dev->flags) ||
	    q_id >= nic_dev->q_params.num_qps || nic_dev->adaptive_rx_coal)
		return 0;

	info.msix_index = nic_dev->q_params.irq_cfg[q_id].msix_entry_idx;
	info.lli_set = 0;
	info.interrupt_coalesc_set = 1;
	info.coalesc_timer_cfg = intr_coal->coalesce_timer_cfg;
	info.pending_limt = intr_coal->pending_limt;
	info.resend_timer_cfg = intr_coal->resend_timer_cfg;
	nic_dev->rxqs[q_id].last_coalesc_timer_cfg =
					intr_coal->coalesce_timer_cfg;
	nic_dev->rxqs[q_id].last_pending_limt = intr_coal->pending_limt;
	err = hinic3_set_interrupt_cfg(nic_dev->hwdev, info,
				       HINIC3_CHANNEL_NIC);
	if (err)
		nicif_warn(nic_dev, drv, netdev,
			   "Failed to set queue%u coalesce", q_id);

	return err;
}

static int is_coalesce_exceed_limit(struct net_device *netdev,
				    const struct ethtool_coalesce *coal)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	if (coal->rx_coalesce_usecs > COALESCE_MAX_TIMER_CFG) {
		nicif_err(nic_dev, drv, netdev,
			  "rx_coalesce_usecs out of range[%d-%d]\n", 0,
			  COALESCE_MAX_TIMER_CFG);
		return -EOPNOTSUPP;
	}

	if (coal->rx_max_coalesced_frames > COALESCE_MAX_PENDING_LIMIT) {
		nicif_err(nic_dev, drv, netdev,
			  "rx_max_coalesced_frames out of range[%d-%d]\n", 0,
			  COALESCE_MAX_PENDING_LIMIT);
		return -EOPNOTSUPP;
	}

	if (coal->rx_coalesce_usecs_low > COALESCE_MAX_TIMER_CFG) {
		nicif_err(nic_dev, drv, netdev,
			  "rx_coalesce_usecs_low out of range[%d-%d]\n", 0,
			  COALESCE_MAX_TIMER_CFG);
		return -EOPNOTSUPP;
	}

	if (coal->rx_max_coalesced_frames_low > COALESCE_MAX_PENDING_LIMIT) {
		nicif_err(nic_dev, drv, netdev,
			  "rx_max_coalesced_frames_low out of range[%d-%d]\n",
			  0, COALESCE_MAX_PENDING_LIMIT);
		return -EOPNOTSUPP;
	}

	if (coal->rx_coalesce_usecs_high > COALESCE_MAX_TIMER_CFG) {
		nicif_err(nic_dev, drv, netdev,
			  "rx_coalesce_usecs_high out of range[%d-%d]\n", 0,
			  COALESCE_MAX_TIMER_CFG);
		return -EOPNOTSUPP;
	}

	if (coal->rx_max_coalesced_frames_high > COALESCE_MAX_PENDING_LIMIT) {
		nicif_err(nic_dev, drv, netdev,
			  "rx_max_coalesced_frames_high out of range[%d-%d]\n",
			  0, COALESCE_MAX_PENDING_LIMIT);
		return -EOPNOTSUPP;
	}

	return 0;
}

static int is_coalesce_allowed_change(struct net_device *netdev,
				      const struct ethtool_coalesce *coal)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct ethtool_coalesce tmp_coal = {0};

	tmp_coal.cmd = coal->cmd;
	tmp_coal.rx_coalesce_usecs = coal->rx_coalesce_usecs;
	tmp_coal.rx_max_coalesced_frames = coal->rx_max_coalesced_frames;
	tmp_coal.tx_coalesce_usecs = coal->tx_coalesce_usecs;
	tmp_coal.tx_max_coalesced_frames = coal->tx_max_coalesced_frames;
	tmp_coal.use_adaptive_rx_coalesce = coal->use_adaptive_rx_coalesce;

	tmp_coal.pkt_rate_low = coal->pkt_rate_low;
	tmp_coal.rx_coalesce_usecs_low = coal->rx_coalesce_usecs_low;
	tmp_coal.rx_max_coalesced_frames_low =
					coal->rx_max_coalesced_frames_low;

	tmp_coal.pkt_rate_high = coal->pkt_rate_high;
	tmp_coal.rx_coalesce_usecs_high = coal->rx_coalesce_usecs_high;
	tmp_coal.rx_max_coalesced_frames_high =
					coal->rx_max_coalesced_frames_high;

	if (memcmp(coal, &tmp_coal, sizeof(struct ethtool_coalesce))) {
		nicif_err(nic_dev, drv, netdev,
			  "Only support to change rx/tx-usecs and rx/tx-frames\n");
		return -EOPNOTSUPP;
	}

	return 0;
}

static int is_coalesce_legal(struct net_device *netdev,
			     const struct ethtool_coalesce *coal)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	int err;

	if (coal->rx_coalesce_usecs != coal->tx_coalesce_usecs) {
		nicif_err(nic_dev, drv, netdev,
			  "tx-usecs must be equal to rx-usecs\n");
		return -EINVAL;
	}

	if (coal->rx_max_coalesced_frames != coal->tx_max_coalesced_frames) {
		nicif_err(nic_dev, drv, netdev,
			  "tx-frames must be equal to rx-frames\n");
		return -EINVAL;
	}

	err = is_coalesce_allowed_change(netdev, coal);
	if (err)
		return err;

	err = is_coalesce_exceed_limit(netdev, coal);
	if (err)
		return err;

	if (coal->rx_coalesce_usecs_low / COALESCE_TIMER_CFG_UNIT >=
	    coal->rx_coalesce_usecs_high / COALESCE_TIMER_CFG_UNIT) {
		nicif_err(nic_dev, drv, netdev,
			  "coalesce_usecs_high(%u) must more than coalesce_usecs_low(%u), after dividing %d usecs unit\n",
			  coal->rx_coalesce_usecs_high,
			  coal->rx_coalesce_usecs_low,
			  COALESCE_TIMER_CFG_UNIT);
		return -EOPNOTSUPP;
	}

	if (coal->rx_max_coalesced_frames_low / COALESCE_PENDING_LIMIT_UNIT >=
	    coal->rx_max_coalesced_frames_high / COALESCE_PENDING_LIMIT_UNIT) {
		nicif_err(nic_dev, drv, netdev,
			  "coalesced_frames_high(%u) must more than coalesced_frames_low(%u),after dividing %d frames unit\n",
			  coal->rx_max_coalesced_frames_high,
			  coal->rx_max_coalesced_frames_low,
			  COALESCE_PENDING_LIMIT_UNIT);
		return -EOPNOTSUPP;
	}

	if (coal->pkt_rate_low >= coal->pkt_rate_high) {
		nicif_err(nic_dev, drv, netdev,
			  "pkt_rate_high(%u) must more than pkt_rate_low(%u)\n",
			  coal->pkt_rate_high,
			  coal->pkt_rate_low);
		return -EOPNOTSUPP;
	}

	return 0;
}

static inline void check_coalesce_aligned(struct hinic3_nic_dev *nic_dev,
					  struct net_device *netdev,
					  u32 item, u32 unit, char *str)
{
	if (item % unit)
		nicif_warn(nic_dev, drv, netdev, "%s in %d units, change to %u\n",
			   str, unit, item - item % unit);
}

#define CHECK_COALESCE_ALIGN(member, unit)				\
	check_coalesce_aligned(nic_dev, netdev, member, unit, #member)

static inline void check_coalesce_changed(struct hinic3_nic_dev *nic_dev,
					  struct net_device *netdev,
					  u32 item, u32 unit, u32 ori_val,
					  char *obj_str, char *str)
{
	if ((item / unit) != ori_val)
		nicif_info(nic_dev, drv, netdev, "Change %s from %d to %u %s\n",
			   str, ori_val * unit, item - item % unit, obj_str);
}

#define CHECK_COALESCE_CHANGED(member, unit, ori_val, obj_str)	\
	check_coalesce_changed(nic_dev, netdev, member, unit, ori_val, obj_str, #member)

static inline void check_pkt_rate_changed(struct hinic3_nic_dev *nic_dev,
					  struct net_device *netdev,
					  u32 item, u32 ori_val, char *obj_str,
					  char *str)
{
	if (item != ori_val)
		nicif_info(nic_dev, drv, netdev, "Change %s from %d to %u %s\n",
			   str, ori_val, item, obj_str);
}

#define CHECK_PKT_RATE_CHANGED(member, ori_val, obj_str)		\
	check_pkt_rate_changed(nic_dev, netdev, member, ori_val, obj_str, #member)

static int set_hw_coal_param(struct hinic3_nic_dev *nic_dev,
			     struct hinic3_intr_coal_info *intr_coal, u16 queue)
{
	u16 i;

	if (queue == COALESCE_ALL_QUEUE) {
		for (i = 0; i < nic_dev->max_qps; i++)
			set_queue_coalesce(nic_dev, i, intr_coal);
	} else {
		if (queue >= nic_dev->q_params.num_qps) {
			nicif_err(nic_dev, drv, nic_dev->netdev,
				  "Invalid queue_id: %u\n", queue);
			return -EINVAL;
		}
		set_queue_coalesce(nic_dev, queue, intr_coal);
	}

	return 0;
}

static void check_coalesce_align(struct net_device *netdev,
				 struct ethtool_coalesce *coal)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	CHECK_COALESCE_ALIGN(coal->rx_coalesce_usecs, COALESCE_TIMER_CFG_UNIT);
	CHECK_COALESCE_ALIGN(coal->rx_max_coalesced_frames,
			     COALESCE_PENDING_LIMIT_UNIT);
	CHECK_COALESCE_ALIGN(coal->rx_coalesce_usecs_high,
			     COALESCE_TIMER_CFG_UNIT);
	CHECK_COALESCE_ALIGN(coal->rx_max_coalesced_frames_high,
			     COALESCE_PENDING_LIMIT_UNIT);
	CHECK_COALESCE_ALIGN(coal->rx_coalesce_usecs_low,
			     COALESCE_TIMER_CFG_UNIT);
	CHECK_COALESCE_ALIGN(coal->rx_max_coalesced_frames_low,
			     COALESCE_PENDING_LIMIT_UNIT);
}

static int check_coalesce_change(struct net_device *netdev,
				 u16 queue, struct ethtool_coalesce *coal)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct hinic3_intr_coal_info *ori_intr_coal = NULL;
	char obj_str[32] = {0};

	if (queue == COALESCE_ALL_QUEUE) {
		ori_intr_coal = &nic_dev->intr_coalesce[0];
		snprintf(obj_str, sizeof(obj_str), "for netdev");
	} else {
		ori_intr_coal = &nic_dev->intr_coalesce[queue];
		snprintf(obj_str, sizeof(obj_str), "for queue %u", queue);
	}
	CHECK_COALESCE_CHANGED(coal->rx_coalesce_usecs, COALESCE_TIMER_CFG_UNIT,
			       ori_intr_coal->coalesce_timer_cfg, obj_str);
	CHECK_COALESCE_CHANGED(coal->rx_max_coalesced_frames,
			       COALESCE_PENDING_LIMIT_UNIT,
			       ori_intr_coal->pending_limt, obj_str);
	CHECK_PKT_RATE_CHANGED(coal->pkt_rate_high,
			       ori_intr_coal->pkt_rate_high, obj_str);
	CHECK_COALESCE_CHANGED(coal->rx_coalesce_usecs_high,
			       COALESCE_TIMER_CFG_UNIT,
			       ori_intr_coal->rx_usecs_high, obj_str);
	CHECK_COALESCE_CHANGED(coal->rx_max_coalesced_frames_high,
			       COALESCE_PENDING_LIMIT_UNIT,
			       ori_intr_coal->rx_pending_limt_high, obj_str);
	CHECK_PKT_RATE_CHANGED(coal->pkt_rate_low,
			       ori_intr_coal->pkt_rate_low, obj_str);
	CHECK_COALESCE_CHANGED(coal->rx_coalesce_usecs_low,
			       COALESCE_TIMER_CFG_UNIT,
			       ori_intr_coal->rx_usecs_low, obj_str);
	CHECK_COALESCE_CHANGED(coal->rx_max_coalesced_frames_low,
			       COALESCE_PENDING_LIMIT_UNIT,
			       ori_intr_coal->rx_pending_limt_low, obj_str);

	return 0;
}

static void init_intr_coal_params(struct hinic3_intr_coal_info *intr_coal,
				  struct ethtool_coalesce *coal)
{
	intr_coal->coalesce_timer_cfg =
		(u8)(coal->rx_coalesce_usecs / COALESCE_TIMER_CFG_UNIT);
	intr_coal->pending_limt = (u8)(coal->rx_max_coalesced_frames /
				      COALESCE_PENDING_LIMIT_UNIT);

	intr_coal->pkt_rate_high = coal->pkt_rate_high;
	intr_coal->rx_usecs_high =
		(u8)(coal->rx_coalesce_usecs_high / COALESCE_TIMER_CFG_UNIT);
	intr_coal->rx_pending_limt_high =
		(u8)(coal->rx_max_coalesced_frames_high /
		     COALESCE_PENDING_LIMIT_UNIT);

	intr_coal->pkt_rate_low = coal->pkt_rate_low;
	intr_coal->rx_usecs_low =
		(u8)(coal->rx_coalesce_usecs_low / COALESCE_TIMER_CFG_UNIT);
	intr_coal->rx_pending_limt_low =
		(u8)(coal->rx_max_coalesced_frames_low /
		     COALESCE_PENDING_LIMIT_UNIT);
}

static int set_coalesce(struct net_device *netdev,
			struct ethtool_coalesce *coal, u16 queue)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct hinic3_intr_coal_info intr_coal = {0};
	u32 last_adaptive_rx;
	int err = 0;

	err = is_coalesce_legal(netdev, coal);
	if (err)
		return err;

	check_coalesce_align(netdev, coal);

	err = check_coalesce_change(netdev, queue, coal);
	if (err)
		return err;

	init_intr_coal_params(&intr_coal, coal);

	last_adaptive_rx = nic_dev->adaptive_rx_coal;
	nic_dev->adaptive_rx_coal = coal->use_adaptive_rx_coalesce;

	/* coalesce timer or pending set to zero will disable coalesce */
	if (!nic_dev->adaptive_rx_coal &&
	    (!intr_coal.coalesce_timer_cfg || !intr_coal.pending_limt))
		nicif_warn(nic_dev, drv, netdev, "Coalesce will be disabled\n");

	/* ensure coalesce paramester will not be changed in auto
	 * moderation work
	 */
	if (HINIC3_CHANNEL_RES_VALID(nic_dev)) {
		if (!nic_dev->adaptive_rx_coal)
			cancel_delayed_work_sync(&nic_dev->moderation_task);
		else if (!last_adaptive_rx)
			queue_delayed_work(nic_dev->workq,
					   &nic_dev->moderation_task,
					   HINIC3_MODERATONE_DELAY);
	}

	return set_hw_coal_param(nic_dev, &intr_coal, queue);
}

static int hinic3_get_coalesce(struct net_device *netdev,
			       struct ethtool_coalesce *coal,
			       struct kernel_ethtool_coalesce *kernel_coal,
			       struct netlink_ext_ack *extack)
{
	return get_coalesce(netdev, coal, COALESCE_ALL_QUEUE);
}

static int hinic3_set_coalesce(struct net_device *netdev,
			       struct ethtool_coalesce *coal,
			       struct kernel_ethtool_coalesce *kernel_coal,
			       struct netlink_ext_ack *extack)
{
	return set_coalesce(netdev, coal, COALESCE_ALL_QUEUE);
}

#if defined(ETHTOOL_PERQUEUE) && defined(ETHTOOL_GCOALESCE)
static int hinic3_get_per_queue_coalesce(struct net_device *netdev, u32 queue,
					 struct ethtool_coalesce *coal)
{
	return get_coalesce(netdev, coal, (u16)queue);
}

static int hinic3_set_per_queue_coalesce(struct net_device *netdev, u32 queue,
					 struct ethtool_coalesce *coal)
{
	return set_coalesce(netdev, coal, (u16)queue);
}
#endif

#ifdef HAVE_ETHTOOL_SET_PHYS_ID
static int hinic3_set_phys_id(struct net_device *netdev,
			      enum ethtool_phys_id_state state)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	int err;

	switch (state) {
	case ETHTOOL_ID_ACTIVE:
		err = hinic3_set_led_status(nic_dev->hwdev,
					    MAG_CMD_LED_TYPE_ALARM,
					    MAG_CMD_LED_MODE_FORCE_BLINK_2HZ);
		if (err)
			nicif_err(nic_dev, drv, netdev,
				  "Set LED blinking in 2HZ failed\n");
		else
			nicif_info(nic_dev, drv, netdev,
				   "Set LED blinking in 2HZ success\n");
		break;

	case ETHTOOL_ID_INACTIVE:
		err = hinic3_set_led_status(nic_dev->hwdev,
					    MAG_CMD_LED_TYPE_ALARM,
					    MAG_CMD_LED_MODE_DEFAULT);
		if (err)
			nicif_err(nic_dev, drv, netdev,
				  "Reset LED to original status failed\n");
		else
			nicif_info(nic_dev, drv, netdev,
				   "Reset LED to original status success\n");
		break;

	default:
		return -EOPNOTSUPP;
	}

	return err;
}
#else
static int hinic3_phys_id(struct net_device *netdev, u32 data)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	nicif_err(nic_dev, drv, netdev, "Not support to set phys id\n");

	return -EOPNOTSUPP;
}
#endif

static void hinic3_get_pauseparam(struct net_device *netdev,
				  struct ethtool_pauseparam *pause)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct nic_pause_config nic_pause = {0};
	int err;

	err = hinic3_get_pause_info(nic_dev->hwdev, &nic_pause);
	if (err) {
		nicif_err(nic_dev, drv, netdev,
			  "Failed to get pauseparam from hw\n");
	} else {
		pause->autoneg = nic_pause.auto_neg == PORT_CFG_AN_ON ?
				 AUTONEG_ENABLE : AUTONEG_DISABLE;
		pause->rx_pause = nic_pause.rx_pause;
		pause->tx_pause = nic_pause.tx_pause;
	}
}

static int hinic3_set_pauseparam(struct net_device *netdev,
				 struct ethtool_pauseparam *pause)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct nic_pause_config nic_pause = {0};
	struct nic_port_info port_info = {0};
	u32 auto_neg;
	int err;

	err = hinic3_get_port_info(nic_dev->hwdev, &port_info,
				   HINIC3_CHANNEL_NIC);
	if (err) {
		nicif_err(nic_dev, drv, netdev,
			  "Failed to get auto-negotiation state\n");
		return -EFAULT;
	}

	auto_neg = port_info.autoneg_state == PORT_CFG_AN_ON ?
		   AUTONEG_ENABLE : AUTONEG_DISABLE;
	if (pause->autoneg != auto_neg) {
		nicif_err(nic_dev, drv, netdev,
			  "To change autoneg please use: ethtool -s <dev> autoneg <on|off>\n");
		return -EOPNOTSUPP;
	}

	nic_pause.auto_neg = pause->autoneg == AUTONEG_ENABLE ?
			     PORT_CFG_AN_ON : PORT_CFG_AN_OFF;
	nic_pause.rx_pause = (u8)pause->rx_pause;
	nic_pause.tx_pause = (u8)pause->tx_pause;

	err = hinic3_set_pause_info(nic_dev->hwdev, nic_pause);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Failed to set pauseparam\n");
		return err;
	}

	nicif_info(nic_dev, drv, netdev, "Set pause options, tx: %s, rx: %s\n",
		   pause->tx_pause ? "on" : "off",
		   pause->rx_pause ? "on" : "off");

	return 0;
}

#ifdef ETHTOOL_GMODULEEEPROM
static int hinic3_get_module_info(struct net_device *netdev,
				  struct ethtool_modinfo *modinfo)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	u8 sfp_type = 0;
	u8 sfp_type_ext = 0;
	int err;

	err = hinic3_get_sfp_type(nic_dev->hwdev, &sfp_type, &sfp_type_ext);
	if (err)
		return err;

	switch (sfp_type) {
	case MODULE_TYPE_SFP:
		modinfo->type = ETH_MODULE_SFF_8472;
		modinfo->eeprom_len = ETH_MODULE_SFF_8472_LEN;
		break;
	case MODULE_TYPE_QSFP:
		modinfo->type = ETH_MODULE_SFF_8436;
		modinfo->eeprom_len = ETH_MODULE_SFF_8436_MAX_LEN;
		break;
	case MODULE_TYPE_QSFP_PLUS:
		if (sfp_type_ext >= 0x3) {
			modinfo->type = ETH_MODULE_SFF_8636;
			modinfo->eeprom_len = ETH_MODULE_SFF_8636_MAX_LEN;
		} else {
			modinfo->type = ETH_MODULE_SFF_8436;
			modinfo->eeprom_len = ETH_MODULE_SFF_8436_MAX_LEN;
		}
		break;
	case MODULE_TYPE_QSFP28:
		modinfo->type = ETH_MODULE_SFF_8636;
		modinfo->eeprom_len = ETH_MODULE_SFF_8636_MAX_LEN;
		break;
	case MODULE_TYPE_DSFP:
		modinfo->type = ETH_MODULE_SFF_8636;
		modinfo->eeprom_len = ETH_MODULE_SFF_8636_MAX_LEN;
		break;
	case MODULE_TYPE_QSFP_CMIS:
		modinfo->type = ETH_MODULE_SFF_8636;
		modinfo->eeprom_len = ETH_MODULE_SFF_8636_MAX_LEN;
		break;
	default:
		nicif_warn(nic_dev, drv, netdev,
			   "Optical module unknown: 0x%x\n", sfp_type);
		return -EINVAL;
	}

	return 0;
}

static int hinic3_get_module_eeprom(struct net_device *netdev,
				    struct ethtool_eeprom *ee, u8 *data)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	u8 sfp_data[STD_SFP_INFO_MAX_SIZE];
	int err;

	if (!ee->len || ((ee->len + ee->offset) > STD_SFP_INFO_MAX_SIZE))
		return -EINVAL;

	memset(data, 0, ee->len);

	err = hinic3_get_sfp_eeprom(nic_dev->hwdev, (u8 *)sfp_data, ee->len);
	if (err == HINIC3_MGMT_CMD_UNSUPPORTED)
		err = hinic3_get_tlv_xsfp_eeprom(nic_dev->hwdev,
						 (u8 *)sfp_data,
						 sizeof(sfp_data));

	if (err)
		return err;

	memcpy(data, sfp_data + ee->offset, ee->len);

	return 0;
}
#endif /* ETHTOOL_GMODULEEEPROM */

#define HINIC3_PRIV_FLAGS_SYMM_RSS	BIT(0)
#define HINIC3_PRIV_FLAGS_LINK_UP	BIT(1)
#define HINIC3_PRIV_FLAGS_RXQ_RECOVERY	BIT(2)

static u32 hinic3_get_priv_flags(struct net_device *netdev)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	u32 priv_flags = 0;

	if (test_bit(HINIC3_SAME_RXTX, &nic_dev->flags))
		priv_flags |= HINIC3_PRIV_FLAGS_SYMM_RSS;

	if (test_bit(HINIC3_FORCE_LINK_UP, &nic_dev->flags))
		priv_flags |= HINIC3_PRIV_FLAGS_LINK_UP;

	if (test_bit(HINIC3_RXQ_RECOVERY, &nic_dev->flags))
		priv_flags |= HINIC3_PRIV_FLAGS_RXQ_RECOVERY;

	return priv_flags;
}

int hinic3_set_rxq_recovery_flag(struct net_device *netdev, u32 priv_flags)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	if (priv_flags & HINIC3_PRIV_FLAGS_RXQ_RECOVERY) {
		if (!HINIC3_SUPPORT_RXQ_RECOVERY(nic_dev->hwdev)) {
			nicif_info(nic_dev, drv, netdev, "Unsupport open rxq recovery\n");
			return -EOPNOTSUPP;
		}

		if (test_and_set_bit(HINIC3_RXQ_RECOVERY, &nic_dev->flags))
			return 0;
		queue_delayed_work(nic_dev->workq,
				   &nic_dev->rxq_check_work, HZ);
		nicif_info(nic_dev, drv, netdev, "open rxq recovery\n");
	} else {
		if (!test_and_clear_bit(HINIC3_RXQ_RECOVERY, &nic_dev->flags))
			return 0;
		cancel_delayed_work_sync(&nic_dev->rxq_check_work);
		nicif_info(nic_dev, drv, netdev, "close rxq recovery\n");
	}

	return 0;
}

static int hinic3_set_symm_rss_flag(struct net_device *netdev, u32 priv_flags)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	if (priv_flags & HINIC3_PRIV_FLAGS_SYMM_RSS) {
		if (test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags)) {
			nicif_err(nic_dev, drv, netdev, "Failed to open Symmetric RSS while DCB is enabled\n");
			return -EOPNOTSUPP;
		}

		if (!test_bit(HINIC3_RSS_ENABLE, &nic_dev->flags)) {
			nicif_err(nic_dev, drv, netdev, "Failed to open Symmetric RSS while RSS is disabled\n");
			return -EOPNOTSUPP;
		}

		set_bit(HINIC3_SAME_RXTX, &nic_dev->flags);
	} else {
		clear_bit(HINIC3_SAME_RXTX, &nic_dev->flags);
	}

	return 0;
}

static int hinic3_set_force_link_flag(struct net_device *netdev, u32 priv_flags)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	u8 link_status = 0;
	int err;

	if (priv_flags & HINIC3_PRIV_FLAGS_LINK_UP) {
		if (test_and_set_bit(HINIC3_FORCE_LINK_UP, &nic_dev->flags))
			return 0;

		if (!HINIC3_CHANNEL_RES_VALID(nic_dev))
			return 0;

		if (netif_carrier_ok(netdev))
			return 0;

		nic_dev->link_status = true;
		netif_carrier_on(netdev);
		nicif_info(nic_dev, link, netdev, "Set link up\n");

		if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev))
			hinic3_notify_all_vfs_link_changed(nic_dev->hwdev,
							   nic_dev->link_status);
	} else {
		if (!test_and_clear_bit(HINIC3_FORCE_LINK_UP, &nic_dev->flags))
			return 0;

		if (!HINIC3_CHANNEL_RES_VALID(nic_dev))
			return 0;

		err = hinic3_get_link_state(nic_dev->hwdev, &link_status);
		if (err) {
			nicif_err(nic_dev, link, netdev, "Get link state err: %d\n", err);
			return err;
		}

		nic_dev->link_status = link_status;

		if (link_status) {
			if (netif_carrier_ok(netdev))
				return 0;

			netif_carrier_on(netdev);
			nicif_info(nic_dev, link, netdev, "Link state is up\n");
		} else {
			if (!netif_carrier_ok(netdev))
				return 0;

			netif_carrier_off(netdev);
			nicif_info(nic_dev, link, netdev, "Link state is down\n");
		}

		if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev))
			hinic3_notify_all_vfs_link_changed(nic_dev->hwdev,
							   nic_dev->link_status);
	}

	return 0;
}

static int hinic3_set_priv_flags(struct net_device *netdev, u32 priv_flags)
{
	int err;

	err = hinic3_set_symm_rss_flag(netdev, priv_flags);
	if (err)
		return err;

	err = hinic3_set_rxq_recovery_flag(netdev, priv_flags);
	if (err)
		return err;

	return hinic3_set_force_link_flag(netdev, priv_flags);
}

#define PORT_DOWN_ERR_IDX  0
#define LP_DEFAULT_TIME    5 /* seconds */

#define TEST_TIME_MULTIPLE 5
static int hinic3_run_lp_test(struct hinic3_nic_dev *nic_dev, u32 test_time)
{
	u8 *lb_test_rx_buf = nic_dev->lb_test_rx_buf;
	struct net_device *netdev = nic_dev->netdev;
	u32 cnt = test_time * TEST_TIME_MULTIPLE;
	struct sk_buff *skb_tmp = NULL;
	struct ethhdr *eth_hdr = NULL;
	struct sk_buff *skb = NULL;
	u8 *test_data = NULL;
	u32 i;
	u8 j;

	skb_tmp = alloc_skb(LP_PKT_LEN, GFP_ATOMIC);
	if (!skb_tmp)
		return -ENOMEM;

	eth_hdr = __skb_put(skb_tmp, ETH_HLEN);
	eth_hdr->h_proto = htons(ETH_P_ARP);
	ether_addr_copy(eth_hdr->h_dest, nic_dev->netdev->dev_addr);
	eth_zero_addr(eth_hdr->h_source);
	skb_reset_mac_header(skb_tmp);

	test_data = __skb_put(skb_tmp, LP_PKT_LEN - ETH_HLEN);
	for (i = ETH_HLEN; i < LP_PKT_LEN; i++)
		test_data[i] = i & 0xFF;

	skb_tmp->queue_mapping = 0;
	skb_tmp->dev = netdev;
	skb_tmp->protocol = htons(ETH_P_ARP);

	for (i = 0; i < cnt; i++) {
		nic_dev->lb_test_rx_idx = 0;
		memset(lb_test_rx_buf, 0, LP_PKT_CNT * LP_PKT_LEN);

		for (j = 0; j < LP_PKT_CNT; j++) {
			skb = pskb_copy(skb_tmp, GFP_ATOMIC);
			if (!skb) {
				dev_kfree_skb_any(skb_tmp);
				nicif_err(nic_dev, drv, netdev,
					  "Copy skb failed for loopback test\n");
				return -ENOMEM;
			}

			/* mark index for every pkt */
			skb->data[LP_PKT_LEN - 1] = j;

			if (hinic3_lb_xmit_frame(skb, netdev)) {
				dev_kfree_skb_any(skb);
				dev_kfree_skb_any(skb_tmp);
				nicif_err(nic_dev, drv, netdev,
					  "Xmit pkt failed for loopback test\n");
				return -EBUSY;
			}
		}

		/* wait till all pkts received to RX buffer */
		msleep(HINIC3_WAIT_PKTS_TO_RX_BUFFER);

		for (j = 0; j < LP_PKT_CNT; j++) {
			if (memcmp((lb_test_rx_buf + (j * LP_PKT_LEN)),
				   skb_tmp->data, (LP_PKT_LEN - 1)) ||
				   (*(lb_test_rx_buf + ((j * LP_PKT_LEN) +
				   (LP_PKT_LEN - 1))) != j)) {
				dev_kfree_skb_any(skb_tmp);
				nicif_err(nic_dev, drv, netdev,
					  "Compare pkt failed in loopback test(index=0x%02x, data[%d]=0x%02x)\n",
					  (j + (i * LP_PKT_CNT)),
					  (LP_PKT_LEN - 1),
					  *(lb_test_rx_buf +
					  (((j * LP_PKT_LEN) +
					  (LP_PKT_LEN - 1)))));
				return -EIO;
			}
		}
	}

	dev_kfree_skb_any(skb_tmp);
	nicif_info(nic_dev, drv, netdev, "Loopback test succeed.\n");
	return 0;
}

enum diag_test_index {
	INTERNAL_LP_TEST = 0,
	EXTERNAL_LP_TEST = 1,
	DIAG_TEST_MAX = 2,
};

#define HINIC3_INTERNAL_LP_MODE 5
static int do_lp_test(struct hinic3_nic_dev *nic_dev, u32 *flags, u32 test_time,
		      enum diag_test_index *test_index)
{
	struct net_device *netdev = nic_dev->netdev;
	u8 *lb_test_rx_buf = NULL;
	int err = 0;

	if (!(*flags & ETH_TEST_FL_EXTERNAL_LB)) {
		*test_index = INTERNAL_LP_TEST;
		if (hinic3_set_loopback_mode(nic_dev->hwdev,
					     HINIC3_INTERNAL_LP_MODE, true)) {
			nicif_err(nic_dev, drv, netdev,
				  "Failed to set port loopback mode before loopback test\n");
			return -EFAULT;
		}

		/* suspend 5000 ms, waiting for port to stop receiving frames */
		msleep(5000);
	} else {
		*test_index = EXTERNAL_LP_TEST;
	}

	lb_test_rx_buf = vmalloc(LP_PKT_CNT * LP_PKT_LEN);
	if (!lb_test_rx_buf) {
		err = -ENOMEM;
	} else {
		nic_dev->lb_test_rx_buf = lb_test_rx_buf;
		nic_dev->lb_pkt_len = LP_PKT_LEN;
		set_bit(HINIC3_LP_TEST, &nic_dev->flags);

		if (hinic3_run_lp_test(nic_dev, test_time))
			err = -EFAULT;

		clear_bit(HINIC3_LP_TEST, &nic_dev->flags);
		msleep(HINIC3_WAIT_CLEAR_LP_TEST);
		vfree(lb_test_rx_buf);
		nic_dev->lb_test_rx_buf = NULL;
	}

	if (!(*flags & ETH_TEST_FL_EXTERNAL_LB)) {
		if (hinic3_set_loopback_mode(nic_dev->hwdev,
					     HINIC3_INTERNAL_LP_MODE, false)) {
			nicif_err(nic_dev, drv, netdev,
				  "Failed to cancel port loopback mode after loopback test\n");
			err = -EFAULT;
		}
	} else {
		*flags |= ETH_TEST_FL_EXTERNAL_LB_DONE;
	}

	return err;
}

static void hinic3_lp_test(struct net_device *netdev,
			   struct ethtool_test *eth_test,
			   u64 *data, u32 test_time)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	enum diag_test_index test_index = 0;
	u8 link_status = 0;
	int err;
	u32 test_time_real = test_time;

	/* don't support loopback test when netdev is closed. */
	if (!test_bit(HINIC3_INTF_UP, &nic_dev->flags)) {
		nicif_err(nic_dev, drv, netdev,
			  "Do not support loopback test when netdev is closed\n");
		eth_test->flags |= ETH_TEST_FL_FAILED;
		data[PORT_DOWN_ERR_IDX] = 1;
		return;
	}
	if (test_time_real == 0)
		test_time_real = LP_DEFAULT_TIME;

	netif_carrier_off(netdev);
	netif_tx_disable(netdev);

	err = do_lp_test(nic_dev, &eth_test->flags,
			 test_time_real, &test_index);
	if (err) {
		eth_test->flags |= ETH_TEST_FL_FAILED;
		data[test_index] = 1;
	}

	netif_tx_wake_all_queues(netdev);

	err = hinic3_get_link_state(nic_dev->hwdev, &link_status);
	if (!err && link_status)
		netif_carrier_on(netdev);
}

static void hinic3_diag_test(struct net_device *netdev,
			     struct ethtool_test *eth_test, u64 *data)
{
	memset(data, 0, DIAG_TEST_MAX * sizeof(u64));

	hinic3_lp_test(netdev, eth_test, data, 0);
}

#if defined(ETHTOOL_GFECPARAM) && defined(ETHTOOL_SFECPARAM)
static int hinic3_get_fecparam(struct net_device *netdev,
			       struct ethtool_fecparam *fecparam)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	u8 advertised_fec = 0;
	u8 supported_fec = 0;
	int err;

	if (fecparam->cmd != ETHTOOL_GFECPARAM) {
		nicif_err(nic_dev, drv, netdev,
			  "get fecparam cmd err.exp:0x%x,real:0x%x\n",
			  ETHTOOL_GFECPARAM, fecparam->cmd);
		return -EINVAL;
	}

	err = get_fecparam(nic_dev->hwdev, &advertised_fec, &supported_fec);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Get fec param failed\n");
		return err;
	}
	fecparam->active_fec = (u32)advertised_fec;
	fecparam->fec = (u32)supported_fec;

	nicif_info(nic_dev, drv, netdev, "Get fec param success\n");
	return 0;
}

static int hinic3_set_fecparam(struct net_device *netdev,
			       struct ethtool_fecparam *fecparam)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	int err;

	if (fecparam->cmd != ETHTOOL_SFECPARAM) {
		nicif_err(nic_dev, drv, netdev,
			  "Set fecparam cmd err.exp:0x%x,real:0x%x\n",
			  ETHTOOL_SFECPARAM, fecparam->cmd);
	return -EINVAL;
	}

	err = set_fecparam(nic_dev->hwdev, (u8)fecparam->fec);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Set fec param failed\n");
		return err;
	}

	nicif_info(nic_dev, drv, netdev, "Set fec param success\n");
	return 0;
}
#endif

static const struct ethtool_ops hinic3_ethtool_ops = {
#ifdef SUPPORTED_COALESCE_PARAMS
	.supported_coalesce_params = ETHTOOL_COALESCE_USECS |
			ETHTOOL_COALESCE_PKT_RATE_RX_USECS |
			ETHTOOL_COALESCE_MAX_FRAMES |
			ETHTOOL_COALESCE_USECS_LOW_HIGH |
			ETHTOOL_COALESCE_MAX_FRAMES_LOW_HIGH,
#endif
#ifdef ETHTOOL_GLINKSETTINGS
#ifndef XENSERVER_HAVE_NEW_ETHTOOL_OPS
	.get_link_ksettings = hinic3_get_link_ksettings,
	.set_link_ksettings = hinic3_set_link_ksettings,
#endif
#endif
#ifndef HAVE_NEW_ETHTOOL_LINK_SETTINGS_ONLY
	.get_settings = hinic3_get_settings,
	.set_settings = hinic3_set_settings,
#endif

	.get_drvinfo = hinic3_get_drvinfo,
	.get_msglevel = hinic3_get_msglevel,
	.set_msglevel = hinic3_set_msglevel,
	.nway_reset = hinic3_nway_reset,
#ifdef CONFIG_MODULE_PROF
	.get_link = hinic3_get_link,
#else
	.get_link = ethtool_op_get_link,
#endif
	.get_ringparam = hinic3_get_ringparam,
	.set_ringparam = hinic3_set_ringparam,
	.get_pauseparam = hinic3_get_pauseparam,
	.set_pauseparam = hinic3_set_pauseparam,
	.get_sset_count = hinic3_get_sset_count,
	.get_ethtool_stats = hinic3_get_ethtool_stats,
	.get_strings = hinic3_get_strings,

	.self_test = hinic3_diag_test,

#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
#ifdef HAVE_ETHTOOL_SET_PHYS_ID
	.set_phys_id = hinic3_set_phys_id,
#else
	.phys_id = hinic3_phys_id,
#endif
#endif

	.get_coalesce = hinic3_get_coalesce,
	.set_coalesce = hinic3_set_coalesce,
#if defined(ETHTOOL_PERQUEUE) && defined(ETHTOOL_GCOALESCE)
	.get_per_queue_coalesce = hinic3_get_per_queue_coalesce,
	.set_per_queue_coalesce = hinic3_set_per_queue_coalesce,
#endif

#if defined(ETHTOOL_GFECPARAM) && defined(ETHTOOL_SFECPARAM)
	.get_fecparam = hinic3_get_fecparam,
	.set_fecparam = hinic3_set_fecparam,
#endif
	.get_rxnfc = hinic3_get_rxnfc,
	.set_rxnfc = hinic3_set_rxnfc,
	.get_priv_flags = hinic3_get_priv_flags,
	.set_priv_flags = hinic3_set_priv_flags,

#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
	.get_channels = hinic3_get_channels,
	.set_channels = hinic3_set_channels,

#ifdef ETHTOOL_GMODULEEEPROM
	.get_module_info = hinic3_get_module_info,
	.get_module_eeprom = hinic3_get_module_eeprom,
#endif

#ifndef NOT_HAVE_GET_RXFH_INDIR_SIZE
	.get_rxfh_indir_size = hinic3_get_rxfh_indir_size,
#endif

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
	.get_rxfh_key_size = hinic3_get_rxfh_key_size,
	.get_rxfh = hinic3_get_rxfh,
	.set_rxfh = hinic3_set_rxfh,
#else
	.get_rxfh_indir = hinic3_get_rxfh_indir,
	.set_rxfh_indir = hinic3_set_rxfh_indir,
#endif

#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
};

#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
static const struct ethtool_ops_ext hinic3_ethtool_ops_ext = {
	.size	= sizeof(struct ethtool_ops_ext),
	.set_phys_id = hinic3_set_phys_id,
	.get_channels = hinic3_get_channels,
	.set_channels = hinic3_set_channels,
#ifdef ETHTOOL_GMODULEEEPROM
	.get_module_info = hinic3_get_module_info,
	.get_module_eeprom = hinic3_get_module_eeprom,
#endif

#ifndef NOT_HAVE_GET_RXFH_INDIR_SIZE
	.get_rxfh_indir_size = hinic3_get_rxfh_indir_size,
#endif

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
	.get_rxfh_key_size = hinic3_get_rxfh_key_size,
	.get_rxfh = hinic3_get_rxfh,
	.set_rxfh = hinic3_set_rxfh,
#else
	.get_rxfh_indir = hinic3_get_rxfh_indir,
	.set_rxfh_indir = hinic3_set_rxfh_indir,
#endif

};
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */

static const struct ethtool_ops hinic3vf_ethtool_ops = {
#ifdef SUPPORTED_COALESCE_PARAMS
	.supported_coalesce_params = ETHTOOL_COALESCE_USECS |
			ETHTOOL_COALESCE_PKT_RATE_RX_USECS |
			ETHTOOL_COALESCE_MAX_FRAMES |
			ETHTOOL_COALESCE_USECS_LOW_HIGH |
			ETHTOOL_COALESCE_MAX_FRAMES_LOW_HIGH,
#endif
#ifdef ETHTOOL_GLINKSETTINGS
#ifndef XENSERVER_HAVE_NEW_ETHTOOL_OPS
	.get_link_ksettings = hinic3_get_link_ksettings,
#endif
#else
	.get_settings = hinic3_get_settings,
#endif
	.get_drvinfo = hinic3_get_drvinfo,
	.get_msglevel = hinic3_get_msglevel,
	.set_msglevel = hinic3_set_msglevel,
	.get_link = ethtool_op_get_link,
	.get_ringparam = hinic3_get_ringparam,

	.set_ringparam = hinic3_set_ringparam,
	.get_sset_count = hinic3_get_sset_count,
	.get_ethtool_stats = hinic3_get_ethtool_stats,
	.get_strings = hinic3_get_strings,

	.get_coalesce = hinic3_get_coalesce,
	.set_coalesce = hinic3_set_coalesce,
#if defined(ETHTOOL_PERQUEUE) && defined(ETHTOOL_GCOALESCE)
	.get_per_queue_coalesce = hinic3_get_per_queue_coalesce,
	.set_per_queue_coalesce = hinic3_set_per_queue_coalesce,
#endif

#if defined(ETHTOOL_GFECPARAM) && defined(ETHTOOL_SFECPARAM)
	.get_fecparam = hinic3_get_fecparam,
	.set_fecparam = hinic3_set_fecparam,
#endif

	.get_rxnfc = hinic3_get_rxnfc,
	.set_rxnfc = hinic3_set_rxnfc,
	.get_priv_flags = hinic3_get_priv_flags,
	.set_priv_flags = hinic3_set_priv_flags,

#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
	.get_channels = hinic3_get_channels,
	.set_channels = hinic3_set_channels,

#ifndef NOT_HAVE_GET_RXFH_INDIR_SIZE
	.get_rxfh_indir_size = hinic3_get_rxfh_indir_size,
#endif

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
	.get_rxfh_key_size = hinic3_get_rxfh_key_size,
	.get_rxfh = hinic3_get_rxfh,
	.set_rxfh = hinic3_set_rxfh,
#else
	.get_rxfh_indir = hinic3_get_rxfh_indir,
	.set_rxfh_indir = hinic3_set_rxfh_indir,
#endif

#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
};

#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
static const struct ethtool_ops_ext hinic3vf_ethtool_ops_ext = {
	.size	= sizeof(struct ethtool_ops_ext),
	.get_channels = hinic3_get_channels,
	.set_channels = hinic3_set_channels,

#ifndef NOT_HAVE_GET_RXFH_INDIR_SIZE
	.get_rxfh_indir_size = hinic3_get_rxfh_indir_size,
#endif

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
	.get_rxfh_key_size = hinic3_get_rxfh_key_size,
	.get_rxfh = hinic3_get_rxfh,
	.set_rxfh = hinic3_set_rxfh,
#else
	.get_rxfh_indir = hinic3_get_rxfh_indir,
	.set_rxfh_indir = hinic3_set_rxfh_indir,
#endif

};
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */

void hinic3_set_ethtool_ops(struct net_device *netdev)
{
	SET_ETHTOOL_OPS(netdev, &hinic3_ethtool_ops);
#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
	set_ethtool_ops_ext(netdev, &hinic3_ethtool_ops_ext);
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
}

void hinic3vf_set_ethtool_ops(struct net_device *netdev)
{
	SET_ETHTOOL_OPS(netdev, &hinic3vf_ethtool_ops);
#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
	set_ethtool_ops_ext(netdev, &hinic3vf_ethtool_ops_ext);
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
}

