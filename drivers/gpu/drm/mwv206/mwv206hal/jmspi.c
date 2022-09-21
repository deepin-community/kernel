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
#include "mwv206dev.h"

#include "jmspi.h"
#include "special_list.h"
#include "oshal.h"

#include "jmirq.h"

#include <linux/string.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#include "gljos.h"

#define MAC206HAL198     0x5000



#define MAC206HAL180             100

#define MAC206HAL181     96


#define MAC206HAL200   0x70
#define MAC206HAL201   0x74


#define MAC206HAL012     0
#define MAC206HAL013     1
#define MAC206HAL016     2
#define MAC206HAL020     3

#define MAC206HAL072(reg, vlaue)   V206DEV006(devInfo, (0x400000 + MAC206HAL198 + reg), vlaue);
#define MAC206HAL071(reg) V206DEV007(devInfo, (0x400000 + MAC206HAL198 + reg));


#define MAC206HAL019        (1 << 10)
#define MAC206HAL017        (1 << 9)
#define MAC206HAL014              (1 << 8)
#define MAC206HAL021         (0xf << 4)
#define MAC206HAL022     (0x1f)
#define MAC206HAL018     (0x1f << 13)
#define MAC206HAL015         (1 << 8)


#define MAC206HAL202         0x1
#define MAC206HAL203         0x2
#define MAC206HAL204    (1<<3)
#define MAC206HAL205   (1<<4)


#define MAC206HAL197    5
#define MAC206HAL179       32



#define MAC206HAL044    0x1


#define MAC206HAL045    0x2


#define MAC206HAL046    0x4


#define MAC206HAL047    0x8

#define MAC206HAL162           MAC206HAL045
#define MAC206HAL177           MAC206HAL046
#define MAC206HAL208     MAC206HAL047


#define MAC206HAL032         64
#define MAC206HAL144    62

#define MAC206HAL176      30
#define MAC206HAL175       32


#define MAC206HAL161      32




#define MAC206HAL196      0



#define MAC206HAL138      0x53504944


typedef struct FUNC206HAL031 {
	void           *mdevInfo;
	unsigned int    mDevKey;
	int             mIndex;
	int             V206DEV050;
	unsigned int    mCurCummuFreq;
	OSHAL_LOCK      mLockSem;

	int             mIntEnable;

	unsigned int    mIRQNumber;


	unsigned int    mFreqDivReg;
	unsigned int    mIntReg;
	unsigned int    mCtrlReg;
	unsigned int    mRxFifoClearReg;
	unsigned int    mTxFifoClearReg;
	unsigned int    mRxStartReg;
	unsigned int    mWriteFifoReg;
	unsigned int    mStatusReg;
	unsigned int    mStartTransReg;
	unsigned int    mReadFifoReg;
	unsigned int    mTxFifoUsedLen;
	unsigned int    mRxFifoUsedLen;

	unsigned int    mSSBuildTimeReg;
	unsigned int    mSSKeepTimeReg;

	unsigned int    mTRDataCountReg;
	unsigned int    mRxIntValveReg;
	unsigned int    mTxIntValveReg;

#if (1 == MAC206HAL196)
	OSHAL_EVENT     mIRQSem;
#endif
} JM_SPI_DEV_INFO, *P_JM_SPI_DEV_INFO;



typedef struct FUNC206HAL032 {
	SPECIAL_SINGLE_LIST_ENTRY   mLink;
	JM_SPI_DEV_INFO             mDevInfo;
} JM_SPI_DEV_LISTNODE, *P_JM_SPI_DEV_LISTNODE;


#define MAC206HAL199(index)   (index << 7)



static SPECIAL_SINGLE_LIST_ENTRY FUNC206HAL061 = {NULL};


static unsigned int FUNC206HAL062;


static OSHAL_LOCK FUNC206HAL064;




static int FUNC206HAL185(JMSPI pSpiDev)
{
	P_JM_SPI_DEV_INFO pJmSpiDevInfo = NULL;

	if (NULL == pSpiDev) {
		return 1;
	}

	pJmSpiDevInfo = (P_JM_SPI_DEV_INFO)pSpiDev;
	if (MAC206HAL138 != pJmSpiDevInfo->mDevKey) {
		return 1;
	}

	return 0;
}



