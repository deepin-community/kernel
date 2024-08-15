// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [TOOL]" fmt

#include "sss_nic_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_dev_define.h"
#include "sss_tool_comm.h"
#include "sss_tool_nic.h"
#include "sss_nic_netdev_ops_api.h"

enum sss_tool_link_mode {
	SSS_TOOL_LINK_MODE_AUTO = 0,
	SSS_TOOL_LINK_MODE_UP,
	SSS_TOOL_LINK_MODE_DOWN,
	SSS_TOOL_LINK_MODE_MAX,
};

typedef void (*sss_tool_set_link_mode_handler_t)(struct sss_nic_dev *nic_dev);

int sss_tool_get_loopback_mode(struct sss_nic_dev *nic_dev, const void *in_buf,
			       u32 in_len, void *out_buf, u32 *out_len)
{
	struct sss_tool_loop_mode *mode = out_buf;

	if (!out_len || !mode) {
		tool_err("Invalid param, use null pointer\n");
		return -EINVAL;
	}

	if (*out_len != sizeof(*mode)) {
		tool_err("Invalid out len: %u is not equal to %lu\n",
			 *out_len, sizeof(*mode));
		return -EINVAL;
	}

	return sss_nic_get_loopback_mode(nic_dev, (u8 *)&mode->loop_mode,
					(u8 *)&mode->loop_ctrl);
}

int sss_tool_set_loopback_mode(struct sss_nic_dev *nic_dev, const void *in_buf,
			       u32 in_len, void *out_buf, u32 *out_len)
{
	int ret;
	const struct sss_tool_loop_mode *mode = in_buf;

	if (!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_INTF_UP)) {
		tool_err("Fail to set lookback mode, netdev is down\n");
		return -EFAULT;
	}

	if (!mode || !out_len) {
		tool_err("Invalid param, use null pointer\n");
		return -EINVAL;
	}

	if (in_len != sizeof(*mode) || *out_len != sizeof(*mode)) {
		tool_err("Invalid in len %d or out len %u is not equal to %lu\n",
			 in_len, *out_len, sizeof(*mode));
		return -EINVAL;
	}

	ret = sss_nic_set_loopback_mode(nic_dev->hwdev, (u8)mode->loop_mode, (u8)mode->loop_ctrl);
	if (ret == 0)
		tool_info("succeed to set loopback mode %u en %u\n",
			  mode->loop_mode, mode->loop_ctrl);

	return ret;
}

static bool sss_tool_check_param_valid(struct sss_nic_dev *nic_dev,
				       const void *in_buf, u32 in_len,
				       const u32 *out_len)
{
	if (!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_INTF_UP)) {
		tool_err("Fail to set link mode, netdev is down\n");
		return false;
	}

	if (!in_buf || !out_len) {
		tool_err("Invalid param, use null pointer\n");
		return -EINVAL;
	}

	if (in_len != sizeof(SSS_TOOL_LINK_MODE_MAX) ||
	    *out_len != sizeof(SSS_TOOL_LINK_MODE_MAX)) {
		tool_err("Invalid in len %d or out len: %u is not equal to %lu\n",
			 in_len, *out_len, sizeof(SSS_TOOL_LINK_MODE_MAX));
		return false;
	}

	return true;
}

static void sss_tool_set_link_status(struct sss_nic_dev *nic_dev, bool status)
{
	struct net_device *netdev = nic_dev->netdev;

	if (!SSS_CHANNEL_RES_VALID(nic_dev) ||
	    SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_LP_TEST) ||
	    SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_FORCE_LINK_UP))
		return;

	if (!status) {
		if (!netif_carrier_ok(netdev))
			return;

		tool_info("Link down\n");
		nic_dev->link_status = status;
		netif_carrier_off(netdev);

	} else {
		if (netif_carrier_ok(netdev))
			return;

		tool_info("Link up\n");
		nic_dev->link_status = status;
		netif_carrier_on(netdev);
	}
}

static void sss_tool_link_mode_auto(struct sss_nic_dev *nic_dev)
{
	u8 link_status;

	if (sss_nic_get_hw_link_state(nic_dev, &link_status))
		link_status = false;

	sss_tool_set_link_status(nic_dev, (bool)link_status);
	tool_info("Success to set link mode to auto, the state is link %s\n",
		  (link_status ? "up" : "down"));
}

static void sss_tool_link_mode_up(struct sss_nic_dev *nic_dev)
{
	sss_tool_set_link_status(nic_dev, true);
	tool_info("Success to set link mode to up\n");
}

