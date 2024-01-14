// SPDX-License-Identifier: GPL-2.0-only
/*
 * enc_dec_user_memory.c - A new interface for handling user space pages
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: yangwencheng <yangwencheng@hygon.cn>
 *
 * Derived from arch/x86/mm/pat/set_memory.c
 * Original Copyright (C) 2002 Andi Kleen, SuSE Labs.
 *
 * Copyright 2002 Andi Kleen, SuSE Labs.
 * Thanks to Ben LaHaise for precious feedback.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/highmem.h>
#include <linux/memblock.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/pfn.h>
#include <linux/gfp.h>
#include <linux/vmalloc.h>

#include <asm/processor.h>
#include <asm/tlbflush.h>
#include <asm/sections.h>
#include <asm/setup.h>
#include <linux/uaccess.h>
#include <asm/pgalloc.h>
#include <asm/proto.h>
#include <asm/memtype.h>
#include <asm/set_memory.h>
#include <asm/csv.h>
#include <linux/hugetlb.h>

#include "../mm_internal.h"

/*
 * The current flushing context - we pass it instead of 5 arguments:
 */
struct cpa_data {
	unsigned long	 *vaddr;
	pgd_t		 *pgd;
	pgprot_t	 mask_set;
	pgprot_t	 mask_clr;
	unsigned long	 numpages;
	unsigned long	 curpage;
	unsigned long	 pfn;
	unsigned int	 is_enc;
	unsigned long	 page_cache_size;
	unsigned long	 page_cache_idx;
	struct page	 **page_cache;
	void		 *cpy_buf;
	struct mm_struct *mm;
};

static inline unsigned long fix_addr(unsigned long addr)
{
#ifdef CONFIG_X86_64
	return (long)(addr << 1) >> 1;
#else
	return addr;
#endif
}

static unsigned long __cpa_addr(struct cpa_data *cpa, unsigned long idx)
{
	return *cpa->vaddr + idx * PAGE_SIZE;
}

static DEFINE_MUTEX(user_mutex);
static void cpa_page_cache_flush(struct cpa_data *cpa)
{
	void *src, *dst;
	struct page **pages = cpa->page_cache;
	unsigned long page_cnt = cpa->page_cache_idx;
	void *buf = cpa->cpy_buf;
	bool is_enc = cpa->is_enc;

	if (cpa->page_cache_idx == 0)
		return;

	src = is_enc ? vmap(pages, page_cnt, 0, PAGE_KERNEL_NOENC)
		     : vmap(pages, page_cnt, 0, PAGE_KERNEL);
	dst = is_enc ? vmap(pages, page_cnt, 0, PAGE_KERNEL)
		     : vmap(pages, page_cnt, 0, PAGE_KERNEL_NOENC);
	BUG_ON(!src || !dst);

	mutex_lock(&user_mutex);

	memcpy(buf, src, page_cnt * PAGE_SIZE);
	clflush_cache_range(src, page_cnt * PAGE_SIZE);

	if (csv3_active())
		csv_memory_enc_dec((u64)src, page_cnt, is_enc);

	memcpy(dst, buf, page_cnt * PAGE_SIZE);

	mutex_unlock(&user_mutex);

	x86_platform.guest.enc_status_change_finish((unsigned long)dst,
						    (int)page_cnt, is_enc);
	vunmap(src);
	vunmap(dst);

	cpa->page_cache_idx = 0;
}

static void cpa_page_cache_insert(struct cpa_data *cpa, struct page *page)
{
	BUG_ON(cpa->page_cache_idx >= cpa->page_cache_size);

	/* aVoid repeating dec/enc page unless it's the last shot */
	if (cpa->is_enc) {
		if (page_private(page) > 0)
			return;
	} else {
		if (page_private(page) > 1)
			return;
	}

	cpa->page_cache[cpa->page_cache_idx++] = page;

	if (cpa->page_cache_idx == cpa->page_cache_size)
		cpa_page_cache_flush(cpa);
}

static pte_t *_lookup_address_cpa(struct cpa_data *cpa, unsigned long address,
				  unsigned int *level)
{
	if (cpa->pgd)
		return lookup_address_in_pgd(cpa->pgd + pgd_index(address),
					     address, level);

	return NULL;
}

static pgprot_t pgprot_clear_protnone_bits(pgprot_t prot)
{
	/*
	 * _PAGE_GLOBAL means "global page" for present PTEs.
	 * But, it is also used to indicate _PAGE_PROTNONE
	 * for non-present PTEs.
	 *
	 * This ensures that a _PAGE_GLOBAL PTE going from
	 * present to non-present is not confused as
	 * _PAGE_PROTNONE.
	 */
	if (!(pgprot_val(prot) & _PAGE_PRESENT))
		pgprot_val(prot) &= ~_PAGE_GLOBAL;

	return prot;
}

