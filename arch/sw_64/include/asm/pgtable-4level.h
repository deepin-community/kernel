/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_PGTABLE_4LEVEL_H
#define _ASM_SW64_PGTABLE_4LEVEL_H

#ifdef __KERNEL__
#ifndef __ASSEMBLY__
/*
 * These are used to make use of C type-checking..
 */
typedef struct { unsigned long pte; } pte_t;
typedef struct { unsigned long pmd; } pmd_t;
typedef struct { unsigned long pgd; } pgd_t;
typedef struct { unsigned long pud; } pud_t;
typedef struct { unsigned long pgprot; } pgprot_t;

#define pte_val(x)	((x).pte)
#define pmd_val(x)	((x).pmd)
#define pgd_val(x)	((x).pgd)
#define pud_val(x)	((x).pud)
#define pgprot_val(x)	((x).pgprot)

#define __pte(x)	((pte_t) { (x) })
#define __pmd(x)	((pmd_t) { (x) })
#define __pud(x)	((pud_t) { (x)  })
#define __pgd(x)	((pgd_t) { (x) })
#define __pgprot(x)	((pgprot_t) { (x) })
#endif /* !__ASSEMBLY__ */

#define PAGE_OFFSET	0xfff0000000000000

#endif
#endif /* _ASM_SW64_PGTABLE_4LEVEL_H */
