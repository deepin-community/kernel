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
** Hw independent and chip independent functions.
**
** NOTE:
** The function in this file WOULD BETTER not use CBIOS_EXTENSION_COMMON.
******************************************************************************/

#include "CBiosShare.h"
#include "CBiosChipShare.h"

#define  BACK_BUFFER_COUNT    64

static  CBIOS_U32       ModuleMask = 0;
static  CBIOS_U32       MaxDbgLevel = 2;
static  CBIOS_U32       ModuleMaskLevel = 0x3;
static  CBIOS_UCHAR*    pDebugBuf = CBIOS_NULL; 
static  CBIOS_UCHAR*    pBackBuf = CBIOS_NULL;
static  CBIOS_BOOL      bEnableBackOutput = CBIOS_FALSE;
static  CBIOS_U32       TimeStamp = 0;
static  CBIOS_U32       BackBufIndex = 0;

CBIOS_UCHAR* ModuleName[] = 
{
    (CBIOS_UCHAR*)"[DISP] General ",
    (CBIOS_UCHAR*)"[DISP] MHL ",
    (CBIOS_UCHAR*)"[DISP] DSI ",
    (CBIOS_UCHAR*)"[DISP] HDMI ",
    (CBIOS_UCHAR*)"[DISP] DP ",
};

CBIOS_UCHAR* DebugLevelName[] = 
{
    (CBIOS_UCHAR*)"Error:  ",
    (CBIOS_UCHAR*)"Warning:  ",
    (CBIOS_UCHAR*)"Info:  ",
    (CBIOS_UCHAR*)"Debug:  ",
    (CBIOS_UCHAR*)"Trace:  ",
};


#ifndef __LINUX__
CBIOS_VOID cbPrintMessage(CBIOS_U32 DebugPrintLevel, CBIOS_CHAR *DebugMessage, ...)
{
    va_list     args;
    CBIOS_UCHAR *pDebugString; 
    CBIOS_BOOL  isSkipPrint = CBIOS_TRUE;
    CBIOS_U32   preLen = 0;
  
    if(((DebugPrintLevel & 0xFF) <= MaxDbgLevel) || ((DebugPrintLevel & ModuleMask) &&((DebugPrintLevel & 0xFF) <= ModuleMaskLevel )))
    {
        isSkipPrint = CBIOS_FALSE;
    }

    if (isSkipPrint)
    {    
        return;
    }

    pDebugString = cbGetDebugBuffer(DebugPrintLevel & 0xFFFF);
 
    if(pDebugString == CBIOS_NULL)
    { 
        return;
    }
    
    preLen = cbAddPrefix(DebugPrintLevel, pDebugString);

    va_start(args, DebugMessage);

    if (cbVsprintf != CBIOS_NULL)
    {
        //new driver has the callback function
        cbVsprintf(pDebugString + preLen, DebugMessage, args);
    }
    else
    {
       
        //old driver has no callback function, then use standard lib function
        vsprintf(pDebugString + preLen, DebugMessage, args);
   
    }
    
    va_end(args);

    cb_DbgPrint((DebugPrintLevel & 0xFF), pDebugString);

    return;
}
#endif

CBIOS_VOID cbDelayMilliSeconds(CBIOS_U32  Milliseconds)
{
    cb_DelayMicroSeconds(1000*Milliseconds);
}

CBIOS_U32  cbAddPrefix(CBIOS_U32  Level, CBIOS_UCHAR*  pBuffer)
{
    CBIOS_U32 index = 0;
    pBuffer[0] = '\0';
    
#ifdef  __LINUX__
    if((Level & 0xFF000000) == BACK_OUTPUT && cbVsnprintf != CBIOS_NULL)
    {
        cbVsnprintf(pBuffer, CBIOSDEBUGMESSAGEMAXBYTES, "%d.", TimeStamp);
        TimeStamp++;
    }
#endif
    
    Level &= 0xFFFF;
    if(Level >> 8)
    {
        index = cbGetLastBitIndex(Level >> 8);
    }

    if (index < sizeofarray(ModuleName))
    {
        cbStrCat(pBuffer, ModuleName[index]);
    }

    index = Level & 0xff;
    if (index < sizeofarray(DebugLevelName))
    {
        cbStrCat(pBuffer, DebugLevelName[index]);
    }

    return cbStrLen(pBuffer);
}

