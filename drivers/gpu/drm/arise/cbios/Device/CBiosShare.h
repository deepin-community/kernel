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

#ifndef _CBIOS_SHARE_H_
#define _CBIOS_SHARE_H_

#ifndef __LINUX__

#ifndef  __func__
#define  __func__  __FUNCTION__
#endif

#ifndef  inline
#define  inline  __inline
#endif

#endif

#include "../CBios.h"

#include "CBiosTypes.h"
#include "CBIOSVER.H"

#ifdef __LINUX__
#define FUNCTION_NAME __func__
#else
#define FUNCTION_NAME __FUNCTION__
#endif


#define LINE_NUM __LINE__


#ifndef ASSERT
#if defined(DBG) && defined(WIN32)
#define ASSERT(x) if(!(x)) __debugbreak()
#else
#define ASSERT(a)
#endif
#endif

typedef enum _CBIOS_BOARD_VERSION
{
    CBIOS_BOARD_VERSION_0 = 0,
    CBIOS_BOARD_VERSION_1 = 1,
    CBIOS_BOARD_VERSION_2 = 2,
    CBIOS_BOARD_VERSION_EVT = 3,
    CBIOS_BOARD_VERSION_MAX,
}CBIOS_BOARD_VERSION;

typedef enum _CBIOS_ACTIVE_TYPE   {
    CBIOS_TYPE_NONE        = 0x00,
    CBIOS_TYPE_CRT         = 0x01,
    CBIOS_TYPE_PANEL       = 0x02,
    CBIOS_TYPE_TV          = 0x04,
    CBIOS_TYPE_HDTV        = 0x08,
    CBIOS_TYPE_DVO         = 0x20,
    CBIOS_TYPE_DUOVIEW     = 0x80,
    CBIOS_TYPE_DSI         = 0x1000,
    CBIOS_TYPE_MHL         = 0x2000,
    CBIOS_TYPE_DP1         = 0x8000,
    CBIOS_TYPE_DP2         = 0x10000,
    CBIOS_TYPE_DP3         = 0x20000,
    CBIOS_TYPE_DP4         = 0x40000,
} CBIOS_ACTIVE_TYPE, *PCBIOS_ACTIVE_TYPE;

#define ALL_DP_TYPES    (CBIOS_TYPE_DP1 | CBIOS_TYPE_DP2 | CBIOS_TYPE_DP3 | CBIOS_TYPE_DP4)

#define CBIOS_BOARD_VERSION_DEFAULT CBIOS_BOARD_VERSION_1

#define CBIOSDEBUGMESSAGEMAXBYTES 64


#define ELT2K_HARDCODE_DSI_CMDMODE  0

/******* these functions must be implemented outside *******/
/* debug print function */

#define     DBG_LEVEL_ERROR         0
#define     DBG_LEVEL_WARNING       1
#define     DBG_LEVEL_INFO          2
#define     DBG_LEVEL_DEBUG         3
#define     DBG_LEVEL_TRACE         4

typedef enum  _DEBUG_MODULE
{
    GENERIC_MODULE = 0,
    MHL_MODULE = 1,
    DSI_MODULE = 2,
    HDMI_MODULE = 3,
    DP_MODULE = 4,
    DBG_MODULE_NUM = 5,
}DEBUG_MODULE;



#define     MAKE_LEVEL(Module, Level)  cbCheckDebugLevel((Module##_MODULE << 8) + DBG_LEVEL_##Level), DBG_LEVEL_##Level, cbGetDebugPrefix((Module##_MODULE << 8) + DBG_LEVEL_##Level)

#define cbTraceEnter(Module)  cbDebugPrint((MAKE_LEVEL(Module, TRACE), "%s: Enter\n", __func__))
#define cbTraceExit(Module)   cbDebugPrint((MAKE_LEVEL(Module, TRACE), "%s: Exit\n", __func__))

//debug print level
#ifdef __LINUX__
#define     DBG_LEVEL_ERROR_MSG     2
#define     DBG_LEVEL_CHIP_INFO     0
#define     DBG_LEVEL_BIOS_VERSION  0
#define     DBG_LEVEL_MODE_INFO     0
#define     DBG_LEVEL_DEVICE_INFO   0
#define     DBG_LEVEL_DEBUG_MSG     1
#else
#define     DBG_LEVEL_ERROR_MSG     0
#define     DBG_LEVEL_CHIP_INFO     0
#define     DBG_LEVEL_BIOS_VERSION  0
#define     DBG_LEVEL_MODE_INFO     0
#define     DBG_LEVEL_DEVICE_INFO   0
#define     DBG_LEVEL_DEBUG_MSG     1
#endif

