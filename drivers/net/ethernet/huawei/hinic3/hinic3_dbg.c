// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/semaphore.h>

#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_mt.h"
#include "hinic3_nic_dev.h"
#include "hinic3_nic_dbg.h"
#include "hinic3_nic_qp.h"
#include "hinic3_rx.h"
#include "hinic3_tx.h"
#include "hinic3_dcb.h"
#include "hinic3_nic.h"
#include "hinic3_bond.h"
#include "nic_mpu_cmd_defs.h"
#include "mag_mpu_cmd.h"
#include "mag_mpu_cmd_defs.h"

typedef int (*nic_driv_module)(struct hinic3_nic_dev *nic_dev,
			       const void *buf_in, u32 in_size,
			       void *buf_out, u32 *out_size);

struct nic_drv_module_handle {
	enum driver_cmd_type	driv_cmd_name;
	nic_driv_module		driv_func;
};

static int get_nic_drv_version(void *buf_out, const u32 *out_size)
{
	struct drv_version_info *ver_info = buf_out;

	if (!buf_out) {
		pr_err("Buf_out is NULL.\n");
		return -EINVAL;
	}

	if (*out_size != sizeof(*ver_info)) {
		pr_err("Unexpect out buf size from user :%u, expect: %lu\n",
		       *out_size, sizeof(*ver_info));
		return -EINVAL;
	}

	snprintf(ver_info->ver, sizeof(ver_info->ver), "%s  %s",
		 HINIC3_NIC_DRV_VERSION, "2025-05-08_00:00:08");

	return 0;
}

static int get_tx_info(struct hinic3_nic_dev *nic_dev, const void *buf_in,
		       u32 in_size, void *buf_out, u32 *out_size)
{
	u16 q_id;

	if (!HINIC3_CHANNEL_RES_VALID(nic_dev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Netdev is down, can't get tx info\n");
		return -EFAULT;
	}

	if (!buf_in || !buf_out) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Buf_in or buf_out is NULL.\n");
		return -EINVAL;
	}

	if (!out_size || in_size != sizeof(u32)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect in buf size from user :%u, expect: %lu\n",
			  in_size, sizeof(u32));
		return -EINVAL;
	}

	q_id = (u16)(*((u32 *)buf_in));

	return hinic3_dbg_get_sq_info(nic_dev->hwdev, q_id, buf_out, *out_size);
}

static int get_q_num(struct hinic3_nic_dev *nic_dev,
		     const void *buf_in, u32 in_size,
		     void *buf_out, u32 *out_size)
{
	if (!HINIC3_CHANNEL_RES_VALID(nic_dev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Netdev is down, can't get queue number\n");
		return -EFAULT;
	}

	if (!buf_out) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Get queue number para buf_out is NULL.\n");
		return -EINVAL;
	}

	if (!out_size || *out_size != sizeof(u16)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect out buf size from user: %u, expect: %lu\n",
			  *out_size, sizeof(u16));
		return -EINVAL;
	}

	*((u16 *)buf_out) = nic_dev->q_params.num_qps;

	return 0;
}

static int get_tx_wqe_info(struct hinic3_nic_dev *nic_dev,
			   const void *buf_in, u32 in_size,
			   void *buf_out, u32 *out_size)
{
	const struct wqe_info *info = buf_in;
	u16 wqebb_cnt = 1;

	if (!HINIC3_CHANNEL_RES_VALID(nic_dev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Netdev is down, can't get tx wqe info\n");
		return -EFAULT;
	}

	if (!buf_in || !buf_out) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Buf_in or buf_out is NULL.\n");
		return -EINVAL;
	}

	if (!out_size || in_size != sizeof(struct wqe_info)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect buf size from user, in_size: %u, expect: %lu\n",
			  in_size, sizeof(struct wqe_info));
		return -EINVAL;
	}

	return hinic3_dbg_get_wqe_info(nic_dev->hwdev, (u16)info->q_id,
				       (u16)info->wqe_id, wqebb_cnt,
				       buf_out, (u16 *)out_size, HINIC3_SQ);
}

