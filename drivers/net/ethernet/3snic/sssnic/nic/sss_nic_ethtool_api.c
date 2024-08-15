// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

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

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_tx.h"
#include "sss_nic_rx.h"
#include "sss_nic_rss.h"
#include "sss_nic_ethtool_stats.h"
#include "sss_nic_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_cfg_define.h"
#include "sss_nic_ethtool_api.h"

#define SSSNIC_COALESCE_PENDING_LIMIT_UNIT	8
#define	SSSNIC_COALESCE_TIMER_CFG_UNIT		5
#define SSSNIC_COALESCE_MAX_PENDING_LIMIT	(255 * SSSNIC_COALESCE_PENDING_LIMIT_UNIT)
#define SSSNIC_COALESCE_MAX_TIMER_CFG		(255 * SSSNIC_COALESCE_TIMER_CFG_UNIT)
#define SSSNIC_WAIT_PKTS_TO_RX_BUFFER		200
#define SSSNIC_WAIT_CLEAR_LP_TEST			100

#define SSSNIC_CHECK_COALESCE_ALIGN(coal, item, unit)				\
do {									\
	if ((coal)->item % (unit))					\
		nicif_warn(nic_dev, drv, netdev,			\
			   "%s in %d units, change to %u\n",		\
			   #item, (unit), ((coal)->item -	\
					   (coal)->item % (unit))); \
} while (0)

#define SSSNIC_CHECK_COALESCE_CHANGED(coal, item, unit, ori_val, obj_str)	\
do {									\
	if (((coal)->item / (unit)) != (ori_val))			\
		nicif_info(nic_dev, drv, netdev,			\
			   "Change %s from %d to %u %s\n",		\
			   #item, (ori_val) * (unit),			\
			   ((coal)->item - (coal)->item % (unit)),	\
			    (obj_str));			\
} while (0)

#define SSSNIC_CHECK_PKT_RATE_CHANGED(coal, item, ori_val, obj_str)		\
do {									\
	if ((coal)->item != (ori_val))					\
		nicif_info(nic_dev, drv, netdev,			\
			   "Change %s from %llu to %u %s\n",		\
			   #item, (ori_val), (coal)->item, (obj_str));	\
} while (0)

#define SSSNIC_PORT_DOWN_ERR_ID  0
#define SSSNIC_LP_DEF_TIME    5 /* seconds */

#define SSSNIC_TEST_TIME_MULTIPLE 5

#define SSSNIC_INTERNAL_LP_MODE 5

#define SSSNIC_WAIT_LOOP_TEST_FINISH_TIMEOUT	5000

void sss_nic_update_qp_depth(struct sss_nic_dev *nic_dev,
			     u32 sq_depth, u32 rq_depth)
{
	u16 i;

	nic_dev->qp_res.sq_depth = sq_depth;
	nic_dev->qp_res.rq_depth = rq_depth;
	for (i = 0; i < nic_dev->max_qp_num; i++) {
		nic_dev->sq_desc_group[i].q_depth = sq_depth;
		nic_dev->rq_desc_group[i].q_depth = rq_depth;
		nic_dev->sq_desc_group[i].qid_mask = sq_depth - 1;
		nic_dev->rq_desc_group[i].qid_mask = rq_depth - 1;
	}
}

int sss_nic_check_ringparam_valid(struct net_device *netdev,
				  const struct ethtool_ringparam *ringparam)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (ringparam->rx_mini_pending != 0 || ringparam->rx_jumbo_pending != 0) {
		nicif_err(nic_dev, drv, netdev,
			  "Unsupport rx_mini_pending: %u, rx_jumbo_pending: %u\n",
			  ringparam->rx_mini_pending, ringparam->rx_jumbo_pending);
		return -EINVAL;
	}

	if (ringparam->tx_pending < SSSNIC_MIN_QUEUE_DEPTH ||
	    ringparam->tx_pending > SSSNIC_MAX_TX_QUEUE_DEPTH ||
	    ringparam->rx_pending < SSSNIC_MIN_QUEUE_DEPTH ||
	    ringparam->rx_pending > SSSNIC_MAX_RX_QUEUE_DEPTH) {
		nicif_err(nic_dev, drv, netdev,
			  "Queue depth out of range tx[%d-%d] rx[%d-%d]\n",
			  ringparam->tx_pending, ringparam->tx_pending,
			  ringparam->rx_pending, ringparam->rx_pending);
		return -EINVAL;
	}

	return 0;
}

