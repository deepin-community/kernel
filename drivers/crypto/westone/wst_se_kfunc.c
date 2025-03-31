// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Westone Security Engine Chip Driver.
 *
 * Copyright (c) 2020 - 2022 Westone Information Industry INC.
 * Copyright (c) 2021 - 2022 Loongson Technology Corporation Limited
 * Copyright (c) 2025 WangYuli <wangyuli@uniontech.com>
 */

/*
 * SE ENCRYPT CHIP Driver
 *
 * Written By: dcm, WESTONE Corporation
 *
 * Copyright (C) 2020 04 WESTONE Corp
 *
 * All rights reserved.
 */

#include <linux/aio.h>
#include <linux/uio.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/pagemap.h>
#include "wst_se_kfunc.h"
#include "wst_se_ktrans.h"

#define SE_ACK_HEADER 16

static void se_trans_callback(void *data)
{
	unsigned short *pussendbuf;
	unsigned char *pucsendbuf;
	struct tag_USER_TRANS_PACKET *pusertranspacket = NULL;
	int iret = WST_SE_OK;

	if (data == NULL) {
		return;
	}
	pusertranspacket = (struct tag_USER_TRANS_PACKET *)data;
	pussendbuf =
		(unsigned short *)pusertranspacket->ptranspacket->pucOutbuf;
	pucsendbuf = (unsigned char *)pussendbuf;
	if (*(pusertranspacket->ptranspacket->piErrCode) != 0) {
		goto Exit;
	}
	if (pussendbuf[3] != 0x0006) {
		iret = BASE_ERROR + pussendbuf[3];
		*(pusertranspacket->ptranspacket->piErrCode) = iret;
		goto Exit;
	}
	if (pusertranspacket->ptranspacket->usOutputLen) {
		memcpy(pusertranspacket->pucOutbuf, pucsendbuf + SE_ACK_HEADER,
		       pusertranspacket->ptranspacket->usOutputLen -
			       SE_ACK_HEADER);
	}
	*(pusertranspacket->ptranspacket->piErrCode) = WST_SE_OK;
Exit:
	pusertranspacket->pcallback(pusertranspacket->pParma);
	if (pusertranspacket->ptranspacket->pdmabufctl)
		se_free_dma_buf(pusertranspacket->ptranspacket->pdmabufctl);
	kfree(pusertranspacket->ptranspacket);
	kfree(pusertranspacket);
	return;
}

static int se_OpertypeValid(int iOperatetype)
{
	switch (iOperatetype) {
	case WST_SE_SM4_ECB_SINGLEENC:
	case WST_SE_SM4_ECB_SINGLEDEC:
	case WST_SE_SM4_CBC_SINGLEENC:
	case WST_SE_SM4_CBC_SINGLEDEC:
	case WST_SE_SM3_HASH_SINGLE:
	case WST_SE_SM3_HMAC_SINGLE:
	case WST_SE_COMMUNICATE:
		break;
	default:
		return WST_SE_ERROR_OPERTYPE;
	}
	return WST_SE_OK;
}

