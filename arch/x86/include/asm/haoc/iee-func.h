/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#ifndef _LINUX_IEE_FUNC_H
#define _LINUX_IEE_FUNC_H

extern void set_iee_page(unsigned long addr, unsigned int order);
extern void unset_iee_page(unsigned long addr, unsigned int order);

#endif  /* _LINUX_IEE_FUNC_H */
