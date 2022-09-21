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

int FUNC206HAL219(V206DEV025 *pDev)
{
	int result;

	result = FUNC206HAL158(pDev->V206DEV051);

	return result;
}

int FUNC206HAL218(V206DEV025 *pDev)
{
	int result = 0;

	FUNC206HAL152(pDev->V206DEV051, &result);

	return result;
}


int FUNC206HAL220(V206DEV025 *pDev, void *arg)
{
	V206IOCTL144 pTRInfo = (V206IOCTL144)arg;
	int result = -1;
	char *pData = NULL;


	V206DEV005("mSlaveAddr %02x, mDestAddr %02x, mDirect %02x, mTRLen %02x, StartBit %d, stopbit %d\n",
			pTRInfo->mSlaveAddr,
			pTRInfo->mDestAddr,
			pTRInfo->mDirect,
			pTRInfo->mTRLen,
			pTRInfo->mIsSetStartBit,
			pTRInfo->mIsSetStopBit);

	if (pTRInfo->mTRLen != 0) {
		pData = (char *)kmalloc(pTRInfo->mTRLen, GFP_KERNEL);

		if (NULL == pData) {
			V206KDEBUG002("[ERROR]:kmalloc failed!\n");
			pTRInfo->mOpResult = -3;
			goto FUNC206HAL012;
		}
	}

	if (V206IOCTL034 == pTRInfo->mDirect) {

		if (NULL != pTRInfo->mpData) {
			V206DEV005("[INFO]:copy data form [0X%p] of user space\n", pTRInfo->mpData);

			if (copy_from_user((void *)pData, pTRInfo->mpData, pTRInfo->mTRLen)) {
				pTRInfo->mOpResult = -4;
				V206KDEBUG002("[ERROR]:copy_from_user failed!\n");
				goto FUNC206HAL012;
			}
		}

		result = FUNC206HAL159(pDev->V206DEV051,
				pTRInfo->mSlaveAddr,
				pTRInfo->mDestAddr,
				pTRInfo->mTRLen,
				pData,
				pTRInfo->mIsSetStartBit,
				pTRInfo->mIsSetStopBit);
		pTRInfo->mOpResult = result;
		if (result < 0) {
			V206KDEBUG002("[ERROR]:jmiicSendNBytes failed!\n");
			goto FUNC206HAL012;
		}
	} else {

		result = FUNC206HAL157(pDev->V206DEV051,
				pTRInfo->mSlaveAddr,
				pTRInfo->mDestAddr,
				pTRInfo->mTRLen,
				pData,
				pTRInfo->mIsSetStartBit,
				pTRInfo->mIsSetStopBit);
		pTRInfo->mOpResult = result;
		if (result < 0) {
			V206KDEBUG002("[ERROR]:jmiicRecvNBytes failed!\n");
			goto FUNC206HAL012;
		}

		if (copy_to_user(pTRInfo->mpData, (void *)pData, pTRInfo->mTRLen)) {
			pTRInfo->mOpResult = -4;
			V206KDEBUG002("[ERROR]:copy_to_user failed!\n");
			goto FUNC206HAL012;
		}
		V206DEV005("[INFO]:jmiicRecvNBytes succeed!\n");
	}

FUNC206HAL012:
	kfree(pData);

	return pTRInfo->mOpResult;
}

int FUNC206HAL352(V206DEV025 *pDev, long arg)
{
	V206IOCTL144 pTRInfo = (V206IOCTL144)arg;
	int ret;

	FUNC206HAL369(pDev, 1);
	switch (pTRInfo->op) {
	case V206IOCTL034:
		ret = FUNC206HAL220(pDev, (void *)arg);
		break;
	case V206IOCTL035:
		ret = FUNC206HAL220(pDev, (void *)arg);
		break;
	case V206IOCTL036:
		ret = FUNC206HAL219(pDev);
		pTRInfo->mOpResult = ret;
		break;
	case V206IOCTL037:
		ret = FUNC206HAL160(pDev->V206DEV051, pTRInfo->mOpResult);
		pTRInfo->mOpResult = ret;
		break;
	case V206IOCTL038:
		ret = FUNC206HAL152(pDev->V206DEV051, pTRInfo->mpData);
		pTRInfo->mOpResult = ret;
		break;
	case V206IOCTL039:
		ret = FUNC206HAL161(pDev->V206DEV051, pTRInfo->mOpResult);
		pTRInfo->mOpResult = ret;
		break;
	case V206IOCTL040:
		ret = FUNC206HAL153(pDev->V206DEV051, pTRInfo->mpData);
		pTRInfo->mOpResult = ret;
		break;
	default:
		ret = -1;
		pTRInfo->mOpResult = ret;
	}
	FUNC206HAL369(pDev, 0);

	return ret;
}


