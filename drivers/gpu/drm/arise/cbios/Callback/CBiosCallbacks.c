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


#include "CBiosCallbacks.h"
#include "../Device/CBiosChipShare.h"

CALLBACK_cbVDbgPrint              cbVDbgPrint = CBIOS_NULL;

CBIOS_CALLBACK_FUNCTIONS FnCallback = {0};

CBIOS_STATUS cbSetCallBackFunctions(PCBIOS_CALLBACK_FUNCTIONS pFnCallBack)
{
    if(pFnCallBack == CBIOS_NULL)
    {
        return CBIOS_ER_INVALID_PARAMETER;
    }

#ifndef UEFI_DIAGTOOL
    if((pFnCallBack->pFnDelayMicroSeconds        == CBIOS_NULL)||
       (pFnCallBack->pFnQuerySystemTime          == CBIOS_NULL)||
       (pFnCallBack->pFnAllocateNonpagedMemory   == CBIOS_NULL)||
       (pFnCallBack->pFnAllocatePagedMemory       == CBIOS_NULL)||
       (pFnCallBack->pFnFreePool                 == CBIOS_NULL)||
       (pFnCallBack->pFnAcquireSpinLock          == CBIOS_NULL)||
       (pFnCallBack->pFnReleaseSpinLock          == CBIOS_NULL))
#else
    if((pFnCallBack->pFnDelayMicroSeconds        == CBIOS_NULL)||
       (pFnCallBack->pFnAllocateNonpagedMemory   == CBIOS_NULL)||
       (pFnCallBack->pFnAllocatePagedMemory       == CBIOS_NULL)||
       (pFnCallBack->pFnFreePool                 == CBIOS_NULL))
#endif
    {
        return CBIOS_ER_INVALID_PARAMETER;
    }

    // remove PrintToFile function for android 4.4 resume hang issue

    pFnCallBack->pFnDbgPrintToFile = CBIOS_NULL;

    cbVDbgPrint = pFnCallBack->pFnVDbgPrint;

    //FnCallback = *pFnCallBack;
    FnCallback.Size=pFnCallBack->Size;
    FnCallback.pFnDelayMicroSeconds=pFnCallBack->pFnDelayMicroSeconds;
    FnCallback.pFnReadUchar=pFnCallBack->pFnReadUchar;
    FnCallback.pFnReadUshort=pFnCallBack->pFnReadUshort;
    FnCallback.pFnReadUlong=pFnCallBack->pFnReadUlong;
    FnCallback.pFnWriteUchar=pFnCallBack->pFnWriteUchar;
    FnCallback.pFnWriteUshort=pFnCallBack->pFnWriteUshort;
    FnCallback.pFnWriteUlong=pFnCallBack->pFnWriteUlong;
    FnCallback.pFnQuerySystemTime=pFnCallBack->pFnQuerySystemTime;
    FnCallback.pFnAllocateNonpagedMemory=pFnCallBack->pFnAllocateNonpagedMemory;
    FnCallback.pFnAllocatePagedMemory=pFnCallBack->pFnAllocatePagedMemory;
    FnCallback.pFnFreePool=pFnCallBack->pFnFreePool;
    FnCallback.pFnAcquireSpinLock=pFnCallBack->pFnAcquireSpinLock;
    FnCallback.pFnReleaseSpinLock=pFnCallBack->pFnReleaseSpinLock;
    FnCallback.pFnAcquireMutex=pFnCallBack->pFnAcquireMutex;
    FnCallback.pFnReleaseMutex=pFnCallBack->pFnReleaseMutex;
    FnCallback.pFnReadPortUchar=pFnCallBack->pFnReadPortUchar;
    FnCallback.pFnWritePortUchar=pFnCallBack->pFnWritePortUchar;
    FnCallback.pFnGetRegistryParameters=pFnCallBack->pFnGetRegistryParameters;
    FnCallback.pFnSetRegistryParameters=pFnCallBack->pFnSetRegistryParameters;
    FnCallback.pFnStrcmp=pFnCallBack->pFnStrcmp;
    FnCallback.pFnStrcpy=pFnCallBack->pFnStrcpy;
    FnCallback.pFnStrncmp=pFnCallBack->pFnStrncmp;
    FnCallback.pFnMemset=pFnCallBack->pFnMemset;
    FnCallback.pFnMemcpy=pFnCallBack->pFnMemcpy;
    FnCallback.pFnMemcmp=pFnCallBack->pFnMemcmp;
    FnCallback.pFnDodiv=pFnCallBack->pFnDodiv;
    FnCallback.pFnVsprintf=pFnCallBack->pFnVsprintf;
    FnCallback.pFnWriteRegisterU32=pFnCallBack->pFnWriteRegisterU32;
    FnCallback.pFnReadRegisterU32=pFnCallBack->pFnReadRegisterU32;
    FnCallback.pFnDbgPrintToFile=pFnCallBack->pFnDbgPrintToFile;
    FnCallback.pFnVsnprintf=pFnCallBack->pFnVsnprintf;

    FnCallback.pFnGpioGetValue=pFnCallBack->pFnGpioGetValue;
    FnCallback.pFnGpioSetValue=pFnCallBack->pFnGpioSetValue;
    FnCallback.pFnGpioRequest=pFnCallBack->pFnGpioRequest;
    FnCallback.pFnGpioFree=pFnCallBack->pFnGpioFree;
    FnCallback.pFnGpioDirectionInput=pFnCallBack->pFnGpioDirectionInput;
    FnCallback.pFnGpioDirectionOutput=pFnCallBack->pFnGpioDirectionOutput;
    FnCallback.pFnGetPlatformConfigU32=pFnCallBack->pFnGetPlatformConfigU32;

    FnCallback.pFnRegulatorGet=pFnCallBack->pFnRegulatorGet;
    FnCallback.pFnRegulatorEnable=pFnCallBack->pFnRegulatorEnable;
    FnCallback.pFnRegulatorDisable=pFnCallBack->pFnRegulatorDisable;
    FnCallback.pFnRegulatorIsEnabled=pFnCallBack->pFnRegulatorIsEnabled;
    FnCallback.pFnRegulatorGetVoltage=pFnCallBack->pFnRegulatorGetVoltage;
    FnCallback.pFnRegulatorSetVoltage=pFnCallBack->pFnRegulatorSetVoltage;
    FnCallback.pFnRegulatorPut=pFnCallBack->pFnRegulatorPut;
    FnCallback.pFnVDbgPrint = pFnCallBack->pFnVDbgPrint;
    return CBIOS_OK;
}