static void sss_tool_link_mode_down(struct sss_nic_dev *nic_dev)
{
	sss_tool_set_link_status(nic_dev, false);
	tool_info("Success to set link mode to down\n");
}

int sss_tool_set_link_mode(struct sss_nic_dev *nic_dev, const void *in_buf,
			   u32 in_len, void *out_buf, u32 *out_len)
{
	const enum sss_tool_link_mode *mode = in_buf;

	sss_tool_set_link_mode_handler_t handler[] = {
		sss_tool_link_mode_auto,
		sss_tool_link_mode_up,
		sss_tool_link_mode_down,
	};

	if (!sss_tool_check_param_valid(nic_dev, in_buf, in_len, out_len))
		return -EFAULT;

	if (*mode >= SSS_TOOL_LINK_MODE_MAX) {
		tool_err("Fail to set link mode, mode %d\n", *mode);
		return  -EINVAL;
	}

	handler[*mode](nic_dev);

	return 0;
}

static int sss_tool_update_pf_bw_limit(struct sss_nic_dev *nic_dev, u32 bw_limit)
{
	int ret;
	u32 old_bw_limit;
	struct sss_nic_port_info port_info = {0};
	struct sss_nic_io *nic_io = nic_dev->nic_io;

	if (!nic_io)
		return -EINVAL;

	if (bw_limit > SSSNIC_PF_LIMIT_BW_MAX) {
		tool_err("Fail to update pf bw limit, bandwidth: %u large then max limit: %u\n",
			 bw_limit, SSSNIC_PF_LIMIT_BW_MAX);
		return -EINVAL;
	}

	old_bw_limit = nic_io->mag_cfg.pf_bw_limit;
	nic_io->mag_cfg.pf_bw_limit = bw_limit;

	if (!SSSNIC_SUPPORT_RATE_LIMIT(nic_io))
		return 0;

	ret = sss_nic_get_hw_port_info(nic_dev, &port_info, SSS_CHANNEL_NIC);
	if (ret != 0) {
		tool_err("Fail to get port info\n");
		nic_io->mag_cfg.pf_bw_limit = bw_limit;
		return -EIO;
	}

	ret = sss_nic_set_pf_rate(nic_dev, port_info.speed);
	if (ret != 0) {
		tool_err("Fail to set pf bandwidth\n");
		nic_io->mag_cfg.pf_bw_limit = bw_limit;
		return ret;
	}

	return 0;
}

static int sss_tool_check_preconditions(struct sss_nic_dev *nic_dev,
					const void *in_buf, u32 in_len,
					void *out_buf, u32 *out_len)
{
	int ret;
	u8 link_state = 0;

	if (SSSNIC_FUNC_IS_VF(nic_dev->hwdev)) {
		tool_err("Fail to set VF bandwidth rate, please use ip link cmd\n");
		return -EINVAL;
	}

	if (!in_buf || !out_buf || !out_len) {
		tool_err("Invalid param, use null pointer\n");
		return -EINVAL;
	}

	if (in_len != sizeof(in_len)) {
		tool_err("Invalid in len %d is not equal to %lu\n",
			 in_len, sizeof(in_len));
		return -EINVAL;
	}

	if (*out_len != sizeof(link_state)) {
		tool_err("Invalid out len %d is not equal to %lu\n",
			 in_len, sizeof(link_state));
			return -EINVAL;
	}

	ret = sss_nic_get_hw_link_state(nic_dev, &link_state);
	if (ret != 0) {
		tool_err("Fail to get link state\n");
		return -EIO;
	}

	if (!link_state) {
		tool_err("Fail to set pf rate, must be link up\n");
		return -EINVAL;
	}

	return 0;
}

int sss_tool_set_pf_bw_limit(struct sss_nic_dev *nic_dev, const void *in_buf,
			     u32 in_len, void *out_buf, u32 *out_len)
{
	int ret;
	u32 pf_bw_limit;

	ret = sss_tool_check_preconditions(nic_dev, in_buf, in_len, out_buf, out_len);
	if (ret != 0)
		return -EINVAL;

	pf_bw_limit = *((u32 *)in_buf);

	ret = sss_tool_update_pf_bw_limit(nic_dev, pf_bw_limit);
	if (ret != 0) {
		tool_err("Fail to set pf bandwidth limit to %d%%\n", pf_bw_limit);
		if (ret < 0)
			return ret;
	}

	*((u8 *)out_buf) = (u8)ret;

	return 0;
}