static int se_setalgo(int iOperatetype, unsigned char *pucsendbuf,
		      int iInbuflen, int iValidTotallen,
		      unsigned char *pucKeybuf, int iKeylen, int iKeyid)
{
	unsigned short *pussendbuf = (unsigned short *)pucsendbuf;
	int ivalue;

	pucsendbuf[59] = 0x40;
	ivalue = iOperatetype & 0x00000f00;
	if (ivalue == 0x0300) {
		if (iKeyid >= 0) {
			pussendbuf[10] = iKeyid;
		}
		pussendbuf[0] = 0x0014;
		if ((iOperatetype & 0x000f0000) == 0x00000000)
			pussendbuf[7] = 0x0001;
		else if ((iOperatetype & 0x000f0000) == 0x00010000)
			pussendbuf[7] = 0x0003;
		else if ((iOperatetype & 0x000f0000) == 0x00020000)
			pussendbuf[7] = 0x0003;
		else if ((iOperatetype & 0x000f0000) == 0x00030000)
			pussendbuf[7] = 0x0003;
	} else

		return WST_SE_NOT_SUPPORT;
	switch (iOperatetype & 0x0f000000) {
	case 0x00000000:
		pussendbuf[19] = 0x0010;
		pussendbuf[25] = 0x0000;
		pussendbuf[26] = 0x0803;
		pussendbuf[27] = 0x0103;
		pussendbuf[28] = 0x0100;
		if ((iOperatetype & 0x000f0000) == 0x0) {
			pussendbuf[17] = 0x0000;
			if ((iOperatetype & 0x0000f000) == 0x0) {
				pussendbuf[24] = 0x6801;
			} else {
				pussendbuf[24] = 0x6802;
				pussendbuf[30] = 0;
				if ((iOperatetype & 0x00000f00) == 0x0200)
					pussendbuf[30] = iInbuflen * 8;
			}
		} else {
			pussendbuf[17] = 0x0010;
			pussendbuf[20] = 0x0010;
			if ((iOperatetype & 0x0000f000) == 0x0) {
				pussendbuf[24] = 0x6803;
			} else {
				pussendbuf[24] = 0x6804;
				pussendbuf[30] = 0;
			}
		}
		break;
	case 0x01000000:

		if ((iOperatetype & 0x000f0000) == 0x00020000) {
			pussendbuf[19] = 0x0010;
			pussendbuf[26] = 0x0803;
			pussendbuf[27] = 0x0107;
			pussendbuf[28] = 0x0100;
		} else if ((iOperatetype & 0x000f0000) == 0x00030000) {
			pussendbuf[19] = 0x0040;
			pussendbuf[26] = 0x080f;
			pussendbuf[27] = 0x0100;
			pussendbuf[28] = 0x0100;
		} else {
			pussendbuf[19] = 0x0020;
			pussendbuf[28] = 0x0101;
		}
		pussendbuf[20] = 0x0020;
		if ((iOperatetype & 0x000f0000) == 0x00030000)
			pussendbuf[20] = 0x0000;
		pussendbuf[25] = 0x00c1;
		pucsendbuf[59] = 0x50;
		if (iValidTotallen % 64)
			pussendbuf[30] = (iValidTotallen % 64) * 8;
		else
			pussendbuf[30] = 0x0200;
		if ((iOperatetype & 0x000f0000) == 0x0) {
			pussendbuf[17] = 0x0000;
		} else {
			pussendbuf[17] = 0x0020;
		}
		if ((iOperatetype & 0x00000f00) == 0x0100)
			pussendbuf[24] = 0xe800;
		else
			pussendbuf[24] = 0xe801;
		if ((iOperatetype & 0x00000f00) == 0x0300)
			pucsendbuf[59] = 0x50;
		if ((iOperatetype & 0x000f0000) == 0x00020000) {
			switch ((iOperatetype & 0x00000f00)) {
			case 0x0300:
				pussendbuf[24] = 0xe800;
				break;
			default:
				return WST_SE_NOT_SUPPORT;
			}
			pucsendbuf[59] = 0x50;
		}
		if ((iOperatetype & 0x000f0000) == 0x00030000) {
			pussendbuf[24] = 0xe804;
			pucsendbuf[59] = 0x50;
		}
		break;
	default:
		return WST_SE_NOT_SUPPORT;
	}
	return WST_SE_OK;
}

void se_swap8(unsigned char *inbuf, int inlen)
{
	int j = 0;
	unsigned char uctmp;
	for (j = 0; j < inlen / 8; j++) {
		uctmp = inbuf[j * 8];
		inbuf[j * 8] = inbuf[j * 8 + 7];
		inbuf[j * 8 + 7] = uctmp;
		uctmp = inbuf[j * 8 + 1];
		inbuf[j * 8 + 1] = inbuf[j * 8 + 6];
		inbuf[j * 8 + 6] = uctmp;
		uctmp = inbuf[j * 8 + 2];
		inbuf[j * 8 + 2] = inbuf[j * 8 + 5];
		inbuf[j * 8 + 5] = uctmp;
		uctmp = inbuf[j * 8 + 3];
		inbuf[j * 8 + 3] = inbuf[j * 8 + 4];
		inbuf[j * 8 + 4] = uctmp;
	}
	return;
}