int FUNC206HAL170(void)
{
	if (0 < FUNC206HAL062) {
		return 0;
	}

	FUNC206HAL062 = 1;

	FUNC206HAL064 = FUNC206HAL440();
	if (0 == FUNC206HAL064) {
		FUNC206HAL062 = 0;
		return -1;
	}

	FUNC206HAL061.Next = NULL;

	return 0;
}


#if (1 == MAC206HAL196)

static int FUNC206HAL177(int intStauts, void *param)
{
	P_JM_SPI_DEV_INFO pSpiDevInfo = (P_JM_SPI_DEV_INFO)param;



	FUNC206HAL428(pSpiDevInfo->mIRQSem);
	return IRQ_HANDLED;
}
#endif


int FUNC206HAL166(void *devInfo, int index, JMSPI *pSpiDev)
{
	V206DEV025 *pDev = (V206DEV025 *)devInfo;
	P_JM_SPI_DEV_LISTNODE pJmSpiDevListNode = NULL;
	P_SPECIAL_SINGLE_LIST_ENTRY pListEntry = NULL;
#if (1 == MAC206HAL196)
	int result;
#endif
	if (NULL == pSpiDev || index < 0 || index > 1) {
		return -1;
	}

	if (0 == FUNC206HAL062) {
		return -2;
	}

	FUNC206HAL439(FUNC206HAL064, WAIT_FOREVER);
	if (NULL != FUNC206HAL061.Next) {

		pListEntry = FUNC206HAL061.Next;
		while (NULL != pListEntry) {
			pJmSpiDevListNode = (P_JM_SPI_DEV_LISTNODE)pListEntry;
			if (devInfo == pJmSpiDevListNode->mDevInfo.mdevInfo
					&& index == pJmSpiDevListNode->mDevInfo.mIndex) {

				pSpiDev = (JMSPI)&pJmSpiDevListNode->mDevInfo;
				FUNC206HAL459(FUNC206HAL064);
				return 0;
			}

			pListEntry = pListEntry->Next;
		}
	}

	pJmSpiDevListNode = (P_JM_SPI_DEV_LISTNODE)FUNC206HAL442(sizeof(JM_SPI_DEV_LISTNODE), 4);
	if (NULL == pJmSpiDevListNode) {
		FUNC206HAL459(FUNC206HAL064);
		return -3;
	}

	memset(pJmSpiDevListNode, 0, sizeof(JM_SPI_DEV_LISTNODE));

	pJmSpiDevListNode->mDevInfo.mdevInfo = pDev;
	pJmSpiDevListNode->mDevInfo.mIndex = index;
	pJmSpiDevListNode->mDevInfo.mDevKey = MAC206HAL138;



	pJmSpiDevListNode->mDevInfo.mIRQNumber = SPI_INT0_NUMBER + index;

	pJmSpiDevListNode->mDevInfo.mFreqDivReg = 0x00 + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mIntReg = 0x04 + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mCtrlReg = 0x08 + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mRxFifoClearReg = 0x0C + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mTxFifoClearReg = 0x10 + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mRxStartReg = 0x14 + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mWriteFifoReg = 0x18 + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mStatusReg = 0x1C + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mStartTransReg = 0x20 + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mReadFifoReg = 0x24 + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mTxFifoUsedLen = 0x28 + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mRxFifoUsedLen = 0x2C + MAC206HAL199(index);

	pJmSpiDevListNode->mDevInfo.mSSBuildTimeReg = 0x30 + MAC206HAL199(
			index);
	pJmSpiDevListNode->mDevInfo.mSSKeepTimeReg = 0x34 + MAC206HAL199(
			index);

	pJmSpiDevListNode->mDevInfo.mTRDataCountReg = 0x38 + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mRxIntValveReg = 0x3C + MAC206HAL199(index);
	pJmSpiDevListNode->mDevInfo.mTxIntValveReg = 0x40 + MAC206HAL199(index);


	MAC206HAL072(pJmSpiDevListNode->mDevInfo.mIntReg, 1);


	MAC206HAL072(pJmSpiDevListNode->mDevInfo.mRxFifoClearReg, 1);
	MAC206HAL072(pJmSpiDevListNode->mDevInfo.mRxFifoClearReg, 0);
	MAC206HAL072(pJmSpiDevListNode->mDevInfo.mTxFifoClearReg, 1);
	MAC206HAL072(pJmSpiDevListNode->mDevInfo.mTxFifoClearReg, 0);





	pJmSpiDevListNode->mDevInfo.V206DEV050 = pDev->V206DEV035 / 2;
	V206KDEBUG001(V206KDEBUG009, "spi ref clock %dMHz\n",
			pJmSpiDevListNode->mDevInfo.V206DEV050);

	pJmSpiDevListNode->mDevInfo.mLockSem = FUNC206HAL440();
	if (0 == pJmSpiDevListNode->mDevInfo.mLockSem) {
		FUNC206HAL443(pJmSpiDevListNode);
		pJmSpiDevListNode = NULL;
		FUNC206HAL459(FUNC206HAL064);
		return -3;
	}

#if (1 == MAC206HAL196)
	pJmSpiDevListNode->mDevInfo.mIRQSem = FUNC206HAL426(EMPTY);
	if (0 == pJmSpiDevListNode->mDevInfo.mIRQSem) {
		FUNC206HAL441(pJmSpiDevListNode->mDevInfo.mLockSem);
		FUNC206HAL443(pJmSpiDevListNode);
		pJmSpiDevListNode = NULL;
		FUNC206HAL459(FUNC206HAL064);
		return -5;
	}

	result = FUNC206HAL431(devInfo,
			pJmSpiDevListNode->mDevInfo.mIRQNumber,
			FUNC206HAL177,
			"jmSpi",
			(void *)&(pJmSpiDevListNode->mDevInfo));
	if (0 == result) {
		FUNC206HAL120(devInfo, pJmSpiDevListNode->mDevInfo.mIRQNumber);
		FUNC206HAL434(pJmSpiDevListNode->mDevInfo.mIRQNumber);

		pJmSpiDevListNode->mDevInfo.mIntEnable = 1;
	}
#endif

	PushEntryList(&FUNC206HAL061, &pJmSpiDevListNode->mLink);
	*pSpiDev = (JMSPI)&pJmSpiDevListNode->mDevInfo;
	FUNC206HAL459(FUNC206HAL064);

	return 0;
}