static int get_rx_info(struct hinic3_nic_dev *nic_dev, const void *buf_in,
		       u32 in_size, void *buf_out, u32 *out_size)
{
	struct nic_rq_info *rq_info = buf_out;
	u16 q_id;
	int err;

	if (!HINIC3_CHANNEL_RES_VALID(nic_dev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Netdev is down, can't get rx info\n");
		return -EFAULT;
	}

	if (!buf_in || !buf_out) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Buf_in or buf_out is NULL.\n");
		return -EINVAL;
	}

	if (!out_size || in_size != sizeof(u32)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect buf size from user, in_size: %u, expect: %lu\n",
			  in_size, sizeof(u32));
		return -EINVAL;
	}

	q_id = (u16)(*((u32 *)buf_in));

	err = hinic3_dbg_get_rq_info(nic_dev->hwdev, q_id, buf_out, *out_size);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Get rq info failed, ret is %d.\n", err);
		return err;
	}

	rq_info->delta = (u16)nic_dev->rxqs[q_id].delta;
	rq_info->ci = (u16)(nic_dev->rxqs[q_id].cons_idx &
		nic_dev->rxqs[q_id].q_mask);
	rq_info->sw_pi = nic_dev->rxqs[q_id].next_to_update;
	rq_info->msix_vector = nic_dev->rxqs[q_id].irq_id;

	rq_info->coalesc_timer_cfg = nic_dev->rxqs[q_id].last_coalesc_timer_cfg;
	rq_info->pending_limt = nic_dev->rxqs[q_id].last_pending_limt;

	return 0;
}

static int get_rx_wqe_info(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			   u32 in_size, void *buf_out, u32 *out_size)
{
	const struct wqe_info *info = buf_in;
	u16 wqebb_cnt = 1;

	if (!HINIC3_CHANNEL_RES_VALID(nic_dev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Netdev is down, can't get rx wqe info\n");
		return -EFAULT;
	}

	if (!buf_in || !buf_out) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Buf_in or buf_out is NULL.\n");
		return -EINVAL;
	}

	if (!out_size || in_size != sizeof(struct wqe_info)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect buf size from user, in_size: %u, expect: %lu\n",
			  in_size, sizeof(struct wqe_info));
		return -EINVAL;
	}

	return hinic3_dbg_get_wqe_info(nic_dev->hwdev, (u16)info->q_id,
				       (u16)info->wqe_id, wqebb_cnt,
				       buf_out, (u16 *)out_size, HINIC3_RQ);
}

static int get_rx_cqe_info(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			   u32 in_size, void *buf_out, u32 *out_size)
{
	const struct wqe_info *info = buf_in;
	u16 q_id = 0;
	u16 idx = 0;

	if (!HINIC3_CHANNEL_RES_VALID(nic_dev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Netdev is down, can't get rx cqe info\n");
		return -EFAULT;
	}

	if (!buf_in || !buf_out || !out_size) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Buf_in or buf_out is NULL.\n");
		return -EINVAL;
	}

	if (in_size != sizeof(struct wqe_info)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect buf size from user, in_size: %u, expect: %lu\n",
			  in_size, sizeof(struct wqe_info));
		return -EINVAL;
	}

	if (*out_size != sizeof(struct hinic3_rq_cqe)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect out buf size from user :%u, expect: %lu\n",
			  *out_size, sizeof(struct hinic3_rq_cqe));
		return -EINVAL;
	}
	q_id = (u16)info->q_id;
	idx = (u16)info->wqe_id;

	if (q_id >= nic_dev->q_params.num_qps ||
	    idx >= nic_dev->rxqs[q_id].q_depth) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Invalid q_id[%u] >= %u, or wqe idx[%u] >= %u.\n",
			  q_id, nic_dev->q_params.num_qps, idx,
			  nic_dev->rxqs[q_id].q_depth);
		return -EFAULT;
	}

	memcpy(buf_out, nic_dev->rxqs[q_id].rx_info[idx].cqe,
	       sizeof(struct hinic3_rq_cqe));

	return 0;
}

static void clean_nicdev_stats(struct hinic3_nic_dev *nic_dev)
{
	u64_stats_update_begin(&nic_dev->stats.syncp);
	nic_dev->stats.netdev_tx_timeout = 0;
	nic_dev->stats.tx_carrier_off_drop = 0;
	nic_dev->stats.tx_invalid_qid = 0;
	nic_dev->stats.rsvd1 = 0;
	nic_dev->stats.rsvd2 = 0;
	u64_stats_update_end(&nic_dev->stats.syncp);
}

static int clear_func_static(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			     u32 in_size, void *buf_out, u32 *out_size)
{
	int i;

	*out_size = 0;
#ifndef HAVE_NETDEV_STATS_IN_NETDEV
	memset(&nic_dev->net_stats, 0, sizeof(nic_dev->net_stats));
#endif
	clean_nicdev_stats(nic_dev);
	for (i = 0; i < nic_dev->max_qps; i++) {
		hinic3_rxq_clean_stats(&nic_dev->rxqs[i].rxq_stats);
		hinic3_txq_clean_stats(&nic_dev->txqs[i].txq_stats);
	}

	return 0;
}

static int get_loopback_mode(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			     u32 in_size, void *buf_out, u32 *out_size)
{
	struct hinic3_nic_loop_mode *mode = buf_out;

	if (!out_size || !mode)
		return -EINVAL;

	if (*out_size != sizeof(*mode)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect out buf size from user: %u, expect: %lu\n",
			  *out_size, sizeof(*mode));
		return -EINVAL;
	}

	return hinic3_get_loopback_mode(nic_dev->hwdev, (u8 *)&mode->loop_mode,
					(u8 *)&mode->loop_ctrl);
}

