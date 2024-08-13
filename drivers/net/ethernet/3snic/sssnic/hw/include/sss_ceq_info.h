/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_CEQ_INFO_H
#define SSS_CEQ_INFO_H

#include <linux/types.h>

#include "sss_hw_ceq.h"
#include "sss_eq_info.h"

#define SSS_MAX_CEQ		32

typedef void (*sss_ceq_event_handler_t)(void *dev, u32 data);

struct sss_ceq_info {
	void					*hwdev;

	sss_ceq_event_handler_t		event_handler[SSS_CEQ_EVENT_MAX];
	void					*event_handler_data[SSS_CEQ_EVENT_MAX];
	void					*ceq_data[SSS_CEQ_EVENT_MAX];
	unsigned long			event_handler_state[SSS_CEQ_EVENT_MAX];

	struct sss_eq			ceq[SSS_MAX_CEQ];
	u16						num;
	u16						rsvd1;
	u32						rsvd2;
};
#endif
