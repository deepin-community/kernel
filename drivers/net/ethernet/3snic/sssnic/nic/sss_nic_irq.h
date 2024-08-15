/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_IRQ_H
#define SSS_NIC_IRQ_H

#include <linux/types.h>

#include "sss_kernel.h"
#include "sss_nic_dev_define.h"

int sss_nic_request_qp_irq(struct sss_nic_dev *nic_dev);
void sss_nic_release_qp_irq(struct sss_nic_dev *nic_dev);

#endif
