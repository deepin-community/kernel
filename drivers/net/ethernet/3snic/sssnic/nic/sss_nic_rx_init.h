/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_RX_INIT_H
#define SSS_NIC_RX_INIT_H

#include <linux/types.h>
#include <linux/device.h>
#include <linux/mm_types.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/u64_stats_sync.h>

#include "sss_nic_io.h"
#include "sss_nic_dev_define.h"

int sss_nic_alloc_rq_res_group(struct sss_nic_dev *nic_dev,
			       struct sss_nic_qp_resource *qp_res);

void sss_nic_free_rq_res_group(struct sss_nic_dev *nic_dev,
			       struct sss_nic_qp_resource *qp_res);

int sss_nic_init_rq_desc_group(struct sss_nic_dev *nic_dev,
			       struct sss_nic_qp_resource *qp_res);

int sss_nic_alloc_rq_desc_group(struct sss_nic_dev *nic_dev);

void sss_nic_free_rq_desc_group(struct sss_nic_dev *nic_dev);

int sss_nic_update_rx_rss(struct sss_nic_dev *nic_dev);

void sss_nic_reset_rx_rss(struct net_device *netdev);

#endif
