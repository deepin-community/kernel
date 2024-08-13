/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_ETHTOOL_H
#define SSS_NIC_ETHTOOL_H

#include <linux/netdevice.h>

void sss_nic_set_ethtool_ops(struct sss_nic_dev *adapter);
#endif