void sss_nic_intr_coal_to_ethtool_coal(struct ethtool_coalesce *ethtool_coal,
				       struct sss_nic_intr_coal_info *nic_coal)
{
	ethtool_coal->rx_coalesce_usecs = nic_coal->coalesce_timer *
					  SSSNIC_COALESCE_TIMER_CFG_UNIT;
	ethtool_coal->tx_coalesce_usecs = ethtool_coal->rx_coalesce_usecs;
	ethtool_coal->rx_coalesce_usecs_low = nic_coal->rx_usecs_low *
					      SSSNIC_COALESCE_TIMER_CFG_UNIT;
	ethtool_coal->rx_coalesce_usecs_high = nic_coal->rx_usecs_high *
					       SSSNIC_COALESCE_TIMER_CFG_UNIT;

	ethtool_coal->rx_max_coalesced_frames = nic_coal->pending_limt *
						SSSNIC_COALESCE_PENDING_LIMIT_UNIT;
	ethtool_coal->tx_max_coalesced_frames =
		ethtool_coal->rx_max_coalesced_frames;
	ethtool_coal->rx_max_coalesced_frames_low =
		nic_coal->rx_pending_limt_low *
		SSSNIC_COALESCE_PENDING_LIMIT_UNIT;
	ethtool_coal->rx_max_coalesced_frames_high =
		nic_coal->rx_pending_limt_high *
		SSSNIC_COALESCE_PENDING_LIMIT_UNIT;

	ethtool_coal->pkt_rate_low = (u32)nic_coal->pkt_rate_low;
	ethtool_coal->pkt_rate_high = (u32)nic_coal->pkt_rate_high;
}

int sss_nic_ethtool_get_coalesce(struct net_device *netdev,
				 struct ethtool_coalesce *ethtool_coal, u16 queue)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	struct sss_nic_intr_coal_info *intr_coal_info = NULL;

	if (queue == SSSNIC_COALESCE_ALL_QUEUE) {
		/* get tx/rx irq0 as default parameters */
		intr_coal_info = &nic_dev->coal_info[0];
	} else {
		if (queue >= nic_dev->qp_res.qp_num) {
			nicif_err(nic_dev, drv, netdev,
				  "Invalid queue_id: %u\n", queue);
			return -EINVAL;
		}
		intr_coal_info = &nic_dev->coal_info[queue];
	}

	sss_nic_intr_coal_to_ethtool_coal(ethtool_coal, intr_coal_info);
	ethtool_coal->use_adaptive_rx_coalesce =
		nic_dev->use_adaptive_rx_coalesce;

	return 0;
}

int sss_nic_set_hw_intr_coal(struct sss_nic_dev *nic_dev,
			     u16 qid, struct sss_nic_intr_coal_info *coal)
{
	struct sss_nic_intr_coal_info *intr_coal_info = NULL;
	struct sss_irq_cfg irq_cfg = {0};
	struct net_device *netdev = nic_dev->netdev;
	int ret;

	intr_coal_info = &nic_dev->coal_info[qid];
	if (intr_coal_info->coalesce_timer != coal->coalesce_timer ||
	    intr_coal_info->pending_limt != coal->pending_limt)
		intr_coal_info->user_set_intr_coal_flag = 1;

	intr_coal_info->coalesce_timer = coal->coalesce_timer;
	intr_coal_info->pending_limt = coal->pending_limt;
	intr_coal_info->rx_pending_limt_low = coal->rx_pending_limt_low;
	intr_coal_info->rx_pending_limt_high = coal->rx_pending_limt_high;
	intr_coal_info->pkt_rate_low = coal->pkt_rate_low;
	intr_coal_info->pkt_rate_high = coal->pkt_rate_high;
	intr_coal_info->rx_usecs_low = coal->rx_usecs_low;
	intr_coal_info->rx_usecs_high = coal->rx_usecs_high;

	if (!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_INTF_UP) ||
	    qid >= nic_dev->qp_res.qp_num ||
	    nic_dev->use_adaptive_rx_coalesce != 0)
		return 0;

	irq_cfg.msix_id = nic_dev->qp_res.irq_cfg[qid].msix_id;
	irq_cfg.lli_set = 0;
	irq_cfg.coalesc_intr_set = 1;
	irq_cfg.coalesc_timer = intr_coal_info->coalesce_timer;
	irq_cfg.resend_timer = intr_coal_info->resend_timer;
	irq_cfg.pending = intr_coal_info->pending_limt;
	nic_dev->rq_desc_group[qid].last_coal_timer =
		intr_coal_info->coalesce_timer;
	nic_dev->rq_desc_group[qid].last_pending_limt = intr_coal_info->pending_limt;
	ret = sss_chip_set_msix_attr(nic_dev->hwdev, irq_cfg,
				     SSS_CHANNEL_NIC);
	if (ret != 0)
		nicif_warn(nic_dev, drv, netdev,
			   "Fail to set queue%u coalesce", qid);

	return ret;
}