CBIOS_VOID  cbPrintWithDbgFlag(CBIOS_U32  DbgFlag, CBIOS_UCHAR*  pBuffer)
{
    if((DbgFlag & 0xFF000000) == BACK_OUTPUT)
    {
        return;
    }
    
    if(((DbgFlag & 0xFF) <= MaxDbgLevel) || ((DbgFlag & ModuleMask) &&((DbgFlag & 0xFF) <= ModuleMaskLevel )))
    {
        if(pBuffer != CBIOS_NULL)
        {
            cb_DbgPrint(0, pBuffer);
        }
    }
}

CBIOS_STATUS  cbDbgLevelCtl(PCBIOS_DBG_LEVEL_CTRL  pDbgLevelCtl)
{
    CBIOS_U32  BackBufCtrl = 0;
    if(pDbgLevelCtl->bGetValue)
    {
        pDbgLevelCtl->DbgLevel = MaxDbgLevel | ModuleMask | (ModuleMaskLevel << 16);
        pDbgLevelCtl->DbgLevel |= ((bEnableBackOutput)? 1 : 0) << 24;
    }
    else
    {
        ModuleMaskLevel = (pDbgLevelCtl->DbgLevel >> 16) & 0xFF;
        ModuleMask = pDbgLevelCtl->DbgLevel & 0xFF00;
        MaxDbgLevel = pDbgLevelCtl->DbgLevel & 0xFF;
        BackBufCtrl = pDbgLevelCtl->DbgLevel >> 24;
        bEnableBackOutput = (BackBufCtrl && BackBufCtrl != 0xFF)? CBIOS_TRUE : CBIOS_FALSE;
        if(BackBufCtrl == 0xFF)
        {
            CBIOS_U32  Index = 0;
            CBIOS_UCHAR* pBuffer = CBIOS_NULL;
            if(pBackBuf)
            {
                for(Index = 0; Index < BACK_BUFFER_COUNT; Index++)
                {
                    pBuffer = pBackBuf + Index * CBIOSDEBUGMESSAGEMAXBYTES;
                    cb_DbgPrint(0, pBuffer);
                }    
                cb_FreePool(pBackBuf);
                pBackBuf = CBIOS_NULL;
            }
        }
    }
    return CBIOS_OK;
}

CBIOS_UCHAR*  cbGetDebugBuffer(CBIOS_U32  DbgFlag)
{
    CBIOS_UCHAR*  pBuffer = CBIOS_NULL;
    if((DbgFlag & 0xFF000000) == BACK_OUTPUT)
    {
        if(bEnableBackOutput)
        {
            if(pBackBuf == CBIOS_NULL)
            {
                pBackBuf = (CBIOS_UCHAR*)cb_AllocateNonpagedPool(CBIOSDEBUGMESSAGEMAXBYTES * BACK_BUFFER_COUNT);
                BackBufIndex = 0;
                TimeStamp = 0;
            }
            pBuffer = pBackBuf + BackBufIndex * CBIOSDEBUGMESSAGEMAXBYTES;
            BackBufIndex = (BackBufIndex+1) % BACK_BUFFER_COUNT;
        }
    }
    else
    {
        if(pDebugBuf == CBIOS_NULL)
        {
            pDebugBuf = (CBIOS_UCHAR*)cb_AllocateNonpagedPool(CBIOSDEBUGMESSAGEMAXBYTES);
        }
        pBuffer = pDebugBuf;
    }
    return pBuffer;
}

CBIOS_VOID  cbReleaseDebugBuffer(CBIOS_VOID)
{
    if(pDebugBuf != CBIOS_NULL)
    {
        cb_FreePool(pDebugBuf);
        pDebugBuf = CBIOS_NULL;
    }
    if(pBackBuf != CBIOS_NULL)
    {
        cb_FreePool(pBackBuf);
        pBackBuf = CBIOS_NULL;
    }
}

