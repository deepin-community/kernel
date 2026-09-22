/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef _PUB_RETURN_H_
#define _PUB_RETURN_H_
#include "pub_print.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 *                           宏定义                                       *
 **************************************************************************/
#ifdef PUB_ERROR
#undef PUB_ERROR
#define PUB_ERROR (0xffffffff) /*直接定义为0xffffffff*/
#else
#define PUB_ERROR (0xffffffff) /*0xffffffff*/
#endif

/** 检查空指针，返回错误 */
#define PUB_CHECK_NULL_PTR_RET_ERR(ptr)                                   \
	do {                                                              \
		if (!ptr) {                                               \
			pr_info("[zxdh_rdma] Null Ptr Err! Fuc:%s,Line:%d,File:%s\n", \
				__func__, __LINE__, __FILE__);            \
			return PUB_ERROR;                                 \
		}                                                         \
	} while (0)

/**************************************************************************
 *                          数据类型                                      *
 **************************************************************************/

/**************************************************************************
 *                         全局函数原型                                       *
 **************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _PUB_RETURN_H_ */
