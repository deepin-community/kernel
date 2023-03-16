// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023 Hisilicon Limited.

#include "hns3_ext.h"

int nic_netdev_match_check(struct net_device *ndev)
{
#define HNS3_DRIVER_NAME_LEN 5

	struct ethtool_drvinfo drv_info;
	struct hnae3_handle *h;

	if (!ndev || !ndev->ethtool_ops ||
	    !ndev->ethtool_ops->get_drvinfo)
		return -EINVAL;

	ndev->ethtool_ops->get_drvinfo(ndev, &drv_info);

	if (strncmp(drv_info.driver, "hns3", HNS3_DRIVER_NAME_LEN))
		return -EINVAL;

	h = hns3_get_handle(ndev);
	if (h->flags & HNAE3_SUPPORT_VF)
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL(nic_netdev_match_check);

static int nic_invoke_pri_ops(struct net_device *ndev, int opcode,
			      void *data, size_t length)

{
	struct hnae3_handle *h;
	int ret;

	if ((!data && length) || (data && !length)) {
		netdev_err(ndev, "failed to check data and length");
		return -EINVAL;
	}

	if (nic_netdev_match_check(ndev))
		return -ENODEV;

	h = hns3_get_handle(ndev);
	if (!h->ae_algo->ops->priv_ops)
		return -EOPNOTSUPP;

	ret = h->ae_algo->ops->priv_ops(h, opcode, data, length);
	if (ret)
		netdev_err(ndev,
			   "failed to invoke pri ops, opcode = %#x, ret = %d\n",
			   opcode, ret);

	return ret;
}

void nic_chip_recover_handler(struct net_device *ndev,
			      enum hnae3_event_type_custom event_t)
{
	dev_info(&ndev->dev, "reset type is %d!!\n", event_t);

	if (event_t == HNAE3_PPU_POISON_CUSTOM)
		event_t = HNAE3_FUNC_RESET_CUSTOM;

	if (event_t != HNAE3_FUNC_RESET_CUSTOM &&
	    event_t != HNAE3_GLOBAL_RESET_CUSTOM &&
	    event_t != HNAE3_IMP_RESET_CUSTOM) {
		dev_err(&ndev->dev, "reset type err!!\n");
		return;
	}

	nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_RESET, &event_t, sizeof(event_t));
}
EXPORT_SYMBOL(nic_chip_recover_handler);

static int nic_check_pfc_storm_para(u32 dir, u32 enable, u32 period_ms,
				    u32 times, u32 recovery_period_ms)
{
	if ((dir != HNS3_PFC_STORM_PARA_DIR_RX &&
	     dir != HNS3_PFC_STORM_PARA_DIR_TX) ||
	     (enable != HNS3_PFC_STORM_PARA_DISABLE &&
	      enable != HNS3_PFC_STORM_PARA_ENABLE))
		return -EINVAL;

	if (period_ms < HNS3_PFC_STORM_PARA_PERIOD_MIN ||
	    period_ms > HNS3_PFC_STORM_PARA_PERIOD_MAX ||
	    recovery_period_ms < HNS3_PFC_STORM_PARA_PERIOD_MIN ||
	    recovery_period_ms > HNS3_PFC_STORM_PARA_PERIOD_MAX ||
	    times <= 0)
		return -EINVAL;

	return 0;
}

int nic_set_pfc_storm_para(struct net_device *ndev, u32 dir, u32 enable,
			   u32 period_ms, u32 times, u32 recovery_period_ms)
{
	struct hnae3_pfc_storm_para para;

	if (nic_check_pfc_storm_para(dir, enable, period_ms, times,
				     recovery_period_ms)) {
		dev_err(&ndev->dev,
			"set pfc storm para failed because invalid input param.\n");
		return -EINVAL;
	}

	para.dir = dir;
	para.enable = enable;
	para.period_ms = period_ms;
	para.times = times;
	para.recovery_period_ms = recovery_period_ms;

	return nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_SET_PFC_STORM_PARA,
				  &para, sizeof(para));
}
EXPORT_SYMBOL(nic_set_pfc_storm_para);

int nic_get_pfc_storm_para(struct net_device *ndev, u32 dir, u32 *enable,
			   u32 *period_ms, u32 *times, u32 *recovery_period_ms)
{
	struct hnae3_pfc_storm_para para;
	int ret;

	if (!enable || !period_ms || !times || !recovery_period_ms ||
	    (dir != HNS3_PFC_STORM_PARA_DIR_RX &&
	     dir != HNS3_PFC_STORM_PARA_DIR_TX)) {
		dev_err(&ndev->dev,
			"get pfc storm para failed because invalid input param.\n");
		return -EINVAL;
	}

	para.dir = dir;
	ret = nic_invoke_pri_ops(ndev, HNAE3_EXT_OPC_GET_PFC_STORM_PARA,
				 &para, sizeof(para));
	if (ret)
		return ret;

	*enable = para.enable;
	*period_ms = para.period_ms;
	*times = para.times;
	*recovery_period_ms = para.recovery_period_ms;
	return 0;
}
EXPORT_SYMBOL(nic_get_pfc_storm_para);