CBIOS_BOARD_VERSION cbGetBoardVersion(PCBIOS_VOID pvcbe)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOARD_VERSION BoardVersion = CBIOS_BOARD_VERSION_DEFAULT;

    if (cbGetPlatformConfigurationU32(pcbe, (CBIOS_UCHAR*)"board_version", (CBIOS_U32*)&BoardVersion, 1))
    {
        if (BoardVersion >= CBIOS_BOARD_VERSION_MAX)
        {
            BoardVersion = CBIOS_BOARD_VERSION_DEFAULT;
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "Invalid board version, use default version!\n"));
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "Current board version: %d\n", BoardVersion));
        }
    }
    else
    {
        BoardVersion = CBIOS_BOARD_VERSION_DEFAULT;
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "Can't get board version, use default version: %d!\n", BoardVersion));
    }

    return BoardVersion;
}


//ulLength: the number of characters after translated. 
//If current number has more bits than ulLenght, the upper bits will not be translated.
//If current number has less bits than ulLengh, this function will fill '0' to upper bits. 
//If ulLenght == 0, do not care about string length, just send out the real number of characters.
CBIOS_BOOL  cbItoA(CBIOS_U32 ulValue, CBIOS_U8 *pStr, CBIOS_U8 byRadix, CBIOS_U32 ulLength)
{
    CBIOS_U8 index[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; 
    CBIOS_U32 i = 0, j = 0;
    CBIOS_U8 ulTmp = 0;

    if (pStr == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbItoA: pStr is null!\n"));
        return CBIOS_FALSE;
    }

    if ((byRadix < 2) || (byRadix > 36))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbItoA: Invalid radix!\n"));
        return CBIOS_FALSE;
    }

    i = 0;
    do
    {
        pStr[i++] = index[ulValue % byRadix];
        ulValue /= byRadix;
    }while(ulValue && ((i < ulLength) || (ulLength == 0)));

    //Fill 0 to upper bits
    if (i < ulLength)
    {
        do
        {
            pStr[i++] = '0';
        }while(i < ulLength);
    }
    pStr[i] = '\0';


    for (j = 0; j < i / 2; j++)
    {
        ulTmp = pStr[j];
        pStr[j] = pStr[i - j - 1];
        pStr[i - j - 1] = ulTmp;
    }

    return CBIOS_TRUE;
   
    
}

CBIOS_U32   cbStrLen(CBIOS_UCHAR * pStrSrc)
{
    CBIOS_U32  strLen = 0, i = 0;
    if(pStrSrc)
    {
        while(pStrSrc[i++] != '\0')
        {
            strLen++;
        }
    }
    return strLen;
}

PCBIOS_UCHAR cbStrCat(CBIOS_UCHAR *pStrDst, CBIOS_UCHAR * pStrSrc)
{
    CBIOS_UCHAR *pTmp = pStrDst;
    
    while (*pTmp)
    {
        pTmp++;
    }

    while (*pStrSrc)
    {
        *pTmp = *pStrSrc;
        pTmp++;
        pStrSrc++;
    }

    *pTmp = '\0';

    return pStrDst;
}

CBIOS_U32 cbRound(CBIOS_U32 Dividend, CBIOS_U32 Divisor, CBIOS_ROUND_METHOD RoundMethod)
{
    CBIOS_U32 ulRet = 0;
    CBIOS_U32 ulRemainder = 0;
    
    if (Divisor == 0)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbRound: fata error -- divisor is ZERO!!!\n"));
        return 0;
    }

    ulRemainder = Dividend % Divisor;
    ulRet = Dividend / Divisor;

    switch (RoundMethod)
    {
    case ROUND_UP: 
        if (ulRemainder != 0)
        {
            ulRet++;
        }
        break;
    case ROUND_NEAREST:
        if ((ulRemainder * 2) >= Divisor)
        {
            ulRet++;
        }
        break;
    case ROUND_DOWN:
        break;
    default:
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbRound: invalid round type, round down by default!\n"));
        break;
    }

    return ulRet;

}