CBIOS_VOID cb_DelayMicroSeconds(CBIOS_U32 Microseconds)
{
    if (FnCallback.pFnDelayMicroSeconds != CBIOS_NULL)
    {
        ((CALLBACK_cbDelayMicroSeconds)FnCallback.pFnDelayMicroSeconds)(Microseconds);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: callback not defined!\n",FUNCTION_NAME));
    }
}

CBIOS_U64 cb_QuerySystemTime(void)
{
    CBIOS_U64 time = 0;

    if (FnCallback.pFnQuerySystemTime != CBIOS_NULL)
    {
        ((CALLBACK_cbQuerySystemTime)FnCallback.pFnQuerySystemTime)(&time);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: callback not defined!\n",FUNCTION_NAME));
    }

    return time;
}

// memory allocation and free function stub
PCBIOS_VOID cb_AllocateNonpagedPool(CBIOS_U32 NumberOfBytes)
{
    PCBIOS_VOID Ret = CBIOS_NULL;

    if (FnCallback.pFnAllocateNonpagedMemory != CBIOS_NULL)
    {
        Ret = ((CALLBACK_cbAllocateNonpagedPool)FnCallback.pFnAllocateNonpagedMemory)(NumberOfBytes);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: callback not defined!\n",FUNCTION_NAME));
    }
    return Ret;
}

PCBIOS_VOID cb_AllocatePagedPool(CBIOS_U32 NumberOfBytes)
{
    PCBIOS_VOID Ret = CBIOS_NULL;

    if (FnCallback.pFnAllocatePagedMemory != CBIOS_NULL)
    {
        Ret = ((CALLBACK_cbAllocatePagedPool)FnCallback.pFnAllocatePagedMemory)(NumberOfBytes);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: callback not defined!\n",FUNCTION_NAME));
    }
    return Ret;
}

