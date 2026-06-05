// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/hugetlb.h>
#include <asm/cmpxchg.h>
#include <asm/haoc/haoc-def.h>
#include <asm/haoc/haoc-bitmap.h>
#include <asm/haoc/iee.h>

#ifdef CONFIG_PTP_S
static inline bool is_changing_pte_prot(pte_t *ptep, pte_t pte);
static inline bool check_pte_dep(char *addr, pte_t pte);
static inline bool check_pmd_dep(char *addr, pmd_t pmd);

bool ptp_is_user_pgtable(const void *ptp)
{
	unsigned long check_addr = (unsigned long)ptp;

	if (!(haoc_enabled && haoc_bitmap_ready && iee_init_done))
		return false;

	/*
	 * iee_get_bitmap_type() relies on __va_to_haoc_bitmap().
	 * Normalize IEE alias addresses back to linear-map address first.
	 */
	if (unlikely(!__is_lm_address((u64)check_addr))) {
		if (!(check_addr & IEE_OFFSET))
			return false;

		check_addr = (unsigned long)__iee_to_virt(check_addr);
		if (unlikely(!__is_lm_address((u64)check_addr)))
			return false;
	}

	if (unlikely(!virt_addr_valid((void *)check_addr)))
		return false;

	return iee_get_bitmap_type(check_addr) == IEE_USER_PGTABLE;
}

/*
 * User fast-path pre-check for PTE updates under CONFIG_PTP_S.
 * Preserve alias/integrity checks before direct write.
 */
void ptp_user_check_pte_update(pte_t *ptep, pte_t pte)
{
	char *addr = (char *)__phys_to_kimg(__pte_to_phys(pte));

	if (!(pte_val(pte) & PTE_VALID))
		return;

	if (haoc_enabled && !is_changing_pte_prot(ptep, pte) &&
		check_addr_in_iee_valid(__phys_to_iee(__pte_to_phys(pte))))
		panic("You are remmaping IEE page to other VA.\n");

	// Avoid mapping a writable VA to kernel code PA.
	check_pte_dep(addr, pte);

}

/*
 * User fast-path pre-check for PMD updates under CONFIG_PTP_S.
 * For leaf PMD mappings, preserve alias checking before direct write.
 */
void ptp_user_check_pmd_update(pmd_t *pmdp, pmd_t pmd)
{
	char *addr = (char *)__phys_to_kimg(__pmd_to_phys(pmd));

	if (!(pmd_val(pmd) & PMD_SECT_VALID))
		return;

	// Check if the pte table is legally allocated.
	if (haoc_enabled && (pmd_val(pmd) & PMD_TABLE_BIT) &&
			!check_addr_in_iee_valid(__phys_to_iee(__pmd_to_phys(pmd))))
		panic("You can't use non-iee-pgtable\n");

	// Avoid mapping a huge pmd to IEE physical page.
	// if(!(pmd_val(pmd) & PMD_TABLE_BIT) && check_addr_range_in_iee_valid(pmd))
	//  panic("Mapping IEE physical page to a huge pmd.\n");

	check_pmd_dep(addr, pmd);
}

/*
 * User fast-path pre-check for PUD updates under CONFIG_PTP_S.
 * For leaf PUD mappings, preserve alias checking before direct write.
 */
void ptp_user_check_pud_update(pud_t *pudp, pud_t pud)
{
	if (!(pud_val(pud) & PMD_SECT_VALID))
		return;

	if (haoc_enabled && (pud_val(pud) & PMD_TABLE_BIT) &&
		!check_addr_in_iee_valid(__phys_to_iee(__pud_to_phys(pud))))
		panic("You can't use non-iee-pgtable\n");
}

/*
 * User fast-path pre-check for P4D/PGD updates under CONFIG_PTP_S.
 * A bad top-level table entry poisons later fault walks before lower-level
 * checks can run, so keep the same target-page validation here too.
 */
void ptp_user_check_p4d_update(p4d_t *p4dp, p4d_t p4d)
{
	if (!(p4d_val(p4d) & PMD_SECT_VALID))
		return;

	if (haoc_enabled && (p4d_val(p4d) & PMD_TABLE_BIT) &&
		!check_addr_in_iee_valid(__phys_to_iee(__p4d_to_phys(p4d))))
		panic("You can't use non-iee-pgtable\n");
}
#endif