static int should_split_large_page(pte_t *kpte, unsigned long address,
				   struct cpa_data *cpa)
{
	unsigned long numpages, pmask, psize, lpaddr, pfn, old_pfn;
	pgprot_t old_prot, req_prot;
	pte_t new_pte, *tmp;
	enum pg_level level;
	struct page *page;
	spinlock_t *ptl;
	int i;

	/*
	 * Check for races, another CPU might have split this page
	 * up already:
	 */
	tmp = _lookup_address_cpa(cpa, address, &level);
	if (tmp != kpte)
		return 1;

	switch (level) {
	case PG_LEVEL_2M:
		old_prot = pmd_pgprot(*(pmd_t *)kpte);
		old_pfn = pmd_pfn(*(pmd_t *)kpte);
		break;
	case PG_LEVEL_1G:
		old_prot = pud_pgprot(*(pud_t *)kpte);
		old_pfn = pud_pfn(*(pud_t *)kpte);
		break;
	default:
		return -EINVAL;
	}

	psize = page_level_size(level);
	pmask = page_level_mask(level);

	/*
	 * Calculate the number of pages, which fit into this large
	 * page starting at address:
	 */
	lpaddr = (address + psize) & pmask;
	numpages = (lpaddr - address) >> PAGE_SHIFT;
	if (numpages < cpa->numpages)
		cpa->numpages = numpages;

	/*
	 * We are safe now. Check whether the new pgprot is the same:
	 * Convert protection attributes to 4k-format, as cpa->mask* are set
	 * up accordingly.
	 */

	/* Clear PSE (aka _PAGE_PAT) and move PAT bit to correct position */
	req_prot = pgprot_large_2_4k(old_prot);

	pgprot_val(req_prot) &= ~pgprot_val(cpa->mask_clr);
	pgprot_val(req_prot) |= pgprot_val(cpa->mask_set);

	/*
	 * req_prot is in format of 4k pages. It must be converted to large
	 * page format: the caching mode includes the PAT bit located at
	 * different bit positions in the two formats.
	 */
	req_prot = pgprot_4k_2_large(req_prot);
	req_prot = pgprot_clear_protnone_bits(req_prot);
	if (pgprot_val(req_prot) & _PAGE_PRESENT)
		pgprot_val(req_prot) |= _PAGE_PSE;

	/*
	 * old_pfn points to the large page base pfn. So we need to add the
	 * offset of the virtual address:
	 */
	pfn = old_pfn + ((address & (psize - 1)) >> PAGE_SHIFT);
	cpa->pfn = pfn;

	/*
	 * Calculate the large page base address and the number of 4K pages
	 * in the large page
	 */
	lpaddr = address & pmask;
	numpages = psize >> PAGE_SHIFT;

	/*
	 * Optimization: If the requested pgprot is the same as the current
	 * pgprot, then the large page can be preserved and no updates are
	 * required independent of alignment and length of the requested
	 * range. The above already established that the current pgprot is
	 * correct, which in consequence makes the requested pgprot correct
	 * as well if it is the same. The static protection scan below will
	 * not come to a different conclusion.
	 */
	if (pgprot_val(req_prot) == pgprot_val(old_prot))
		return 0;

	/*
	 * If the requested range does not cover the full page, split it up
	 */
	if (address != lpaddr || cpa->numpages != numpages)
		return 1;

	/* All checks passed. Update the large page mapping. */
	new_pte = pfn_pte(old_pfn, req_prot);
	cpa_page_cache_flush(cpa);
	page = pfn_to_page(old_pfn);
	for (i = 0; i < cpa->numpages; i++)
		cpa_page_cache_insert(cpa, page++);

	ptl = pmd_lock(cpa->mm, (pmd_t *)kpte);
	set_pte_atomic(kpte, new_pte);
	spin_unlock(ptl);

	return 0;
}

