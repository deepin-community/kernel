// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [TOOL]" fmt

#include "sss_nic_mag_cfg.h"
#include "sss_tool_comm.h"
#include "sss_tool_nic.h"
#include "sss_tool_nic_dcb.h"
#include "sss_tool_nic_qp_info.h"
#include "sss_tool_nic_phy_attr.h"
#include "sss_tool_nic_stats.h"

typedef int (*sss_tool_cmd_func)(struct sss_nic_dev *nic_dev,
				 const void *in_buf, u32 in_len,
				 void *out_buf, u32 *out_len);

struct sss_tool_cmd_handle {
	enum sss_tool_driver_cmd_type	cmd_type;
	sss_tool_cmd_func		func;
};

static int sss_tool_get_nic_version(void *out_buf, const u32 *out_len)
{
	struct sss_tool_drv_version_info *ver_info = out_buf;
	int ret;

	if (!out_buf || !out_len) {
		tool_err("Invalid param, use null pointer.\n");
		return -EINVAL;
	}

	if (*out_len != sizeof(*ver_info)) {
		tool_err("Invalid out len :%u is not equal to %lu\n",
			 *out_len, sizeof(*ver_info));
		return -EINVAL;
	}

	ret = snprintf(ver_info->ver, sizeof(ver_info->ver), "%s  %s",
		       SSSNIC_DRV_VERSION, __TIME_STR__);
	if (ret < 0)
		return -EINVAL;

	return 0;
}

static const struct sss_tool_cmd_handle sss_tool_nic_cmd_handle[] = {
	{SSS_TOOL_GET_TX_INFO,		sss_tool_get_tx_info},
	{SSS_TOOL_GET_RX_INFO,		sss_tool_get_rx_info},
	{SSS_TOOL_GET_TX_WQE_INFO,	sss_tool_get_tx_wqe_info},
	{SSS_TOOL_GET_RX_WQE_INFO,	sss_tool_get_rx_wqe_info},
	{SSS_TOOL_GET_Q_NUM,		sss_tool_get_q_num},
	{SSS_TOOL_GET_RX_CQE_INFO,	sss_tool_get_rx_cqe_info},
	{SSS_TOOL_GET_INTER_NUM,	sss_tool_get_inter_num},
	{SSS_TOOL_SET_PF_BW_LIMIT,	sss_tool_set_pf_bw_limit},
	{SSS_TOOL_GET_PF_BW_LIMIT,	sss_tool_get_pf_bw_limit},
	{SSS_TOOL_GET_LOOPBACK_MODE,	sss_tool_get_loopback_mode},
	{SSS_TOOL_SET_LOOPBACK_MODE,	sss_tool_set_loopback_mode},
	{SSS_TOOL_GET_TX_TIMEOUT,	sss_tool_get_netdev_tx_timeout},
	{SSS_TOOL_SET_TX_TIMEOUT,	sss_tool_set_netdev_tx_timeout},
	{SSS_TOOL_GET_SSET_COUNT,	sss_tool_get_sset_count},
	{SSS_TOOL_GET_SSET_ITEMS,	sss_tool_get_sset_stats},
	{SSS_TOOL_GET_XSFP_PRESENT,	sss_tool_get_xsfp_present},
	{SSS_TOOL_GET_XSFP_INFO,	sss_tool_get_xsfp_info},
	{SSS_TOOL_GET_ULD_DEV_NAME,	sss_tool_get_netdev_name},
	{SSS_TOOL_CLEAR_FUNC_STATS,	sss_tool_clear_func_stats},
	{SSS_TOOL_SET_LINK_MODE,	sss_tool_set_link_mode},
	{SSS_TOOL_DCB_STATE,		sss_tool_dcb_mt_dcb_state},
	{SSS_TOOL_QOS_DEV,		sss_tool_dcb_mt_qos_map},
	{SSS_TOOL_GET_QOS_COS,		sss_tool_dcb_mt_hw_qos_get},
};

static int sss_tool_cmd_to_nic_driver(struct sss_nic_dev *nic_dev,
				      u32 cmd, const void *in_buf,
				      u32 in_len, void *out_buf, u32 *out_len)
{
	int idx;
	int cmd_num = ARRAY_LEN(sss_tool_nic_cmd_handle);
	enum sss_tool_driver_cmd_type cmd_type = (enum sss_tool_driver_cmd_type)cmd;
	int ret = -EINVAL;

	mutex_lock(&nic_dev->qp_mutex);
	for (idx = 0; idx < cmd_num; idx++) {
		if (cmd_type == sss_tool_nic_cmd_handle[idx].cmd_type) {
			ret = sss_tool_nic_cmd_handle[idx].func
					(nic_dev, in_buf, in_len, out_buf, out_len);
			break;
		}
	}
	mutex_unlock(&nic_dev->qp_mutex);

	if (idx == cmd_num)
		tool_err("Fail to send to nic driver, cmd %d is not exist\n", cmd_type);

	return ret;
}

int sss_tool_ioctl(void *uld_dev, u32 cmd, const void *in_buf,
		   u32 in_len, void *out_buf, u32 *out_len)
{
	if (cmd == SSS_TOOL_GET_DRV_VERSION)
		return sss_tool_get_nic_version(out_buf, out_len);

	if (!uld_dev)
		return -EINVAL;

	return sss_tool_cmd_to_nic_driver(uld_dev, cmd, in_buf, in_len, out_buf, out_len);
}
