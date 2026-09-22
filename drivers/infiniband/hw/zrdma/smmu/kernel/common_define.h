/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef COMMON_DEFINE_H
#define COMMON_DEFINE_H

#define SMMU_DRIVER_IN_KERNEL

#include "cmdk.h"
#include "pub_return.h"

/**************************************************************************
 *                        Macro                                           *
 **************************************************************************/
#define MEMCPY(a, b, c, d) memcpy(a, c, d)
#define MEMSET(a, b, c, d) memset(a, c, d)

//#define BSP_ADS4
#ifndef SMMU_BIG_ENDIAN
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SMMU_BIG_ENDIAN
#endif
#endif

extern u8 g_ucMmu600PrintModuleId;

#define SMMU_PRINT(level, format, ...)                  \
	do {                                            \
		if (level >= g_ucMmu600PrintModuleId) { \
			printk(format, ##__VA_ARGS__);  \
		} else {                                \
			;                               \
		}                                       \
	} while (0)

#define SMMU_POINTER_CHECK(ptr) PUB_CHECK_NULL_PTR_RET_ERR(ptr)

extern u32 uswap_32(u32 v);
extern u64 uswap_64(u64 v);

#ifdef SMMU_BIG_ENDIAN

#define SMMU_SWAP_32(x) uswap_32(x)
#define SMMU_SWAP_64(x) uswap_64(x)

#else

#define SMMU_SWAP_32(x) (x)
#define SMMU_SWAP_64(x) (x)

#endif

#endif
