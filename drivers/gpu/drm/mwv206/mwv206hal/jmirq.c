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
#include "jmirq.h"

#include "mwv206dev.h"

#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/err.h>

#include "mwv206kconfig.h"



#define MAC206HAL137(dev, reg, value) V206DEV006(dev, reg, value)
#define MAC206HAL079(dev, reg) V206DEV007(dev, reg)



#define MAC206HAL087                0x6000



#define MAC206HAL136              0x6004



#define MAC206HAL080              0x6008



#define MAC206HAL134              0x600C



#define MAC206HAL135               0x6010



#define MAC206HAL088            0x6014



#define MAC206HAL089            0x6018



#define MAC206HAL090            0x601C




#define MAC206HAL081           0x6020



#define MAC206HAL127       0x6024



#define MAC206HAL128       0x6028




#define MAC206HAL129       0x602C




#define MAC206HAL086      0x6030



#define MAC206HAL118         0x6034



#define MAC206HAL119         0x6038



#define MAC206HAL120         0x603C




#define MAC206HAL084        0x6040




#define MAC206HAL097          0x6044



#define MAC206HAL098          0x6048




#define MAC206HAL099          0x604C



#define MAC206HAL121        0x6050



#define MAC206HAL122        0x6054





#define MAC206HAL123        0x6058



#define MAC206HAL124        0x605C




#define MAC206HAL125        0x6060




#define MAC206HAL126        0x6064



#define MAC206HAL085       0x6068




#define MAC206HAL112         0x606C



#define MAC206HAL113         0x6070



#define MAC206HAL114     0x6074



#define MAC206HAL115         0x6078



#define MAC206HAL116         0x607C



#define MAC206HAL117         0x6080




#define MAC206HAL083        0x6084



#define MAC206HAL100        0x6088



#define MAC206HAL101        0x608C



#define MAC206HAL104        0x6090



#define MAC206HAL105        0x6094



#define MAC206HAL106        0x6098



#define MAC206HAL107        0x609C



#define MAC206HAL108        0x60A0



#define MAC206HAL109        0x60A4



#define MAC206HAL110        0x60A8



#define MAC206HAL111        0x60AC




#define MAC206HAL102       0x60B0



#define MAC206HAL103       0x60B4



#define MAC206HAL091      0x60B8



#define MAC206HAL092      0x60BC



#define MAC206HAL093      0x60C0



#define MAC206HAL094      0x60C4



#define MAC206HAL095      0x60C8



#define MAC206HAL096      0x60CC



#define MAC206HAL082     0x60D0



#define MAC206HAL130              0x60D4



#define MAC206HAL131              0x60D8



#define MAC206HAL132              0x60DC



#define MAC206HAL133              0x60E0



void FUNC206HAL132(void *devInfo)
{
	unsigned int intStatus[4];
	unsigned int i, j;
	int irqNum;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	intStatus[0] = MAC206HAL079(pDeviceData, 0x400000 + MAC206HAL118);
	intStatus[1] = MAC206HAL079(pDeviceData, 0x400000 + MAC206HAL119);
	intStatus[2] = MAC206HAL079(pDeviceData, 0x400000 + MAC206HAL120);
	intStatus[3] = MAC206HAL079(pDeviceData, 0x400000 + MAC206HAL084);

	MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL097, intStatus[0]);
	MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL098, intStatus[1]);
	MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL099, intStatus[2]);



	for (i = 0; i < 4; i++) {
		if (0 != intStatus[i]) {

			for (j = 0; j < 32; j++) {

				irqNum = (i << 5) + j;
				if (0 != (intStatus[i] & (1 << j))) {
					if (NULL != pDeviceData->V206DEV070[irqNum].V206DEV070) {
						pDeviceData->V206DEV070[irqNum].V206DEV070(irqNum, (void *)(pDeviceData->V206DEV070[irqNum].param));
					}
				}
			}
		}
	}
}