#define sizeofarray(a) (sizeof(a)/sizeof(a[0]))
#define SIZEOF_STRUCT_TILL_MEMBER(MY_STRUCT_VAR, MY_MEMBER_VAR)\
    ((CBIOS_U32)((PCBIOS_CHAR)(&(MY_STRUCT_VAR->MY_MEMBER_VAR)) - (PCBIOS_CHAR)(((MY_STRUCT_VAR))) + (sizeof(MY_STRUCT_VAR->MY_MEMBER_VAR))))

#define container_of(ptr, sample, member) \
    (((PCBIOS_CHAR)(ptr) == CBIOS_NULL) ? CBIOS_NULL : (PCBIOS_VOID)((PCBIOS_CHAR)(ptr) - ((PCBIOS_CHAR)(&(((sample)(0))->member)) - (PCBIOS_CHAR)(0))))

#ifndef MORE_THAN_1BIT
#define MORE_THAN_1BIT(x)       ( (CBIOS_BOOL)( ((x) - 1) & (x) ) )
#endif

#ifndef LOW_WORD
#define LOW_WORD(x)          ( (x) & 0xffff )
#endif

#ifndef HIGH_WORD
#define HIGH_WORD(x)         ( ((x) >> 16 ) & 0xffff )
#endif

#ifndef WORD_CAT
#define WORD_CAT(low, high)  ( ( (low) & 0xffff ) | (( (high) & 0xffff ) << 16) )
#endif

typedef enum _CBIOS_ROUND_METHOD
{
    ROUND_DOWN = 0,
    ROUND_UP,
    ROUND_NEAREST
}CBIOS_ROUND_METHOD;

CBIOS_UCHAR*  cbGetDebugPrefix(CBIOS_U32  Level);
CBIOS_BOOL  cbCheckDebugLevel(CBIOS_U32  DbgFlag);
CBIOS_STATUS  cbDbgLevelCtl(PCBIOS_DBG_LEVEL_CTRL pDbgLevelCtl);
CBIOS_VOID cbDelayMilliSeconds(CBIOS_U32 Milliseconds);
CBIOS_BOOL  cbItoA(CBIOS_U32 ulValue, CBIOS_U8 *pStr, CBIOS_U8 byRadix, CBIOS_U32 ulLength);
CBIOS_U32   cbStrLen(CBIOS_UCHAR * pStrSrc);
PCBIOS_UCHAR cbStrCat(CBIOS_UCHAR *pStrDst, CBIOS_UCHAR * pStrSrc);
CBIOS_S32  cbStrnCmp(CBIOS_UCHAR *pStr1, CBIOS_UCHAR * pStr2, CBIOS_U32  Lenth);
CBIOS_S32  cbStrCmp(CBIOS_UCHAR *pStr1, const CBIOS_UCHAR * pStr2);
PCBIOS_UCHAR cbStrCpy(CBIOS_UCHAR *pStrDst, CBIOS_UCHAR * pStrSrc);
CBIOS_U32 cbRound(CBIOS_U32 Dividend, CBIOS_U32 Divisor, CBIOS_ROUND_METHOD RoundMethod);
CBIOS_BOARD_VERSION cbGetBoardVersion(PCBIOS_VOID pvcbe);
CBIOS_STATUS cbGetExtensionSize(CBIOS_U32 *pulExtensionSize);

#ifdef __BIG_ENDIAN__
static inline CBIOS_U16 cb_swab16(CBIOS_U16 x)
{
    CBIOS_U16 temp = ((x & 0x00ffu) << 8) |
                     ((x & 0xff00u) >> 8);
    return temp;
}

static inline CBIOS_U32 cb_swab32(CBIOS_U32 x)
{
    CBIOS_U32 temp = ((x & 0x000000ff) << 24) |
                     ((x & 0x0000ff00) <<  8) |
                     ((x & 0x00ff0000) >>  8) |
                     ((x & 0xff000000) >> 24);
    return temp;
}
#else
#define cb_swab16(x) x
#define cb_swab32(x) x
#endif


#endif /* _CBIOS_SHARE_H_ */