int sss_nic_check_coal_param_support(struct net_device *netdev,
				     const struct ethtool_coalesce *coal)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (coal->cmd & ~SSSNIC_SUPPORTED_COALESCE_PARAMS) {
		nicif_err(nic_dev, drv, netdev,
			  "Only support to change rx/tx-usecs and rx/tx-frames\n");

		return -EOPNOTSUPP;
	}

	return 0;
}

int sss_nic_check_coal_param_valid(struct net_device *netdev,
				   const struct ethtool_coalesce *coal)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (coal->rx_coalesce_usecs != coal->tx_coalesce_usecs) {
		nicif_err(nic_dev, drv, netdev,
			  "Coal param: tx-usecs must be equal to rx-usecs\n");
		return -EINVAL;
	}

	if (coal->rx_max_coalesced_frames != coal->tx_max_coalesced_frames) {
		nicif_err(nic_dev, drv, netdev,
			  "Coal param: tx-frames must be equal to rx-frames\n");
		return -EINVAL;
	}

	if (coal->rx_coalesce_usecs > SSSNIC_COALESCE_MAX_TIMER_CFG) {
		nicif_err(nic_dev, drv, netdev,
			  "Coal param: rx_coalesce_usecs out of range[%d-%d]\n", 0,
			  SSSNIC_COALESCE_MAX_TIMER_CFG);
		return -EOPNOTSUPP;
	}

	if (coal->rx_coalesce_usecs_low > SSSNIC_COALESCE_MAX_TIMER_CFG) {
		nicif_err(nic_dev, drv, netdev,
			  "Coal param: rx_coalesce_usecs_low out of range[%d-%d]\n", 0,
			  SSSNIC_COALESCE_MAX_TIMER_CFG);
		return -EOPNOTSUPP;
	}

	if (coal->rx_coalesce_usecs_high > SSSNIC_COALESCE_MAX_TIMER_CFG) {
		nicif_err(nic_dev, drv, netdev,
			  "Coal param: rx_coalesce_usecs_high out of range[%d-%d]\n", 0,
			  SSSNIC_COALESCE_MAX_TIMER_CFG);
		return -EOPNOTSUPP;
	}

	if (coal->rx_max_coalesced_frames > SSSNIC_COALESCE_MAX_PENDING_LIMIT) {
		nicif_err(nic_dev, drv, netdev,
			  "Coal param: rx_max_coalesced_frames out of range[%d-%d]\n", 0,
			  SSSNIC_COALESCE_MAX_PENDING_LIMIT);
		return -EOPNOTSUPP;
	}

	if (coal->rx_max_coalesced_frames_low >
	    SSSNIC_COALESCE_MAX_PENDING_LIMIT) {
		nicif_err(nic_dev, drv, netdev,
			  "Coal param: rx_max_coalesced_frames_low out of range[%d-%d]\n",
			  0, SSSNIC_COALESCE_MAX_PENDING_LIMIT);
		return -EOPNOTSUPP;
	}

	if (coal->rx_max_coalesced_frames_high >
	    SSSNIC_COALESCE_MAX_PENDING_LIMIT) {
		nicif_err(nic_dev, drv, netdev,
			  "Coal param: rx_max_coalesced_frames_high out of range[%d-%d]\n",
			  0, SSSNIC_COALESCE_MAX_PENDING_LIMIT);
		return -EOPNOTSUPP;
	}

	return 0;
}

