/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef _CBIOS_CALLBACKS_H_
#define _CBIOS_CALLBACKS_H_

#include "../Device/CBiosShare.h"

#define CBIOS_DBG_PRINT 1

#if CBIOS_DBG_PRINT
#define cbDebugPrint(arg)  \
do{ \
    if(cbVDbgPrint != CBIOS_NULL)  \
    { \
        cbVDbgPrint arg;   \
    }   \
}while(0)
#else /*else DBG*/
#define cbDebugPrint(arg)
#endif

#ifndef WCHAR
typedef unsigned short WCHAR;    // wc,   16-bit UNICODE character
typedef WCHAR *NWPSTR, *LPWSTR, *PWSTR;
#endif

/******Call Back function *********/
//******** Debug Print functions *******************************
typedef CBIOS_VOID    (*CALLBACK_cbDbgPrint)(CBIOS_U32 DebugPrintLevel, PCBIOS_UCHAR DebugMessage);

//******** Get Registry Parameters ********************************
typedef CBIOS_S32 (*CALLBACK_cbGetRegistryParameters)(PCBIOS_VOID pAdapterContext, PWSTR ParameterName, CBIOS_UCHAR IsParameterFileName, PCBIOS_VOID Context);
typedef CBIOS_S32 (*CALLBACK_cbSetRegistryParameters)(PCBIOS_VOID pAdapterContext, PWSTR ValueName, PCBIOS_VOID ValueData, CBIOS_U32 ValueLength);

//******** time delay functions ********************************
typedef CBIOS_VOID    (*CALLBACK_cbDelayMicroSeconds)(CBIOS_U32 Microseconds);

//////////////////////////////////////////////////////////////////////////////
typedef CBIOS_VOID    (*CALLBACK_cbQuerySystemTime)(CBIOS_U64* LiTime);
/////////////////////////////////////////////////////////////////////////////
// memory allocation and free function stub
typedef PCBIOS_VOID   (*CALLBACK_cbAllocateNonpagedPool)(CBIOS_U32 NumberOfBytes);
typedef PCBIOS_VOID    (*CALLBACK_cbAllocatePagedPool)(CBIOS_U32 NumberOfBytes);
typedef CBIOS_VOID    (*CALLBACK_cbFreePool)(PCBIOS_VOID pPoolMem);
typedef CBIOS_U64    (*CALLBACK_cbAcquireSpinLock)(PCBIOS_VOID pvSpinLock);
typedef CBIOS_VOID    (*CALLBACK_cbReleaseSpinLock)(PCBIOS_VOID pvSpinLock, CBIOS_U64  IrqlStatus);
typedef CBIOS_VOID    (*CALLBACK_cbAcquireMutex)(PCBIOS_VOID pvMutex);
typedef CBIOS_VOID    (*CALLBACK_cbReleaseMutex)(PCBIOS_VOID pvMutex);

//CBIOS_VOID cbQuerySystemTime(LARGE_INTEGER* LiTime);
typedef CBIOS_S32     (*CALLBACK_cbStrCmp)(PCBIOS_UCHAR s1, const CBIOS_UCHAR* s2);
typedef PCBIOS_CHAR   (*CALLBACK_cbStrCpy)(PCBIOS_CHAR s1, PCBIOS_CHAR s2);
typedef CBIOS_S32     (*CALLBACK_cbStrNCmp)(PCBIOS_UCHAR s1, PCBIOS_UCHAR s2, CBIOS_U32 length);
typedef PCBIOS_VOID   (*CALLBACK_cbMemSet)(PCBIOS_VOID pBuf, CBIOS_S32 value, CBIOS_U32 length);
typedef PCBIOS_VOID   (*CALLBACK_cbMemCpy)(PCBIOS_VOID pBuf1, PCBIOS_VOID pBuf2, CBIOS_U32 length);
typedef CBIOS_S32     (*CALLBACK_cbMemCmp)(PCBIOS_VOID pBuf1, PCBIOS_VOID pBuf2, CBIOS_U32 length);
typedef CBIOS_U64     (*CALLBACK_cbDoDiv)(CBIOS_U64 a, CBIOS_U64 b);
typedef CBIOS_VOID  (*CALLBACK_cbVDbgPrint)(CBIOS_BOOL  bEnablePrint, CBIOS_U32 PrintLevel, PCBIOS_UCHAR PrefixStr, PCBIOS_UCHAR DbgMsg, ...);

extern CALLBACK_cbVDbgPrint              cbVDbgPrint;



CBIOS_STATUS cbSetCallBackFunctions(PCBIOS_CALLBACK_FUNCTIONS pFnCallBack);

//******** Debug Print functions *******************************


//******** time delay functions ********************************
CBIOS_VOID cb_DelayMicroSeconds(CBIOS_U32 Microseconds);

CBIOS_U64 cb_QuerySystemTime(void);

PCBIOS_VOID cb_AllocateNonpagedPool(CBIOS_U32 NumberOfBytes);
PCBIOS_VOID cb_AllocatePagedPool(CBIOS_U32 NumberOfBytes);
CBIOS_VOID cb_FreePool(PCBIOS_VOID pPoolMem);
CBIOS_U64 cb_AcquireSpinLock(PCBIOS_VOID pvSpinLock);
CBIOS_VOID cb_ReleaseSpinLock(PCBIOS_VOID pvSpinLock, CBIOS_U64  NewIrql);
CBIOS_VOID cb_AcquireMutex(PCBIOS_VOID pvMutex);
CBIOS_VOID cb_ReleaseMutex(PCBIOS_VOID pvMutex);

//******** Get Registry Parameters ********************************
CBIOS_S32 cb_GetRegistryParameters(PCBIOS_VOID pAdapterContext, PWSTR ParameterName, CBIOS_UCHAR IsParameterFileName, PCBIOS_VOID Context);
CBIOS_S32 cb_SetRegistryParameters(PCBIOS_VOID pAdapterContext, PWSTR ValueName, PCBIOS_VOID ValueData, CBIOS_U32 ValueLength);

CBIOS_S32 cb_strcmp(PCBIOS_UCHAR s1, const CBIOS_UCHAR* s2);
PCBIOS_CHAR cb_strcpy(PCBIOS_CHAR s1, PCBIOS_CHAR s2);
CBIOS_S32 cb_strncmp(PCBIOS_UCHAR s1, PCBIOS_UCHAR s2, CBIOS_U32 length);
PCBIOS_VOID cb_memset(PCBIOS_VOID pBuf, CBIOS_S32 value, CBIOS_U32 length);
PCBIOS_VOID cb_memcpy(PCBIOS_VOID pBuf1, PCBIOS_VOID pBuf2, CBIOS_U32 length);
CBIOS_S32 cb_memcmp(PCBIOS_VOID pBuf1, PCBIOS_VOID pBuf2, CBIOS_U32 length);
CBIOS_U64 cb_do_div(CBIOS_U64 a, CBIOS_U64 b);
CBIOS_VOID cbPrintMsgToFile(CBIOS_U32 DebugPrintLevel, PCBIOS_CHAR DebugMessage, PCBIOS_VOID pBuffer, CBIOS_U32 Size);
CBIOS_BOOL cbGetPlatformConfigurationU32(PCBIOS_VOID pvcbe, CBIOS_U8 *pName, CBIOS_U32 *pBuffer, CBIOS_U32 Length);

#endif
