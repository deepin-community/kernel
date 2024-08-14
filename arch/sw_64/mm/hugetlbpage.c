// SPDX-License-Identifier: GPL-2.0
/*
 * SW64 Huge TLB Page Support for Kernel.
 */

#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <linux/hugetlb.h>
#include <linux/err.h>

#include <asm/mman.h>
#include <asm/tlb.h>

/*
 * pmd_huge() returns 1 if @pmd is hugetlb related entry, that is normal
 * hugetlb entry or non-present (migration or hwpoisoned) hugetlb entry.
 * Otherwise, returns 0.
 */
int pmd_huge(pmd_t pmd)
{
	return !pmd_none(pmd) &&
		(pmd_val(pmd) & (_PAGE_VALID | _PAGE_LEAF)) != _PAGE_VALID;
}

int pud_huge(pud_t pud)
{
	return 0;
}

pte_t *sw64_256m_hugepte_alloc(struct mm_struct *mm, pud_t *pud, unsigned long addr)
{
	int i;
	struct page *page;
	pmd_t *pmd;
	pte_t *pte = NULL;

	pmd = pmd_alloc(mm, pud, addr);
	if (pmd == NULL)
		return NULL;

	pte = pte_alloc_map(mm, pmd, addr);
	if (pte == NULL)
		return NULL;

	page = virt_to_page(pte);
	pmd_val(*pmd) = pmd_val(*pmd) | _PAGE_LEAF | _PAGE_CONT;
	for (i = 1; i < 32; i++)
		pmd_val(*(pmd+i)) = pmd_val(*pmd);
	return pte;
}

pte_t *huge_pte_alloc(struct mm_struct *mm, struct vm_area_struct *vma,
			unsigned long addr, unsigned long sz)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pte_t *pte = NULL;

	pgd = pgd_offset(mm, addr);
	p4d = p4d_alloc(mm, pgd, addr);
	pud = pud_alloc(mm, p4d, addr);
	if (pud) {
		if (sz == PMD_SIZE) {
			if (want_pmd_share(vma, addr) && pud_none(*pud))
				pte = huge_pmd_share(mm, vma, addr, pud);
			else
				pte = (pte_t *)pmd_alloc(mm, pud, addr);
		} else if (sz == (PMD_SIZE << 5)) {
			pte = sw64_256m_hugepte_alloc(mm, pud, addr);
		} else	{
			pr_warn("Unsupported page size %lx\n", sz);
			return NULL;
		}
	}
	BUG_ON(pte && !pte_none(*pte) && !pte_huge(*pte));

	return pte;
}

pte_t *huge_pte_offset(struct mm_struct *mm, unsigned long addr,
		unsigned long sz)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd = NULL;
	pte_t *pte = NULL;

	pgd = pgd_offset(mm, addr);
	if (pgd_present(*pgd)) {
		p4d = p4d_offset(pgd, addr);
		if (p4d_present(*p4d)) {
			pud = pud_offset(p4d, addr);
			if (pud_present(*pud)) {
				pmd = pmd_offset(pud, addr);
				if (sz == CONT_PMD_SIZE) {
					if (!pmd_none(*pmd))
						pte = pte_offset_map(pmd, addr);
					else
						return NULL;
				} else if (sz == PMD_SIZE)
					pte = (pte_t *) pmd;
			}
		}
	}
	return pte;
}

static inline int sw64_huge_pmd_bad(pmd_t pmd)
{
	return !(((pmd_val(pmd) & ~_PFN_MASK) == _PAGE_TABLE) ||
			((pmd_val(pmd) & _PAGE_CONT) == _PAGE_CONT));
}

static inline int sw64_huge_pmd_none_or_clear_bad(pmd_t *pmd)
{
	if (pmd_none(*pmd))
		return 1;
	if (unlikely(sw64_huge_pmd_bad(*pmd))) {
		pmd_clear_bad(pmd);
		return 1;
	}
	return 0;
}

static void sw64_huge_free_pte_range(struct mmu_gather *tlb, pmd_t *pmd,
		unsigned long addr)
{
	if ((((unsigned long)pmd & 0xffUL) == 0) &&
		((pmd_val(*pmd) & _PAGE_CONT) == _PAGE_CONT)) {
		pgtable_t token = pmd_pgtable(*pmd);

		pmd_clear(pmd);
		pte_free_tlb(tlb, token, addr);
		mm_dec_nr_ptes(tlb->mm);
	} else {
		pmd_clear(pmd);
	}
}

static inline void sw64_huge_free_pmd_range(struct mmu_gather *tlb, pud_t *pud,
		unsigned long addr, unsigned long end,
		unsigned long floor, unsigned long ceiling)
{
	pmd_t *pmd;
	unsigned long next;
	unsigned long start;

	start = addr;
	pmd = pmd_offset(pud, addr);
	do {
		next = pmd_addr_end(addr, end);
		if (sw64_huge_pmd_none_or_clear_bad(pmd))
			continue;
		sw64_huge_free_pte_range(tlb, pmd, addr);
	} while (pmd++, addr = next, addr != end);

	start &= PUD_MASK;
	if (start < floor)
		return;
	if (ceiling) {
		ceiling &= PUD_MASK;
		if (!ceiling)
			return;
	}
	if (end - 1 > ceiling - 1)
		return;

	pmd = pmd_offset(pud, start);
	pud_clear(pud);
	pmd_free_tlb(tlb, pmd, start);
	mm_dec_nr_pmds(tlb->mm);
}