int sss_nic_check_coal_param_range(struct net_device *netdev,
				   const struct ethtool_coalesce *coal)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (coal->rx_coalesce_usecs_low / SSSNIC_COALESCE_TIMER_CFG_UNIT >=
	    coal->rx_coalesce_usecs_high / SSSNIC_COALESCE_TIMER_CFG_UNIT) {
		nicif_err(nic_dev, drv, netdev,
			  "Coal param: coalesce_usecs_high(%u) must more than coalesce_usecs_low(%u)\n",
			  coal->rx_coalesce_usecs_high,
			  coal->rx_coalesce_usecs_low);
		return -EOPNOTSUPP;
	}

	if (coal->rx_max_coalesced_frames_low / SSSNIC_COALESCE_PENDING_LIMIT_UNIT >=
	    coal->rx_max_coalesced_frames_high / SSSNIC_COALESCE_PENDING_LIMIT_UNIT) {
		nicif_err(nic_dev, drv, netdev,
			  "Coal param: coalesced_frames_high(%u) must more than coalesced_frames_low(%u)\n",
			  coal->rx_max_coalesced_frames_high,
			  coal->rx_max_coalesced_frames_low);
		return -EOPNOTSUPP;
	}

	if (coal->pkt_rate_low >= coal->pkt_rate_high) {
		nicif_err(nic_dev, drv, netdev,
			  "Coal param: pkt_rate_high(%u) must more than pkt_rate_low(%u)\n",
			  coal->pkt_rate_high,
			  coal->pkt_rate_low);
		return -EOPNOTSUPP;
	}

	return 0;
}

int sss_nic_coalesce_check(struct net_device *netdev,
			   const struct ethtool_coalesce *coal)
{
	int ret;

	ret = sss_nic_check_coal_param_support(netdev, coal);
	if (ret != 0)
		return ret;

	ret = sss_nic_check_coal_param_valid(netdev, coal);
	if (ret != 0)
		return ret;

	ret = sss_nic_check_coal_param_range(netdev, coal);
	if (ret != 0)
		return ret;

	return 0;
}

int sss_nic_set_coal_param_to_hw(struct sss_nic_dev *nic_dev,
				 struct sss_nic_intr_coal_info *intr_coal_info, u16 queue)
{
	u16 i;

	if (queue < nic_dev->qp_res.qp_num) {
		sss_nic_set_hw_intr_coal(nic_dev, queue, intr_coal_info);
		return 0;
	} else if (queue != SSSNIC_COALESCE_ALL_QUEUE) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Invalid queue_id: %u\n", queue);
		return -EINVAL;
	}

	for (i = 0; i < nic_dev->max_qp_num; i++)
		sss_nic_set_hw_intr_coal(nic_dev, i, intr_coal_info);

	return 0;
}

void sss_nic_coalesce_align_check(struct net_device *netdev,
				  struct ethtool_coalesce *coal)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	SSSNIC_CHECK_COALESCE_ALIGN(coal, rx_coalesce_usecs,
				    SSSNIC_COALESCE_TIMER_CFG_UNIT);
	SSSNIC_CHECK_COALESCE_ALIGN(coal, rx_coalesce_usecs_low,
				    SSSNIC_COALESCE_TIMER_CFG_UNIT);
	SSSNIC_CHECK_COALESCE_ALIGN(coal, rx_coalesce_usecs_high,
				    SSSNIC_COALESCE_TIMER_CFG_UNIT);
	SSSNIC_CHECK_COALESCE_ALIGN(coal, rx_max_coalesced_frames,
				    SSSNIC_COALESCE_PENDING_LIMIT_UNIT);
	SSSNIC_CHECK_COALESCE_ALIGN(coal, rx_max_coalesced_frames_low,
				    SSSNIC_COALESCE_PENDING_LIMIT_UNIT);
	SSSNIC_CHECK_COALESCE_ALIGN(coal, rx_max_coalesced_frames_high,
				    SSSNIC_COALESCE_PENDING_LIMIT_UNIT);
}

