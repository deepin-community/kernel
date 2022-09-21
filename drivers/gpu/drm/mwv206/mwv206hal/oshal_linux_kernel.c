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
#include <linux/semaphore.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/interrupt.h>


#include "oshal.h"
#include "jmirq.h"
#include "gljos.h"
#include "mwv206dev.h"
#include "mwv206oshal.h"


OSHAL_TASK FUNC206HAL450(char *name, int priority, int options, int stackSize, OSHAL_TASKFUNC entry,
		unsigned long param0, unsigned long param1)
{
	return 0;
}

int FUNC206HAL452 (OSHAL_TASK *ptaskId)
{
	return 0;
}

int FUNC206HAL449 (OSHAL_TASK taskId)
{
	return 0;
}

int FUNC206HAL455 (OSHAL_TASK taskId)
{
	return 0;
}

OSHAL_TASK FUNC206HAL430(void)
{
	return 0;
}

int FUNC206HAL454(void)
{
	return 0;
}

int FUNC206HAL456(void)
{
	return 0;
}

int FUNC206HAL453(OSHAL_TASK taskId)
{
	return 0;
}

int FUNC206HAL438(OSHAL_TASK taskId)
{
	return 0;
}



int FUNC206HAL451(int ticks)
{
	FUNC206LXDEV128(ticks);
	return 0;
}

int FUNC206HAL457(void)
{
	return 0;
}

int FUNC206HAL424(void)
{
	return 60;
}

int FUNC206HAL425(int clkrate)
{
	return 0;
}


OSHAL_LOCK FUNC206HAL440(void)
{
	return FUNC206LXDEV118();
}

int FUNC206HAL441(OSHAL_LOCK lockId)
{
	FUNC206LXDEV119(lockId);
	return 0;
}

int FUNC206HAL439(OSHAL_LOCK lockId, int timeout)
{
	return FUNC206LXDEV116(lockId);
}

int FUNC206HAL458(OSHAL_LOCK lockId)
{
	return FUNC206LXDEV135(lockId);
}

int FUNC206HAL459(OSHAL_LOCK lockId)
{
	FUNC206LXDEV136(lockId);
	return 0;
}


OSHAL_EVENT FUNC206HAL426(int initialStatus)
{
	return FUNC206LXDEV101();
}


int FUNC206HAL427(OSHAL_EVENT eventId)
{
	FUNC206LXDEV102(eventId);
	return 0;
}


int FUNC206HAL429(OSHAL_EVENT eventId, int timeout)
{
	return FUNC206LXDEV105(eventId, timeout);
}


int FUNC206HAL428(OSHAL_EVENT eventId)
{
	return FUNC206LXDEV103(eventId);
}



OSHAL_MSGQ FUNC206HAL444(int maxMessages, int maxMsgSize)
{
	return 0;
}

int FUNC206HAL445(OSHAL_MSGQ qid)
{
	return 0;
}

int FUNC206HAL448(OSHAL_MSGQ qid, void *message, int msgSize, int wait)
{
	return 0;
}

int FUNC206HAL446 (OSHAL_MSGQ qid, void *message, int msgSize, int wait)
{
	return 0;
}

int FUNC206HAL447(OSHAL_MSGQ qid)
{
	return 0;
}


int FUNC206HAL435(int *save)
{
	return 0;
}

int FUNC206HAL436(int *restore)
{
	return 0;
}

int FUNC206HAL431(void *devInfo, unsigned int interruptId, OSHAL_IRQFUNC fptr, const char *dev_name, void *param)
{
	return FUNC206LXDEV108(interruptId, fptr, param);
}


int FUNC206HAL433(int interruptId, OSHAL_IRQFUNC fptr, void *param)
{
	return FUNC206LXDEV110(interruptId, fptr, param);

}

int FUNC206HAL434(int interruptId)
{
	return  FUNC206LXDEV111(interruptId);

}

int FUNC206HAL432(int interruptId)
{
	return FUNC206LXDEV109(interruptId);
}


void *FUNC206HAL442(int size, int alignsize)
{
	return FUNC206LXDEV010(size);
}

int FUNC206HAL443(void *pMem)
{
	return FUNC206LXDEV120(pMem);
}


void FUNC206HAL437(OS_JMIRQ_MSG_QUEUE msgPressQueue, P_JMIRQ_MSG pIrqMsg)
{
	queue_work((struct workqueue_struct *)msgPressQueue, (struct work_struct *)&pIrqMsg->mIrqMsg);
}


int FUNC206HAL017(P_JMIRQ_MSG pIrqMsg, IRQMSG_PRESS_CALLBACK callback)
{
	INIT_WORK((struct work_struct *)&pIrqMsg->mIrqMsg, (void *)callback);

	return 0;
}