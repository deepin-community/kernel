/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_PGTABLE_H
#define _ASM_SW64_PGTABLE_H


#include <asm-generic/pgtable-nop4d.h>

/*
 * This file contains the functions and defines necessary to modify and use
 * the sw64 page table tree.
 *
 * This hopefully works with any standard sw64 page-size, as defined
 * in <asm/page.h> (currently 8192).
 */
#include <linux/mmzone.h>
#include <linux/mm_types.h>

#include <asm/memory.h>
#include <asm/page.h>
#include <asm/processor.h>	/* For TASK_SIZE */
#include <asm/setup.h>

struct mm_struct;
struct vm_area_struct;

static inline void set_pmd(pmd_t *pmdp, pmd_t pmd)
{
	*pmdp = pmd;
}

static inline void set_pmd_at(struct mm_struct *mm, unsigned long addr,
			      pmd_t *pmdp, pmd_t pmdval)
{
	set_pmd(pmdp, pmdval);
}

static inline void set_pud(pud_t *pudp, pud_t pud)
{
	*pudp = pud;
}

static inline void set_p4d(p4d_t *p4dp, p4d_t p4d)
{
	*p4dp = p4d;
}
/* PGDIR_SHIFT determines what a forth-level page table entry can map */
#define PGDIR_SHIFT	(PAGE_SHIFT + 3 * (PAGE_SHIFT - 3))
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE - 1))

/* PUD_SHIFT determines the size of the area a third-level page table can map */
#define PUD_SHIFT	(PAGE_SHIFT + 2 * (PAGE_SHIFT - 3))
#define PUD_SIZE	(1UL << PUD_SHIFT)
#define PUD_MASK	(~(PUD_SIZE-1))

/* PMD_SHIFT determines the size of the area a second-level page table can map */
#define PMD_SHIFT	(PAGE_SHIFT + (PAGE_SHIFT - 3))
#define PMD_SIZE	(1UL << PMD_SHIFT)
#define PMD_MASK	(~(PMD_SIZE - 1))

#define CONT_PMD_SHIFT		6
#define CONT_PMDS		(1 << CONT_PMD_SHIFT)
#define CONT_PMD_SIZE		(CONT_PMDS * PMD_SIZE)
#define CONT_PMD_MASK		(~(CONT_PMD_SIZE - 1))

/*
 * Entries per page directory level:  the sw64 is three-level, with
 * all levels having a one-page page table.
 */
#define PTRS_PER_PTE	(1UL << (PAGE_SHIFT - 3))
#define PTRS_PER_PMD	(1UL << (PAGE_SHIFT - 3))
#define PTRS_PER_PGD	(1UL << (PAGE_SHIFT - 3))
#define PTRS_PER_PUD	(1UL << (PAGE_SHIFT - 3))

#define USER_PTRS_PER_PGD	(TASK_SIZE / PGDIR_SIZE)
#define FIRST_USER_ADDRESS	0UL

/* Number of pointers that fit on a page:  this will go away. */
#define PTRS_PER_PAGE	(1UL << (PAGE_SHIFT - 3))

#define VMALLOC_START	(-2 * PGDIR_SIZE)
#ifndef CONFIG_SPARSEMEM_VMEMMAP
#define VMALLOC_END	(-PGDIR_SIZE)
#else
#define VMEMMAP_END	(-PGDIR_SIZE)
#define vmemmap		((struct page *)VMEMMAP_END - (1UL << (MAX_PHYSMEM_BITS - PAGE_SHIFT)))
#define VMALLOC_END	((unsigned long)vmemmap)
#endif

/*
 * HMcode-imposed page table bits
 */
#if defined(CONFIG_SUBARCH_C3B)