CBIOS_VOID cb_FreePool(PCBIOS_VOID pPoolMem)
{
    if (FnCallback.pFnFreePool != CBIOS_NULL)
    {
        ((CALLBACK_cbFreePool)FnCallback.pFnFreePool)(pPoolMem);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: callback func is not defined!\n", FUNCTION_NAME));
    }
}

CBIOS_U64 cb_AcquireSpinLock(PCBIOS_VOID pvSpinLock)
{
    CBIOS_U64  ret = 0;

    if (FnCallback.pFnAcquireSpinLock != CBIOS_NULL)
    {
        ret = ((CALLBACK_cbAcquireSpinLock)FnCallback.pFnAcquireSpinLock)(pvSpinLock);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: callback func is not defined!\n", FUNCTION_NAME));
    }
    return  ret;
}

CBIOS_VOID cb_ReleaseSpinLock(PCBIOS_VOID pvSpinLock, CBIOS_U64  NewIrql)
{
    if (FnCallback.pFnReleaseSpinLock != CBIOS_NULL)
    {
        ((CALLBACK_cbReleaseSpinLock)FnCallback.pFnReleaseSpinLock)(pvSpinLock, NewIrql);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: callback func is not defined!\n", FUNCTION_NAME));
    }
}

CBIOS_VOID cb_AcquireMutex(PCBIOS_VOID pvMutex)
{
    if (FnCallback.pFnAcquireMutex != CBIOS_NULL)
    {
        ((CALLBACK_cbAcquireMutex)FnCallback.pFnAcquireMutex)(pvMutex);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: callback func is not defined!\n", FUNCTION_NAME));
    }
}

CBIOS_VOID cb_ReleaseMutex(PCBIOS_VOID pvMutex)
{
    if (FnCallback.pFnReleaseMutex != CBIOS_NULL)
    {
        ((CALLBACK_cbReleaseMutex)FnCallback.pFnReleaseMutex)(pvMutex);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: callback func is not defined!\n", FUNCTION_NAME));
    }
}

CBIOS_S32 cb_GetRegistryParameters(PCBIOS_VOID pAdapterContext, PWSTR ParameterName, CBIOS_UCHAR IsParameterFileName, PCBIOS_VOID Context)
{
    CBIOS_S32 Ret = 0;

#ifndef __LINUX__
    if (FnCallback.pFnGetRegistryParameters != CBIOS_NULL)
    {
        Ret = ((CALLBACK_cbGetRegistryParameters)FnCallback.pFnGetRegistryParameters)(pAdapterContext, ParameterName,IsParameterFileName,Context);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),("cb_GetRegistryParameters is CBIOS_NULL!\n")));
    }
#endif

    return Ret;
}

CBIOS_S32 cb_SetRegistryParameters(PCBIOS_VOID pAdapterContext, PWSTR ValueName, PCBIOS_VOID ValueData, CBIOS_U32 ValueLength)
{
    CBIOS_S32 Ret = 0;

#ifndef __LINUX__
    if (FnCallback.pFnSetRegistryParameters != CBIOS_NULL)
    {
        Ret = ((CALLBACK_cbSetRegistryParameters)FnCallback.pFnSetRegistryParameters)(pAdapterContext, ValueName,ValueData,ValueLength);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),("cb_SetRegistryParameters is CBIOS_NULL!\n")));
    }
#endif

    return Ret;

}

CBIOS_S32 cb_strcmp(PCBIOS_UCHAR s1, const CBIOS_UCHAR* s2)
{
    CBIOS_S32 Ret = 0;

    if (FnCallback.pFnStrcmp != CBIOS_NULL)
    {
        //new driver has the callback function
        Ret = ((CALLBACK_cbStrCmp)FnCallback.pFnStrcmp)(s1, s2);
    }
    else
    {
        Ret = cbStrCmp(s1, s2);
    }

    return Ret;
}