void sss_nic_coalesce_change_check(struct net_device *netdev,
				   struct ethtool_coalesce *coal, u16 queue)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	struct sss_nic_intr_coal_info *intr_coal_info = NULL;
	char obj_str[32] = {0};

	if (queue == SSSNIC_COALESCE_ALL_QUEUE) {
		intr_coal_info = &nic_dev->coal_info[0];
		snprintf(obj_str, sizeof(obj_str), "for netdev");
	} else {
		intr_coal_info = &nic_dev->coal_info[queue];
		snprintf(obj_str, sizeof(obj_str), "for queue %u", queue);
	}

	SSSNIC_CHECK_COALESCE_CHANGED(coal, rx_coalesce_usecs,
				      SSSNIC_COALESCE_TIMER_CFG_UNIT,
				      intr_coal_info->coalesce_timer, obj_str);
	SSSNIC_CHECK_COALESCE_CHANGED(coal, rx_coalesce_usecs_low,
				      SSSNIC_COALESCE_TIMER_CFG_UNIT,
				      intr_coal_info->rx_usecs_low, obj_str);
	SSSNIC_CHECK_COALESCE_CHANGED(coal, rx_coalesce_usecs_high,
				      SSSNIC_COALESCE_TIMER_CFG_UNIT,
				      intr_coal_info->rx_usecs_high, obj_str);
	SSSNIC_CHECK_COALESCE_CHANGED(coal, rx_max_coalesced_frames,
				      SSSNIC_COALESCE_PENDING_LIMIT_UNIT,
				      intr_coal_info->pending_limt, obj_str);
	SSSNIC_CHECK_COALESCE_CHANGED(coal, rx_max_coalesced_frames_low,
				      SSSNIC_COALESCE_PENDING_LIMIT_UNIT,
				      intr_coal_info->rx_pending_limt_low, obj_str);
	SSSNIC_CHECK_COALESCE_CHANGED(coal, rx_max_coalesced_frames_high,
				      SSSNIC_COALESCE_PENDING_LIMIT_UNIT,
				      intr_coal_info->rx_pending_limt_high, obj_str);
	SSSNIC_CHECK_PKT_RATE_CHANGED(coal, pkt_rate_low,
				      intr_coal_info->pkt_rate_low, obj_str);
	SSSNIC_CHECK_PKT_RATE_CHANGED(coal, pkt_rate_high,
				      intr_coal_info->pkt_rate_high, obj_str);
}

void sss_nic_ethtool_coalesce_to_intr_coal_info(struct sss_nic_intr_coal_info *nic_coal,
						struct ethtool_coalesce *ethtool_coal)
{
	nic_coal->coalesce_timer =
		(u8)(ethtool_coal->rx_coalesce_usecs / SSSNIC_COALESCE_TIMER_CFG_UNIT);
	nic_coal->pending_limt = (u8)(ethtool_coal->rx_max_coalesced_frames /
				      SSSNIC_COALESCE_PENDING_LIMIT_UNIT);
	nic_coal->pkt_rate_low = ethtool_coal->pkt_rate_low;
	nic_coal->pkt_rate_high = ethtool_coal->pkt_rate_high;
	nic_coal->rx_usecs_low =
		(u8)(ethtool_coal->rx_coalesce_usecs_low / SSSNIC_COALESCE_TIMER_CFG_UNIT);
	nic_coal->rx_usecs_high =
		(u8)(ethtool_coal->rx_coalesce_usecs_high / SSSNIC_COALESCE_TIMER_CFG_UNIT);
	nic_coal->rx_pending_limt_low =
		(u8)(ethtool_coal->rx_max_coalesced_frames_low /
		     SSSNIC_COALESCE_PENDING_LIMIT_UNIT);
	nic_coal->rx_pending_limt_high =
		(u8)(ethtool_coal->rx_max_coalesced_frames_high /
		     SSSNIC_COALESCE_PENDING_LIMIT_UNIT);
}

int sss_nic_ethtool_set_coalesce(struct net_device *netdev,
				 struct ethtool_coalesce *coal, u16 queue)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	struct sss_nic_intr_coal_info intr_coal_info = {0};
	u32 last_adaptive_rx;
	int ret = 0;

	ret = sss_nic_coalesce_check(netdev, coal);
	if (ret != 0)
		return ret;

	sss_nic_coalesce_align_check(netdev, coal);
	sss_nic_coalesce_change_check(netdev, coal, queue);

	sss_nic_ethtool_coalesce_to_intr_coal_info(&intr_coal_info, coal);

	last_adaptive_rx = nic_dev->use_adaptive_rx_coalesce;
	nic_dev->use_adaptive_rx_coalesce = coal->use_adaptive_rx_coalesce;

	if (nic_dev->use_adaptive_rx_coalesce == 0 &&
	    (intr_coal_info.coalesce_timer == 0 ||
	     intr_coal_info.pending_limt == 0))
		nicif_warn(nic_dev, drv, netdev, "Coalesce will be disabled\n");

	if (SSS_CHANNEL_RES_VALID(nic_dev) != 0) {
		if (nic_dev->use_adaptive_rx_coalesce == 0)
			cancel_delayed_work_sync(&nic_dev->moderation_task);
		else if (last_adaptive_rx == 0)
			queue_delayed_work(nic_dev->workq,
					   &nic_dev->moderation_task,
					   SSSNIC_MODERATONE_DELAY);
	}

	return sss_nic_set_coal_param_to_hw(nic_dev, &intr_coal_info, queue);
}

