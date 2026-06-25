// SPDX-License-Identifier: GPL-2.0
#include <linux/stdarg.h>
#include <linux/memblock.h>
#include <asm/pgalloc.h>
#include <asm/sysreg.h>
#include <asm/haoc/iee.h>
#include <asm/haoc/iee-asm.h>
#include <asm/haoc/iee-si.h>
#include <asm/haoc/iee-access.h>
#ifdef CONFIG_IEE_PTRP
#include <asm/haoc/iee-token.h>
#endif
#ifdef CONFIG_PTP
#include <asm/haoc/iee-mmu.h>
#endif

u64 __ro_after_init iee_si_reserved_pg_dir;

static inline unsigned long iee_si_mask(unsigned long mask,
					unsigned long new_val, unsigned long old_val)
{
	return (new_val & mask) | (old_val & ~mask);
}

#ifdef CONFIG_IEE_PTRP
static inline void iee_si_check_ttbr0(void)
{
	u64 old_ttbr0 = read_sysreg(ttbr0_el1);
	u64 old_phys = (old_ttbr0 & PAGE_MASK) & ~TTBR_ASID_MASK;
	struct task_token *token = (struct task_token *)iee_get_task_token(current);
	/* Phys in TTBR0 shall be the same with current->mm->pgd. */
	if (!(current == &init_task) && token->pgd
		&& old_phys != iee_si_reserved_pg_dir) {
		u64 token_phys = __pa(token->pgd);

		if (old_phys != token_phys)
			pr_err("IEE: Pgd set error. old ttbr0:%llx, token ttbr0:%llx",
				old_phys, token_phys);
	}
}
#else
static inline void iee_si_check_ttbr0(void) {}
#endif

unsigned long __iee_si_code iee_si_handler(int flag, ...)
{
	va_list pArgs;
	unsigned long old_val, new_val, ttbr1, ttbr0;

	va_start(pArgs, flag);
	switch (flag) {
	case IEE_SI_TEST:
		break;
	case IEE_SI_SET_SCTLR_EL1:
		old_val = read_sysreg(sctlr_el1);
		new_val = va_arg(pArgs, u64);
		new_val = iee_si_mask(IEE_SI_SCTLR_MASK, new_val, old_val);
		write_sysreg(new_val, sctlr_el1);
		break;
	case IEE_SI_SET_TTBR0:
		ttbr0 = va_arg(pArgs, u64) & ~TTBR_ASID_MASK;
		/* Skip checking before init IEE. */
		if (iee_init_done)
			iee_si_check_ttbr0();
		/* Load the reserved IEE ASID into TTBR0.*/
		if (haoc_enabled)
			ttbr0 |= FIELD_PREP(TTBR_ASID_MASK, IEE_ASID);
		write_sysreg(ttbr0, ttbr0_el1);
		break;
	case IEE_SI_CONTEXT_SWITCH:
		ttbr1 = va_arg(pArgs, u64);
		ttbr0 = va_arg(pArgs, u64) & ~TTBR_ASID_MASK;
		ttbr0 |= FIELD_PREP(TTBR_ASID_MASK, IEE_ASID);
		/* Skip checking before init IEE. */
		if (iee_init_done) {
			u64 new_asid = ttbr1 >> 48;
			/* ASID shall not be the one reserved for IEE. */
			if (new_asid == IEE_ASID)
				panic("IEE SI: Using reserved IEE ASID in TTRB0.");

			iee_si_check_ttbr0();
		}
		write_sysreg(ttbr1, ttbr1_el1);
		write_sysreg(ttbr0, ttbr0_el1);
		break;
	case IEE_SI_CONTEXT_SWITCH_PRE_INIT:
		ttbr1 = va_arg(pArgs, u64);
		ttbr0 = va_arg(pArgs, u64);

		if (iee_init_done)
			panic("IEE: Using legacy context switch after IEE init.");
		if (haoc_enabled)
			ttbr0 |= FIELD_PREP(TTBR_ASID_MASK, IEE_ASID);
		write_sysreg(ttbr1, ttbr1_el1);
		write_sysreg(ttbr0, ttbr0_el1);
		break;
	case IEE_SI_SET_VBAR:
		new_val = va_arg(pArgs, u64);
		write_sysreg(new_val, vbar_el1);
		break;
	case IEE_SI_SET_TCR_EL1:
		new_val = va_arg(pArgs, u64);
		old_val = read_sysreg(tcr_el1);
		new_val = iee_si_mask(IEE_SI_TCR_MASK, new_val, old_val);
		write_sysreg(new_val, tcr_el1);
		break;
	default:
		break;
	}
	va_end(pArgs);
	return 0;
}

