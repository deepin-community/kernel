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
#ifndef _MWV206MEMMGR_H_
#define _MWV206MEMMGR_H_

#include "gljos.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *MemMgr;
typedef void *SaveMemMgr;

typedef struct memmgr_s {
	MemMgr (*FUNC206HAL204)(int maxblockcount);
	void (*FUNC206HAL199)(MemMgr mgr);
	int (*FUNC206HAL197)(MemMgr mgr, unsigned int addr, int size);
	unsigned int (*FUNC206HAL206)(MemMgr mgr, int size, int alignsize, void *userdata);
	int (*FUNC206HAL201)(MemMgr mgr, unsigned int addr, void *userdata);
	unsigned int (*FUNC206HAL213)(MemMgr mgr);
	unsigned int (*FUNC206HAL207)(MemMgr mgr, unsigned int *addr);
	void (*FUNC206HAL212)(MemMgr mgr);
	void (*FUNC206HAL202)(MemMgr mgr, void *userdata);
	GLJ_LOCK (*FUNC206HAL203)(MemMgr mgr);
	int (*FUNC206HAL211)(MemMgr mgr, unsigned int addr, void *userdata);
} memmgr_t;

extern const memmgr_t FUNC206HAL418;

int FUNC206HAL420(void *V206DEV103);
int FUNC206HAL419(void *V206DEV103);

#ifdef __cplusplus
}
#endif

#endif