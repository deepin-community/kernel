// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>
#include <linux/memblock.h>
#include <asm/pgalloc.h>
#include <asm/haoc/iee.h>
#include <asm/haoc/iee-init.h>
#include <asm/haoc/haoc.h>
#include <asm/haoc/haoc-bitmap.h>
#include <asm/haoc/iee-mmu.h>

bool haoc_bitmap_ready;

static void * __init haoc_bitmap_alloc_block_zero(unsigned long size)
{
	#ifdef CONFIG_PTP
	void *p = __va(early_iee_pgtable_alloc(0));

	if (!p)
		return NULL;
	#else
	void *p = memblock_alloc(size, size);

	if (!p)
		return NULL;
	memset(p, 0, size);
	#endif
	return p;
}

/* Set APTable on pgd entry of bitmap addresses to prevent kernel writes. */
static inline void bitmap_p4d_populate(struct mm_struct *mm, p4d_t *p4dp, pud_t *pudp)
{
	p4dval_t p4dval = P4D_TYPE_TABLE | P4D_TABLE_AF | PGD_APTABLE_RO;

	p4dval |= (mm == &init_mm) ? P4D_TABLE_UXN : P4D_TABLE_PXN;
	__p4d_populate(p4dp, __pa(pudp), p4dval);
}

static pte_t * __init haoc_bitmap_pte_populate(pmd_t *pmd, unsigned long addr)
{
	pte_t *pte = pte_offset_kernel(pmd, addr);

	if (pte_none(ptep_get(pte))) {
		pte_t entry;
		void *p = __va(early_iee_data_alloc(0));

		if (!p)
			return NULL;

		entry = pfn_pte(__pa(p) >> PAGE_SHIFT, PAGE_KERNEL);
		#ifdef CONFIG_PTP
		write_sysreg((read_sysreg(TCR_EL1) | TCR_HPD1) & ~TCR_A1, tcr_el1);
		isb();
		WRITE_ONCE(*__ptr_to_iee(pte), entry);
		write_sysreg((read_sysreg(TCR_EL1) & ~TCR_HPD1) | TCR_A1, tcr_el1);
		isb();
		#else
		set_pte_at(&init_mm, addr, pte, entry);
		#endif
	}
	return pte;
}

static pmd_t * __init haoc_bitmap_pmd_populate(pud_t *pud, unsigned long addr)
{
	pmd_t *pmd = pmd_offset(pud, addr);
	void *p;

	if (pmd_none(*pmd)) {
		p = haoc_bitmap_alloc_block_zero(PAGE_SIZE);
		if (!p)
			return NULL;
		pmd_populate_kernel(&init_mm, pmd, p);
	}
	return pmd;
}

static pud_t * __init haoc_bitmap_pud_populate(p4d_t *p4d, unsigned long addr)
{
	pud_t *pud = pud_offset(p4d, addr);
	void *p;

	if (pud_none(*pud)) {
		p = haoc_bitmap_alloc_block_zero(PAGE_SIZE);
		if (!p)
			return NULL;
		pmd_init(p);
		pud_populate(&init_mm, pud, p);
	}
	return pud;
}

static p4d_t * __init haoc_bitmap_p4d_populate(pgd_t *pgd, unsigned long addr)
{
	p4d_t *p4d = p4d_offset(pgd, addr);
	void *p;

	if (p4d_none(*p4d)) {
		p = haoc_bitmap_alloc_block_zero(PAGE_SIZE);
		if (!p)
			return NULL;
		pud_init(p);
		bitmap_p4d_populate(&init_mm, p4d, p);
	}
	return p4d;
}

static pgd_t * __init haoc_bitmap_pgd_populate(unsigned long addr)
{
	pgd_t *pgd = pgd_offset_k(addr);
	void *p;

	if (pgd_none(*pgd)) {
		p = haoc_bitmap_alloc_block_zero(PAGE_SIZE);
		if (!p)
			return NULL;
		pgd_populate(&init_mm, pgd, p);
	}
	return pgd;
}