static int set_loopback_mode(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			     u32 in_size, void *buf_out, u32 *out_size)
{
	const struct hinic3_nic_loop_mode *mode = buf_in;
	int err;

	if (!test_bit(HINIC3_INTF_UP, &nic_dev->flags)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Netdev is down, can't set loopback mode\n");
		return -EFAULT;
	}

	if (!mode || !out_size || in_size != sizeof(*mode))
		return -EINVAL;

	if (*out_size != sizeof(*mode)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect out buf size from user: %u, expect: %lu\n",
			  *out_size, sizeof(*mode));
		return -EINVAL;
	}

	err = hinic3_set_loopback_mode(nic_dev->hwdev, (u8)mode->loop_mode,
				       (u8)mode->loop_ctrl);
	if (err == 0)
		nicif_info(nic_dev, drv, nic_dev->netdev,
			   "Set loopback mode %u en %u succeed\n",
			   mode->loop_mode, mode->loop_ctrl);

	return err;
}

enum hinic3_nic_link_mode {
	HINIC3_LINK_MODE_AUTO = 0,
	HINIC3_LINK_MODE_UP,
	HINIC3_LINK_MODE_DOWN,
	HINIC3_LINK_MODE_MAX,
};

static int set_link_mode_param_valid(struct hinic3_nic_dev *nic_dev,
				     const void *buf_in, u32 in_size,
				     const u32 *out_size)
{
	if (!test_bit(HINIC3_INTF_UP, &nic_dev->flags)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Netdev is down, can't set link mode\n");
		return -EFAULT;
	}

	if (!buf_in || !out_size ||
	    in_size != sizeof(enum hinic3_nic_link_mode))
		return -EINVAL;

	if (*out_size != sizeof(enum hinic3_nic_link_mode)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect out buf size from user: %u, expect: %lu\n",
			  *out_size, sizeof(enum hinic3_nic_link_mode));
		return -EINVAL;
	}

	return 0;
}

static int set_link_mode(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			 u32 in_size, void *buf_out, u32 *out_size)
{
	const enum hinic3_nic_link_mode *link = buf_in;
	u8 link_status;

	if (set_link_mode_param_valid(nic_dev, buf_in, in_size, out_size))
		return -EFAULT;

	switch (*link) {
	case HINIC3_LINK_MODE_AUTO:
		if (hinic3_get_link_state(nic_dev->hwdev, &link_status))
			link_status = false;
		hinic3_link_status_change(nic_dev, (bool)link_status);
		nicif_info(nic_dev, drv, nic_dev->netdev,
			   "Set link mode: auto succeed, now is link %s\n",
			   (link_status ? "up" : "down"));
		break;
	case HINIC3_LINK_MODE_UP:
		hinic3_link_status_change(nic_dev, true);
		nicif_info(nic_dev, drv, nic_dev->netdev,
			   "Set link mode: up succeed\n");
		break;
	case HINIC3_LINK_MODE_DOWN:
		hinic3_link_status_change(nic_dev, false);
		nicif_info(nic_dev, drv, nic_dev->netdev,
			   "Set link mode: down succeed\n");
		break;
	default:
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Invalid link mode %d to set\n", *link);
		return  -EINVAL;
	}

	return 0;
}

static int set_pf_bw_limit(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			   u32 in_size, void *buf_out, u32 *out_size)
{
	u32 pf_bw_limit;
	int err;
	struct hinic3_nic_io *nic_io = NULL;
	struct net_device *net_dev = nic_dev->netdev;

	if (hinic3_support_roce(nic_dev->hwdev, NULL) &&
	    hinic3_is_bond_dev_status_actived(net_dev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "The rate limit func is not supported when RoCE bonding is enabled\n");
		return -EINVAL;
	}

	if (HINIC3_FUNC_IS_VF(nic_dev->hwdev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "To set VF bandwidth rate, please use ip link cmd\n");
		return -EINVAL;
	}

	if (!buf_in || !buf_out || in_size != sizeof(u32) ||
	    !out_size || *out_size != sizeof(u8))
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(nic_dev->hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	nic_io->direct = HINIC3_NIC_TX;
	pf_bw_limit = *((u32 *)buf_in);

	err = hinic3_set_pf_bw_limit(nic_dev->hwdev, pf_bw_limit);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Failed to set pf bandwidth limit to %u%%\n",
			  pf_bw_limit);
		if (err < 0)
			return err;
	}

	*((u8 *)buf_out) = (u8)err;

	return 0;
}