static int __change_page_attr(struct cpa_data *cpa)
{
	unsigned long address;
	int do_split;
	unsigned int level;
	pte_t *kpte, old_pte;
	struct vm_area_struct *vma;
	unsigned long pfn;
	pte_t new_pte;

	address = __cpa_addr(cpa, cpa->curpage);
repeat:
	kpte = _lookup_address_cpa(cpa, address, &level);
	if (!kpte || pte_none(*kpte)) {
		pr_info("addr 0x%lx doesn't exist, shouldn't happen\n", address);
		return -EFAULT;
	}

	old_pte = *kpte;
	if (level == PG_LEVEL_4K) {
		pgprot_t new_prot = pte_pgprot(old_pte);

		pfn = pte_pfn(old_pte);

		pgprot_val(new_prot) &= ~pgprot_val(cpa->mask_clr);
		pgprot_val(new_prot) |= pgprot_val(cpa->mask_set);

		new_prot = pgprot_clear_protnone_bits(new_prot);

		/*
		 * We need to keep the pfn from the existing PTE,
		 * after all we're only going to change it's attributes
		 * not the memory it points to
		 */
		new_pte = pfn_pte(pfn, new_prot);
		cpa->pfn = pfn;
		/*
		 * Do we really change anything ?
		 */
		if (pte_val(old_pte) != pte_val(new_pte)) {
			cpa_page_cache_insert(cpa, pte_page(old_pte));
			cpa_page_cache_flush(cpa);
			//pte_lockptr()
			set_pte_atomic(kpte, new_pte);
		}
		cpa->numpages = 1;
		return 0;
	}

	/*
	 * Check, whether we can keep the large page intact
	 * and just change the pte:
	 */
	do_split = should_split_large_page(kpte, address, cpa);
	/*
	 * When the range fits into the existing large page,
	 * return. cp->numpages and cpa->tlbflush have been updated in
	 * try_large_page:
	 */
	if (do_split <= 0)
		return do_split;

	vma = find_vma(cpa->mm, address);
	__split_huge_pmd(vma, (pmd_t *)kpte, address, false, NULL);

	goto repeat;
}

static int __change_page_attr_set_clr(struct cpa_data *cpa)
{
	unsigned long numpages = cpa->numpages;
	unsigned long rempages = numpages;
	int ret = 0;

	while (rempages) {
		/*
		 * Store the remaining nr of pages for the large page
		 * preservation check.
		 */
		cpa->numpages = rempages;

		mmap_write_lock(cpa->mm);
		ret = __change_page_attr(cpa);
		mmap_write_unlock(cpa->mm);
		if (ret)
			goto out;

		/*
		 * Adjust the number of pages with the result of the
		 * CPA operation. Either a large page has been
		 * preserved or a single page update happened.
		 */
		BUG_ON(cpa->numpages > rempages || !cpa->numpages);
		rempages -= cpa->numpages;
		cpa->curpage += cpa->numpages;
	}

	cpa_page_cache_flush(cpa);
out:
	/* Restore the original numpages */
	cpa->numpages = numpages;
	return ret;
}

static int __set_memory_enc_dec_userspace(struct mm_struct *mm, unsigned long addr,
					  int numpages, bool enc)
{
	struct cpa_data cpa;
	struct page **pages = NULL;
	void *cpy_buf = NULL;
	size_t pages_size, cpy_buf_size;
	unsigned long page_cnt;
	int ret;

	/* Nothing to do if memory encryption is not active */
	if (!cc_platform_has(CC_ATTR_MEM_ENCRYPT))
		return 0;

	/* Should not be working on unaligned addresses */
	if (WARN_ONCE(addr & ~PAGE_MASK, "misaligned address: %#lx\n", addr))
		addr &= PAGE_MASK;

	if (!mm) {
		pr_err("Enc/Dec for user but no mm input\n");
		return -EINVAL;
	}

	if (unlikely(!access_ok((const void __user *)addr, numpages * PAGE_SIZE)))
		return -EINVAL;

	if (numpages >= PTRS_PER_PMD)
		page_cnt = PTRS_PER_PMD;
	else
		page_cnt = numpages;

	pages_size = sizeof(struct page *) * page_cnt;
	cpy_buf_size = page_cnt * PAGE_SIZE;

	pages = vzalloc(pages_size);
	if (!pages)
		return -ENOMEM;

	cpy_buf = vzalloc(cpy_buf_size);
	if (!cpy_buf) {
		vfree(pages);
		return -ENOMEM;
	}

	memset(&cpa, 0, sizeof(cpa));

	cpa.vaddr = &addr;
	cpa.numpages = numpages;
	cpa.page_cache = pages;
	cpa.page_cache_size = page_cnt;
	cpa.page_cache_idx = 0;
	cpa.cpy_buf = cpy_buf;
	cpa.mask_set = enc ? __pgprot(_PAGE_ENC) : __pgprot(0);
	cpa.mask_clr = enc ? __pgprot(0) : __pgprot(_PAGE_ENC);
	cpa.is_enc = enc ? 1 : 0;
	cpa.mm = mm;
	cpa.pgd = mm->pgd;

	flush_tlb_all();

	ret = __change_page_attr_set_clr(&cpa);

	flush_tlb_all();

	vfree(pages);
	vfree(cpy_buf);

	return ret;
}

int set_memory_encrypted_userspace(struct mm_struct *mm, unsigned long addr, int numpages)
{
	return __set_memory_enc_dec_userspace(mm, addr, numpages, true);
}
EXPORT_SYMBOL_GPL(set_memory_encrypted_userspace);

int set_memory_decrypted_userspace(struct mm_struct *mm, unsigned long addr, int numpages)
{
	return __set_memory_enc_dec_userspace(mm, addr, numpages, false);
}
EXPORT_SYMBOL_GPL(set_memory_decrypted_userspace);
