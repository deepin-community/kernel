/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_PGALLOC_H
#define _ASM_SW64_PGALLOC_H

#include <linux/mm.h>
#include <linux/mmzone.h>
#include <asm-generic/pgalloc.h>        /* for pte_{alloc,free}_one */

/*
 * Allocate and free page tables. The xxx_kernel() versions are
 * used to allocate a kernel page table - this turns on ASN bits
 * if any.
 */

static inline void
pmd_populate(struct mm_struct *mm, pmd_t *pmd, pgtable_t pte)
{
	unsigned long pfn = page_to_pfn(pte);

	set_pmd(pmd, __pmd((pfn << _PFN_SHIFT) | _PAGE_TABLE));
}

static inline void
pmd_populate_kernel(struct mm_struct *mm, pmd_t *pmd, pte_t *pte)
{
	unsigned long pfn = virt_to_pfn(pte);

	set_pmd(pmd, __pmd((pfn << _PFN_SHIFT) | _PAGE_TABLE));
}

static inline void
pud_populate(struct mm_struct *mm, pud_t *pud, pmd_t *pmd)
{
	unsigned long pfn = virt_to_pfn(pmd);

	set_pud(pud, __pud((pfn << _PFN_SHIFT) | _PAGE_TABLE));
}

static inline void
p4d_populate(struct mm_struct *mm, p4d_t *p4d, pud_t *pud)
{
	unsigned long pfn = virt_to_pfn(pud);

	set_p4d(p4d, __p4d((pfn << _PFN_SHIFT) | _PAGE_TABLE));
}

extern pgd_t *pgd_alloc(struct mm_struct *mm);

#define check_pgt_cache()	do { } while (0)

#endif /* _ASM_SW64_PGALLOC_H */
