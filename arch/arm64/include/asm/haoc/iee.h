/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#ifndef _LINUX_IEE_H
#define _LINUX_IEE_H

#include <linux/mm.h>
#include <linux/types.h>
#include <asm/haoc/iee-func.h>

extern unsigned long iee_tcr;
extern unsigned long kernel_tcr;
extern bool iee_init_done;

#define IEE_OFFSET 0x400000000000
#define IEE_DATA_ORDER (PMD_SHIFT - PAGE_SHIFT)

#define __phys_to_iee(x)	(__phys_to_virt(x) | IEE_OFFSET)
#define __virt_to_iee(x)	(((u64)x) | IEE_OFFSET)
#define __kimg_to_iee(x)	(__phys_to_iee(__pa_symbol(x)))
#define __page_to_iee(x)	(__phys_to_iee(page_to_phys(x)))

#define __iee_to_virt(x)	(((u64)x) & ~IEE_OFFSET)
#define __iee_to_phys(x)	(__pa(__iee_to_virt(x)))

/* Support conversion from both kernel and linear addresses. */
#define __ptr_to_iee(x)	({		\
	typeof(x) __val;			\
	if (__is_lm_address((u64)x))	\
		__val = ((typeof(x))(__virt_to_iee((u64)x)));	\
	else						\
		__val = ((typeof(x))(__kimg_to_iee((u64)x)));	\
	__val;						\
})

#define SET_UPAGE(x)	__pgprot(pgprot_val(x) | PTE_USER)
#define SET_PPAGE(x)	__pgprot(pgprot_val(x) & (~PTE_USER))
#define SET_INVALID(x)	__pgprot(pgprot_val(x) & (~PTE_VALID))
#define SET_NG(x)	__pgprot(pgprot_val(x) | PTE_NG)

#define PGD_APTABLE_RO	(_AT(pudval_t, 1) << 62)
#define PGD_APTABLE		(_AT(pudval_t, 1) << 61)
#define PGD_PXNTABLE	(_AT(pudval_t, 1) << 59)
#define PGD_UXNTABLE	(_AT(pudval_t, 1) << 60)

#define TCR_HPD1		(UL(1) << 42)

void iee_init_mappings(pgd_t *pgdp);
void iee_init_post(void);
void iee_stack_init(void);
void iee_init_tcr(void);

#define IEE_STACK_ORDER	0x3
#define IEE_STACK_SIZE	(PAGE_SIZE << IEE_STACK_ORDER)

#define IEE_CHECK(condition) do {	\
	if (unlikely(condition))	\
		panic("IEE check failed on %s.", __func__);	\
} while (0)

extern void arm64_enter_nmi(struct pt_regs *regs);
extern const char *esr_get_class_string(unsigned long esr);

#endif
