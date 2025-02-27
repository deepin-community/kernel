/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#ifndef _LINUX_IEE_MMU_H
#define _LINUX_IEE_MMU_H

extern phys_addr_t __init early_iee_stack_alloc(int order);
extern void __iee_create_pgd_mapping_locked(pgd_t *pgdir, phys_addr_t phys,
				 unsigned long virt, phys_addr_t size,
				 pgprot_t prot,
				 phys_addr_t (*pgtable_alloc)(int),
				 int flags);
extern void __init iee_init_mappings(pgd_t *pgdp);
extern void __init init_early_iee_data(void);
extern void __init early_iee_data_cache_init(void);

#endif