#define _PAGE_VALID	0x0001
#define _PAGE_PRESENT	_PAGE_VALID
#define _PAGE_FOR	0x0002	/* used for page protection (fault on read) */
#define _PAGE_FOW	0x0004	/* used for page protection (fault on write) */
#define _PAGE_FOE	0x0008	/* used for page protection (fault on exec) */
#define _PAGE_ASM	0x0010
#define _PAGE_CONT	0x0020  /* used for 256M page size bit */
#define _PAGE_LEAF	0x0040  /* used for 8M page size bit */
#define _PAGE_PROTNONE	0x0080  /* used for numa page balancing */
#define _PAGE_SPECIAL	0x0100
#define _PAGE_KRE	0x0400	/* xxx - see below on the "accessed" bit */
#define _PAGE_URE	0x0800	/* xxx */
#define _PAGE_KWE	0x4000	/* used to do the dirty bit in software */
#define _PAGE_UWE	0x8000	/* used to do the dirty bit in software */

/* .. and these are ours ... */
#define _PAGE_DIRTY	0x20000
#define _PAGE_ACCESSED	0x40000

#define _PAGE_SPLITTING		0x200000	/* For Transparent Huge Page */
#define _PAGE_DEVMAP		0x400000	/* For ZONE DEVICE page */

#define _PAGE_BIT_FOW		2		/* bit of _PAGE_FOW */
#define _PAGE_BIT_ACCESSED	18		/* bit of _PAGE_ACCESSED */
#define _PAGE_BIT_SPLITTING	21		/* bit of _PAGE_SPLITTING */
#define _PAGE_BIT_DEVMAP	22		/* bit of _PAGE_DEVMAP */
/*
 * NOTE! The "accessed" bit isn't necessarily exact:  it can be kept exactly
 * by software (use the KRE/URE/KWE/UWE bits appropriately), but I'll fake it.
 * Under Linux/sw64, the "accessed" bit just means "read", and I'll just use
 * the KRE/URE bits to watch for it. That way we don't need to overload the
 * KWE/UWE bits with both handling dirty and accessed.
 *
 * Note that the kernel uses the accessed bit just to check whether to page
 * out a page or not, so it doesn't have to be exact anyway.
 */

/* Used for swap PTEs only. */
#define _PAGE_SWP_EXCLUSIVE	_BITUL(5)

#define __DIRTY_BITS	(_PAGE_DIRTY | _PAGE_KWE | _PAGE_UWE)
#define __ACCESS_BITS	(_PAGE_ACCESSED | _PAGE_KRE | _PAGE_URE)

#define _PFN_SHIFT	28

/*
 * All the normal masks have the "page accessed" bits on, as any time they are used,
 * the page is accessed. They are cleared only by the page-out routines
 */
#define PAGE_NONE	__pgprot(__ACCESS_BITS | _PAGE_FOR | _PAGE_FOW | _PAGE_FOE | _PAGE_PROTNONE)
#define PAGE_KERNEL	__pgprot(_PAGE_VALID | _PAGE_ASM | _PAGE_KRE | _PAGE_KWE)
#define _PAGE_NORMAL(x)	__pgprot(_PAGE_VALID | __ACCESS_BITS | (x))

#define page_valid_kern(x)	(0)

#elif defined(CONFIG_SUBARCH_C4)

#define _PAGE_VALID	0x0001
#define _PAGE_PRESENT	_PAGE_VALID
#define _PAGE_FOR	0x0002	/* used for page protection (fault on read) */
#define _PAGE_FOW	0x0004	/* used for page protection (fault on write) */
#define _PAGE_FOE	0x0008	/* used for page protection (fault on exec) */
#define _PAGE_FIXED	0x0010
#define _PAGE_CONT	0x0020	/* used for 512M page size bit*/
#define _PAGE_LEAF	0x0040	/* used for huge page bit */
#define _PAGE_PCD	0x0080	/* used for page cache disabled */

