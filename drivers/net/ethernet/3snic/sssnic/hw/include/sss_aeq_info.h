/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_AEQ_INFO_H
#define SSS_AEQ_INFO_H

#include <linux/types.h>
#include <linux/workqueue.h>

#include "sss_eq_info.h"
#include "sss_hw_aeq.h"

#define SSS_MAX_AEQ				4

typedef void (*sss_aeq_hw_event_handler_t)(void *pri_handle, u8 *data, u8 size);
typedef u8 (*sss_aeq_sw_event_handler_t)(void *pri_handle, u8 event, u8 *data);

struct sss_aeq_info {
	void						*hwdev;

	sss_aeq_hw_event_handler_t			hw_event_handler[SSS_AEQ_EVENT_MAX];
	void						*hw_event_data[SSS_AEQ_EVENT_MAX];
	sss_aeq_sw_event_handler_t			sw_event_handler[SSS_AEQ_SW_EVENT_MAX];
	void						*sw_event_data[SSS_AEQ_SW_EVENT_MAX];
	unsigned long				hw_event_handler_state[SSS_AEQ_EVENT_MAX];
	unsigned long				sw_event_handler_state[SSS_AEQ_SW_EVENT_MAX];

	struct sss_eq				aeq[SSS_MAX_AEQ];
	u16							num;
	u16							rsvd1;
	u32							rsvd2;
	struct workqueue_struct		*workq;
};
#endif