static phys_addr_t __init iee_si_early_pgtable_alloc(void)
{
	phys_addr_t phys;

#ifdef CONFIG_PTP
	phys = early_iee_pgtable_alloc(0);
#else
	phys = memblock_phys_alloc_range(PAGE_SIZE, PAGE_SIZE, 0,
					 MEMBLOCK_ALLOC_NOLEAKTRACE);
#endif
	if (!phys)
		panic("Failed to allocate page table page\n");

	return phys;
}

static inline pgprot_t iee_pmd_pgprot(pmd_t pmd)
{
	unsigned long pfn = pmd_pfn(pmd);

	return __pgprot(pmd_val(pfn_pmd(pfn, __pgprot(0))) ^ pmd_val(pmd));
}

/* Split PMD block mappings so SI text can carry table/page permissions. */
static void __init iee_si_split_pmd_early(pud_t *pudp, unsigned long addr)
{
	pmd_t *pmdp = pmd_offset(pudp, addr);
	struct page *origin_page;
	phys_addr_t pte_phys;

	if (!pmd_leaf(*pmdp))
		return;

	if (pmd_val(*pmdp) & PTE_CONT) {
		pmd_t *cont_pmdp = pmd_offset(pudp, addr & CONT_PMD_MASK);
		int i;

		for (i = 0; i < CONT_PMDS; i++, cont_pmdp++)
			set_pmd(cont_pmdp, __pmd(pmd_val(*cont_pmdp) & ~PTE_CONT));
	}

	origin_page = pmd_page(*pmdp);
	pte_phys = iee_si_early_pgtable_alloc();
	if (!pte_phys)
		panic("Alloc pgtable error.\n");
	else {
		pte_t *ptep = __va(pte_phys);
		pgprot_t pgprot = iee_pmd_pgprot(*pmdp);
		int i;

#ifdef CONFIG_PTP
		iee_memset(ptep, 0, PAGE_SIZE);
#else
		memset(ptep, 0, PAGE_SIZE);
#endif
		for (i = 0; i < PTRS_PER_PMD; i++, ptep++) {
			pte_t entry;

			pgprot = __pgprot(pgprot_val(pgprot) | PTE_NG | PTE_TYPE_PAGE);
			entry = mk_pte(origin_page + i, pgprot);
			set_pte(ptep, entry);
		}
	}

	__pmd_populate(pmdp, pte_phys, PMD_TYPE_TABLE | PMD_TABLE_UXN);
}

static inline void iee_si_setup_data(void)
{
	iee_si_reserved_pg_dir = phys_to_ttbr(__pa_symbol(reserved_pg_dir));
}

/* Map iee si code PXNTable=1 on pmd page tables. */
static int __init iee_si_init_code(void)
{
	u64 addr = (u64)__iee_si_text_start;
	u64 end = (u64)__iee_si_text_end;
	pgd_t *pgdir = swapper_pg_dir;

	while (addr < end) {
		pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);
		p4d_t *p4dp = p4d_offset(pgdp, addr);
		pud_t *pudp = pud_offset(p4dp, addr);
		pmd_t *pmdp = pmd_offset(pudp, addr);
		pmd_t pmd = READ_ONCE(*pmdp);
		pte_t *ptep;
		pte_t pte;

		if ((pmd_val(pmd) & PMD_TYPE_MASK) == PMD_TYPE_SECT) {
			pr_info("IEE SI: code is on a pmd block. Splitting...");
			iee_si_split_pmd_early(pudp, addr);
			pmd = READ_ONCE(*pmdp);
		}

		pmd = __pmd(pmd_val(pmd) | PGD_PXNTABLE);
		set_pmd(pmdp, pmd);

		if ((pmd_val(pmd) & PMD_TYPE_MASK) == PMD_TYPE_TABLE) {
			ptep = pte_offset_kernel(pmdp, addr);
			pte = READ_ONCE(*ptep);

			if (pte_val(pte) & PTE_CONT) {
				pte_t *cont_ptep = pte_offset_kernel(pmdp,
								     addr & CONT_PTE_MASK);
				int i;

				for (i = 0; i < CONT_PTES; i++, cont_ptep++)
					set_pte(cont_ptep,
						__pte(pte_val(*cont_ptep) & ~PTE_CONT));
				pte = READ_ONCE(*ptep);
			}
			pte = __pte(pte_val(pte) | PTE_NG);
			set_pte(ptep, pte);
		}
		addr += PAGE_SIZE;
	}

	flush_tlb_all();
	return 1;
}

void __init iee_si_init(void)
{
	iee_si_setup_data();
	
	if (iee_si_init_code())
		pr_info("IEE: Sensitive instruction protection is ready.");
}