int FUNC206HAL137(void *devInfo, int interruptId, OSHAL_IRQFUNC ftpr, void *param)
{
	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	V206KDEBUG001(V206KDEBUG007, "[INFO] jmRegisterIrq (%d) ftpr = %p\n", interruptId, ftpr);

	pDeviceData->V206DEV070[interruptId].param = (unsigned long)param;
	pDeviceData->V206DEV070[interruptId].V206DEV070 = ftpr;
	pDeviceData->V206DEV070[interruptId].type = 1;

	return 0;
}



int FUNC206HAL120(void *devInfo, int irqNumber)
{
	unsigned int regAddr;
	unsigned int regValue;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (irqNumber < 0 || irqNumber > 95) {
		return -1;
	}

	regAddr = 0x400000 + MAC206HAL088 + ((irqNumber >> 5) << 2);
	regValue = MAC206HAL079(pDeviceData, regAddr);
	regValue |= (1 << (irqNumber & 0x1F));

	MAC206HAL137(pDeviceData, regAddr, regValue);

	return 0;
}



int FUNC206HAL118(void *devInfo, int irqNumber)
{
	unsigned int regAddr;
	unsigned int regValue;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (irqNumber < 0 || irqNumber > 95) {
		return -1;
	}

	regAddr = 0x400000 + MAC206HAL088 + ((irqNumber >> 5) << 2);
	regValue = MAC206HAL079(pDeviceData, regAddr);
	regValue &= ~(1 << (irqNumber & 0x1F));

	MAC206HAL137(pDeviceData, regAddr, regValue);

	return 0;
}



int FUNC206HAL119(void *devInfo, int irqNumber)
{
	unsigned int regValue;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (irqNumber < 0 || irqNumber > 15) {
		return -1;
	}

	regValue = MAC206HAL079(pDeviceData, 0x400000 + MAC206HAL081);
	regValue |= (1 << irqNumber);

	MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL081, regValue);

	return 0;
}



int FUNC206HAL117(void *devInfo, int irqNumber)
{
	unsigned int regValue;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (irqNumber < 0 || irqNumber > 15) {
		return -1;
	}

	regValue = MAC206HAL079(pDeviceData, 0x400000 + MAC206HAL081);
	regValue &= ~(1 << irqNumber);

	MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL081, regValue);

	return 0;
}



int FUNC206HAL122(void *devInfo, int irqNumber, int *pStatus)
{
	unsigned int regAddr;
	unsigned int regValue;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (irqNumber < 0 || irqNumber > 95 || NULL == pStatus) {
		return -1;
	}

	regAddr = 0x400000 + MAC206HAL127 + ((irqNumber >> 5) << 2);
	regValue = MAC206HAL079(pDeviceData, regAddr);

	*pStatus = (regValue >> (irqNumber & 0x1F)) & 0x1;

	return 0;
}




int FUNC206HAL121(void *devInfo, int irqNumber, int *pStatus)
{
	unsigned int regAddr;
	unsigned int regValue;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (irqNumber < 0 || irqNumber > 15 || NULL == pStatus) {
		return -1;
	}

	regAddr = 0x400000 + MAC206HAL086 + ((irqNumber >> 5) << 2);
	regValue = MAC206HAL079(pDeviceData, regAddr);

	*pStatus = (regValue >> irqNumber) & 0x1;

	return 0;
}




int FUNC206HAL115(void *devInfo, int irqNumber, int *pIntIsValid)
{
	unsigned int regAddr;
	unsigned int regValue;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (irqNumber < 0 || irqNumber > 95 || NULL == pIntIsValid) {
		return -1;
	}

	regAddr = 0x400000 + MAC206HAL118 + ((irqNumber >> 5) << 2);
	regValue = MAC206HAL079(pDeviceData, regAddr);

	*pIntIsValid = (regValue >> (irqNumber & 0x1F)) & 0x1;

	return 0;
}




