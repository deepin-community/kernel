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
#ifndef _OSHAL_H_
#define _OSHAL_H_


#include "mwv206oshal.h"


#define MSB(x)  (((x) >> 8) & 0xff)
#define LSB(x)  ((x) & 0xff)
#define MSW(x) (((x) >> 16) & 0xffff)
#define LSW(x) ((x) & 0xffff)


#define WORDSWAP(x) (MSW(x) | (LSW(x) << 16))

#define LLSB(x) ((x) & 0xff)
#define LNLSB(x) (((x) >> 8) & 0xff)
#define LNMSB(x) (((x) >> 16) & 0xff)
#define LMSB(x)  (((x) >> 24) & 0xff)
#define LONGSWAP(x) ((LLSB(x) << 24) | \
		(LNLSB(x) << 16)| \
		(LNMSB(x) << 8) | \
		(LMSB(x)))

#define OFFSET(structure, member)   \
	((long) &(((structure *) 0)->member))

#define MEMBER_SIZE(structure, member)  \
	(sizeof (((structure *) 0)->member))

#define NELEMENTS(array)         \
	(sizeof (array) / sizeof ((array)[0]))


#define CHAR_FROM_CONST(x)  (char *)(x)
#define VOID_FROM_CONST(x)  (void *)(x)

#define FOREVER for (;;)


#define FAST

#define IMPORT  extern
#define LOCAL   static

#include <linux/workqueue.h>

typedef struct _tag_irq_notify_msg {
	struct work_struct      mIrqMsg;
} JMIRQ_MSG, *P_JMIRQ_MSG;


typedef unsigned long OS_JMIRQ_MSG_QUEUE;


typedef void (*IRQMSG_PRESS_CALLBACK)(void *param);

#define GLJ_OK              0
#define GLJ_ERROR           -1
#define TASK_USE_FLOAT      1

#ifdef __cplusplus
extern "C" {
#endif



OSHAL_TASK  FUNC206HAL450(char *name, int priority, int options, int stackSize, OSHAL_TASKFUNC entry,
		unsigned long param0, unsigned long param1);
int         FUNC206HAL452 (OSHAL_TASK *ptaskId);
int         FUNC206HAL449 (OSHAL_TASK taskId);
int         FUNC206HAL455 (OSHAL_TASK taskId);
OSHAL_TASK  FUNC206HAL430 (void);
int         FUNC206HAL454 (void);
int         FUNC206HAL456 (void);
int         FUNC206HAL453(OSHAL_TASK taskId);
int         FUNC206HAL438(OSHAL_TASK taskId);



int         FUNC206HAL451(int ticks);
int         FUNC206HAL457(void);
int         FUNC206HAL424(void);
int         FUNC206HAL425(int clkrate);



OSHAL_EVENT FUNC206HAL426(int initialStatus);
int         FUNC206HAL427(OSHAL_EVENT eventId);
int         FUNC206HAL429(OSHAL_EVENT eventId, int timeout);
int         FUNC206HAL428(OSHAL_EVENT eventId);



OSHAL_MSGQ  FUNC206HAL444(int maxMessages, int maxMsgSize);
int         FUNC206HAL445 (OSHAL_MSGQ qid);
int         FUNC206HAL448(OSHAL_MSGQ qid, void *message, int msgSize, int wait);
int         FUNC206HAL446 (OSHAL_MSGQ qid, void *message, int msgSize, int wait);
int         FUNC206HAL447(OSHAL_MSGQ qid);




int         FUNC206HAL435(int *save);
int         FUNC206HAL436(int *restore);
int         FUNC206HAL431(void *devInfo, unsigned int interruptId, OSHAL_IRQFUNC fptr, const char *dev_name,
		void *param);
int         FUNC206HAL433(int interruptId, OSHAL_IRQFUNC fptr, void *param);
int         FUNC206HAL434(int interruptId);
int         FUNC206HAL432(int interruptId);





void FUNC206HAL437(OS_JMIRQ_MSG_QUEUE msgPressQueue, P_JMIRQ_MSG pIrqMsg);



int FUNC206HAL017(P_JMIRQ_MSG pIrqMsg, IRQMSG_PRESS_CALLBACK callback);



OS_JMIRQ_MSG_QUEUE osCreateIrqMsgQueue(const char *name);



void OSDestroyIrqMsgPressQueue(OS_JMIRQ_MSG_QUEUE queue);



#ifdef __cplusplus
}
#endif
#endif