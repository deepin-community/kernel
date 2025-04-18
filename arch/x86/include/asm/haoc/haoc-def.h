/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#ifndef _LINUX_HAOC_DEF_H
#define _LINUX_HAOC_DEF_H

enum {
	IEE_OP_MEMCPY,
	IEE_OP_MEMSET,
	IEE_OP_SET_FREEPTR,
	IEE_OP_TEST_CLEAR_BIT,
	IEE_FLAG_END
};

#ifdef CONFIG_IEE_SIP
#define IEE_SIP_TEST    0
#define IEE_WRITE_CR0   1
#define IEE_WRITE_CR3   2
#define IEE_WRITE_CR4   3
#define IEE_LOAD_IDT    4
#endif

#endif
