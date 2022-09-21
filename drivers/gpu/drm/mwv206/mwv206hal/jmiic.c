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
#include <linux/delay.h>
#include <linux/interrupt.h>

#include "oshal.h"
#include "special_list.h"

#include "jmiic.h"
#include "jmirq.h"

#include "gljos.h"
#include "mwv206reg.h"
#include "mwv206kconfig.h"

#define MAC206HAL042       10

#define MAC206HAL143       (240)

#define MAC206HAL078     1


#define MAC206HAL059          (0x0)
#define MAC206HAL060                (0x4)
#define MAC206HAL061        (0x8)
#define MAC206HAL062          (0xc)
#define MAC206HAL063          (0x10)
#define MAC206HAL064          (0x14)
#define MAC206HAL065           (0x18)
#define MAC206HAL066          (0x1c)
#define MAC206HAL067       (0x20)
#define MAC206HAL068        (0x24)
#define MAC206HAL069               (0x28)

#define MAC206HAL036                  1
#define MAC206HAL037                0

#define MAC206HAL038          0

#define MAC206HAL073                0

#define MAC206HAL040(_reg, _bit) (((1<<(_bit))&(_reg)) ? 1 : 0)
#define MAC206HAL010(_reg, _bit) ((_reg) &= (~(1ul<<(_bit))))
#define MAC206HAL178(_reg, _bit) ((_reg) |= (1ul<<(_bit)))

#define MAC206HAL077(dev, reg, value) V206DEV006(dev, 0x400000 + reg, value)
#define MAC206HAL076(dev, reg) V206DEV007(dev, 0x400000 + reg)

#define MAC206HAL041      0


typedef struct FUNC206HAL029 {
	void           *mdevInfo;
	unsigned int    mClkDIV;
	unsigned int    mRefClk;

	unsigned int    mCommClk;
	unsigned int    mCMD;
	OSHAL_LOCK      mLockSem;
	unsigned int    mIntEnable;
#if (1 == MAC206HAL041)
	OSHAL_EVENT     mIRQSem;
#endif
	unsigned int    mIRQ;
	unsigned int    mIntRegAddr;
	unsigned int    mArbitrateReg;
} JM_IIC_INFO, *P_JM_IIC_INFO;

typedef struct FUNC206HAL028 {
	SPECIAL_SINGLE_LIST_ENTRY   mLink;
	JM_IIC_INFO                 mModuleInfo;
} JM_IIC_DEV_LISTNODE, *P_JM_IIC_DEV_LISTNODE;

#define MAC206HAL074      0x49324344


#define MAC206HAL075       "jmiic-1.0.0-20160111-0900"



static unsigned int FUNC206HAL059;

static OSHAL_LOCK FUNC206HAL196;


static SPECIAL_SINGLE_LIST_ENTRY FUNC206HAL060 = {NULL};


static int FUNC206HAL155(P_JM_IIC_INFO pJmIICInfo, int index)
{
	return 0;
}



#if (1 == MAC206HAL041)

static int FUNC206HAL123(unsigned int intStauts, void *param)
{
	P_JM_IIC_INFO pJmIICInfo = (P_JM_IIC_INFO)param;

	V206DEV005("[IRQ] at %d of %s\n", __LINE__, __FUNCTION__);

	FUNC206HAL428(pJmIICInfo->mIRQSem);
	return IRQ_HANDLED;
}
#endif



int FUNC206HAL156(void)
{
	if (1 == FUNC206HAL059) {
		return 0;
	}

	FUNC206HAL196 = FUNC206LXDEV118();
	FUNC206HAL060.Next = NULL;

	FUNC206HAL059 = 1;

	return 0;
}




