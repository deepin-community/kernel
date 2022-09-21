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
#ifndef _JMIIC_H_
#define _JMIIC_H_


typedef void *JMIIC;

#ifdef __cplusplus
extern "C" {
#endif



int FUNC206HAL158(JMIIC iicHandle);



int FUNC206HAL159(JMIIC iicHandle,
		int slaveaddr,
		int V206DEV033,
		int nbytes,
		char *pvalue,
		int isSetStartBit,
		int isSetStopBit);



int FUNC206HAL157(JMIIC iicHandle,
		int slaveaddr,
		int V206DEV033,
		int nbytes,
		char *pvalue,
		int isSetStartBit,
		int isSetStopBit);



int FUNC206HAL160(JMIIC iicHandle, int clkDiv);



int FUNC206HAL161(JMIIC iicHandle,  unsigned int commuFreq);


int FUNC206HAL153(JMIIC iicHandle, unsigned int *commuFreq);


int FUNC206HAL152(JMIIC iicHandle, int *clkDiv);



int FUNC206HAL156(void);



int FUNC206HAL151(void *pDeviceInfo, JMIIC *pI2cHandle);


#ifdef __cplusplus
}
#endif

#endif