static int set_rx_pf_bw_limit(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			      u32 in_size, void *buf_out, u32 *out_size)
{
	u32 pf_bw_limit;
	int err;
	struct hinic3_nic_io *nic_io = NULL;
	struct net_device *net_dev = nic_dev->netdev;

	if (hinic3_support_roce(nic_dev->hwdev, NULL) &&
	    hinic3_is_bond_dev_status_actived(net_dev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "The rate limit func is not supported when RoCE bonding is enabled\n");
		return -EINVAL;
	}

	if (HINIC3_FUNC_IS_VF(nic_dev->hwdev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "To set VF bandwidth rate, please use ip link cmd\n");
		return -EINVAL;
	}

	if (!buf_in || !buf_out || in_size != sizeof(u32) || !out_size || *out_size != sizeof(u8))
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(nic_dev->hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	nic_io->direct = HINIC3_NIC_RX;
	pf_bw_limit = *((u32 *)buf_in);

	err = hinic3_set_pf_bw_limit(nic_dev->hwdev, pf_bw_limit);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to set pf bandwidth limit to %d%%\n",
			  pf_bw_limit);
		if (err < 0)
			return err;
	}

	*((u8 *)buf_out) = (u8)err;

	return 0;
}

static int get_pf_bw_limit(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			   u32 in_size, void *buf_out, u32 *out_size)
{
	struct hinic3_nic_io *nic_io = NULL;
	u32 *rate_limit = (u32 *)buf_out;

	if (HINIC3_FUNC_IS_VF(nic_dev->hwdev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "To get VF bandwidth rate, please use ip link cmd\n");
		return -EINVAL;
	}

	if (!buf_out || !out_size)
		return -EINVAL;

	if (*out_size != sizeof(u32) * 2) { // 2:Stored in an array, TX and RX, both length are u32
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect out buf size from user: %d, expect: %lu\n",
			  *out_size, sizeof(u32) * 2);
		return -EFAULT;
	}

	nic_io = hinic3_get_service_adapter(nic_dev->hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	rate_limit[HINIC3_NIC_RX] = nic_io->nic_cfg.pf_bw_rx_limit;
	rate_limit[HINIC3_NIC_TX] = nic_io->nic_cfg.pf_bw_tx_limit;

	nicif_info(nic_dev, drv, nic_dev->netdev,
		   "read rate cfg success rx rate is: %u, tx rate is : %u\n",
		   rate_limit[HINIC3_NIC_RX], rate_limit[HINIC3_NIC_TX]);
	return 0;
}

static int get_sset_count(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			  u32 in_size, void *buf_out, u32 *out_size)
{
	u32 count;

	if (!buf_in || in_size != sizeof(u32) || !out_size ||
	    *out_size != sizeof(u32) || !buf_out) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Invalid parameters, in_size: %u\n", in_size);
		return -EINVAL;
	}

	switch (*((u32 *)buf_in)) {
	case HINIC3_SHOW_SSET_IO_STATS:
		count = hinic3_get_io_stats_size(nic_dev);
		break;
	default:
		count = 0;
		break;
	}

	*((u32 *)buf_out) = count;

	return 0;
}

static int get_sset_stats(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			  u32 in_size, void *buf_out, u32 *out_size)
{
	struct hinic3_show_item *items = buf_out;
	u32 sset, count, size;
	int err;

	if (!buf_in || in_size != sizeof(u32) || !out_size || !buf_out) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Invalid parameters, in_size: %u\n", in_size);
		return -EINVAL;
	}

	size = sizeof(u32);
	err = get_sset_count(nic_dev, buf_in, in_size, &count, &size);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Get sset count failed, ret=%d\n", err);
		return -EINVAL;
	}
	if (count * sizeof(*items) != *out_size) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect out buf size from user :%u, expect: %lu\n",
			  *out_size, count * sizeof(*items));
		return -EINVAL;
	}

	sset = *((u32 *)buf_in);

	switch (sset) {
	case HINIC3_SHOW_SSET_IO_STATS:
		err = hinic3_get_io_stats(nic_dev, items);
		if (err < 0)
			return -EINVAL;
		break;

	default:
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unknown %u to get stats\n", sset);
		err = -EINVAL;
		break;
	}

	return err;
}

