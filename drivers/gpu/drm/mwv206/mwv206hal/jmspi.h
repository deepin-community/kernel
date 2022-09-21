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
#ifndef _JMSPIDRIVER_H_
#define _JMSPIDRIVER_H_

typedef void *JMSPI;

#ifdef __cplusplus
extern "C" {
#endif



int FUNC206HAL170(void);




int FUNC206HAL166(void *pDevInfo, int index, JMSPI *pSpiDev);



int FUNC206HAL172(JMSPI pSpiDev,
		int slave,
		const char *cmdContent,
		unsigned int cmdLength,
		const char *pTxBuffer,
		unsigned int *pSendLen,
		char *pRxBuffer,
		unsigned int *pRecvLen,
		unsigned int startRcv,
		unsigned int transBytes,
		int sclkPhase,
		int sclkPolarity,
		int lsbFirst,
		int recvSampMode);



int FUNC206HAL141(JMSPI pSpiDev,
		int slave,
		int sclkPhase,
		int sclkPolarity,
		int lsbFirst,
		int recvSampMode);




int FUNC206HAL173(JMSPI pSpiDev,
		const char *cmdContent,
		unsigned int cmdLength,
		const char *pTxBuffer,
		unsigned int *pSendLen,
		char *pRxBuffer,
		unsigned int *pRecvLen,
		unsigned int startRcv,
		unsigned int transBytes);



int FUNC206HAL171(JMSPI pSpiDev,
		unsigned int commuFreq);



int FUNC206HAL168(JMSPI pSpiDev,
		int *pRefClockFreq,
		unsigned int *pCommuFreq);


#ifdef __cplusplus
}
#endif

#endif