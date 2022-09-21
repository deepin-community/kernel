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
#ifndef __GLJ_KERNEL_H__
#define __GLJ_KERNEL_H__


typedef unsigned int u32;
typedef unsigned long  GLJOS_SPINLOCK;
typedef unsigned long GLJOS_WAIT_QUEUE_HEAD;
typedef unsigned long GLJOS_SEMAPHORE;
typedef unsigned long GLJOS_ATOMIC;


GLJOS_SPINLOCK FUNC206HAL092(void);
unsigned long FUNC206HAL094(GLJOS_SPINLOCK sl);
void FUNC206HAL095(GLJOS_SPINLOCK sl, unsigned long flags);
void FUNC206HAL093(GLJOS_SPINLOCK sl);


GLJOS_WAIT_QUEUE_HEAD FUNC206HAL097(void);
int FUNC206HAL096(GLJOS_WAIT_QUEUE_HEAD wq, int condition);
void FUNC206HAL099(GLJOS_WAIT_QUEUE_HEAD wq);
void FUNC206HAL098(GLJOS_WAIT_QUEUE_HEAD wq);


GLJOS_SEMAPHORE FUNC206HAL088(void);
int FUNC206HAL090(GLJOS_SEMAPHORE s);
void FUNC206HAL091(GLJOS_SEMAPHORE s);
void FUNC206HAL089(GLJOS_SEMAPHORE s);


GLJOS_ATOMIC FUNC206HAL072(void);
int FUNC206HAL076(GLJOS_ATOMIC a);
void FUNC206HAL075(GLJOS_ATOMIC a);
void FUNC206HAL073(GLJOS_ATOMIC a);
void FUNC206HAL074(GLJOS_ATOMIC a);


void FUNC206HAL085(int val, int *addr);
void FUNC206HAL086(u32 val, u32 *addr);
void FUNC206HAL087(unsigned long val, unsigned long *addr);
int FUNC206HAL081(int *addr);
unsigned int FUNC206HAL082(unsigned int *addr);
unsigned long FUNC206HAL083(unsigned long *addr);


unsigned long FUNC206HAL077(void *to, const void  *from,
		unsigned long n);
unsigned long FUNC206HAL078(void  *to, const void *from,
		unsigned long n);


void FUNC206HAL079(unsigned int i);
void FUNC206HAL080(unsigned int i);


void *FUNC206HAL084(void *s, int c, unsigned int n);

#endif