int FUNC206HAL114(void *devInfo, int irqNumber, int *pIntIsValid)
{
	unsigned int regValue;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (irqNumber < 0 || irqNumber > 15 || NULL == pIntIsValid) {
		return -1;
	}

	regValue = MAC206HAL079(pDeviceData, 0x400000 + MAC206HAL084);

	*pIntIsValid = (regValue >> irqNumber) & 0x1;

	return 0;
}



int FUNC206HAL131(void *devInfo, int pIntPendID, int *pPendValid)
{
	unsigned int regValue;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (pIntPendID < 0 || pIntPendID > 2 || NULL == pPendValid) {
		return -1;
	}

	regValue = MAC206HAL079(pDeviceData, 0x400000 + MAC206HAL118 + (pIntPendID << 2));

	*pPendValid = regValue;

	return 0;
}



int FUNC206HAL116(void *devInfo, int irqNumber)
{
	unsigned int regAddr;
	unsigned int regValue;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (irqNumber < 0 || irqNumber > 95) {
		return -1;
	}

	regAddr = 0x400000 + MAC206HAL097 + ((irqNumber >> 5) << 2);
	regValue = 1 << (irqNumber & 0x1F);
	MAC206HAL137(pDeviceData, regAddr, regValue);

	return 0;
}



int FUNC206HAL133(void *devInfo, int clearRegIndex, unsigned int clearValue)
{
	unsigned int regAddr;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (clearRegIndex < 0 || clearRegIndex > 2) {
		return -1;
	}

	regAddr = 0x400000 + MAC206HAL097 + (clearRegIndex << 2);
	MAC206HAL137(pDeviceData, regAddr, clearValue);

	return 0;
}





int FUNC206HAL140(void *devInfo, int irqNumber, int target)
{
	unsigned int regAddr;
	unsigned int regValue;
	unsigned int bitOffset;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (irqNumber < 0 || irqNumber > 95) {
		return -1;
	}

	regAddr = 0x400000 + MAC206HAL121 + ((irqNumber >> 4) << 2);
	regValue = MAC206HAL079(pDeviceData, regAddr);

	bitOffset = (irqNumber & 0xF) << 1;
	regValue &= ~(3 << bitOffset);
	regValue |= (target << bitOffset);

	MAC206HAL137(pDeviceData, regAddr, regValue);

	return 0;
}




int FUNC206HAL139(void *devInfo, int irqNumber, int target)
{
	unsigned int regValue;
	unsigned int bitOffset;

	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	if (irqNumber < 0 || irqNumber > 15) {
		return -1;
	}

	regValue = MAC206HAL079(pDeviceData, 0x400000 + MAC206HAL085);

	bitOffset = irqNumber << 1;
	regValue &= ~(3 << bitOffset);
	regValue |= (target << bitOffset);

	MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL085, regValue);

	return 0;
}


int FUNC206HAL138(void *devInfo, int target)
{
	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;

	MAC206HAL137(pDeviceData, 0xC200A8, 0x0);

	switch (target) {
	case 0:
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL121, 0);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL122, 0);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL123, 0);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL124, 0);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL125, 0);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL126, 0);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL085, 0);
		break;

	case 1:
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL121, 0x55555555);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL122, 0x55555555);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL123, 0x55555555);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL124, 0x55555555);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL125, 0x55555555);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL126, 0x55555555);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL085, 0x55555555);



		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL091, 0xFFFFFFFF);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL092, 0xFFFFFFFF);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL093, 0xFFFFFFFF);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL094, 0xFFFFFFFF);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL095, 0xFFFFFFFF);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL096, 0xFFFFFFFF);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL082, 0xFFFFFFFF);



		break;

	case 2:
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL121, 0xAAAAAAAA);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL122, 0xAAAAAAAA);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL123, 0xAAAAAAAA);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL124, 0xAAAAAAAA);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL125, 0xAAAAAAAA);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL126, 0xAAAAAAAA);
		MAC206HAL137(pDeviceData, 0x400000 + MAC206HAL085, 0xAAAAAAAA);
		break;

	default:
		break;
	}

	return 0;
}