static int FUNC206HAL240(V206DEV025 *pMwv206DeviceData,
		V206IOCTL154 pTransContext)
{
	int result;

	V206KDEBUG001(V206KDEBUG009, "[%s]:\nspi %d, slave %d, phase %d, polarity %d, lsb %d, sampmode %d\n",
			__FUNCTION__, pTransContext->spi, pTransContext->slave, pTransContext->sclkPhase,
			pTransContext->sclkPolarity, pTransContext->lsbFirst, pTransContext->recvSampMode);

	if (pTransContext->spi == 0) {
		result = FUNC206HAL141(pMwv206DeviceData->V206DEV053,
				pTransContext->slave,
				pTransContext->sclkPhase,
				pTransContext->sclkPolarity,
				pTransContext->lsbFirst,
				pTransContext->recvSampMode);
	} else {
		result = FUNC206HAL141(pMwv206DeviceData->V206DEV054,
				pTransContext->slave,
				pTransContext->sclkPhase,
				pTransContext->sclkPolarity,
				pTransContext->lsbFirst,
				pTransContext->recvSampMode);
	}

	return result;
}


static int FUNC206HAL267(V206DEV025 *pMwv206DeviceData,
		V206IOCTL150 commuFreq)
{
	int result = 0;

	V206KDEBUG001(V206KDEBUG007, "[INFO] %s at %d of %s\n",
			__FUNCTION__, __LINE__, __FILE__);

	if (commuFreq->spi == 0) {
		result = FUNC206HAL171(pMwv206DeviceData->V206DEV053,
				commuFreq->freq);
	} else {
		result = FUNC206HAL171(pMwv206DeviceData->V206DEV054,
				commuFreq->freq);
	}
	if (0 != result) {
		V206KDEBUG001(V206KDEBUG006, "[ERROR] %s failed, result = %d\n",
				__FUNCTION__,
				result);
	}

	return result;
}

static int FUNC206HAL266(V206DEV025 *pMwv206DeviceData,
		V206IOCTL150 commuFreq)
{
	unsigned int cfreq;
	int refclock;
	int result;

	V206KDEBUG001(V206KDEBUG007, "[INFO] %s at %d of %s\n",
			__FUNCTION__, __LINE__, __FILE__);

	if (commuFreq->spi == 0) {
		result = FUNC206HAL168(pMwv206DeviceData->V206DEV053,
				&refclock, &cfreq);
	} else {
		result = FUNC206HAL168(pMwv206DeviceData->V206DEV054,
				&refclock, &cfreq);
	}
	if (0 != result) {
		V206KDEBUG001(V206KDEBUG006, "[ERROR] %s failed, result = %d\n",
				__FUNCTION__,
				result);
		return result;
	}

	return copy_to_user(commuFreq->freq_out, &cfreq, sizeof(cfreq));
}

