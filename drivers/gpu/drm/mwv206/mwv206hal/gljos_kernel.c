/*
 * JM7200 GPU driver
 *
 * Copyright (c) 2018 ChangSha JingJiaMicro Electronics Co., Ltd.
 *
 * Author:
 *      rfshen <jjwgpu@jingjiamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>

#include "gljos_kernel.h"

GLJOS_SPINLOCK FUNC206HAL092(void)
{
	spinlock_t *sl = NULL;

	sl = (spinlock_t *)kzalloc(sizeof(spinlock_t), GFP_KERNEL);
	if (sl) {
		spin_lock_init(sl);
	}

	return (GLJOS_SPINLOCK)sl;
}

unsigned long FUNC206HAL094(GLJOS_SPINLOCK sl)
{
	unsigned long flags;

	spin_lock_irqsave((spinlock_t *)sl, flags);
	return flags;
}

void FUNC206HAL095(GLJOS_SPINLOCK sl, unsigned long flags)
{
	spin_unlock_irqrestore((spinlock_t *)sl, flags);
}

void FUNC206HAL093(GLJOS_SPINLOCK sl)
{
	if (sl != 0) {
		kfree((spinlock_t *)sl);
	}
}


GLJOS_WAIT_QUEUE_HEAD FUNC206HAL097(void)
{
	wait_queue_head_t *wait_head = NULL;
	wait_head = (wait_queue_head_t *)kzalloc(sizeof(wait_queue_head_t),
			GFP_KERNEL);
	if (wait_head) {
		init_waitqueue_head(wait_head);
	}
	return (GLJOS_WAIT_QUEUE_HEAD)wait_head;
}

int FUNC206HAL096(GLJOS_WAIT_QUEUE_HEAD wq, int condition)
{
	wait_queue_head_t h = *((wait_queue_head_t *)wq);
	return wait_event_interruptible(h,
			condition);
}

void FUNC206HAL099(GLJOS_WAIT_QUEUE_HEAD wq)
{
	wake_up_interruptible_all((wait_queue_head_t *)wq);
}

void FUNC206HAL098(GLJOS_WAIT_QUEUE_HEAD wq)
{
	if (wq != 0) {
		kfree((wait_queue_head_t *)wq);
	}
}


GLJOS_SEMAPHORE FUNC206HAL088(void)
{
	struct semaphore *sema = NULL;
	sema = (struct semaphore *)kzalloc(sizeof(struct semaphore),
			GFP_KERNEL);
	if (sema) {
		sema_init(sema, 1);
	}
	return (GLJOS_SEMAPHORE)sema;
}

int FUNC206HAL090(GLJOS_SEMAPHORE s)
{
	return down_interruptible((struct semaphore *)s);
}

void FUNC206HAL091(GLJOS_SEMAPHORE s)
{
	up((struct semaphore *)s);
}

void FUNC206HAL089(GLJOS_SEMAPHORE s)
{
	if (s != 0) {
		kfree((struct semaphore *)s);
	}
}


GLJOS_ATOMIC FUNC206HAL072(void)
{
	atomic_t *a = NULL;
	a = (atomic_t *)kzalloc(sizeof(atomic_t),
			GFP_KERNEL);
	if (a) {
		memset(a, 0, sizeof(atomic_t));
	}
	return (GLJOS_ATOMIC)a;
}

int FUNC206HAL076(GLJOS_ATOMIC a)
{
	return atomic_read((atomic_t *)a);
}

void FUNC206HAL075(GLJOS_ATOMIC a)
{
	atomic_inc((atomic_t *)a);
}

void FUNC206HAL073(GLJOS_ATOMIC a)
{
	atomic_dec((atomic_t *)a);
}

void FUNC206HAL074(GLJOS_ATOMIC a)
{
	if (a != 0) {
		kfree((atomic_t *)a);
	}
}


void FUNC206HAL085(int val, int *addr)
{
	__put_user(val, addr);
}

void FUNC206HAL086(u32 val, u32 *addr)
{
	__put_user(val, addr);
}

void FUNC206HAL087(unsigned long val, unsigned long *addr)
{
	__put_user(val, addr);
}

int FUNC206HAL081(int *addr)
{
	int v;
	__get_user(v, addr);
	return v;
}

unsigned long FUNC206HAL083(unsigned long *addr)
{
	unsigned long v;
	__get_user(v, addr);
	return v;
}

unsigned int FUNC206HAL082(unsigned int *addr)
{
	unsigned int v;
	__get_user(v, addr);
	return v;
}


unsigned long FUNC206HAL077(void *to, const void  *from,
		unsigned long n)
{
	return copy_from_user(to, from, n);
}

unsigned long FUNC206HAL078(void  *to, const void *from,
		unsigned long n)
{
	return copy_to_user(to, from, n);
}


void FUNC206HAL079(unsigned int i)
{
	disable_irq(i);
}

void FUNC206HAL080(unsigned int i)
{
	enable_irq(i);
}


void *FUNC206HAL084(void *s, int c, unsigned int n)
{
	return memset(s, c, n);
}