static int update_pcp_dscp_cfg(struct hinic3_nic_dev *nic_dev,
			       struct hinic3_dcb_config *wanted_dcb_cfg,
			       const struct hinic3_mt_qos_dev_cfg *qos_in)
{
	struct hinic3_dcb *dcb = nic_dev->dcb;
	int i;
	u8 cos_num = 0, valid_cos_bitmap = 0;

	if (qos_in->cfg_bitmap & CMD_QOS_DEV_PCP2COS) {
		for (i = 0; i < NIC_DCB_UP_MAX; i++) {
			if (!(dcb->func_dft_cos_bitmap &
			      BIT(qos_in->pcp2cos[i]))) {
				nicif_err(nic_dev, drv, nic_dev->netdev,
					  "Invalid cos=%u, func cos valid map is %u",
					  qos_in->pcp2cos[i],
					  dcb->func_dft_cos_bitmap);
				return -EINVAL;
			}

			if ((BIT(qos_in->pcp2cos[i]) & valid_cos_bitmap) == 0) {
				valid_cos_bitmap |= (u8)BIT(qos_in->pcp2cos[i]);
				cos_num++;
			}
		}

		memcpy(wanted_dcb_cfg->pcp2cos, qos_in->pcp2cos,
		       sizeof(qos_in->pcp2cos));
		wanted_dcb_cfg->pcp_user_cos_num = cos_num;
		wanted_dcb_cfg->pcp_valid_cos_map = valid_cos_bitmap;
	}

	if (qos_in->cfg_bitmap & CMD_QOS_DEV_DSCP2COS) {
		cos_num = 0;
		valid_cos_bitmap = 0;
		for (i = 0; i < NIC_DCB_IP_PRI_MAX; i++) {
			u8 cos = qos_in->dscp2cos[i] == DBG_DFLT_DSCP_VAL ?
				 dcb->wanted_dcb_cfg.dscp2cos[i] :
				 qos_in->dscp2cos[i];

			if (cos >= NIC_DCB_UP_MAX ||
			    !(dcb->func_dft_cos_bitmap & BIT(cos))) {
				nicif_err(nic_dev, drv, nic_dev->netdev,
					  "Invalid cos=%u, func cos valid map is %u",
					  cos, dcb->func_dft_cos_bitmap);
				return -EINVAL;
			}

			if ((BIT(cos) & valid_cos_bitmap) == 0) {
				valid_cos_bitmap |= (u8)BIT(cos);
				cos_num++;
			}
		}

		for (i = 0; i < NIC_DCB_IP_PRI_MAX; i++)
			wanted_dcb_cfg->dscp2cos[i] =
				qos_in->dscp2cos[i] == DBG_DFLT_DSCP_VAL ?
				dcb->hw_dcb_cfg.dscp2cos[i] :
				qos_in->dscp2cos[i];
		wanted_dcb_cfg->dscp_user_cos_num = cos_num;
		wanted_dcb_cfg->dscp_valid_cos_map = valid_cos_bitmap;
	}

	return 0;
}

static int update_wanted_qos_cfg(struct hinic3_nic_dev *nic_dev,
				 struct hinic3_dcb_config *wanted_dcb_cfg,
				 const struct hinic3_mt_qos_dev_cfg *qos_in)
{
	struct hinic3_dcb *dcb = nic_dev->dcb;
	int ret;
	u8 cos_num, valid_cos_bitmap;

	if (qos_in->cfg_bitmap & CMD_QOS_DEV_TRUST) {
		if (qos_in->trust > HINIC3_DCB_DSCP) {
			nicif_err(nic_dev, drv, nic_dev->netdev,
				  "Invalid trust=%u\n", qos_in->trust);
			return -EINVAL;
		}

		wanted_dcb_cfg->trust = qos_in->trust;
	}

	if (qos_in->cfg_bitmap & CMD_QOS_DEV_DFT_COS) {
		if (!(BIT(qos_in->dft_cos) & dcb->func_dft_cos_bitmap)) {
			nicif_err(nic_dev, drv, nic_dev->netdev,
				  "Invalid dft_cos=%u\n", qos_in->dft_cos);
			return -EINVAL;
		}

		wanted_dcb_cfg->default_cos = qos_in->dft_cos;
	}

	ret = update_pcp_dscp_cfg(nic_dev, wanted_dcb_cfg, qos_in);
	if (ret)
		return ret;

	if (wanted_dcb_cfg->trust == HINIC3_DCB_PCP) {
		cos_num = wanted_dcb_cfg->pcp_user_cos_num;
		valid_cos_bitmap = wanted_dcb_cfg->pcp_valid_cos_map;
	} else {
		cos_num = wanted_dcb_cfg->dscp_user_cos_num;
		valid_cos_bitmap = wanted_dcb_cfg->dscp_valid_cos_map;
	}

	if (!(BIT(wanted_dcb_cfg->default_cos) & valid_cos_bitmap)) {
		nicif_info(nic_dev, drv, nic_dev->netdev,
			   "Current default_cos=%u, change to %u\n",
			   wanted_dcb_cfg->default_cos,
			   (u8)fls(valid_cos_bitmap) - 1);
		wanted_dcb_cfg->default_cos = (u8)fls(valid_cos_bitmap) - 1;
	}

	return 0;
}

