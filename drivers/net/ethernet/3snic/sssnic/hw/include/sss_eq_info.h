/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_EQ_INFO_H
#define SSS_EQ_INFO_H

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#include "sss_hw_common.h"
#include "sss_hw_irq.h"
#include "sss_hw_svc_cap.h"

#define SSS_EQ_IRQ_NAME_LEN		64

enum sss_eq_type {
	SSS_AEQ,
	SSS_CEQ
};

typedef void (*sss_init_desc_handler_t)(void *eq);
typedef u32 (*sss_chip_init_attr_handler_t)(void *eq);

struct sss_eq {
	char				*name;
	void				*hwdev;
	enum sss_eq_type	type;
	u32					page_size;
	u32					old_page_size;
	u32					len;

	u32		ci;

	u16		wrap;
	u16		qid;

	u16		entry_size;
	u16		page_num;

	u32		num_entry_per_pg;

	struct sss_irq_desc		irq_desc;
	char					irq_name[SSS_EQ_IRQ_NAME_LEN];

	struct sss_dma_addr_align	*page_array;

	struct work_struct			aeq_work;
	struct tasklet_struct		ceq_tasklet;

	u64		hw_intr_jiffies;
	u64		sw_intr_jiffies;

	sss_init_desc_handler_t			init_desc_handler;
	sss_chip_init_attr_handler_t	init_attr_handler;
	irq_handler_t					irq_handler;
};

struct sss_eq_cfg {
	enum sss_service_type type;
	int id;
	int free; /* 1 - alocated, 0- freed */
};

struct sss_eq_info {
	struct sss_eq_cfg *eq;

	u8 ceq_num;

	u8 remain_ceq_num;

	/* mutex used for allocate EQs */
	struct mutex eq_mutex;
};

#endif