static int se_symtrans(int iOperatetype, unsigned char *pucDatain, int iInlen,
		       int iValidTotallen, unsigned char *pucDataout,
		       int iOutlen, unsigned char *pucKeybuf, int iKeylen,
		       int iKeyid, unsigned char *pucIvbuf, int iIvlen,
		       int iLicenseid, unsigned char *pucLicensebuf,
		       int iLicenselen, int *piErrCode, PSECallBackfn pcallback,
		       void *pParma)
{
	int ilen, icmdlen, isendoutlen, iSenddatalen, iret = WST_SE_OK;
	struct tag_TRANS_PACKET *ptranspacket = NULL;
	struct tag_USER_TRANS_PACKET *pusertranspacket = NULL;
	unsigned char *pucsendbuf = NULL;
	unsigned short *pussendbuf = NULL;
	struct tag_dma_buf_ctl *pdmabufctl = NULL;
	int irandomlen = 0;
	unsigned char ucrandom[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				       0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				       0x00, 0x00, 0x00, 0x00 };
	pdmabufctl = se_get_dma_buf(1);
	if (!pdmabufctl)
		return WST_SE_ERROR_MALLOC;
	pucsendbuf = pdmabufctl->pDmaBuf;
	*(int *)ucrandom = iValidTotallen * 8;
	se_swap8(ucrandom, 8);
	ptranspacket = (struct tag_TRANS_PACKET *)kmalloc(
		sizeof(struct tag_TRANS_PACKET), GFP_KERNEL);
	if (!ptranspacket) {
		iret = WST_SE_ERROR_MALLOC;
		goto Exit;
	}
	pusertranspacket = (struct tag_USER_TRANS_PACKET *)kmalloc(
		sizeof(struct tag_USER_TRANS_PACKET), GFP_KERNEL);
	if (!pusertranspacket) {
		iret = WST_SE_ERROR_MALLOC;
		goto Exit;
	}
	pussendbuf = (unsigned short *)pucsendbuf;
	ilen = sizeof(struct tag_TRANS_PACKET);
	icmdlen = 0x40;
	memset(pucsendbuf, 0x00, icmdlen);
	iSenddatalen = icmdlen;
	if (iKeyid != -1)
		iKeylen = 0;

	pussendbuf[1] = 0x0040;

	if (iOperatetype == WST_SE_COMMUNICATE) {
		pussendbuf[0] = 0x0115;
		pussendbuf[2] = iInlen;
		pussendbuf[7] = 0x0001;
		pussendbuf[15] = iInlen;
		memcpy(pucsendbuf + 64, pucDatain, iInlen);
		pussendbuf[18] = iOutlen;
		iSenddatalen = 64 + iInlen;
	} else {
		if ((iOperatetype & 0x0f000000) == 0x0) {
			irandomlen = 0;
			pussendbuf[4] = 0x0001;
			pussendbuf[2] =
				0 + iLicenselen + iKeylen + iIvlen + iInlen;
		} else if (((iOperatetype & 0x0f0f0000) == 0x01000000) ||
			   ((iOperatetype & 0x0f0f0000) == 0x01010000)) {
			if ((iOperatetype & 0x00000f00) != 0x000) {
				pussendbuf[6] |= 0x0020;
				pussendbuf[21] = 8;
				irandomlen = 16;
			}
			pussendbuf[4] = 0x0000;
			pussendbuf[2] = 0 + iLicenselen + iKeylen + iIvlen +
					iInlen + irandomlen;
		} else if (((iOperatetype & 0x0f0f0000) == 0x01020000) ||
			   ((iOperatetype & 0x0f0f0000) == 0x01030000)) {
			pussendbuf[4] = 0x0000;
			pussendbuf[2] = 0 + iLicenselen + iKeylen + iIvlen +
					iInlen + irandomlen;
		}

		if (iKeylen > 0) {
			pussendbuf[4] |= 0x0004;
		}
		if (iLicenseid >= 0)
			pussendbuf[4] |= 0x0002;
		if (iIvlen > 0)
			pussendbuf[4] |= 0x0008;
		if (iInlen > 0)
			pussendbuf[4] |= 0x0040;
		if (irandomlen > 0)
			pussendbuf[4] |= 0x0010;

		if (((iOperatetype & 0x000f0000) == 0x00010000) ||
		    ((iOperatetype & 0x000f0000) == 0x00020000)) {
			if (iIvlen > 0)
				pussendbuf[6] |= 0x0008;
			else
				pussendbuf[6] |= 0x0004;
		}
		if (iKeyid >= 0) {
			pussendbuf[6] |= 0x0001;
		} else {
			if (iKeylen > 0) {
				pussendbuf[6] |= 0x0002;
			}
		}
		if (iLicenseid >= 0)
			pussendbuf[8] = iLicenseid;
		pussendbuf[9] = 0;
		iret = se_setalgo(iOperatetype, pucsendbuf, iInlen,
				  iValidTotallen, pucKeybuf, iKeylen, iKeyid);
		if (iret != WST_SE_OK) {
			goto Exit;
		}
		if (iLicenselen) {
			pussendbuf[16] |= iLicenselen;
			memcpy(pucsendbuf + iSenddatalen, pucLicensebuf,
			       iLicenselen);
			iSenddatalen += iLicenselen;
		}
		if (iKeylen) {
			memcpy(pucsendbuf + iSenddatalen, pucKeybuf, iKeylen);
			iSenddatalen += iKeylen;
		}
		if (iIvlen) {
			memcpy(pucsendbuf + iSenddatalen, pucIvbuf, iIvlen);
			iSenddatalen += iIvlen;
		}
		if ((iOperatetype & 0x0f000000) == 0x01000000) {
			memcpy(pucsendbuf + iSenddatalen, ucrandom, irandomlen);
			iSenddatalen += irandomlen;
		}
		if (iInlen) {
			pussendbuf[15] = iInlen;
			memcpy(pucsendbuf + iSenddatalen, pucDatain, iInlen);
			iSenddatalen += iInlen;
		}

		if (iOutlen > 0)
			pussendbuf[18] = iOutlen;
		if (((iOperatetype & 0x0f000f00) == 0x01000100) ||
		    ((iOperatetype & 0x0f000f00) == 0x01000200) ||
		    ((iOperatetype & 0x0f000f00) == 0x01000300))
			pussendbuf[18] = 32;
		if (((iOperatetype & 0x0f0f0000) == 0x01020000))
			pussendbuf[18] = 32;
		if (((iOperatetype & 0x0f0f0000) == 0x01030000))
			pussendbuf[18] = 32;
	}
	isendoutlen = SE_ACK_HEADER + iOutlen;
	pusertranspacket->pcallback = pcallback;
	pusertranspacket->pParma = pParma;
	pusertranspacket->ptranspacket = ptranspacket;
	pusertranspacket->pucOutbuf = pucDataout;
	ptranspacket->pcallback = se_trans_callback;
	ptranspacket->pParma = pusertranspacket;
	ptranspacket->piErrCode = piErrCode;
	ptranspacket->usFlags = WST_GO_CHANNEL0;
	ptranspacket->usInputLen = iSenddatalen;
	ptranspacket->usOutputLen = (unsigned short)isendoutlen;
	ptranspacket->pucInbuf = pucsendbuf;
	ptranspacket->pucOutbuf = pucsendbuf;
	ptranspacket->pdmabufctl = pdmabufctl;

	*piErrCode = 0;
	iret = se_kernelwrite(ptranspacket->pucInbuf, ptranspacket->usInputLen,
			      ptranspacket->pucOutbuf,
			      &(ptranspacket->usOutputLen),
			      (unsigned char)(ptranspacket->usFlags),
			      (unsigned char *)ptranspacket->piErrCode,
			      ptranspacket->pcallback, ptranspacket->pParma);
	if (iret == WST_SE_OK) {
		return iret;
	}
Exit:
	if (pdmabufctl)
		se_free_dma_buf(pdmabufctl);
	if (ptranspacket)
		kfree(ptranspacket);
	if (pusertranspacket)
		kfree(pusertranspacket);
	return iret;
}