#define MAC206HAL160  do\
{\
	fifonum = MAC206HAL071(pJmSpiDevInfo->mRxFifoUsedLen);\
	for (i = 0; (i < fifonum) && (readCount < (*pRecvLen)); i++) {\
		pReadPos[readCount] = MAC206HAL071(pJmSpiDevInfo->mReadFifoReg);\
		readCount += 1;\
	} \
} while (0)



#define MAC206HAL226 do\
{\
	i = 0;\
	while (sendCount < (*pSendLen)) {\
		MAC206HAL072(pJmSpiDevInfo->mWriteFifoReg, pWritePos[sendCount]);\
		sendCount += 1;\
		i += 1;\
		if (i == MAC206HAL176) {\
			break;\
		} \
	} \
} while (0)



#define MAC206HAL011 do\
{\
	MAC206HAL072(pJmSpiDevInfo->mIntReg, 1);\
	MAC206HAL072(pJmSpiDevInfo->mTxFifoClearReg, 1);\
	MAC206HAL072(pJmSpiDevInfo->mTxFifoClearReg, 0);\
	MAC206HAL072(pJmSpiDevInfo->mRxFifoClearReg, 1);\
	MAC206HAL072(pJmSpiDevInfo->mRxFifoClearReg, 0);\
	MAC206HAL072(pJmSpiDevInfo->mTRDataCountReg, 0);\
} while (0)\



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
		int recvSampMode)
{
	unsigned int status = 0;
	unsigned int tick = 0;
	unsigned int control = 0;
	unsigned int i, readCount, sendCount;
	unsigned int fifonum;
	P_JM_SPI_DEV_INFO pJmSpiDevInfo = NULL;
	char *pWritePos = NULL;
	char *pReadPos;
	void *devInfo = NULL;


	unsigned int timeoutValve;

	V206KDEBUG001(V206KDEBUG007, "[%s]L%d: begin.\n", __FUNCTION__, __LINE__);
	if (1 == FUNC206HAL185(pSpiDev)) {
		V206KDEBUG001(V206KDEBUG006, "jmspihandle Invalid\n");
		return -1;
	}
	V206KDEBUG001(V206KDEBUG007, "[%s]L%d\n", __FUNCTION__, __LINE__);
	if ((2 <= sclkPhase)
			|| (2 <= sclkPolarity)
			|| (2 <= lsbFirst)
			|| (2 <= recvSampMode)
			|| (NULL == pSendLen)
			|| (NULL == pRecvLen)
			|| (cmdLength > 0 && NULL == cmdContent)
			|| (*pSendLen > 0 && NULL == pTxBuffer)
			|| (*pRecvLen > 0 && NULL == pRxBuffer)) {
		V206KDEBUG001(V206KDEBUG006, "sclkPhase %d, sclkPolarity %d, lsbFirst %d, recvSampMode %d\n",
				sclkPhase,
				sclkPolarity,
				lsbFirst,
				recvSampMode
			    );
		V206KDEBUG001(V206KDEBUG006, "pSendLen %p, pRecvLen %p, pTxBuffer %p, pRxBuffer %p, cmdContent %p\n",
				pSendLen,
				pRecvLen,
				pTxBuffer,
				pRxBuffer,
				cmdContent
			    );
		return -1;
	}

	pJmSpiDevInfo = (P_JM_SPI_DEV_INFO)pSpiDev;
	devInfo = pJmSpiDevInfo->mdevInfo;


	timeoutValve = FUNC206LXDEV098();


	if (FUNC206HAL439(pJmSpiDevInfo->mLockSem, MAC206HAL197) != 0) {
		V206KDEBUG001(V206KDEBUG006, "[ERROR] lock SPI timeout.\n");
		return -2;
	}


	MAC206HAL072(pJmSpiDevInfo->mIntReg, 1);


	MAC206HAL072(pJmSpiDevInfo->mTxFifoClearReg, 1);
	MAC206HAL072(pJmSpiDevInfo->mTxFifoClearReg, 0);
	MAC206HAL072(pJmSpiDevInfo->mRxFifoClearReg, 1);
	MAC206HAL072(pJmSpiDevInfo->mRxFifoClearReg, 0);

	readCount = 0;
	sendCount = 0;
	pWritePos = (char *)pTxBuffer;
	pReadPos = pRxBuffer;


	control = ((sclkPhase << MAC206HAL012)
			| (sclkPolarity << MAC206HAL013)
			| (lsbFirst << MAC206HAL016)
			| (recvSampMode << MAC206HAL020));
	control = (control & (~MAC206HAL021)) | ((slave << 4) & MAC206HAL021);

#if (1 == MAC206HAL196)
	if (1 == pJmSpiDevInfo->mIntEnable)
#endif
	{
		control |= MAC206HAL014;

		if (*pSendLen > MAC206HAL144) {
			control |= MAC206HAL019;
			MAC206HAL072(pJmSpiDevInfo->mTxIntValveReg, MAC206HAL176);
		}

		if (*pRecvLen > MAC206HAL144) {
			control |= MAC206HAL017;
			MAC206HAL072(pJmSpiDevInfo->mRxIntValveReg, MAC206HAL161);
		}
	}

	MAC206HAL072(pJmSpiDevInfo->mCtrlReg, control);


	MAC206HAL072(pJmSpiDevInfo->mTRDataCountReg, transBytes);


	MAC206HAL072(pJmSpiDevInfo->mRxStartReg, (startRcv & 0x1f));


	V206KDEBUG001(V206KDEBUG009, "send cmd len %d\n", cmdLength);

	for (i = 0; i < cmdLength; i++) {
		V206KDEBUG001(V206KDEBUG009, "[%d]: %#x\n", i, cmdContent[i]);
		MAC206HAL072(pJmSpiDevInfo->mWriteFifoReg, cmdContent[i]);
	}

	V206KDEBUG001(V206KDEBUG009, "send data len %d\n", *pSendLen);

	for (i = 0; i < *pSendLen; i++) {
		V206KDEBUG001(V206KDEBUG009, "[%d]: %#x\n", i, pWritePos[sendCount]);
		MAC206HAL072(pJmSpiDevInfo->mWriteFifoReg, pWritePos[sendCount]);
		sendCount += 1;


		if (sendCount == (MAC206HAL144 - cmdLength)) {
			break;
		}
	}

	MAC206HAL072(pJmSpiDevInfo->mStartTransReg, 1);

#if (1 == MAC206HAL196)
	if (1 == pJmSpiDevInfo->mIntEnable) {
		while (1) {

			if (OK != FUNC206HAL429(pJmSpiDevInfo->mIRQSem, timeoutValve)) {
				MAC206HAL011;
				FUNC206HAL459(pJmSpiDevInfo->mLockSem);
				*pRecvLen = readCount;
				*pSendLen = sendCount;
				V206KDEBUG001(V206KDEBUG006, "[ERROR] %s wait irq timeout.\n", __FUNCTION__);
				return -3;
			}


			status = MAC206HAL071(pJmSpiDevInfo->mIntReg);


			if (0 != (status & MAC206HAL177)) {

				MAC206HAL226;
			}


			if (0 != (status & MAC206HAL162)) {

				MAC206HAL160;
			}


			if (0 != (status & MAC206HAL208)) {
				if (*pRecvLen > 0) {
					MAC206HAL160;
				}


				break;
			}
		}
	} else
#endif
	{
		tick = 0;
		while (1) {

			status = MAC206HAL071(pJmSpiDevInfo->mIntReg);
			if (status == 0) {
				tick += 1;
				if (tick > timeoutValve) {
					MAC206HAL011;
					FUNC206HAL459(pJmSpiDevInfo->mLockSem);
					*pRecvLen = readCount;
					*pSendLen = sendCount;
					V206KDEBUG001(V206KDEBUG006, "[ERROR] %s wait trans finished timeout.\n", __FUNCTION__);
					return -3;
				}

				FUNC206LXDEV128(1);
				continue;
			}
			MAC206HAL072(pJmSpiDevInfo->mIntReg, 1);
			tick = 0;


			if (0 != (status & MAC206HAL177)) {

				V206KDEBUG001(V206KDEBUG007, "SEND_INT_FLAG\n");
				MAC206HAL226;
			}


			if (0 != (status & MAC206HAL162)) {

				V206KDEBUG001(V206KDEBUG007, "RECV_INT_FLAG\n");
				MAC206HAL160;
			}


			if (0 != (status & MAC206HAL208)) {
				V206KDEBUG001(V206KDEBUG007, "TRANS_COMPLETE_FLAG\n");
				if (*pRecvLen > 0) {
					MAC206HAL160;
				}

				V206KDEBUG001(V206KDEBUG007, "[DEBUG] %s finished.\n", __FUNCTION__);


				break;
			}
		}
	}

	MAC206HAL011;
	FUNC206HAL459(pJmSpiDevInfo->mLockSem);

	*pRecvLen = readCount;
	*pSendLen = sendCount;

	return 0;
}