int FUNC206HAL151(void *devInfo, JMIIC *pI2cHandle)
{
	P_JM_IIC_DEV_LISTNODE pJmIICDevListNode = NULL;
	P_SPECIAL_SINGLE_LIST_ENTRY pListEntry = NULL;
	V206DEV025 *pDev = (V206DEV025 *)devInfo;

#if (1 == MAC206HAL041)
	int result;
#endif

	if (0 == FUNC206HAL059) {
		return -2;
	}

	FUNC206LXDEV116(FUNC206HAL196);

	if (NULL != FUNC206HAL060.Next) {

		pListEntry = FUNC206HAL060.Next;
		while (NULL != pListEntry) {
			pJmIICDevListNode = (P_JM_IIC_DEV_LISTNODE)pListEntry;
			if (devInfo == pJmIICDevListNode->mModuleInfo.mdevInfo) {

				*pI2cHandle = (JMIIC)&pJmIICDevListNode->mModuleInfo;
				FUNC206LXDEV136(FUNC206HAL196);
				return 0;
			}

			pListEntry = pListEntry->Next;
		}
	}

	pJmIICDevListNode = (P_JM_IIC_DEV_LISTNODE)FUNC206HAL442(sizeof(JM_IIC_DEV_LISTNODE), 4);
	if (NULL == pJmIICDevListNode) {
		FUNC206LXDEV136(FUNC206HAL196);
		return -4;
	}

	pJmIICDevListNode->mModuleInfo.mClkDIV = 0;
	pJmIICDevListNode->mModuleInfo.mIntEnable = 0;
	pJmIICDevListNode->mModuleInfo.mdevInfo = devInfo;
	pJmIICDevListNode->mModuleInfo.mIRQ = I2C_INT_NUMBER;
	pJmIICDevListNode->mModuleInfo.mIntRegAddr = (0x5500) + MAC206HAL064;
	pJmIICDevListNode->mModuleInfo.mArbitrateReg = (0x5500) + MAC206HAL068;
	pJmIICDevListNode->mModuleInfo.mRefClk = pDev->V206DEV035 / 2;
	V206DEV005("[INFO]:I2C ref clk %dMHz(AXI_CLK/2)\n", pJmIICDevListNode->mModuleInfo.mRefClk);
	pJmIICDevListNode->mModuleInfo.mLockSem = FUNC206HAL440();
	if (0 == pJmIICDevListNode->mModuleInfo.mLockSem) {
		FUNC206LXDEV136(FUNC206HAL196);

		FUNC206HAL443(pJmIICDevListNode);
		pJmIICDevListNode = NULL;

		return -2;
	}

#if 0

	MAC206HAL077(pJmIICDevListNode->mModuleInfo.mIntRegAddr, MAC206HAL036);


	MAC206HAL077(pJmIICDevListNode->mModuleInfo.mArbitrateReg, 0);
#endif


#if (1 == MAC206HAL041)
	pJmIICDevListNode->mModuleInfo.mIRQSem = FUNC206HAL426(EMPTY);
	if (0 == pJmIICDevListNode->mModuleInfo.mIRQSem) {
		FUNC206HAL441(pJmIICDevListNode->mModuleInfo.mLockSem);
		FUNC206LXDEV136(FUNC206HAL196);

		FUNC206HAL443(pJmIICDevListNode);
		pJmIICDevListNode = NULL;

		return -3;
	}

	result = FUNC206HAL431(pJmIICDevListNode->mModuleInfo.mdevInfo,
			pJmIICDevListNode->mModuleInfo.mIRQ,
			FUNC206HAL123,
			"jmIIC",
			(void *)&pJmIICDevListNode->mModuleInfo);
	if (0 == result) {
		pJmIICDevListNode->mModuleInfo.mIntEnable = 1;
	}
#endif

	PushEntryList(&FUNC206HAL060, &pJmIICDevListNode->mLink);
	*pI2cHandle = (JMIIC)&pJmIICDevListNode->mModuleInfo;

	FUNC206LXDEV136(FUNC206HAL196);

	return 0;
}



int FUNC206HAL158(JMIIC iicHandle)
{
	P_JM_IIC_INFO pJmIICInfo = (P_JM_IIC_INFO)iicHandle;


	if (NULL == iicHandle) {
		return -1;
	}

	FUNC206HAL155(pJmIICInfo, 1);


	FUNC206LXDEV116(pJmIICInfo->mLockSem);

	MAC206HAL077(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL059, 0);
	FUNC206LXDEV128(1);

	MAC206HAL077(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL059, 1);
	FUNC206LXDEV128(1);

	FUNC206LXDEV136(pJmIICInfo->mLockSem);

	return 0;
}


int FUNC206HAL160(JMIIC iicHandle, int clkDiv)
{
	P_JM_IIC_INFO pJmIICInfo = (P_JM_IIC_INFO)iicHandle;

	if (NULL == iicHandle) {
		V206KDEBUG002("[ERROR]:i2c handle is invalid!\n");
		return -1;
	}

	FUNC206HAL155(pJmIICInfo, 1);


	FUNC206LXDEV116(pJmIICInfo->mLockSem);

	MAC206HAL077(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL063, clkDiv * 4);

	FUNC206LXDEV136(pJmIICInfo->mLockSem);

	return 0;
}



