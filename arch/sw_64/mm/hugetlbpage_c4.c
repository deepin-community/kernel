// SPDX-License-Identifier: GPL-2.0
/*
 * SW_64 Huge TLB Page Support for Kernel.
 *
 */

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <linux/hugetlb.h>
#include <linux/pagemap.h>
#include <linux/err.h>
#include <linux/sysctl.h>
#include <asm/mman.h>
#include <asm/tlb.h>
#include <asm/tlbflush.h>
#include <asm/pgalloc.h>


/*
 * pmd_huge() returns 1 if @pmd is hugetlb related entry, that is normal
 * hugetlb entry or non-present (migration or hwpoisoned) hugetlb entry.
 * Otherwise, returns 0.
 */
int pmd_huge(pmd_t pmd)
{
	return !pmd_none(pmd) &&
		(pmd_val(pmd) & (_PAGE_PRESENT|_PAGE_LEAF)) != _PAGE_PRESENT;
}

int pud_huge(pud_t pud)
{
	return !pud_none(pud) &&
		(pud_val(pud) & (_PAGE_PRESENT|_PAGE_LEAF)) != _PAGE_PRESENT;
}
EXPORT_SYMBOL(pud_huge);

/*
 * Select all bits except the pfn
 */
static inline pgprot_t pte_pgprot(pte_t pte)
{
	unsigned long pfn = pte_pfn(pte);

	return __pgprot(pte_val(pfn_pte(pfn, __pgprot(0))) ^ pte_val(pte));
}

static inline int num_contig_ptes(unsigned long size, size_t *pgsize)
{
	int contig_ptes = 0;

	*pgsize = size;

	switch (size) {
	case PUD_SIZE:
	case PMD_SIZE:
		contig_ptes = 1;
		break;
	case CONT_PMD_SIZE:
		*pgsize = PMD_SIZE;
		contig_ptes = CONT_PMDS;
		break;
	default:
		break;
	}

	return contig_ptes;
}

static pte_t get_and_clear(struct mm_struct *mm,
			unsigned long addr, pte_t *ptep,
			unsigned long pgsize, unsigned long ncontig)
{
	pte_t orig_pte = huge_ptep_get(ptep);
	unsigned long i;

	for (i = 0; i < ncontig; i++, addr += pgsize, ptep++) {
		pte_t pte = ptep_get_and_clear(mm, addr, ptep);

		if (pte_dirty(pte))
			orig_pte = pte_mkdirty(orig_pte);

		if (pte_young(pte))
			orig_pte = pte_mkyoung(orig_pte);
	}

	return orig_pte;
}

static pte_t get_clear_contig_flush(struct mm_struct *mm, unsigned long addr,
			pte_t *ptep, unsigned long pgsize,
			unsigned long ncontig)
{
	pte_t orig_pte = get_and_clear(mm, addr, ptep, pgsize, ncontig);
	struct vm_area_struct vma = TLB_FLUSH_VMA(mm, 0);
	unsigned long i, saddr = addr;

	for (i = 0; i < ncontig; i++, addr += pgsize, ptep++)
		pte_clear(mm, addr, ptep);

	flush_tlb_range(&vma, saddr, addr);
	return orig_pte;
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
	if (!pud)
		return NULL;

	if (sz == PUD_SIZE) {
		pte = (pte_t *)pud;
	} else if (sz == PMD_SIZE) {
		if (want_pmd_share(vma, addr) && pud_none(*pud))
			pte = huge_pmd_share(mm, vma, addr, pud);
		else
			pte = (pte_t *)pmd_alloc(mm, pud, addr);
	} else if (sz == (PMD_SIZE * CONT_PMDS)) {
		pte = (pte_t *)pmd_alloc(mm, pud, addr);
		WARN_ON(addr & (sz - 1));
	}

	WARN_ON(pte && !pte_none(*pte) && !pte_huge(*pte));
	return pte;
}

pte_t *huge_pte_offset(struct mm_struct *mm, unsigned long addr,
			unsigned long sz)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd = NULL;

	pgd = pgd_offset(mm, addr);
	if (!pgd_present(*pgd))
		return NULL;

	p4d = p4d_offset(pgd, addr);
	if (!p4d_present(*p4d))
		return NULL;

	pud = pud_offset(p4d, addr);

	if (sz != PUD_SIZE && pud_none(*pud))
		return NULL;
	/* hugepage or swap? */
	if (pud_huge(*pud) || !pud_present(*pud))
		return (pte_t *)pud;
	/* table; check the next level */

	if (sz == CONT_PMD_SIZE)
		addr &= CONT_PMD_MASK;

	pmd = pmd_offset(pud, addr);
	if (!(sz == PMD_SIZE || sz == CONT_PMD_SIZE) &&
			pmd_none(*pmd))
		return NULL;
	if (pmd_huge(*pmd) || !pmd_present(*pmd))
		return (pte_t *)pmd;

	return NULL;
}

pte_t arch_make_huge_pte(pte_t entry, unsigned int shift, vm_flags_t flags)
{
	size_t pagesize = 1UL << shift;

	if (pagesize == CONT_PMD_SIZE) {
		entry = pmd_pte(pmd_mkcont(pte_pmd(entry)));
	} else if (pagesize != PUD_SIZE && pagesize != PMD_SIZE) {
		pr_warn("%s: unrecognized huge page size 0x%lx\n",
				__func__, pagesize);
	}
	return entry;
}

void huge_pte_clear(struct mm_struct *mm, unsigned long addr,
		pte_t *ptep, unsigned long sz)
{
	int i, ncontig;
	size_t pgsize;

	ncontig = num_contig_ptes(sz, &pgsize);

	for (i = 0; i < ncontig; i++, addr += pgsize, ptep++)
		pte_clear(mm, addr, ptep);
}