static inline void sw64_huge_free_pud_range(struct mmu_gather *tlb, p4d_t *p4d,
		unsigned long addr, unsigned long end,
		unsigned long floor, unsigned long ceiling)
{
	pud_t *pud;
	unsigned long next;
	unsigned long start;

	start = addr;
	pud = pud_offset(p4d, addr);
	do {
		next = pud_addr_end(addr, end);
		if (pud_none_or_clear_bad(pud))
			continue;
		sw64_huge_free_pmd_range(tlb, pud, addr, next, floor, ceiling);
	} while (pud++, addr = next, addr != end);

	start &= PGDIR_MASK;
	if (start < floor)
		return;
	if (ceiling) {
		ceiling &= PGDIR_MASK;
		if (!ceiling)
			return;
	}
	if (end - 1 > ceiling - 1)
		return;

	pud = pud_offset(p4d, start);
	p4d_clear(p4d);
	pud_free_tlb(tlb, pud, start);
	mm_dec_nr_puds(tlb->mm);
}

#ifdef CONFIG_HUGETLB_PAGE
static unsigned long hugetlb_get_unmapped_area_bottomup(struct file *file,
		unsigned long addr, unsigned long len,
		unsigned long pgoff, unsigned long flags)
{
	struct hstate *h = hstate_file(file);
	struct vm_unmapped_area_info info;

	info.flags = 0;
	info.length = len;
	info.low_limit = current->mm->mmap_legacy_base;
	info.high_limit = TASK_SIZE;
	info.align_mask = PAGE_MASK & ~huge_page_mask(h);
	info.align_offset = 0;
	return vm_unmapped_area(&info);
}

static unsigned long hugetlb_get_unmapped_area_topdown(struct file *file,
		unsigned long addr0, unsigned long len,
		unsigned long pgoff, unsigned long flags)
{
	struct hstate *h = hstate_file(file);
	struct vm_unmapped_area_info info;
	unsigned long addr;

	info.flags = VM_UNMAPPED_AREA_TOPDOWN;
	info.length = len;
	info.low_limit = PAGE_SIZE;
	info.high_limit = current->mm->mmap_base;
	info.align_mask = PAGE_MASK & ~huge_page_mask(h);
	info.align_offset = 0;
	addr = vm_unmapped_area(&info);

	/*
	 * A failed mmap() very likely causes application failure,
	 * so fall back to the bottom-up function here. This scenario
	 * can happen with large stack limits and large mmap()
	 * allocations.
	 */
	if (addr & ~PAGE_MASK) {
		VM_BUG_ON(addr != -ENOMEM);
		info.flags = 0;
		info.low_limit = TASK_UNMAPPED_BASE;
		info.high_limit = TASK_SIZE;
		addr = vm_unmapped_area(&info);
	}

	return addr;
}

unsigned long
hugetlb_get_unmapped_area(struct file *file, unsigned long addr,
		unsigned long len, unsigned long pgoff, unsigned long flags)
{
	struct hstate *h = hstate_file(file);
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;

	if (len & ~huge_page_mask(h))
		return -EINVAL;
	if (len > TASK_SIZE)
		return -ENOMEM;

	if (flags & MAP_FIXED) {
		if (prepare_hugepage_range(file, addr, len))
			return -EINVAL;
		return addr;
	}

	if (addr) {
		addr = ALIGN(addr, huge_page_size(h));
		vma = find_vma(mm, addr);
		if (TASK_SIZE - len >= addr &&
		    (!vma || addr + len <= vma->vm_start))
			return addr;
	}
	if (mm->get_unmapped_area == arch_get_unmapped_area)
		return hugetlb_get_unmapped_area_bottomup(file, addr, len,
				pgoff, flags);
	else
		return hugetlb_get_unmapped_area_topdown(file, addr, len,
				pgoff, flags);
}

#if (defined(CONFIG_FORCE_MAX_ZONEORDER) && (CONFIG_FORCE_MAX_ZONEORDER >= 16))
static __init int sw64_256m_hugetlb_init(void)
{
	if (!size_to_hstate(1UL << (PMD_SHIFT + 5)))
		hugetlb_add_hstate(PMD_SHIFT + 5 - PAGE_SHIFT);
	return 0;
}
arch_initcall(sw64_256m_hugetlb_init);
#endif
#endif /* CONFIG_HUGETLB_PAGE */

bool __init arch_hugetlb_valid_size(unsigned long size)
{
	switch (size) {
	case PMD_SIZE:
	case (PMD_SIZE<<5):
		return true;
	}

	return false;
}
