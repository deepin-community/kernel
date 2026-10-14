/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#ifndef _LINUX_IEE_H
#define _LINUX_IEE_H

#include <linux/percpu.h>

extern unsigned long IEE_OFFSET;
extern bool haoc_enabled;
extern bool haoc_init_done;
#define __iee_pa(x) (__pa(x - IEE_OFFSET))
#define __phys_to_iee(x) ((void *)(__va(x) + IEE_OFFSET))
#define __page_to_phys(x) (page_to_pfn(x) << PAGE_SHIFT)
#define __page_to_iee(x) ((unsigned long)(__phys_to_iee(__page_to_phys(x))))
#define __slab_to_iee(x) (__page_to_iee(folio_page(slab_folio(x), 0)))
//#define __addr_to_iee(x) (__phys_to_iee(__pa(x)))

#define __virt_to_iee(x)	(((u64)x) + IEE_OFFSET)
#define __kimg_to_iee(x)	(__phys_to_iee(__pa_symbol(x)))
#define __ptr_to_iee(x)	({		\
	typeof(x) __val;			\
	if ((u64)x < VMALLOC_START)	\
		__val = ((typeof(x))(__virt_to_iee((u64)x)));	\
	else						\
		__val = ((typeof(x))(__kimg_to_iee((u64)x)));	\
	__val;						\
})
#define __addr_to_iee(x)	({		\
	u64 __val;			\
	if ((u64)x < VMALLOC_START)	\
		__val = (u64)__virt_to_iee((u64)x);	\
	else						\
		__val = (u64)__kimg_to_iee((u64)x);	\
	__val;						\
})
#ifdef CONFIG_IEE_PTRP
#define __addr_to_token(x) iee_get_task_token(x)
#endif

#define IEE_DATA_ORDER (PMD_SHIFT - PAGE_SHIFT)
#define IEE_STACK_ORDER 0
struct iee_stack {
	void *stack;
};

DECLARE_PER_CPU(struct iee_stack, iee_stacks);

extern void *alloc_low_pages(unsigned int num);
extern void iee_early_init(void);
extern void iee_init(void);
extern void iee_post_init(void);
extern bool iee_init_done;
#ifdef CONFIG_IEE_PTRP
struct task_struct;
struct task_token;
extern struct task_token *iee_get_task_token(struct task_struct *task);
#endif
#endif
