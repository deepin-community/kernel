/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _WST_SE_KFUNC
#define _WST_SE_KFUNC

#include "wst_se_define.h"

int wst_se_symtrans(int iOperatetype, unsigned char *pucDatain, int iInlen,
		    int iValidTotallen, unsigned char *pucDataout, int iOutlen,
		    unsigned char *pucKeybuf, int iKeylen, int iKeyid,
		    unsigned char *pucIvbuf, int iIvlen, int iLicenseid,
		    unsigned char *pucLicensebuf, int iLicenselen,
		    int *piErrCode, PSECallBackfn pcallback, void *pParma);

int wst_se_asymtrans(int iOperatetype, unsigned char *pucPkbuf, int iPklen,
		     int iPkid, unsigned char *pucSessionPkbuf,
		     int iSessionPklen, unsigned char *pucSkbuf, int iSklen,
		     int iSkid, unsigned char *pucDatain, int iInlen,
		     unsigned char *pRandom, int iRandomlen,
		     unsigned char *pucDataout, int iOutlen, int iLicenseid,
		     unsigned char *pucLicensebuf, int iLicenselen,
		     int *piErrCode, PSECallBackfn pcallback, void *pParma);
int wst_se_810_trans(unsigned char *pucDatain, int iInlen,
		     unsigned char *pucDataout, int iOutlen);

#endif
