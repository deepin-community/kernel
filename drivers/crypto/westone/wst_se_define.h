/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _WST_SE_DEFINE_H
#define _WST_SE_DEFINE_H

typedef void (*PSECallBackfn)(void *pParam);

#define WST_SE_OK 0 //function syccess
#define WST_SE_FAILURE 1
#define WST_SE_ERROR_MALLOC 2
#define WST_SE_ERROR_OPEN 3
#define WST_SE_ERROR_ID 4
#define WST_SE_ERROR_OPERTYPE 5
#define WST_SE_ERROR_KEYID 6
#define WST_SE_NOT_SUPPORT 7
#define WST_SE_ERROR_FULL 8
#define WST_SE_VERIFY_ERROR 0xb
#define WST_SE_NOT_SUPPORT_VERIFY 0xd
#define WST_SE_ERROR_LENGTH 0xc
#define WST_SE_HAS_OPEN 0xd
#define WST_COPY_MEM_ERROR 0xe
#define WST_SE_PARAM_ERROR 0xf

#define BASE_ERROR 0x1000
#define WST_SE_ERROR_LICENSE 0x100b
#define WST_SE_ERROR_NOT_SUPPORT_TYPE 0x100d

#endif