static int dcb_mt_qos_map(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			  u32 in_size, void *buf_out, u32 *out_size)
{
	struct hinic3_dcb *dcb = nic_dev->dcb;
	const struct hinic3_mt_qos_dev_cfg *qos_in = buf_in;
	struct hinic3_mt_qos_dev_cfg *qos_out = buf_out;
	u8 i;
	int err;

	if (!buf_out || !out_size || !buf_in)
		return -EINVAL;

	if (*out_size != sizeof(*qos_out) || in_size != sizeof(*qos_in)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect buf size from user, in_size: %u, out_size: %u, expect: %lu\n",
			  in_size, *out_size, sizeof(*qos_in));
		return -EINVAL;
	}

	memcpy(qos_out, qos_in, sizeof(*qos_in));
	qos_out->head.status = 0;
	if (qos_in->op_code & MT_DCB_OPCODE_WR) {
		memcpy(&dcb->wanted_dcb_cfg, &dcb->hw_dcb_cfg,
		       sizeof(struct hinic3_dcb_config));
		err = update_wanted_qos_cfg(nic_dev, &dcb->wanted_dcb_cfg,
					    qos_in);
		if (err) {
			qos_out->head.status = MT_EINVAL;
			return 0;
		}

		err = hinic3_dcbcfg_set_up_bitmap(nic_dev);
		if (err)
			qos_out->head.status = MT_EIO;
	} else {
		qos_out->dft_cos = dcb->hw_dcb_cfg.default_cos;
		qos_out->trust = dcb->hw_dcb_cfg.trust;
		for (i = 0; i < NIC_DCB_UP_MAX; i++)
			qos_out->pcp2cos[i] = dcb->hw_dcb_cfg.pcp2cos[i];
		for (i = 0; i < NIC_DCB_IP_PRI_MAX; i++)
			qos_out->dscp2cos[i] = dcb->hw_dcb_cfg.dscp2cos[i];
	}

	return 0;
}

static int dcb_mt_dcb_state(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			    u32 in_size, void *buf_out, u32 *out_size)
{
	const struct hinic3_mt_dcb_state *dcb_in = buf_in;
	struct hinic3_mt_dcb_state *dcb_out = buf_out;
	int err;
	u8 user_cos_num;
	u8 netif_run = 0;

	if (!buf_in || !buf_out || !out_size)
		return -EINVAL;

	if (*out_size != sizeof(*dcb_out) || in_size != sizeof(*dcb_in)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect buf size from user, in_size: %u, out_size: %u, expect: %lu\n",
			  in_size, *out_size, sizeof(*dcb_in));
		return -EINVAL;
	}

	user_cos_num = hinic3_get_dev_user_cos_num(nic_dev);
	memcpy(dcb_out, dcb_in, sizeof(*dcb_in));
	dcb_out->head.status = 0;
	if (dcb_in->op_code & MT_DCB_OPCODE_WR) {
		if (test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags) ==
			     dcb_in->state)
			return 0;

		if (netif_running(nic_dev->netdev)) {
			netif_run = 1;
			hinic3_vport_down(nic_dev);
		}

		err = hinic3_setup_cos(nic_dev->netdev,
				       dcb_in->state ? user_cos_num : 0,
				       netif_run);
		if (err)
			goto setup_cos_fail;

		if (netif_run) {
			err = hinic3_vport_up(nic_dev);
			if (err)
				goto vport_up_fail;
		}
	} else {
		dcb_out->state = !!test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags);
	}

	return 0;

vport_up_fail:
	hinic3_setup_cos(nic_dev->netdev, dcb_in->state ? 0 : user_cos_num,
			 netif_run);

setup_cos_fail:
	if (netif_run)
		hinic3_vport_up(nic_dev);

	return err;
}

static int dcb_mt_hw_qos_get(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			     u32 in_size, void *buf_out, u32 *out_size)
{
	struct hinic3_dcb *dcb = nic_dev->dcb;
	const struct hinic3_mt_qos_cos_cfg *cos_cfg_in = buf_in;
	struct hinic3_mt_qos_cos_cfg *cos_cfg_out = buf_out;

	if (!buf_in || !buf_out || !out_size)
		return -EINVAL;

	if (*out_size != sizeof(*cos_cfg_out) ||
	    in_size != sizeof(*cos_cfg_in)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect buf size from user, in_size: %u, out_size: %u, expect: %lu\n",
			  in_size, *out_size, sizeof(*cos_cfg_in));
		return -EINVAL;
	}

	memcpy(cos_cfg_out, cos_cfg_in, sizeof(*cos_cfg_in));
	cos_cfg_out->head.status = 0;

	cos_cfg_out->port_id = hinic3_physical_port_id(nic_dev->hwdev);
	cos_cfg_out->func_cos_bitmap = (u8)dcb->func_dft_cos_bitmap;
	cos_cfg_out->port_cos_bitmap = (u8)dcb->port_dft_cos_bitmap;
	cos_cfg_out->func_max_cos_num = dcb->cos_config_num_max;

	return 0;
}