/* and these are sw definition */
#define _PAGE_WCD	0x0100
#define _PAGE_ACCESSED	0x0200
#define _PAGE_SPLITTING	0x0400	/* For Transparent Huge Page */
#define _PAGE_SPECIAL	0x0800
#define _PAGE_DEVMAP	0x1000	/* For ZONE DEVICE page */
#define _PAGE_KERN	0x2000
#define _PAGE_DIRTY	_BITUL(62)
#define _PAGE_PROTNONE	_BITUL(63)
#define _PAGE_BIT_FOW		2	/* bit of _PAGE_FOW */
#define _PAGE_BIT_ACCESSED	9	/* bit of _PAGE_ACCESSED */
#define _PAGE_BIT_SPLITTING	10	/* bit of _PAGE_SPLITTING */
#define _PAGE_BIT_DEVMAP	12	/* bit of _PAGE_DEVMAP */

/* Used for swap PTEs only. */
#define _PAGE_SWP_EXCLUSIVE	_BITUL(5)

#define __DIRTY_BITS		_PAGE_DIRTY
#define __ACCESS_BITS		_PAGE_ACCESSED

#define _PFN_SHIFT	24

/*
 * All the normal masks have the "page accessed" bits on, as any time they are used,
 * the page is accessed. They are cleared only by the page-out routines
 */
#define PAGE_NONE		__pgprot(__ACCESS_BITS | _PAGE_FOR | _PAGE_FOW | _PAGE_FOE | _PAGE_LEAF | _PAGE_PROTNONE)
#define PAGE_KERNEL		__pgprot(_PAGE_VALID | _PAGE_KERN | _PAGE_LEAF)
#define _PAGE_NORMAL(x)		__pgprot(_PAGE_VALID | __ACCESS_BITS | _PAGE_LEAF | (x))

#define page_valid_kern(x)	((x & (_PAGE_VALID | _PAGE_KERN)) == (_PAGE_VALID | _PAGE_KERN))
#endif

#define PFN_PTE_SHIFT	_PFN_SHIFT

#define __PFN_BITS	(MAX_PHYSMEM_BITS - PAGE_SHIFT)
#define _PFN_MASK	(GENMASK(__PFN_BITS - 1, 0) << _PFN_SHIFT)

#define _PAGE_TABLE	(_PAGE_VALID | __DIRTY_BITS | __ACCESS_BITS)
#define _PAGE_CHG_MASK	(_PFN_MASK | __DIRTY_BITS | __ACCESS_BITS | _PAGE_SPECIAL | _PAGE_LEAF | _PAGE_CONT)

#define PAGE_READONLY_NOEXEC	_PAGE_NORMAL(_PAGE_FOE | _PAGE_FOW)
#define PAGE_EXEC		_PAGE_NORMAL(_PAGE_FOW | _PAGE_FOR)
#define PAGE_READONLY_EXEC	_PAGE_NORMAL(_PAGE_FOW)
#define PAGE_COPY_NOEXEC	PAGE_READONLY_NOEXEC
#define PAGE_COPY_EXEC		PAGE_READONLY_EXEC
/*
 * Since we don't have hardware dirty-bit management yet, shared
 * writable page has FOW bit set to make sure dirty-bit could be
 * set properly.
 */
#define PAGE_SHARED_NOEXEC	PAGE_READONLY_NOEXEC
#define PAGE_SHARED_EXEC	PAGE_READONLY_EXEC

/* For backward compatibility */
#define PAGE_READONLY		PAGE_READONLY_EXEC
#define PAGE_COPY		PAGE_COPY_EXEC
#define PAGE_SHARED		PAGE_SHARED_EXEC

/*
 * The hardware can handle write-only mappings, but as the sw64
 * architecture does byte-wide writes with a read-modify-write
 * sequence, it's not practical to have write-without-read privs.
 * Thus the "-w- -> rw-" and "-wx -> rwx" mapping here (and in
 * arch/sw_64/mm/fault.c)
 */