int FUNC206HAL152(JMIIC iicHandle, int *clkDiv)
{
	P_JM_IIC_INFO pJmIICInfo = (P_JM_IIC_INFO)iicHandle;
	unsigned int value;
	int ret;

	if (NULL == iicHandle) {
		return -1;
	}


	FUNC206LXDEV116(pJmIICInfo->mLockSem);

	FUNC206HAL155(pJmIICInfo, 1);

	value = MAC206HAL076(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL063);
	value = value >> 2;
	ret = copy_to_user(clkDiv, &value, sizeof(int));

	FUNC206LXDEV136(pJmIICInfo->mLockSem);

	return ret;
}



int FUNC206HAL153(JMIIC iicHandle, unsigned int *commuFreq)
{
	P_JM_IIC_INFO piic = (P_JM_IIC_INFO)iicHandle;

	if (iicHandle == NULL) {
		V206KDEBUG002("[ERROR]:i2c handle is NULL\n");
		return -1;
	}
	V206DEV005("*commuFreq %d\n", *commuFreq);

	return copy_to_user(commuFreq, &piic->mCommClk, sizeof(unsigned int));
}



int FUNC206HAL161(JMIIC iicHandle,  unsigned int commuFreq)
{
	P_JM_IIC_INFO piic = (P_JM_IIC_INFO)iicHandle;
	unsigned int freqDiv;
	int refClockFreq;

	if (piic->mRefClk == 0) {
		V206KDEBUG002("[ERROR]:refClockFreq 0MHz\n");
		return -1;
	}

	if (0 == commuFreq) {
		V206KDEBUG002("[ERROR]:commuFreq %dKHz\n", commuFreq);
		return -2;
	}

	refClockFreq = piic->mRefClk;
	freqDiv = FUNC206HAL472(piic->mRefClk * 1000, commuFreq);


	freqDiv /= 4;

	V206DEV005("[INFO]:refClock=%dMHz, commuFreq=%dKHz\n", refClockFreq, commuFreq);
	V206DEV005("[INFO]:set clock div reg %d(0x%08X)\n", freqDiv, freqDiv);



	FUNC206LXDEV116(piic->mLockSem);


	MAC206HAL077(piic->mdevInfo, (0x5500) + MAC206HAL063, freqDiv);

	piic->mCommClk = commuFreq;

	FUNC206LXDEV136(piic->mLockSem);

	return 0;
}



static unsigned long FUNC206HAL154(P_JM_IIC_INFO pJmIICInfo)
{
	unsigned int errorCode = 0;
	volatile unsigned long value;

	value = MAC206HAL076(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL065);
	if (0xFFFFFFFF != value) {
		errorCode = value;
	} else {
		return value;
	}

	value = MAC206HAL076(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL068);
	if (1 == value) {
		errorCode |= 1 << 1;
	}

	if (0xFFFFFFFF == value) {
		return value;
	}

	return errorCode;
}



static int FUNC206HAL150(P_JM_IIC_INFO pJmIICInfo, unsigned int errorStatus)
{
	if (1 == (1 & errorStatus)) {
		MAC206HAL077(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL065, 0);
	}

	if (2 == (2 & errorStatus)) {
		MAC206HAL077(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL068, 0);
	}

	return 0;
}



