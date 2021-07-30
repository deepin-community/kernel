/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HYGON Platform Security Processor (PSP) interface driver
 *
 * Copyright (C) 2016-2023 Hygon Info Technologies Ltd.
 *
 * Author: Baoshun Fang <baoshunfang@hygon.cn>
 */

#ifndef __CCP_HYGON_RINGBUF_H__
#define __CCP_HYGON_RINGBUF_H__

#include <linux/psp-hygon.h>

int csv_queue_init(struct csv_queue *queue,
		   void *buffer, unsigned int size, size_t esize);
void csv_queue_cleanup(struct csv_queue *queue);

#endif /* __CCP_HYGON_RINGBUF_H__ */
