// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#include <asm/haoc/iee.h>
#include <asm/haoc/haoc-def.h>

/* This is an example of iee handler function.
 * The first Parameter must be reserved, and later parameters shall be the same
 * with outer API like iee_memset();
 */
#if defined(clang)
__attribute__((optnone))
#elif defined(GNUC) || defined(GNUG)
__attribute__((optimize("O0")))
#endif
void __iee_code _iee_memset(unsigned long __unused, void *ptr, int data, size_t n)
{
	/* Maybe you need to verify the input parameters first. */
	char *_ptr;

	/* Write the page by IEE address of the input pointer as the address it
	 * points to shall be already read-only to prevent kernel access.
	 */
	_ptr = __ptr_to_iee((char *)ptr);

	while (n--)
		*_ptr++ = data;
}