int FUNC206HAL141(JMSPI pSpiDev,
		int slave,
		int sclkPhase,
		int sclkPolarity,
		int lsbFirst,
		int recvSampMode)
{
	unsigned long control = 0;
	P_JM_SPI_DEV_INFO pJmSpiDevInfo = NULL;
	void *devInfo = NULL;

	if (1 == FUNC206HAL185(pSpiDev)
			|| 2 <= sclkPhase
			|| 2 <= sclkPolarity
			|| 2 <= lsbFirst
			|| 2 <= recvSampMode) {
		V206KDEBUG001(V206KDEBUG006, "[%s]:spi handel %s, phase %d, polarity %d, lsb %d, samp %d\n",
				__FUNCTION__, FUNC206HAL185(pSpiDev) ? "Invalid" : "Valid",
				sclkPhase, sclkPolarity, lsbFirst, recvSampMode);
		return -1;
	}

	pJmSpiDevInfo = (P_JM_SPI_DEV_INFO)pSpiDev;
	devInfo = pJmSpiDevInfo->mdevInfo;


	if (FUNC206HAL439(pJmSpiDevInfo->mLockSem, MAC206HAL197) != 0) {
		V206KDEBUG001(V206KDEBUG006, "[ERROR] lock SPI timeout.\n");
		return -2;
	}


	MAC206HAL072(pJmSpiDevInfo->mIntReg, 1);


	MAC206HAL072(pJmSpiDevInfo->mTxFifoClearReg, 1);
	MAC206HAL072(pJmSpiDevInfo->mTxFifoClearReg, 0);
	MAC206HAL072(pJmSpiDevInfo->mRxFifoClearReg, 1);
	MAC206HAL072(pJmSpiDevInfo->mRxFifoClearReg, 0);


	control = ((sclkPhase << MAC206HAL012)
			| (sclkPolarity << MAC206HAL013)
			| (lsbFirst << MAC206HAL016)
			| (recvSampMode << MAC206HAL020));
	control = (control & (~MAC206HAL021)) | ((slave << 9) & MAC206HAL021);

#if (1 == MAC206HAL196)
	if (1 == pJmSpiDevInfo->mIntEnable) {
		control |= MAC206HAL014;
	}
#endif
	MAC206HAL072(pJmSpiDevInfo->mCtrlReg, control);

	FUNC206HAL459(pJmSpiDevInfo->mLockSem);

	return 0;
}