#define __P000		PAGE_NONE
#define __P001		PAGE_READONLY_NOEXEC
#define __P010		PAGE_COPY_NOEXEC
#define __P011		PAGE_COPY_NOEXEC
#define __P100		PAGE_EXEC
#define __P101		PAGE_READONLY_EXEC
#define __P110		PAGE_COPY_EXEC
#define __P111		PAGE_COPY_EXEC

#define __S000		PAGE_NONE
#define __S001		PAGE_READONLY_NOEXEC
#define __S010		PAGE_SHARED_NOEXEC
#define __S011		PAGE_SHARED_NOEXEC
#define __S100		PAGE_EXEC
#define __S101		PAGE_READONLY_EXEC
#define __S110		PAGE_SHARED_EXEC
#define __S111		PAGE_SHARED_EXEC

/*
 * pgprot_noncached() is only for infiniband pci support, and a real
 * implementation for RAM would be more complicated.
 */
#define pgprot_noncached(prot)		(prot)

/*
 * ZERO_PAGE is a global shared page that is always zero:  used
 * for zero-mapped memory areas etc..
 */
extern unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)];
#define ZERO_PAGE(vaddr)	(virt_to_page(empty_zero_page))

static inline void set_pte(pte_t *ptep, pte_t pteval)
{
	*ptep = pteval;

	if (page_valid_kern(pte_val(pteval))) {
		mb();
		if ((pte_val(pteval) & _PAGE_FOE) == 0)
			imemb();
	}
}

static inline void set_pte_at(struct mm_struct *mm, unsigned long addr,
			      pte_t *ptep, pte_t pteval)
{
	set_pte(ptep, pteval);
}

#define pud_write pud_write
static inline int pud_write(pud_t pud)
{
	return !(pud_val(pud) & _PAGE_FOW);
}

static inline pte_t pfn_pte(unsigned long pfn, pgprot_t prot)
{
	pte_t pte;

	pte_val(pte) = (pfn << _PFN_SHIFT) | pgprot_val(prot);
	return pte;
}

static inline pmd_t pfn_pmd(unsigned long pfn, pgprot_t prot)
{
	pmd_t pmd;

	pmd_val(pmd) = (pfn << _PFN_SHIFT) | pgprot_val(prot);
	return pmd;
}
static inline pud_t pfn_pud(unsigned long pfn, pgprot_t pgprot)
{
	pud_t pud;

	pud_val(pud) = (pfn << _PFN_SHIFT) | pgprot_val(pgprot);
	return pud;
}

static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	pte_val(pte) = (pte_val(pte) & _PAGE_CHG_MASK) | pgprot_val(newprot);
	return pte;
}

static inline pmd_t pmd_modify(pmd_t pmd, pgprot_t newprot)
{
	pmd_val(pmd) = (pmd_val(pmd) & _PAGE_CHG_MASK) | pgprot_val(newprot);
	return pmd;
}

/*
 * Conversion functions:  convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 */
#define page_to_pa(page)	(page_to_pfn(page) << PAGE_SHIFT)

#define p4d_pfn(p4d)		((p4d_val(p4d) & _PFN_MASK) >> _PFN_SHIFT)
#define pud_pfn(pud)		((pud_val(pud) & _PFN_MASK) >> _PFN_SHIFT)
#define pmd_pfn(pmd)		((pmd_val(pmd) & _PFN_MASK) >> _PFN_SHIFT)
#define pte_pfn(pte)		((pte_val(pte) & _PFN_MASK) >> _PFN_SHIFT)

#define p4d_page(p4d)		pfn_to_page(p4d_pfn(p4d))
#define pud_page(pud)		pfn_to_page(pud_pfn(pud))
#define pmd_page(pmd)		pfn_to_page(pmd_pfn(pmd))
#define pte_page(pte)		pfn_to_page(pte_pfn(pte))

#define mk_pte(page, prot)	pfn_pte(page_to_pfn(page), prot)

static inline pmd_t *pud_pgtable(pud_t pud)
{
	return (pmd_t *)pfn_to_virt(pud_pfn(pud));
}

