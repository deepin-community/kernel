// SPDX-License-Identifier: GPL-2.0

#include <linux/pagewalk.h>
#include <linux/pgtable.h>
#include <linux/hugetlb.h>

#include <asm/tlbflush.h>
#include <asm/set_memory.h>

struct pageattr_masks {
	pgprot_t set_mask;
	pgprot_t clear_mask;
};

static unsigned long
set_pageattr_masks(unsigned long val, struct mm_walk *walk)
{
	struct pageattr_masks *masks = walk->private;
	unsigned long new_val = val;

	new_val &= ~(pgprot_val(masks->clear_mask));
	new_val |= (pgprot_val(masks->set_mask));

	return new_val;
}

static int pageattr_pud_entry(pud_t *pud, unsigned long addr,
			      unsigned long next, struct mm_walk *walk)
{
	pud_t val = READ_ONCE(*pud);

	if (pud_huge(val)) {
		val = __pud(set_pageattr_masks(pud_val(val), walk));
		set_pud(pud, val);
	}

	return 0;
}

static int pageattr_pmd_entry(pmd_t *pmd, unsigned long addr,
			      unsigned long next, struct mm_walk *walk)
{
	pmd_t val = READ_ONCE(*pmd);

	if (pmd_huge(val)) {
		val = __pmd(set_pageattr_masks(pmd_val(val), walk));
		set_pmd(pmd, val);
	}

	return 0;
}

static int pageattr_pte_entry(pte_t *pte, unsigned long addr,
			      unsigned long next, struct mm_walk *walk)
{
	pte_t val = READ_ONCE(*pte);

	val = __pte(set_pageattr_masks(pte_val(val), walk));
	set_pte(pte, val);

	return 0;
}

static const struct mm_walk_ops pageattr_ops = {
	.pud_entry = pageattr_pud_entry,
	.pmd_entry = pageattr_pmd_entry,
	.pte_entry = pageattr_pte_entry,
};

static int __set_memory(unsigned long addr, int numpages, pgprot_t set_mask,
			pgprot_t clear_mask)
{
	int ret;
	unsigned long start = addr;
	unsigned long end = start + PAGE_SIZE * numpages;
	struct pageattr_masks masks = {
		.set_mask = set_mask,
		.clear_mask = clear_mask
	};

	if (!numpages)
		return 0;

	mmap_write_lock(&init_mm);
	ret = walk_page_range_novma(&init_mm, start, end, &pageattr_ops, NULL,
				    &masks);
	mmap_write_unlock(&init_mm);

	local_flush_tlb_all();

	return ret;
}

int set_memory_ro(unsigned long addr, int numpages)
{
	if (sunway_support_kpt)
		return __set_memory(addr, numpages, __pgprot(_PAGE_FOW),
				    __pgprot(_PAGE_FOR));
	else
		return 0;
}

int set_memory_rw(unsigned long addr, int numpages)
{
	if (sunway_support_kpt)
		return __set_memory(addr, numpages, __pgprot(0),
				    __pgprot(_PAGE_FOR | _PAGE_FOW));
	else
		return 0;
}

int set_memory_x(unsigned long addr, int numpages)
{
	if (sunway_support_kpt)
		return __set_memory(addr, numpages, __pgprot(0),
				    __pgprot(_PAGE_FOE));
	else
		return 0;
}

int set_memory_nx(unsigned long addr, int numpages)
{
	if (sunway_support_kpt)
		return __set_memory(addr, numpages, __pgprot(_PAGE_FOE),
				    __pgprot(0));
	else
		return 0;
}
