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
#ifndef _GLJOS_H_
#define _GLJOS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long GLJ_TASK;
typedef unsigned long GLJ_LOCK;
typedef unsigned long GLJ_EVENT;
typedef unsigned long GLJ_MSGQ;
typedef unsigned long GLJ_DEVICEDRV;
#ifdef __linux__
typedef unsigned long GLJ_TICK;
#else
typedef unsigned int GLJ_TICK;
#endif

typedef int (*GLJOSTASKFUNC)(unsigned long p0, unsigned long p1);

#define GLJ_OK   0
#define GLJ_ERROR -1
#define TASK_USE_FLOAT   1


GLJ_TASK  FUNC206LXDEV127(char *name, int priority, int options, int stackSize, GLJOSTASKFUNC entry,
		unsigned long param0, unsigned long param1);


int FUNC206LXDEV129 (GLJ_TASK *ptaskId);


int FUNC206LXDEV126 (GLJ_TASK taskId);


int FUNC206LXDEV132 (GLJ_TASK taskId);


GLJ_TASK FUNC206LXDEV107 (void);


GLJ_TASK FUNC206LXDEV106(void);


int FUNC206LXDEV131 (void);


int FUNC206LXDEV133 (void);


int FUNC206LXDEV130(GLJ_TASK taskId);


int FUNC206LXDEV115(GLJ_TASK taskId);


int FUNC206LXDEV128(int ticks);


GLJ_TICK FUNC206LXDEV134(void);


GLJ_TICK FUNC206LXDEV098(void);


int FUNC206LXDEV099(int clkrate);


GLJ_LOCK FUNC206LXDEV118(void);


int FUNC206LXDEV119(GLJ_LOCK lockId);


int FUNC206LXDEV116(GLJ_LOCK lockId);


int FUNC206LXDEV117(GLJ_LOCK lockId, int timeout);


int FUNC206LXDEV135(GLJ_LOCK lockId);


int FUNC206LXDEV136(GLJ_LOCK lockId);


int FUNC206LXDEV114(GLJ_LOCK lockId);


GLJ_TASK gljosLockGetOwnerTGID(GLJ_LOCK lockId);


GLJ_TASK gljosLockGetOwnerPID(GLJ_LOCK lockId);


void gljosReleaseLock(GLJ_LOCK lockId);


GLJ_EVENT FUNC206LXDEV101(void);


int FUNC206LXDEV102(GLJ_EVENT eventId);


int FUNC206LXDEV105(GLJ_EVENT eventId, unsigned long timeout);


int FUNC206LXDEV103(GLJ_EVENT eventId);


int FUNC206LXDEV104(GLJ_EVENT eventId);


GLJ_MSGQ FUNC206LXDEV121(int maxMessages, int maxMsgSize);


int FUNC206LXDEV122 (GLJ_MSGQ qid);


int FUNC206LXDEV125(GLJ_MSGQ qid, void *message, int msgSize, int wait);


int FUNC206LXDEV123 (GLJ_MSGQ qid, void *message, int msgSize, int wait);


int FUNC206LXDEV124(GLJ_MSGQ qid);


typedef int (*GLJOSINTRFUNC)(int param1, void *param);



int FUNC206LXDEV112(int *save);


int FUNC206LXDEV113(int *restore);


int FUNC206LXDEV108(int interruptId, GLJOSINTRFUNC fptr, void *param);


int FUNC206LXDEV110(int interruptId, GLJOSINTRFUNC fptr, void *param);


int FUNC206LXDEV111(int interruptId);


int FUNC206LXDEV109(int interruptId);


void *FUNC206LXDEV010(int size);


int FUNC206LXDEV120(void *pMem);



void FUNC206LXDEV012(void);

#ifdef __cplusplus
}
#endif

#endif