static inline pud_t *p4d_pgtable(p4d_t p4d)
{
	return (pud_t *)pfn_to_virt(p4d_pfn(p4d));
}

static inline unsigned long p4d_page_vaddr(p4d_t p4d)
{
	return (unsigned long)pfn_to_virt(p4d_pfn(p4d));
}

static inline unsigned long pud_page_vaddr(pud_t pud)
{
	return (unsigned long)pfn_to_virt(pud_pfn(pud));
}

static inline unsigned long pmd_page_vaddr(pmd_t pmd)
{
	return (unsigned long)pfn_to_virt(pmd_pfn(pmd));
}

static inline int pte_none(pte_t pte)
{
	return !pte_val(pte);
}

static inline int pte_valid(pte_t pte)
{
	return !!(pte_val(pte) & _PAGE_VALID);
}

static inline int pte_present(pte_t pte)
{
	return !!(pte_val(pte) & (_PAGE_VALID | _PAGE_PROTNONE));
}

static inline int pte_huge(pte_t pte)
{
	return !!(pte_val(pte) & _PAGE_LEAF);
}

static inline void pte_clear(struct mm_struct *mm,
			     unsigned long addr, pte_t *ptep)
{
	pte_val(*ptep) = 0;
}

#define pte_accessible(mm, pte)	\
	(mm_tlb_flush_pending(mm) ? pte_present(pte) : pte_valid(pte))

static inline int pmd_none(pmd_t pmd)
{
	return !pmd_val(pmd);
}

static inline int pmd_bad(pmd_t pmd)
{
	return (pmd_val(pmd) & ~_PFN_MASK) != _PAGE_TABLE;
}

static inline int pmd_present(pmd_t pmd)
{
	/*
	 * Checking for _PAGE_LEAF is needed too because
	 * split_huge_page will temporarily clear the valid bit (but
	 * the _PAGE_LEAF flag will remain set at all times while the
	 * _PAGE_VALID bit is clear).
	 */
	return !!(pmd_val(pmd) & (_PAGE_VALID | _PAGE_PROTNONE | _PAGE_LEAF));
}

static inline void pmd_clear(pmd_t *pmdp)
{
	pmd_val(*pmdp) = 0;
}

static inline int pmd_dirty(pmd_t pmd)
{
	return !!(pmd_val(pmd) & _PAGE_DIRTY);
}

#define pmd_young pmd_young
static inline int pmd_young(pmd_t pmd)
{
	return !!(pmd_val(pmd) & _PAGE_ACCESSED);
}

#define __HAVE_ARCH_PMD_WRITE
#define pmd_write pmd_write
static inline int pmd_write(pmd_t pmd)
{
	return !(pmd_val(pmd) & _PAGE_FOW);
}

static inline pmd_t pmd_wrprotect(pmd_t pmd)
{
	pmd_val(pmd) |= _PAGE_FOW;
	return pmd;
}

static inline pmd_t pmd_mkinvalid(pmd_t pmd)
{
	pmd_val(pmd) &= ~(_PAGE_VALID | _PAGE_PROTNONE);
	return pmd;
}

static inline pmd_t pmd_mkclean(pmd_t pmd)
{
	pmd_val(pmd) &= ~(__DIRTY_BITS);
	pmd_val(pmd) |= _PAGE_FOW;
	return pmd;
}

static inline pmd_t pmd_mkold(pmd_t pmd)
{
	pmd_val(pmd) &= ~(__ACCESS_BITS);
	return pmd;
}

static inline pmd_t pmd_mkwrite_novma(pmd_t pmd)
{
	pmd_val(pmd) &= ~_PAGE_FOW;
	return pmd;
}

static inline pmd_t pmd_mkdirty(pmd_t pmd)
{
	pmd_val(pmd) |= __DIRTY_BITS;
	return pmd;
}

static inline pmd_t pmd_mkdevmap(pmd_t pmd)
{
	pmd_val(pmd) |= _PAGE_DEVMAP;
	return pmd;
}

