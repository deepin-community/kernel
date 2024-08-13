/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_FILTER_H
#define SSS_NIC_FILTER_H

#include <linux/workqueue.h>
#include "sss_nic_dev_define.h"

void sss_nic_set_rx_mode_work(struct work_struct *work);
void sss_nic_clean_mac_list_filter(struct sss_nic_dev *nic_dev);

#endif