int FUNC206HAL173(JMSPI pSpiDev,
		const char *cmdContent,
		unsigned int cmdLength,
		const char *pTxBuffer,
		unsigned int *pSendLen,
		char *pRxBuffer,
		unsigned int *pRecvLen,
		unsigned int startRcv,
		unsigned int transBytes)
{
	unsigned int status = 0;
	unsigned int tick = 0;
#if (1 == MAC206HAL196)
	unsigned int control, tmpControl;
#endif
	unsigned int i, readCount, sendCount;
	unsigned int fifonum;
	P_JM_SPI_DEV_INFO pJmSpiDevInfo = NULL;
	unsigned char *pWritePos = NULL;
	char *pReadPos;
	void *devInfo = NULL;


	unsigned int timeoutValve;

	if (1 == FUNC206HAL185(pSpiDev)
			|| NULL == pSendLen
			|| NULL == pRecvLen
			|| (cmdLength > 0 && NULL == cmdContent)
			|| (*pSendLen > 0 && NULL == pTxBuffer)
			|| (*pRecvLen > 0 && NULL == pRxBuffer)) {
		V206KDEBUG001(V206KDEBUG006, "[%s]:spi handle %s, sendlen %d, recvlen %d\n",
				__FUNCTION__, FUNC206HAL185(pSpiDev) ? "Invalid" : "Valid",
				*pSendLen, *pRecvLen);
		return -1;
	}

	pJmSpiDevInfo = (P_JM_SPI_DEV_INFO)pSpiDev;
	devInfo = pJmSpiDevInfo->mdevInfo;

	timeoutValve = FUNC206HAL472(640000, pJmSpiDevInfo->mCurCummuFreq);


	if (FUNC206HAL439(pJmSpiDevInfo->mLockSem, MAC206HAL197) != 0) {
		V206KDEBUG001(V206KDEBUG006, "[ERROR] lock SPI timeout.\n");
		return -2;
	}


	MAC206HAL072(pJmSpiDevInfo->mIntReg, 1);


	MAC206HAL072(pJmSpiDevInfo->mTxFifoClearReg, 1);
	MAC206HAL072(pJmSpiDevInfo->mTxFifoClearReg, 0);
	MAC206HAL072(pJmSpiDevInfo->mRxFifoClearReg, 1);
	MAC206HAL072(pJmSpiDevInfo->mRxFifoClearReg, 0);

	readCount = 0;
	sendCount = 0;
	pWritePos = (char *)pTxBuffer;
	pReadPos = pRxBuffer;


#if (1 == MAC206HAL196)
	if (1 == pJmSpiDevInfo->mIntEnable) {
		control = MAC206HAL071(pJmSpiDevInfo->mCtrlReg);
		tmpControl = control | MAC206HAL014;

		if (*pSendLen > MAC206HAL144) {
			tmpControl |= MAC206HAL019;
			MAC206HAL072(pJmSpiDevInfo->mTxIntValveReg, MAC206HAL176);
		}

		if (*pRecvLen > MAC206HAL144) {
			tmpControl |= MAC206HAL017;
			MAC206HAL072(pJmSpiDevInfo->mRxIntValveReg, MAC206HAL161);
		}

		if (control != tmpControl) {
			MAC206HAL072(pJmSpiDevInfo->mCtrlReg, tmpControl);
		}
	}
#endif


	MAC206HAL072(pJmSpiDevInfo->mTRDataCountReg, transBytes);


	MAC206HAL072(pJmSpiDevInfo->mRxStartReg, (startRcv & 0x1f));


	for (i = 0; i < cmdLength; i++) {
		MAC206HAL072(pJmSpiDevInfo->mWriteFifoReg, cmdContent[i]);
	}


	for (i = 0; i < *pSendLen; i++) {
		MAC206HAL072(pJmSpiDevInfo->mWriteFifoReg, pWritePos[sendCount]);
		sendCount += 1;


		if (sendCount == (MAC206HAL144 - cmdLength)) {
			break;
		}
	}

	MAC206HAL072(pJmSpiDevInfo->mStartTransReg, 1);

#if (1 == MAC206HAL196)
	if (1 == pJmSpiDevInfo->mIntEnable) {
		while (1) {

			if (OK != FUNC206HAL429(pJmSpiDevInfo->mIRQSem, timeoutValve)) {
				MAC206HAL011;
				FUNC206HAL459(pJmSpiDevInfo->mLockSem);
				*pRecvLen = readCount;
				*pSendLen = sendCount;
				V206KDEBUG001(V206KDEBUG006, "[ERROR] %s wait irq timeout.\n", __FUNCTION__);
				return -3;
			}


			status = MAC206HAL071(pJmSpiDevInfo->mIntReg);


			if (0 != (status & MAC206HAL177)) {

				MAC206HAL226;
			}


			if (0 != (status & MAC206HAL162)) {

				MAC206HAL160;
			}


			if (0 != (status & MAC206HAL208)) {
				if (*pRecvLen > 0) {
					MAC206HAL160;
				}


				break;
			}
		}
	} else
#endif
	{
		tick = 0;
		while (1) {

			status = MAC206HAL071(pJmSpiDevInfo->mIntReg);
			if (status == 0) {
				tick += 1;
				if (tick > timeoutValve) {
					MAC206HAL011;
					FUNC206HAL459(pJmSpiDevInfo->mLockSem);
					*pRecvLen = readCount;
					*pSendLen = sendCount;
					V206KDEBUG001(V206KDEBUG006, "[ERROR] %s wait trans finished timeout.\n", __FUNCTION__);
					return -3;
				}

				FUNC206LXDEV128(1);
				continue;
			}
			MAC206HAL072(pJmSpiDevInfo->mIntReg, 1);
			tick = 0;


			if (0 != (status & MAC206HAL162)) {

				MAC206HAL160;
			}


			if (0 != (status & MAC206HAL177)) {

				MAC206HAL226;
			}


			if (0 != (status & MAC206HAL208)) {
				if (*pRecvLen > 0) {
					MAC206HAL160;
				}

				V206KDEBUG001(V206KDEBUG007, "[DEBUG] %s finished.\n", __FUNCTION__);


				break;
			}
		}
	}

	MAC206HAL011;
	FUNC206HAL459(pJmSpiDevInfo->mLockSem);

	*pRecvLen = readCount;
	*pSendLen = sendCount;

	return 0;
}



