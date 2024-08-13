/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_RX_RESET_H
#define SSS_NIC_RX_RESET_H

#include <linux/types.h>
#include <linux/device.h>
#include <linux/mm_types.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/u64_stats_sync.h>

void sss_nic_rq_watchdog_handler(struct work_struct *work);

#endif