static int FUNC206HAL268(V206DEV025 *pMwv206DeviceData,
		V206IOCTL152 pTransParam)
{
	int result = 0;

	char *pSendData = NULL;
	char *pRecvData = NULL;
	char *pCmdData = NULL;
	unsigned int sendLen;
	unsigned int recvLen;

	if (pTransParam->cmdLength > 0) {
		V206KDEBUG001(V206KDEBUG009, "L%d:cmdLength %d\n", __LINE__, pTransParam->cmdLength);

		pCmdData = (char *)kmalloc(pTransParam->cmdLength, GFP_KERNEL);
		if (NULL == pCmdData) {
			V206KDEBUG001(V206KDEBUG006, "[ERROR] mwv206IICSendRecv failed at %d\n",
					__LINE__);
			goto FUNC206HAL019;
		}

		result = copy_from_user(pCmdData, pTransParam->cmdContent, pTransParam->cmdLength);
	}

	result = copy_from_user((void *)&sendLen, pTransParam->pSendLen, sizeof(unsigned int));
	if (sendLen > 0) {
		V206KDEBUG001(V206KDEBUG009, "L%d:pSendLen %d\n", __LINE__, sendLen);

		pSendData = (char *)kmalloc(sendLen, GFP_KERNEL);
		if (NULL == pSendData) {
			V206KDEBUG001(V206KDEBUG006, "[ERROR] mwv206IICSendRecv failed at %d\n",
					__LINE__);
			result = -4;
			goto FUNC206HAL019;
		}

		result = copy_from_user(pSendData, pTransParam->pTxBuffer, sendLen);
	}

	result = copy_from_user(&recvLen, pTransParam->pRecvLen, sizeof(unsigned int));
	if (recvLen > 0) {
		V206KDEBUG001(V206KDEBUG009, "L%d:pRecvLen %d\n", __LINE__, recvLen);

		pRecvData = (char *)kmalloc(recvLen, GFP_KERNEL);
		if (NULL == pRecvData) {
			V206KDEBUG001(V206KDEBUG006, "[ERROR] mwv206IICSendRecv failed at %d\n",
					__LINE__);
			result = -4;
			goto FUNC206HAL019;
		}
	}

	if (pTransParam->spi == 0) {
		V206KDEBUG001(V206KDEBUG009, "[DEBUG] mwv206_SpiTrans USE mSpiHandle0 at %d\n",
				__LINE__);

		result = FUNC206HAL172(pMwv206DeviceData->V206DEV053,
				pTransParam->slave,
				pCmdData,
				pTransParam->cmdLength,
				pSendData,
				&sendLen,
				pRecvData,
				&recvLen,
				pTransParam->startRcv,
				pTransParam->transBytes,
				pTransParam->sclkPhase,
				pTransParam->sclkPolarity,
				pTransParam->lsbFirst,
				pTransParam->recvSampMode);
	} else {
		V206KDEBUG001(V206KDEBUG009, "[DEBUG] mwv206_SpiTrans USE mSpiHandle1 at %d\n",
				__LINE__);

		result = FUNC206HAL172(pMwv206DeviceData->V206DEV054,
				pTransParam->slave,
				pCmdData,
				pTransParam->cmdLength,
				pSendData,
				&sendLen,
				pRecvData,
				&recvLen,
				pTransParam->startRcv,
				pTransParam->transBytes,
				pTransParam->sclkPhase,
				pTransParam->sclkPolarity,
				pTransParam->lsbFirst,
				pTransParam->recvSampMode);
	}
	result = copy_to_user(pTransParam->pSendLen, (void *)&sendLen, sizeof(unsigned int));
	result = copy_to_user(pTransParam->pRecvLen, (void *)&recvLen, sizeof(unsigned int));

	if (recvLen > 0) {
		result = copy_to_user(pTransParam->pRxBuffer, pRecvData, recvLen);
	}

FUNC206HAL019:
	if (NULL != pSendData) {
		kfree(pSendData);
		pSendData = NULL;
	}

	if (NULL != pRecvData) {
		kfree(pRecvData);
		pRecvData = NULL;
	}

	if (NULL != pCmdData) {
		kfree(pCmdData);
		pCmdData = NULL;
	}

	return result;
}