static inline pmd_t pmd_mkyoung(pmd_t pmd)
{
	pmd_val(pmd) |= __ACCESS_BITS;
	return pmd;
}

static inline pmd_t pmd_mkhuge(pmd_t pmd)
{
	pmd_val(pmd) |= _PAGE_LEAF;
	return pmd;
}

static inline pmd_t pmd_mkcont(pmd_t pmd)
{
	pmd_val(pmd) |= _PAGE_CONT;
	return pmd;
}

static inline int pud_none(pud_t pud)
{
	return !pud_val(pud);
}

static inline int pud_bad(pud_t pud)
{
	return (pud_val(pud) & ~_PFN_MASK) != _PAGE_TABLE;
}

static inline int pud_present(pud_t pud)
{
	return !!(pud_val(pud) & _PAGE_VALID);
}

static inline void pud_clear(pud_t *pudp)
{
	pud_val(*pudp) = 0;
}

static inline pud_t pud_mkhuge(pud_t pud)
{
	pud_val(pud) |= _PAGE_LEAF;
	return pud;
}

static inline int p4d_none(p4d_t p4d)
{
	return !p4d_val(p4d);
}

static inline int p4d_bad(p4d_t p4d)
{
	return (p4d_val(p4d) & ~_PFN_MASK) != _PAGE_TABLE;
}

static inline int p4d_present(p4d_t p4d)
{
	return !!(p4d_val(p4d) & _PAGE_VALID);
}

static inline void p4d_clear(p4d_t *p4dp)
{
	p4d_val(*p4dp) = 0;
}

static inline pte_t pmd_pte(pmd_t pmd)
{
	return __pte(pmd_val(pmd));
}

static inline pmd_t pte_pmd(pte_t pte)
{
	return __pmd(pte_val(pte));
}

/*
 * The following only work if pte_present() is true.
 * Undefined behaviour if not..
 */
static inline int pte_write(pte_t pte)
{
	return !(pte_val(pte) & _PAGE_FOW);
}

static inline int pte_dirty(pte_t pte)
{
	return !!(pte_val(pte) & _PAGE_DIRTY);
}

static inline int pte_young(pte_t pte)
{
	return !!(pte_val(pte) & _PAGE_ACCESSED);
}

static inline int pte_special(pte_t pte)
{
	return !!(pte_val(pte) & _PAGE_SPECIAL);
}

static inline int pte_cont(pte_t pte)
{
	return !!(pte_val(pte) & _PAGE_CONT);
}

static inline pte_t pte_wrprotect(pte_t pte)
{
	pte_val(pte) |= _PAGE_FOW;
	return pte;
}

static inline pte_t pte_mkclean(pte_t pte)
{
	pte_val(pte) &= ~(__DIRTY_BITS);
	pte_val(pte) |= _PAGE_FOW;
	return pte;
}

static inline pte_t pte_mkold(pte_t pte)
{
	pte_val(pte) &= ~(__ACCESS_BITS);
	return pte;
}

static inline pte_t pte_mkwrite_novma(pte_t pte)
{
	pte_val(pte) &= ~_PAGE_FOW;
	return pte;
}

static inline pte_t pte_mkdirty(pte_t pte)
{
	pte_val(pte) |= __DIRTY_BITS;
	return pte;
}

static inline pte_t pte_mkyoung(pte_t pte)
{
	pte_val(pte) |= __ACCESS_BITS;
	return pte;
}

static inline pte_t pte_mkhuge(pte_t pte)
{
	pte_val(pte) |= _PAGE_LEAF;
	return pte;
}

static inline pte_t pte_mkspecial(pte_t pte)
{
	pte_val(pte) |= _PAGE_SPECIAL;
	return pte;
}

static inline pte_t pte_mkdevmap(pte_t pte)
{
	pte_val(pte) |= _PAGE_SPECIAL;
	return pte;
}