int sss_tool_get_pf_bw_limit(struct sss_nic_dev *nic_dev, const void *in_buf,
			     u32 in_len, void *out_buf, u32 *out_len)
{
	struct sss_nic_io *nic_io = NULL;

	if (SSSNIC_FUNC_IS_VF(nic_dev->hwdev)) {
		tool_err("Fail to get VF bandwidth rate, please use ip link cmd\n");
		return -EINVAL;
	}

	if (!out_buf || !out_len) {
		tool_err("Invalid param, use null pointer\n");
		return -EINVAL;
	}

	if (*out_len != sizeof(in_len)) {
		tool_err("Invalid out len %d is not equal to %lu\n",
			 *out_len, sizeof(in_len));
		return -EFAULT;
	}

	nic_io = nic_dev->nic_io;
	if (!nic_io)
		return -EINVAL;

	*((u32 *)out_buf) = nic_io->mag_cfg.pf_bw_limit;

	return 0;
}

int sss_tool_get_netdev_name(struct sss_nic_dev *nic_dev, const void *in_buf,
			     u32 in_len, void *out_buf, u32 *out_len)
{
	if (!out_buf || !out_len) {
		tool_err("Invalid param, use null pointer\n");
		return -EFAULT;
	}

	if (*out_len != IFNAMSIZ) {
		tool_err("Invalid out len %u is not equal to %u\n\n",
			 *out_len, IFNAMSIZ);
		return -EINVAL;
	}

	strscpy(out_buf, nic_dev->netdev->name, IFNAMSIZ);

	return 0;
}

int sss_tool_get_netdev_tx_timeout(struct sss_nic_dev *nic_dev, const void *in_buf,
				   u32 in_len, void *out_buf, u32 *out_len)
{
	int *tx_timeout = out_buf;
	struct net_device *net_dev = nic_dev->netdev;

	if (!out_buf || !out_len) {
		tool_err("Fail to get netdev tx timeout, use null pointer\n");
		return -EFAULT;
	}

	if (*out_len != sizeof(in_len)) {
		tool_err("Fail to get netdev tx timeout, out len %u is not equal to %lu\n",
			 *out_len, sizeof(in_len));
		return -EINVAL;
	}

	*tx_timeout = net_dev->watchdog_timeo;

	return 0;
}

int sss_tool_set_netdev_tx_timeout(struct sss_nic_dev *nic_dev, const void *in_buf,
				   u32 in_len, void *out_buf, u32 *out_len)
{
	const int *tx_timeout = in_buf;
	struct net_device *net_dev = nic_dev->netdev;

	if (!in_buf) {
		tool_err("Invalid in buf is null\n");
		return -EFAULT;
	}

	if (in_len != sizeof(in_len)) {
		tool_err("Invalid in len: %u is not equal to %lu\n",
			 in_len, sizeof(in_len));
		return -EINVAL;
	}

	net_dev->watchdog_timeo = *tx_timeout * HZ;
	tool_info("Success to set tx timeout check period to %ds\n", *tx_timeout);

	return 0;
}

int sss_tool_get_xsfp_present(struct sss_nic_dev *nic_dev, const void *in_buf,
			      u32 in_len, void *out_buf, u32 *out_len)
{
	struct sss_nic_mbx_get_xsfp_present *sfp_info = out_buf;

	if (!in_buf || !out_buf || !out_len) {
		tool_err("Invalid param, use null pointer\n");
		return -EFAULT;
	}

	if (in_len != sizeof(*sfp_info) || *out_len != sizeof(*sfp_info)) {
		tool_err("Invalid in len: %u or out len: %u is not equal to %lu\n",
			 in_len, *out_len, sizeof(*sfp_info));
		return -EINVAL;
	}

	sfp_info->abs_status = sss_nic_if_sfp_absent(nic_dev);
	sfp_info->head.state = 0;

	return 0;
}

int sss_tool_get_xsfp_info(struct sss_nic_dev *nic_dev, const void *in_buf,
			   u32 in_len, void *out_buf, u32 *out_len)
{
	int ret;
	struct sss_nic_mbx_get_xsfp_info *xsfp_info = out_buf;

	if (!in_buf || !out_buf || !out_len) {
		tool_err("Invalid param, use null pointer\n");
		return -EFAULT;
	}

	if (in_len != sizeof(*xsfp_info) || *out_len != sizeof(*xsfp_info)) {
		tool_err("Invalid in len: %u or out len: %u is not equal to %lu\n",
			 in_len, *out_len, sizeof(*xsfp_info));
		return -EINVAL;
	}

	ret = sss_nic_get_sfp_info(nic_dev, xsfp_info);
	if (ret != 0)
		xsfp_info->head.state = SSS_TOOL_EIO;

	return 0;
}