int FUNC206HAL159(JMIIC iicHandle,
		int slaveaddr,
		int V206DEV033,
		int nbytes,
		char *pvalue,
		int isSetStartBit,
		int isSetStopBit)
{
	volatile int tick;
	int wrcount;
	volatile unsigned int value;
	unsigned int errorCode;
	P_JM_IIC_INFO pJmIICInfo = (P_JM_IIC_INFO)iicHandle;

	if (NULL == pJmIICInfo) {
		V206KDEBUG002("[ERROR]:i2c handle is invalid!\n");
		return -1;
	}

	FUNC206HAL155(pJmIICInfo, 1);


	FUNC206LXDEV116(pJmIICInfo->mLockSem);


	FUNC206HAL150(pJmIICInfo, 3);


	tick = 0;
	while (1) {
		value = MAC206HAL076(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL069);
		if (0 != value) {
			if (tick > MAC206HAL143) {
				V206KDEBUG002("[ERROR]:i2c busy!\n");
				FUNC206LXDEV136(pJmIICInfo->mLockSem);
				return -10;
			}

			FUNC206LXDEV128(MAC206HAL078);
		} else {
			break;
		}

		tick += 1;
	}



	value = MAC206HAL076(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL067);
	wrcount = 0;
	while ((value > 0) && (0xFFFFFFFF != value)) {
		if (wrcount == 0) {
			MAC206HAL077(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL066, V206DEV033);
		} else {
			MAC206HAL077(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL066, pvalue[wrcount - 1]);
		}

		wrcount += 1;
		if (wrcount == (nbytes + 1)) {
			break;
		}

		value = MAC206HAL076(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL067);
	}

	if (wrcount != (nbytes + 1)) {
		FUNC206LXDEV136(pJmIICInfo->mLockSem);

		V206KDEBUG002("[ERROR]:write data not enough!\n");
		if (value == 0) {
			V206KDEBUG002("[ERROR]:left SpaceCount == 0,maybe data has not send out succeessfully!\n");
		}

		return -11;
	}

#if (1 == MAC206HAL041)
	if (1 == pJmIICInfo->mIntEnable) {
		FUNC206HAL120(pJmIICInfo->mdevInfo, pJmIICInfo->mIRQ);

		FUNC206LXDEV111(pJmIICInfo->mIRQ);
	}
#endif


	pJmIICInfo->mCMD = 0;

	if (1 == isSetStartBit) {
		MAC206HAL178(pJmIICInfo->mCMD, 25);
	}


	if (1 == isSetStopBit) {
		MAC206HAL178(pJmIICInfo->mCMD, 24);
	}

	pJmIICInfo->mCMD |= ((nbytes + 1) & 0x1FFF);
	pJmIICInfo->mCMD |= ((slaveaddr & 0xFE) << 16);
	MAC206HAL077(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL060, pJmIICInfo->mCMD);


#if (1 == MAC206HAL041)

	if (1 == pJmIICInfo->mIntEnable) {
		V206DEV005("[INFO] wait for iic trans finished.\n");

		if (0 != FUNC206HAL429(pJmIICInfo->mIRQSem, MAC206HAL143)) {
#if (1 == MAC206HAL073)
			FUNC206HAL122(pJmIICInfo->mdevInfo, pJmIICInfo->mIRQ, (int *)&value);
			V206KDEBUG002("[ERROR]: wait for communication finish timeout!(status=%#x)\n", value);
#endif
			FUNC206HAL118(pJmIICInfo->mdevInfo, pJmIICInfo->mIRQ);

			FUNC206LXDEV109(pJmIICInfo->mIRQ);
			FUNC206LXDEV136(pJmIICInfo->mLockSem);
			return -10;
		}
	} else
#endif
	{
		tick = 0;
		while (1) {

			value = MAC206HAL076(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL064);
			if (MAC206HAL036 == value) {

				MAC206HAL077(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL064, MAC206HAL036);
				break;
			}

			if (tick > MAC206HAL143) {
				V206KDEBUG002("[ERROR]: wait for communication finish timeout!\n");
				FUNC206LXDEV136(pJmIICInfo->mLockSem);
				return -10;
			}
			FUNC206LXDEV128(MAC206HAL078);
			tick += 1;
		}
	}


#if (1 == MAC206HAL041)
	if (1 == pJmIICInfo->mIntEnable) {
#if (1 == MAC206HAL073)
		FUNC206HAL122(pJmIICInfo->mdevInfo, pJmIICInfo->mIRQ, (int *)&value);
		V206DEV005("[INFO]:iic trans finished, intStauts=%d\n", value);
#endif

		FUNC206HAL118(pJmIICInfo->mdevInfo, pJmIICInfo->mIRQ);

		FUNC206HAL432(pJmIICInfo->mIRQ);
	}
#endif


	errorCode = FUNC206HAL154(pJmIICInfo);
	if ((0 != errorCode) || (0xFFFFFFFF == value)) {
		if (0xFFFFFFFF != value) {
			FUNC206HAL150(pJmIICInfo, errorCode);
#if (1 == MAC206HAL073)
			V206KDEBUG002("[ERROR]:IIC error(0x%X) at %d of %s\n", errorCode, __LINE__, __FUNCTION__);
#endif
		}

		FUNC206LXDEV136(pJmIICInfo->mLockSem);


		return -10;
	}

	if (0 < wrcount) {
		wrcount -= 1;
	}

	FUNC206LXDEV136(pJmIICInfo->mLockSem);

	return wrcount;
}