#ifdef CONFIG_NUMA_BALANCING
/*
 * See the comment in include/asm-generic/pgtable.h
 */
static inline int pte_protnone(pte_t pte)
{
	return (pte_val(pte) & (_PAGE_PROTNONE | _PAGE_VALID))
		== _PAGE_PROTNONE;
}

static inline int pmd_protnone(pmd_t pmd)
{
	return (pmd_val(pmd) & (_PAGE_PROTNONE | _PAGE_VALID))
		== _PAGE_PROTNONE;
}
#endif

#ifdef CONFIG_ARCH_HAS_PTE_DEVMAP
static inline int pte_devmap(pte_t a)
{
	return (pte_val(a) & _PAGE_DEVMAP) == _PAGE_DEVMAP;
}
#endif

static inline int pmd_cont(pmd_t pmd)
{
	return !!(pmd_val(pmd) & _PAGE_CONT);
}

#ifdef CONFIG_TRANSPARENT_HUGEPAGE

/* We don't have hardware dirty/accessed bits, generic_pmdp_establish is fine.*/
#define pmdp_establish generic_pmdp_establish

static inline int pmd_trans_splitting(pmd_t pmd)
{
	return !!(pmd_val(pmd) & _PAGE_SPLITTING);
}

static inline int pmd_trans_huge(pmd_t pmd)
{
	return !!(pmd_val(pmd) & _PAGE_LEAF);
}

static inline int has_transparent_hugepage(void)
{
	return 1;
}

#ifdef CONFIG_ARCH_HAS_PTE_DEVMAP
static inline int pmd_devmap(pmd_t pmd)
{
	return !!(pmd_val(pmd) & _PAGE_DEVMAP);
}

#ifdef CONFIG_HAVE_ARCH_TRANSPARENT_HUGEPAGE_PUD
static inline int pud_devmap(pud_t pud)
{
	return !!(pud_val(pud) & _PAGE_DEVMAP);
}
#else
static inline int pud_devmap(pud_t pud)
{
	return 0;
}
#endif

static inline int pgd_devmap(pgd_t pgd)
{
	return 0;
}
#endif
#endif /* CONFIG_TRANSPARENT_HUGEPAGE */

#define __HAVE_ARCH_PMDP_GET_AND_CLEAR
static inline pmd_t pmdp_get_and_clear(struct mm_struct *mm,
				       unsigned long addr, pmd_t *pmdp)
{
	unsigned long pmd_val = xchg(&pmdp->pmd, 0);
	pmd_t pmd = (pmd_t){pmd_val};
	return pmd;
}

#define __HAVE_ARCH_PMDP_SET_WRPROTECT
static inline void pmdp_set_wrprotect(struct mm_struct *mm,
				      unsigned long addr, pmd_t *pmdp)
{
	set_bit(_PAGE_BIT_FOW, (unsigned long *)pmdp);
}

#define mk_pmd(page, prot)	pfn_pmd(page_to_pfn(page), (prot))

#define __HAVE_ARCH_PMDP_SET_ACCESS_FLAGS
extern int pmdp_set_access_flags(struct vm_area_struct *vma,
				 unsigned long address, pmd_t *pmdp,
				 pmd_t entry, int dirty);

#define __HAVE_ARCH_PMDP_TEST_AND_CLEAR_YOUNG
extern int pmdp_test_and_clear_young(struct vm_area_struct *vma,
				     unsigned long addr, pmd_t *pmdp);

#define __HAVE_ARCH_PMDP_CLEAR_YOUNG_FLUSH
extern int pmdp_clear_flush_young(struct vm_area_struct *vma,
				  unsigned long address, pmd_t *pmdp);

#define __HAVE_ARCH_PMDP_SPLITTING_FLUSH
extern void pmdp_splitting_flush(struct vm_area_struct *vma,
				 unsigned long addr, pmd_t *pmdp);

extern pgd_t swapper_pg_dir[1024];