PCBIOS_CHAR cb_strcpy(PCBIOS_CHAR s1, PCBIOS_CHAR s2)
{
    PCBIOS_CHAR Ret = CBIOS_NULL;

    if (FnCallback.pFnStrcpy!= CBIOS_NULL)
    {
        //new driver has the callback function
        Ret = ((CALLBACK_cbStrCpy)FnCallback.pFnStrcpy)(s1, s2);
    }
    else
    {
        Ret = cbStrCpy(s1, s2);
    }

    return Ret;
}

CBIOS_S32 cb_strncmp(PCBIOS_UCHAR s1, PCBIOS_UCHAR s2, CBIOS_U32 length)
{
    CBIOS_S32 Ret = 0;

    if (FnCallback.pFnStrncmp != CBIOS_NULL)
    {
        //new driver has the callback function
        Ret = ((CALLBACK_cbStrNCmp)FnCallback.pFnStrncmp)(s1, s2, length);
    }
    else
    {
        Ret = cbStrnCmp(s1, s2, length);
    }

    return Ret;
}

PCBIOS_VOID cb_memset(PCBIOS_VOID pBuf, CBIOS_S32 value, CBIOS_U32 length)
{
    PCBIOS_VOID Ret = CBIOS_NULL;

    if (FnCallback.pFnMemset != CBIOS_NULL)
    {
        //new driver has the callback function
        Ret = ((CALLBACK_cbMemSet)FnCallback.pFnMemset)(pBuf, value, length);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: callback func is not defined!\n", FUNCTION_NAME));
    }

    return Ret;
}

PCBIOS_VOID cb_memcpy(PCBIOS_VOID pBuf1, PCBIOS_VOID pBuf2, CBIOS_U32 length)
{
    PCBIOS_VOID Ret = CBIOS_NULL;

    if (FnCallback.pFnMemcpy!= CBIOS_NULL)
    {
        //new driver has the callback function
        Ret = ((CALLBACK_cbMemCpy)FnCallback.pFnMemcpy)(pBuf1, pBuf2, length);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: callback func is not defined!\n", FUNCTION_NAME));
    }

    return Ret;
}

CBIOS_S32 cb_memcmp(PCBIOS_VOID pBuf1, PCBIOS_VOID pBuf2, CBIOS_U32 length)
{
    CBIOS_S32 Ret = 0;

    if (FnCallback.pFnMemcmp != CBIOS_NULL)
    {
        //new driver has the callback function
        Ret = ((CALLBACK_cbMemCmp)FnCallback.pFnMemcmp)(pBuf1, pBuf2, length);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: callback func is not defined!\n", FUNCTION_NAME));
    }

    return Ret;
}


CBIOS_U64 cb_do_div(CBIOS_U64 a, CBIOS_U64 b)
{
    CBIOS_U64 Ret = 0;

    if (FnCallback.pFnDodiv != CBIOS_NULL)

    {
        //new driver has the callback function
        Ret = ((CALLBACK_cbDoDiv)FnCallback.pFnDodiv)(a, b);
    }
    else
    {
        Ret = 0;
    }

    return Ret;
}

CBIOS_VOID cbPrintMsgToFile(CBIOS_U32 DebugPrintLevel, PCBIOS_CHAR DebugMessage, PCBIOS_VOID pBuffer, CBIOS_U32 Size)
{
    if (FnCallback.pFnDbgPrintToFile != CBIOS_NULL)
    {
        ((CALLBACK_cbDbgPrintToFile)FnCallback.pFnDbgPrintToFile)(DebugPrintLevel, DebugMessage, pBuffer, Size);
    }

    return;
}

CBIOS_BOOL cbGetPlatformConfigurationU32(PCBIOS_VOID pvcbe, CBIOS_U8 *pName, CBIOS_U32 *pBuffer, CBIOS_U32 Length)
{

    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    CBIOS_BOOL bRet = CBIOS_FALSE;

    if (FnCallback.pFnGetPlatformConfigU32 != CBIOS_NULL)
    {
        bRet = ((CALLBACK_cbGetPlatformConfigU32)FnCallback.pFnGetPlatformConfigU32)(pcbe->pAdapterContext, pName, pBuffer, Length);
    }
    else
    {
        bRet = CBIOS_FALSE;
    }

    return bRet;
}

