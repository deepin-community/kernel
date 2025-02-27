// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#include <asm/haoc/haoc.h>

typedef void (*iee_func)(void);

/*
 * Register IEE handler functions here.
 * IEE gate would find out the specific handler function inside this array
 * using the index that iee_rw_gate() gives, so the arrangement of these
 * IEE functions should correspond one-to-one with the enum entries in haoc-def.h,
 * such as IEE_OP_MEMSET to call _iee_memset().
 */
iee_func iee_funcs[] = {
	(iee_func)_iee_memset,
	NULL
};