/* Create mappings if that address is not mapped. */
static pte_t * __init haoc_bitmap_populate_address(unsigned long addr)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	pgd = haoc_bitmap_pgd_populate(addr);
	if (!pgd)
		return NULL;
	p4d = haoc_bitmap_p4d_populate(pgd, addr);
	if (!p4d)
		return NULL;
	pud = haoc_bitmap_pud_populate(p4d, addr);
	if (!pud)
		return NULL;
	pmd = haoc_bitmap_pmd_populate(pud, addr);
	if (!pmd)
		return NULL;
	pte = haoc_bitmap_pte_populate(pmd, addr);
	if (!pte)
		return NULL;

	return pte;
}

/* Map haoc bitmap array after vmemmap region. */
int __init haoc_bitmap_sparse_init(void)
{
	unsigned long start_pfn, end_pfn;
	int i, nid;
	/* Iterate through available memory blocks. */
	for_each_mem_pfn_range(i, MAX_NUMNODES, &start_pfn, &end_pfn, &nid) {
		u64 bitmap_start = ALIGN_DOWN(((u64)__pfn_to_haoc_bitmap(start_pfn)),
								PAGE_SIZE);
		u64 bitmap_end = ALIGN(((u64)__pfn_to_haoc_bitmap(end_pfn)),
								PAGE_SIZE);
		u64 addr = bitmap_start;
		pte_t *pte;

		for (; addr < bitmap_end; addr += PAGE_SIZE) {
			pte = haoc_bitmap_populate_address(addr);
			if (!pte)
				pr_err("HAOC: failed on bitmap init.");
		}
	}
	haoc_bitmap_ready = true;
	return 0;
}

void __init haoc_bitmap_setup(void)
{
	unsigned long __maybe_unused start;
	unsigned long __maybe_unused end;
	int __maybe_unused num_pages;

	/* Setup bitmap types of global data. */
	#ifdef CONFIG_PTP
	start = (unsigned long)idmap_pg_dir;
	end = (unsigned long)__iee_ptp_data_end;
	num_pages = (end - start) / PAGE_SIZE;
	iee_set_bitmap_type(start, num_pages, IEE_PGTABLE);
	start = (unsigned long)init_pg_dir;
	end = (unsigned long)init_pg_end;
	num_pages = (end - start) / PAGE_SIZE;
	iee_set_bitmap_type(start, num_pages, IEE_PGTABLE);
	#endif
	#ifdef CONFIG_IEE_SELINUX_P
	start = (unsigned long)__iee_selinux_data_start;
	end = (unsigned long)__iee_selinux_data_end;
	num_pages = (end - start) / PAGE_SIZE;
	iee_set_bitmap_type(start, num_pages, IEE_SELINUX);
	#endif
	#ifdef CONFIG_VARP
	/* Map .iee.varp as RO pages, the variable need to be protected can be added */
	start = (unsigned long)__iee_varp_data_start;
	end = (unsigned long)__iee_varp_data_end;
	num_pages = (end - start) / PAGE_SIZE;
	iee_set_bitmap_type(start, num_pages, IEE_VARP);
	#endif

	/* Mark bitmaps of early allocated iee data. */
	setup_iee_early_data_bitmap();
}

#pragma GCC push_options
#pragma GCC optimize("O0")
static void __iee_code _iee_bitmap_memset(void *ptr, int data, size_t n)
{
	char *_ptr;

	_ptr = (char *)ptr;

	while (n--)
		*_ptr++ = data;
}
#pragma GCC pop_options

void __iee_code _iee_set_bitmap_type(unsigned long __unused,
				u64 va, enum HAOC_BITMAP_TYPE type, int num_pages)
{
	_iee_bitmap_memset(__va_to_haoc_bitmap(va), type, num_pages);
}