int FUNC206HAL157(JMIIC iicHandle,
		int slaveaddr,
		int V206DEV033,
		int nbytes,
		char *pvalue,
		int isSetStartBit,
		int isSetStopBit)
{
	int tick;
	int recount, steprecount, fifocount;
	volatile unsigned int value;
	unsigned int errorCode;
	P_JM_IIC_INFO pJmIICInfo = (P_JM_IIC_INFO)iicHandle;
	if (NULL == pJmIICInfo) {
		return -1;
	}

	FUNC206HAL155(pJmIICInfo, 1);


	FUNC206LXDEV116(pJmIICInfo->mLockSem);


	FUNC206HAL150(pJmIICInfo, 3);


	tick = 0;
	while (1) {
		value = MAC206HAL076(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL069);
		if (0 != value) {
			if (tick > MAC206HAL143) {
				FUNC206LXDEV136(pJmIICInfo->mLockSem);
				return -10;
			}

			FUNC206LXDEV128(MAC206HAL078);
		} else {
			break;
		}

		tick += 1;
	}

#if (1 == MAC206HAL041)
	if (1 == pJmIICInfo->mIntEnable) {
		FUNC206HAL120(pJmIICInfo->mdevInfo, pJmIICInfo->mIRQ);

		FUNC206HAL434(pJmIICInfo->mIRQ);
	}
#endif


	pJmIICInfo->mCMD = 0;

	if (1 == isSetStartBit) {
		MAC206HAL178(pJmIICInfo->mCMD, 25);
	}


	if (1 == isSetStopBit) {
		MAC206HAL178(pJmIICInfo->mCMD, 24);
	}

	pJmIICInfo->mCMD |= (nbytes & 0x1FFF);
	pJmIICInfo->mCMD |= (((slaveaddr | 0x01) & 0xFF) << 16);
	MAC206HAL077(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL060, pJmIICInfo->mCMD);

#if (1 == MAC206HAL041)

	if (1 == pJmIICInfo->mIntEnable) {

		if (0 != FUNC206HAL429(pJmIICInfo->mIRQSem, MAC206HAL143)) {
			FUNC206HAL118(pJmIICInfo->mdevInfo, pJmIICInfo->mIRQ);

			FUNC206LXDEV109(pJmIICInfo->mIRQ);
			FUNC206LXDEV136(pJmIICInfo->mLockSem);
			return -10;
		}
	} else
#endif
	{

		tick = 0;
		while (1) {

			value = MAC206HAL076(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL064);
			if (MAC206HAL036 == value) {

				MAC206HAL077(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL064, MAC206HAL036);
				break;
			}

			if (tick > MAC206HAL143) {
				FUNC206LXDEV136(pJmIICInfo->mLockSem);

				return -10;
			}

			FUNC206LXDEV128(MAC206HAL078);
			tick += 1;
		}
	}


	recount = 0;
	value = MAC206HAL076(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL061);
	while ((0xFFFFFFFF != value) && (0 < (value & 0xFFF))) {
		steprecount = 0;
		fifocount = value & 0xFFF;

		while (steprecount < fifocount) {
			value = MAC206HAL076(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL062);
			if (value != 0xFFFFFFFF) {
				pvalue[recount] = (unsigned char)(value & 0xFF);
				recount += 1;
				steprecount += 1;

				if (recount == nbytes) {
					goto FUNC206HAL013;
				}
			} else {
#if (1 == MAC206HAL041)
				if (1 == pJmIICInfo->mIntEnable) {
					FUNC206HAL118(pJmIICInfo->mdevInfo, pJmIICInfo->mIRQ);

					FUNC206LXDEV109(pJmIICInfo->mIRQ);
				}
#endif

				FUNC206LXDEV136(pJmIICInfo->mLockSem);

				return -10;
			}
		}

		value = MAC206HAL076(pJmIICInfo->mdevInfo, (0x5500) + MAC206HAL061);
	}


FUNC206HAL013:

#if (1 == MAC206HAL041)
	if (1 == pJmIICInfo->mIntEnable) {
		FUNC206HAL118(pJmIICInfo->mdevInfo, pJmIICInfo->mIRQ);

		FUNC206HAL432(pJmIICInfo->mIRQ);
	}
#endif

	if (0xFFFFFFFF == value) {
		FUNC206LXDEV136(pJmIICInfo->mLockSem);
		return -10;
	}


	errorCode = FUNC206HAL154(pJmIICInfo);
	if ((0 != errorCode) || (0xFFFFFFFF == value)) {
		if (0xFFFFFFFF != value) {
			FUNC206HAL150(pJmIICInfo, errorCode);
#if (1 == MAC206HAL073)
			V206KDEBUG002("[ERROR] IIC error(0x%X) at %d of %s\n", errorCode, __LINE__, __FUNCTION__);
#endif
		}

		FUNC206LXDEV136(pJmIICInfo->mLockSem);


		return -10;
	}

	FUNC206LXDEV136(pJmIICInfo->mLockSem);

	return recount;
}