static int get_inter_num(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			 u32 in_size, void *buf_out, u32 *out_size)
{
	u16 intr_num;

	intr_num = hinic3_intr_num(nic_dev->hwdev);

	if (!buf_out || !out_size) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Buf_out or out_size is NULL.\n");
		return -EINVAL;
	}

	if (*out_size != sizeof(u16)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect out buf size from user :%u, expect: %lu\n",
			  *out_size, sizeof(u16));
		return -EFAULT;
	}
	*(u16 *)buf_out = intr_num;

	return 0;
}

static int get_netdev_name(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			   u32 in_size, void *buf_out, u32 *out_size)
{
	if (!buf_out || !out_size) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Buf_out or out_size is NULL.\n");
		return -EINVAL;
	}

	if (*out_size != IFNAMSIZ) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect out buf size from user :%u, expect: %u\n",
			  *out_size, IFNAMSIZ);
		return -EFAULT;
	}

	strscpy(buf_out, nic_dev->netdev->name, IFNAMSIZ);

	return 0;
}

static int get_netdev_tx_timeout(struct hinic3_nic_dev *nic_dev,
				 const void *buf_in, u32 in_size,
				 void *buf_out, u32 *out_size)
{
	struct net_device *net_dev = nic_dev->netdev;
	int *tx_timeout = buf_out;

	if (!buf_out || !out_size)
		return -EINVAL;

	if (*out_size != sizeof(int)) {
		nicif_err(nic_dev, drv, net_dev,
			  "Unexpect buf size from user, out_size: %u, expect: %lu\n",
			  *out_size, sizeof(int));
		return -EINVAL;
	}

	*tx_timeout = net_dev->watchdog_timeo;

	return 0;
}

static int set_netdev_tx_timeout(struct hinic3_nic_dev *nic_dev,
				 const void *buf_in, u32 in_size,
				 void *buf_out, u32 *out_size)
{
	struct net_device *net_dev = nic_dev->netdev;
	const int *tx_timeout = buf_in;

	if (!buf_in)
		return -EINVAL;

	if (in_size != sizeof(int)) {
		nicif_err(nic_dev, drv, net_dev,
			  "Unexpect buf size from user, in_size: %u, expect: %lu\n",
			  in_size, sizeof(int));
		return -EINVAL;
	}

	net_dev->watchdog_timeo = *tx_timeout * HZ;
	nicif_info(nic_dev, drv, net_dev,
		   "Set tx timeout check period to %ds\n", *tx_timeout);

	return 0;
}

static int get_xsfp_present(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			    u32 in_size, void *buf_out, u32 *out_size)
{
	struct mag_cmd_get_xsfp_present *sfp_abs = buf_out;

	if (!buf_in || !buf_out || !out_size)
		return -EINVAL;

	if (*out_size != sizeof(*sfp_abs) || in_size != sizeof(*sfp_abs)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect buf size from user, in_size: %u, out_size: %u, expect: %lu\n",
			  in_size, *out_size, sizeof(*sfp_abs));
		return -EINVAL;
	}

	sfp_abs->head.status = 0;
	sfp_abs->abs_status = hinic3_if_sfp_absent(nic_dev->hwdev);

	return 0;
}

static int get_xsfp_tlv_info(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			     u32 in_size, void *buf_out, u32 *out_size)
{
	struct drv_tag_mag_cmd_get_xsfp_tlv_rsp *sfp_tlv_info = buf_out;
	const struct tag_mag_cmd_get_xsfp_tlv_req *sfp_tlv_info_req = buf_in;
	int err;

	if (!buf_in || !buf_out || !out_size)
		return -EINVAL;

	if (*out_size != sizeof(*sfp_tlv_info) ||
	    in_size != sizeof(*sfp_tlv_info_req)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect buf size from user, in_size: %u, out_size: %u, expect: %lu\n",
			  in_size, *out_size, sizeof(*sfp_tlv_info));
		return -EINVAL;
	}

	err = hinic3_get_sfp_tlv_info(nic_dev->hwdev,
				      sfp_tlv_info, sfp_tlv_info_req);
	if (err != 0) {
		sfp_tlv_info->head.status = MT_EIO;
		return 0;
	}

	return 0;
}

static int get_xsfp_info(struct hinic3_nic_dev *nic_dev, const void *buf_in,
			 u32 in_size, void *buf_out, u32 *out_size)
{
	struct mag_cmd_get_xsfp_info *sfp_info = buf_out;
	int err;

	if (!buf_in || !buf_out || !out_size)
		return -EINVAL;

	if (*out_size != sizeof(*sfp_info) || in_size != sizeof(*sfp_info)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unexpect buf size from user, in_size: %u, out_size: %u, expect: %lu\n",
			  in_size, *out_size, sizeof(*sfp_info));
		return -EINVAL;
	}

	err = hinic3_get_sfp_info(nic_dev->hwdev, sfp_info);
	if (err) {
		sfp_info->head.status = MT_EIO;
		return 0;
	}

	return 0;
}

