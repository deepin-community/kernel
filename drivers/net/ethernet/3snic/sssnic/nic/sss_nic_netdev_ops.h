/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_NETDEV_OPS_H
#define SSS_NIC_NETDEV_OPS_H

#include <linux/types.h>
#include <linux/netdevice.h>

#include "sss_nic_dev_define.h"

void sss_nic_set_netdev_ops(struct sss_nic_dev *nic_dev);
bool sss_nic_is_netdev_ops_match(const struct net_device *netdev);

#endif
