/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_STATISTICS_H
#define SSS_HW_STATISTICS_H

#include <linux/types.h>
#include <linux/atomic.h>

#include "sss_hw_event.h"
#include "sss_hw_aeq.h"

struct sss_qmm_stats {
	atomic_t qmm_rsv_cnt[134];
};

struct sss_link_event_stats {
	atomic_t link_down_stats;
	atomic_t link_up_stats;
};

struct sss_fault_event_stats {
	atomic_t chip_fault_stats[22][SSS_FAULT_LEVEL_MAX];
	atomic_t fault_type_stat[SSS_FAULT_TYPE_MAX];
	atomic_t pcie_fault_stats;
};

struct sss_hw_stats {
	atomic_t heart_lost_stats;
	struct sss_qmm_stats qmm_stats;
	struct sss_link_event_stats link_event_stats;
	struct sss_fault_event_stats fault_event_stats;
	atomic_t nic_ucode_event_stats[SSS_ERR_MAX];
};

#define SSS_CHIP_FAULT_SIZE (110 * 1024)

#endif