void sss_nic_module_type_sfp(struct ethtool_modinfo *modinfo,
			     u8 sfp_type_ext)
{
	modinfo->type = ETH_MODULE_SFF_8472;
	modinfo->eeprom_len = ETH_MODULE_SFF_8472_LEN;
}

void sss_nic_module_type_qsfp(struct ethtool_modinfo *modinfo,
			      u8 sfp_type_ext)
{
	modinfo->type = ETH_MODULE_SFF_8436;
	modinfo->eeprom_len = ETH_MODULE_SFF_8436_MAX_LEN;
}

void sss_nic_module_type_qsfp_plus(struct ethtool_modinfo *modinfo, u8 sfp_type_ext)
{
	if (sfp_type_ext < SSSNIC_SFP_TYPE_EXT_FLAG) {
		modinfo->type = ETH_MODULE_SFF_8436;
		modinfo->eeprom_len = ETH_MODULE_SFF_8436_MAX_LEN;
	} else {
		modinfo->type = ETH_MODULE_SFF_8636;
		modinfo->eeprom_len = ETH_MODULE_SFF_8636_MAX_LEN;
	}
}

void sss_nic_module_type_qsfp28(struct ethtool_modinfo *modinfo,
				u8 sfp_type_ext)
{
	modinfo->type = ETH_MODULE_SFF_8636;
	modinfo->eeprom_len = ETH_MODULE_SFF_8636_MAX_LEN;
}

int sss_nic_set_rq_recovery_flag(struct net_device *netdev, u32 flag)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (flag & SSSNIC_PRIV_FLAG_RQ_RECOVERY) {
		if (!SSSNIC_SUPPORT_RXQ_RECOVERY(nic_dev->nic_io)) {
			nicif_info(nic_dev, drv, netdev, "Unsupport open rq recovery\n");
			return -EOPNOTSUPP;
		}

		if (SSSNIC_TEST_SET_NIC_DEV_FLAG(nic_dev, SSSNIC_RXQ_RECOVERY))
			return 0;
		queue_delayed_work(nic_dev->workq, &nic_dev->rq_watchdog_work, HZ);
		nicif_info(nic_dev, drv, netdev, "Succss to open rq recovery\n");
	} else {
		if (!SSSNIC_TEST_CLEAR_NIC_DEV_FLAG(nic_dev, SSSNIC_RXQ_RECOVERY))
			return 0;
		cancel_delayed_work_sync(&nic_dev->rq_watchdog_work);
		nicif_info(nic_dev, drv, netdev, "Success to close rq recovery\n");
	}

	return 0;
}

int sss_nic_set_symm_rss_flag(struct net_device *netdev, u32 flag)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if ((flag & SSSNIC_PRIV_FLAG_SYMM_RSS) != 0) {
		if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE)) {
			nicif_err(nic_dev, drv, netdev, "Fail to open Symmetric RSS while DCB is enabled\n");
			return -EOPNOTSUPP;
		}

		if (!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_RSS_ENABLE)) {
			nicif_err(nic_dev, drv, netdev, "Fail to open Symmetric RSS while RSS is disabled\n");
			return -EOPNOTSUPP;
		}

		SSSNIC_SET_NIC_DEV_FLAG(nic_dev, SSSNIC_SAME_RXTX);
	} else {
		SSSNIC_CLEAR_NIC_DEV_FLAG(nic_dev, SSSNIC_SAME_RXTX);
	}

	return 0;
}

void sss_nic_force_link_up(struct sss_nic_dev *nic_dev)
{
	if (SSSNIC_TEST_SET_NIC_DEV_FLAG(nic_dev, SSSNIC_FORCE_LINK_UP))
		return;

	if (!SSS_CHANNEL_RES_VALID(nic_dev))
		return;

	if (netif_carrier_ok(nic_dev->netdev))
		return;

	nic_dev->link_status = true;
	netif_carrier_on(nic_dev->netdev);
	nicif_info(nic_dev, link, nic_dev->netdev, "Set link up\n");

	if (!SSSNIC_FUNC_IS_VF(nic_dev->hwdev))
		sss_nic_notify_all_vf_link_state(nic_dev->nic_io, nic_dev->link_status);
}

