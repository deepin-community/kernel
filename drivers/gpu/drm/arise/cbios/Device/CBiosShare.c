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

#include "CBiosShare.h"
#include "CBiosChipShare.h"


static  CBIOS_U32       ModuleMask = 0;
static  CBIOS_U32       MaxDbgLevel = 2;
static  CBIOS_U32       ModuleMaskLevel = 0x3;
static  CBIOS_UCHAR DbgBuffer[CBIOSDEBUGMESSAGEMAXBYTES];


CBIOS_UCHAR* ModuleName[] =
{
    (CBIOS_UCHAR*)"[GFDISP] General ",
    (CBIOS_UCHAR*)"[GFDISP] MHL ",
    (CBIOS_UCHAR*)"[GFDISP] DSI ",
    (CBIOS_UCHAR*)"[GFDISP] HDMI ",
    (CBIOS_UCHAR*)"[GFDISP] DP ",
};

CBIOS_UCHAR* DebugLevelName[] =
{
    (CBIOS_UCHAR*)"Error:  ",
    (CBIOS_UCHAR*)"Warning:  ",
    (CBIOS_UCHAR*)"Info:  ",
    (CBIOS_UCHAR*)"Debug:  ",
    (CBIOS_UCHAR*)"Trace:  ",
};

CBIOS_UCHAR*  cbGetDebugPrefix(CBIOS_U32  Level)
{
    CBIOS_U32 index = 0;
    DbgBuffer[0] = '\0';

    Level &= 0xFFFF;

    index = Level >> 8;
    if (index < sizeofarray(ModuleName))
    {
        cbStrCat(DbgBuffer, ModuleName[index]);
    }

    index = Level & 0xff;
    if (index < sizeofarray(DebugLevelName))
    {
        cbStrCat(DbgBuffer, DebugLevelName[index]);
    }

    return DbgBuffer;
}

CBIOS_BOOL  cbCheckDebugLevel(CBIOS_U32  DbgFlag)
{
    CBIOS_U32  Level = DbgFlag & 0xFF;
    CBIOS_U32  ModuIndex = (DbgFlag >> 8) & 0xFF;

    if((Level <= MaxDbgLevel) || (((1 << ModuIndex) & ModuleMask) &&(Level <= ModuleMaskLevel )))
    {
        return CBIOS_TRUE;
    }
    else
    {
        return CBIOS_FALSE;
    }
}

CBIOS_VOID cbDelayMilliSeconds(CBIOS_U32  Milliseconds)
{
    cb_DelayMicroSeconds(1000*Milliseconds);
}

CBIOS_STATUS  cbDbgLevelCtl(PCBIOS_DBG_LEVEL_CTRL  pDbgLevelCtl)
{

    if(pDbgLevelCtl->bGetValue)
    {
        pDbgLevelCtl->DbgLevel = MaxDbgLevel |(ModuleMask << 8)| (ModuleMaskLevel << 16);
    }
    else
    {
        ModuleMaskLevel = (pDbgLevelCtl->DbgLevel >> 16) & 0xFF;
        ModuleMask = (pDbgLevelCtl->DbgLevel >> 8) & 0xFF;
        MaxDbgLevel = pDbgLevelCtl->DbgLevel & 0xFF;
    }
    return CBIOS_OK;
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

CBIOS_S32  cbStrnCmp(CBIOS_UCHAR *pStr1, CBIOS_UCHAR * pStr2, CBIOS_U32  Lenth)
{
    CBIOS_U32 i = 0;
    CBIOS_S32  Ret = 0;

    if(!pStr1 || !pStr2)
    {
        return 0;
    }

    for(i = 0; i < Lenth; i++)
    {
        if(!pStr1[i] || !pStr2[i] || (pStr1[i] != pStr2[i]))
        {
            Ret = pStr1[i] - pStr2[i];
            break;
        }
    }

    return  Ret;
}

CBIOS_S32  cbStrCmp(CBIOS_UCHAR *pStr1, const CBIOS_UCHAR * pStr2)
{
    CBIOS_S32  Ret = 0;

    if(!pStr1 || !pStr2)
    {
        return 0;
    }

    while(*pStr1 == *pStr2)
    {
        if(*pStr1 == '\0')
        {
            return 0;
        }
        pStr1++;
        pStr2++;
    }
    Ret = *pStr1 -*pStr2;

    return Ret;
}

PCBIOS_UCHAR cbStrCpy(CBIOS_UCHAR *pStrDst, CBIOS_UCHAR * pStrSrc)
{
    CBIOS_UCHAR *pTmp = pStrDst;

    if(!pStrDst || !pStrSrc)
    {
        return CBIOS_NULL;
    }

    while(*pStrSrc)
    {
        *pTmp++ = *pStrSrc++;
    }

    *pTmp = '\0';

    return pStrDst;
}