/*
 * The sw64 doesn't have any external MMU info:  the kernel page
 * tables contain all the necessary information.
 */
#define update_mmu_cache(vma, address, ptep) do { } while (0)
#define update_mmu_cache_pmd(vma, address, pmd) do { } while (0)

static inline void update_mmu_cache_range(struct vm_fault *vmf,
		struct vm_area_struct *vma, unsigned long address,
		pte_t *ptep, unsigned int nr)
{
}

#if defined(CONFIG_SUBARCH_C3B)

/*
 * Encode and decode a swap entry:
 *
 * Format of swap PTE:
 *	bit  0:		_PAGE_VALID (must be zero)
 *	bit  6:		_PAGE_LEAF (must be zero)
 *	bit  7:		_PAGE_PROTNONE (must be zero)
 *	bits 8-15:	swap type
 *	bits 16-63:	swap offset
 */
#define __SWP_TYPE_SHIFT	8
#define __SWP_TYPE_BITS		8

#elif defined(CONFIG_SUBARCH_C4)

/*
 * Encode and decode a swap entry:
 *
 * Format of swap PTE:
 *	bit  0:		_PAGE_VALID (must be zero)
 *	bit  6:		_PAGE_LEAF (must be zero)
 *	bits 7-11:	swap type
 *	bits 12-58:	swap offset
 *	bit  63:	_PAGE_PROTNONE (must be zero)
 */
#define __SWP_TYPE_SHIFT	7
#define __SWP_TYPE_BITS		5

#endif

#define __SWP_OFFSET_BITS	47
#define __SWP_TYPE_MASK		((1UL << __SWP_TYPE_BITS) - 1)
#define __SWP_OFFSET_SHIFT	(__SWP_TYPE_BITS + __SWP_TYPE_SHIFT)
#define __SWP_OFFSET_MASK	((1UL << __SWP_OFFSET_BITS) - 1)

#define __swp_type(x)		(((x).val >> __SWP_TYPE_SHIFT) & __SWP_TYPE_MASK)
#define __swp_offset(x)		(((x).val >> __SWP_OFFSET_SHIFT) & __SWP_OFFSET_MASK)
#define __swp_entry(type, offset) \
	((swp_entry_t) { ((type) << __SWP_TYPE_SHIFT) | ((offset) << __SWP_OFFSET_SHIFT) })

#define __pte_to_swp_entry(pte)	((swp_entry_t) { pte_val(pte) })
#define __swp_entry_to_pte(x)	((pte_t) { (x).val })

static inline int pte_swp_exclusive(pte_t pte)
{
	return !!(pte_val(pte) & _PAGE_SWP_EXCLUSIVE);
}

static inline pte_t pte_swp_mkexclusive(pte_t pte)
{
	pte_val(pte) |= _PAGE_SWP_EXCLUSIVE;
	return pte;
}

static inline pte_t pte_swp_clear_exclusive(pte_t pte)
{
	pte_val(pte) &= ~_PAGE_SWP_EXCLUSIVE;
	return pte;
}

#define kern_addr_valid(addr)	(1)

#define pte_ERROR(e) \
	pr_err("%s: %d: bad pte %016lx.\n", __FILE__, __LINE__, pte_val(e))
#define pmd_ERROR(e) \
	pr_err("%s: %d: bad pmd %016lx.\n", __FILE__, __LINE__, pmd_val(e))
#define pud_ERROR(e) \
	pr_err("%s: %d: bad pud %016lx.\n", __FILE__, __LINE__, pud_val(e))
#define pgd_ERROR(e) \
	pr_err("%s: %d: bad pgd %016lx.\n", __FILE__, __LINE__, pgd_val(e))
extern void paging_init(void);

/* We have our own get_unmapped_area to cope with ADDR_LIMIT_32BIT.  */
#define HAVE_ARCH_UNMAPPED_AREA

#endif /* _ASM_SW64_PGTABLE_H */
