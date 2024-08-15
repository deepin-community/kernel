/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_IRQ_INFO_H
#define SSS_IRQ_INFO_H

#include <linux/types.h>
#include <linux/mutex.h>

#include "sss_hw_svc_cap.h"
#include "sss_hw_irq.h"

struct sss_irq {
	enum sss_service_type type;
	int busy; /* 1 - allocated, 0 - freed */
	struct sss_irq_desc desc;
};

struct sss_irq_info {
	struct sss_irq *irq;
	u16 total_num;
	u16 free_num;
	u16 max_num; /* device max irq number */

	struct mutex irq_mutex; /* mutex is used to allocate eq */
};

#endif