int sss_nic_force_link_down(struct sss_nic_dev *nic_dev)
{
	int ret;
	u8 link_status = 0;

	if (!SSSNIC_TEST_CLEAR_NIC_DEV_FLAG(nic_dev, SSSNIC_FORCE_LINK_UP))
		return 0;

	if (!SSS_CHANNEL_RES_VALID(nic_dev))
		return 0;

	ret = sss_nic_get_hw_link_state(nic_dev, &link_status);
	if (ret != 0) {
		nicif_err(nic_dev, link, nic_dev->netdev, "Fail to get link state: %d\n", ret);
		return ret;
	}

	nic_dev->link_status = link_status;

	if (link_status != 0) {
		if (netif_carrier_ok(nic_dev->netdev))
			return 0;

		netif_carrier_on(nic_dev->netdev);
		nicif_info(nic_dev, link, nic_dev->netdev, "Link state is up\n");
	} else {
		if (!netif_carrier_ok(nic_dev->netdev))
			return 0;

		netif_carrier_off(nic_dev->netdev);
		nicif_info(nic_dev, link, nic_dev->netdev, "Link state is down\n");
	}

	if (!SSSNIC_FUNC_IS_VF(nic_dev->hwdev))
		sss_nic_notify_all_vf_link_state(nic_dev->nic_io, nic_dev->link_status);

	return ret;
}

int sss_nic_set_force_link_flag(struct net_device *netdev, u32 flag)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	int ret = 0;

	if ((flag & SSSNIC_PRIV_FLAG_LINK_UP) != 0)
		sss_nic_force_link_up(nic_dev);
	else
		ret = sss_nic_force_link_down(nic_dev);

	return ret;
}

int sss_nic_finish_loop_test(struct sss_nic_dev *nic_dev,
			     struct sk_buff *skb_tmp, u32 test_time)
{
	struct net_device *netdev = nic_dev->netdev;
	u8 *loop_test_rx_buf = nic_dev->loop_test_rx_buf;
	u32 cnt = test_time * SSSNIC_TEST_TIME_MULTIPLE;
	struct sk_buff *skb = NULL;
	int lp_pkt_cnt = nic_dev->loop_pkt_len;
	unsigned char pkt_mark_data;
	u32 i;
	u32 j;

	for (i = 0; i < cnt; i++) {
		nic_dev->loop_test_rx_cnt = 0;
		memset(loop_test_rx_buf, 0, SSSNIC_LP_PKT_CNT * lp_pkt_cnt);

		for (j = 0; j < SSSNIC_LP_PKT_CNT; j++) {
			skb = pskb_copy(skb_tmp, GFP_ATOMIC);
			if (!skb) {
				nicif_err(nic_dev, drv, netdev,
					  "Fail to copy skb for loopback test\n");
				return -ENOMEM;
			}

			/* mark index for every pkt */
			skb->data[lp_pkt_cnt - 1] = j;

			if (sss_nic_loop_start_xmit(skb, netdev) != NETDEV_TX_OK) {
				dev_kfree_skb_any(skb);
				nicif_err(nic_dev, drv, netdev,
					  "Fail to xmit pkt for loopback test\n");
				return -EBUSY;
			}
		}

		/* wait till all pkts received to RX buffer */
		msleep(SSSNIC_WAIT_PKTS_TO_RX_BUFFER);

		for (j = 0; j < SSSNIC_LP_PKT_CNT; j++) {
			pkt_mark_data = *(loop_test_rx_buf + (j * lp_pkt_cnt) + (lp_pkt_cnt - 1));
			if (memcmp((loop_test_rx_buf + (j * lp_pkt_cnt)),
				   skb_tmp->data, (lp_pkt_cnt - 1)) != 0 ||
			    pkt_mark_data != j) {
				nicif_err(nic_dev, drv, netdev,
					  "Fail to compare pkt in loopback test(index=0x%02x, data[%d]=0x%02x)\n",
					  (j + (i * SSSNIC_LP_PKT_CNT)),
					  (lp_pkt_cnt - 1), pkt_mark_data);
				return -EIO;
			}
		}
	}

	return 0;
}

static struct sk_buff *sss_nic_alloc_loop_skb(struct sss_nic_dev *nic_dev)
{
	struct net_device *netdev = nic_dev->netdev;
	struct sk_buff *skb = NULL;
	struct ethhdr *eth_hdr = NULL;
	u8 *test_data = NULL;
	u32 i;

	skb = alloc_skb(nic_dev->loop_pkt_len, GFP_ATOMIC);
	if (!skb)
		return skb;

