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
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/freezer.h>
#include <asm/dma-mapping.h>
#include "gljos.h"
#include "mwv206dev.h"
#include "mwv206kdebug.h"


GLJ_TASK FUNC206LXDEV127(char *name, int priority, int options, int stackSize, GLJOSTASKFUNC entry,
		unsigned long param0, unsigned long param1)
{
	return 0;
}

int FUNC206LXDEV129 (GLJ_TASK *ptaskId)
{
	return 0;
}

int FUNC206LXDEV126 (GLJ_TASK taskId)
{
	return 0;
}

int FUNC206LXDEV132 (GLJ_TASK taskId)
{
	return 0;
}

GLJ_TASK FUNC206LXDEV107(void)
{
	return get_current()->pid;
}

GLJ_TASK FUNC206LXDEV106(void)
{
#ifdef CONFIG_SMP
	return get_current()->tgid;
#else
	return 0;
#endif
}

int FUNC206LXDEV131(void)
{
	return 0;
}

int FUNC206LXDEV133(void)
{
	return 0;
}

int FUNC206LXDEV130(GLJ_TASK taskId)
{
	return 0;
}

int FUNC206LXDEV115(GLJ_TASK taskId)
{
	return 0;
}



int FUNC206LXDEV128(int ticks)
{
	GLJ_TICK t;

	mwv206_timed_loop (t, 1, ticks) {
		;
	}

	return 0;
}

GLJ_TICK FUNC206LXDEV134(void)
{
	return jiffies;
}

GLJ_TICK FUNC206LXDEV098(void)
{
	return HZ;
}


int FUNC206LXDEV099(int clkrate)
{
	return -1;
}



GLJ_LOCK FUNC206LXDEV118(void)
{
	struct mutex *localMutex = NULL;

	localMutex = (struct mutex *)kzalloc(sizeof(struct mutex), GFP_KERNEL);

	mutex_init(localMutex);

	return (GLJ_LOCK)localMutex;
}

int FUNC206LXDEV119(GLJ_LOCK lockId)
{
	kfree((void *)lockId);
	return 0;
}

int FUNC206LXDEV116(GLJ_LOCK lockId)
{
	mutex_lock((struct mutex *)lockId);
	V206DEV005("%s: 0x%lx\n", __FUNCTION__, lockId);
	return 0;
}


int FUNC206LXDEV117(GLJ_LOCK lockId, int timeout)
{

V206KDEBUG002("[ERROR] %s is unimplemented.", __FUNCTION__);
return 0;
}

int FUNC206LXDEV135(GLJ_LOCK lockId)
{
	return mutex_trylock((struct mutex *)lockId);
}

int FUNC206LXDEV136(GLJ_LOCK lockId)
{
	mutex_unlock((struct mutex *)lockId);
	V206DEV005("%s: 0x%lx\n", __FUNCTION__, lockId);
	return 0;
}

int FUNC206LXDEV114(GLJ_LOCK lockId)
{
	int ret;
	ret = mutex_is_locked((struct mutex *)lockId);
	return ret;
}


GLJ_EVENT FUNC206LXDEV101(void)
{
	struct completion *comp;
	comp = (struct completion *)kmalloc(sizeof(struct completion), GFP_KERNEL);
	init_completion(comp);
	return (GLJ_EVENT)comp;
}

int FUNC206LXDEV104(GLJ_EVENT eventId)
{
	init_completion((struct completion *)eventId);
	return 0;
}

int FUNC206LXDEV102(GLJ_EVENT eventId)
{
	kfree((void *)eventId);
	return 0;
}

int FUNC206LXDEV105(GLJ_EVENT eventId, unsigned long timeout)
{
	int ret;

	ret = wait_for_completion_timeout((struct completion *)eventId, timeout);
	if (ret > 0) {
		return 0;
	} else if (ret == 0) {
		return -1;
	} else {
		V206KDEBUG002("wait_for_completion_timeout failed!.\n");
		return -2;
	}
}

int FUNC206LXDEV103(GLJ_EVENT eventId)
{
	complete((struct completion *)eventId);
	return 0;
}

#if 0

GLJ_MSGQ FUNC206LXDEV121(int maxMessages, int maxMsgSize)
{
	return (GLJ_MSGQ)msgQCreate(maxMessages, maxMsgSize, MSG_Q_PRIORITY);
}

int FUNC206LXDEV122(GLJ_MSGQ qid)
{
	return msgQDelete(qid);
}

int FUNC206LXDEV125(GLJ_MSGQ qid, void *message, int msgSize, int wait)
{
	return msgQSend(qid, message, msgSize, wait, MSG_PRI_NORMAL);
}

int FUNC206LXDEV123 (GLJ_MSGQ qid, void *message, int msgSize, int wait)
{
	return msgQReceive(qid, message, msgSize, wait);
}

int FUNC206LXDEV124(GLJ_MSGQ qid)
{
	return msgQNumMsgs(qid);
}

int FUNC206LXDEV112(int *save)
{
	*save = intLock();
	return *save;
}

int FUNC206LXDEV113(int *restore)
{
	intUnlock(*restore);
	return 0;
}
#endif
int FUNC206LXDEV108(int interruptId, GLJOSINTRFUNC fptr, void *param)
{

	V206KDEBUG002("[ERROR] %s\n", __FUNCTION__);
	return 0;
}

int FUNC206LXDEV110(int interruptId, GLJOSINTRFUNC fptr, void *param)
{

	V206KDEBUG002("[ERROR] %s\n", __FUNCTION__);
	return 0;
}

int FUNC206LXDEV111(int interruptId)
{
	V206KDEBUG002("[ERROR] %s\n", __FUNCTION__);
	return 0;

}

int FUNC206LXDEV109(int interruptId)
{
	V206KDEBUG002("[ERROR] %s\n", __FUNCTION__);
	return 0;

}



void *FUNC206LXDEV010(int size)
{
	return kmalloc(size, GFP_KERNEL);
}

int FUNC206LXDEV120(void *pMem)
{
	kfree(pMem);
	return 0;
}

void *FUNC206LXDEV009(struct device *dev,
		unsigned long long *dma_handle, int size)
{
	return dma_alloc_coherent(dev, size, (dma_addr_t *)dma_handle, GFP_KERNEL | GFP_DMA);
}

int FUNC206LXDEV100(struct device *dev,
		int size, void *vaddr, unsigned long long dma_handle)
{
	dma_free_coherent(dev, size, vaddr, dma_handle);
	return 0;
}

unsigned long long FUNC206LXDEV011(unsigned long long vaddr)
{
	return __pa(vaddr);
}


void FUNC206LXDEV012(void)
{
	mb();
}