bool check_addr_in_iee_valid(unsigned long addr)
{
	pgd_t *pgdir = swapper_pg_dir;

	pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);
	p4d_t *p4dp = p4d_offset(pgdp, addr);
	pud_t *pudp;
	pmd_t *pmdp;
	pte_t *ptep;

	if (!(p4d_val(READ_ONCE(*p4dp)) & PTE_VALID))
		return false;

	pudp = pud_offset(p4dp, addr);

	if (!(pud_val(READ_ONCE(*pudp)) & PTE_VALID))
		return false;

	pmdp = pmd_offset(pudp, addr);

	if (!(pmd_val(READ_ONCE(*pmdp)) & PTE_VALID))
		return false;

	ptep = pte_offset_kernel(pmdp, addr);

	return (pte_val(READ_ONCE(*ptep)) & PTE_VALID);
}

// Return true if it is only changing prot of a pte.
static inline bool is_changing_pte_prot(pte_t *ptep, pte_t pte)
{
	if (((pte_val(*ptep) ^ pte_val(pte)) & PTE_ADDR_MASK) == 0)
		return true;
	else
		return false;
}

// Return true if the modify does not break DEP.
static inline bool check_pte_dep(char *addr, pte_t pte)
{
	// DEP for kernel code and readonly data
	// _text: .text start addr, __init_begin: .rodata end addr
	if (addr >= _stext && addr < _etext) {
		if ((PTE_WRITE & pte_val(pte)) // DBM == 1 --> writable
			|| !(PTE_RDONLY & pte_val(pte))) { // DBM == 0 && AP[2] = 0 --> writable
			panic("Can't make kernel's text/readonly page as writable!\n"
					   "addr = 0x%16llx, pte_val = 0x%16llx",
				  (u64)addr, pte_val(pte));
		}
	}
	return true;
}

// Return true if the modify does not break DEP.
static inline bool check_pmd_dep(char *addr, pmd_t pmd)
{
	// DEP for kernel code and readonly data
	// _text: .text start addr, __init_begin: .rodata end addr
	if (addr >= _stext && addr < _etext) {
		if ((PTE_WRITE & pmd_val(pmd)) || // DBM == 1 --> writable
			!(PTE_RDONLY & pmd_val(pmd))) { // DBM == 0 && AP[2] = 0 --> writable
			panic("Can't make kernel's text/readonly page as writable!\n"
					   "addr = 0x%16llx, pmd_val = 0x%16llx",
				  (u64)addr, pmd_val(pmd));
		}
	}
	return true;
}

void __iee_code _iee_set_static_pgd(int flag, pgd_t *pgdp, pgd_t pgd)
{
	if (haoc_enabled && (pgd_val(pgd) & PMD_TABLE_BIT) &&
			!check_addr_in_iee_valid(__phys_to_iee(__pgd_to_phys(pgd))))
		panic("You can't use non-iee-pgtable\n");

	WRITE_ONCE(*__ptr_to_iee(pgdp), pgd);
}

void __iee_code _iee_set_bm_pte(int flag, pte_t *ptep, pte_t pte)
{
	WRITE_ONCE(*__ptr_to_iee(ptep), pte);
}

void __iee_code _iee_set_pte(int flag, pte_t *ptep, pte_t pte)
{
	char *addr = (char *)__phys_to_kimg(__pte_to_phys(pte));

	if (!(pte_val(pte) & PTE_VALID)) {
		WRITE_ONCE(*__ptr_to_iee(ptep), pte);
		return;
	}

	// Avoid mapping a new VA to IEE PA.
	if (haoc_enabled && !is_changing_pte_prot(ptep, pte) &&
		check_addr_in_iee_valid(__phys_to_iee(__pte_to_phys(pte))))
		panic("You are remmaping IEE page to other VA.\n");

	// Avoid mapping a writable VA to kernel code PA.
	if (!check_pte_dep(addr, pte))
		return;

	WRITE_ONCE(*__ptr_to_iee(ptep), pte);
}