int FUNC206HAL168(JMSPI pSpiDev,
		int *pRefClockFreq,
		unsigned int *pCommuFreq)
{
	P_JM_SPI_DEV_INFO pJmSpiDevInfo = NULL;
	volatile unsigned int value;
	void *devInfo = NULL;

	if (1 == FUNC206HAL185(pSpiDev)
			|| NULL == pRefClockFreq
			|| NULL == pCommuFreq) {
		return -1;
	}

	pJmSpiDevInfo = (P_JM_SPI_DEV_INFO)pSpiDev;
	devInfo = pJmSpiDevInfo->mdevInfo;


	if (FUNC206HAL439(pJmSpiDevInfo->mLockSem, MAC206HAL197) != 0) {
		V206KDEBUG001(V206KDEBUG006, "[ERROR] lock SPI timeout.\n");
		return -2;
	}


	value = MAC206HAL071(pJmSpiDevInfo->mFreqDivReg);

	FUNC206HAL459(pJmSpiDevInfo->mLockSem);

	*pRefClockFreq = pJmSpiDevInfo->V206DEV050;

	*pCommuFreq = FUNC206HAL472(pJmSpiDevInfo->V206DEV050 * 500, value);

	return 0;
}



int FUNC206HAL171(JMSPI pSpiDev,
		unsigned int commuFreq)
{
	P_JM_SPI_DEV_INFO pJmSpiDevInfo = (P_JM_SPI_DEV_INFO)pSpiDev;
	unsigned int freqDiv;
	void *devInfo = NULL;
	int refClockFreq;

	if (1 == FUNC206HAL185(pSpiDev)) {
		V206KDEBUG001(V206KDEBUG006, "[ERROR]:spi handle is invalid!\n");
		return -1;
	}

	if (pJmSpiDevInfo->V206DEV050 == 0) {
		V206KDEBUG001(V206KDEBUG006, "[ERROR]:mRefClockFreq = 0!\n");
		return -2;
	}

	if (commuFreq == 0) {
		V206KDEBUG001(V206KDEBUG006, "[ERROR]:commuFreq = 0!\n");
		return -3;
	}

	refClockFreq = pJmSpiDevInfo->V206DEV050;
	pJmSpiDevInfo->mCurCummuFreq = commuFreq;
	devInfo = pJmSpiDevInfo->mdevInfo;


	freqDiv = FUNC206HAL472(refClockFreq * 500, commuFreq);

	V206KDEBUG001(V206KDEBUG007, "[INFO] %s, refClock=%dMHz, commuFreq=%dKHz\n", __FUNCTION__, refClockFreq, commuFreq);
	V206KDEBUG001(V206KDEBUG007, "[INFO] set clock div reg %d(0x%08X)\n", freqDiv, freqDiv);


	if (FUNC206HAL439(pJmSpiDevInfo->mLockSem, MAC206HAL197) != 0) {
		V206KDEBUG001(V206KDEBUG006, "[ERROR] lock SPI timeout.\n");
		return -4;
	}


	MAC206HAL072(pJmSpiDevInfo->mFreqDivReg, freqDiv);

	FUNC206HAL459(pJmSpiDevInfo->mLockSem);

	return 0;
}


void FUNC206HAL169(JMSPI pSpiDev)
{
	unsigned int version0, version1, version2;
	P_JM_SPI_DEV_INFO pJmSpiDevInfo = (P_JM_SPI_DEV_INFO)pSpiDev;
	void *devInfo = pJmSpiDevInfo->mdevInfo;
	int index = pJmSpiDevInfo->mIndex;

	version0 = MAC206HAL071(MAC206HAL200 + MAC206HAL199(index));
	version1 = MAC206HAL071(MAC206HAL201 + MAC206HAL199(index));
	version2 = (version1 & 0xF) + ((version1 >> 4) & 0xF) * 10;
	V206KDEBUG001(V206KDEBUG007, "[INFO] SPI CONTROLER VER:%04X_%02X_%02X-%d.%d.%d\n",
			version0 >> 16,
			(version0 >> 8) & 0xFF,
			version0 & 0xFF,
			(version1 >> 12) & 0xF,
			(version1 >> 8) & 0xF,
			version2);
}