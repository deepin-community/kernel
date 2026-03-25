// SPDX-License-Identifier: GPL-2.0
/*
 * Helper routines for building identity mapping page tables. This is
 * included by both the compressed kernel and the regular kernel.
 */

static inline void ptp_set_pte_pre_init(pte_t *ptep, pte_t pte)
{
	WRITE_ONCE(*ptep, pte);
}

static inline void ptp_set_pmd_pre_init(pmd_t *pmdp, pmd_t pmd)
{
	WRITE_ONCE(*pmdp, pmd);
}

static inline void ptp_set_pud_pre_init(pud_t *pudp, pud_t pud)
{
	WRITE_ONCE(*pudp, pud);
}

static inline void ptp_set_p4d_pre_init(p4d_t *p4dp, p4d_t p4d)
{
	pgd_t pgd;

	if (pgtable_l5_enabled() || !IS_ENABLED(CONFIG_PAGE_TABLE_ISOLATION)) {
		WRITE_ONCE(*p4dp, p4d);
		return;
	}

	pgd = native_make_pgd(native_p4d_val(p4d));
	pgd = pti_set_user_pgtbl((pgd_t *)p4dp, pgd);
	WRITE_ONCE(*p4dp, native_make_p4d(native_pgd_val(pgd)));
}

static inline void ptp_set_pgd_pre_init(pgd_t *pgdp, pgd_t pgd)
{
	WRITE_ONCE(*pgdp, pti_set_user_pgtbl(pgdp, pgd));
}

static void ident_pmd_init(struct x86_mapping_info *info, pmd_t *pmd_page,
			   unsigned long addr, unsigned long end)
{
	addr &= PMD_MASK;
	for (; addr < end; addr += PMD_SIZE) {
		pmd_t *pmd = pmd_page + pmd_index(addr);

		if (pmd_present(*pmd))
			continue;

		ptp_set_pmd_pre_init(pmd, __pmd((addr - info->offset) | info->page_flag));
	}
}

static int ident_pud_init(struct x86_mapping_info *info, pud_t *pud_page,
			  unsigned long addr, unsigned long end)
{
	unsigned long next;

	for (; addr < end; addr = next) {
		pud_t *pud = pud_page + pud_index(addr);
		pmd_t *pmd;

		next = (addr & PUD_MASK) + PUD_SIZE;
		if (next > end)
			next = end;

		if (info->direct_gbpages) {
			pud_t pudval;

			if (pud_present(*pud))
				continue;

			addr &= PUD_MASK;
			pudval = __pud((addr - info->offset) | info->page_flag);
			ptp_set_pud_pre_init(pud, pudval);
			continue;
		}

		if (pud_present(*pud)) {
			pmd = pmd_offset(pud, 0);
			ident_pmd_init(info, pmd, addr, next);
			continue;
		}
		pmd = (pmd_t *)info->alloc_pgt_page(info->context);
		if (!pmd)
			return -ENOMEM;
		ident_pmd_init(info, pmd, addr, next);
		ptp_set_pud_pre_init(pud, __pud(__pa(pmd) | info->kernpg_flag));
	}

	return 0;
}

static int ident_p4d_init(struct x86_mapping_info *info, p4d_t *p4d_page,
			  unsigned long addr, unsigned long end)
{
	unsigned long next;
	int result;

	for (; addr < end; addr = next) {
		p4d_t *p4d = p4d_page + p4d_index(addr);
		pud_t *pud;

		next = (addr & P4D_MASK) + P4D_SIZE;
		if (next > end)
			next = end;

		if (p4d_present(*p4d)) {
			pud = pud_offset(p4d, 0);
			result = ident_pud_init(info, pud, addr, next);
			if (result)
				return result;

			continue;
		}
		pud = (pud_t *)info->alloc_pgt_page(info->context);
		if (!pud)
			return -ENOMEM;

		result = ident_pud_init(info, pud, addr, next);
		if (result)
			return result;

		ptp_set_p4d_pre_init(p4d, __p4d(__pa(pud) | info->kernpg_flag));
	}

	return 0;
}

int ptp_kernel_ident_mapping_init(struct x86_mapping_info *info, pgd_t *pgd_page,
			      unsigned long pstart, unsigned long pend)
{
	unsigned long addr = pstart + info->offset;
	unsigned long end = pend + info->offset;
	unsigned long next;
	int result;

	/* Set the default pagetable flags if not supplied */
	if (!info->kernpg_flag)
		info->kernpg_flag = _KERNPG_TABLE;

	/* Filter out unsupported __PAGE_KERNEL_* bits: */
	info->kernpg_flag &= __default_kernel_pte_mask;

	for (; addr < end; addr = next) {
		pgd_t *pgd = pgd_page + pgd_index(addr);
		p4d_t *p4d;

		next = (addr & PGDIR_MASK) + PGDIR_SIZE;
		if (next > end)
			next = end;

		if (pgd_present(*pgd)) {
			p4d = p4d_offset(pgd, 0);
			result = ident_p4d_init(info, p4d, addr, next);
			if (result)
				return result;
			continue;
		}

		p4d = (p4d_t *)info->alloc_pgt_page(info->context);
		if (!p4d)
			return -ENOMEM;
		result = ident_p4d_init(info, p4d, addr, next);
		if (result)
			return result;
		if (pgtable_l5_enabled()) {
			ptp_set_pgd_pre_init(pgd, __pgd(__pa(p4d) | info->kernpg_flag));
		} else {
			/*
			 * With p4d folded, pgd is equal to p4d.
			 * The pgd entry has to point to the pud page table in this case.
			 */
			pud_t *pud = pud_offset(p4d, 0);

			ptp_set_pgd_pre_init(pgd, __pgd(__pa(pud) | info->kernpg_flag));
		}
	}

	return 0;
}