static const struct nic_drv_module_handle nic_driv_module_cmd_handle[] = {
	{.driv_cmd_name = TX_INFO,		.driv_func = get_tx_info},
	{.driv_cmd_name = Q_NUM,		.driv_func = get_q_num},
	{.driv_cmd_name = TX_WQE_INFO,		.driv_func = get_tx_wqe_info},
	{.driv_cmd_name = RX_INFO,		.driv_func = get_rx_info},
	{.driv_cmd_name = RX_WQE_INFO,		.driv_func = get_rx_wqe_info},
	{.driv_cmd_name = RX_CQE_INFO,		.driv_func = get_rx_cqe_info},
	{.driv_cmd_name = GET_INTER_NUM,	.driv_func = get_inter_num},
	{.driv_cmd_name = CLEAR_FUNC_STASTIC,	.driv_func = clear_func_static},
	{.driv_cmd_name = GET_LOOPBACK_MODE,	.driv_func = get_loopback_mode},
	{.driv_cmd_name = SET_LOOPBACK_MODE,	.driv_func = set_loopback_mode},
	{.driv_cmd_name = SET_LINK_MODE,	.driv_func = set_link_mode},
	{.driv_cmd_name = SET_TX_PF_BW_LIMIT,	.driv_func = set_pf_bw_limit},
	{.driv_cmd_name = GET_PF_BW_LIMIT,	.driv_func = get_pf_bw_limit},
	{.driv_cmd_name = GET_SSET_COUNT,	.driv_func = get_sset_count},
	{.driv_cmd_name = GET_SSET_ITEMS,	.driv_func = get_sset_stats},
	{.driv_cmd_name = DCB_STATE,		.driv_func = dcb_mt_dcb_state},
	{.driv_cmd_name = QOS_DEV,		.driv_func = dcb_mt_qos_map},
	{.driv_cmd_name = GET_QOS_COS,		.driv_func = dcb_mt_hw_qos_get},
	{.driv_cmd_name = GET_ULD_DEV_NAME,	.driv_func = get_netdev_name},
	{.driv_cmd_name = GET_TX_TIMEOUT,	.driv_func = get_netdev_tx_timeout},
	{.driv_cmd_name = SET_TX_TIMEOUT,	.driv_func = set_netdev_tx_timeout},
	{.driv_cmd_name = GET_XSFP_PRESENT,	.driv_func = get_xsfp_present},
	{.driv_cmd_name = GET_XSFP_INFO,	.driv_func = get_xsfp_info},
	{.driv_cmd_name = GET_XSFP_INFO_COMP_CMIS, .driv_func = get_xsfp_tlv_info},
	{.driv_cmd_name = SET_RX_PF_BW_LIMIT,	.driv_func = set_rx_pf_bw_limit}
};

static int send_to_nic_driver(struct hinic3_nic_dev *nic_dev,
			      u32 cmd, const void *buf_in,
			      u32 in_size, void *buf_out, u32 *out_size)
{
	int index, num_cmds = (int)(sizeof(nic_driv_module_cmd_handle) /
				sizeof(nic_driv_module_cmd_handle[0]));
	enum driver_cmd_type cmd_type = (enum driver_cmd_type)cmd;
	int err = 0;

	if (cmd_type == DCB_STATE || cmd_type == QOS_DEV)
		rtnl_lock();

	mutex_lock(&nic_dev->nic_mutex);
	for (index = 0; index < num_cmds; index++) {
		if (cmd_type ==
			nic_driv_module_cmd_handle[index].driv_cmd_name) {
			err = nic_driv_module_cmd_handle[index].driv_func
					(nic_dev, buf_in,
					 in_size, buf_out, out_size);
			break;
		}
	}
	mutex_unlock(&nic_dev->nic_mutex);

	if (cmd_type == DCB_STATE || cmd_type == QOS_DEV)
		rtnl_unlock();

	if (index == num_cmds) {
		pr_err("Can't find callback for %d\n", cmd_type);
		return -EINVAL;
	}

	return err;
}

int hinic3_ioctl(void *uld_dev, u32 cmd, const void *buf_in,
	      u32 in_size, void *buf_out, u32 *out_size)
{
	if (cmd == GET_DRV_VERSION)
		return get_nic_drv_version(buf_out, out_size);
	else if (!uld_dev)
		return -EINVAL;

	return send_to_nic_driver(uld_dev, cmd, buf_in,
				  in_size, buf_out, out_size);
}

