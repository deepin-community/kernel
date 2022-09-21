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
#ifndef _JMIRQ_H_
#define _JMIRQ_H_

#include "mwv206dev.h"
#include "oshal.h"

#define SD_INT_NUMBER       0
#define SPI_INT0_NUMBER     19
#define SPI_INT1_NUMBER     20
#define PS2_INT_NUMBER      21
#define I2C_INT_NUMBER      22


#ifdef __cplusplus
extern "C" {
#endif


void FUNC206HAL132(void *devInfo);



int FUNC206HAL137(void *devInfo, int interruptId, OSHAL_IRQFUNC ftpr, void *param);



int FUNC206HAL120(void *devInfo, int irqNumber);



int FUNC206HAL118(void *devInfo, int irqNumber);




int FUNC206HAL119(void *devInfo, int irqNumber);




int FUNC206HAL117(void *devInfo, int irqNumber);




int FUNC206HAL122(void *devInfo, int irqNumber, int *pStatus);




int FUNC206HAL121(void *devInfo, int irqNumber, int *pStatus);




int FUNC206HAL115(void *devInfo, int irqNumber, int *pIntIsValid);




int FUNC206HAL114(void *devInfo, int irqNumber, int *pIntIsValid);




int FUNC206HAL116(void *devInfo, int irqNumber);




int FUNC206HAL140(void *devInfo, int irqNumber, int target);



int FUNC206HAL139(void *devInfo, int irqNumber, int target);



int FUNC206HAL138(void *devInfo, int target);




int FUNC206HAL131(void *devInfo, int pIntPendID, int *pPendValid);




int FUNC206HAL133(void *devInfo, int clearRegIndex, unsigned int clearValue);


#ifdef __cplusplus
}
#endif

#endif