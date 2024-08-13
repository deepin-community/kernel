/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_TX_INIT_H
#define SSS_NIC_TX_INIT_H

#include <net/ipv6.h>
#include <net/checksum.h>
#include <net/ip6_checksum.h>
#include <linux/ip.h>
#include <linux/ipv6.h>

#include "sss_nic_io.h"
#include "sss_nic_dev_define.h"

int sss_nic_alloc_sq_desc_group(struct sss_nic_dev *nic_dev);
void sss_nic_free_sq_desc_group(struct sss_nic_dev *nic_dev);
int sss_nic_alloc_sq_resource(struct sss_nic_dev *nic_dev,
			      struct sss_nic_qp_resource *qp_res);
void sss_nic_free_sq_resource(struct sss_nic_dev *nic_dev,
			      struct sss_nic_qp_resource *qp_res);
void sss_nic_init_all_sq(struct sss_nic_dev *nic_dev,
			 struct sss_nic_qp_resource *qp_res);
void sss_nic_flush_all_sq(struct sss_nic_dev *nic_dev);

#endif