void __iee_code _iee_set_pmd(int flag, pmd_t *pmdp, pmd_t pmd)
{
	char *addr = (char *)__phys_to_kimg(__pmd_to_phys(pmd));

	if (!(pmd_val(pmd) & PMD_SECT_VALID)) {
		WRITE_ONCE(*__ptr_to_iee(pmdp), pmd);
		return;
	}

	// Check if the pte table is legally allocated.
	if (haoc_enabled && (pmd_val(pmd) & PMD_TABLE_BIT) &&
			!check_addr_in_iee_valid(__phys_to_iee(__pmd_to_phys(pmd))))
		panic("You can't use non-iee-pgtable\n");

	// Avoid mapping a huge pmd to IEE physical page.
	// if(!(pmd_val(pmd) & PMD_TABLE_BIT) && check_addr_range_in_iee_valid(pmd))
	//  panic("Mapping IEE physical page to a huge pmd.\n");

	if (!check_pmd_dep(addr, pmd))
		return;

	WRITE_ONCE(*__ptr_to_iee(pmdp), pmd);
}

void __iee_code _iee_set_pud(int flag, pud_t *pudp, pud_t pud)
{
	if (!(pud_val(pud) & PMD_SECT_VALID)) {
		WRITE_ONCE(*__ptr_to_iee(pudp), pud);
		return;
	}

	if (haoc_enabled && (pud_val(pud) & PMD_TABLE_BIT) &&
			!check_addr_in_iee_valid(__phys_to_iee(__pud_to_phys(pud))))
		panic("You can't use non-iee-pgtable\n");

	WRITE_ONCE(*__ptr_to_iee(pudp), pud);
}

void __iee_code _iee_set_p4d(int flag, p4d_t *p4dp, p4d_t p4d)
{
	if (!(p4d_val(p4d) & PMD_SECT_VALID)) {
		WRITE_ONCE(*__ptr_to_iee(p4dp), p4d);
		return;
	}

	if (haoc_enabled && (p4d_val(p4d) & PMD_TABLE_BIT) &&
			!check_addr_in_iee_valid(__phys_to_iee(__p4d_to_phys(p4d))))
		panic("You can't use non-iee-pgtable\n");

	WRITE_ONCE(*__ptr_to_iee(p4dp), p4d);
}

void __iee_code _iee_set_swapper_pgd(int flag, pgd_t *pgdp, pgd_t pgd)
{
	if (!(pgd_val(pgd) & PMD_SECT_VALID)) {
		WRITE_ONCE(*__ptr_to_iee(pgdp), pgd);
		return;
	}

	if (haoc_enabled && (pgd_val(pgd) & PMD_TABLE_BIT) &&
			!check_addr_in_iee_valid(__phys_to_iee(__pgd_to_phys(pgd))))
		panic("You can't use non-iee-pgtable\n");

	WRITE_ONCE(*__ptr_to_iee(pgdp), pgd);
}

pteval_t __iee_code _iee_set_xchg_relaxed(int flag, pte_t *ptep, pteval_t pteval)
{
	pteval_t ret = xchg_relaxed((pteval_t *)__ptr_to_iee(ptep), pteval);
	return ret;
}

pmdval_t __iee_code _iee_set_pmd_xchg_relaxed(int flag, pmd_t *pmdp,
							pmdval_t pmdval)
{
	pmdval_t ret = xchg_relaxed((pmdval_t *)__ptr_to_iee(pmdp), pmdval);
	return ret;
}

pteval_t __iee_code _iee_set_cmpxchg_relaxed(int flag, pte_t *ptep,
							pteval_t old_pteval, pteval_t new_pteval)
{
	pteval_t pteval = cmpxchg_relaxed((pteval_t *)__ptr_to_iee(ptep),
			old_pteval, new_pteval);

	return pteval;
}

pmdval_t __iee_code _iee_set_pmd_cmpxchg_relaxed(int flag, pmd_t *pmdp,
							pmdval_t old_pmdval, pmdval_t new_pmdval)
{
	pmdval_t pmdval = cmpxchg_relaxed((pmdval_t *)__ptr_to_iee(pmdp),
			old_pmdval, new_pmdval);

	return pmdval;
}

