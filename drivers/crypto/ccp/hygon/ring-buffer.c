// SPDX-License-Identifier: GPL-2.0-only
/*
 * HYGON Platform Security Processor (PSP) interface
 *
 * Copyright (C) 2016-2023 Hygon Info Technologies Ltd.
 *
 * Author: Baoshun Fang <baoshunfang@hygon.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/align.h>
#include <linux/string.h>

#include "ring-buffer.h"

int csv_queue_init(struct csv_queue *queue,
		   void *buffer, unsigned int size, size_t esize)
{
	size /= esize;

	queue->head = 0;
	queue->tail = 0;
	queue->esize = esize;
	queue->data = (u64)buffer;
	queue->mask = size - 1;
	queue->data_align = ALIGN(queue->data, CSV_RING_BUFFER_ALIGN);

	return 0;
}

void csv_queue_cleanup(struct csv_queue *queue)
{
	memset((void *)queue, 0, sizeof(struct csv_queue));
}
