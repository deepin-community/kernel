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

#define HUGE_PMD_ORDER 9

#include <asm/haoc/haoc-bitmap.h>
#include <linux/slab.h>
extern void set_iee_pages(unsigned long addr, int num_pages,
			  enum HAOC_BITMAP_TYPE type);
extern void unset_iee_pages(unsigned long addr, int num_pages);
extern void set_iee_page(unsigned long addr, unsigned int order);
extern void unset_iee_page(unsigned long addr, unsigned int order);
extern bool iee_free_slab_data(struct kmem_cache *s, struct slab *slab, unsigned int order);
extern unsigned int iee_calculate_order(struct kmem_cache *s, unsigned int order);

extern void iee_free_slab(struct kmem_cache *s, struct slab *slab,
                void (*do_free_slab)(struct work_struct *work));
extern void iee_free_cred_slab(struct work_struct *work);
#endif  /* _LINUX_IEE_FUNC_H */
