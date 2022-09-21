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
#ifndef _MWV206_OSHAL_H_
#define _MWV206_OSHAL_H_


#define IAR         0

#undef OS_VXWORKS
#undef OS_LINUX

#define OS_VXWORKS      1
#define OS_LINUX        2

#define CUR_OS      OS_LINUX


#define _SOC_               0
#define PCIE_ENDPOINT       1

#define CUR_APP_SCENCE      PCIE_ENDPOINT

#ifndef WAIT_FOREVER
#define WAIT_FOREVER -1U
#endif

#ifndef OK
#define OK  0
#endif

#ifndef FULL
#define FULL    1
#endif


#ifndef EMPTY
#define EMPTY   0
#endif


#ifndef NULL
#define NULL ((void *)0)
#endif


typedef int BOOL;

#ifndef FALSE
#define FALSE   0
#endif


#ifndef TRUE
#define TRUE    1
#endif

#define EOS             '\0'


#define OK              0


#define NO_WAIT         0

typedef unsigned long OSHAL_TASK;
typedef unsigned long OSHAL_LOCK;
typedef unsigned long OSHAL_EVENT;
typedef unsigned long OSHAL_MSGQ;
typedef unsigned long OSHAL_DEVICEDRV;

typedef int (*OSHAL_TASKFUNC)(unsigned long p0, unsigned long p1);
typedef int (*OSHAL_IRQFUNC)(int, void *);


OSHAL_LOCK  FUNC206HAL440(void);
int         FUNC206HAL441(OSHAL_LOCK lockId);
int         FUNC206HAL439(OSHAL_LOCK lockId, int timeout);
int         FUNC206HAL458(OSHAL_LOCK lockId);
int         FUNC206HAL459(OSHAL_LOCK lockId);


void       *FUNC206HAL442(int size, int alignsize);
int         FUNC206HAL443(void *pMem);
#endif