int FUNC206HAL269(V206DEV025 *pMwv206DeviceData,
		V206IOCTL156 pTransParam)
{
	int result = -1;

	char *pSendData = NULL;
	char *pRecvData = NULL;
	char *pCmdData = NULL;
	unsigned int sendLen;
	unsigned int recvLen;

	if (pTransParam->cmdLength > 0) {
		pCmdData = (char *)kmalloc(pTransParam->cmdLength, GFP_KERNEL);
		if (NULL == pCmdData) {
			V206KDEBUG001(V206KDEBUG006, "[ERROR] mwv206IICSendRecv failed at %d\n",
					__LINE__);
			return -ENOMEM;
		}

		if (copy_from_user(pCmdData, pTransParam->cmdContent, pTransParam->cmdLength)) {
			result = -1;
			V206KDEBUG001(V206KDEBUG006, "[ERROR] mwv206_SpiTrans failed at %d\n",
					__LINE__);
			goto FUNC206HAL018;
		}
	}

	result = copy_from_user((void *)&sendLen, pTransParam->pSendLen, sizeof(unsigned int));
	if (sendLen > 0) {
		V206KDEBUG001(V206KDEBUG009, "L%d:pSendLen %d\n", __LINE__, sendLen);

		pSendData = (char *)kmalloc(sendLen, GFP_KERNEL);
		if (NULL == pSendData) {
			V206KDEBUG001(V206KDEBUG006, "[ERROR] mwv206IICSendRecv failed at %d\n",
					__LINE__);
			goto FUNC206HAL018;
		}

		result = copy_from_user(pSendData, pTransParam->pRxBuffer, sendLen);
	}

	result = copy_from_user(&recvLen, pTransParam->pRecvLen, sizeof(unsigned int));
	if (recvLen > 0) {
		V206KDEBUG001(V206KDEBUG009, "L%d:pRecvLen %d\n", __LINE__, recvLen);

		pRecvData = (char *)kmalloc(recvLen, GFP_KERNEL);
		if (NULL == pRecvData) {
			V206KDEBUG001(V206KDEBUG006, "[ERROR] mwv206IICSendRecv failed at %d\n",
					__LINE__);
			goto FUNC206HAL018;
		}
	}

	if (pTransParam->spi == 0) {
		result = FUNC206HAL173(pMwv206DeviceData->V206DEV053,
				pCmdData,
				pTransParam->cmdLength,
				pSendData,
				&sendLen,
				pRecvData,
				&recvLen,
				pTransParam->startRcv,
				pTransParam->transBytes);
	} else {
		result = FUNC206HAL173(pMwv206DeviceData->V206DEV054,
				pCmdData,
				pTransParam->cmdLength,
				pSendData,
				&sendLen,
				pRecvData,
				&recvLen,
				pTransParam->startRcv,
				pTransParam->transBytes);
	}
	result = copy_to_user(pTransParam->pSendLen, (void *)&sendLen, sizeof(unsigned int));
	result = copy_to_user(pTransParam->pRecvLen, (void *)&recvLen, sizeof(unsigned int));

	if (recvLen > 0) {
		result = copy_to_user(pTransParam->pRxBuffer, pRecvData, recvLen);
	}

FUNC206HAL018:
	if (NULL != pSendData) {
		kfree(pSendData);
		pSendData = NULL;
	}

	if (NULL != pRecvData) {
		kfree(pRecvData);
		pRecvData = NULL;
	}

	if (NULL != pCmdData) {
		kfree(pCmdData);
		pCmdData = NULL;
	}

	return result;
}

int FUNC206HAL402(V206DEV025 *pDev, long arg)
{
	V206IOCTL175 *pctl = (V206IOCTL175 *)arg;
	int ret;

	switch (pctl->op) {
	case V206IOCTL041:
		ret = FUNC206HAL240(pDev, &pctl->u.trans_context);
		break;
	case V206IOCTL042:
		ret = FUNC206HAL267(pDev, &pctl->u.commufreq);
		break;
	case V206IOCTL043:
		ret = FUNC206HAL266(pDev, &pctl->u.commufreq);
		break;
	case V206IOCTL044:
		ret = FUNC206HAL268(pDev, &pctl->u.trans);
		break;
	case V206IOCTL045:
		ret = FUNC206HAL269(pDev, &pctl->u.trans_ex);
		break;
	default:
		ret = -1;
	}

	return ret;
}