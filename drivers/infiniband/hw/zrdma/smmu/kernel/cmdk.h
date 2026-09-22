/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef CMDK_H
#define CMDK_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 *                           宏定义                                       *
 **************************************************************************/
/* 基本返回值 */
#ifdef CMDK_OK
#undef CMDK_OK
#define CMDK_OK (0)
#else
#define CMDK_OK (0)
#endif

#ifdef CMDK_ERROR
#undef CMDK_ERROR
#define CMDK_ERROR (0xffff) /*直接定义为0xffff*/
#else
#define CMDK_ERROR (0xffff) /*0xffff*/
#endif

#if (!defined(NULL))
#undef NULL
#define NULL (0)
#endif

#ifndef PRIVATE
#define PRIVATE static
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL (0)
#else
#define NULL ((void *)0)
#endif
#endif

#if (!defined(TRUE) || (TRUE != 1))
#undef TRUE
#define TRUE 1
#endif

#if (!defined(FALSE) || (FALSE != 0))
#undef FALSE
#define FALSE 0
#endif

/**************************************************************************
 *                          数据类型                                      *
 **************************************************************************/
typedef int (*FUNCPTR)(void);
typedef void (*VOIDFUNCPTR)(void);

/**************************************************************************
 *                         全局函数原型                                       *
 **************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* CMDK_H */
