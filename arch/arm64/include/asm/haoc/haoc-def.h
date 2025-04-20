/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#ifndef _LINUX_HAOC_DEF_H
#define _LINUX_HAOC_DEF_H

/* Place the enum entries in the order corresponding to iee_funcs array. */
enum {
	IEE_OP_MEMSET,
#ifdef CONFIG_IEE_PTRP
	IEE_OP_SET_TOKEN_PGD,
	IEE_OP_INIT_TOKEN,
	IEE_OP_INVALIDATE_TOKEN,
	IEE_OP_VALIDATE_TOKEN,
#endif
	IEE_FLAG_END
};

/* The entry gate of all IEE APIs. The first parameter must be a valid
 * IEE function index.
 */
extern unsigned long long iee_rw_gate(int flag, ...);

#define __iee_code		__section(".iee.text")
#define __iee_data		__section(".iee.data")

#endif
