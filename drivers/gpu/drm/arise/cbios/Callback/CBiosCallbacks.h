//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************


/*****************************************************************************
** DESCRIPTION:
** CBios hw independent callback function prototype.
**
** NOTE:
** The hw dependent callback function SHOULD NOT be added to this file.
******************************************************************************/

#ifndef _CBIOS_CALLBACKS_H_
#define _CBIOS_CALLBACKS_H_

#include "../Device/CBiosShare.h"

#define CBIOS_DBG_PRINT 1

#if CBIOS_DBG_PRINT
#ifdef __LINUX__
#define cbDebugPrint1(DebugCtlFlag, DebugMessage, args...) \
    do{ \
        CBIOS_UCHAR* pDbgBuff = cbGetDebugBuffer(DebugCtlFlag); \
        if(pDbgBuff != CBIOS_NULL && cbVsnprintf != CBIOS_NULL) \
        { \
            CBIOS_U32  preLen = cbAddPrefix(DebugCtlFlag, pDbgBuff); \
            cbVsnprintf(pDbgBuff + preLen, CBIOSDEBUGMESSAGEMAXBYTES-preLen-1, DebugMessage, ##args); \
            cbPrintWithDbgFlag(DebugCtlFlag, pDbgBuff);\
        } \
    }while(0)
#define cbDebugPrint(arg) cbDebugPrint1 arg
#else 
#define cbDebugPrint(arg) cbPrintMessage arg
#endif
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
#ifdef  __LINUX__
typedef CBIOS_S32   (*CALLBACK_cbVsprintf)(PCBIOS_UCHAR buf, PCBIOS_CHAR fmt, ...);
typedef CBIOS_S32   (*CALLBACK_cbVsnprintf)(PCBIOS_UCHAR buf, CBIOS_U32 size, PCBIOS_CHAR fmt, ...);
#else
typedef CBIOS_S32     (*CALLBACK_cbVsprintf)(PCBIOS_UCHAR buf, PCBIOS_CHAR fmt, va_list  args);
typedef CBIOS_S32     (*CALLBACK_cbVsnprintf)(PCBIOS_UCHAR buf, CBIOS_U32 size, PCBIOS_CHAR fmt, va_list args);
#endif

extern CALLBACK_cbVsprintf                  cbVsprintf;
extern CALLBACK_cbVsnprintf                 cbVsnprintf;

CBIOS_STATUS cbSetCallBackFunctions(PCBIOS_CALLBACK_FUNCTIONS pFnCallBack);

//******** Debug Print functions *******************************
CBIOS_VOID cb_DbgPrint(CBIOS_U32 DebugPrintLevel, PCBIOS_UCHAR DebugMessage);

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
