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

#include <asm/haoc/haoc-bitmap.h>

extern void set_iee_address(unsigned long addr, unsigned int order, bool valid);
extern void set_iee_address_valid(unsigned long lm_addr, unsigned int order);
extern void set_iee_address_invalid(unsigned long lm_addr, unsigned int order);
extern void iee_set_logical_mem(unsigned long addr, unsigned int order, bool prot);
extern void put_pages_into_iee(unsigned long addr, int order);
extern void put_pages_into_iee_rw(unsigned long addr, int order);
extern void remove_pages_from_iee(unsigned long addr, int order);
extern void set_iee_page_type(unsigned long addr, int order,
			      enum HAOC_BITMAP_TYPE type);
extern void set_iee_page(unsigned long addr, int order, enum HAOC_BITMAP_TYPE type);
extern void unset_iee_page(unsigned long addr, int order);

#endif
