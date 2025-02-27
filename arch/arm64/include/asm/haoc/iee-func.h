/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#ifndef _LINUX_IEE_ASM_FUNC_H
#define _LINUX_IEE_ASM_FUNC_H

extern void put_pages_into_iee(unsigned long addr, int order);
extern void set_iee_page(unsigned long addr, int order);
extern void unset_iee_page(unsigned long addr, int order);

#endif
