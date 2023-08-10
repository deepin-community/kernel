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
#include <linux/minmax.h>

#include <asm/barrier.h>

#include "ring-buffer.h"

static void enqueue_data(struct csv_queue *queue,
			 const void *src,
			 unsigned int len, unsigned int off)
{
	unsigned int size = queue->mask + 1;
	unsigned int esize = queue->esize;
	unsigned int l;
	void *data;

	off &= queue->mask;
	if (esize != 1) {
		off *= esize;
		size *= esize;
		len *= esize;
	}
	l = min(len, size - off);

	data = (void *)queue->data_align;
	memcpy(data + off, src, l);
	memcpy(data, src + l, len - l);

	/*
	 * Make sure that the data in the ring buffer is up to date before
	 * incrementing the queue->tail index counter.
	 */
	smp_wmb();
}

static void dequeue_data(struct csv_queue *queue,
			 void *dst, unsigned int len, unsigned int off)
{
	unsigned int size = queue->mask + 1;
	unsigned int esize = queue->esize;
	unsigned int l;

	off &= queue->mask;
	if (esize != 1) {
		off *= esize;
		size *= esize;
		len *= esize;
	}
	l = min(len, size - off);

	memcpy(dst, (void *)(queue->data + off), l);
	memcpy((void *)((uintptr_t)dst + l), (void *)queue->data, len - l);

	/*
	 * Make sure that the data is copied before incrementing the
	 * queue->tail index counter.
	 */
	smp_wmb();
}

static unsigned int queue_avail_size(struct csv_queue *queue)
{
	/*
	 * According to the nature of unsigned Numbers, it always work
	 * well even though tail < head. Reserved 1 element to distinguish
	 * full and empty.
	 */
	return queue->mask - (queue->tail - queue->head);
}

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

unsigned int csv_enqueue_cmd(struct csv_queue *queue,
			     const void *buf, unsigned int len)
{
	unsigned int size;

	size = queue_avail_size(queue);
	if (len > size)
		len = size;

	enqueue_data(queue, buf, len, queue->tail);
	queue->tail += len;
	return len;
}

unsigned int csv_dequeue_stat(struct csv_queue *queue,
			      void *buf, unsigned int len)
{
	unsigned int size;

	size = queue->tail - queue->head;
	if (len > size)
		len = size;

	dequeue_data(queue, buf, len, queue->head);
	queue->head += len;
	return len;
}

unsigned int csv_dequeue_cmd(struct csv_queue *ring_buf,
	void *buf, unsigned int len)
{
	unsigned int size;

	size = ring_buf->tail - ring_buf->head;
	if (len > size)
		len = size;

	dequeue_data(ring_buf, buf, len, ring_buf->head);
	ring_buf->head += len;
	return len;
}

unsigned int csv_cmd_queue_size(struct csv_queue *ring_buf)
{
	unsigned int free_size;

	free_size = queue_avail_size(ring_buf);
	return ring_buf->mask - free_size;
}