int wst_se_symtrans(int iOperatetype, unsigned char *pucDatain, int iInlen,
		    int iValidTotallen, unsigned char *pucDataout, int iOutlen,
		    unsigned char *pucKeybuf, int iKeylen, int iKeyid,
		    unsigned char *pucIvbuf, int iIvlen, int iLicenseid,
		    unsigned char *pucLicensebuf, int iLicenselen,
		    int *piErrCode, PSECallBackfn pcallback, void *pParma)
{
	int iret = WST_SE_OK;
	if ((iInlen > WST_SE_MAX_LEN) || (iOutlen > WST_SE_MAX_LEN) ||
	    (iInlen % 16))
		return WST_SE_ERROR_LENGTH;
	iret = se_OpertypeValid(iOperatetype);
	if (iret != WST_SE_OK)
		return iret;
	return se_symtrans(iOperatetype, pucDatain, iInlen, iValidTotallen,
			   pucDataout, iOutlen, pucKeybuf, iKeylen, iKeyid,
			   pucIvbuf, iIvlen, iLicenseid, pucLicensebuf,
			   iLicenselen, piErrCode, pcallback, pParma);
}

static int se_asymtrans(int iOperatetype, unsigned char *pucPkbuf, int iPklen,
			int iPkid, unsigned char *pucSessionPkbuf,
			int iSessionPklen, unsigned char *pucSkbuf, int iSklen,
			int iSkid, unsigned char *pucDatain, int iInlen,
			unsigned char *pRandom, int iRandomlen,
			unsigned char *pucDataout, int iOutlen, int iLicenseid,
			unsigned char *pucLicensebuf, int iLicenselen,
			int *piErrCode, PSECallBackfn pcallback, void *pParma)
{
	int ilen, icmdlen, isendoutlen, iSenddatalen, iret = WST_SE_OK;
	struct tag_TRANS_PACKET *ptranspacket = NULL;
	struct tag_USER_TRANS_PACKET *pusertranspacket = NULL;
	unsigned char *pucsendbuf = NULL;
	unsigned short *pussendbuf = NULL;
	struct tag_dma_buf_ctl *pdmabufctl = NULL;
	pdmabufctl = se_get_dma_buf(1);
	if (!pdmabufctl)
		return WST_SE_ERROR_MALLOC;
	pucsendbuf = pdmabufctl->pDmaBuf;
	pussendbuf = (unsigned short *)pucsendbuf;
	ptranspacket = (struct tag_TRANS_PACKET *)kmalloc(
		sizeof(struct tag_TRANS_PACKET), GFP_KERNEL);
	if (!ptranspacket) {
		iret = WST_SE_ERROR_MALLOC;
		goto Exit;
	}
	pusertranspacket = (struct tag_USER_TRANS_PACKET *)kmalloc(
		sizeof(struct tag_USER_TRANS_PACKET), GFP_KERNEL);
	if (!pusertranspacket) {
		iret = WST_SE_ERROR_MALLOC;
		goto Exit;
	}
	ilen = sizeof(struct tag_TRANS_PACKET);
	icmdlen = 0x40;
	memset(pucsendbuf, 0x00, icmdlen);
	iSenddatalen = icmdlen;
	pussendbuf[0] = 0x0114;
	pussendbuf[1] = 0x0040;
	if (iLicenseid >= 0) {
		pussendbuf[4] |= 0x0002;
	}
	pussendbuf[7] = 0x0001;
	if (iLicenseid >= 0)
		pussendbuf[8] = iLicenseid;
	pussendbuf[9] = 0;
	pucsendbuf[48] = 0x08;
	pucsendbuf[49] = 0x09;
	pucsendbuf[50] = 0x0a;
	pucsendbuf[51] = 0x0b;
	switch (iOperatetype) {
	case WST_SE_SM2_ENC:
		pussendbuf[2] = iPklen + iInlen + iRandomlen;
		if (iPkid >= 0) {
			pussendbuf[4] |= 0x0018;
			pussendbuf[5] |= 0x0029;
			pussendbuf[10] = iPkid;
		} else {
			pussendbuf[4] |= 0x001c;
			pussendbuf[5] |= 0x002a;
		}
		pucsendbuf[63] = 0x04;
		break;
	case WST_SE_SM2_DEC:

		pussendbuf[2] = iSklen + iInlen + iRandomlen;
		if (iSkid >= 0) {
			pussendbuf[4] |= 0x002c;
			pussendbuf[5] |= 0x009a;
			pussendbuf[10] = iSkid;
		} else {
			pussendbuf[4] |= 0x003c;
			pussendbuf[5] |= 0x00aa;
		}
		pucsendbuf[52] = 0x0c;
		pucsendbuf[63] = 0x0c;
		break;
	case WST_SE_SM2_SING:
		pussendbuf[2] = iSklen + iInlen + iRandomlen;
		if (iSkid >= 0) {
			pussendbuf[4] |= 0x0018;
			pussendbuf[5] |= 0x0029;
			pussendbuf[10] = iSkid;
		} else {
			pussendbuf[4] |= 0x001c;
			pussendbuf[5] |= 0x002a;
		}
		pucsendbuf[60] = 0x04;
		break;
	case WST_SE_SM2_VERIFY:
		pussendbuf[2] = iPklen + iInlen + iRandomlen;
		if (iPkid >= 0) {
			pussendbuf[4] |= 0x0070;
			pussendbuf[5] |= 0x02a5;
			pussendbuf[10] = iPkid;
			pussendbuf[11] = iPkid + 800;
		} else {
			pussendbuf[4] |= 0x007c;
			pussendbuf[5] |= 0x02aa;
		}
		pucsendbuf[52] = 0x0c;
		pucsendbuf[53] = 0x0d;
		pucsendbuf[60] = 0x0c;
		break;
	}
	if (iLicenselen > 0) {
		pussendbuf[16] = iLicenselen;
		memcpy(pucsendbuf + iSenddatalen, pucLicensebuf, iLicenselen);
		iSenddatalen += iLicenselen;
	}
	pussendbuf[17] = 0x0020;
	pussendbuf[18] = iOutlen;
	if (iPklen > 0) {
		memcpy(pucsendbuf + iSenddatalen, pucPkbuf, iPklen);
		iSenddatalen += iPklen;
	}
	if (iSessionPklen > 0) {
		memcpy(pucsendbuf + iSenddatalen, pucSessionPkbuf,
		       iSessionPklen);
		iSenddatalen += iSessionPklen;
	}
	if (iSklen > 0) {
		memcpy(pucsendbuf + iSenddatalen, pucSkbuf, iSklen);
		iSenddatalen += iSklen;
	}
	if (iInlen > 0) {
		memcpy(pucsendbuf + iSenddatalen, pucDatain, iInlen);
		iSenddatalen += iInlen;
	}
	if (iRandomlen > 0) {
		memcpy(pucsendbuf + iSenddatalen, pRandom, iRandomlen);
		iSenddatalen += iRandomlen;
	}
	isendoutlen = SE_ACK_HEADER + iOutlen;
	pusertranspacket->pcallback = pcallback;
	pusertranspacket->pParma = pParma;
	pusertranspacket->ptranspacket = ptranspacket;
	pusertranspacket->pucOutbuf = pucDataout;
	ptranspacket->pcallback = se_trans_callback;
	ptranspacket->pParma = pusertranspacket;
	ptranspacket->piErrCode = piErrCode;
	ptranspacket->usFlags = WST_GO_CHANNEL1;
	ptranspacket->usInputLen = iSenddatalen;
	ptranspacket->usOutputLen = (unsigned short)isendoutlen;
	ptranspacket->pucInbuf = pucsendbuf;
	ptranspacket->pucOutbuf = pucsendbuf;
	ptranspacket->pdmabufctl = pdmabufctl;

	*piErrCode = 0;
	iret = se_kernelwrite(ptranspacket->pucInbuf, ptranspacket->usInputLen,
			      ptranspacket->pucOutbuf,
			      &(ptranspacket->usOutputLen),
			      (unsigned char)(ptranspacket->usFlags),
			      (unsigned char *)ptranspacket->piErrCode,
			      ptranspacket->pcallback, ptranspacket->pParma);
	if (iret == WST_SE_OK) {
		return iret;
	}
Exit:
	if (pdmabufctl)
		se_free_dma_buf(pdmabufctl);
	if (ptranspacket)
		kfree(ptranspacket);
	if (pusertranspacket)
		kfree(pusertranspacket);
	return iret;
}

