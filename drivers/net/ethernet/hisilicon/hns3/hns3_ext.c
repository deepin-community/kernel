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
