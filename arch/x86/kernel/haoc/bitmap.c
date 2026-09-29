// SPDX-License-Identifier: GPL-2.0
#include <asm/pgalloc.h>
#include <linux/types.h>
#include <linux/memblock.h>
#include <linux/init.h>
#include <asm/haoc/haoc-bitmap.h>

unsigned long iee_bitmap_base __ro_after_init = __VIEEBITMAP_BASE_L4;
EXPORT_SYMBOL(iee_bitmap_base);

static pte_t * __init iee_bitmap_pte_populate(pmd_t *pmd, unsigned long addr)
{
	pte_t *pte = pte_offset_kernel(pmd, addr);

	if (pte_none(*pte)) {
		void *p = (void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 0);

		if (!p)
			return NULL;
		pte_t entry = pfn_pte(__pa(p) >> PAGE_SHIFT,
				__pgprot(pgprot_val(PAGE_KERNEL) & (~__RW) & (~___D)));

		set_pte_at(&init_mm, addr, pte, entry);
	}
	return pte;
}

static pmd_t * __init iee_bitmap_pmd_populate(pud_t *pud, unsigned long addr)
{
	pmd_t *pmd = pmd_offset(pud, addr);

	if (pmd_none(*pmd)) {
#ifdef CONFIG_PTP
		void *p = iee_cache_alloc(&pg_cache, GFP_KERNEL | __GFP_ZERO);
#else
		void *p = (void *)get_zeroed_page(GFP_KERNEL);
#endif

		if (!p)
			return NULL;
		pmd_populate_kernel(&init_mm, pmd, p);
	}
	return pmd;
}

static pud_t * __init iee_bitmap_pud_populate(p4d_t *p4d, unsigned long addr)
{
	pud_t *pud = pud_offset(p4d, addr);

	if (pud_none(*pud)) {
#ifdef CONFIG_PTP
		void *p = iee_cache_alloc(&pg_cache, GFP_KERNEL | __GFP_ZERO);
#else
		void *p = (void *)get_zeroed_page(GFP_KERNEL);
#endif

		if (!p)
			return NULL;
		pud_populate(&init_mm, pud, p);
	}
	return pud;
}

static p4d_t * __init iee_bitmap_p4d_populate(pgd_t *pgd, unsigned long addr)
{
	p4d_t *p4d = p4d_offset(pgd, addr);

	if (p4d_none(*p4d)) {
#ifdef CONFIG_PTP
		void *p = iee_cache_alloc(&pg_cache, GFP_KERNEL | __GFP_ZERO);
#else
		void *p = (void *)get_zeroed_page(GFP_KERNEL);
#endif

		if (!p)
			return NULL;
		p4d_populate(&init_mm, p4d, p);
	}
	return p4d;
}

static pgd_t * __init iee_bitmap_pgd_populate(unsigned long addr)
{
	pgd_t *pgd = pgd_offset_k(addr);

	if (pgd_none(*pgd)) {
#ifdef CONFIG_PTP
		void *p = iee_cache_alloc(&pg_cache, GFP_KERNEL | __GFP_ZERO);
#else
		void *p = (void *)get_zeroed_page(GFP_KERNEL);
#endif

		if (!p)
			return NULL;
		pgd_populate(&init_mm, pgd, p);
	}
	return pgd;
}

static pte_t * __init iee_bitmap_populate(unsigned long addr)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	pgd = iee_bitmap_pgd_populate(addr);
	if (!pgd)
		return NULL;
	p4d = iee_bitmap_p4d_populate(pgd, addr);
	if (!p4d)
		return NULL;
	pud = iee_bitmap_pud_populate(p4d, addr);
	if (!pud)
		return NULL;
	pmd = iee_bitmap_pmd_populate(pud, addr);
	if (!pmd)
		return NULL;
	pte = iee_bitmap_pte_populate(pmd, addr);
	return pte;
}

static void __init iee_set_bitmap_type_init(unsigned long addr)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	phys_addr_t phys_addr;

	pgd = pgd_offset_k(addr);
	p4d = p4d_offset(pgd, addr);
	pud = pud_offset(p4d, addr);
	pmd = pmd_offset(pud, addr);
	pte = pte_offset_kernel(pmd, addr);
	phys_addr = pte_pfn(*pte) << PAGE_SHIFT;
	iee_set_bitmap_type(phys_addr, 1, IEE_BITMAP);
}

void __init iee_init_bitmap(void)
{
	unsigned long addr, start, end;
	pte_t *pte;

	start = PAGE_ALIGN_DOWN((unsigned long)(__pfn_to_iee_bitmap(0)));
	end = PAGE_ALIGN((unsigned long)(__pfn_to_iee_bitmap(max_pfn)));
	addr = start;
	for (; addr < end; addr += PAGE_SIZE) {
		pte = iee_bitmap_populate(addr);
		if (!pte)
			panic("No mem for iee bitmap init");
	}
	addr = start;
	for (; addr < end; addr += PAGE_SIZE)
		iee_set_bitmap_type_init(addr);
	pr_info("IEE bitmap: [0x%lx, 0x%lx]!", start, end);
}

static inline void verify_bitmap_ptr(uint8_t *bitmap_ptr)
{
	unsigned long addr = (unsigned long)bitmap_ptr;
	unsigned long start = PAGE_ALIGN_DOWN((unsigned long)(__pfn_to_iee_bitmap(0)));
	unsigned long end = PAGE_ALIGN((unsigned long)(__pfn_to_iee_bitmap(max_pfn)));

	if (addr < start || addr >= end)
		panic("IEE detected non-bitmap ptr: %lx", addr);
}

static uint8_t *iee_bitmap_rw_alias(uint8_t *bitmap_ptr)
{
	unsigned long addr = (unsigned long)bitmap_ptr;
	pgd_t *pgd = pgd_offset_k(addr);
	p4d_t *p4d = p4d_offset(pgd, addr);
	pud_t *pud = pud_offset(p4d, addr);
	pmd_t *pmd = pmd_offset(pud, addr);
	pte_t *pte = pte_offset_kernel(pmd, addr);
	phys_addr_t phys = (pte_pfn(*pte) << PAGE_SHIFT) | (addr & ~PAGE_MASK);

	return __va(phys);
}

void _iee_set_bitmap_type(unsigned long __unused,
				uint8_t *bitmap_ptr, enum HAOC_BITMAP_TYPE type, int num_pages)
{
	verify_bitmap_ptr(bitmap_ptr);
	if (!haoc_enabled)
		bitmap_ptr = iee_bitmap_rw_alias(bitmap_ptr);
	memset(bitmap_ptr, type, num_pages);
}