int wst_se_asymtrans(int iOperatetype, unsigned char *pucPkbuf, int iPklen,
		     int iPkid, unsigned char *pucSessionPkbuf,
		     int iSessionPklen, unsigned char *pucSkbuf, int iSklen,
		     int iSkid, unsigned char *pucDatain, int iInlen,
		     unsigned char *pRandom, int iRandomlen,
		     unsigned char *pucDataout, int iOutlen, int iLicenseid,
		     unsigned char *pucLicensebuf, int iLicenselen,
		     int *piErrCode, PSECallBackfn pcallback, void *pParma)
{
	if ((iInlen > WST_SE_MAX_LEN) || (iOutlen > WST_SE_MAX_LEN))
		return WST_SE_ERROR_LENGTH;
	switch (iOperatetype) {
	case WST_SE_SM2_SING: {
		if ((iInlen != 32) || (iOutlen != 68))
			return WST_SE_ERROR_LENGTH;
	} break;
	case WST_SE_SM2_VERIFY: {
		if ((iInlen != 96) || (iOutlen != 4))
			return WST_SE_ERROR_LENGTH;
	} break;
	default:
		return WST_SE_ERROR_OPERTYPE;
	}
	return se_asymtrans(iOperatetype, pucPkbuf, iPklen, iPkid,
			    pucSessionPkbuf, iSessionPklen, pucSkbuf, iSklen,
			    iSkid, pucDatain, iInlen, pRandom, iRandomlen,
			    pucDataout, iOutlen, iLicenseid, pucLicensebuf,
			    iLicenselen, piErrCode, pcallback, pParma);
}

int wst_se_810_trans(unsigned char *pucDatain, int iInlen,
		     unsigned char *pucDataout, int iOutlen)
{
	int iret;
	int ierrcode;
	struct tag_TRANS_PACKET transpacket;

	transpacket.pcallback = NULL;
	transpacket.pParma = NULL;
	transpacket.piErrCode = &ierrcode;
	transpacket.usFlags = WST_GO_CHANNEL2;
	transpacket.usInputLen = iInlen;
	transpacket.usOutputLen = (unsigned short)iOutlen;
	transpacket.pucInbuf = pucDatain;
	transpacket.pucOutbuf = pucDataout;
	iret = se_kernelwrite(transpacket.pucInbuf, transpacket.usInputLen,
			      transpacket.pucOutbuf, &(transpacket.usOutputLen),
			      WST_GO_CHANNEL2, NULL, NULL, NULL);
	return iret;
}

EXPORT_SYMBOL(wst_se_symtrans);
EXPORT_SYMBOL(wst_se_asymtrans);
EXPORT_SYMBOL(wst_se_810_trans);