void set_huge_pte_at(struct mm_struct *mm, unsigned long addr,
		pte_t *ptep, pte_t pte, unsigned long sz)
{
	size_t pgsize;
	int i;
	int ncontig;
	unsigned long pfn;
	pgprot_t hugeprot;

	/*
	 * Code needs to be expanded to handle huge swap and migration
	 * entries. Needed for HUGETLB and MEMORY_FAILURE.
	 */
	WARN_ON(!pte_present(pte));

	if (!pte_cont(pte)) {
		set_pte_at(mm, addr, ptep, pte);
		return;
	}

	ncontig = num_contig_ptes(sz, &pgsize);
	pfn = pte_pfn(pte);
	hugeprot = pte_pgprot(pte);

	get_and_clear(mm, addr, ptep, pgsize, ncontig);

	for (i = 0; i < ncontig; i++, ptep++, addr += pgsize)
		set_pte_at(mm, addr, ptep, pfn_pte(pfn, hugeprot));
}

void set_huge_swap_pte_at(struct mm_struct *mm, unsigned long addr,
		pte_t *ptep, pte_t pte, unsigned long sz)
{
	int i, ncontig;
	size_t pgsize;

	ncontig = num_contig_ptes(sz, &pgsize);

	for (i = 0; i < ncontig; i++, ptep++)
		set_pte(ptep, pte);
}

void huge_ptep_set_wrprotect(struct mm_struct *mm,
		unsigned long addr, pte_t *ptep)
{
	unsigned long pfn;
	pgprot_t hugeprot;
	int ncontig, i;
	size_t pgsize;
	pte_t pte;

	if (!pte_cont(READ_ONCE(*ptep))) {
		ptep_set_wrprotect(mm, addr, ptep);
		return;
	}

	ncontig = CONT_PMDS;

	pte = get_and_clear(mm, addr, ptep, pgsize, ncontig);
	pte = pte_wrprotect(pte);

	hugeprot = pte_pgprot(pte);
	pfn = pte_pfn(pte);

	for (i = 0; i < ncontig; i++, ptep++, addr += pgsize)
		set_pte_at(mm, addr, ptep, pfn_pte(pfn, hugeprot));
}

pte_t huge_ptep_get_and_clear(struct mm_struct *mm,
		unsigned long addr, pte_t *ptep)
{
	int ncontig;
	size_t pgsize;
	pte_t orig_pte = huge_ptep_get(ptep);

	if (!pte_cont(orig_pte))
		return ptep_get_and_clear(mm, addr, ptep);

	ncontig = CONT_PMDS;

	return get_and_clear(mm, addr, ptep, pgsize, ncontig);
}

pte_t huge_ptep_clear_flush(struct vm_area_struct *vma,
			unsigned long addr, pte_t *ptep)
{
	struct mm_struct *mm = vma->vm_mm;
	size_t pgsize;
	int ncontig;

	if (!pte_cont(READ_ONCE(*ptep)))
		return ptep_clear_flush(vma, addr, ptep);

	ncontig = CONT_PMDS;
	return get_clear_contig_flush(mm, addr, ptep, pgsize, ncontig);
}

static int __cont_access_flags_changed(pte_t *ptep, pte_t pte, int ncontig)
{
	int i;

	if (pte_write(pte) != pte_write(huge_ptep_get(ptep)))
		return 1;

	for (i = 0; i < ncontig; i++) {
		pte_t orig_pte = huge_ptep_get(ptep + i);

		if (pte_dirty(pte) != pte_dirty(orig_pte))
			return 1;

		if (pte_young(pte) != pte_young(orig_pte))
			return 1;
	}

	return 0;
}

int huge_ptep_set_access_flags(struct vm_area_struct *vma,
		unsigned long addr, pte_t *ptep,
		pte_t pte, int dirty)
{
	int ncontig, i;
	size_t pgsize = 0;
	unsigned long pfn = pte_pfn(pte);
	pgprot_t hugeprot;
	pte_t orig_pte;

	if (!pte_cont(pte))
		return ptep_set_access_flags(vma, addr, ptep, pte, dirty);

	ncontig = CONT_PMDS;

	if (!__cont_access_flags_changed(ptep, pte, ncontig))
		return 0;

	orig_pte = get_and_clear(vma->vm_mm, addr, ptep, pgsize, ncontig);
	flush_tlb_fix_spurious_fault(vma, addr, ptep);

	/* Make sure we don't lose the dirty or young state */
	if (pte_dirty(orig_pte))
		pte = pte_mkdirty(pte);

	if (pte_young(orig_pte))
		pte = pte_mkyoung(pte);

	hugeprot = pte_pgprot(pte);
	for (i = 0; i < ncontig; i++, ptep++, addr += pgsize)
		set_pte_at(vma->vm_mm, addr, ptep, pfn_pte(pfn, hugeprot));

	return 1;
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
#endif /* CONFIG_HUGETLB_PAGE */

static __init int setup_hugepagesz(char *opt)
{
	unsigned long ps = memparse(opt, &opt);

	switch (ps) {
	case PUD_SIZE:
	case PMD_SIZE * CONT_PMDS:
	case PMD_SIZE:
		hugetlb_add_hstate(ilog2(ps) - PAGE_SHIFT);
		return 1;
	}

	pr_err("hugepagesz: Unsupported page size %lu M\n",
			ps >> 20);
	return 0;
}
__setup("hugepagesz=", setup_hugepagesz);
