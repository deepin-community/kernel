/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#ifndef _LINUX_IEE_ACCESS_H
#define _LINUX_IEE_ACCESS_H

#include <asm/haoc/iee.h>
#include <asm/haoc/haoc-def.h>

extern unsigned long long iee_rw_gate(int flag, ...);

static inline void iee_memcpy(void *dst, const void *src, size_t n)
{
	if (haoc_enabled)
		iee_rw_gate(IEE_OP_MEMCPY, dst, src, n);
	else
		memcpy(dst, src, n);
}

static inline void iee_memset(void *ptr, int data, size_t n)
{
	if (haoc_enabled)
		iee_rw_gate(IEE_OP_MEMSET, ptr, data, n);
	else
		memset(ptr, data, n);
}

static inline void iee_set_freeptr(void **pptr, void *ptr)
{
	if (haoc_enabled)
		iee_rw_gate(IEE_OP_SET_FREEPTR, pptr, ptr);
	else
		*pptr = ptr;
}

static inline unsigned long iee_test_and_clear_bit(long nr, unsigned long *addr)
{
	if (haoc_enabled)
		return iee_rw_gate(IEE_OP_TEST_CLEAR_BIT, nr, addr);
	else
		return test_and_clear_bit(nr, addr);
}

#endif