	eth_hdr = __skb_put(skb, ETH_HLEN);
	eth_hdr->h_proto = htons(ETH_P_ARP);
	ether_addr_copy(eth_hdr->h_dest, nic_dev->netdev->dev_addr);
	eth_zero_addr(eth_hdr->h_source);
	skb_reset_mac_header(skb);

	test_data = __skb_put(skb, nic_dev->loop_pkt_len - ETH_HLEN);
	for (i = ETH_HLEN; i < nic_dev->loop_pkt_len; i++)
		test_data[i] = i & 0xFF;

	skb->queue_mapping = 0;
	skb->dev = netdev;
	skb->protocol = htons(ETH_P_ARP);

	return skb;
}

static int sss_nic_run_loop_test(struct sss_nic_dev *nic_dev, u32 test_time)
{
	struct net_device *netdev = nic_dev->netdev;
	struct sk_buff *skb_tmp = NULL;
	int ret;

	skb_tmp = sss_nic_alloc_loop_skb(nic_dev);
	if (!skb_tmp) {
		nicif_err(nic_dev, drv, netdev,
			  "Fail to create lp test skb for loopback test\n");
		return -ENOMEM;
	}

	ret = sss_nic_finish_loop_test(nic_dev, skb_tmp, test_time);
	if (ret != 0) {
		dev_kfree_skb_any(skb_tmp);
		return ret;
	}

	dev_kfree_skb_any(skb_tmp);
	nicif_info(nic_dev, drv, netdev, "Success to loopback test.\n");
	return 0;
}

static int sss_nic_do_loop_test(struct sss_nic_dev *nic_dev, u32 *flags,
				u32 test_time, enum sss_nic_lp_test_type *test_index)
{
	struct net_device *netdev = nic_dev->netdev;
	int ret = 0;

	if (!(*flags & ETH_TEST_FL_EXTERNAL_LB)) {
		*test_index = SSSNIC_INTERNAL_LP_TEST;
		if (sss_nic_set_loopback_mode(nic_dev,
					      SSSNIC_INTERNAL_LP_MODE, true)) {
			nicif_err(nic_dev, drv, netdev,
				  "Fail to set port loopback mode before loopback test\n");
			return -EFAULT;
		}

		/* suspend 5000 ms, waiting for port to stop receiving frames */
		msleep(SSSNIC_WAIT_LOOP_TEST_FINISH_TIMEOUT);
	} else {
		*test_index = SSSNIC_EXTERNAL_LP_TEST;
	}

	SSSNIC_SET_NIC_DEV_FLAG(nic_dev, SSSNIC_LP_TEST);

	if (sss_nic_run_loop_test(nic_dev, test_time))
		ret = -EFAULT;

	SSSNIC_CLEAR_NIC_DEV_FLAG(nic_dev, SSSNIC_LP_TEST);
	msleep(SSSNIC_WAIT_CLEAR_LP_TEST);

	if (!(*flags & ETH_TEST_FL_EXTERNAL_LB)) {
		if (sss_nic_set_loopback_mode(nic_dev,
					      SSSNIC_INTERNAL_LP_MODE, false)) {
			nicif_err(nic_dev, drv, netdev,
				  "Fail to cancel port loopback mode after loopback test\n");
			ret = -EFAULT;
		}
	} else {
		*flags |= ETH_TEST_FL_EXTERNAL_LB_DONE;
	}
	return ret;
}

void sss_nic_loop_test(struct net_device *netdev,
		       struct ethtool_test *eth_test, u64 *data)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	enum sss_nic_lp_test_type test_type = SSSNIC_INTERNAL_LP_TEST;
	u32 act_test_time = SSSNIC_LP_DEF_TIME;
	u8 link_state = 0;
	int ret;

	if (!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_INTF_UP)) {
		nicif_err(nic_dev, drv, netdev,
			  "Fail to entry loopback test when netdev is closed\n");
		eth_test->flags |= ETH_TEST_FL_FAILED;
		data[SSSNIC_PORT_DOWN_ERR_ID] = 1;
		return;
	}

	netif_carrier_off(netdev);
	netif_tx_disable(netdev);

	ret = sss_nic_do_loop_test(nic_dev, &eth_test->flags, act_test_time, &test_type);
	if (ret) {
		eth_test->flags |= ETH_TEST_FL_FAILED;
		data[test_type] = 1;
	}

	netif_tx_wake_all_queues(netdev);

	ret = sss_nic_get_hw_link_state(nic_dev, &link_state);
	if (!ret && link_state)
		netif_carrier_on(netdev);
}
