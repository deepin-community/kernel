/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#ifndef _LINUX_IEE_ACCESS_H
#define _LINUX_IEE_ACCESS_H

#include <asm/haoc/haoc-def.h>
#include <asm/haoc/iee.h>

/* An example of IEE API. */
static inline void iee_memset(void *ptr, int data, size_t n)
{
	if (haoc_enabled)
		iee_rw_gate(IEE_OP_MEMSET, ptr, data, n);
	else
		memset(ptr, data, n);
}

#endif