void __iee_code _iee_set_sensitive_pte(int flag, pte_t *lm_ptep, pte_t *iee_ptep,
						int order, int use_block_pmd, bool writable)
{
	int i;

	lm_ptep = __ptr_to_iee(lm_ptep);
	iee_ptep = __ptr_to_iee(iee_ptep);
	if (use_block_pmd) {
		#ifdef CONFIG_IEE_ALLOW_SPLIT_LM
		pmd_t pmd = __pmd(pte_val(READ_ONCE(*lm_ptep)));

		if (writable)
			pmd = __pmd((pmd_val(pmd) & ~PMD_SECT_RDONLY) | PTE_DBM);
		else
			pmd = __pmd((pmd_val(pmd) | PMD_SECT_RDONLY) & ~PTE_DBM);
		WRITE_ONCE(*lm_ptep, __pte(pmd_val(pmd)));
		#else
		if (order == IEE_DATA_ORDER) {
			pmd_t pmd = __pmd(pte_val(READ_ONCE(*lm_ptep)));

			if (writable)
				pmd = __pmd((pmd_val(pmd) & ~PMD_SECT_RDONLY) | PTE_DBM);
			else
				pmd = __pmd((pmd_val(pmd) | PMD_SECT_RDONLY) & ~PTE_DBM);
			WRITE_ONCE(*lm_ptep, __pte(pmd_val(pmd)));
		} else {
			/* Give up RO protection on linear mapping to avoid page table spliting. */
			// pr_err("IEE: give up RO protection to avoid page table split.");
		}
		#endif
		for (i = 0; i < (1 << order); i++) {
			pte_t pte = READ_ONCE(*iee_ptep);

			pte = __pte(pte_val(pte) | PTE_VALID);
			WRITE_ONCE(*iee_ptep, pte);
			iee_ptep++;
		}
	} else {
		for (i = 0; i < (1 << order); i++) {
			pte_t pte = READ_ONCE(*lm_ptep);

			if (writable)
				pte = __pte((pte_val(pte) & ~PTE_RDONLY) | PTE_DBM);
			else
				pte = __pte((pte_val(pte) | PTE_RDONLY) & ~PTE_DBM);
			WRITE_ONCE(*lm_ptep, pte);
			pte = READ_ONCE(*iee_ptep);
			pte = __pte(pte_val(pte) | PTE_VALID);
			WRITE_ONCE(*iee_ptep, pte);
			lm_ptep++;
			iee_ptep++;
		}
	}
}

void __iee_code _iee_unset_sensitive_pte(int flag, pte_t *lm_ptep, pte_t *iee_ptep,
						int order, int use_block_pmd)
{
	int i;

	lm_ptep = __ptr_to_iee(lm_ptep);
	iee_ptep = __ptr_to_iee(iee_ptep);
	if (use_block_pmd) {
		#ifdef CONFIG_IEE_ALLOW_SPLIT_LM
		pmd_t pmd = __pmd(pte_val(READ_ONCE(*lm_ptep)));

		pmd = __pmd(pmd_val(pmd) | PTE_DBM);
		WRITE_ONCE(*lm_ptep, __pte(pmd_val(pmd)));
		#else
		if (order == IEE_DATA_ORDER) {
			pmd_t pmd = __pmd(pte_val(READ_ONCE(*lm_ptep)));

			pmd = __pmd(pmd_val(pmd) | PTE_DBM);
			WRITE_ONCE(*lm_ptep, __pte(pmd_val(pmd)));
		} else {
			/* Give up RO protection on linear mapping to avoid page table spliting. */
			// pr_err("IEE: give up RO protection to avoid page table split.");
		}
		#endif
		for (i = 0; i < (1 << order); i++) {
			pte_t pte = READ_ONCE(*iee_ptep);

			pte = __pte(pte_val(pte) & ~PTE_VALID);
			WRITE_ONCE(*iee_ptep, pte);
			iee_ptep++;
		}
	} else {
		for (i = 0; i < (1 << order); i++) {
			pte_t pte = READ_ONCE(*lm_ptep);

			pte = __pte(pte_val(pte) | PTE_DBM);
			WRITE_ONCE(*lm_ptep, pte);
			pte = READ_ONCE(*iee_ptep);
			pte = __pte(pte_val(pte) & ~PTE_VALID);
			WRITE_ONCE(*iee_ptep, pte);
			lm_ptep++;
			iee_ptep++;
		}
	}
}
